cmake_minimum_required(VERSION 3.10)

project(SerialSnake)

set(CXX_STANDARD C++11)
include_directories(${PROJECT_SOURCE_DIR})

add_executable(SerialSnake SerialPort.cpp Snake.cpp Game.cpp)

target_link_libraries(SerialSnake ncursesw)