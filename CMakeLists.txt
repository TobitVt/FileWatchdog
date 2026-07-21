# --- Shared core library: scanning/hashing/comparing/DB logic, no CLI or Qt code ---
add_library(FileIntegrityCore STATIC
    core.cpp
    database.cpp
    sqlite3.c
)

target_include_directories(FileIntegrityCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --- CLI executable ---
add_executable(FileIntegrityMonitor
    main.cpp
)

target_link_libraries(FileIntegrityMonitor PRIVATE
    FileIntegrityCore
)

# --- Qt GUI executable (uncomment once gui_main.cpp / MainWindow exist) ---
# find_package(Qt6 COMPONENTS Widgets REQUIRED)
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTORCC ON)
#
# add_executable(FileIntegrityMonitorGui
#     gui_main.cpp
#     mainwindow.cpp
#     mainwindow.h
#     mainwindow.ui
# )
#
# target_link_libraries(FileIntegrityMonitorGui PRIVATE
#     FileIntegrityCore
#     Qt6::Widgets
# )