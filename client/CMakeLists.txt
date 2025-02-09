cmake_minimum_required(VERSION 3.10)

# Set the project name
project(client C)

set(CMAKE_C_COMPILER gcc)

# Add an executable
add_executable(client main.c)