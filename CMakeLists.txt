#~~~ nes_sockets - Nicolas Engels

cmake_minimum_required(VERSION 3.16)
project(nes_sockets VERSION 5)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_compile_options(/W4 /WX)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unqualified-std-cast-call)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

#~~ nes_sockets Library
set(nes_sck_srcs
  include/byte_op.h         src/byte_op.cpp
  include/cfg.h
  include/net_exc.h
  include/socket.h          src/socket.cpp
  include/socket_serv.h     src/socket_serv.cpp
  include/socket_util.h     src/socket_util.cpp
  include/tls_socket.h      src/tls_socket.cpp
  include/tls_socket_serv.h src/tls_socket_serv.cpp
)

if (WIN32)
  list(APPEND nes_sck_srcs include/win_socket.h src/win_socket.cpp)
else ()
  list(APPEND nes_sck_srcs include/unix_socket.h src/unix_socket.cpp)
endif ()

add_library(nes_sockets ${nes_sck_srcs})

target_include_directories(nes_sockets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_property(TARGET nes_sockets PROPERTY CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
target_link_libraries(nes_sockets PUBLIC OpenSSL::SSL)
target_link_libraries(nes_sockets PUBLIC OpenSSL::Crypto)

if (WIN32)
  target_link_libraries(nes_sockets PUBLIC wsock32 ws2_32)
endif ()

#~~ Examples
# HTTP GET
add_executable(ex_http_get EXCLUDE_FROM_ALL examples/http_get.cpp)

set_property(TARGET ex_http_get PROPERTY CXX_STANDARD 20)
target_link_libraries(ex_http_get nes_sockets)

# TCP Echo Server
add_executable(ex_tcp_echo_server EXCLUDE_FROM_ALL examples/tcp_echo_server.cpp)

set_property(TARGET ex_tcp_echo_server PROPERTY CXX_STANDARD 23)
target_link_libraries(ex_tcp_echo_server nes_sockets)
