# CMake for BACnet settings library
#
# @author Steve Karg <skarg@users.sourceforge.net>
# @date May 2024
# @copyright SPDX-License-Identifier: Apache-2.0
zephyr_interface_library_named(bacnet_settings)

zephyr_library()

zephyr_library_include_directories(include)

zephyr_library_sources(
  bacnet_storage.c
  bacnet_settings.c
  )

zephyr_library_sources_ifdef(CONFIG_BACNET_SETTINGS_SHELL
  bacnet_settings_shell.c
  bacnet_storage_shell.c
  )
