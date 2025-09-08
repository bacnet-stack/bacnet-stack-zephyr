/**
 * @file
 * @brief BACnet Stack sample Smart Actuator (B-SA) main file
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
/* BACnet Stack basic objects enabled in prj.conf */
#include "bacnet/basic/object/ao.h"
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
static const uint32_t Device_Instance = 260124;
static const char *Device_Name = "BACnet Smart Actuator (B-SA)";
/* object instances */
static const uint32_t Actuator_Instance = 1;
/* timer for Actuator Update Interval */
static struct mstimer Actuator_Update_Timer;

static void BACnet_Smart_Actuator_Datalink_Init(void)
{
	/* nothing to do */
}

/**
 * @brief Callback data for WriteProperty restore iterator
 * @param write_function The WriteProperty function to call
 * @param context The context to pass to the WriteProperty function
 * @return true if the WriteProperty succeeded
 */
static bool Settings_Restore_Callback(BACNET_WRITE_PROPERTY_DATA *wp_data, void *context)
{
	(void)context;
	return Device_Write_Property(wp_data);
}

/**
 * @brief BACnet Project Initialization Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Smart_Actuator_Init_Handler(void *context)
{
	(void)context;
	LOG_INF("BACnet Stack Initialized");
	BACnet_Smart_Actuator_Datalink_Init();
	/* initialize objects with default values for this basic sample */
	Device_Init(NULL);
	Device_Set_Object_Instance_Number(Device_Instance);
	Device_Object_Name_ANSI_Init(Device_Name);
	Analog_Output_Create(Actuator_Instance);
	Analog_Output_Name_Set(Actuator_Instance, "Actuator");
	Analog_Output_Units_Set(Actuator_Instance, UNITS_PERCENT);
	Analog_Output_Min_Pres_Value_Set(Actuator_Instance, 0.0f);
	Analog_Output_Max_Pres_Value_Set(Actuator_Instance, 100.0f);
	/* restore any property values previously stored via WriteProperty */
	bacnet_settings_init();
	bacnet_settings_write_property_restore(&Settings_Restore_Callback, NULL);
	/* writable property values are stored with WriteProperty.
	   Set this callback after restore to prevent recursion. */
	bacnet_basic_store_callback_set(bacnet_settings_basic_store);
	LOG_INF("BACnet Device ID: %u", Device_Object_Instance_Number());
	/* start the seconds cyclic timer */
	mstimer_set(&Actuator_Update_Timer, 1000);
	srand(sys_rand32_get());
}

/**
 * @brief BACnet Project Task Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Smart_Actuator_Task_Handler(void *context)
{
	float percent = 0.0f, change = 0.0f;

	(void)context;
	if (mstimer_expired(&Actuator_Update_Timer)) {
		mstimer_reset(&Actuator_Update_Timer);
		/* simulate an internal software program,
		   and update the BACnet object values */
		if (Analog_Output_Out_Of_Service(Actuator_Instance)) {
			return;
		}
		percent = Analog_Output_Present_Value(Actuator_Instance);
		change = -1.0f + 2.0f * ((float)rand()) / RAND_MAX;
		percent += change;
		Analog_Output_Present_Value_Set(Actuator_Instance, percent, BACNET_MAX_PRIORITY);
	}
}

int main(void)
{
	bool port_initialized = false;

	LOG_INF("BACnet Device: %s", Device_Name);
	LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
	LOG_INF("BACnet Stack Max APDU: %d", MAX_APDU);
	bacnet_basic_init_callback_set(BACnet_Smart_Actuator_Init_Handler, NULL);
	bacnet_basic_task_callback_set(BACnet_Smart_Actuator_Task_Handler, NULL);
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
