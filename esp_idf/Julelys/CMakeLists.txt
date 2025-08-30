#
# Minimal CMake configuration for an ESP‑IDF CLI application.
# Place this file in the project root (Julelys).

cmake_minimum_required(VERSION 3.5)
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(julelys_cli)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE include)