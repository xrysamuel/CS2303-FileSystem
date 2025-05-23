# CS2303 File System

This project is a toy-version, from-scratch implementation of an **ext2-like network file system** developed in C. It was created as a course assignment and is ideal for educational purposes, helping students understand the internal workings of a file system. For an in-depth look at its design and features, please refer to the [full report](doc/Report.md).

## 1 Getting started

To get started with the file system, follow these steps:

**Compile the project:**

```bash
make
```

**Run the basic disk server:**

```bash
./build/BDS diskfile.bin 400 400 20 10000
```

**Run the command-line disk client:**

```bash
./build/BDC_command 127.0.0.1 10000
```

**Run the disk client with randomly generated commands for testing:**

```bash
./build/BDC_random 127.0.0.1 10000
```

**Start the file system server:**

```bash
./build/FS 127.0.0.1 10000 10001
```

**Run the file system client:**

```bash
./build/FC 127.0.0.1 10001
```

## 2 Usage

### 2.1 Basic operations

Make file, make folder, remove file, remove folder, insert / delete characters in file:

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

List:

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

### 2.2 Multi-users

First, we create and switch to a new account named "samuel" with the password "samuelpassword" using the `cacc` command. This command is versatile: it creates the account if it doesn't exist or switches to it if it does. Once logged in as "samuel," we create a file called "samuel-file."

Next, we switch to the "bob" account. Since "bob" lacks write permissions for "samuel-file," any attempt by Bob to write to it will fail.

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

In another scenario, we create a temporary account, "temp-acc," make a file, and change its mode to "511". Finally, we remove "temp-acc." It's important to note that when a user is deleted, their UID (user identifier) is not reclaimed. To safeguard data, files and folders belonging to the deleted user are not automatically removed. The root user (UID 0) retains full permissions over these files, ensuring they remain accessible.


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
-r-x--x--x     0     3          0 May 31 00:01 tempfile

> rmacc temp-acc wrongpassword
Error: Permission denied.
> rmacc temp-acc temppassword
Account removed.
```

Now, consider a scenario where two clients access the file system simultaneously. The following synchronized sessions demonstrate concurrent file operations. Here, it's also evident that Bob cannot create files within Samuel's directory (/home/samuel) due to permission restrictions.

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

### 2.3 Error handling

The following examples demonstrates some of the detectable error:

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

## 3 Feedback

If you find any bugs or have suggestions for improvements, feel free to open an issue. Your feedback is always welcome!