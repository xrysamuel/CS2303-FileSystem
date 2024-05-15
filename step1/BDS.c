#include "std.h"
#include "socket.h"
#include "error_type.h"
#include "buffer.h"

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

    diskfile_fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    EXIT_IF(diskfile_fd < 0, , "Error: Could not open file '%s'.\n", filename);

    int result = lseek(diskfile_fd, filesize - 1, SEEK_SET);
    EXIT_IF(result < 0, close(diskfile_fd), "Error: Could not stretch file '%s'.\n", filename);

    result = write(diskfile_fd, "", 1);
    EXIT_IF(result != 1, close(diskfile_fd), "Error: Could not write last byte of file '%s'.\n", filename);

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

int diskfile_read(char **p_buffer, int *p_size, int cylinder, int sector)
{
    RET_ERR_IF(cylinder >= n_cylinders || sector >= n_sectors || cylinder < 0 || sector < 0, , INVALID_ARG_ERROR);

    RET_ERR_IF(diskfile == NULL, , DEFAULT_ERROR);

    long start = (cylinder * n_sectors + sector) * sector_size;
    char *internal_buffer = (char *)malloc(sector_size);
    EXIT_IF(internal_buffer == NULL, , "Error: Allocation failed.\n");

    sem_wait(&diskfile_mutex);
    arm_delay(disk_arm_loc, cylinder);
    disk_arm_loc = cylinder;
    printf("Read: cylinder = %d, sector = %d.\n", cylinder, sector);
    memcpy(internal_buffer, &diskfile[start], sector_size);
    sem_post(&diskfile_mutex);

    *p_buffer = internal_buffer;
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

int error_response(char **p_res_buffer, int *p_res_size)
{
    char str[] = "No";
    int result = str_to_buffer(str, p_res_buffer, p_res_size);
    RET_ERR_IF(IS_ERROR(result), , result);

    return *p_res_size;
}

int response(const char *req_buffer, int req_size, char **p_res_buffer, int *p_res_size)
{
    int result;
    if (starts_with(req_buffer, req_size, "I"))
    {
        char str[100];
        sprintf(str, "%d %d", n_cylinders, n_sectors);
        result = str_to_buffer(str, p_res_buffer, p_res_size);
        RET_ERR_IF(IS_ERROR(result), , error_response(p_res_buffer, p_res_size));

        return *p_res_size;
    }
    else if (starts_with(req_buffer, req_size, "R"))
    {
        int cylinder, sector;
        char *req_str = NULL;
        result = buffer_to_str(req_buffer, req_size, &req_str);
        RET_ERR_IF(IS_ERROR(result), , result);

        result = sscanf(req_str, "R %d %d", &cylinder, &sector);
        free(req_str);
        RET_ERR_IF(result != 2, , error_response(p_res_buffer, p_res_size));

        char *buffer;
        int size = 0;
        result = diskfile_read(&buffer, &size, cylinder, sector);
        RET_ERR_IF(IS_ERROR(result), , error_response(p_res_buffer, p_res_size));

        result = concat_buffer(p_res_buffer, p_res_size, "Yes ", 4, buffer, size);
        free(buffer);
        RET_ERR_IF(IS_ERROR(result), , result);

        return *p_res_size;
    }
    else if (starts_with(req_buffer, req_size, "W"))
    {
        char *req_str = (char *)malloc(req_size * sizeof(char));
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
        RET_ERR_IF(data_buffer == NULL, free(req_str), error_response(p_res_buffer, p_res_size));

        int cylinder, sector, len;
        result = sscanf(req_str, "W %d %d %d", &cylinder, &sector, &len);
        RET_ERR_IF(result != 3, free(req_str), error_response(p_res_buffer, p_res_size));
        RET_ERR_IF(len != data_buffer_size, free(req_str), error_response(p_res_buffer, p_res_size));

        result = diskfile_write(data_buffer, data_buffer_size, cylinder, sector);
        free(req_str);
        RET_ERR_IF(IS_ERROR(result), , error_response(p_res_buffer, p_res_size));

        char str[] = "Yes";
        int result = str_to_buffer(str, p_res_buffer, p_res_size);
        RET_ERR_IF(IS_ERROR(result), , result);

        return *p_res_size;
    }
    else
    {
        return error_response(p_res_buffer, p_res_size);
    }
}

void *worker(void *p_sockfd)
{
    int sockfd = *(int *)p_sockfd;
    int result = 0;
    signal(SIGPIPE, sigpipe_handler);
    while(true)
    {
        char *req_buffer = NULL;
        int req_buffer_size = 0;
        result = recv_message(sockfd, &req_buffer, &req_buffer_size);
        TEXIT_IF(IS_ERROR(result), , "Error: Could not receive message.\n");

        char *res_buffer = NULL;
        int res_buffer_size = 0;
        result = response(req_buffer, req_buffer_size, &res_buffer, &res_buffer_size);
        free(req_buffer);
        TEXIT_IF(IS_ERROR(result), , "Error: Response error.\n");

        result = send_message(sockfd, res_buffer, res_buffer_size);
        free(res_buffer);
        TEXIT_IF(IS_ERROR(result), , "Error: Could not send message.\n");
    }
    pthread_exit(NULL);
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
    EXIT_IF(n_cylinders <= 0 || n_sectors <= 0 || delay <= 0 || port <= 0, , "Numbers must be greater than 0.\n");

    int result;
    diskfile_init();
    int sockfd = create_socket();
    bind_socket(sockfd, port);
    listen_socket(sockfd);
    while (true)
    {
        int client_sockfd = wait_for_client(sockfd);
        pthread_t thread;
        result = pthread_create(&thread, NULL, worker, &client_sockfd);
        EXIT_IF(result != 0, close(sockfd), "Error: Failed to create thread.\n");        
    }
    diskfile_close();

    return 0;
}