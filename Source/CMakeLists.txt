# Enable ARM11 compilation for baremetal.
if (CTR_BM)
    ctr_bm_enable_arm11()
endif()

# Setup library.
if (CTR_ENABLE_QTMRAM)
    set(CTR11_SOURCES QTMRAM.c)
endif()

if (CTR_BM)
    set(CTR11_SOURCES ${CTR11_SOURCES} libn3ds.c)
else()
    set(CTR11_SOURCES ${CTR11_SOURCES} libctru.c)
endif()

add_library(ctr11 STATIC ${CTR11_SOURCES})

if (CTR_BM)
    ctr_bm_link_libn3ds(ctr11 PUBLIC)
endif()

target_include_directories(ctr11 PUBLIC ../Include PRIVATE .)
target_compile_options(ctr11 PRIVATE -Wall -Werror)

if (CTR_ENABLE_QTMRAM)
    target_compile_definitions(ctr11 PRIVATE CTR_ENABLE_QTMRAM)
endif()

install(TARGETS ctr11)
install(DIRECTORY Include/ DESTINATION include)