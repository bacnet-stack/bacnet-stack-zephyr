/**
 * @file
 * @brief BACnet Stack sample Application Specific Controller (B-ASC) main file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2025
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <stdint.h>
#include <stdlib.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack core API */
#include "bacnet/version.h"
#include "bacnet/datetime.h"
#include "bacnet/bactext.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/bramfs.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/linear.h"
/* BACnet Stack basic device API -
   see bacnet/basic/server/bacnet_device.c for details */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/server/bacnet_basic.h"
#include "bacnet/basic/server/bacnet_port.h"
/* BACnet Stack Zephyr services */
#include <bacnet_settings/bacnet_settings.h>
/* Logging module registration is already done in ports/zephyr/main.c */
#include <bacnet_osif/bacnet_log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

/* Default values before we get the device instance and name from settings */
static const uint32_t Device_Instance = 260127;
static const char *Device_Name =
    "BACnet Application Specific Controller (B-ASC)";

/**
 * @brief Callback data for WriteProperty restore iterator
 * @param write_function The WriteProperty function to call
 * @param context The context to pass to the WriteProperty function
 * @return true if the WriteProperty succeeded
 */
static bool
Settings_Restore_Callback(BACNET_WRITE_PROPERTY_DATA *wp_data, void *context)
{
    (void)context;
    return Device_Write_Property(wp_data);
}

/**
 * @brief BACnet Project Initialization Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Device_Init_Handler(void *context)
{
    BACNET_CREATE_OBJECT_DATA object_data = { 0 };
    unsigned int i = 0;
    uint32_t instance;
    const char *empty_data = "empty";

    (void)context;
    LOG_INF("BACnet Stack Initialized");
    /* create some dynamically created objects as examples */
    object_data.object_instance = BACNET_MAX_INSTANCE;
    for (i = 0; i <= BACNET_OBJECT_TYPE_RESERVED_MIN; i++) {
        object_data.object_type = i;
        if (Device_Create_Object(&object_data)) {
            LOG_INF(
                "Created object %s-%u\n", bactext_object_type_name(i),
                (unsigned)object_data.object_instance);
        }
    }
    /* Backup and restore uses a file */
    instance = bacfile_index_to_instance(0);
    if (instance >= BACNET_MAX_INSTANCE) {
        LOG_ERR("No file object instance available for backup/restore");
    } else {
        bacfile_pathname_set(instance, "bacnet.dat");
        Device_Configuration_File_Set(0, instance);
        /* use a RAM FS backend for this sample */
        bacfile_ramfs_init();
        bacfile_ramfs_write_stream_data(
            bacfile_pathname(instance), 0, empty_data, strlen(empty_data));
        LOG_INF(
            "Backup/Restore file-%u path=%s", instance,
            bacfile_pathname(instance));
    }
    /* initialize objects with default values for this basic sample */
    Device_Set_Object_Instance_Number(Device_Instance);
    Device_Object_Name_ANSI_Init(Device_Name);
    /* restore any property values previously stored via WriteProperty */
    bacnet_settings_init();
    bacnet_settings_write_property_restore(&Settings_Restore_Callback, NULL);
    /* writable property values are stored with WriteProperty.
       Set this callback after restore to prevent recursion. */
    bacnet_basic_store_callback_set(bacnet_settings_basic_store);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WRITE_GROUP, handler_write_group);
    /* initialize timesync callback function. */
    /* local time and date */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    handler_timesync_set_callback_set(datetime_timesync);
    datetime_init();
    /* done */
    LOG_INF("BACnet Device ID: %u", Device_Object_Instance_Number());
    bacnet_basic_task_object_timer_set(1000UL);
    srand(sys_rand32_get());
}

/**
 * @brief BACnet Project Task Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Device_Task_Handler(void *context)
{
    (void)context;
}

int main(void)
{
    bool port_initialized = false;

    LOG_INF("BACnet Device: %s", Device_Name);
    LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
    LOG_INF("BACnet Stack Max APDU: %d", MAX_APDU);
    bacnet_basic_init_callback_set(BACnet_Device_Init_Handler, NULL);
    bacnet_basic_task_callback_set(BACnet_Device_Task_Handler, NULL);
    bacnet_basic_init();
    for (;;) {
        k_sleep(K_MSEC(CONFIG_BACNET_BASIC_SERVER_KSLEEP));
        bacnet_basic_task();
        if (port_initialized) {
            bacnet_port_task();
        } else {
            port_initialized = bacnet_port_init();
        }
    }

    return 0;
}
