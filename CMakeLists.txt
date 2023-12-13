cmake_minimum_required(VERSION 3.10)

project(SerialSnake)

set(CXX_STANDARD C++11)
include_directories(${PROJECT_SOURCE_DIR})

add_executable(SerialSnake SerialPort.cpp Snake.cpp Game.cpp)

if (MINGW OR NOT WIN32)
    target_link_libraries(SerialSnake ncursesw)
endif()