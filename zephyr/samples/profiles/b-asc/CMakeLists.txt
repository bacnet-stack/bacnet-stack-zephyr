# @brief CMake for BACnet basic sample
# @author Steve Karg <skarg@users.sourceforge.net>
# @date November 2025
# @copyright SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(MY_PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# The native_sim Ethernet driver Kconfig symbol was renamed in newer Zephyr,
# so select the matching fragment based on what the Zephyr tree provides.
if(BOARD MATCHES "^native_sim" AND DEFINED ENV{ZEPHYR_BASE})
  if(EXISTS $ENV{ZEPHYR_BASE}/drivers/ethernet/Kconfig.native_tap)
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/native_sim_eth_tap.conf)
  else()
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/native_sim_eth_posix.conf)
  endif()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(${MY_PROJECT_NAME})

target_sources(app PRIVATE src/main.c)
