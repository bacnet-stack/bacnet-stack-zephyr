/**
 * @file
 * @brief BACnet Stack sample Lighting Supervisor (B-LS) main file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
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
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/linear.h"
/* BACnet Stack basic device API -
   see bacnet/basic/server/bacnet_device.c for details */
#include "bacnet/basic/object/device.h"
/* BACnet Stack basic objects enabled in prj.conf */
#include "bacnet/basic/object/blo.h"
#include "bacnet/basic/object/lo.h"
#include "bacnet/basic/object/channel.h"
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#include "bacnet/basic/server/bacnet_basic.h"
#include "bacnet/basic/server/bacnet_port.h"
/* BACnet Stack Zephyr services */
#include <bacnet_settings/bacnet_settings.h>
/* Logging module registration is already done in ports/zephyr/main.c */
#include <bacnet_osif/bacnet_log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

/* Default values before we get the device instance and name from settings */
static const uint32_t Device_Instance = 260126;
static const char *Device_Name = "BACnet Lighting Supervisor (B-LS)";
/* instances for our objects */
static const uint32_t Lighting_Instance = 1;
static const uint32_t Binary_Lighting_Instance = 1;
static const uint32_t Channel_Instance = 1;
/* observer for WriteGroup notifications */
static BACNET_WRITE_GROUP_NOTIFICATION Write_Group_Notification;

/**
 * @brief BACnet Lighting Output tracking value handler
 * @param object-instance [in] The object-instance number of the object
 * @param old_value [in] The value to track
 */
static void Lighting_Output_Tracking_Value_Handler(
    uint32_t object_instance, float old_value, float value)
{
    uint16_t steps = 0;

    (void)old_value;
    if (object_instance != Lighting_Instance) {
        return;
    }
    /* Tracking value are 0.0 and 1.0-100.0 normalized */
    if (isgreaterequal(value, 1.0) && islessequal(value, 100.0)) {
        steps = linear_interpolate(1.0, value, 100.0, 1, UINT16_MAX);
    } else if (isgreater(value, 100.0)) {
        steps = UINT16_MAX;
    } else {
        steps = 0;
    }
    LOG_INF(
        "Lighting Output[%lu]: value=%f step=%u/%u",
        (unsigned long)object_instance, (double)value, (unsigned)steps,
        (unsigned)UINT16_MAX);
}

/**
 * @brief Callback for write value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - value prior to write
 * @param  value - value of the write
 */
void Binary_Lighting_Output_Present_Value_Handler(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV old_value,
    BACNET_BINARY_LIGHTING_PV value)
{
    (void)old_value;
    if (object_instance != Binary_Lighting_Instance) {
        return;
    }
    LOG_INF(
        "Binary Lighting Output[%lu]: value=%d", (unsigned long)object_instance,
        value);
}

/**
 * @brief Callback for blink warning notification
 * @param  object_instance - object-instance number of the object
 */
void Binary_Lighting_Output_Blink_Warn_Handler(uint32_t object_instance)
{
    if (object_instance != Binary_Lighting_Instance) {
        return;
    }
    LOG_INF(
        "Binary Lighting Output[%lu]: Blink Warning",
        (unsigned long)object_instance);
}

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
static void BACnet_Lighting_Device_Init_Handler(void *context)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member;

    (void)context;
    LOG_INF("BACnet Stack Initialized");
    /* initialize objects with default values for this basic sample */
    Device_Set_Object_Instance_Number(Device_Instance);
    Device_Object_Name_ANSI_Init(Device_Name);
    /* lighting output object */
    Lighting_Output_Create(Lighting_Instance);
    Lighting_Output_Name_Set(Lighting_Instance, "Light-1");
    /* binary lighting output object */
    Binary_Lighting_Output_Create(Binary_Lighting_Instance);
    Binary_Lighting_Output_Name_Set(Binary_Lighting_Instance, "Binary-Light-1");
    /* channel object */
    Channel_Create(Channel_Instance);
    Channel_Name_Set(Channel_Instance, "Lights");
    Channel_Number_Set(Channel_Instance, 1);
    Channel_Control_Groups_Element_Set(Channel_Instance, 1, 1);
    /* configure the channel members */
    member.objectIdentifier.type = OBJECT_LIGHTING_OUTPUT;
    member.objectIdentifier.instance = Lighting_Instance;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = Device_Instance;
    Channel_Reference_List_Member_Element_Set(Channel_Instance, 1, &member);
    /* restore any property values previously stored via WriteProperty */
    bacnet_settings_init();
    bacnet_settings_write_property_restore(&Settings_Restore_Callback, NULL);
    /* writable property values are stored with WriteProperty.
       Set this callback after restore to prevent recursion. */
    bacnet_basic_store_callback_set(bacnet_settings_basic_store);
    /* lighting output callbacks */
    Lighting_Output_Write_Present_Value_Callback_Set(
        Lighting_Output_Tracking_Value_Handler);
    Binary_Lighting_Output_Write_Value_Callback_Set(
        Binary_Lighting_Output_Present_Value_Handler);
    Binary_Lighting_Output_Blink_Warn_Callback_Set(
        Binary_Lighting_Output_Blink_Warn_Handler);
    /* link WriteGroup service to our channel object  */
    Write_Group_Notification.callback = Channel_Write_Group;
    handler_write_group_notification_add(&Write_Group_Notification);
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
    /* set the BACnet Basic Task device object timer for lighting output use */
    bacnet_basic_task_object_timer_set(10UL);
    srand(sys_rand32_get());
}

/**
 * @brief BACnet Project Task Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Lighting_Device_Task_Handler(void *context)
{
    (void)context;
}

int main(void)
{
    bool port_initialized = false;

    LOG_INF("BACnet Device: %s", Device_Name);
    LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
    LOG_INF("BACnet Stack Max APDU: %d", MAX_APDU);
    bacnet_basic_init_callback_set(BACnet_Lighting_Device_Init_Handler, NULL);
    bacnet_basic_task_callback_set(BACnet_Lighting_Device_Task_Handler, NULL);
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
