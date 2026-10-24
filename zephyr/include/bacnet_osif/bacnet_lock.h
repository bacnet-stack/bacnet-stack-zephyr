/**
 * @file
 * @brief BACnet task lock for stack library data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_OSIF_BACNET_LOCK_H
#define BACNET_OSIF_BACNET_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Take the BACnet task lock.
 * @note The mutex is recursive for the owner and waits forever.
 *  Do not call this from an interrupt. Do not hold it across a long
 *  wait such as DHCP or a confirmed request to another device.
 *  A real-time port thread must not call this. It exchanges PDUs
 *  with the stack through a queue.
 */
void bacnet_lock(void);

/**
 * @brief Release the BACnet task lock.
 */
void bacnet_unlock(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BACNET_OSIF_BACNET_LOCK_H */
