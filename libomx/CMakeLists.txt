cmake_minimum_required(VERSION 3.12)
project(libomx)
set(CMAKE_BUILD_TYPE Debug)
find_package(PkgConfig)
IF (UNIX)
    pkg_search_module(BRCMEGL REQUIRED brcmegl)
ENDIF(UNIX)
pkg_search_module(LIBAVFORMAT REQUIRED libavformat)
pkg_search_module(LIBAVCODEC REQUIRED libavcodec)
pkg_search_module(LIBAVUTIL REQUIRED libavutil)
pkg_search_module(LIBSWRESAMPLE REQUIRED libswresample)

set(CMAKE_CXX_STANDARD 14)

add_library(libomx NativePlayer.cpp NativePlayer.h VideoThread.cpp VideoThread.h AudioThread.cpp AudioThread.h ilclient/ILComponent.cpp ilclient/ILComponent.h ilclient/VideoDecodeComponent.cpp ilclient/VideoDecodeComponent.h ilclient/ilc.h ilclient/ILClient.cpp ilclient/ILClient.h ilclient/ILClientException.h ilclient/ILComponentException.h ilclient/VideoRenderComponent.cpp ilclient/VideoRenderComponent.h ilclient/ClockComponent.cpp ilclient/ClockComponent.h ilclient/VideoSchedulerComponent.cpp ilclient/VideoSchedulerComponent.h ilclient/ILTunnel.h ffsource/FFSource.cpp ffsource/FFSource.h ffsource/FFFrame.h ilclient/AudioRenderComponent.cpp ilclient/AudioRenderComponent.h)

target_include_directories(libomx SYSTEM PUBLIC ${BRCMEGL_INCLUDE_DIRS} ${LIBAVFORMAT_INCLUDE_DIRS} ${LIBAVCODEC_INCLUDE_DIRS} ${LIBAVUTIL_INCLUDE_DIRS} ${LIBSWRESAMPLE_INCLUDE_DIRS})
target_link_libraries(libomx -lpthread -lopenmaxil ${BRCMEGL_LIBRARIES} ${LIBAVFORMAT_LIBRARIES} ${LIBAVCODEC_LIBRARIES} ${LIBAVUTIL_LIBRARIES} ${LIBSWRESAMPLE_LIBRARIES} )



