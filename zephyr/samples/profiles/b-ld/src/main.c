/**
 * @file
 * @brief BACnet Stack sample Lighting Device (B-LD) main file
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
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/linear.h"
/* BACnet Stack basic device API - see bacnet_basic/device.c for details */
#include "bacnet/basic/object/device.h"
/* BACnet Stack basic objects enabled in prj.conf */
#include "bacnet/basic/object/lo.h"
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#include "bacnet_basic/bacnet_basic.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include "bacnet_osif/bacnet_log.h"
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

/* FIXME: get the device instance and name from settings! */
static const uint32_t Device_Instance = 260125;
static const char *Device_Name = "BACnet Lighting Device (B-LD)";
/* object instances */
static const uint32_t Lighting_Instance = 1;

/**
 * @brief BACnet Lighting Output tracking value handler
 * @param object-instance [in] The object-instance number of the object
 * @param old_value [in] The value to track
 */
void BACnet_Lighting_Output_Tracking_Value_Handler(uint32_t object_instance,
						   float old_value, float value)
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
	LOG_INF("Lighting Output[%lu]: value=%f step=%u/%u",
		(unsigned long)object_instance, (double)value, (unsigned)steps,
		(unsigned)UINT16_MAX);
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
	/* initialize objects for this basic sample */
	Device_Init(NULL);
	Device_Set_Object_Instance_Number(Device_Instance);
	Device_Object_Name_ANSI_Init(Device_Name);
	Lighting_Output_Create(Lighting_Instance);
	Lighting_Output_Name_Set(Lighting_Instance, "Light-1");
	Lighting_Output_Write_Present_Value_Callback_Set(
		BACnet_Lighting_Output_Tracking_Value_Handler);
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
	LOG_INF("BACnet Device: %s", Device_Name);
	LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
	LOG_INF("BACnet Stack Max APDU: %d", MAX_APDU);
	bacnet_basic_init_callback_set(BACnet_Lighting_Device_Init_Handler,
				       NULL);
	bacnet_basic_task_callback_set(BACnet_Lighting_Device_Task_Handler,
				       NULL);
	/* work happens in server module */
	for (;;) {
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
