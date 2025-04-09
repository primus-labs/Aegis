configure_lit_site_cfg(
        ${CMAKE_CURRENT_LIST_DIR}/lit.site.cfg.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
        MAIN_CONFIG
        ${CMAKE_CURRENT_LIST_DIR}/lit.cfg.py
)

set(AEGIS_TEST_DEPENDS
    FileCheck
    aegiscompiler
    )

add_lit_testsuite(check-aegis "Running Tests for aegis"
            ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${AEGIS_TEST_DEPENDS}
        )
set_target_properties(check-aegis PROPERTIES FOLDER "Tests")

add_lit_testsuites(AEGIS ${CMAKE_CURRENT_LIST_DIR} DEPENDS ${AEGIS_TEST_DEPENDS})


# -------------------------------------------------------------------------------
#                                   Unit tests
# -------------------------------------------------------------------------------
option(AEGIS_UNIT_TESTS "Enables the build of unittests" ON)
if(AEGIS_UNIT_TESTS)
  add_subdirectory(unit_tests)
endif()
