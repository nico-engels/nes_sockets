# nes_sockets

Simple socket and secure socket in modern C++.

Non-blocking implementation of sockets for Windows and Linux for sequencial (not asynchronous) programming.

# Requisites

- C++20

- OpenSSL Library

# Building

This project uses CMake as build system. In directory ./build/ run:

```
cmake ..
cmake --build .
```

Will produce the static library (nes_sockets.a) to link your program.
You can configure another build systems in cmake options.
And can also add the dependency if you use CMake as well.

# Examples

In examples folder:

- `http_get.cpp`: show basic usage of sockets to use the HTTP protocol. CMake build target `ex_http_get`.

- `tcp_echo_server.cpp`: a echo server that listen to incoming conections. Prints the received data
in terminal and sends back the same data. CMake build target `ex_tcp_echo_server`.