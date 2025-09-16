/**
 * @file
 * @brief API for Get/Set of BACnet application encoded settings handlers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_SETTINGS_H
#define BACNET_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"
#include "bacnet/wp.h"

/**
 * @brief Callback data for WriteProperty restore iterator
 * @param write_function The WriteProperty function to call
 * @param context The context to pass to the WriteProperty function
 * @return true if the WriteProperty succeeded
 */
typedef bool (*bacnet_settings_restore_callback)(
    BACNET_WRITE_PROPERTY_DATA *wp_data, void *context);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bacnet_settings_basic_store(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    int application_data_len);
bool bacnet_settings_write_property_store(BACNET_WRITE_PROPERTY_DATA *wp_data);
bool bacnet_settings_write_property_restore(
    bacnet_settings_restore_callback cb, void *context);

int bacnet_settings_value_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_APPLICATION_DATA_VALUE *value);
bool bacnet_settings_value_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_APPLICATION_DATA_VALUE *value);
bool bacnet_settings_value_parse(
    const char *value_string,
    uint16_t object_type,
    uint32_t property_id,
    BACNET_APPLICATION_DATA_VALUE *value);

int bacnet_settings_real_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    float default_value,
    float *value);
bool bacnet_settings_real_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    float value);

int bacnet_settings_unsigned_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_UNSIGNED_INTEGER default_value,
    BACNET_UNSIGNED_INTEGER *value);
bool bacnet_settings_unsigned_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    BACNET_UNSIGNED_INTEGER value);

int bacnet_settings_signed_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    int32_t default_value,
    int32_t *value);
bool bacnet_settings_signed_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    int32_t value);

int bacnet_settings_characterstring_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *default_value,
    BACNET_CHARACTER_STRING *value);

bool bacnet_settings_characterstring_ansi_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *cstring);

int bacnet_settings_string_get(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *default_value,
    char *value,
    size_t value_size);

bool bacnet_settings_string_set(
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t property_id,
    uint32_t array_index,
    const char *value);

bool bacnet_settings_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
