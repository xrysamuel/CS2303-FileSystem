# CS2303 File System

## 1 Compile

```
make
```

## 2 Run

Run basic disk server:

```
./build/BDS diskfile.bin 400 400 20 10000
```

Run basic disk client, command line based:

```
./build/BDC_command 127.0.0.1 10000
```

Run basic disk client, random data client:

```
./build/BDC_random 127.0.0.1 10000
```

Run file system:

```
./build/FS 127.0.0.1 10000 10001
```

Run file system client:

```
./build/FC 127.0.0.1 10001
```

## 3 Test Run

### 3.1 Basic operations

File reading and writing, file or directory reading and writing, file or directory deletion:

```
> f

> ls
drwxrwxrwx     0     0       1024 May 30 16:30 ..
drwxrwxrwx     0     0       1024 May 30 16:30 .
drwxrwxrwx     0     0        512 May 30 16:30 home
-rwxr-xr-x     0     0          0 May 30 16:30 passwd

> mk file

> mkdir dir

> ls
drwxrwxrwx     0     0       1536 May 30 16:31 ..
drwxrwxrwx     0     0       1536 May 30 16:31 .
drwxrwxrwx     0     0        512 May 30 16:30 home
-rwxr-xr-x     0     0          0 May 30 16:30 passwd
-rwxr-xr-x     0     0          0 May 30 16:30 file
drwxr-xr-x     0     0        512 May 30 16:31 dir

> rmdir dir

> w file 10 0123456789

> i file 3 3 abc

> d file 8 2

> cat file
012abc34789
> rm file

> ls    
drwxrwxrwx     0     0       1024 May 30 16:32 ..
drwxrwxrwx     0     0       1024 May 30 16:32 .
drwxrwxrwx     0     0        512 May 30 16:30 home
-rwxr-xr-x     0     0          0 May 30 16:30 passwd

> e
```

The meaning of each field in "ls" is similar to the meaning of fields in "ls -al" in a Linux system. The first field indicates whether the item is a directory and provides permission information. The second field indicates the group ID (gid) of the file. The third field indicates the user ID (uid) of the file. The fourth field indicates the file size. The fifth field indicates the last modification information of the file. The sixth field indicates the file name.

```
drwxrwxrwx     0     0       1536 May 30 17:15 ..
drwxrwxrwx     0     0       1536 May 30 17:15 .
drwxrwxrwx     0     0       1536 May 30 17:14 home
-rwxr-xr-x     0     0          8 May 30 17:08 passwd
drwxr-xr-x     0     1        768 May 30 17:09 layer1
-rwxr-xr-x     0     0         32 May 30 17:15 file
```

Change directory:

```
> ls        
drwxrwxrwx     0     0       1280 May 30 16:40 ..
drwxrwxrwx     0     0       1280 May 30 16:40 .
drwxrwxrwx     0     0       1024 May 30 16:40 home
-rwxr-xr-x     0     0          4 May 30 16:34 passwd
drwxr-xr-x     0     1        768 May 30 16:40 layer1

> cd layer1

> ls
drwxrwxrwx     0     0       1280 May 30 16:41 ..
drwxr-xr-x     0     1        768 May 30 16:40 .
drwxr-xr-x     0     1        768 May 30 16:40 layer2

> cd layer2

> ls
drwxr-xr-x     0     1        768 May 30 16:41 ..
drwxr-xr-x     0     1        768 May 30 16:40 .
drwxr-xr-x     0     1        512 May 30 16:40 layer3

> cd layer3

> ls
drwxr-xr-x     0     1        768 May 30 16:41 ..
drwxr-xr-x     0     1        512 May 30 16:40 .

> cd ../../..

> ls
drwxrwxrwx     0     0       1280 May 30 16:41 ..
drwxrwxrwx     0     0       1280 May 30 16:41 .
drwxrwxrwx     0     0       1024 May 30 16:40 home
-rwxr-xr-x     0     0          4 May 30 16:34 passwd
drwxr-xr-x     0     1        768 May 30 16:41 layer1

> cd /layer1/layer2

> ls
drwxr-xr-x     0     1        768 May 30 17:05 ..
drwxr-xr-x     0     1        768 May 30 17:03 .
drwxr-xr-x     0     1        512 May 30 17:03 layer3

> cd / 

> ls  
drwxrwxrwx     0     0       1280 May 30 17:05 ..
drwxrwxrwx     0     0       1280 May 30 17:05 .
drwxrwxrwx     0     0       1024 May 30 16:40 home
-rwxr-xr-x     0     0          4 May 30 16:34 passwd
drwxr-xr-x     0     1        768 May 30 17:05 layer1

> cd layer1/layer2

> ls
drwxr-xr-x     0     1        768 May 30 17:05 ..
drwxr-xr-x     0     1        768 May 30 17:05 .
drwxr-xr-x     0     1        512 May 30 17:03 layer3

> 
```

### 3.2 Support for multi-users

First, we create an account with uid=1. Then, we switch to the account with uid=1 (using the "cacc" command, which creates the account if it doesn't exist and switches to the corresponding account if it does). During the creation process, a folder called "/home/home-1" is created in the home directory. We create a file called "file-uid-1" under account 1. Then, we switch to account 0. We find that account 0 cannot write to "file-uid-1" or modify "/home/home-1".

```
> cacc 1
Created new account.
> cacc 1
Changed to existed account.
> mk file-uid-1

> ls
drwxrwxrwx     0     0       1280 May 30 16:35 ..
drwxrwxrwx     0     0       1280 May 30 16:35 .
drwxrwxrwx     0     0       1024 May 30 16:35 home
-rwxr-xr-x     0     0          4 May 30 16:34 passwd
-rwxr-xr-x     0     1          0 May 30 16:35 file-uid-1

> cacc 0
Changed to existed account.
> w file-uid-1 5 abcde
Error: Permission denied.
> cd home

> ls
drwxrwxrwx     0     0       1280 May 30 16:35 ..
drwxrwxrwx     0     0       1024 May 30 16:35 .
drwxr-xr-x     0     0        512 May 30 16:34 home-0
drwxr-xr-x     0     1        512 May 30 16:34 home-1

> cd home-1

> mkdir dir
Error: Permission denied.
```

Now, two clients are accessing the file system simultaneously. The following two sessions are synchronized to demonstrate that two users can perform file operations simultaneously.

```
> cacc 3
Created new account.
> cacc 3
Changed to existed account.
> cd /home/home-3

> ls
drwxrwxrwx     0     0       1536 May 30 17:10 ..
drwxr-xr-x     0     3        512 May 30 17:08 .

> mk file-3

> ls
drwxrwxrwx     0     0       1536 May 30 17:10 ..
drwxr-xr-x     0     3        768 May 30 17:11 .
-rwxr-xr-x     0     3          0 May 30 17:11 file-3

> 
```

```
> cacc 2
Created new account.
> cacc 2
Changed to existed account.
> cd /home/home-2

> ls
drwxrwxrwx     0     0       1536 May 30 17:10 ..
drwxr-xr-x     0     2        512 May 30 17:08 .

> mk file-2

> ls
drwxrwxrwx     0     0       1536 May 30 17:10 ..
drwxr-xr-x     0     2        768 May 30 17:10 .
-rwxr-xr-x     0     2          0 May 30 17:10 file-2

> 
```

### 3.3 Error handling

The following demonstrates some of the supported error handling:

```
> no-such-command
Error: Wrong format.
> rm no-such-file  
Error: Not found.
> mkdir home
Error: Already exists.
> w file 1000 length doesn't match
Error.
> w file here supposed to be a number                 
Error.
> cd /layer1/layer2/no-such-dir
Error: Not found.
```

It is not possible to test errors that occur under extreme conditions, such as disk full, read/write errors, etc.

For further information, please see the [report](doc/Report.md).