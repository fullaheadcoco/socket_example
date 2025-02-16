cmake_minimum_required(VERSION 3.10)

# Set the project name
project(server C)

set(CMAKE_C_COMPILER gcc)

add_library(err_handle src/err_handle.c include/err_handle.h)

# Add an executable
add_executable(server src/server.c src/err_handle.c include/err_handle.h)
add_executable(client src/client.c src/err_handle.c include/err_handle.h)