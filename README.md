# nes_sockets

Simple socket and secure socket in modern C++.

Non-blocking implementation of sockets for Windows and Linux for sequencial (not asynchronous) programming.

# Requisites

C++20

OpenSSL Library

# Building

This project uses CMake as build system. In directory ./build/ run:

cmake --build .
make

Will produce the static library (nes_sockets.a) to link your program.
You can configure another build systems in cmake options.
And can also add the dependency if you use CMake as well.

# Examples

See examples folder.