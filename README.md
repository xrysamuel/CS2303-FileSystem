# CS2303 File System

## Compile

```
make
```

## Run

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