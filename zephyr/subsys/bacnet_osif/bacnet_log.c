/*
 * Copyright (C) 2020 Legrand North America, Inc.
 *
 * SPDX-License-Identifier: MIT
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
