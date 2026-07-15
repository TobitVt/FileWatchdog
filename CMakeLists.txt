cmake_minimum_required(VERSION 3.16)

project(FileIntegrityMonitor LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FileIntegrityMonitor
    main.cpp
    database.cpp
    sqlite3.c
)

target_include_directories(FileIntegrityMonitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)