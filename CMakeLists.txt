#~~~ nes_sockets - Nicolas Engels

#[[
cd build

-- Windows Mingw64
set PATH=%PATH%;C:\msys64\mingw64\bin\
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

-- Windows MSVC++
set PATH=%PATH%;"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\"
cmake -G "Visual Studio 17 2022" ..
cmake --build .

-- PI
cmake -DCMAKE_BUILD_TYPE=Debug .. -DCMAKE_C_COMPILER=/usr/bin/arm-linux-gnueabihf-gcc-10 -DCMAKE_CXX_COMPILER=/usr/bin/arm-linux-gnueabihf-g++-10
]]
cmake_minimum_required(VERSION 3.16)
project(nes_sockets VERSION 5)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

#~~ Biblioteca nes_sockets
set(nes_sck_srcs
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

#~~ Executáveis de testes
# Teste HTTP GET
add_executable(teste_http_get EXCLUDE_FROM_ALL teste/teste_http_get.cpp)

set_property(TARGET teste_http_get PROPERTY CXX_STANDARD 20)
target_link_libraries(teste_http_get nes_sockets)

# Teste Mail
add_executable(teste_mail EXCLUDE_FROM_ALL teste/teste_mail.cpp)

set_property(TARGET teste_mail PROPERTY CXX_STANDARD 20)
target_link_libraries(teste_mail nes_sockets)
