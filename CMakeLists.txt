cmake_minimum_required(VERSION 2.8)

project(vm_eira)

# Set the output folder where your program will be created
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# The following folder will be included
include_directories("${PROJECT_SOURCE_DIR}")

FIND_PACKAGE (Threads)

set(SOURCES main.c display.c prg.h utils.c ioport.c cpu.c prg.c)

add_executable(vm_eira ${PROJECT_SOURCE_DIR}/${SOURCES})
add_executable(asm2bin ${PROJECT_SOURCE_DIR}/asm2bin.c)

target_link_libraries (vm_eira ${CMAKE_THREAD_LIBS_INIT})
