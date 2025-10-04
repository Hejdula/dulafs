cmake_minimum_required(VERSION 3.00)
project(dulafs C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -lm")

# Include src directory for headers
include_directories(src)

# Collect all source files from src directory
file(GLOB SOURCES
    "src/*.c"
)

# Create executable
add_executable(dulafs.out ${SOURCES})
