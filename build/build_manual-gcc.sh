#!/bin/bash -x
set -e

rm -f ../bin/*
rm -rf gcm.cache

g++-latest -Wall -Werror -std=c++23 -fmodules-ts -fsearch-include-path bits/std.cc -c -o ../bin/std.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/cfg.cppm -c -o ../bin/cfg.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/nes_exc.cppm -c -o ../bin/nes_exc.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/net_exc.cppm -c -o ../bin/net_exc.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/byte_op.cppm -c -o ../bin/byte_op.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/socket_util.cppm -c -o ../bin/socket_util.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/unix_socket.cppm -c -o ../bin/unix_socket.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/unix_socket_impl.cpp -c -o ../bin/unix_socket_impl.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/win_socket.cppm -c -o ../bin/win_socket.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/socket.cppm -c -o ../bin/socket.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/socket_serv.cppm -c -o ../bin/socket_serv.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/tls_socket.cppm -c -o ../bin/tls_socket.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../src/tls_socket_serv.cppm -c -o ../bin/tls_socket_serv.o

ar rcs ../bin/libnes_sockets.a ../bin/cfg.o ../bin/byte_op.o ../bin/nes_exc.o ../bin/net_exc.o ../bin/socket_util.o \
 ../bin/unix_socket.o ../bin/unix_socket_impl.o ../bin/win_socket.o ../bin/socket.o ../bin/socket_serv.o \
 ../bin/tls_socket.o ../bin/tls_socket_serv.o

g++-latest -Wall -Werror -std=c++23 -fmodules-ts -lssl -lcrypto ../examples/http_get.cpp ../bin/libnes_sockets.a \
 -o ../bin/ex_http_get
g++-latest -Wall -Werror -std=c++23 -fmodules-ts -lssl -lcrypto ../examples/tcp_echo_server.cpp ../bin/libnes_sockets.a \
 -o ../bin/ex_tcp_echo_server

g++-latest -Wall -Werror -std=c++23 -fmodules-ts ../tests/qtest.cppm -c -o ../bin/qtest.o
g++-latest -Wall -Werror -std=c++23 -fmodules-ts -lssl -lcrypto ../tests/main.cpp ../bin/libnes_sockets.a ../bin/qtest.o \
 -o ../bin/nes_socket_test