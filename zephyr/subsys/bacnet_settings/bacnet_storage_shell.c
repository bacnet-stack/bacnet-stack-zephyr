/**
 * @file
 * @brief The BACnet shell commands for debugging and testing storage
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
#include <bacnet_settings/bacnet_storage.h>
#include <stdio.h>

static const char Storage_Base_Name[] = CONFIG_BACNET_STORAGE_BASE_NAME;

/**
 * @brief Get or set a string using BACnet storage subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_string(const struct shell *sh, size_t argc, char **argv)
{
    char key_name[BACNET_STORAGE_KEY_SIZE_MAX + 1] = { 0 };
    uint8_t data[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
    BACNET_STORAGE_KEY key = { 0 }, test_key = { 0 };
    size_t arg_len = 0;
    int rc;

    rc = bacnet_storage_key_parse(&key, argc, argv);
    if (rc < 0) {
        return rc;
    }
    /* convert the key to a string for the shell */
    (void)bacnet_storage_key_encode(key_name, sizeof(key_name), &key);
    /* convert the key string to numbers for a test */
    if (bacnet_storage_key_decode(key_name, &test_key) == 0) {
        shell_print(
            sh, "key=%s/%lu/%lu/%lu/%lu", Storage_Base_Name,
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
    sub_bacnet_storage_cmds,
    SHELL_CMD(list, NULL, "list BACnet storage strings", cmd_list),
    SHELL_CMD(
        string,
        NULL,
        "<object type> <object instance> <property id>[array index] [string]",
        cmd_string),
    SHELL_CMD(
        delete,
        NULL,
        "<object type> <object instance> <property id>[array index]",
        cmd_delete),
    SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD(
    (bacnet),
    storage,
    &sub_bacnet_storage_cmds,
    "BACnet storage commands",
    NULL,
    1,
    0);
