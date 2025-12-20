cmake_minimum_required(VERSION 3.16)
project(file_search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(search_engine
    src/main.cpp
    src/thread_pool.cpp
    src/engine.cpp
)

# ---- Qt GUI app ----
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)


add_executable(search_engine_gui
    src/gui_main.cpp
    src/engine.cpp
    src/thread_pool.cpp
)

target_link_libraries(search_engine_gui PRIVATE Qt6::Widgets Qt6::Concurrent)
