/**
 * @file
 * @brief operating system interface for logging
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
/* Standard includes */
#include <stdint.h>

/* Zephyr includes */
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_BACNETSTACK_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bacnet);
