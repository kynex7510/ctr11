cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# C23, CXX, ASM required for libn3ds.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(ctr11 C CXX ASM)

option(CTR_ENABLE_QTMRAM "Enable QTMRAM" OFF)

add_subdirectory(Source)