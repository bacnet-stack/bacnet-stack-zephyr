/**
 * @file
 * @brief The BACnet shell commands for debugging and testing settings
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <zephyr/shell/shell.h>
#include <bacnet_settings/bacnet_storage.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
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
	uint16_t object_type = 0;
	uint32_t object_instance = 0;
	uint32_t property_id = 75;
	uint32_t array_index = BACNET_STORAGE_ARRAY_INDEX_NONE;
	long value = 0;
	unsigned long unsigned_value = 0;
	int found_index = 0, scan_count = 0;
	char property_name[80] = {0};

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
	if (!bacnet_storage_strtoul(argv[2], &unsigned_value)) {
		shell_error(sh, "Invalid object-instance: %s.", argv[2]);
		return -EINVAL;
	}
	if (unsigned_value > 4194303) {
		shell_error(sh, "Invalid object-instance: %s. Must be 0-4194303.", argv[2]);
		return -EINVAL;
	}
	object_instance = (uint32_t)unsigned_value;
	/* property can have [] to denote priority or array */
	scan_count = sscanf(argv[3], "%lu[%u]", &unsigned_value, &array_index);
	if (scan_count < 1) {
		scan_count = sscanf(argv[3], "%80s[%u]", property_name, &array_index);
		if (scan_count < 1) {
			shell_error(sh, "Invalid property: %s.", argv[3]);
			return -EINVAL;
		}
		if (bactext_property_strtol(property_name, &found_index)) {
			value = found_index;
		} else {
			shell_error(sh, "Invalid property: %s.", property_name);
			return -EINVAL;
		}
		if (value > UINT32_MAX) {
			shell_error(sh, "Invalid property: %s. Must be 0-4294967295.", argv[3]);
			return -EINVAL;
		}
		unsigned_value = (uint32_t)value;
	}
	if (scan_count < 2) {
		array_index = BACNET_STORAGE_ARRAY_INDEX_NONE;
	}
	property_id = (uint32_t)unsigned_value;
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
static int cmd_value(const struct shell *sh, size_t argc, char **argv)
{
	char key_name[BACNET_STORAGE_KEY_SIZE_MAX + 1] = {0};
	uint8_t data[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = {0};
	BACNET_STORAGE_KEY key = {0};
	int rc, len, data_len;
	unsigned enumerated_value = 0;
	unsigned long unsigned_value = 0;
	float real_value = 0.0f;
	long signed_value = 0;
	int scan_count = 0;
	unsigned object_type = 0;
	unsigned instance = 0;
	bool status = false;
	BACNET_APPLICATION_DATA_VALUE value = {0};
	BACNET_OBJECT_PROPERTY_VALUE object_value = {0};
	char value_name[80] = {0};
	char *value_string = NULL;

	rc = cmd_key(&key, sh, argc, argv);
	if (rc < 0) {
		return rc;
	}
	/* read the current value which also determines
	   the tag when setting the value */
	rc = bacnet_storage_get(&key, data, sizeof(data));
	if (rc < 0) {
		shell_error(sh, "Unable to get %s", key_name);
		return -EINVAL;
	}
	data_len = rc;
	/* convert the key to a string for the shell to print */
	(void)bacnet_storage_key_encode(key_name, sizeof(key_name), &key);
	/* check for an assigned tag */
	if ((argc > 5) && bacnet_storage_strtoul(argv[4], &unsigned_value)) {
		value.tag = unsigned_value;
		value_string = argv[5];
	} else if (argc > 4) {
		value_string = argv[4];
	}
	if (value_string) {
		len = bacapp_decode_application_data(data, data_len, &value);
		/* convert the string value into a tagged union value */
		if (isalpha(value_string[0])) {
			if (property_list_commandable_member(key.object_type, key.property_id) &&
			    (bacnet_strnicmp(value_string, "NULL", 4) == 0)) {
				/* check for case insensitive NULL string */
				value.tag = BACNET_APPLICATION_TAG_NULL;
				status = true;
			} else if (bacnet_stricmp(value_string, "true") == 0) {
				value.type.Boolean = true;
				value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
				status = true;
			} else if (bacnet_stricmp(value_string, "false") == 0) {
				value.type.Boolean = false;
				value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
				status = true;
			} else {
				status = bactext_object_property_strtoul(
					(BACNET_OBJECT_TYPE)key.object_type,
					(BACNET_PROPERTY_ID)key.property_id, value_string,
					&enumerated_value);
				if (status) {
					value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
					value.type.Enumerated = (uint32_t)enumerated_value;
				}
			}
		}
		if (!status) {
			switch (value.tag) {
			case BACNET_APPLICATION_TAG_ENUMERATED:
				if (bacnet_storage_strtoul(value_string, &unsigned_value)) {
					value.type.Enumerated = (uint32_t)unsigned_value;
					status = true;
				}
				break;
			case BACNET_APPLICATION_TAG_UNSIGNED_INT:
				if (bacnet_storage_strtoul(value_string, &unsigned_value)) {
					value.type.Unsigned_Int = (uint32_t)unsigned_value;
					status = true;
				}
				break;
			case BACNET_APPLICATION_TAG_SIGNED_INT:
				if (bacnet_storage_strtol(value_string, &signed_value)) {
					value.type.Signed_Int = (int32_t)signed_value;
					status = true;
				}
				break;
			case BACNET_APPLICATION_TAG_REAL:
				if (bacnet_storage_strtof(value_string, &real_value)) {
					value.type.Real = (float)real_value;
					status = true;
				}
				break;
			case BACNET_APPLICATION_TAG_BIT_STRING:
				status = bitstring_init_ascii(&value.type.Bit_String, value_string);
				break;
			case BACNET_APPLICATION_TAG_OCTET_STRING:
				status = octetstring_init_ascii_hex(&value.type.Octet_String,
								    value_string);
				break;
			case BACNET_APPLICATION_TAG_CHARACTER_STRING:
				status = characterstring_init_ansi(&value.type.Character_String,
								   value_string);
				break;
			case BACNET_APPLICATION_TAG_DATE:
				status = datetime_date_init_ascii(&value.type.Date, value_string);
				break;
			case BACNET_APPLICATION_TAG_TIME:
				status = datetime_time_init_ascii(&value.type.Time, value_string);
				break;
			case BACNET_APPLICATION_TAG_OBJECT_ID:
				scan_count =
					sscanf(value_string, "%4d:%7u", &object_type, &instance);
				if (scan_count == 2) {
					value.type.Object_Id.type = (uint16_t)object_type;
					value.type.Object_Id.instance = instance;
				} else {
					status = false;
				}
				break;
			default:
				break;
			}
		}
		if (status) {
			shell_print(sh, "Parsed %s = %s as tag=%u", key_name, value_string,
				    value.tag);
			len = bacapp_encode_application_data(NULL, &value);
			if (len <= 0) {
				return false;
			} else if (len > sizeof(data)) {
				return false;
			}
			len = bacapp_encode_application_data(data, &value);
			rc = bacnet_storage_set(&key, data, len);
			if (rc == 0) {
				shell_print(sh, "Set %s = %s", key_name, value_string);
			} else {
				shell_error(sh, "Unable to set %s = %s", key_name, value_string);
				return -EINVAL;
			}
		} else {
			shell_error(sh, "Unable to parse value for %s = %s", key_name,
				    value_string);
			return -EINVAL;
		}
	} else {
		/* convert to printable value */
		len = bacapp_decode_known_property(data, rc, &value, key.object_type,
						   key.property_id);
		if (len < 0) {
			shell_error(sh, "Unable to decode value for %s", key_name);
			return -EINVAL;
		}
		object_value.object_type = key.object_type;
		object_value.object_instance = key.object_instance;
		object_value.object_property = key.property_id;
		object_value.array_index = key.array_index;
		object_value.value = &value;
		bacapp_snprintf_value(value_name, sizeof(value_name), &object_value);
		shell_print(sh, "Get %s = %s", key_name, value_name);
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
	char data_string[80] = {0};
	char hex_string[3] = {0};
	unsigned i;
	const struct shell *sh = context;

	for (i = 0; i < data_len && i < ((sizeof(data_string) / 2) - 1); i++) {
		snprintf(hex_string, sizeof(hex_string), "%02X", ((const uint8_t *)data)[i]);
		strcat(data_string, hex_string);
	}
	if (key->array_index == BACNET_STORAGE_ARRAY_INDEX_NONE) {
		shell_print(sh, "%s/%u/%u/%u=%s", Storage_Base_Name, key->object_type,
			    key->object_instance, key->property_id, data_string);
	} else {
		shell_print(sh, "%s/%u/%u/%u/%u=%s", Storage_Base_Name, key->object_type,
			    key->object_instance, key->property_id, key->array_index, data_string);
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
	SHELL_CMD(value, NULL, "<object type> <object instance> <property id>[array index] [value]",
		  cmd_value),
	SHELL_CMD(string, NULL,
		  "<object type> <object instance> <property id>[array index] [string]",
		  cmd_string),
	SHELL_CMD(delete, NULL, "<object type> <object instance> <property id>[array index]",
		  cmd_delete),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((bacnet), settings, &sub_bacnet_settings_cmds, "BACnet settings commands", NULL, 1,
		 0);
