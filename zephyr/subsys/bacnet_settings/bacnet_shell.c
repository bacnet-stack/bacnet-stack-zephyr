/**
 * @file
 * @brief The BACnet shell commands for debugging and testing
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
#include <bacnet_settings/bacnet_storage.h>
#include <bacnet/bactext.h>
#include <bacnet/wp.h>

static const char Storage_Base_Name[] = CONFIG_BACNET_STORAGE_BASE_NAME;

/**
 * @brief Get or set a string using BACnet storage subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_key(BACNET_STORAGE_KEY *key, const struct shell *sh, size_t argc, char **argv)
{
	uint16_t object_type;
	uint32_t object_instance;
	uint32_t property_id = 77;
	uint32_t array_index = BACNET_STORAGE_ARRAY_INDEX_NONE;
	long value = 0;
	int found_index = 0;

	if (argc < 3) {
		shell_error(sh, "Usage: %s <object-type> <instance> <property> [value]", argv[0]);
		return -EINVAL;
	}
	if (bactext_object_type_strtol(argv[1], &found_index)) {
		value = found_index;
	} else {
		shell_error(sh, "Invalid object-type: %s.", argv[1]);
		return -EINVAL;
	}
	if ((value < 0) || (value >= UINT16_MAX)) {
		shell_error(sh, "Invalid object-type: %s. Must be 0-65535.", argv[1]);
		return -EINVAL;
	}
	object_type = (uint16_t)value;
	value = strtoul(argv[2], NULL, 0);
	if (value > 4194303) {
		shell_error(sh, "Invalid object-instance: %s. Must be 0-4194303.", argv[2]);
		return -EINVAL;
	}
	object_instance = (uint32_t)value;
	if (bactext_property_strtol(argv[3], &found_index)) {
		value = found_index;
	} else {
		shell_error(sh, "Invalid property: %s.", argv[1]);
		return -EINVAL;
	}
	if (value > UINT32_MAX) {
		shell_error(sh, "Invalid property: %s. Must be 0-4294967295.", argv[3]);
		return -EINVAL;
	}
	property_id = (uint32_t)value;
	/* setup the storage key */
	bacnet_storage_key_init(key, object_type, object_instance, property_id, array_index);

	return 0;
}

/**
 * @brief Get or set a string using BACnet storage subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_string(const struct shell *sh, size_t argc, char **argv)
{
	char key_name[BACNET_STORAGE_KEY_SIZE_MAX + 1] = {0};
	uint8_t data[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = {0};
	BACNET_STORAGE_KEY key = {0}, test_key = {0};
	size_t arg_len = 0;
	int rc;

	rc = cmd_key(&key, sh, argc, argv);
	if (rc < 0) {
		return rc;
	}
	/* convert the key to a string for the shell */
	(void)bacnet_storage_key_encode(key_name, sizeof(key_name), &key);
	/* convert the key string to numbers for a test */
	if (bacnet_storage_key_decode(key_name, &test_key) == 0) {
		shell_print(sh, "key=%s/%lu/%lu/%lu/%lu", Storage_Base_Name,
			    (unsigned long)test_key.object_type,
			    (unsigned long)test_key.object_instance,
			    (unsigned long)test_key.property_id,
			    (unsigned long)test_key.array_index);
	}
	if (argc > 4) {
		arg_len = strlen(argv[4]);
		rc = bacnet_storage_set(&key, argv[4], arg_len);
		if (rc == 0) {
			shell_print(sh, "Set %s = %s", key_name, argv[4]);
		} else {
			shell_error(sh, "Unable to set %s = %s", key_name, argv[4]);
			return -EINVAL;
		}
	} else {
		rc = bacnet_storage_get(&key, data, sizeof(data));
		if (rc < 0) {
			shell_error(sh, "Unable to get %s", key_name);
			return -EINVAL;
		}
		shell_print(sh, "Get %s = %s", key_name, data);
	}

	return 0;
}

/**
 * @brief Get or set a string using BACnet storage subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_delete(const struct shell *sh, size_t argc, char **argv)
{
	char key_name[BACNET_STORAGE_KEY_SIZE_MAX + 1] = {0};
	BACNET_STORAGE_KEY key = {0};
	int rc;

	rc = cmd_key(&key, sh, argc, argv);
	if (rc < 0) {
		return rc;
	}
	/* convert the key to a string for the shell */
	(void)bacnet_storage_key_encode(key_name, sizeof(key_name), &key);
	if (argc > 3) {
		rc = bacnet_storage_delete(&key);
		if (rc == 0) {
			shell_print(sh, "Deleted %s", key_name);
		} else {
			shell_error(sh, "Unable to delete %s", key_name);
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * @brief Callback from the Zephyr settings_restore iterator
 * @param key [in] The BACnet object type
 * @param data [in] The data to restore
 * @param data_len [in] The length of the data
 * @return 0 on success, negative on failure.
 */
static int print_storage_data(BACNET_STORAGE_KEY *key, const void *data, size_t data_len,
			      void *context)
{
	const struct shell *sh = context;
	if (key->array_index == BACNET_STORAGE_ARRAY_INDEX_NONE) {
		shell_print(sh, "%s/%u/%u/%u", Storage_Base_Name, key->object_type,
			    key->object_instance, key->property_id);
	} else {
		shell_print(sh, "%s/%u/%u/%u/%u", Storage_Base_Name, key->object_type,
			    key->object_instance, key->property_id, key->array_index);
	}

	return 0;
}

/**
 * @brief List all the BACnet stored settings
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_list(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bacnet_storage_load_callback_set(print_storage_data, (void *)sh);
	if (err) {
		shell_error(sh, "Failed to set storage load callback");
		return -EINVAL;
	}
	err = bacnet_storage_load();
	if (err) {
		shell_error(sh, "Failed to load storage");
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_bacnet_settings_cmds, SHELL_CMD(list, NULL, "list BACnet storage strings", cmd_list),
	SHELL_CMD(string, NULL, "get or set BACnet storage string", cmd_string),
	SHELL_CMD(delete, NULL, "delete BACnet storage string", cmd_delete), SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((bacnet), settings, &sub_bacnet_settings_cmds, "BACnet settings commands", NULL, 1,
		 0);
