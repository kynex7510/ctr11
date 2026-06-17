# Enable ARM11 compilation for baremetal.
if (CTR_BM)
    ctr_bm_enable_arm11()
endif()

# Setup library.
set(CTR11_SOURCES QTMRAM.c)

if (CTR_BM)
    set(CTR11_SOURCES ${CTR11_SOURCES} libn3ds.c)
else()
    set(CTR11_SOURCES ${CTR11_SOURCES} libctru.c)
endif()

add_library(ctr11 OBJECT ${CTR11_SOURCES})

if (CTR_BM)
    ctr_bm_link_libn3ds(ctr11 PUBLIC)
endif()

target_include_directories(ctr11 PUBLIC ../Include PRIVATE .)
target_compile_options(ctr11 PRIVATE -Wall -Werror)

install(TARGETS ctr11)
install(DIRECTORY Include/ DESTINATION include)