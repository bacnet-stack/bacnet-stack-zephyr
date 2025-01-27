# CMake for BACnet RS485 Driver
#
# @author Steve Karg <skarg@users.sourceforge.net>
# @date January 2025
# @copyright SPDX-License-Identifier: Apache-2.0
zephyr_sources(rs485.c)

message(STATUS "RS485: CONFIG_BACNET_DRIVER_RS485 \"${CONFIG_BACNET_DRIVER_RS485}\"")
