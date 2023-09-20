# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.16)

project(halo-world)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(balltze-lib/balltze.cmake)

add_library(${PROJECT_NAME} MODULE
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} balltze)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "halo-world")
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-static -static-libgcc -static-libstdc++")
