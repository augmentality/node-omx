cmake_minimum_required(VERSION 3.12)
project(node_omx)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_STANDARD 14)

add_compile_definitions(HAVE_LIBOPENMAX=2 OMX OMX_SKIP64BIT USE_EXTERNAL_OMX HAVE_LIBBCM_HOST USE_EXTERNAL_LIBBCM_HOST USE_VCHIQ_ARM)

include(NodeJS.cmake)
nodejs_init()

add_subdirectory( libomx )

add_nodejs_module(${PROJECT_NAME} node-omx.cpp Player.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE libomx)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE libomx)