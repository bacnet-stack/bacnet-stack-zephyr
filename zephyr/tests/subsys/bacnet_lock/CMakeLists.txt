cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bacnet_lock)

target_sources(app PRIVATE
    src/main.c
)

add_compile_definitions(CONFIG_ZTEST_NEW_API)
