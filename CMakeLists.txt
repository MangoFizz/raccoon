# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.16)

project(raccoon)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(balltze-lib/balltze.cmake)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(-DRACCOON_EXPORTS)

add_library(raccoon SHARED
    src/postprocess/pixelate_filter.cpp
    src/postprocess/shaders.rc
    src/medals/h4.cpp
    src/medals/medals.cpp
    src/medals/queue.cpp
    src/resources/resources.cpp
    src/main.cpp
)

target_link_libraries(raccoon balltze)
set_target_properties(raccoon PROPERTIES PREFIX "")
set_target_properties(raccoon PROPERTIES OUTPUT_NAME "raccoon")
set_target_properties(raccoon PROPERTIES LINK_FLAGS "-static -static-libgcc -static-libstdc++")

install(TARGETS raccoon DESTINATION "${CMAKE_INSTALL_PREFIX}")
