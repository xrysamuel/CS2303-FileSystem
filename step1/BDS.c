#include "common.h"
#include "socket.h"
#include "error_type.h"
#include "buffer.h"
#include "server.h"

int diskfile_fd = -1;
char *diskfile = NULL;
sem_t diskfile_mutex;
int disk_arm_loc = 0;

char *filename;
int n_cylinders = 512;
int n_sectors = 16;
int delay = 20;

const int sector_size = 256;

void diskfile_init()
{
    long filesize = n_cylinders * n_sectors * sector_size;
    printf("Init: filesize = %ld bytes\n", filesize);

    // filename -> diskfile_fd
    diskfile_fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    EXIT_IF(diskfile_fd < 0, , "Error: Could not open file '%s'.\n", filename);

    // stretch diskfile_fd
    int result = lseek(diskfile_fd, filesize - 1, SEEK_SET);
    EXIT_IF(result < 0, close(diskfile_fd), "Error: Could not stretch file '%s'.\n", filename);

    // write to diskfile_fd
    result = write(diskfile_fd, "", 1);
    EXIT_IF(result != 1, close(diskfile_fd), "Error: Could not write last byte of file '%s'.\n", filename);

    // diskfile_fd -> diskfile
    diskfile = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, diskfile_fd, 0);
    EXIT_IF(diskfile == MAP_FAILED, close(diskfile_fd), "Error: Could not map file '%s'.\n", filename);

    sem_init(&diskfile_mutex, 0, 1);
    disk_arm_loc = 0;
    printf("Init: disk_arm_loc = cylinder 0.\n");
}

void diskfile_close()
{
    long filesize = n_cylinders * n_sectors * sector_size;
    sem_destroy(&diskfile_mutex);
    int result = munmap(diskfile, filesize);
    EXIT_IF(result < 0, close(diskfile_fd), "Error: Could not unmap file '%s'.\n", filename);

    close(diskfile_fd);
    diskfile = NULL;
}

void arm_delay(int cylinder_1, int cylinder_2)
{
    usleep(delay * abs(cylinder_1 - cylinder_2));
}

int diskfile_read(char *buffer, int *p_size, int max_size, int cylinder, int sector)
{
    RET_ERR_IF(cylinder >= n_cylinders || sector >= n_sectors || cylinder < 0 || sector < 0, , INVALID_ARG_ERROR);

    RET_ERR_IF(diskfile == NULL, , DEFAULT_ERROR);

    RET_ERR_IF(sector_size > max_size, , BUFFER_OVERFLOW);

    long start = (cylinder * n_sectors + sector) * sector_size;

    sem_wait(&diskfile_mutex);
    arm_delay(disk_arm_loc, cylinder);
    disk_arm_loc = cylinder;
    printf("Read: cylinder = %d, sector = %d.\n", cylinder, sector);
    memcpy(buffer, &diskfile[start], sector_size);
    sem_post(&diskfile_mutex);

    *p_size = sector_size;
    return sector_size;
}

int diskfile_write(char *buffer, int size, int cylinder, int sector)
{
    RET_ERR_IF(cylinder >= n_cylinders || sector >= n_sectors || cylinder < 0 || sector < 0 || size < 0, , INVALID_ARG_ERROR);

    RET_ERR_IF(diskfile == NULL, , DEFAULT_ERROR);

    long filesize = n_cylinders * n_sectors * sector_size;
    long start = (cylinder * n_sectors + sector) * sector_size;
    long end = start + size;
    RET_ERR_IF(end > filesize, , INVALID_ARG_ERROR);

    sem_wait(&diskfile_mutex);
    arm_delay(disk_arm_loc, cylinder);
    disk_arm_loc = cylinder;
    printf("Write: cylinder = %d, sector = %d.\n", cylinder, sector);
    memcpy(&diskfile[start], buffer, size);
    sem_post(&diskfile_mutex);
    return size;
}

int response(const char *req_buffer, int req_size, char *res_buffer, int *p_res_size, int max_res_size)
{
    int result;
    if (starts_with(req_buffer, req_size, "I"))
    {
        // str
        char str[20];
        snprintf(str, 20, "%d %d", n_cylinders, n_sectors);

        // str -> res_buffer
        result = str_to_buffer(str, res_buffer, p_res_size, max_res_size);
        RET_ERR_IF(IS_ERROR(result), , str_to_buffer("No", res_buffer, p_res_size, max_res_size));

        return *p_res_size;
    }
    else if (starts_with(req_buffer, req_size, "R"))
    {
        // req_buffer -> req_str
        char req_str[DEFAULT_BUFFER_CAPACITY];
        result = buffer_to_str(req_buffer, req_size, req_str, DEFAULT_BUFFER_CAPACITY);
        RET_ERR_RESULT(result); 

        // req_str -> cylinders, sector
        int cylinder, sector;
        result = sscanf(req_str, "R %d %d", &cylinder, &sector);
        RET_ERR_IF(result != 2, , str_to_buffer("No", res_buffer, p_res_size, max_res_size));

        // cylinders, sector -> buffer
        int size;
        char buffer[DEFAULT_BUFFER_CAPACITY];
        result = diskfile_read(buffer, &size, DEFAULT_BUFFER_CAPACITY, cylinder, sector);
        RET_ERR_IF(IS_ERROR(result), , str_to_buffer("No", res_buffer, p_res_size, max_res_size));

        // buffer -> res_buffer
        result = concat_buffer(res_buffer, p_res_size, max_res_size, "Yes ", 4, buffer, size);
        RET_ERR_RESULT(result); 

        return *p_res_size;
    }
    else if (starts_with(req_buffer, req_size, "W"))
    {
        // req_buffer -> req_str, data_buffer
        char *req_str = (char *)malloc(req_size * sizeof(char));
        RET_ERR_IF(req_str == NULL, , BAD_ALLOC_ERROR);
        memcpy(req_str, req_buffer, req_size);
        int space_count = 0;
        char *data_buffer = NULL;
        int data_buffer_size = 0;
        for (int i = 0; i < req_size; i++)
        {
            if (req_str[i] == ' ')
            {
                space_count++;
                if (space_count == 4)
                {
                    req_str[i] = '\0';
                    data_buffer = &req_str[i + 1];
                    data_buffer_size = req_size - i - 1;
                }
            }
        }
        RET_ERR_IF(data_buffer == NULL, free(req_str), str_to_buffer("No", res_buffer, p_res_size, max_res_size));

        // req_str -> cylinder, sector, len
        int cylinder, sector, len;
        result = sscanf(req_str, "W %d %d %d", &cylinder, &sector, &len);
        RET_ERR_IF(result != 3, free(req_str), str_to_buffer("No", res_buffer, p_res_size, max_res_size));
        RET_ERR_IF(len != data_buffer_size, free(req_str), str_to_buffer("No", res_buffer, p_res_size, max_res_size));

        // data_buffer, cylinder, sector -> diskfile_write()
        result = diskfile_write(data_buffer, data_buffer_size, cylinder, sector);
        free(req_str);
        RET_ERR_IF(IS_ERROR(result), , str_to_buffer("No", res_buffer, p_res_size, max_res_size));

        return str_to_buffer("Yes", res_buffer, p_res_size, max_res_size);
    }
    else
    {
        return str_to_buffer("No", res_buffer, p_res_size, max_res_size);
    }
}

int main(int argc, char *argv[])
{
    EXIT_IF(argc != 6, , "Usage: %s <disk filename> <#cylinders> <#sector per cylinder> <track-to-track delay> <#port>\n", argv[0]);

    filename = argv[1];
    n_cylinders = atoi(argv[2]);
    n_sectors = atoi(argv[3]);
    delay = atoi(argv[4]);
    int port = atoi(argv[5]);

    EXIT_IF(filename == NULL || filename[0] == '\0', , "Error: Invalid filename.\n");
    EXIT_IF(n_cylinders <= 0 || n_sectors <= 0 || delay <= 0 || port <= 0, , "Error: Invalid arguments.\n");

    diskfile_init();
    simple_server(port, response);
    diskfile_close();

    return 0;
}