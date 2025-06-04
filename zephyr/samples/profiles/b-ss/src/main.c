/**
 * @file
 * @brief BACnet Stack sample Smart Sensor (B-SS) main file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
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
#include "bacnet/basic/sys/mstimer.h"
/* BACnet Stack basic device API -
   see bacnet/basic/server/bacnet_device.c for details */
#include "bacnet/basic/object/device.h"
/* BACnet Stack basic objects - also enable in prj.conf */
#include "bacnet/basic/object/ai.h"
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

static const char *Device_Name = "BACnet Smart Sensor (B-SS)";
/* object instances */
static const uint32_t Sensor_Instance = 1;
/* timer for Sensor Update Interval */
static struct mstimer Sensor_Update_Timer;

/**
 * @brief BACnet Project Initialization Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Smart_Sensor_Init_Handler(void *context)
{
	const float default_temperature = 25.0f;
	uint32_t array_index = BACNET_ARRAY_ALL;
	bool status = false;
	int i;
	int32_t analog_input_writeable_property_list[] = {
		/* list of properties to set via WriteProperty */
		PROP_OUT_OF_SERVICE,
		PROP_PRESENT_VALUE,
		PROP_UNITS,
		PROP_COV_INCREMENT,
	};
	int32_t device_writeable_property_list[] = {
		/* list of properties to set via WriteProperty */
		PROP_OBJECT_IDENTIFIER,
		PROP_OBJECT_NAME,
	};

	(void)context;
	LOG_INF("BACnet Stack Initialized");
	/* initialize objects with default values for this basic sample */
	Device_Object_Name_ANSI_Init(Device_Name);
	Analog_Input_Create(Sensor_Instance);
	Analog_Input_Name_Set(Sensor_Instance, "Sensor");
	Analog_Input_Present_Value_Set(Sensor_Instance, default_temperature);
	Analog_Input_Units_Set(Sensor_Instance, UNITS_DEGREES_CELSIUS);
	Analog_Input_COV_Increment_Set(Sensor_Instance, 1.0f);
	/* restore any property values previously stored via WriteProperty */
	for (i = 0; i < ARRAY_SIZE(device_writeable_property_list); i++) {
		status = bacnet_settings_write_property_restore(
			OBJECT_DEVICE, BACNET_MAX_INSTANCE, device_writeable_property_list[i],
			array_index, Device_Write_Property_Local);
		if (!status) {
			/* no settings stored for this property, use defaults */
		}
	}
	for (i = 0; i < ARRAY_SIZE(analog_input_writeable_property_list); i++) {
		status = bacnet_settings_write_property_restore(
			OBJECT_ANALOG_INPUT, Sensor_Instance,
			analog_input_writeable_property_list[i], array_index,
			Analog_Input_Write_Property);
		if (!status) {
			/* no settings stored for this property, use defaults */
		}
	}
	/* These writable property values are stored WriteProperty.
	   Set this callback after init to prevent recursion. */
	Device_Write_Property_Store_Callback_Set(bacnet_settings_write_property_store);
	LOG_INF("BACnet Device ID: %u", Device_Object_Instance_Number());
	/* start the seconds cyclic timer */
	mstimer_set(&Sensor_Update_Timer, 1000);
	srand(sys_rand32_get());
}

/**
 * @brief BACnet Project Task Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Smart_Sensor_Task_Handler(void *context)
{
	float temperature = 0.0f, change = 0.0f;

	(void)context;
	if (mstimer_expired(&Sensor_Update_Timer)) {
		mstimer_reset(&Sensor_Update_Timer);
		/* simulate a sensor reading, and update the BACnet object values */
		if (Analog_Input_Out_Of_Service(Sensor_Instance)) {
			return;
		}
		temperature = Analog_Input_Present_Value(Sensor_Instance);
		change = -1.0f + 2.0f * ((float)rand()) / RAND_MAX;
		temperature += change;
		Analog_Input_Present_Value_Set(Sensor_Instance, temperature);
	}
}

int main(void)
{
	bool port_initialized = false;

	LOG_INF("BACnet Device: %s", Device_Name);
	LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
	LOG_INF("BACnet Stack Max APDU: %d", MAX_APDU);
	bacnet_basic_init_callback_set(BACnet_Smart_Sensor_Init_Handler, NULL);
	bacnet_basic_task_callback_set(BACnet_Smart_Sensor_Task_Handler, NULL);
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
