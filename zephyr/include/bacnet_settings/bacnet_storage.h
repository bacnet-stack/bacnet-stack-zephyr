/**
 * @file
 * @brief API for the BACnet storage tasks for handling the device specific
 *  non-volatile object data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_STORAGE_H
#define BACNET_STORAGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/settings/settings.h>

#define BACNET_STORAGE_VALUE_SIZE_MAX   SETTINGS_MAX_VAL_LEN
#define BACNET_STORAGE_KEY_SIZE_MAX     SETTINGS_MAX_NAME_LEN
#define BACNET_STORAGE_ARRAY_INDEX_NONE UINT32_MAX

typedef struct bacnet_storage_key_t {
	uint16_t object_type;
	uint32_t object_instance;
	uint32_t property_id;
	uint32_t array_index;
} BACNET_STORAGE_KEY;

typedef int (*bacnet_storage_restore_callback)(BACNET_STORAGE_KEY *key, const void *data,
					       size_t data_size, void *context);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int bacnet_storage_init(void);

int bacnet_storage_load_callback_set(bacnet_storage_restore_callback cb, void *context);
int bacnet_storage_load(void);

int bacnet_storage_handler_set(const char *name, size_t len, settings_read_cb read_cb,
			       void *cb_arg);
int bacnet_storage_handler_commit(void);
int bacnet_storage_handler_export(int (*cb)(const char *name, const void *value, size_t val_len));

bool bacnet_storage_strtoul(const char *search_name, unsigned long *long_value);

void bacnet_storage_key_init(BACNET_STORAGE_KEY *key, uint16_t object_type,
			     uint32_t object_instance, uint32_t property_id, uint32_t array_index);
int bacnet_storage_key_decode(const char *path, BACNET_STORAGE_KEY *key);
int bacnet_storage_key_encode(char *buffer, size_t buffer_size, BACNET_STORAGE_KEY *key);

int bacnet_storage_set(BACNET_STORAGE_KEY *key, const void *data, size_t data_size);
int bacnet_storage_get(BACNET_STORAGE_KEY *key, void *data, size_t data_size);
int bacnet_storage_delete(BACNET_STORAGE_KEY *key);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
