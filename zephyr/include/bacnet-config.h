/**
 * @file
 * @brief Port specific configuration for BACnet Stack for Zephyr OS
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_PORTS_ZEPHYR_BACNET_CONFIG_H
#define BACNET_PORTS_ZEPHYR_BACNET_CONFIG_H

#if ! defined BACNET_CONFIG_H || ! BACNET_CONFIG_H
#error bacnet-config.h included outside of BACNET_CONFIG_H control
#endif

/* provides platform specific define for ARRAY_SIZE */
#include <zephyr/sys/util.h>
#endif
