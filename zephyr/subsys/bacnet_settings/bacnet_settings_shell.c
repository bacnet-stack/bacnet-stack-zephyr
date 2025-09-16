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
#include <zephyr/shell/shell.h>
#include <bacnet_settings/bacnet_storage.h>
#include <bacnet_settings/bacnet_settings.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
#include <bacnet/wp.h>

static const char Storage_Base_Name[] = CONFIG_BACNET_STORAGE_BASE_NAME;

/**
 * @brief Get or set a string using BACnet settings subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_value(const struct shell *sh, size_t argc, char **argv)
{
    char key_name[BACNET_STORAGE_KEY_SIZE_MAX + 1] = { 0 };
    uint8_t data[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
    BACNET_STORAGE_KEY key = { 0 };
    int rc, len, data_len;
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_OBJECT_PROPERTY_VALUE object_value = { 0 };
    unsigned long unsigned_value = 0;
    char value_name[80] = { 0 };
    char *value_string = NULL;

    rc = bacnet_storage_key_parse(&key, argc, argv);
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
        status = bacnet_settings_value_parse(
            value_string, key.object_type, key.property_id, &value);
        if (status) {
            shell_print(
                sh, "Parsed %s = %s as tag=%u", key_name, value_string,
                value.tag);
            len = bacapp_encode_application_data(NULL, &value);
            if (len <= 0) {
                return -ENOTSUP;
            } else if (len > sizeof(data)) {
                return -EINVAL;
            }
            len = bacapp_encode_application_data(data, &value);
            if (len <= 0) {
                return -EINVAL;
            }
            rc = bacnet_storage_set(&key, data, len);
            if (rc == 0) {
                shell_print(sh, "Set %s = %s", key_name, value_string);
            } else {
                shell_error(
                    sh, "Unable to set %s = %s", key_name, value_string);
                return -EINVAL;
            }
        } else {
            shell_error(
                sh, "Unable to parse value for %s = %s", key_name,
                value_string);
            return -EINVAL;
        }
    } else {
        /* convert to printable value */
        len = bacapp_decode_known_property(
            data, rc, &value, key.object_type, key.property_id);
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
 * @brief Get or set a string using BACnet settings subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_delete(const struct shell *sh, size_t argc, char **argv)
{
    char key_name[BACNET_STORAGE_KEY_SIZE_MAX + 1] = { 0 };
    BACNET_STORAGE_KEY key = { 0 };
    int rc;

    rc = bacnet_storage_key_parse(&key, argc, argv);
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
static int print_storage_data(
    BACNET_STORAGE_KEY *key, const void *data, size_t data_len, void *context)
{
    char data_string[80] = { 0 };
    char hex_string[3] = { 0 };
    unsigned i;
    const struct shell *sh = context;

    for (i = 0; i < data_len && i < ((sizeof(data_string) / 2) - 1); i++) {
        snprintf(
            hex_string, sizeof(hex_string), "%02X", ((const uint8_t *)data)[i]);
        strcat(data_string, hex_string);
    }
    if (key->array_index == BACNET_STORAGE_ARRAY_INDEX_NONE) {
        shell_print(
            sh, "%s/%u/%u/%u=%s", Storage_Base_Name, key->object_type,
            key->object_instance, key->property_id, data_string);
    } else {
        shell_print(
            sh, "%s/%u/%u/%u/%u=%s", Storage_Base_Name, key->object_type,
            key->object_instance, key->property_id, key->array_index,
            data_string);
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
        shell_error(sh, "Failed to set settings load callback");
        return -EINVAL;
    }
    err = bacnet_storage_load();
    if (err) {
        shell_error(sh, "Failed to load settings");
        return -EINVAL;
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_bacnet_settings_cmds,
    SHELL_CMD(list, NULL, "list BACnet settings strings", cmd_list),
    SHELL_CMD(
        value,
        NULL,
        "<object type> <object instance> <property id>[array index] [value]",
        cmd_value),
    SHELL_CMD(
        delete,
        NULL,
        "<object type> <object instance> <property id>[array index]",
        cmd_delete),
    SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD(
    (bacnet),
    settings,
    &sub_bacnet_settings_cmds,
    "BACnet settings commands",
    NULL,
    1,
    0);
