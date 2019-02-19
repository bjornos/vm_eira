cmake_minimum_required(VERSION 3.7)

project(vm_eira)

# Set the output folder where your program will be created
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_BUILD_TYPE Debug)

find_package(Threads REQUIRED)

find_package(SDL2 REQUIRED)
find_file(SDL2_INCLUDE_DIR NAME SDL.h HINTS SDL2)
find_library(SDL2_LIBRARY NAME SDL2)
string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES) # fix bug in sdl2-config.cmake

# The following folder will be included
include_directories("${PROJECT_SOURCE_DIR}")
include_directories(SDL2Test ${SDL2_INCLUDE_DIRS})

set(SOURCES main.c cpu.c vdc.c vdc_vga.c vdc_console.c utils.c ioport.c prg.c)

add_executable(vm_eira ${PROJECT_SOURCE_DIR}/${SOURCES})
add_executable(asm2bin ${PROJECT_SOURCE_DIR}/asm2bin.c)

target_link_libraries(vm_eira ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(vm_eira ${SDL2_LIBRARIES})
