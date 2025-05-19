# nes_sockets

Simple socket and secure socket in modern C++.

Non-blocking implementation of sockets for Windows and Linux for sequencial (not asynchronous) programming.

# Branch using Modules

This branch is experimental using the new C++20 modules system. It will use the modules in repo files and
the standard library using module `std`.

CMake support WIP. Until then it will use a bash script for building.

# Requisites

- C++23

- OpenSSL Library

# Building

This project uses CMake as build system. In directory ./build/ run:

```
cmake ..
cmake --build .
```

Will produce the static library in `bin` to link your program.
You can configure another build systems in cmake options.
And can also add the dependency if you use CMake as well.

# Tests

Is used a homemade testing framework called `qtest`. To build the testing of `nes_sockets` use the CMake
target `nes_socket_test` will link to the library and produces the executable `nes_socket_test`.

Example of sucessful run:

```
~/.../build$ ../bin/nes_socket_test
Automated Test Suite
Default path dir: ".../nes_sockets/bin"
### Test Summary ###
32 test executed in 1 package. (0 errors).
```

Example of failure:

```
~/.../build$ ../bin/nes_socket_test
Automated Test Suite
Default path dir: ".../nes_sockets/bin"
### Test Summary ###
--- Package nes_sockets ---

### Test nes::net::socket[_serv] ###

Test localhost communication

 8: [ ER ]         qtest::eq(bin_to_strv(data_recv), "abcde");
.../nes_sockets/tests/main.cpp:158
lhs: abcd
rhs: abcde


32 test executed in 1 package. (1 error).
```

# Examples

In examples folder:

- `http_get.cpp`: show basic usage of sockets to use the HTTP protocol. CMake build target `ex_http_get`.

- `tcp_echo_server.cpp`: a echo server that listen to incoming conections. Prints the received data
in terminal and sends back the same data. CMake build target `ex_tcp_echo_server`.
