cmake_minimum_required(VERSION 3.12)
project(node_omx)

set(CMAKE_CXX_STANDARD 14)

include(NodeJS.cmake)
nodejs_init()

add_nodejs_module(${PROJECT_NAME} node-omx.cpp Player.cpp)