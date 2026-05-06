# CMake for BACnet shell library
#
# @author Steve Karg <skarg@users.sourceforge.net>
# @date May 2024
# @copyright SPDX-License-Identifier: Apache-2.0
zephyr_interface_library_named(bacnet_shell)

zephyr_library()

zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_BACNETSTACK_BACNET_SHELL
  bacnet_shell.c
)

zephyr_library_sources_ifdef(CONFIG_BACNETSTACK_BACNET_BASIC_SHELL
  bacnet_shell_datetime.c
  bacnet_shell_lighting_output.c
  bacnet_shell_objects.c
  bacnet_shell_packets.c
  bacnet_shell_property.c
  bacnet_shell_reinit.c
  bacnet_shell_uptime.c
)
