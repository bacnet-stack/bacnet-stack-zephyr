# Copyright (c) 2022 Legrand North America, LLC.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Extract module path and names
string(REGEX REPLACE
  "/zephyr/tests/[a-zA-Z_/-]*$" "/stack"
  BACNET_BASE
  ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
  "/zephyr/tests/" "/stack/src/"
  BACNET_SRC_PATH
  ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
  "/zephyr/tests/" "/stack/test/"
  BACNET_TEST_PATH
  ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(BACNET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Update include path for this module
list(APPEND BACNET_INCLUDE ${BACNET_BASE}/src)

if(BOARD STREQUAL unit_testing)
  file(RELATIVE_PATH BACNET_INCLUDE $ENV{ZEPHYR_BASE} ${BACNET_BASE}/src)
  list(APPEND INCLUDE ${BACNET_INCLUDE})
  list(APPEND SOURCES
    ${BACNET_BASE}/src/bacnet/basic/object/access_point.c
    ${BACNET_BASE}/src/bacnet/access_rule.c
    ${BACNET_BASE}/src/bacnet/authentication_factor.c
    ${BACNET_BASE}/src/bacnet/authentication_factor_format.c
    ${BACNET_BASE}/src/bacnet/bacaction.c
    ${BACNET_BASE}/src/bacnet/bacaddr.c
    ${BACNET_BASE}/src/bacnet/bacapp.c
    ${BACNET_BASE}/src/bacnet/bacdcode.c
    ${BACNET_BASE}/src/bacnet/bacdest.c
    ${BACNET_BASE}/src/bacnet/bacdevobjpropref.c
    ${BACNET_BASE}/src/bacnet/bacint.c
    ${BACNET_BASE}/src/bacnet/baclog.c
    ${BACNET_BASE}/src/bacnet/bacreal.c
    ${BACNET_BASE}/src/bacnet/bacstr.c
    ${BACNET_BASE}/src/bacnet/bactext.c
    ${BACNET_BASE}/src/bacnet/basic/sys/bigend.c
    ${BACNET_BASE}/src/bacnet/datetime.c
    ${BACNET_BASE}/src/bacnet/basic/sys/days.c
    ${BACNET_BASE}/src/bacnet/indtext.c
    ${BACNET_BASE}/src/bacnet/hostnport.c
    ${BACNET_BASE}/src/bacnet/lighting.c
    ${BACNET_BASE}/src/bacnet/proplist.c
    ${BACNET_BASE}/src/bacnet/shed_level.c
    ${BACNET_BASE}/src/bacnet/timer_value.c
    ${BACNET_BASE}/src/bacnet/timestamp.c
    ${BACNET_BASE}/src/bacnet/weeklyschedule.c
    ${BACNET_BASE}/src/bacnet/bactimevalue.c
    ${BACNET_BASE}/src/bacnet/dailyschedule.c
    ${BACNET_BASE}/src/bacnet/calendar_entry.c
    ${BACNET_BASE}/src/bacnet/special_event.c
    ${BACNET_BASE}/src/bacnet/channel_value.c
    ${BACNET_BASE}/src/bacnet/secure_connect.c
    ${BACNET_BASE}/src/bacnet/wp.c
    ${BACNET_TEST_PATH}/src/main.c
    )

  set(CONF_FILE "${CONF_FILE};prj.unit_testing.conf")
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(${BACNET_NAME})
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(${BACNET_NAME})

  target_include_directories(app PRIVATE ${BACNET_INCLUDE})
  target_sources(app PRIVATE
    ${BACNET_TEST_PATH}/src/main.c
    )
endif()

add_compile_definitions(
  CONFIG_ZTEST_NEW_API
  BACNET_STACK_DEPRECATED_DISABLE
  )
