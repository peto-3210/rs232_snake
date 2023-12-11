cmake_minimum_required(VERSION 3.10)

project(Serial)

include_directories(${PROJECT_SOURCE_DIR}/SerialPort.h)

add_executable(Serial SerialPort.cpp main.cpp)