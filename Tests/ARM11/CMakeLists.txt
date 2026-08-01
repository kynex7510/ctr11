if (CTR_BM)
    ctr_bm_enable_arm11()
endif()

set(TEST_SOURCES
    Memory.c
    MemVars.s
    Main.c  
)

add_executable(ctr11-tests ${TEST_SOURCES})
target_link_libraries(ctr11-tests PRIVATE ctr11)

if (CTR_BM)
    target_link_options(ctr11-tests PRIVATE -T ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)
    ctr_bm_add_firm_binary(ctr11-tests_bin ctr11-tests)
else()
    ctr_create_3dsx(ctr11-tests)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ctr11-tests.3dsx TYPE BIN)
endif()