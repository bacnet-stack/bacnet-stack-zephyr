/**
 * @file
 * @brief BACnet OS interface for ReinitializeDevice reboot handling
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_OSIF_BACNET_REINIT_H
#define BACNET_OSIF_BACNET_REINIT_H

#include <stdint.h>

typedef void (*bacnet_reinitialize_device_coldstart_callback)(void *context);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bacnet_reinitialize_device_task(
    bacnet_reinitialize_device_coldstart_callback coldstart_callback,
    void *context);
void bacnet_reinitialize_device_init(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BACNET_OSIF_BACNET_REINIT_H */
