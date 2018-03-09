# Challenge(C++)

## Installation

To install gRPC on your system, follow the instructions to build from source
[here](../../INSTALL.md). This also installs the protocol buffer compiler
`protoc` (if you don't have it already), and the C++ gRPC plugin for `protoc`.

## Send String, Number, and FIle C++ gRPC!

Here's how to build and run the C++ implementation of the [StringNumberFile](challenge.proto)

### Client and server implementations

The client implementation is at [challenge_client.cc](challenge/challenge_client.cc).

The server implementation is at [challenge_server.cc](challenge/challenge_server.cc).

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
