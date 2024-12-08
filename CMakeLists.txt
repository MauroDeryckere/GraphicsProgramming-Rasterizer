cmake_minimum_required(VERSION 3.27)

# Project Name
project(GP1_Rasterizer)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(project)


# REDUNDANT, use this only if you want to let CMake build SDL
# include(FetchContent)
# FetchContent_Declare(
#  SDL2
#  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
#  GIT_TAG release-2.30.7
#  GIT_SHALLOW TRUE
#  GIT_PROGRESS TRUE
#)
#FetchContent_MakeAvailable(SDL2)
