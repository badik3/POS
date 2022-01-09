cmake_minimum_required(VERSION 3.18)
project(untitled7 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_CFLAGS} -pthread")
add_executable(server server.c)
add_executable(client client.c)