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
drwxrwxrwx     0     0       1024 May 30 23:49 ..
drwxrwxrwx     0     0       1024 May 30 23:49 .
drwxrwxrwx     0     0        512 May 30 23:49 home
-rwxr-xr-x     0     0          0 May 30 23:49 passwd

> mk file
Success.
> mkdir dir
Success.
> ls
drwxrwxrwx     0     0       1536 May 30 23:49 ..
drwxrwxrwx     0     0       1536 May 30 23:49 .
drwxrwxrwx     0     0        512 May 30 23:49 home
-rwxr-xr-x     0     0          0 May 30 23:49 passwd
-rwxr-xr-x     0     0          0 May 30 23:49 file
drwxr-xr-x     0     0        512 May 30 23:49 dir

> rmdir dir
Success.
> w file 10 0123456789
Success.
> i file 3 3 abc
Success.
> d file 8 2
Success.
> cat file
012abc34789
> rm file
Success.
> ls
drwxrwxrwx     0     0       1024 May 30 23:49 ..
drwxrwxrwx     0     0       1024 May 30 23:49 .
drwxrwxrwx     0     0        512 May 30 23:49 home
-rwxr-xr-x     0     0          0 May 30 23:49 passwd

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
> mkdir layer1
Success.
> cd layer1

> mkdir layer2
Success.
> cd layer2

> mkdir layer3
Success.
> cd layer3

> cd ../../..

> ls
drwxrwxrwx     0     0       1280 May 30 23:50 ..
drwxrwxrwx     0     0       1280 May 30 23:50 .
drwxrwxrwx     0     0        512 May 30 23:49 home
-rwxr-xr-x     0     0          0 May 30 23:49 passwd
drwxr-xr-x     0     0        768 May 30 23:50 layer1

> cd /layer1/layer2

> ls
drwxr-xr-x     0     0        768 May 30 23:50 ..
drwxr-xr-x     0     0        768 May 30 23:50 .
drwxr-xr-x     0     0        512 May 30 23:50 layer3

> cd /

> ls
drwxrwxrwx     0     0       1280 May 30 23:50 ..
drwxrwxrwx     0     0       1280 May 30 23:50 .
drwxrwxrwx     0     0        512 May 30 23:49 home
-rwxr-xr-x     0     0          0 May 30 23:49 passwd
drwxr-xr-x     0     0        768 May 30 23:50 layer1

> cd layer1/layer2

> ls
drwxr-xr-x     0     0        768 May 30 23:51 ..
drwxr-xr-x     0     0        768 May 30 23:50 .
drwxr-xr-x     0     0        512 May 30 23:50 layer3
```

### 3.2 Support for multi-users

First, we Creates a new account with the username "samuel" and the password "samuelpassword". Then, we switch to the account "samuel" (using the "cacc" command too, which creates the account if it doesn't exist and switches to the corresponding account if it does). We create a file called "samuel-file" under account "samuel". Then, we switch to account "bob". We can find that account "bob" cannot write to "file-uid-1", because he hasn't write permission for "samuel-file".

```
> f

> ls
drwxrwxrwx     0     0       1024 May 30 23:51 ..
drwxrwxrwx     0     0       1024 May 30 23:51 .
drwxrwxrwx     0     0        512 May 30 23:51 home
-rwxr-xr-x     0     0          0 May 30 23:51 passwd

> cacc root rootpassword
New account created.
> cacc root rootpassword
Change to existed account.
> cacc samuel samuelpassword
New account created.
> cacc samuel samuelpassword
Change to existed account.
> mk samuel-file         
Success.
> ls
drwxrwxrwx     0     0       1280 May 30 23:52 ..
drwxrwxrwx     0     0       1280 May 30 23:52 .
drwxrwxrwx     0     0        512 May 30 23:51 home
-rwxr-xr-x     0     0       1016 May 30 23:51 passwd
-rwxr-xr-x     0     1          0 May 30 23:52 samuel-file

> cacc bob bobpassword
New account created.
> cacc bob bobpassword
Change to existed account.
> w samuel-file 5 abcde
Error: Permission denied.
> cacc samuel wrongpassword
Error: Permission denied.
> cacc samuel samuelpassword
Change to existed account.
> w samuel-file 5 abcde
Success.
> cat samuel-file
abcde
```

Here, we create a temporary account "temp-acc", make a file and change its mode to "511" (i.e. "rwxrwxrwx"), and finally we remove the account "temp-acc". After deleting a user, the UID (user identifier) will not be reclaimed. In order to protect the data, the files and folders belonging to that user will not be deleted. The UID 0 (root user) has full permissions for these files and folders, so there is no need to worry that they will become inaccessible.

```
> cacc temp-acc temppassword
New account created.
> cacc temp-acc temppassword
Change to existed account.
> mk tempfile
Success.
> ls
drwxrwxrwx     0     0       1536 May 31 00:01 ..
drwxrwxrwx     0     0       1536 May 31 00:01 .
drwxrwxrwx     0     0        512 May 30 23:51 home
-rwxr-xr-x     0     0       2032 May 30 23:58 passwd
-rwxr-xr-x     0     1          5 May 30 23:53 samuel-file
-rwxr-xr-x     0     3          0 May 31 00:01 tempfile

> chmod tempfile 511
Success.
> ls
drwxrwxrwx     0     0       1536 May 31 00:01 ..
drwxrwxrwx     0     0       1536 May 31 00:01 .
drwxrwxrwx     0     0        512 May 30 23:51 home
-rwxr-xr-x     0     0       2032 May 30 23:58 passwd
-rwxr-xr-x     0     1          5 May 30 23:53 samuel-file
-rwxrwxrwx     0     3          0 May 31 00:01 tempfile

> rmacc temp-acc wrongpassword
Error: Permission denied.
> rmacc temp-acc temppassword
Account removed.
```

Now, two clients are accessing the file system simultaneously. The following two sessions are synchronized to demonstrate that two users can perform file operations simultaneously. Here we can also see that Bob is unable to create files within the directory created by Samuel ("/home/samuel").

```
> cacc samuel samuelpassword
Change to existed account.
> ls
drwxrwxrwx     0     0       1536 May 31 00:06 ..
drwxrwxrwx     0     0       1536 May 31 00:06 .
drwxrwxrwx     0     0        512 May 30 23:51 home
-rwxr-xr-x     0     0       2032 May 31 00:01 passwd
-rwxr-xr-x     0     1          5 May 30 23:53 samuel-file
-rwxrwxrwx     0     3          0 May 31 00:01 tempfile

> cd home

> mkdir samuel
Success.
> cd /home/samuel

> mk samuelfile        
Success.
> ls
drwxrwxrwx     0     0        768 May 31 00:06 ..
drwxr-xr-x     0     1        768 May 31 00:07 .
-rwxr-xr-x     0     1          0 May 31 00:07 samuelfile

```

```
> cacc bob bobpassword
Change to existed account.
> ls
drwxrwxrwx     0     0       1536 May 31 00:07 ..
drwxrwxrwx     0     0       1536 May 31 00:07 .
drwxrwxrwx     0     0        768 May 31 00:06 home
-rwxr-xr-x     0     0       2032 May 31 00:01 passwd
-rwxr-xr-x     0     1          5 May 30 23:53 samuel-file
-rwxrwxrwx     0     3          0 May 31 00:01 tempfile

> cd home

> mkdir bob 
Success.
> cd /home/bob

> mk bobfile
Success.
> ls
drwxrwxrwx     0     0       1024 May 31 00:07 ..
drwxr-xr-x     0     2        768 May 31 00:08 .
-rwxr-xr-x     0     2          0 May 31 00:08 bobfile

> cd /home/samuel

> mk bobfile
Error: Permission denied.
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
> cd /no-such-dir/
Error: Not found.
```

It is not possible to test errors that occur under extreme conditions, such as disk full, read/write errors, etc.

For further information, please see the [report](doc/Report.md).