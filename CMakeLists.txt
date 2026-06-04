cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# C23, CXX, ASM required for libn3ds.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(ctrcompat C CXX ASM)

# Enable ARM11 compilation for baremetal.
if (CTR_BM)
    ctr_bm_enable_arm11()
endif()

# Setup library.
set(CTRCOMPAT_SOURCES Source/QTMRAM.c)

if (CTR_BM)
    set(CTRCOMPAT_SOURCES ${CTRCOMPAT_SOURCES} Source/libn3ds.c)
else()
    set(CTRCOMPAT_SOURCES ${CTRCOMPAT_SOURCES} Source/libctru.c)
endif()

add_library(ctrcompat STATIC ${CTRCOMPAT_SOURCES})

if (CTR_BM)
    ctr_bm_link_libn3ds(ctrcompat)
endif()

target_include_directories(ctrcompat PUBLIC Include)
target_compile_options(ctrcompat PRIVATE -Wall -Werror)

install(TARGETS ctrcompat)
install(DIRECTORY Include/ DESTINATION include)