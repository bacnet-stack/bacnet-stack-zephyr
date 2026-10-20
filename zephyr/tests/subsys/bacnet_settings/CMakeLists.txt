cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bacnet_settings)

string(REGEX REPLACE
    "/zephyr/tests/[0-9a-zA-Z_/-]*$"
    "/zephyr"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(app PRIVATE
    ${SRC_DIR}/include
)

target_sources(app PRIVATE
    src/main.c
    src/storage_erase.c
    ${SRC_DIR}/subsys/bacnet_settings/bacnet_storage.c
)

add_compile_definitions(
    CONFIG_ZTEST_NEW_API
    CONFIG_BACNET_SETTINGS_BASE_NAME=\".bacnet\"
    CONFIG_BACNET_STORAGE_BASE_NAME=\".bacnet\"
)

# bacnet_storage_erase() tests intercept the flash area calls made by
# bacnet_storage.c (and only those) to count erases and inject errors.
set_source_files_properties(
    ${SRC_DIR}/subsys/bacnet_settings/bacnet_storage.c
    PROPERTIES COMPILE_DEFINITIONS
    "flash_area_open=bacnet_test_flash_area_open;flash_area_erase=bacnet_test_flash_area_erase"
)

# XIP firmware overlap guard scenarios, see src/storage_erase.c
if(BACNET_TEST_XIP_SCENARIO STREQUAL "overlap")
    target_compile_definitions(app PRIVATE BACNET_TEST_XIP_OVERLAP)
elseif(BACNET_TEST_XIP_SCENARIO STREQUAL "other_device")
    target_compile_definitions(app PRIVATE BACNET_TEST_XIP_OTHER_DEVICE)
elseif(BACNET_TEST_XIP_SCENARIO)
    message(FATAL_ERROR "unknown BACNET_TEST_XIP_SCENARIO ${BACNET_TEST_XIP_SCENARIO}")
endif()
