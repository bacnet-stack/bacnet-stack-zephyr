cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bacnet_lock)

target_sources(app PRIVATE
    src/main.c
)

# The clock stores the shared time, then the shared date. Those stores are
# inside datetime_copy() in one file, so wrap that function. Also wrap
# datetime_copy_time() for a direct store from another file. The stack
# revision in west.yml has no test hook for this pause.
zephyr_ld_options(
    -Wl,--wrap=datetime_copy
    -Wl,--wrap=datetime_copy_time
)

add_compile_definitions(CONFIG_ZTEST_NEW_API)
