cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bacnet_settings)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

target_sources(app PRIVATE
    src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../subsys/bacnet_settings/bacnet_storage.c
)

add_compile_definitions(
    CONFIG_ZTEST_NEW_API
    CONFIG_BACNET_STORAGE_BASE_NAME=\"bacnet\"
)
