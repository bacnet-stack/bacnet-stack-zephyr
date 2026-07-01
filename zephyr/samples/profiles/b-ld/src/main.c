/**
 * @file
 * @brief BACnet Stack sample Lighting Device (B-LD) main file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack core API */
#include "bacnet/version.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/linear.h"
/* BACnet Stack basic device API -
   see bacnet/basic/server/bacnet_device.c for details */
#include "bacnet/basic/object/device.h"
/* BACnet Stack basic objects enabled in prj.conf */
#include "bacnet/basic/object/lo.h"
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
static const char *Device_Name = "BACnet Lighting Device (B-LD)";
static const uint32_t Device_Instance = 260125;
/* object instances */
static const uint32_t Lighting_Instance = 1;

#define HAS_PWM_LEDS DT_HAS_COMPAT_STATUS_OKAY(pwm_leds)
#if HAS_PWM_LEDS
#define LED_PWM_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(pwm_leds)
#define NUM_LEDS DT_NUM_CHILD_STATUS_OKAY(LED_PWM_NODE_ID)
#endif

/**
 * @brief BACnet Lighting Output tracking value handler
 * @param object-instance [in] The object-instance number of the object
 * @param old_value [in] The value to track
 */
void BACnet_Lighting_Output_Tracking_Value_Handler(
    uint32_t object_instance, float old_value, float value)
{
    uint8_t steps = 0;

    (void)old_value;
    if (object_instance != Lighting_Instance) {
        return;
    }
    /* Tracking value are 0.0 and 1.0-100.0 normalized */
    if (isgreaterequal(value, 1.0) && islessequal(value, 100.0)) {
        steps = (uint8_t)linear_interpolate(1.0, value, 100.0, 1, 100.0);
    } else if (isgreater(value, 100.0)) {
        steps = 100;
    } else {
        steps = 0;
    }
    LOG_INF(
        "Lighting Output[%lu]: value=%f step=%u/%u",
        (unsigned long)object_instance, (double)value, (unsigned)steps,
        (unsigned)UINT8_MAX);
#if HAS_PWM_LEDS
    int err;
    uint8_t led = 0;
    const struct device *led_pwm;

    /* hardware control */
    led_pwm = DEVICE_DT_GET(LED_PWM_NODE_ID);
    if (!device_is_ready(led_pwm)) {
        LOG_ERR("Device %s is not ready", led_pwm->name);
        return;
    }
    if (!NUM_LEDS) {
        LOG_ERR("No LEDs found for %s", led_pwm->name);
        return;
    }
    if (object_instance > 0U) {
        led = object_instance - 1U;
    }
    err = led_set_brightness(led_pwm, led, steps);
    if (err < 0) {
        LOG_ERR(
            "Failed to set brightness of LED %u to %u: %d", (unsigned)led,
            (unsigned)steps, err);
    }
#else
    LOG_DBG("No pwm-leds devicetree node: skipping hardware LED update");
#endif
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
    if (wp_data == NULL) {
        return false;
    }
    if ((wp_data->object_type == OBJECT_DEVICE) &&
        (wp_data->object_instance == BACNET_MAX_INSTANCE)) {
        wp_data->object_instance = Device_Object_Instance_Number();
    }

    return Device_Write_Property(wp_data);
}

/**
 * @brief BACnet Project Initialization Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Lighting_Device_Init_Handler(void *context)
{
    (void)context;
    LOG_INF("BACnet Stack Initialized");
    /* initialize objects with default values for this basic sample */
    Device_Set_Object_Instance_Number(Device_Instance);
    Device_Object_Name_ANSI_Init(Device_Name);
    /* lighting output object */
    Lighting_Output_Create(Lighting_Instance);
    Lighting_Output_Name_Set(Lighting_Instance, "Light-1");
    /* restore any property values previously stored via WriteProperty */
    bacnet_settings_init();
    bacnet_settings_write_property_restore(&Settings_Restore_Callback, NULL);
    /* writable property values are stored with WriteProperty.
       Set this callback after restore to prevent recursion. */
    bacnet_basic_store_callback_set(bacnet_settings_basic_store);
    /* lighting output callbacks */
    Lighting_Output_Write_Present_Value_Callback_Set(
        BACnet_Lighting_Output_Tracking_Value_Handler);
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
