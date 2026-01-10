# @brief CMake for BACnet basic sample
# @author Steve Karg <skarg@users.sourceforge.net>
# @date November 2025
# @copyright SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(MY_PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(${MY_PROJECT_NAME})

target_sources(app PRIVATE src/main.c)
