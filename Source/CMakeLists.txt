# Enable ARM11 compilation for baremetal.
if (CTR_BM)
    ctr_bm_enable_arm11()
endif()

# Setup library.
set(CTR11_SOURCES
    Cache.c
    Impl.c
    Memory.c
    QTMRAM.c
    Random.c
    Sync.c
    Tick.c    
)

if (CTR_ENABLE_TESTING)
    set(CTR11_SOURCES ${CTR11_SOURCES} Testing.c)
endif()

add_library(ctr11 STATIC ${CTR11_SOURCES})

target_include_directories(ctr11 PUBLIC ../Include PRIVATE .)
target_compile_options(ctr11 PRIVATE -Wall -Werror)

if (CTR_ENABLE_TESTING)
    target_compile_definitions(ctr11 PUBLIC CTR_ENABLE_TESTING)
endif()

if (CTR_BM)
    target_compile_definitions(ctr11 PUBLIC CTR_BM)
    ctr_bm_link_libn3ds(ctr11 PUBLIC)
endif()

install(TARGETS ctr11)
install(DIRECTORY ../Include/ DESTINATION include)