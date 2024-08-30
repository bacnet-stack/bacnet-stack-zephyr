/**
 * @file
 * @brief BACnet Notification Class Objects
 * @date 2022
 * @author Legrand North America, LLC.
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "object.h"
#include "bacnet/basic/object/nc.h"

#if defined(INTRINSIC_REPORTING)
    OBJECT_FUNCTIONS(Notification_Class, NOTIFICATION_CLASS_INFO);
#endif
