#!/bin/bash -x
set -e

rm -f ../bin/*.*

clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals algorithm -o ../bin/algorithm.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals array -o ../bin/array.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals atomic -o ../bin/atomic.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals bit -o ../bin/bit.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals charconv -o ../bin/charconv.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals chrono -o ../bin/chrono.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals cstddef -o ../bin/cstddef.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals cstdint -o ../bin/cstdint.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals cstring -o ../bin/cstring.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals cctype -o ../bin/cctype.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals format -o ../bin/format.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals -Wno-ambiguous-ellipsis \
 functional -o ../bin/functional.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals iomanip -o ../bin/iomanip.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals iostream -o ../bin/iostream.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals iterator -o ../bin/iterator.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals mutex -o ../bin/mutex.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals optional -o ../bin/optional.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals print -o ../bin/print.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals ranges -o ../bin/ranges.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals span -o ../bin/span.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals stdexcept -o ../bin/stdexcept.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals string -o ../bin/string.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals string_view -o ../bin/string_view.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals thread -o ../bin/thread.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals -Wno-deprecated-builtins \
 -Wno-keyword-compat type_traits -o ../bin/type_traits.pcm
clang++ -std=c++23 -xc++-system-header --precompile -Wno-unknown-warning-option -Wno-user-defined-literals -Wno-deprecated-builtins vector \
 -o ../bin/vector.pcm

clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/chrono.pcm -Wno-unqualified-std-cast-call \
 -Wno-experimental-header-units ../src/cfg.cppm -o ../bin/cfg.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/format.pcm  \
 -fmodule-file=../bin/stdexcept.pcm -fmodule-file=../bin/string.pcm -fmodule-file=../bin/string_view.pcm -Wno-unqualified-std-cast-call \
 -Wno-experimental-header-units ../src/nes_exc.cppm -o ../bin/nes_exc.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/stdexcept.pcm -Wno-unqualified-std-cast-call \
 -Wno-experimental-header-units ../src/net_exc.cppm -o ../bin/net_exc.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/algorithm.pcm -fmodule-file=../bin/array.pcm \
 -fmodule-file=../bin/bit.pcm -fmodule-file=../bin/cstddef.pcm -fmodule-file=../bin/cstdint.pcm -fmodule-file=../bin/iterator.pcm -fmodule-file=../bin/span.pcm \
 -fmodule-file=../bin/stdexcept.pcm -fmodule-file=../bin/string.pcm -fmodule-file=../bin/string_view.pcm -fmodule-file=../bin/type_traits.pcm \
 -fmodule-file=../bin/vector.pcm -Wno-unqualified-std-cast-call -Wno-experimental-header-units ../src/byte_op.cppm -o ../bin/byte_op.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/algorithm.pcm  \
 -fmodule-file=../bin/chrono.pcm -fmodule-file=../bin/cstddef.pcm -fmodule-file=../bin/span.pcm -fmodule-file=../bin/vector.pcm  \
 -fmodule-file=../bin/thread.pcm -Wno-unqualified-std-cast-call -Wno-experimental-header-units ../src/socket_util.cppm -o ../bin/socket_util.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/cstddef.pcm -fmodule-file=../bin/optional.pcm \
 -fmodule-file=../bin/span.pcm -fmodule-file=../bin/vector.pcm -fmodule-file=../bin/string.pcm -Wno-unqualified-std-cast-call \
 -Wno-experimental-header-units ../src/unix_socket.cppm -o ../bin/unix_socket.o
 clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-file=../bin/array.pcm -fmodule-file=../bin/atomic.pcm \
 -fmodule-file=../bin/chrono.pcm -fmodule-file=../bin/cstring.pcm -fmodule-file=../bin/functional.pcm -fmodule-file=../bin/stdexcept.pcm \
 -fmodule-file=../bin/string.pcm -fmodule-file=../bin/thread.pcm -Wno-unqualified-std-cast-call -Wno-experimental-header-units \
 ../src/unix_socket_impl.cpp -o ../bin/unix_socket_impl.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/cstddef.pcm -fmodule-file=../bin/cstdint.pcm \
 -fmodule-file=../bin/optional.pcm -fmodule-file=../bin/span.pcm -fmodule-file=../bin/string.pcm -fmodule-file=../bin/type_traits.pcm -fmodule-file=../bin/vector.pcm \
-Wno-unqualified-std-cast-call -Wno-experimental-header-units ../src/win_socket.cppm -o ../bin/win_socket.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/chrono.pcm -fmodule-file=../bin/span.pcm \
 -fmodule-file=../bin/string.pcm -fmodule-file=../bin/string_view.pcm -fmodule-file=../bin/type_traits.pcm -fmodule-file=../bin/vector.pcm \
 -Wno-unqualified-std-cast-call -Wno-experimental-header-units ../src/socket.cppm -o ../bin/socket.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/optional.pcm \
 -Wno-unqualified-std-cast-call -Wno-experimental-header-units ../src/socket_serv.cppm -o ../bin/socket_serv.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/atomic.pcm -fmodule-file=../bin/chrono.pcm \
 -fmodule-file=../bin/cstddef.pcm -fmodule-file=../bin/mutex.pcm -fmodule-file=../bin/span.pcm -fmodule-file=../bin/string.pcm \
 -fmodule-file=../bin/string_view.pcm -fmodule-file=../bin/thread.pcm -fmodule-file=../bin/vector.pcm \
 -Wno-unqualified-std-cast-call -Wno-experimental-header-units ../src/tls_socket.cppm -o ../bin/tls_socket.o
clang++ -Wall -Werror -std=c++23 -c -fprebuilt-module-path=../bin/ -fmodule-output -fmodule-file=../bin/atomic.pcm -fmodule-file=../bin/chrono.pcm \
 -fmodule-file=../bin/mutex.pcm -fmodule-file=../bin/optional.pcm -fmodule-file=../bin/thread.pcm \
 -Wno-unqualified-std-cast-call -Wno-experimental-header-units ../src/tls_socket_serv.cppm -o ../bin/tls_socket_serv.o

ar rcs ../bin/libnes_sockets.a ../bin/cfg.o ../bin/byte_op.o ../bin/nes_exc.o ../bin/net_exc.o ../bin/socket_util.o \
 ../bin/unix_socket.o ../bin/unix_socket_impl.o ../bin/win_socket.o ../bin/socket.o ../bin/socket_serv.o \
 ../bin/tls_socket.o ../bin/tls_socket_serv.o

clang++ -Wall -Werror -std=c++23 -fprebuilt-module-path=../bin/ -Wno-unqualified-std-cast-call -Wno-experimental-header-units \
 -fmodule-file=../bin/cctype.pcm -fmodule-file=../bin/chrono.pcm -fmodule-file=../bin/iomanip.pcm -fmodule-file=../bin/iostream.pcm \
 -fmodule-file=../bin/string.pcm -fmodule-file=../bin/string_view.pcm -fmodule-file=../bin/thread.pcm ../examples/http_get.cpp \
 ../bin/libnes_sockets.a  -lssl -lcrypto -o ../bin/ex_http_get
clang++ -Wall -Werror -std=c++23 -fprebuilt-module-path=../bin/ -Wno-unqualified-std-cast-call -Wno-experimental-header-units \
 -fmodule-file=../bin/algorithm.pcm -fmodule-file=../bin/charconv.pcm -fmodule-file=../bin/chrono.pcm -fmodule-file=../bin/cstddef.pcm \
 -fmodule-file=../bin/iostream.pcm -fmodule-file=../bin/ranges.pcm -fmodule-file=../bin/stdexcept.pcm -fmodule-file=../bin/string.pcm \
 -fmodule-file=../bin/span.pcm -fmodule-file=../bin/thread.pcm -fmodule-file=../bin/vector.pcm ../examples/tcp_echo_server.cpp \
 ../bin/libnes_sockets.a  -lssl -lcrypto -o ../bin/ex_tcp_echo_server

