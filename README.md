# simple-ftp

### Description

This is a simple file transfer client and host using Unix sockets.  The server is written in C and the client is written in Python

### Instructions:

To compile ftserver, use makefile

To run ftserver, use command:

```
    ftserver <port>
```

To run ftclient, use one of the following commands:

```
    python3 ftclient.py <hostname> <port> -l <data port>
    python3 ftclient.py <hostname> <port> -g <filename> <data port>
```
    

##### Example
Start ftserver on host1 using

```
    ftserver 6748
```

Use ftclient on host2 to get directory listing, request data transfer on port 6749

```
    python3 ftclient.py host1 6748 -l 6749
```

User ftclient on host2 to request shortfile.txt from server on host1, request data transfer on port 6749

```
    python3 ftclient.py host1 6748 -g shortfile.txt 6749
```

User ftclient on host2 to request longfile.txt from server on host1, request data transfer on port 6749

```
    python3 ftclient.py host1 6748 -g longfile.txt 6749
```
