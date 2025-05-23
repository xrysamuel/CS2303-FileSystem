# Report

## 1 Overview

This project delivers an ext2-like network file system for educational purposes built upon three key components. The **Basic Disk Server (BDS)** simulates a hard disk, with the **Basic Disk Client (BDC)** providing behavioral tests for the BDS. The **File Server (FS)** manages the file system itself, while the **File Client (FC)** offers users an interface to interact with it. These components are loosely coupled and communicate efficiently via sockets.

Key features of this project include:

- **Robust error handling** through a unified mechanism.
- A custom **socket connection library**.
- **Optimized disk I/O**, achieved by:
    - Loading only essential data into memory.
    - Implementing a caching mechanism in the FS to reduce disk access, using an LRU approximation algorithm for cache replacement.
    - Maintaining bitmap search history to further minimize disk access and expedite bitmap read/write operations.
- **Permission controls** for enhanced security.
- **Mutex locks** to prevent data conflicts during read/write operations.

This report will delve into the design details of the file system.

## 2 Socket

We've designed a unified mechanism to effectively manage socket connections.

TCP's streaming nature can lead to **"sticky packet"** issues, where message boundaries are lost (e.g., "123" followed by "4567" might be received as "12345"). To solve this, our project transmits message content with **a length header** to clearly define message boundaries. This approach is necessary because all ASCII characters, including null terminators, can be part of the message, making C-style string termination methods unsuitable.

## 3 Basic Disk Server

The Basic Disk Server (BDS) functions as a virtual hard disk. It treats a file as a disk and divides it into multiple **cylinders**, which are further divided into **sectors**.

The BDS handles three types of requests:

- `I`: Information request. It provides two integers representing the disk's geometry: the number of cylinders and sectors per cylinder.
- `R <#cylinder> <#sector>`: Read request for a specific sector. The server responds with "Yes" followed by a whitespace and 256 bytes of data if the block exists, or "No" if the block is absent or en error occurs.
- `W <#cylinder> <#sector> <#len> <data>`: Write request for a sector. Writes data to a specified sector. The server responds with "Yes" if the write is valid and proceeds; otherwise, it responds with "No."

The BDS simulates **head movement delay**, with the delay proportional to the difference in cylinder numbers. A mutual exclusion lock is implemented to prevent conflicts during read and write operations.

It's crucial to note that the `W` request's "data" field can contain `\0` characters. Therefore, parsing methods that rely on C-style strings, like `sscanf`, are unsuitable. The data field must be manually separated by spaces.

## 4 File Server

### 4.1 Configuration

Our file system partitions the hard disk into several sections, including super block, block bitmap, inode bitmap, inode tables and data blocks, similar to the ext2 file system.

- **Superblock:** This single-block (256 bytes) section records vital file system information, including block sizes and overall statistics.
- **Block Bitmap:** Spanning 16,384 blocks, this bitmap tracks the allocation status of data blocks, with each bit representing a data block's availability.
- **Inode Bitmap:** Similar to the block bitmap, this 16-block section tracks the allocation status of inodes.
- **Inode Table:** This table contains the actual inode structures, each representing a file or directory. It stores metadata for efficient access and management. Each partition supports a maximum of 32,768 inodes.
- **Data Blocks:** These blocks hold the actual file data. The number of data blocks is limited to less than 33,554,432, allowing for a maximum of 8 GB of storage.

Our file system supports a disk space range **from 20 MB to 8 GB**. You can adjust these configuration values by modifying the macro definitions in `fsconfig.h`. For instance, to support more inodes, you'd increase the space allocated for the inode bitmap and inode table.

![](media/partition.drawio.svg)

Inodes and the superblock serve as pivotal data structures for organizing files and directories. Each inode encapsulates file or directory metadata, structured as follows:

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

This structure includes fields for file type/permissions (`mode`), owner IDs (`uid`, `gid`), size, various timestamps (`atime`, `ctime`, `mtime`, `dtime`), and pointers to data blocks.

The inode utilizes **12 direct block** pointers for immediate data access. For larger files, it employs **singly, doubly, and triply indirect block pointers**. These indirect blocks contain pointers to other blocks, which in turn point to data blocks, enabling the file system to handle files exceeding direct block capacity.

Space allocation involves adding new pointers to the current indirect block, allocating a new indirect block if the current one is full. Deallocation reverses this process. The maximum file size supported by this structure is (12 + 64 + 64 * 64 + 64 * 64 * 64) * 256 Bytes = 65 MB.

The superblock preserves essential file system details. Its format is:

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

This includes the fixed block size (256 bytes), counts of free inodes and data blocks, pointers to various bitmaps and tables, a format flag, and reserved space.

### 4.2 Program structure

The File Server (FS) employs **a layered program structure** with four levels of abstraction:

- Disk Layer: Abstracts basic disk read/write, incorporating a caching system for speed.
- Blocks Layer: Manages block and inode allocation/deallocation, and block read/write.
- Inodes Layer: Provides interfaces for manipulating inode files.
- FS Layer: Offers high-level file tree manipulation (read, write, create, delete files/directories).

Please refer to the diagram below for a detailed illustration:

![](media/filesystem.drawio.svg)

Each layer builds upon the interfaces of the layer below it, fostering **modular development**, **maintainability**, and **extensibility**.

### 4.3 Disk layer

In the disk layer, a caching mechanism is employed to optimize `disk_write` and `disk_read` operations. Specifically, a cache of size `CACHE_SIZE` is maintained within the disk layer. Each cache block stores a specific block from the disk, and an auxiliary array indicates tracks which sector's block is in each cache block.

When a `disk_write` request is issued, the cache is first checked. If the block is found, it's updated. If the block is not found, a victim cache block is chosen from the cache and its contents are written back to the disk, and then cleared to load the desired block.

`disk_read` requests follow a similar caching strategy.

The cache replacement strategy is an **approximate LRU algorithm** called "second chance" algorithm. This algorithm uses a circular linked list structure, a victim pointer, and "second chance" bit associated with each cache block. Here are the steps of the second chance cache replacement algorithm:

1. A victim pointer starts at the first cache block in the circular linked list.
2. When a cache block needs to be replaced, examine the reference bit of the current victim block:
    - If the reference bit is 0, it means the block has not been accessed recently and can be chosen as the victim block for replacement.
    - If the reference bit is 1, it means the block has been accessed recently. Give it a "second chance" by setting its reference bit to 0 and move the victim pointer to the next cache block.
3. Repeat step 2 until a cache block with a reference bit of 0 is found. This block becomes the victim block for replacement.
4. Write back the contents of the victim block to the disk if it has been modified.
5. Replace the victim block with the new block requested, updating its contents and reference bit.
6. The victim pointer advances to the next block in the circular list.

This "second chance" approach prioritizes less recently accessed blocks for replacement, improving cache hit rates and overall performance.

Let's conduct a simple experiment. Consider executing these commands:

```
> ls
> mk file
> w file 3 abc
```

Without caching, these would result in **95 distinct read/write requests** from upper layers to the disk layer.

```
r 0, r 16401, r 16401, r 49169, r 49170, r 49171, r 49172, r 49173, r 49177, w 16401, r 16401, r 16401, r 16402, r 16403, r 16404, r 16407, r 16401, w 16401, r 16401, w 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 16401, r 16401, r 16401, r 49169, r 49170, r 49171, r 49172, r 49173, r 49177, w 16401, r 16401, r 16401, r 16402, r 16403, r 16404, r 16407, r 16385, r 16385, w 16385, w 16408, r 16401, r 1, r 1, w 1, w 16401, r 16401, w 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 49190, w 16401, r 16401, r 16401, r 49169, r 49170, r 49171, r 49172, r 49173, r 49177, r 49190, w 16401, r 16401, r 16401, r 16402, r 16403, r 16404, r 16407, r 16408, r 16408, r 1, r 1, w 1, w 16408, r 16408, w 49192, w 16408, r 16401, w 16401, r 16401, w 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 49190, w 16401
```

Thanks to the caching mechanism, the actual requests sent from the disk layer to the BDS are significantly fewer: only **32 requests**.

```
r 0, r 16401, r 49170, r 49171, r 49172, r 49173, r 49177, r 49177, r 16402, r 16403, r 16404, r 16407, r 16385, r 16408, r 1, r 49192, w 16401, r 49169, w 49170, w 49171, w 49172, w 49173, w 49177, w 16402, w 16403, w 16404, w 16407, w 16385, w 16408, w 1, w 49190, w 49192
```

This demonstrates how caching drastically reduces I/O to the BDS.

### 4.4 Block layer

The Block layer manages inode and data block allocation/deallocation and their read/write operations by interacting with the superblock and block bitmaps. Each bit in the bitmap indicates a block's allocation status (1 for allocated, 0 for unallocated). Each inode block and data block is identified by a globally unique `inode_id` and `block_id`, respectively.

When allocating a data block, the Block layer searches for the first 0 bit in the data block bitmap and sets it to 1. To avoid excessive disk I/O from repeated full bitmap scans, we maintain a variable tracking the last known location of a potential 0 bit, dynamically updating it during bit operations.

We also aim to minimize fragmentation, which means allocating files in contiguous spaces as much as possible. Assuming that proximate allocation requests are often for the same file, we optimize the search for the first 0 bit. The system looks for a subsequent sequence of 0 bits that meets a certain threshold, ensuring that newly allocated blocks are as contiguous as possible.

### 4.5 Inodes layer

Beyond inode creation/destruction, the Inodes layer provides three key interfaces for manipulating inode files (files or directories):

- Resizing: Allocates/deallocates data blocks directly or indirectly pointed to by the inode.
- Reading: Reads a specified number of characters from an inode file, starting at a given position.
- Overwriting: Overwrites a specified number of characters in the inode file, starting at a given position.

These interfaces enable complex file operations like insertion and deletion, without loading the entire file into memory; only necessary blocks are loaded.

A significant challenge is determining the actual block_id for a given data block within a file, requiring traversal of indirect data block pointers from single level to triple level. To simplify this, an intermediary variable type, `visit_path_t`, is employed. True to its name, it contains the path to access the actual data block entries, resembling a clock's hour, minute, and second hand to pinpoint a specific second. For example, the 4368th data block's visit path might be (tblock_ptr, 0, 3, 4).

This intermediary variable type not only facilitates block position conversions but also streamlines depth-first traversal and reverse depth-first traversal of data blocks. This is crucial for allocating or deallocating data blocks during file resizing. For instance, deallocating 4263rd data block (tblock_ptr, 0, 1, 0) implies deallocating (tblock_ptr, 0, 1). Similarly, allocating 4172nd data block (tblock_ptr, 0, 0, 0), requires allocating (tblock_ptr), (tblock_ptr, 0), and (tblock_ptr, 0, 0).

### 4.6 FS layer

The FS layer is the topmost layer of the File Server, responsible for translating actual commands, such as `ls` or `cd`, into a series of operations on inode files and providing feedback to the users. Each command is executed with an attached context. The context stores client's working directory, UID, and GID.

This layer implements file locks to prevent concurrent read/write operations on the same file and enforces permission management. Different commands require different read and write permissions. For example, write (`w`) command checks if the user is the file owner or in the same group, and if the file's mode allows writing. `cd` command verifies if the user has access to the target directory, and `mk` command checks if the user has access rights to the working directory. By default, the created file is readable and writable by the owner, and readable by other users (i.e. `rwxr-xr-x`).

The FS layer supports the following requests:

- `f`: Format. Initializes the file system (superblock, bitmaps), creates the root directory (inode_id = 0), `/home` directory, and a `passwd` file for user data.
- `mk <filename>`: Creates a file in the current directory.
- `mkdir <dirname>`: Creates a subdirectory with the specified name in the current directory.
- `rm <filename>`: Deletes a file from the current directory.
- `cd <path>`: Changes the current working directory to the specified path. Paths can be either a **relative or absolute** path, for example, "/path/to/target", "path/to/target", "/path/to/target/", or "path/to/target/". When the file system starts, the initial working path is set to "/". It also supports "." and "..".
- `rmdir <dirname>`: Deletes a directory.
- `ls`: Lists files and directories in the current directory with size and modification time.
- `cat <filename>`: Reads and returns file contents.
- `w <filename> <#len> <data>`: Overwrites file contents with the specified name with the given data, which should be of length `len`. **Extends or truncates the file as needed.**
- `i <filename> <#pos> <#len> <data>`: Inserts data into a file at `pos`. If `pos` exceeds file size, data is appended.
- `d <filename> <#pos> <#len>`: Deletes contents from a file starting at `pos` (0-indexed) up to `len` bytes or until the end of the file.

Since the data in the file is stored contiguously, the `i` command saves all the data after the specified insertion position, changes the file size, writes the data to be inserted after that position, and finally appends the saved data at the end. The `d` command works in a similar way by moving the subsequent data to the front and adjusting the file size accordingly.

Directory operations (`mk`, `mkdir`, `rm`, `cd`, `rmdir`, `ls`) utilize a directory tree structure. A directory is a special inode file containing multiple entries that point to subdirectories and files within the directory. The structure of each entry is as follows:

```c
struct dir_entry_t
{
    char name[MAX_NAME_LEN];
    int inode_id;
};
```

Each entry contains a subdirectory or file's name and its corresponding inode ID. Notably, the files and subdirectories themselves do not store their names within their own structures.

The `cd` command leverages the tree-like nature of directories by using a recursive approach. For example, to navigating to "path/to/target," we first change to the "path" directory and then recursively call `cd`, this time targeting "to/target."

Crucially, ".." and "." are treated as two actual directory entries, acting as pointers to the current and parent directories. If we don't do this, accessing the parent directory would become exceptionally difficult since we would need to traverse from the root directory each time.


### 4.7 Multi-users

The permission management model is fully consistent with Linux's `rwxrwxrwx` scheme:

- The first three characters represent the owner's permissions: `r` for read, `w` for write, and `x` for execute.
- The second three characters represent the group's permissions.
- The third three characters represent permissions for other users.

Each type of permission corresponds to a `uint16` integer stored in the mode field of the inode.

Multi-user support is provided via these commands:

- `cacc <account> <password>`: Changes to an existing account. If the user account does not exist, it will be created.
- `rmacc <account> <password>`: Removes an account.
- `chmod <filename> <#mode>`: Changes a file's mode.

By default, when a user creates a file or folder, the permissions are set to `rwxr-xr-x`, preventing modification by other users.

A special `passwd` directory (inode_id = 1) in the root mimics Linux's `/etc/passwd`, storing user information as `acc_entry_t` entries:

```c
struct acc_entry_t
{
    char name[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int uid; // < 0 means invalid
    int gid;
};
```

Each user's UID corresponds to their position in the `passwd` file. When a user is deleted, their entry isn't removed; their uid is set to a negative value. This prevents UID reuse and potential privacy issues. File accessibility after user deletion isn't a concern, as the root user (UID = 0) always has permission to modify any file's mode.

## 5 Error Handling and Robustness

Previously mentioned, this project utilizes a **unified error handling approach** to simplify debugging and bolster system robustness.

All functions in the project return an integer error code. All errors are propagated through return values. Errors are categorized as follows:

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

When a function encounters an error it cannot handle, it propagates the error or re-raise an error to its caller. If a function can handle an error, the response varies:

- **Unrecoverable errors with global cleanup:** If the function can perform global cleanup (e.g., releasing system resources), it terminates the entire process or thread. For example, `simple_client` handles a connection interruption (`READ_ERROR` or `WRITE_ERROR`) this way.
- **Recoverable error:** the error is processed within the function, and not propagated further.
- **User feedback:** If the function is responsible for providing user feedback, it generates an appropriate error message based on the error type (e.g., an FS Layer function encountering `PERMISSION_DENIED`).

The project also uses several macros for structured error management:

```c
// Returns the specified return value if the given condition is true, performs necessary cleanup actions.
#define RET_ERR_IF(condition, cleanup, returnval)
// Exits the program with a failure status if the given condition is true, performs necessary cleanup actions, and prints a formatted error message.
#define EXIT_IF(condition, cleanup, format, ...)
// Terminates the current thread if the given condition is true, performs necessary cleanup actions, and prints a formatted error message.
#define TEXIT_IF(condition, cleanup, format, ...)
```

These macros enable functions to print informative error messages when returning an error or terminating a thread/process, greatly **aiding debugging**. For example, a TCP server exit might show the TCP client's error propagation path:


```
utils/socket.c(96): return -3
utils/client.c(12): return -3
Error: Could not get response.
utils/client.c(52): Broken pipe
```

To further enhance robustness, we address buffer overflows, a common C programming vulnerability. Internal interfaces consistently pass both the buffer pointer and its size/capacity parameters. We prioritize safer C library functions like `strncmp` and `snprintf`. When a safe version isn't available, we implement custom secure solutions (e.g., for `sscanf`).

## Appendix

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