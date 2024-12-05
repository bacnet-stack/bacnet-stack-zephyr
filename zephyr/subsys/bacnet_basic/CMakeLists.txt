# CMake for BACnet settings library
#
# @author Steve Karg <skarg@users.sourceforge.net>
# @date May 2024
# @copyright SPDX-License-Identifier: Apache-2.0
zephyr_library(bacnet_basic)

message(STATUS "BACNET BASIC: CONFIG_BACNET_BASIC_SERVER_STACK_SIZE \"${CONFIG_BACNET_BASIC_SERVER_STACK_SIZE}\"")
message(STATUS "BACNET BASIC: CONFIG_BACNET_BASIC_SERVER_INIT_PRIORITY \"${CONFIG_BACNET_BASIC_SERVER_INIT_PRIORITY}\"")
message(STATUS "BACNET BASIC: CONFIG_BACNET_BASIC_SERVER_THREAD_PRIORITY \"${CONFIG_BACNET_BASIC_SERVER_THREAD_PRIORITY}\"")
message(STATUS "BACNET BASIC: CONFIG_BACNET_BASIC_SERVER_KSLEEP \"${CONFIG_BACNET_BASIC_SERVER_KSLEEP}\"")

zephyr_library_include_directories(include)

zephyr_library_sources(
  bacnet_port.c
  bacnet_port_ipv4.c
  bacnet_port_ipv6.c
  bacnet_basic.c
  device.c
  server.c
  )

zephyr_library_sources_ifdef(CONFIG_BACNET_BASIC_DEVICE_SHELL
  bacnet_shell_objects.c
  bacnet_shell_packets.c
  bacnet_shell_uptime.c
)
