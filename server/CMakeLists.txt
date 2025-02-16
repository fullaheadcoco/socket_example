cmake_minimum_required(VERSION 3.10)

# Set the project name
project(server C)

set(CMAKE_C_COMPILER gcc)

# Add an executable
add_executable(server main.c err_handle.c err_handle.h)