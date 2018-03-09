# Challenge(C++)

## Send String, Number, and FIle C++ gRPC!

Protoc definitions [StringNumberFile](challenge.proto)

### Client and server implementations

The client implementation is at [challenge_client.cc](challenge_client.cc).

The server implementation is at [challenge_server.cc](challenge_server.cc).

### Try it!
Build client and server:

```sh
$ make
```

Run the server, which will listen on port 50051:

```sh
$ ./challenge_server
```

Run the client (in a different terminal):

```sh
$ ./challenge_client
```

### Test script:
```sh
sh test_script.sh
```

### Yocto Tests:
I've tested the above script both on the newest macOS High Sierra v10.13.3
and Yocto(installed from https://github.com/gmacario/easy-build/tree/master/build-yocto)
Script ran on both without any issues.

I ran yocto in docker,
cloned git's grpc there,
installed required packages(https://github.com/grpc/grpc/blob/master/INSTALL.md),
installed protoc,
sh test_script.sh -> Worked


