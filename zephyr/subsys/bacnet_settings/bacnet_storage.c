/**
 * @file
 * @brief The BACnet storage tasks for handling the device specific object data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zephyr/settings/settings.h>
#if defined(CONFIG_SETTINGS_FILE) && defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#elif defined(CONFIG_SETTINGS_FILE) && defined(CONFIG_FILE_SYSTEM_EXT2)
#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>
#endif
/* me! */
#include "bacnet_settings/bacnet_storage.h"

#ifdef CONFIG_BACNET_SETTINGS_BASE_NAME
#define BACNET_STORAGE_BASE_NAME CONFIG_BACNET_SETTINGS_BASE_NAME
#else
#define BACNET_STORAGE_BASE_NAME ".bacnet"
#endif

/* Logging module registration is already done in bacnet/ports/zephyr/main.c */
#include "bacnet_osif/bacnet_log.h"
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);
#define FAIL_MSG "fail (err %d)"

#define STORAGE_PARTITION    storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION)

static bacnet_storage_restore_callback BACnet_Storage_Restore_Callback;
static void *BACnet_Storage_Restore_Context;

/**
 * @brief Restore a BACnet storage item
 * @param key BACnet key (type, instance, property, array index)
 * @param data - pointer to the data to restore
 * @param data_size - size of the data to restore
 * @return 0 on success, negative on failure
 */
int bacnet_storage_restore(BACNET_STORAGE_KEY *key, const void *data, size_t data_size)
{
	int err = 0;

	if (BACnet_Storage_Restore_Callback) {
		err = BACnet_Storage_Restore_Callback(key, data, data_size,
						      BACnet_Storage_Restore_Context);
	}

	return err;
}

/**
 * @brief Attempt to convert a numeric string into a unsigned long integer
 * @param search_name - string to convert
 * @param value - where to put the converted value
 * @return true if converted and found_index is set
 * @return false if not converted and found_index is not set
 */
bool bacnet_storage_strtoul(const char *search_name, unsigned long *long_value)
{
	char *endptr;
	unsigned long value;

	value = strtoul(search_name, &endptr, 0);
	if (endptr == search_name) {
		/* No digits found */
		return false;
	}
	if (value == ULONG_MAX) {
		/* If the correct value is outside the range of representable values,
		   {ULONG_MAX} shall be returned */
		return false;
	}
	if (*endptr != '\0') {
		/* Extra text found */
		return false;
	}
	if (long_value) {
		*long_value = (unsigned)value;
	}

	return true;
}

/* dynamic main tree handler */
struct settings_handler bacnet_storage_handler = {
	.name = "bacnet",
	/* This gets called when asking for a settings element value
	   by its name using settings_runtime_get() from the runtime backend.*/
	.h_get = NULL,
	/* This gets called when the value is loaded from persisted storage
	   with settings_load(), or when using settings_runtime_set() from
	   the runtime backend.*/
	.h_set = bacnet_storage_handler_set,
	/* This gets called after the settings have been loaded in full.
	   Sometimes you don’t want an individual setting value to take
	   effect right away, for example if there are multiple settings
	   which are interdependent.*/
	.h_commit = bacnet_storage_handler_commit,
	/* This gets called to write all current settings.
	   This happens when settings_save() tries to save the settings
	   or transfer to any user-implemented back-end.*/
	.h_export = bacnet_storage_handler_export};

/**
 * @brief Initialize the non-volatile data
 */
int bacnet_storage_init(void)
{
	int rc = 0;

#if defined(CONFIG_SETTINGS_FILE) && defined(CONFIG_FILE_SYSTEM_LITTLEFS)
	FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

	/* mounting info */
	static struct fs_mount_t littlefs_mnt = {.type = FS_LITTLEFS,
						 .fs_data = &cstorage,
						 .storage_dev = (void *)STORAGE_PARTITION_ID,
						 .mnt_point = "/ff"};

	rc = fs_mount(&littlefs_mnt);
	if (rc != 0) {
		LOG_INF("mounting littlefs error: [%d]", rc);
	} else {
		rc = fs_unlink(CONFIG_SETTINGS_FILE_PATH);
		if ((rc != 0) && (rc != -ENOENT)) {
			H("can't delete config file%d", rc);
		} else {
			LOG_INF("FS initialized: OK");
		}
	}
#endif
	rc = settings_subsys_init();
	if (rc) {
		LOG_ERR("settings subsys initialization: fail (err %d)", rc);
		return rc;
	}
	rc = settings_register(&bacnet_storage_handler);
	if (rc) {
		LOG_ERR("settings_register failed (err %d)", rc);
		return rc;
	}

	LOG_INF("settings subsys initialization: OK.");

	return rc;
}

/**
 * @brief Set the callback function for restoring BACnet storage items
 * @param restore_cb - pointer to the restore callback function
 * @return 0=success, negative on error
 */
int bacnet_storage_load_callback_set(bacnet_storage_restore_callback restore_cb, void *context)
{
	int rc = 0;

	BACnet_Storage_Restore_Callback = restore_cb;
	BACnet_Storage_Restore_Context = context;

	return rc;
}

/**
 * @brief Load all BACnet settings from storage
 * @return 0=success, negative on error
 */
int bacnet_storage_load(void)
{
	int rc = 0;

	rc = settings_load();
	if (rc) {
		LOG_ERR("settings_load failed (err %d)", rc);
		return rc;
	}
	LOG_INF("settings_load: OK.");

	return rc;
}

/**
 * @brief Initialize a BACnet key object with optional array
 * @param key BACnet key (type, instance, property, array index)
 * @param object_type BACnet object type
 * @param object_instance BACnet object instance
 * @param property_id BACnet property id
 * @param array_index BACnet array index
 */
void bacnet_storage_key_init(BACNET_STORAGE_KEY *key, uint16_t object_type,
			     uint32_t object_instance, uint32_t property_id, uint32_t array_index)
{
	if (key) {
		key->object_type = object_type;
		key->object_instance = object_instance;
		key->property_id = property_id;
		key->array_index = array_index;
	}
}

/**
 * @brief Create a storage key string for a BACnet object property
 * @param buffer buffer to store key string
 * @param buffer_size size of key buffer
 * @param key BACnet key (type, instance, property, array index)
 * @return length of the string
 */
int bacnet_storage_key_encode(char *buffer, size_t buffer_size, BACNET_STORAGE_KEY *key)
{
	int rc = 0;
	const char base_name[] = CONFIG_BACNET_STORAGE_BASE_NAME;

	if (buffer) {
		memset(buffer, 0, buffer_size);
	}
	if (key->array_index == BACNET_STORAGE_ARRAY_INDEX_NONE) {
		rc = snprintf(buffer, buffer_size, "%s%c%u%c%lu%c%lu", base_name,
			      SETTINGS_NAME_SEPARATOR, (unsigned short)key->object_type,
			      SETTINGS_NAME_SEPARATOR, (unsigned long)key->object_instance,
			      SETTINGS_NAME_SEPARATOR, (unsigned long)key->property_id);
	} else {
		rc = snprintf(buffer, buffer_size, "%s%c%u%c%lu%c%lu%c%lu", base_name,
			      SETTINGS_NAME_SEPARATOR, (unsigned short)key->object_type,
			      SETTINGS_NAME_SEPARATOR, (unsigned long)key->object_instance,
			      SETTINGS_NAME_SEPARATOR, (unsigned long)key->property_id,
			      SETTINGS_NAME_SEPARATOR, (unsigned long)key->array_index);
	}

	return rc;
}

/**
 * @brief Decode a storage key string into a BACnet object property
 * @param name settings name key string
 * @param key BACnet key (type, instance, property, array index)
 * @return 0=success, negative on error
 */
int bacnet_storage_key_decode(const char *path, BACNET_STORAGE_KEY *key)
{
	const char *next;
	size_t next_len;
	char object_type_name[SETTINGS_MAX_DIR_DEPTH + 1] = {0};
	char object_instance_name[SETTINGS_MAX_DIR_DEPTH + 1] = {0};
	char property_id_name[SETTINGS_MAX_DIR_DEPTH + 1] = {0};
	char array_index_name[SETTINGS_MAX_DIR_DEPTH + 1] = {0};
	unsigned long long_value = 0;
	const char base_name[] = CONFIG_BACNET_STORAGE_BASE_NAME;

	/* settings root name */
	if (settings_name_steq(path, base_name, &next) && next) {
		/* OPTIONAL - called from shell */
		path = next;
	}
	/* object-type */
	next_len = settings_name_next(path, &next);
	if (next) {
		if (next_len + 1 > sizeof(object_type_name)) {
			LOG_ERR("object-type name too long: %d", next_len);
			return -EINVAL;
		}
		memcpy(object_type_name, path, next_len);
		if (bacnet_storage_strtoul(object_type_name, &long_value)) {
			key->object_type = long_value;
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	/* object-instance */
	path = next;
	next_len = settings_name_next(path, &next);
	if (next) {
		if (next_len + 1 > sizeof(object_instance_name)) {
			LOG_ERR("object-instance name too long: %d", next_len);
			return -EINVAL;
		}
		memcpy(object_instance_name, path, next_len);
		if (bacnet_storage_strtoul(object_instance_name, &long_value)) {
			key->object_instance = long_value;
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	/* property-id */
	path = next;
	next_len = settings_name_next(path, &next);
	if (next) {
		if (next_len + 1 > sizeof(property_id_name)) {
			LOG_ERR("property-id name too long: %d", next_len);
			return -EINVAL;
		}
		memcpy(property_id_name, path, next_len);
		if (bacnet_storage_strtoul(property_id_name, &long_value)) {
			key->property_id = long_value;
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	/* array-index - OPTIONAL */
	path = next;
	next_len = settings_name_next(path, &next);
	if (next) {
		if (next_len + 1 > sizeof(array_index_name)) {
			LOG_ERR("array-index name too long: %d", next_len);
			return -EINVAL;
		}
		memcpy(array_index_name, path, next_len);
		if (bacnet_storage_strtoul(array_index_name, &long_value)) {
			key->array_index = long_value;
		} else {
			return -EINVAL;
		}
	} else {
		key->array_index = BACNET_STORAGE_ARRAY_INDEX_NONE;
	}

	return 0;
}

/**
 * @brief settings callback: Set a value in BACnet storage
 * @param path - settings name key string
 * @param data_len - length of the data to set
 * @param read_cb - callback to read the value
 * @param cb_arg - callback argument
 * @return 0=success, negative on error
 */
int bacnet_storage_handler_set(const char *path, size_t data_len, settings_read_cb read_cb,
			       void *cb_arg)
{
	int rc = -EINVAL;
	BACNET_STORAGE_KEY key = {0};
	uint8_t data[BACNET_STORAGE_VALUE_SIZE_MAX] = {0};

	if (bacnet_storage_key_decode(path, &key) == 0) {
		/* get the data if there is any */
		if (data_len == 0) {
			rc = 0;
		} else {
			rc = read_cb(cb_arg, &data, sizeof(data));
			if (rc < 0) {
				/* On error returns -ERRNO code. */
				if (rc == -ENOENT) {
					rc = 0;
				} else {
					LOG_ERR("Data restore error: %d", rc);
				}
			} else {
				rc = bacnet_storage_restore(&key, data, data_len);
				if (rc == 0) {
					LOG_INF("Data restored:%s %d bytes", path, data_len);
				}
			}
		}
	}

	return rc;
}

/**
 * @brief settings callback: Commit all changes to BACnet storage
 * @return 0=success, negative on error
 */
int bacnet_storage_handler_commit(void)
{
	LOG_INF("Restored all settings");
	return 0;
}

/**
 * @brief settings callback: This gets called to write all current settings.
 *  This happens when settings_save() tries to save the settings
 *  or transfer to any user-implemented back-end.
 * @param cb - callback function to receive the settings
 * @return 0=success, negative on error
 */
int bacnet_storage_handler_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	LOG_INF("FIXME: Export requested");
	return 0;
}

/**
 * @brief Set a value with a specific key to non-volatile storage
 * @param key BACnet key (type, instance, property, array index)
 * @param data [in] one or more bytes of data
 * @param data_len [in] Value length in bytes.
 * @return 0 on success, non-zero on failure.
 */
int bacnet_storage_set(BACNET_STORAGE_KEY *key, const void *data, size_t data_len)
{
	char name[SETTINGS_MAX_NAME_LEN + 1] = {0};
	int rc;

	rc = bacnet_storage_key_encode(name, sizeof(name), key);
	LOG_INF("Set a key-value pair. Key=%s", name);
	rc = settings_save_one(name, data, data_len);
	if (rc) {
		LOG_INF(FAIL_MSG, rc);
	} else {
		LOG_HEXDUMP_INF(data, data_len, "value");
	}

	return rc;
}

/**
 * @brief Structure to hold immediate values
 */
struct direct_immediate_value {
	size_t value_size;
	size_t value_len;
	void *value;
	bool fetched;
};

/**
 * @brief Direct loader for immediate values
 * @param name [in] Key in string format.
 * @param len [in] Length of the key
 * @param read_cb [in] Callback to read the value
 * @param cb_arg [in] Callback argument
 * @param param [in] Callback parameter
 * @return 0 on success, non-zero on failure.
 */
static int direct_loader_immediate_value(const char *name, size_t len, settings_read_cb read_cb,
					 void *cb_arg, void *param)
{
	const char *next;
	size_t name_len;
	int rc;
	struct direct_immediate_value *context = (struct direct_immediate_value *)param;

	/* only the exact match and ignore descendants of the searched name */
	name_len = settings_name_next(name, &next);
	if (name_len == 0) {
		rc = read_cb(cb_arg, context->value, len);
		if ((rc >= 0) && (rc <= context->value_size)) {
			context->fetched = true;
			context->value_len = rc;
			LOG_INF("immediate load: OK.");
			return 0;
		}
		return -EINVAL;
	}

	/* other keys aren't served by the callback
	 * Return success in order to skip them
	 * and keep storage processing.
	 */
	return 0;
}

/**
 * @brief Load an immediate value from non-volatile storage
 * @param name [in] Key in string format.
 * @param value [out] Buffer to store the value
 * @param value_size [in] size of the buffer
 * @return value length in bytes on success 0..N, negative on failure.
 */
static int load_immediate_value(const char *name, void *value, size_t value_size)
{
	int rc;
	struct direct_immediate_value context;

	context.fetched = false;
	context.value_size = value_size;
	context.value_len = 0;
	context.value = value;

	rc = settings_load_subtree_direct(name, direct_loader_immediate_value, (void *)&context);
	if (rc == 0) {
		if (!context.fetched) {
			rc = -ENOENT;
		}
	}

	return context.value_len;
}

/**
 * @brief Get a value with a specific key to non-volatile storage
 * @param key BACnet key (type, instance, property, array index)
 * @param data [out] Binary value.
 * @param data_size [in] requested value length in bytes
 * @return data length on success 0..N, negative on failure.
 */
int bacnet_storage_get(BACNET_STORAGE_KEY *key, void *data, size_t data_size)
{
	char name[SETTINGS_MAX_NAME_LEN + 1] = {0};
	int rc;

	rc = bacnet_storage_key_encode(name, sizeof(name), key);
	LOG_INF("Get a key-value pair. Key=<%s>", name);
	rc = load_immediate_value(name, data, data_size);
	if (rc == 0) {
		LOG_INF("empty entry");
	} else if (rc > 0) {
		LOG_HEXDUMP_INF(data, rc, "value");
	} else if (rc == -ENOENT) {
		LOG_INF("no entry");
	} else {
		LOG_INF("unexpected" FAIL_MSG, rc);
	}

	return rc;
}

/**
 * @brief Delete a value with a specific key from non-volatile storage
 * @param key BACnet key (type, instance, property, array index)
 * @return 0 on success, non-zero on failure.
 */
int bacnet_storage_delete(BACNET_STORAGE_KEY *key)
{
	char name[SETTINGS_MAX_NAME_LEN + 1] = {0};
	int rc;

	rc = bacnet_storage_key_encode(name, sizeof(name), key);
	LOG_INF("Delete a key-value pair. Key=%s", name);
	rc = settings_delete(name);
	if (rc) {
		LOG_INF(FAIL_MSG, rc);
	}

	return rc;
}
