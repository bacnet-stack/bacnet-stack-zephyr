/**
 * @file
 * @brief The BACnet datalink initialization functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2025
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_DATALINK_MSTP_INIT_H
#define BACNET_DATALINK_MSTP_INIT_H

#include <stdint.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void mstp_init_uuid(const uint8_t *new_uuid, size_t length);
void mstp_init_mac(uint8_t mac);
void mstp_init_baud(uint32_t baud);
void mstp_init_max_master(uint8_t max_master);
void mstp_init_port(uint8_t mac, uint32_t baud, uint8_t max_master);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
