/**
 * @file
 * @brief BACnet shell commands for debugging and testing
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/bacapp.h"
/* BACnet objects API */
#include "bacnet/basic/object/device.h"

/**
 * @brief List all BACnet objects in this device
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_objects(const struct shell *sh, size_t argc, char **argv)
{
	int count;
	BACNET_OBJECT_TYPE object_type;
	uint32_t instance;
	uint32_t array_index;
	bool found;

	(void)argc;
	(void)argv;
	/* display the object-list as well formed JSON */
	shell_print(sh, "{\"%s\": [", bactext_property_name(PROP_OBJECT_LIST));
	count = Device_Object_List_Count();
	for (array_index = 1; array_index <= count; array_index++) {
		found = Device_Object_List_Identifier(array_index, &object_type, &instance);
		if (found) {
			shell_print(sh, "{\"%s\":{\"%s\":%u}}%s",
				    bactext_property_name(PROP_OBJECT_IDENTIFIER),
				    bactext_object_type_name(object_type), instance,
				    (array_index == count) ? "]," : ",");
		}
	}
	shell_print(sh, "\"object-list-size\": %d}", count);

	return 0;
}

SHELL_SUBCMD_ADD((bacnet), objects, NULL, "list of BACnet objects", cmd_objects, 1, 0);
