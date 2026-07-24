cmake_minimum_required(VERSION 3.16)

project(FileIntegrityMonitor LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)




add_library(FileIntegrityCore STATIC
    core.cpp
    database.cpp
    sqlite3.c
)

target_include_directories(FileIntegrityCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(FileIntegrityMonitor
    main.cpp
)

target_link_libraries(FileIntegrityMonitor PRIVATE
    FileIntegrityCore
)


find_package(Qt6 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(FileIntegrityMonitorGui
    gui_main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_link_libraries(FileIntegrityMonitorGui PRIVATE
    FileIntegrityCore
    Qt6::Widgets
)