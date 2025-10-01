cmake_minimum_required(VERSION 3.00)
project(project_name C)
set(CMAKE_C_CSTANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -lm")
include_directories(HEADER_FILES)
file(GLOB MAIN
    "main.c"
    "repl.c"
    "commands.c"
    "globals/globals.c" 
    "Encryption/cipher.c"  
    "Backend/document.c"
    "nosql.c"
    "DocumentHashmap/DocumentHashMap.c"
    )
    add_executable(dulafs.out ${MAIN})
