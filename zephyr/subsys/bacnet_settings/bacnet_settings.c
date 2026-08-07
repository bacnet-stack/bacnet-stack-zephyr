/**
 * @file
 * @brief Handle Get/Set of BACnet application encoded settings
 * @date May 2024
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <bacnet_settings/bacnet_storage.h>
#include <bacnet_settings/bacnet_settings.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"
#include "bacnet/bactext.h"
#include "bacnet/proplist.h"
#include "bacnet/wp.h"

/* Callback for restore */
static bacnet_settings_restore_callback Restore_Callback;

/**
 * @brief Store the BACnet data after a WriteProperty for object property
 * @param object_type - BACnet object type
 * @param object_instance - BACnet object instance
 * @param object_property - BACnet object property
 * @param array_index - BACnet array index
 * @param application_data - pointer to the data
 * @param application_data_len - length of the data
 * @note Used directly with bacnet_basic_store_callback_set() function
 */
void bacnet_settings_basic_store(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    int application_data_len)
{
    BACNET_STORAGE_KEY key = { 0 };

    bacnet_storage_key_init(
        &key, object_type, object_instance, object_property, array_index);
    /* store the data */
    (void)bacnet_storage_set(&key, application_data, application_data_len);
}

/**
 * @brief Store application data to an object property after a successful
 *  WriteProperty of the object property
 * @param wp_data - pointer to the write property data
 * @note Used directly with Device_Write_Property_Store_Callback_Set() function
 */
bool bacnet_settings_write_property_store(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    BACNET_STORAGE_KEY key = { 0 };
    BACNET_ARRAY_INDEX array_index;
    int rv;

    if (property_list_bacnet_array_member(
            wp_data->object_type, wp_data->object_property)) {
        array_index = wp_data->array_index;
    } else if (wp_data->object_property == PROP_PRESENT_VALUE) {
        /* indirect Priority_Array write */
        if (property_list_commandable_member(
                wp_data->object_type, wp_data->object_property)) {
            /* store the priority as an array index to be used on restore */
            array_index = wp_data->priority;
        } else {
            array_index = BACNET_ARRAY_ALL;
        }
    } else {
        array_index = wp_data->array_index;
    }
    /* create the key */
    bacnet_storage_key_init(
        &key, wp_data->object_type, wp_data->object_instance,
        wp_data->object_property, array_index);
    /* store the data */
    rv = bacnet_storage_set(
        &key, wp_data->application_data, wp_data->application_data_len);
    if (rv < 0) {
        return false;
    }

    return true;
}

/**
 * @brief Write data to the write_function for the specific object
 *  instance property.
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index or priority if commandable
 * @param data [in] The data to restore
 * @param data_len [in] The length of the data
 * @param write_function [in] the WriteProperty function of the object
 * @param context [in] The context to pass to the WriteProperty function
 * @return 0 on success, negative on failure.
 */
static int bacnet_settings_restore(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const void *data,
    size_t data_len,
    bacnet_settings_restore_callback restore_function,
    void *context)
{
    int err = -EINVAL;
    bool status = false;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    if (data && (data_len > 0) && (data_len <= MAX_APDU)) {
        wp_data.application_data_len = (int)data_len;
        memcpy(&wp_data.application_data[0], data, data_len);
        wp_data.object_type = object_type;
        wp_data.object_instance = object_instance;
        wp_data.object_property = property_id;
        if (property_list_commandable_member(object_type, property_id)) {
            /* commandable: the priority is stored as an array index */
            wp_data.priority = array_index;
            wp_data.array_index = BACNET_ARRAY_ALL;
        } else {
            wp_data.priority = BACNET_MAX_PRIORITY;
            wp_data.array_index = array_index;
        }
        if (restore_function) {
            status = restore_function(&wp_data, context);
            if (status) {
                err = 0;
            } else {
                /* map the BACnet Error to Zephyr Error */
                switch (wp_data.error_code) {
                    case ERROR_CODE_UNKNOWN_OBJECT:
                    case ERROR_CODE_UNKNOWN_PROPERTY:
                        err = -ENOENT;
                        break;
                    case ERROR_CODE_WRITE_ACCESS_DENIED:
                        err = -EACCES;
                        break;
                    case ERROR_CODE_DUPLICATE_NAME:
                        err = -EEXIST;
                        break;
                    case ERROR_CODE_VALUE_OUT_OF_RANGE:
                        err = -ERANGE;
                        break;
                    case ERROR_CODE_INVALID_DATA_TYPE:
                    case ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY:
                        err = -EINVAL;
                        break;
                    case ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY:
                        err = -ENOSPC;
                        break;
                    case ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED:
                    case ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED:
                        err = -ENOTSUP;
                        break;
                    default:
                        err = -EINVAL;
                        break;
                }
            }
        }
    }

    return err;
}

/**
 * @brief Callback from the Zephyr settings_restore iterator
 * @param key [in] The BACnet object type
 * @param data [in] The data to restore
 * @param data_len [in] The length of the data
 * @return 0 on success, negative on failure.
 */
static int bacnet_storage_restore_handler(
    BACNET_STORAGE_KEY *key, const void *data, size_t data_len, void *context)
{
    int err = -EINVAL;

    if (key) {
        err = bacnet_settings_restore(
            key->object_type, key->object_instance, key->property_id,
            key->array_index, data, data_len, Restore_Callback, context);
    }

    return err;
}

/**
 * @brief Utilize the settings_restore iterator
 * @param write_function [in] the WriteProperty function of the device object
 * @return true on success, false on failure.
 */
bool bacnet_settings_write_property_restore(
    bacnet_settings_restore_callback cb, void *context)
{
    int err;

    if (!cb) {
        return false;
    }
    Restore_Callback = cb;
    err = bacnet_storage_load_callback_set(
        bacnet_storage_restore_handler, context);
    if (err) {
        return false;
    }
    /* iterate over all stored settings and call the restore callback for each
     */
    err = bacnet_storage_load();
    if (err) {
        return false;
    }

    return true;
}

/**
 * @brief Get a BACnet SIGNED INTEGER value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_value_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
    BACNET_STORAGE_KEY key = { 0 };
    int stored_len, len;

    bacnet_storage_key_init(
        &key, object_type, object_instance, property_id, array_index);
    stored_len = bacnet_storage_get(&key, name, sizeof(name));
    if (stored_len > 0) {
        len = bacapp_decode_application_data(name, stored_len, value);
        if (len <= 0) {
            if (value) {
                value->tag = MAX_BACNET_APPLICATION_TAG;
            }
        }
    }

    return stored_len;
}

/**
 * @brief Store a BACnet SIGNED INTEGER value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param value [in] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_value_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX] = { 0 };
    BACNET_STORAGE_KEY key = { 0 };
    int rc, len;

    bacnet_storage_key_init(
        &key, object_type, object_instance, property_id, array_index);
    len = bacapp_encode_application_data(NULL, value);
    if (len <= 0) {
        return false;
    } else if (len > (int)sizeof(name)) {
        return false;
    }
    len = bacapp_encode_application_data(name, value);
    rc = bacnet_storage_set(&key, name, len);

    return rc == 0;
}

/**
 * @brief Parse a BACnet application data value from a string
 * @param value_string [in] The string to parse
 * @param object_type [in] The BACnet object type
 * @param property_id [in] The BACnet property id
 * @param value [out] The parsed BACnet application data value
 * @return true on success, false on failure.
 * @note This is a helper function for bacnet_settings_shell.c module
 * @note This is a simpler version of bacapp_parse_application_data()
 */
bool bacnet_settings_value_parse(
    const char *value_string,
    uint16_t object_type,
    uint32_t property_id,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    unsigned enumerated_value = 0;
    unsigned long unsigned_value = 0;
    float real_value = 0.0f;
    long signed_value = 0;
    int scan_count = 0;
    unsigned object_id_instance = 0, object_id_type = 0;
    bool status = false;

    if (!value_string) {
        return false;
    }
    if (!value) {
        return false;
    }
    /* convert the string value into a tagged union value */
    if (isalpha((unsigned char)value_string[0])) {
        if (property_list_commandable_member(object_type, property_id) &&
            (bacnet_strnicmp(value_string, "NULL", 4) == 0)) {
            /* check for case insensitive NULL string */
            value->tag = BACNET_APPLICATION_TAG_NULL;
            status = true;
        } else if (bacnet_stricmp(value_string, "true") == 0) {
            value->type.Boolean = true;
            value->tag = BACNET_APPLICATION_TAG_BOOLEAN;
            status = true;
        } else if (bacnet_stricmp(value_string, "false") == 0) {
            value->type.Boolean = false;
            value->tag = BACNET_APPLICATION_TAG_BOOLEAN;
            status = true;
        } else {
            status = bactext_object_property_strtoul(
                (BACNET_OBJECT_TYPE)object_type,
                (BACNET_PROPERTY_ID)property_id, value_string,
                &enumerated_value);
            if (status) {
                value->tag = BACNET_APPLICATION_TAG_ENUMERATED;
                value->type.Enumerated = (uint32_t)enumerated_value;
            }
        }
    }
    if (!status) {
        switch (value->tag) {
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (bacnet_storage_strtoul(value_string, &unsigned_value)) {
                    value->type.Enumerated = (uint32_t)unsigned_value;
                    status = true;
                }
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (bacnet_storage_strtoul(value_string, &unsigned_value)) {
                    value->type.Unsigned_Int = (uint32_t)unsigned_value;
                    status = true;
                }
                break;
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (bacnet_storage_strtol(value_string, &signed_value)) {
                    value->type.Signed_Int = (int32_t)signed_value;
                    status = true;
                }
                break;
            case BACNET_APPLICATION_TAG_REAL:
                if (bacnet_storage_strtof(value_string, &real_value)) {
                    value->type.Real = (float)real_value;
                    status = true;
                }
                break;
            case BACNET_APPLICATION_TAG_BIT_STRING:
                status =
                    bitstring_init_ascii(&value->type.Bit_String, value_string);
                break;
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                status = octetstring_init_ascii_hex(
                    &value->type.Octet_String, value_string);
                break;
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                status = characterstring_init_ansi(
                    &value->type.Character_String, value_string);
                break;
            case BACNET_APPLICATION_TAG_DATE:
                status =
                    datetime_date_init_ascii(&value->type.Date, value_string);
                break;
            case BACNET_APPLICATION_TAG_TIME:
                status =
                    datetime_time_init_ascii(&value->type.Time, value_string);
                break;
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                scan_count = sscanf(
                    value_string, "%4u:%7u", &object_id_type,
                    &object_id_instance);
                if (scan_count == 2) {
                    value->type.Object_Id.type = (uint16_t)object_id_type;
                    value->type.Object_Id.instance =
                        (uint32_t)object_id_instance;
                } else {
                    status = false;
                }
                break;
            default:
                break;
        }
    }

    return status;
}

/**
 * @brief Parse a key for the BACnet storage subsystem
 * @param argc Number of arguments
 * @param argv Argument list
 * @param object_type Pointer to the object type
 * @param object_instance Pointer to the object instance
 * @param property_id Pointer to the property id
 * @param array_index Pointer to the array index
 * @return 0 on success, negative on failure
 * @note used by the shell to parse arguments
 */
int bacnet_settings_object_parse(
    size_t argc,
    char **argv,
    uint16_t *object_type,
    uint32_t *object_instance,
    uint32_t *property_id,
    uint32_t *array_index)
{
    unsigned long unsigned_value = 0;
    unsigned long array_value = 0;
    uint32_t found_index = 0;
    int scan_count = 0;
    char property_name[80] = { 0 };

    if (argc < 3) {
        return -EINVAL;
    }
    if (!bactext_object_type_strtol(argv[1], &found_index)) {
        return -EINVAL;
    }
    if (found_index >= UINT16_MAX) {
        return -EINVAL;
    }
    if (object_type) {
        *object_type = (uint16_t)found_index;
    }
    if (!bacnet_storage_strtoul(argv[2], &unsigned_value)) {
        return -EINVAL;
    }
    if (unsigned_value > BACNET_MAX_INSTANCE) {
        return -EINVAL;
    }
    if (object_instance) {
        *object_instance = (uint32_t)unsigned_value;
    }
    /* property can have [] to denote priority or array */
    scan_count = sscanf(argv[3], "%lu[%lu]", &unsigned_value, &array_value);
    if (scan_count < 1) {
        scan_count = sscanf(argv[3], "%80s[%lu]", property_name, &array_value);
        if (scan_count < 1) {
            return -EINVAL;
        }
        if (!bactext_property_strtol(property_name, &found_index)) {
            return -EINVAL;
        }
        if (property_id) {
            *property_id = found_index;
        }
        if (array_index) {
            *array_index = (uint32_t)array_value;
        }
    } else if (scan_count < 1) {
        return -EINVAL;
    } else {
        if (property_id) {
            *property_id = (uint32_t)unsigned_value;
        }
    }
    if (scan_count < 2) {
        if (array_index) {
            *array_index = BACNET_STORAGE_ARRAY_INDEX_NONE;
        }
    }

    return 0;
}

/**
 * @brief Get a BACnet REAL value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_real_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    float default_value,
    float *value)
{
    int stored_len;
    BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

    stored_len = bacnet_settings_value_get(
        object_type, object_instance, property_id, array_index, &bvalue);
    if ((stored_len >= 0) && (bvalue.tag == BACNET_APPLICATION_TAG_REAL)) {
        if (value) {
            *value = bvalue.type.Real;
        }
    } else {
        if (value) {
            *value = default_value;
        }
    }

    return stored_len;
}

/**
 * @brief Store a BACnet REAL value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param value [in] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_real_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    float value)
{
    BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

    bvalue.context_specific = false;
    bvalue.tag = BACNET_APPLICATION_TAG_REAL;
    bvalue.type.Real = value;

    return bacnet_settings_value_set(
        object_type, object_instance, property_id, array_index, &bvalue);
}

/**
 * @brief Get a BACnet UNSIGNED value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_unsigned_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_UNSIGNED_INTEGER default_value,
    BACNET_UNSIGNED_INTEGER *value)
{
    uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
    BACNET_STORAGE_KEY key = { 0 };
    int stored_len, len;

    bacnet_storage_key_init(
        &key, object_type, object_instance, property_id, array_index);
    stored_len = bacnet_storage_get(&key, name, sizeof(name));
    if (stored_len > 0) {
        len = bacnet_unsigned_application_decode(name, stored_len, value);
        if (len <= 0) {
            if (value) {
                *value = default_value;
            }
        }
    } else {
        if (value) {
            *value = default_value;
        }
    }

    return stored_len;
}

/**
 * @brief Store a BACnet UNSIGNED value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param value [int] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_unsigned_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_UNSIGNED_INTEGER value)
{
    BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

    bvalue.context_specific = false;
    bvalue.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    bvalue.type.Unsigned_Int = value;

    return bacnet_settings_value_set(
        object_type, object_instance, property_id, array_index, &bvalue);
}

/**
 * @brief Get a BACnet SIGNED INTEGER value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_signed_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    int32_t default_value,
    int32_t *value)
{
    uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
    BACNET_STORAGE_KEY key = { 0 };
    int stored_len, len;

    bacnet_storage_key_init(
        &key, object_type, object_instance, property_id, array_index);
    stored_len = bacnet_storage_get(&key, name, sizeof(name));
    if (stored_len > 0) {
        len = bacnet_signed_application_decode(name, stored_len, value);
        if (len <= 0) {
            if (value) {
                *value = default_value;
            }
        }
    } else {
        if (value) {
            *value = default_value;
        }
    }

    return stored_len;
}

/**
 * @brief Store a BACnet SIGNED INTEGER value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param value [in] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_signed_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    int32_t value)
{
    BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

    bvalue.context_specific = false;
    bvalue.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
    bvalue.type.Signed_Int = value;

    return bacnet_settings_value_set(
        object_type, object_instance, property_id, array_index, &bvalue);
}

/**
 * @brief Get a BACnet CHARACTER_STRING value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [out] The character string value
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_characterstring_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *default_value,
    BACNET_CHARACTER_STRING *value)
{
    uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
    BACNET_STORAGE_KEY key = { 0 };
    int stored_len, len;

    bacnet_storage_key_init(
        &key, object_type, object_instance, property_id, array_index);
    stored_len = bacnet_storage_get(&key, name, sizeof(name));
    if (stored_len > 0) {
        len =
            bacnet_character_string_application_decode(name, stored_len, value);
        if (len <= 0) {
            characterstring_init_ansi(value, default_value);
        }
    } else {
        characterstring_init_ansi(value, default_value);
    }

    return stored_len;
}

/**
 * @brief Store a BACnet CHARACTER_STRING value to non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [out] The character string value
 * @return true on success, false on failure.
 */
bool bacnet_settings_characterstring_ansi_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *cstring)
{
    BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };
    bool status;

    bvalue.context_specific = false;
    bvalue.tag = BACNET_APPLICATION_TAG_CHARACTER_STRING;
    status = characterstring_init_ansi(&bvalue.type.Character_String, cstring);
    if (!status) {
        status = bacnet_settings_value_set(
            object_type, object_instance, property_id, array_index, &bvalue);
    }

    return status;
}

/**
 * @brief Get a C-string value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [out] The string value
 * @param value_size [in] The size of the string value
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_string_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *default_value,
    char *value,
    size_t value_size)
{
    BACNET_STORAGE_KEY key = { 0 };
    int rc;

    bacnet_storage_key_init(
        &key, object_type, object_instance, property_id, array_index);
    rc = bacnet_storage_get(&key, value, value_size);
    if (rc <= 0) {
        if (default_value) {
            strncpy(value, default_value, value_size);
            rc = (int)strlen(default_value);
        }
    }

    return rc;
}

/**
 * @brief Get a C-string value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [in] The character string value
 * @return true on success, false on failure.
 */
bool bacnet_settings_string_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *value)
{
    BACNET_STORAGE_KEY key = { 0 };
    int rc;

    if (!value) {
        return false;
    }
    bacnet_storage_key_init(
        &key, object_type, object_instance, property_id, array_index);
    rc = bacnet_storage_set(&key, (const char *)value, strlen(value) + 1);

    return rc == 0;
}

/**
 * @brief Initialize the BACnet settings storage
 * @return true=success, false on error
 */
bool bacnet_settings_init(void)
{
    int err;

    err = bacnet_storage_init();
    if (err) {
        return false;
    }

    return true;
}
