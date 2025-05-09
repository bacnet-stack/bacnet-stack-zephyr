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
/* BACnet Stack basic device API - see bacnet_basic/device.c for details */
#include "bacnet/basic/object/device.h"
/* BACnet Stack basic objects enabled in prj.conf */
#include "bacnet/basic/object/ao.h"
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#include "bacnet/basic/server/bacnet_basic.h"
#include "bacnet/basic/server/bacnet_port.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include "bacnet_osif/bacnet_log.h"
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

/* FIXME: get the device instance and name from settings! */
static const uint32_t Device_Instance = 260124;
static const char *Device_Name = "BACnet Smart Actuator (B-SA)";
/* object instances */
static const uint32_t Actuator_Instance = 1;
/* timer for Actuator Update Interval */
static struct mstimer Actuator_Update_Timer;

static void BACnet_Smart_Actuator_Datalink_Init(void)
{
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
	/* initialize objects for this basic sample */
	Device_Init(NULL);
	Device_Set_Object_Instance_Number(Device_Instance);
	Device_Object_Name_ANSI_Init(Device_Name);
	Analog_Output_Create(Actuator_Instance);
	Analog_Output_Name_Set(Actuator_Instance, "Actuator");
	Analog_Output_Units_Set(Actuator_Instance, UNITS_PERCENT);
	Analog_Output_Min_Pres_Value_Set(Actuator_Instance, 0.0f);
	Analog_Output_Max_Pres_Value_Set(Actuator_Instance, 100.0f);
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
	LOG_INF("BACnet Device: %s", Device_Name);
	LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
	LOG_INF("BACnet Stack Max APDU: %d", MAX_APDU);
	bacnet_basic_init_callback_set(BACnet_Smart_Actuator_Init_Handler, NULL);
	bacnet_basic_task_callback_set(BACnet_Smart_Actuator_Task_Handler, NULL);
	bacnet_basic_init();
	for (;;) {
		if (bacnet_port_init()) {
			break;
		} else {
			LOG_ERR("Server: port initialization failed");
			k_sleep(K_MSEC(1000));
		}
	}
	for (;;) {
		k_sleep(K_MSEC(CONFIG_BACNET_BASIC_SERVER_KSLEEP));
		bacnet_basic_task();
		bacnet_port_task();
	}

	return 0;
}
