cmake_minimum_required(VERSION 3.00)
project(dulafs C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -lm")

# Default to a Debug build so binaries include debug symbols for the debugger.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Make sure debug builds include symbols and disable optimizations which
# make breakpoints unreliable.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

# Include src directory for headers
include_directories(src)

# Collect all source files from src directory
file(GLOB SOURCES
    "src/*.c"
)

# Create executable
add_executable(dulafs.out ${SOURCES})
