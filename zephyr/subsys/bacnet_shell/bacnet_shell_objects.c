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
 * @brief Parse a BACnet object and instance from command line arguments
 * @param argc Number of arguments
 * @param argv Argument list
 * @param r_object_type Pointer to return object type
 * @param r_object_instance Pointer to return object instance
 * @return 0 on success, negative on failure
 * @note used by the shell to parse arguments
 */
static int bacnet_object_instance_parse(
    const struct shell *sh,
    size_t argc,
    char **argv,
    uint16_t *r_object_type,
    uint32_t *r_object_instance)
{
    uint16_t object_type = 0;
    uint32_t object_instance = 0;
    unsigned unsigned_value = 0;

    if (argc < 2) {
        shell_error(sh, "parse: %s <object-type> <instance>", argv[0]);
        return -EINVAL;
    }
    if (!bactext_object_type_strtol(argv[1], &unsigned_value)) {
        shell_error(sh, "parse: Invalid object-type: %s.", argv[1]);
        return -EINVAL;
    }
    if (unsigned_value > BACNET_MAX_OBJECT) {
        shell_error(
            sh, "parse: Invalid object-type: %s. Must be 0-%u.", argv[1],
            BACNET_MAX_OBJECT);
        return -EINVAL;
    }
    object_type = (uint16_t)unsigned_value;
    if (!bactext_strtoul(argv[2], &unsigned_value)) {
        shell_error(sh, "parse: Invalid object-instance: %s.", argv[2]);
        return -EINVAL;
    }
    if (unsigned_value > 4194303) {
        shell_error(
            sh, "parse: Invalid object-instance: %s. Must be 0-4194303.",
            argv[2]);
        return -EINVAL;
    }
    object_instance = (uint32_t)unsigned_value;
    if (r_object_type) {
        *r_object_type = object_type;
    }
    if (r_object_instance) {
        *r_object_instance = object_instance;
    }

    return 0;
}

/**
 * @brief List all BACnet objects in this device
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_object_list(const struct shell *sh, size_t argc, char **argv)
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
        found =
            Device_Object_List_Identifier(array_index, &object_type, &instance);
        if (found) {
            shell_print(
                sh, "{\"%s\":{\"%s\":%u}}%s",
                bactext_property_name(PROP_OBJECT_IDENTIFIER),
                bactext_object_type_name(object_type), instance,
                (array_index == count) ? "]," : ",");
        }
    }
    shell_print(sh, "\"object-list-size\": %d}", count);

    return 0;
}

/**
 * @brief Create a BACnet object in this device
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_object_create(const struct shell *sh, size_t argc, char **argv)
{
    BACNET_CREATE_OBJECT_DATA data = { 0 };
    uint16_t object_type = 0;
    uint32_t object_instance = 0;
    bool status = false;
    int err = 0;

    err = bacnet_object_instance_parse(
        sh, argc, argv, &object_type, &object_instance);
    if (err) {
        return err;
    }
    data.object_type = (BACNET_OBJECT_TYPE)object_type;
    data.object_instance = object_instance;
    status = Device_Create_Object(&data);
    if (!status) {
        shell_error(
            sh, "create: Unable to create object %s(%u)-%u: %s",
            bactext_object_type_name(data.object_type),
            (unsigned)data.object_type, (unsigned)data.object_instance,
            bactext_error_code_name(data.error_code));
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Delete a BACnet object in this device
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_object_delete(const struct shell *sh, size_t argc, char **argv)
{
    BACNET_DELETE_OBJECT_DATA data = { 0 };
    uint16_t object_type = 0;
    uint32_t object_instance = 0;
    bool status = false;
    int err = 0;

    err = bacnet_object_instance_parse(
        sh, argc, argv, &object_type, &object_instance);
    if (err) {
        return err;
    }
    data.object_type = (BACNET_OBJECT_TYPE)object_type;
    data.object_instance = object_instance;
    status = Device_Delete_Object(&data);
    if (!status) {
        shell_error(
            sh, "delete: Unable to delete object %s(%u)-%u: %s",
            bactext_object_type_name(data.object_type),
            (unsigned)data.object_type, (unsigned)data.object_instance,
            bactext_error_code_name(data.error_code));
        return -EINVAL;
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    object_sub_cmd,
    SHELL_CMD(list, NULL, "list all objects", cmd_object_list),
    SHELL_CMD(
        create, NULL, "<object type> <object instance>", cmd_object_create),
    SHELL_CMD(
        delete, NULL, "<object type> <object instance>", cmd_object_delete),
    SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD(
    (bacnet), object, &object_sub_cmd, "BACnet object commands", NULL, 1, 0);
