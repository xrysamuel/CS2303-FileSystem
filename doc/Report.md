# Report

## 1 Overview

This project implements a ext2-like file system consisting of three components. The first component is the Basic Disk Server (BDS), responsible for simulating a hard disk, and the Basic Disk Client (BDC) tests the functionality of BDS. The second component is the File Server (FS), responsible for managing the file system. The third component is the File Client (FC), which provides an interface for users to interact with the file system. These three components are loosely coupled and connected through sockets.

Key features of this project include:
- Unified error handling mechanism to enhance robustness.
- A separate component for socket connenction (designed from scratch, without reference to the given template), allowing BDS, FS, and FC to abstract away the underlying communication details.
- Minimizing disk I/O:
    - Only necessary data are loaded to memory.
    - Caching mechanism on the FS side to reduce disk access, utilizing the LRU approximation algorithm for cache replacement.
    - Historical information recording of the bitmap to minimize disk access and expedite bitmap read/write.
- Permission control for multi-users.
- Mutex locks to prevent data conflicts during read/write.

## 2 TCP Socket

In this project, BDC serves as the server for FS, while FS serves as the server for FC. We design a unified mechanism to manage TCP connections and communication effectively.

During TCP communication, packet fragmentation, known as the "sticky packet" issue, may occur due to the stream-oriented nature of TCP. For instance, if one side sends "123" followed by "4567," the receiving end may read "12345." To address this, we need to establish message boundaries. The first approach is terminating each message with a unique character. However, this method cannot be applied to our project as our protocol may use all ASCII characters in the message (for same reason, we cannot use C-style string in our project). Alternatively, the project adopts the second method which involves transmitting the message length followed by the message content to identify the message boundaries.

Given that all three components in this project follow the Read-Evaluate-Print-Loop (REPL) pattern, where the server handles evaluation and the client manages reading and printing, we can design two independent components "simple_server" and "simple_client" to perform this loop jointly as well as handle the TCP communication between them. These two functions take the function pointer as parameters, so that the server can only focus on the evaluation logic and the client focus on the reading and printing aspects and simply pass the logic as callback function to the "simple_server" and "simple_client". This approach ensures code reusability and mitigates potential errors that may arise from dealing with communication intricacies.


## 3 Basic Disk Server

The Basic Disk Server (BDS) emulates a physical disk. It treats a file as a disk and partitions it into multiple cylinders, which are further divided into sectors.

The BDS is capable of handling three types of requests:
- `I`: Information request. The disk provides two integers representing the disk's geometry: the number of cylinders and the number of sectors per cylinder.
- `R <#cylinder> <#sector>`: Read request for a specific sector. The disk responds with "Yes" followed by a whitespace and the 256-byte information if the block exists, or "No" if the block does not exist or error occurs.
- `W <#cylinder> <#sector> <#len> <data>`: Write request for a sector. The disk responds with "Yes" and writes the data to the specified sector if it is a valid write request, or "No" otherwise.

The BDS also simulates the delay caused by head movement, where the delay is proportional to the difference in accessing cylinders. To prevent conflicts, a mutual exclusion lock is implemented for read and write operations.

It is important to note that the "data" field may contain ASCII 0 characters when processing `W` request. Therefore, parsing methods based on C-style strings such as `sscanf` are not suitable here. Instead, the data field must be manually separated by spaces.

## 4 File Server

### 4.1 Configuration

Our file system divides the hard disk into several spaces, including super block, block bitmap, inode bitmap, inode tables and data blocks. This division is consistent with the division of a block group in EXT2.
- Superblock: The superblock provides essential information about the file system, such as the block sizes and overall file system statistics. In our configuration, it occupies one block, i.e. 256 bytes.
- Block Bitmap: The block bitmap is responsible for tracking the allocation status of data blocks within the file system. It consists of a series of bits, with each bit representing the availability of a corresponding data block. In our configuration, the block bitmap spans 16384 blocks.
- Inode Bitmap: Similar to the block bitmap, the inode bitmap tracks the allocation status of inodes within the file system. Inodes store metadata about files and directories, such as permissions, timestamps, and file sizes. The inode bitmap spans 16 blocks in this configuration.
- Inode Table: The inode table contains the actual inode structures, each representing a file or directory. It stores the metadata associated with each file or directory, allowing for efficient access and management. In this configuration, each partition contains 32768 inodes at most.
- Data Blocks: The data blocks hold the actual file data, including the contents of files and directories. The size of the data blocks determines the maximum disk size supported by the file system. In this configuration, the number of data blocks is limited to less than 33,554,432, i.e. 8GB.

![](media/partition.drawio.svg)

In a file system, the inode and superblock are crucial data structures for organizing and managing files and directories. Then, we will explore the format of these structures in our file system.

Let's begin with the format of the inode. The inode (index node) contains metadata about a file or directory. Here is the format of the inode structure:

```c
struct inode_t
{
    u_int16_t mode;          // type and permission
    u_int16_t uid;           // user id
    u_int32_t size;          // size in bytes, max size
    u_int32_t atime;         // last access time (in POSIX time)
    u_int32_t ctime;         // creation time (in POSIX time)
    u_int32_t mtime;         // last modification time (in POSIX time)
    u_int32_t dtime;         // deletion time (in POSIX time)
    u_int32_t block_ptr[12]; // direct block pointer
    u_int32_t sblock_ptr;    // singly indirect block pointer
    u_int32_t dblock_ptr;    // doubly indirect block pointer
    u_int32_t tblock_ptr;    // triply indirect block pointer
    u_int16_t gid;           // group id
    char reserved[170];
};
```

The inode structure contains various fields such as mode (indicating the type and permissions of the file/directory), uid (owner's user ID), size (file size in bytes), atime (last access time), ctime (creation time), mtime (last modification time), dtime (deletion time), block_ptr (direct block pointers), sblock_ptr (singly indirect block pointer), dblock_ptr (doubly indirect block pointer), tblock_ptr (triply indirect block pointer), gid (owner's group ID), and reserved (unused space).

The inode has 12 direct block pointers that directly point to data blocks. The file system assigns data blocks to these pointers to allocate space for the file. For file sizes exceeding the capacity of the direct blocks, the file system employs indirect blocks. The inode's indirect block pointers (singly, doubly, or triply) point to blocks that contain multiple pointers. Each pointer within these blocks points to another block that, in turn, contains additional pointers pointing to data blocks.

The allocation process for these indirect blocks is similar across all levels. The file system checks the previous level's capacity and, if necessary, assigns a new block for the indirect block pointer. It then initializes the block pointers within the allocated block. The file system continues this allocation process by assigning new blocks for the pointers within the indirect blocks, ensuring data blocks are allocated until the required space is obtained.

The max file size supported by this inode and data block structure is (12 + 64 + 64 * 64 + 64 * 64 * 64) * 256 Bytes = 65 MB .

The superblock stores essential information about the file system. Here is the format of the superblock structure:

```c
struct superblock_t
{
    u_int32_t block_size;       // fixed as 256
    u_int32_t n_free_inodes;    // unallocated inodes
    u_int32_t n_free_blocks;    // unallocated data blocks
    u_int32_t block_bitmap_ptr; // data block bitmap pointer
    u_int32_t inode_bitmap_ptr; // inode block bitmap pointer
    u_int32_t inode_table_ptr;  // inode table pointer
    u_int32_t data_blocks_ptr;  // data blocks pointer
    char formatted;             // whether this partition has been formatted
    char reserved[227];
};
```

The superblock structure contains fields such as block_size (indicating the fixed block size, which is 256 in our configuration), n_free_inodes (the count of unallocated inodes), n_free_blocks (the count of unallocated data blocks), block_bitmap_ptr (pointer to the data block bitmap), inode_bitmap_ptr (pointer to the inode block bitmap), inode_table_ptr (pointer to the inode table), data_blocks_ptr (pointer to the data blocks), formatted (a flag indicating whether the disk has been formatted), and reserved (unused space).

### 4.2 Program structure

File Server (FS) adopts a layered program structure consisting of four layers:
- disk: This layer offers a fundamental abstraction layer for the read and write interfaces in the basic disk server, incorporating a caching mechanism.
- blocks: This layer provides interfaces for block and inode allocation and deallocation, as well as read and write operations for blocks.
- inodes: This layer offers manipulation interfaces for inode files.
- fs: This layer provides manipulation interfaces for the file tree, encompassing operations such as writing to a file, reading from a file, creating files or directories, and removing files or directories.

Refer to the figure below for a detailed illustration:

![](media/filesystem.drawio.svg)

Each layer utilizes the interface of the layer below it, providing a progressively increasing level of abstraction.

### 4.3 Disk layer

In the Disk layer, a caching mechanism is employed for disk_write and disk_read operations. Specifically, a cache of size CACHE_SIZE is maintained as a global variable in the disk layer. Each cache block stores a specific block from a disk, and an auxiliary array indicates which sector's block is stored in each cache block.

When a disk_write request is issued, the cache is first searched. If the block is found, the contents of the buffer are directly written into the corresponding cache block. If the block is not found, a victim cache block is selected from the entire cache. Its contents are written back to the hard disk, and then the block we want to write is read into that position in the cache block, followed by writing to that cache block.

The processing strategy for disk_read requests is similar.

The cache replacement strategy used is an approximate LRU algorithm called "second chance." Specifically, the cache is maintained as a circular linked list, and a victim pointer is used to implement the second chance cache replacement algorithm. The second chance cache replacement algorithm, also known as the clock algorithm, is a modified version of the least recently used (LRU) algorithm. It operates based on the concept of a circular linked list and a "reference" or "second chance" bit associated with each cache block. Here are the steps of the second chance cache replacement algorithm:
1. Initialize a victim pointer to point to the first cache block in the circular linked list.
2. When a cache block needs to be replaced, examine the reference bit of the current victim block:
    - If the reference bit is 0, it means the block has not been accessed recently and can be chosen as the victim block for replacement.
    - If the reference bit is 1, it means the block has been accessed recently. Give it a "second chance" by setting its reference bit to 0 and move the victim pointer to the next cache block.
3. Repeat step 2 until a cache block with a reference bit of 0 is found. This block becomes the victim block for replacement.
4. Write back the contents of the victim block to the disk if it has been modified.
5. Replace the victim block with the new block requested, updating its contents and reference bit.
6. Move the victim pointer to the next cache block in the circular linked list.

By using the second chance algorithm, the cache can prioritize blocks that have not been accessed recently for replacement, while giving a chance for recently accessed blocks to remain in the cache. This helps to improve cache hit rates and overall performance by keeping frequently accessed data in the cache.

Let's conduct a simple experiment. We execute three commands consecutively on our file system:

```
> ls
> mk file
> w file 3 abc
```

To execute these commands, a total of 95 read and write requests are sent from the upper layers to the disk layer, with "r/w XXX" indicating operating on the requested sector:

```
r 0, r 16401, r 16401, r 49169, r 49170, r 49171, r 49172, r 49173, r 49177, w 16401, r 16401, r 16401, r 16402, r 16403, r 16404, r 16407, r 16401, w 16401, r 16401, w 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 16401, r 16401, r 16401, r 49169, r 49170, r 49171, r 49172, r 49173, r 49177, w 16401, r 16401, r 16401, r 16402, r 16403, r 16404, r 16407, r 16385, r 16385, w 16385, w 16408, r 16401, r 1, r 1, w 1, w 16401, r 16401, w 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 49190, w 16401, r 16401, r 16401, r 49169, r 49170, r 49171, r 49172, r 49173, r 49177, r 49190, w 16401, r 16401, r 16401, r 16402, r 16403, r 16404, r 16407, r 16408, r 16408, r 1, r 1, w 1, w 16408, r 16408, w 49192, w 16408, r 16401, w 16401, r 16401, w 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 49190, w 16401
```

Thanks to the presence of the caching mechanism, the actual sequence of read and write requests sent from the disk to the BDS (Block Device Service) is:

```
r 0, r 16401, r 49170, r 49171, r 49172, r 49173, r 49177, r 49177, r 16402, r 16403, r 16404, r 16407, r 16385, r 16408, r 1, r 49192, w 16401, r 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 16402, w 16403, w 16404, w 16407, w 16385, w 16408, w 1, w 49190, w 49192
```

It only contains 32 requests. We can see that the caching mechanism significantly reduces the number of requests to the BDS.

### 4.4 Block layer

The Block layer controls the allocation and read/write operations of inode blocks and data blocks, by reading and writing of the superblock, inode block bitmap, and data block bitmap. Each bit in the bitmap corresponds to whether a block is allocated, with 1 indicating allocated and 0 indicating unallocated. Each inode block and data block is identified by a globally unique inode_id and block_id, respectively.

When the upper layer sends a request to the Block layer for data block allocation, the Block layer searches for the first 0 bit in the data block bitmap and sets it to 1. Obviously, starting from the first bit of the bitmap each time results in a significant amount of disk I/O. Therefore, we need an additional variable to indicate the block where the first possible 0 may occur, record the block calculated when searching for the first 0 bit, and dynamically update it when setting or clearing bits.

Furthermore, we aim to avoid fragmentation, which means allocating files in contiguous spaces as much as possible. Assuming that allocation requests from approximately the same time period are likely to come from the same file, we want the aforementioned variable to point to a bitmap block where there are many consecutive 0 bits following it. Therefore, when finding the first 0 bit in the data block bitmap, we need to search for a few more bits afterwards to ensure that the number of subsequent 0 bits reaches a threshold if possible.

### 4.5 Inodes layer

In addition to providing interfaces for creating or destroying inodes, the Inodes layer offers three interfaces for manipulating inode files (files or directories). The first is resizing the inode file, which entails allocating and deallocating data blocks directly or indirectly pointed to by the inode. The second reads a specified number of characters from the inode file, starting from a given position. The third overwrites a specified number of characters in the inode file, starting from a given position. These three interfaces enable more complex read and write operations on files, such as insertion and deletion.

It is important to note that when performing read and write operations on inode files, the entire file is not loaded into memory. Only the necessary blocks for reading and writing are loaded.

The most difficult aspect of implementing these three interfaces is determining the actual block_id of a given data block within a file. This involves traversing indirect data block pointers from single level to triple level. Hence, an intermediary variable type called visit_path_t is employed in the project. True to its name, it contains the path to access the actual data block entries. This type comprises a path type and three entries, resembling a clock, which enables locating a specific second based on a.m./p.m., hours, minutes, and seconds. For example, the visit path of 4368th data block of some certain inode file is (tblock_ptr, 0, 3, 4).

By introducing this intermediary variable type, not only can convenient block position conversions be achieved, but also deep-first traversal and reverse deep-first traversal of data blocks can be easily implemented. This facilitates the allocation or deallocation of data blocks when resizing the inode file. For instance, after deallocating 4263rd data block (tblock_ptr, 0, 1, 0), we need to deallocate (tblock_ptr, 0, 1) and when allocating 4172nd data bloc (tblock_ptr, 0, 0, 0), we need to first allocate (tblock_ptr), (tblock_ptr, 0), (tblock_ptr, 0, 0). We find this process follows a depth-first traversal, and the visit path records the "parent node" of the data block to be accessed, which simplifies this process.

### 4.6 FS layer

The FS layer is the topmost layer of the File Server, responsible for translating actual commands, such as `ls` or `cd`, into a series of operations on inode files and providing feedback to the users. Each command is executed with an attached context, where each client corresponds to a specific context. The context stores information about the client's working directory, UID, and GID.

At this layer, file locks for read and write operations are implemented. Multiple users cannot simultaneously perform read and write operations on the same file. Additionally, permission management is enforced, as different commands require different read and write permissions. For example, when a user issues a write (`w`) command, the associated context is checked to determine if the user is the owner of the file or belongs to the same group as the file. Based on the file's mode, it is determined whether the user has the authority to write to the file. Similarly, when a user issues a `cd` command, it is verified if the user has access rights to the target directory. Furthermore, when a user issues an `mk` command, it is checked if the user has access rights to the working directory. By default, the created file is readable and writable by the owner, and readable by other users.

The FS layer can perform the following file operations:
- `f`: Format. This command formats the file system on the disk by initializing the superblock and bitmap that the file system relies on. In addition, the system will create a root directory (inode_id = 0), as well as a \home directory and a passwd file to manage user data.
- `mk <filename>`: Create file. This command creates a file with the specified name in the file system.
- `mkdir <dirname>`: Create directory. This command creates a subdirectory with the specified name in the current directory.
- `rm <filename>`: Delete file. This command deletes the file with the specified name from the current directory.
- `cd <path>`: Change directory. This command changes the current working directory to the specified path. The path follows the format used in Linux and can be either a relative or absolute path, for example, "/path/to/target", "path/to/target", "/path/to/target/", or "path/to/target/". When the file system starts, the initial working path is set to "/". It also supports the use of "." and "..".
- `rmdir <dirname>`: Delete directory. This command deletes the directory with the specified name from the current directory.
- `ls`: Directory listing. This command returns a listing of files and directories in the current directory, along with additional information such as file size and last modified time.
- `cat <filename>`: Read file. This command reads the contents of the file with the specified name and returns the data from it.
- `w <filename> <#len> <data>`: Write file. This command overwrites the contents of the file with the specified name with the given data, which should be of length len. If the new data is longer than the existing data in the file, the file will be extended. If the new data is shorter, the file will be truncated.
- `i <filename> <#pos> <#len> <data>`: Insert into file. This command inserts the specified data, of length len, into the file at the position before the character at index pos (0-indexed). If pos exceeds the size of the file, the data will be inserted at the end of the file.
- `d <filename> <#pos> <#len>`: Delete from file. This command deletes contents from the file starting from the character at index pos (0-indexed) up to len bytes or until the end of the file.

Since the data in the file is stored contiguously, the `i` command saves all the data after the specified insertion position, changes the file size, writes the data to be inserted after that position, and finally appends the saved data at the end. The `d` command works in a similar way by moving the subsequent data to the front and adjusting the file size accordingly.

Furthermore, to achieve the function of file system, we have designed a directory tree structure. Specifically, a directory is a special inode file containing multiple entries that point to subdirectories and files within the directory. The structure of each entry is as follows:

```c
struct dir_entry_t
{
    char name[MAX_NAME_LEN];
    int inode_id;
};
```

Each entry stores the name and inode ID of a subdirectory or file. It is important to note that subdirectories and files do not store their own names.

When implementing the `cd` command, we can leverage the tree-like nature of directories by using a recursive approach. For example, to access "path/to/target," we can first change to the "path" directory and then recursively call `cd`, this time targeting "to/target."

Additionally, it should be noted that we treat ".." and "." as two actual directory entries, equivalent to having a pointer pointing to the parent node in a tree data structure. If we don't do this, accessing the parent directory would become exceptionally difficult since we would need to traverse from the root directory each time.


### 4.7 Multi-users

The permission management model in this project is fully consistent with Linux. Specifically, our permissions are represented by `rwxrwxrwx`, where each character has the following meanings:
- The first three characters represent the owner's permissions: `r` for read, `w` for write, and `x` for execute.
- The second three characters represent the group's permissions.
- The third three characters represent permissions for other users.

Each type of permission corresponds to a uint16 integer, which is stored in the mode field of the inode.

To support multiple users, we provide following command:
- `cacc <account> <password>`: Change account. If the user account does not exist, it will be created. If the account already exists, the current session will switch to the specified user.
- `rmacc <account> <password>`: Remove account. This command deletes a user account.
- `chmod <filename> <#mode>`: Change mode. This command changes the mode of the file with the specified name.

By default, when a user creates a file or folder, the permissions are set to `rwxr-xr-w`, which means that other users cannot modify the file or folder.

To implement the above functionalities, we have designed a special directory called "passwd" (inode_id = 1) in the root directory. As the name suggests, its function is similar to `/etc/passwd` in Linux. It stores information about each user in the form of entries. The structure of each entry is as follows:

```c
struct acc_entry_t
{
    char name[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int uid; // < 0 means invalid
    int gid;
};
```

Each user's UID should correspond to their position in the "passwd" file. When deleting a user, their entry in the file is not removed but their UID is set to a negative value. This is done to prevent the reuse of UIDs and potential privacy issues if a new user is assigned the same UID as a previously deleted user. Additionally, there is no need to worry about the files becoming inaccessible after deleting a user since the root user (UID = 0) always has the permissions to modify the mode of all files.


## 5 Error Handling

Previously mentioned, this project utilizes a unified error handling approach to reduce debugging complexity and enhance system robustness.

All functions in the project return an integer value representing an error code. All errors are propagated through return values. Specifically, the project categorizes errors into the following types:

```c
#define SUCCESS 0
#define DEFAULT_ERROR -1
// unrecoverable fault
#define READ_ERROR -2
#define WRITE_ERROR -3
#define INVALID_ARG_ERROR -4
#define BAD_ALLOC_ERROR -5
#define BUFFER_OVERFLOW -6

// file system errors
#define DISK_FULL_ERROR -7
#define PERMISSION_DENIED -8
#define NOT_FOUND -9
#define ALREADY_EXISTS -10
```

When an error occurs within a function, if the function cannot handle the error, it must forward or re-interpret the error to the caller, propagating it downward. However, if the function can handle the error, it falls into several scenarios:
- In the case of an unrecoverable error, but the current function is capable of performing global cleanup actions, releasing all global system resources, it terminates the entire process or thread. For example, `simple_client` encounters a connection interruption (`READ_ERROR` or `WRITE_ERROR`).
- In the case of a recoverable error, the error is processed within the function, and it does not propagate further.
- If the current function is responsible for generating feedback to the user, it can generate an appropriate error message based on the error semantics. For instance, functions in FS Layer encounters `PERMISSION_DENIED` error.

Furthermore, the project employs several macros:

```c
// Returns the specified return value if the given condition is true, performs necessary cleanup actions.
#define RET_ERR_IF(condition, cleanup, returnval)
// Exits the program with a failure status if the given condition is true, performs necessary cleanup actions, and prints a formatted error message.
#define EXIT_IF(condition, cleanup, format, ...)
// Terminates the current thread if the given condition is true, performs necessary cleanup actions, and prints a formatted error message.
#define TEXIT_IF(condition, cleanup, format, ...)
```

By utilizing these macros, when a function returns an error value or terminates the entire thread or process, it can print an error message, facilitating debugging. For example, when the TCP Server exits, it will print the error propagation path within the TCP Client:

```
utils/socket.c(96): return -3
utils/client.c(12): return -3
Error: Could not get response.
utils/client.c(52): Broken pipe
```

## Appendix

### A.1 Interfaces

All interfaces concerning socket communication are listed as follow:
```c
// socket
int create_socket();
void bind_socket(int sockfd, uint16_t port);
void listen_socket(int sockfd);
int wait_for_client(int sockfd);
void connect_to_server(int sockfd, const char* ip, uint16_t port);
int send_message(int sockfd, const char *buffer, int size);
int recv_message(int sockfd, char *buffer, int *p_size, int max_size);
void sigpipe_handler(int sig);

// server
typedef int (*response_t)(int sockfd, const char *req_buffer, int req_size, char *res_buffer, int *p_res_size, int max_res_size);
int simple_server(int port, response_t response);

// client
typedef int (*get_request_t)(char *req_buffer, int *req_size, int max_req_size, int cycle);
typedef int (*handle_response_t)(const char *res_buffer, int res_size, int cycle);
void simple_client(const char *server_ip, int port, get_request_t get_request, handle_response_t handle_response);
void custom_client_init(const char *server_ip, int port, response_t *response);
void custom_client_close();
```

For all function or interface declarations of File Server (FS), refer to the following:
```c
// disk
// Initializes the disk server with the specified server IP and port.
void disk_init(const char *server_ip, int port); 
 // Closes the disk server.
void disk_close();
// Retrieves the total number of blocks available in the disk.
void get_n_blocks(int *p_n_blocks);
// Reads the contents of the specified block from the disk into the provided buffer. Returns an error code.
int disk_read(char buffer[BLOCK_SIZE], int block); 
// Writes the contents of the provided buffer to the specified block on the disk. Returns an error code.
int disk_write(const char buffer[BLOCK_SIZE], int block);

// blocks
// Closes the blocks layer.
void blocks_close();
// Formats the blocks layer for initialization.
int blocks_format();
// Initializes the blocks layer with the specified server IP and port.
void blocks_init(const char *server_ip, int port);
// Deallocates the specified inode. Returns an error code.
int deallocate_inode(int inode_id);
// Allocates a new inode and assigns its ID to the provided pointer. Returns an error code.
int allocate_inode(int *inode_id);
// Reads the specified inode from storage into the provided inode structure. Returns an error code.
int read_inode(int inode_id, struct inode_t *inode);
 // Writes the provided inode to storage. Returns an error code.
int write_inode(int inode_id, const struct inode_t *inode);
// Deallocates the specified block. Returns an error code.
int deallocate_block(int block_id);
// Allocates a new block and assigns its ID to the provided pointer. Returns an error code.
int allocate_block(int *block_id);
// Reads the contents of the specified block from storage into the provided block buffer. Returns an error code.
int read_block(int block_id, char block[BLOCK_SIZE]);
// Writes the contents of the provided block buffer to the specified block in storage. Returns an error code.
int write_block(int block_id, const char block[BLOCK_SIZE]); 

// inodes
// Closes the inodes layer.
void inodes_close(); 
// Initializes the inodes layer with the specified server IP and port.
void inodes_init(const char *server_ip, int port); 
// Formats the inodes layer for initialization.
int inodes_format(); 
// Creates a new inode with the specified mode, user ID, and group ID. Assigns the new inode's ID to the provided pointer. Returns an error code.
int create_inode(int *inode_id, u_int16_t mode, u_int16_t uid, u_int16_t gid); 
// Deletes the specified inode. Returns an error code.
int delete_inode(int inode_id); 
// Resizes the file associated with the specified inode to the specified size. Returns an error code.
int inode_file_resize(int inode_id, int size);
// Reads data from the file associated with the specified inode starting from the specified position and stores it in the provided buffer. Returns an error code.
int inode_file_read(int inode_id, char *buffer, int start, int size); 
// Writes the provided data buffer to the file associated with the specified inode starting from the specified position. Returns an error code.
int inode_file_write(int inode_id, const char *buffer, int start, int size); 
// Retrieves the specified inode from storage and stores it in the provided inode structure. Returns an error code.
int get_inode(int inode_id, struct inode_t *inode); 

// fs
struct dir_entry_t {
    char name[MAX_NAME_LEN];
    int inode_id;
};

enum auth_t {
    READ_AUTH,
    WRITE_AUTH,
    EXE_AUTH,
    NO_AUTH
};
struct response_arg_t {
    struct context_t *p_context;
    char *res_buffer;
    int *p_res_size;
    int max_size;
    const char *req_buffer;
    int req_size;
};

typedef int (*fs_op_t)(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **entries);

// Wraps and executes the specified file system operation with the provided arguments and authorization level. Returns an error code.
int fs_operation_wrapper(fs_op_t fs_op, struct response_arg_t arg, enum auth_t auth);
// Lists the directory entries and write to the arg.res_buffer. Returns an error code.
int list(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **entries);
// Creates a new file with name specified in arg.req_buffer and updates the directory entries. Returns an error code.
int make_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);
// Removes the file with name specified in arg.req_buffer and updates the directory entries. Returns an error code.
int remove_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);
// Creates a new directory with name specified in arg.req_buffer and updates the directory entries. Returns an error code.
int make_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);
// Removes the specified directory with name specified in arg.req_buffer and updates the directory entries. Returns an error code.
int remove_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries); 
// Changes the current directory to the specified directory with path specified in arg.req_buffer. Returns an error code.
int change_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries); 
// Retrieves the specified file using the name specified in arg.req_buffer. Returns an error code.
int catch_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries); 
// Writes data to the specified file using name specified in arg.req_buffer. Returns an error code.
int write_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries); 
// Inserts data into the specified file using name specified in arg.req_buffer. Returns an error code.
int insert_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries); 
// Deletes data from the specified file using name specified in arg.req_buffer. Returns an error code.
int delete_in_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries); 

int change_account(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int remove_account(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);
```