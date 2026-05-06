/**
 * @file
 * @brief The BACnet shell commands for debugging and testing property values
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2025
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <bacnet/basic/object/device.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
#include <bacnet/wp.h>

/**
 * @brief Parse a BACnet object type and instance
 * @param sh Shell context
 * @param argc Number of arguments
 * @param argv Argument list
 * @param object_type Pointer to the object type
 * @param object_instance Pointer to the object instance
 * @return 0 on success, negative on failure
 * @note used by the shell to parse arguments
 */
int bacnet_shell_object_type_instance_parse(
    const struct shell *sh,
    size_t argc,
    char **argv,
    uint16_t *object_type,
    uint32_t *object_instance)
{
    uint32_t found_index = 0;

    if (argc < 2) {
        shell_help(sh);
        return -EINVAL;
    }
    if (!bactext_object_type_strtol(argv[1], &found_index)) {
        shell_error(sh, "ERROR: invalid object-type");
        return -EINVAL;
    }
    if ((found_index < 0) || (found_index >= UINT16_MAX)) {
        shell_error(
            sh,
            "ERROR: Invalid object-type: %s. "
            "Must be 0-%u.",
            argv[1], UINT16_MAX);
        return -EINVAL;
    }
    if (object_type) {
        *object_type = (uint16_t)found_index;
    }
    if (found_index == OBJECT_DEVICE) {
        if (object_instance) {
            *object_instance = Device_Object_Instance_Number();
        }
    } else if (argc > 2) {
        if (!bacnet_string_to_uint32(argv[2], &found_index)) {
            shell_error(
                sh,
                "parse: Invalid object-instance: %s. "
                "Must be 0-%u.",
                argv[2], BACNET_MAX_INSTANCE);
            return -EINVAL;
        }
        if (object_instance) {
            *object_instance = found_index;
        }
    } else {
        shell_help(sh);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Parse a BACnet object type, instance, property, and array index
 * @param sh Shell context
 * @param argc Number of arguments
 * @param argv Argument list
 * @param object_type Pointer to the object type
 * @param object_instance Pointer to the object instance
 * @param property_id Pointer to the property id
 * @param array_index Pointer to the array index
 * @return 0 on success, negative on failure
 * @note used by the shell to parse arguments
 */
int bacnet_shell_property_parse(
    const struct shell *sh,
    size_t argc,
    char **argv,
    uint16_t *object_type,
    uint32_t *object_instance,
    uint32_t *property_id,
    uint32_t *array_index)
{
    unsigned long unsigned_value = 0;
    unsigned array_value = 0;
    uint32_t found_index = 0;
    char name[80] = "";
    int scan_count = 0;
    int err;

    if (array_index) {
        *array_index = BACNET_ARRAY_ALL;
    }
    if (argc < 4) {
        shell_error(
            sh, "parse: %s <object-type> <instance> <property>[index] [value]",
            argv[0]);
        return -EINVAL;
    }
    err = bacnet_shell_object_type_instance_parse(
        sh, argc, argv, object_type, object_instance);
    if (isalpha(argv[3][0])) {
        /* choose a property by name with optional [] to denote array */
        scan_count = sscanf(argv[3], "%79[^[][%u]", name, &array_value);
        if (scan_count < 1) {
            shell_error(sh, "parse: missing property: %s.", argv[3]);
            return -EINVAL;
        }
        if (!bactext_property_strtol(name, &found_index)) {
            shell_error(sh, "parse: invalid property name: %s.", argv[3]);
            return -EINVAL;
        }
        if (property_id) {
            *property_id = found_index;
        }

    } else {
        /* choose a property by number */
        scan_count = sscanf(argv[3], "%lu[%u]", &unsigned_value, &array_value);
        if (scan_count < 1) {
            shell_error(sh, "parse: missing property: %s.", argv[3]);
            return -EINVAL;
        }
        if (unsigned_value > UINT32_MAX) {
            shell_error(
                sh, "parse: Invalid property: %s. Must be 0-%lu.", argv[3],
                UINT32_MAX);
            return -EINVAL;
        }
        if (property_id) {
            *property_id = (uint32_t)unsigned_value;
        }
    }
    if (scan_count >= 2) {
        if (array_value > UINT32_MAX) {
            shell_error(
                sh, "parse: Invalid array index: %s. Must be 0-%lu.", argv[3],
                UINT32_MAX);
            return -EINVAL;
        }
        if (array_index) {
            *array_index = (uint32_t)array_value;
        }
    }

    return 0;
}

/**
 * @brief Initialize the WriteProperty parameter structure data from the
 * ReadProperty parameter structure data and the length of the property value
 * @param wpdata [in,out] The structure to hold the WriteProperty request
 * @param rpdata [in] The structure to hold the ReadProperty request
 * @param len [in] ReadProperty application data length
 */
static void bacnet_shell_write_property_parameter_init(
    BACNET_WRITE_PROPERTY_DATA *wpdata,
    BACNET_READ_PROPERTY_DATA *rpdata,
    int len)
{
    if (wpdata && rpdata) {
        /* WriteProperty parameters */
        wpdata->object_type = rpdata->object_type;
        wpdata->object_instance = rpdata->object_instance;
        wpdata->object_property = rpdata->object_property;
        wpdata->array_index = rpdata->array_index;
        if (len > 0) {
            memcpy(
                &wpdata->application_data, rpdata->application_data,
                sizeof(wpdata->application_data));
            wpdata->application_data_len = len;
        } else {
            wpdata->application_data_len = 0;
        }
        wpdata->priority = BACNET_NO_PRIORITY;
        wpdata->error_code = ERROR_CODE_SUCCESS;
    }
}

/**
 * @brief determine if the JSON string value requires quoations or not
 * @param value string value to analyze
 * @return true if the string requires quotations
 */
static bool cmd_json_is_quoted(const char *value)
{
    bool quotes = false;
    int digit = 0, len = 0, decimal_points = 0;
    bool digits_only = false;

    /*  In JSON, the treatment of values as quoted or numeric
        depends on their data type:
        String values: Must always be enclosed in double quotes.
        Numeric values: Should not be enclosed in double quotes.
        They are represented directly as numbers
        (integers or floating-point numbers).
        Boolean values: true and false are keywords and are not quoted.
        Null value: null is a keyword and is not quoted. */
    if ((strcmp(value, "true") == 0) || (strcmp(value, "false") == 0) ||
        (strcmp(value, "null") == 0)) {
        quotes = false;
    } else {
        len = strlen(value);
        if (len && (value[0] == '"')) {
            /* already quoted */
            quotes = false;
        } else if (len) {
            digits_only = true;
            while (len) {
                len--;
                digit = value[len];
                if (!isdigit(digit)) {
                    if ((len == 0) && (value[len] == '-')) {
                        /* negative sign could be numeric */
                    } else if (value[len] == '.') {
                        /* one decimal place could be numeric */
                        decimal_points++;
                        if (decimal_points > 1) {
                            digits_only = false;
                        }
                    } else {
                        digits_only = false;
                    }
                }
            }
            if (digits_only) {
                quotes = false;
            } else {
                quotes = true;
            }
        } else {
            /* empty value string */
            quotes = true;
        }
    }

    return quotes;
}

static int cmd_json_print_key_value(
    const struct shell *sh,
    const char *prefix,
    const char *key,
    const char *value,
    const char *suffix,
    const char *append)
{
    const char *isquoted = "";

    if (cmd_json_is_quoted(value)) {
        isquoted = "\"";
    }
    shell_print(
        sh, "%s{\"%s\": %s%s%s}%s%s", prefix, key, isquoted, value, isquoted,
        suffix, append);

    return 0;
}

static int cmd_json_print_list_value(
    const struct shell *sh,
    const char *key,
    const char *value,
    bool first_value,
    bool last_value,
    const char *append)
{
    const char *isquoted = "";
    const char *suffix = ",";

    if (cmd_json_is_quoted(value)) {
        isquoted = "\"";
    }
    if (first_value) {
        shell_print(sh, "{\"%s\": [", key);
    }
    if (last_value) {
        suffix = "]";
    }
    shell_print(sh, "%s%s%s%s", isquoted, value, isquoted, suffix);
    if (last_value) {
        shell_print(sh, "}%s", append);
    }

    return 0;
}

static int cmd_json_print_hex_dump(
    const struct shell *sh,
    const char *prefix,
    const char *key,
    const char *buffer,
    int buffer_length,
    const char *suffix,
    const char *append)
{
    size_t str_size = ((size_t)buffer_length * 2U) + 1U;
    char str[str_size];
    int i, s, slen;

    s = 0;
    for (i = 0; i < buffer_length; i++) {
        unsigned byte = (unsigned)(unsigned char)buffer[i];
        slen = sprintf(&str[s], "%02x", byte);
        if (slen != 2) {
            break;
        }
        s += slen;
    }
    str[s] = '\0';
    cmd_json_print_key_value(sh, prefix, key, str, suffix, append);

    return 0;
}

static int cmd_json_print_key_error(
    const struct shell *sh,
    const char *key,
    const char *error_class,
    const char *error_code,
    const char *append)
{
    shell_print(
        sh, "{\"%s\": {\"error-class\": %s, \"error-code\": %s}}%s", key,
        error_class, error_code, append);

    return 0;
}

static int cmd_json_print_property_value_tag(
    const struct shell *sh,
    const char *property,
    unsigned long array_index,
    unsigned long priority,
    unsigned long value_tag,
    const char *value_string,
    bool success,
    const char *append)
{
    shell_print(
        sh,
        "{\"%s\": {\"array-index\": %lu, \"priority\": %lu, "
        "\"value-tag\": \"%s\", \"value\": \"%s\", \"success\": %s}}%s",
        property, array_index, priority,
        bactext_application_tag_name(value_tag), value_string,
        success ? "true" : "false", append);

    return 0;
}

static char *
bactext_name(const char *name, unsigned long value, char *buffer, size_t size)
{
    if (name) {
        return bacnet_snprintf_to_ascii(buffer, size, "%s", name);
    }
    return bacnet_ultoa(value, buffer, size);
}

/**
 * @brief Get or set a BACnet object property value
 * @param sh Shell
 * @return 0 on success, negative on failure
 */
static int cmd_value_print(
    const struct shell *sh,
    BACNET_READ_PROPERTY_DATA *rpdata,
    BACNET_APPLICATION_DATA_VALUE *value,
    int apdu_len,
    bool skip_print,
    const char *append)
{
    BACNET_OBJECT_PROPERTY_VALUE object_value = { 0 };
    uint8_t *apdu;
    int len, str_len;
    bool first_value = true;
    bool last_value = false;
    bool list_value = false;
    const char *prefix = "";
    const char *suffix = "";
    char property_name[80] = "";
    char class_name[80] = "";
    char code_name[80] = "";

    bactext_name(
        bactext_property_name_default(rpdata->object_property, NULL),
        rpdata->object_property, property_name, sizeof(property_name));
    if (apdu_len < 0) {
        bactext_name(
            bactext_error_class_name_default(rpdata->error_class, NULL),
            rpdata->error_class, class_name, sizeof(class_name));
        bactext_name(
            bactext_error_code_name_default(rpdata->error_code, NULL),
            rpdata->error_code, code_name, sizeof(code_name));
        cmd_json_print_key_error(
            sh, property_name, class_name, code_name, append);
        return 0;
    }
    if (apdu_len == 0) {
        cmd_json_print_key_value(sh, "", property_name, "null", "", append);
        /* set the tag if we know what it should be */
        value->tag = bacapp_known_property_tag(
            rpdata->object_type, rpdata->object_property);
        return 0;
    }
    apdu = rpdata->application_data;
    for (;;) {
        len = bacapp_decode_known_array_property(
            apdu, apdu_len, value, rpdata->object_type, rpdata->object_property,
            rpdata->array_index);
        if (len < 0) {
            cmd_json_print_hex_dump(
                sh, prefix, property_name, apdu, apdu_len, suffix, append);
            break;
        }
        if (first_value && (len > 0) && (len < apdu_len)) {
            list_value = true;
        }
        if (len >= apdu_len) {
            last_value = true;
        }
        /* configure object-value for printing */
        object_value.object_type = rpdata->object_type;
        object_value.object_instance = rpdata->object_instance;
        object_value.object_property = rpdata->object_property;
        object_value.array_index = rpdata->array_index;
        object_value.value = value;
        if (!skip_print) {
            str_len = bacapp_snprintf_value(NULL, 0, &object_value);
            if (str_len > 0) {
                char str[str_len + 1];
                bacapp_snprintf_value(str, str_len + 1, &object_value);
                if (list_value) {
                    cmd_json_print_list_value(
                        sh, property_name, str, first_value, last_value,
                        append);
                } else {
                    cmd_json_print_key_value(
                        sh, prefix, property_name, str, suffix, append);
                }
            } else {
                cmd_json_print_hex_dump(
                    sh, prefix, property_name, apdu, len, suffix, append);
            }
        }
        first_value = false;
        if (last_value) {
            break;
        }
        if (len > 0) {
            apdu_len -= len;
            if (apdu) {
                apdu += len;
            }
        } else {
            break;
        }
    }

    return 0;
}

/**
 * @brief Get or set a BACnet object property value
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int
cmd_print_value_all(const struct shell *sh, BACNET_READ_PROPERTY_DATA *rpdata)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    struct special_property_list_t pPropertyList = { 0 };
    unsigned count = 0, index = 0, counter = 0;
    int apdu_len, err;
    bool skip_print = false;
    char *append = ",";

    if (!rpdata) {
        return -EINVAL;
    }
    Device_Objects_Property_List(
        rpdata->object_type, rpdata->object_instance, &pPropertyList);
    count = pPropertyList.Required.count + pPropertyList.Optional.count +
        pPropertyList.Proprietary.count;
    shell_print(
        sh, "{\"%s\":\"(%s:%u)\",",
        bactext_property_name(PROP_OBJECT_IDENTIFIER),
        bactext_object_type_name(rpdata->object_type), rpdata->object_instance);
    /* display the list as well formed JSON */
    shell_print(sh, "\"%s\": [", bactext_property_name(PROP_PROPERTY_LIST));
    index = 0;
    while (pPropertyList.Required.pList[index] != -1) {
        counter++;
        rpdata->object_property = pPropertyList.Required.pList[index];
        apdu_len = Device_Read_Property(rpdata);
        if (counter == count) {
            append = "],";
        }
        bacapp_value_list_init(&value, 1);
        err = cmd_value_print(sh, rpdata, &value, apdu_len, skip_print, append);
        index++;
    }
    index = 0;
    while (pPropertyList.Optional.pList[index] != -1) {
        counter++;
        rpdata->object_property = pPropertyList.Optional.pList[index];
        apdu_len = Device_Read_Property(rpdata);
        if (counter == count) {
            append = "],";
        }
        bacapp_value_list_init(&value, 1);
        err = cmd_value_print(sh, rpdata, &value, apdu_len, skip_print, append);
        index++;
    }
    index = 0;
    while (pPropertyList.Proprietary.pList[index] != -1) {
        counter++;
        rpdata->object_property = pPropertyList.Proprietary.pList[index];
        apdu_len = Device_Read_Property(rpdata);
        if (counter == count) {
            append = "],";
        }
        bacapp_value_list_init(&value, 1);
        err = cmd_value_print(sh, rpdata, &value, apdu_len, skip_print, append);
        index++;
    }
    shell_print(sh, "\"property-list-size\": %d}", count);

    return 0;
}

/**
 * @brief Get or set a BACnet object property value
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_value(const struct shell *sh, size_t argc, char **argv)
{
    uint8_t apdu[CONFIG_BACNET_MAX_APDU_SIZE] = { 0 };
    int err = 0, apdu_len = 0;
    bool status = false;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    uint16_t object_type = 0;
    uint32_t object_instance = 0;
    uint32_t object_property = MAX_BACNET_PROPERTY_ID;
    uint32_t array_index = BACNET_ARRAY_ALL;
    uint32_t found_index = 0;
    long priority = BACNET_NO_PRIORITY;
    char *value_string = NULL;
    bool null_value = false;
    bool skip_print = false;
    bool print_all_properties = false;
    char property_name[80] = "";
    size_t application_data_len = 0;

    if ((argc == 2) || (argc == 3)) {
        err = bacnet_shell_object_type_instance_parse(
            sh, argc, argv, &object_type, &object_instance);
        if (err == 0) {
            print_all_properties = true;
        } else {
            return err;
        }
    } else if (argc > 3) {
        err = bacnet_shell_property_parse(
            sh, argc, argv, &object_type, &object_instance, &object_property,
            &array_index);
        if (err != 0) {
            return err;
        }
    } else {
        shell_help(sh);
        return -EINVAL;
    }
    if (argc > 4) {
        /* this is WriteProperty since there is a value string argument */
        value_string = argv[4];
        skip_print = true;
    }
    /* ReadProperty */
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = object_type;
    rpdata.object_instance = object_instance;
    rpdata.object_property = object_property;
    rpdata.array_index = array_index;
    if (print_all_properties) {
        cmd_print_value_all(sh, &rpdata);
        return 0;
    }
    apdu_len = Device_Read_Property(&rpdata);
    bacapp_value_list_init(&value, 1);
    err = cmd_value_print(sh, &rpdata, &value, apdu_len, skip_print, "");
    if (value_string) {
        /* WriteProperty */
        /* allow for optional priority using @ symbol for commandables */
        if (property_list_commandable_member(object_type, object_property)) {
            /* search the new_text for the @ symbol */
            char *at_ptr = strchr(value_string, '@');
            if (at_ptr) {
                /* convert the priority value after the @ symbol
                  into an integer */
                priority = strtol(at_ptr + 1, NULL, 0);
                if (priority < BACNET_MIN_PRIORITY) {
                    priority = BACNET_NO_PRIORITY;
                }
                if (priority > BACNET_MAX_PRIORITY) {
                    priority = BACNET_NO_PRIORITY;
                }
                /* null terminate the string at the @ symbol */
                *at_ptr = 0;
            }
            /* check for case insensitive NULL string */
            if (bacnet_strnicmp(value_string, "NULL", 4) == 0) {
                null_value = true;
            }
        }
        /* convert the string value into a tagged union value */
        if (null_value) {
            value.tag = BACNET_APPLICATION_TAG_NULL;
            status = true;
        } else if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
            status = bactext_object_property_strtoul(
                (BACNET_OBJECT_TYPE)object_type,
                (BACNET_PROPERTY_ID)object_property, value_string,
                &found_index);
            if (status) {
                value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
                value.type.Enumerated = found_index;
            }
        } else {
            status =
                bacapp_parse_application_data(value.tag, value_string, &value);
        }
        /* convert the property identifier into a string */
        bactext_name(
            bactext_property_name_default(object_property, NULL),
            object_property, property_name, sizeof(property_name));
        if (status) {
            apdu_len = bacapp_encode_data(apdu, &value);
            if (apdu_len) {
                bacnet_shell_write_property_parameter_init(&wpdata, &rpdata, 0);
                wpdata.priority = priority;
                application_data_len =
                    min(apdu_len, sizeof(wpdata.application_data));
                wpdata.application_data_len = application_data_len;
                memcpy(&wpdata.application_data, apdu, application_data_len);
                status = Device_Write_Property(&wpdata);
                if (status) {
                    cmd_json_print_property_value_tag(
                        sh, property_name, array_index, priority, value.tag,
                        value_string, status, "");
                } else {
                    cmd_json_print_key_error(
                        sh, property_name,
                        bactext_error_class_name(wpdata.error_class),
                        bactext_error_code_name(wpdata.error_code), "");
                }
            } else {
                cmd_json_print_key_error(
                    sh, property_name,
                    bactext_error_class_name(ERROR_CLASS_PROPERTY),
                    bactext_error_code_name(ERROR_CODE_UNEXPECTED_DATA), "");
            }
        } else {
            cmd_json_print_property_value_tag(
                sh, property_name, array_index, priority, value.tag,
                value_string, status, "");
        }
    }

    return 0;
}

/**
 * @brief List all the BACnet property for a given object
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_list(const struct shell *sh, size_t argc, char **argv)
{
    uint16_t object_type = 0;
    uint32_t object_instance = 0;
    struct special_property_list_t pPropertyList = { 0 };
    unsigned count = 0, index = 0, counter = 0;
    char property_name[80] = "";
    int err;

    if ((argc == 2) || (argc == 3)) {
        err = bacnet_shell_object_type_instance_parse(
            sh, argc, argv, &object_type, &object_instance);
        if (err != 0) {
            shell_help(sh);
            return err;
        }
    } else {
        shell_help(sh);
        return -EINVAL;
    }
    Device_Objects_Property_List(
        (BACNET_OBJECT_TYPE)object_type, object_instance, &pPropertyList);
    count = pPropertyList.Required.count + pPropertyList.Optional.count +
        pPropertyList.Proprietary.count;
    /* display the property-list as well formed JSON */
    shell_print(
        sh, "{\"%s\":\"(%s:%u)\",",
        bactext_property_name(PROP_OBJECT_IDENTIFIER),
        bactext_object_type_name(object_type), object_instance);
    shell_print(sh, "\"%s\": [", bactext_property_name(PROP_PROPERTY_LIST));
    index = 0;
    while (pPropertyList.Required.pList[index] != -1) {
        counter++;
        bactext_name(
            bactext_property_name_default(
                pPropertyList.Required.pList[index], NULL),
            pPropertyList.Required.pList[index], property_name,
            sizeof(property_name));
        shell_print(
            sh, "\"%s\"%s", property_name, (counter == count) ? "]," : ",");
        index++;
    }
    index = 0;
    while (pPropertyList.Optional.pList[index] != -1) {
        counter++;
        bactext_name(
            bactext_property_name_default(
                pPropertyList.Optional.pList[index], NULL),
            pPropertyList.Optional.pList[index], property_name,
            sizeof(property_name));
        shell_print(
            sh, "\"%s\"%s", property_name, (counter == count) ? "]," : ",");
        index++;
    }
    index = 0;
    while (pPropertyList.Proprietary.pList[index] != -1) {
        counter++;
        shell_print(
            sh, "%lu %s", (unsigned long)pPropertyList.Proprietary.pList[index],
            (counter == count) ? "]," : ",");
        index++;
    }
    shell_print(sh, "\"property-list-size\": %d}", count);

    return 0;
}

/**
 * @brief List all the BACnet property for a given object
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_size(const struct shell *sh, size_t argc, char **argv)
{
    /* display the sizes as well formed JSON */
    shell_print(
        sh, "{\"read-property-data-size\": %lu,",
        (unsigned long)sizeof(BACNET_READ_PROPERTY_DATA));
    shell_print(
        sh, "\"application-data-unit-size\": %lu,", (unsigned long)MAX_APDU);
    shell_print(
        sh, "\"write-property-data-size\": %lu,",
        (unsigned long)sizeof(BACNET_WRITE_PROPERTY_DATA));
    shell_print(
        sh, "\"application-data-value-size\": %lu,",
        (unsigned long)sizeof(BACNET_APPLICATION_DATA_VALUE));
    shell_print(
        sh, "\"object-property-value-size\": %lu}",
        (unsigned long)sizeof(BACNET_OBJECT_PROPERTY_VALUE));

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_bacnet_property_cmds,
    SHELL_CMD(list, NULL, "<object type> <object-instance>", cmd_list),
    SHELL_CMD(size, NULL, "show the size of value data types", cmd_size),
    SHELL_CMD(
        value,
        NULL,
        "<object type> <object instance> "
        "<property id>[array index] [value]",
        cmd_value),
    SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD(
    (bacnet),
    property,
    &sub_bacnet_property_cmds,
    "BACnet property commands",
    NULL,
    1,
    0);
