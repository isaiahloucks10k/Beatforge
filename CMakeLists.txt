cmake_minimum_required(VERSION 3.29)
project(BeatForge)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Geode SDK
find_package(Geode REQUIRED)

# Add source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.h
)

# Create mod target
geode_add_mod(
    TARGET BeatForge
    SOURCES ${SOURCES}
    PACKAGE com.beatforge
)

# Link FMOD (comes with GD, we just use headers)
target_include_directories(BeatForge PRIVATE
    ${GEODE_INCLUDE_DIRS}/fmod
)