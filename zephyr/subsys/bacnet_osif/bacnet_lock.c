/**
 * @file
 * @brief BACnet task lock for stack library data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <bacnet_osif/bacnet_lock.h>

static K_MUTEX_DEFINE(bacnet_mutex);

void bacnet_lock(void)
{
    k_mutex_lock(&bacnet_mutex, K_FOREVER);
}

void bacnet_unlock(void)
{
    k_mutex_unlock(&bacnet_mutex);
}
