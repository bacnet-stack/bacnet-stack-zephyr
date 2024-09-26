/**
 * @file
 * @brief Logging level configuration for BACnet Stack for Zephyr OS
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_OSIF_BACNET_LOG_H
#define BACNET_OSIF_BACNET_LOG_H
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#ifndef CONFIG_BACNETSTACK_LOG_LEVEL
#define CONFIG_BACNETSTACK_LOG_LEVEL LOG_LEVEL_NONE
#endif

#endif
