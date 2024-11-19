/**
 * @file
 * @brief Implementation of global websocket functions.
 * @author Kirill Neznamov
 * @date May 2022
 * @copyright SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "websocket-global.h"

static K_MUTEX_DEFINE(websocket_mutex);
static K_MUTEX_DEFINE(websocket_dispatch_mutex);

void bsc_websocket_global_lock(void)
{
    k_mutex_lock(&websocket_mutex, K_FOREVER);
}

void bsc_websocket_global_unlock(void)
{
    k_mutex_unlock(&websocket_mutex);
}

void bws_dispatch_lock(void)
{
    k_mutex_lock(&websocket_dispatch_mutex, K_FOREVER);
}

void bws_dispatch_unlock(void)
{
    k_mutex_unlock(&websocket_dispatch_mutex);
}
