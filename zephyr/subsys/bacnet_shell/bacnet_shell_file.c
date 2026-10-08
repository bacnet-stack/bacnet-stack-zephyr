/**
 * @file
 * @brief BACnet File object shell commands
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/data/json.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/create_object.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/device.h"

#define FILE_CHUNK_SIZE CONFIG_BACNETSTACK_BACNET_FILE_SHELL_CHUNK_SIZE
#define FILE_HEX_BUFFER_SIZE (2U * FILE_CHUNK_SIZE + 1U)
#define FILE_IMPORT_COMMAND_OVERHEAD 74U

BUILD_ASSERT(
    (2U * FILE_CHUNK_SIZE + FILE_IMPORT_COMMAND_OVERHEAD) <
        CONFIG_SHELL_CMD_BUFF_SIZE,
    "BACnet File shell chunk is too large for the shell command buffer");

struct bacnet_file_import_chunk {
    struct json_obj_token instance;
    struct json_obj_token offset;
    char *data;
};

static const struct json_obj_descr file_import_chunk_descr[] = {
    JSON_OBJ_DESCR_PRIM(
        struct bacnet_file_import_chunk, instance, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(
        struct bacnet_file_import_chunk, offset, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(struct bacnet_file_import_chunk, data, JSON_TOK_STRING),
};

static int file_instance_parse(
    const struct shell *sh, const char *value, uint32_t *instance)
{
    if (!bacnet_string_to_uint32(value, instance)) {
        shell_error(sh, "Invalid file instance: %s", value);
        return -EINVAL;
    }
    if (!bacfile_valid_instance(*instance)) {
        shell_error(sh, "File instance %u does not exist", (unsigned)*instance);
        return -EINVAL;
    }

    return 0;
}

static int
file_offset_parse(const struct shell *sh, const char *value, uint32_t *offset)
{
    if (!bacnet_string_to_uint32(value, offset) || *offset > INT32_MAX) {
        shell_error(sh, "Invalid file offset: %s", value);
        return -EINVAL;
    }

    return 0;
}

static bool
file_import_uint32_parse(const struct json_obj_token *token, uint32_t *value)
{
    size_t index;
    uint32_t parsed = 0;

    if (!token->start || (token->length == 0U) || (token->length > 10U)) {
        return false;
    }
    for (index = 0; index < token->length; index++) {
        unsigned digit;

        if ((token->start[index] < '0') || (token->start[index] > '9')) {
            return false;
        }
        digit = (unsigned)(token->start[index] - '0');
        if (parsed > (UINT32_MAX - digit) / 10U) {
            return false;
        }
        parsed = (parsed * 10U) + digit;
    }
    *value = parsed;

    return true;
}

static int file_path_require(
    const struct shell *sh, uint32_t instance, const char **pathname)
{
    *pathname = bacfile_pathname(instance);
    if (!*pathname || !(*pathname)[0]) {
        shell_error(sh, "File %u has no pathname", (unsigned)instance);
        return -EINVAL;
    }

    return 0;
}

static int file_writable_path_require(
    const struct shell *sh, uint32_t instance, const char **pathname)
{
    int err = file_path_require(sh, instance, pathname);

    if (err) {
        return err;
    }
    if (bacfile_read_only(instance)) {
        shell_error(sh, "File %u is read-only", (unsigned)instance);
        return -EPERM;
    }

    return 0;
}

static int file_stream_require(const struct shell *sh, uint32_t instance)
{
    if (!bacfile_file_access_stream(instance)) {
        shell_error(
            sh, "File %u does not use stream access", (unsigned)instance);
        return -ENOTSUP;
    }

    return 0;
}

static int file_write_hex(
    const struct shell *sh, uint32_t instance, uint32_t offset, const char *hex)
{
    uint8_t data[FILE_CHUNK_SIZE];
    size_t hex_len = strlen(hex);
    size_t data_len;
    size_t bytes_written;
    const char *pathname;
    int err;

    if (hex_len == 0U) {
        shell_error(sh, "Hex data must not be empty");
        return -EINVAL;
    }
    if (hex_len > 2U * FILE_CHUNK_SIZE) {
        shell_error(
            sh, "Hex data exceeds the %u-byte chunk limit",
            (unsigned)FILE_CHUNK_SIZE);
        return -E2BIG;
    }
    if ((hex_len & 1U) != 0U) {
        shell_error(sh, "Hex data must contain an even number of characters");
        return -EINVAL;
    }
    data_len = hex_len / 2U;
    if (hex2bin(hex, hex_len, data, sizeof(data)) != data_len) {
        shell_error(sh, "Hex data contains a non-hexadecimal character");
        return -EINVAL;
    }

    err = file_writable_path_require(sh, instance, &pathname);
    if (err) {
        return err;
    }
    err = file_stream_require(sh, instance);
    if (err) {
        return err;
    }

    bytes_written = bacfile_write_offset(
        instance, (int32_t)offset, data, (uint32_t)data_len);
    if (bytes_written != data_len) {
        shell_error(
            sh, "Wrote %u of %u bytes to file %u at offset %u",
            (unsigned)bytes_written, (unsigned)data_len, (unsigned)instance,
            (unsigned)offset);
        return -EIO;
    }

    shell_print(
        sh, "Wrote %u bytes to file %u at offset %u", (unsigned)data_len,
        (unsigned)instance, (unsigned)offset);
    return 0;
}

static int cmd_file_list(const struct shell *sh, size_t argc, char **argv)
{
    unsigned count = bacfile_count();
    unsigned index;

    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    for (index = 0; index < count; index++) {
        uint32_t instance = bacfile_index_to_instance(index);
        const char *name = bacfile_name_ansi(instance);
        const char *mime_type = bacfile_file_type(instance);

        shell_print(
            sh, "%u %s %s %u %s", (unsigned)instance, name ? name : "",
            mime_type ? mime_type : "", (unsigned)bacfile_file_size(instance),
            bacfile_read_only(instance) ? "read-only" : "read-write");
    }

    return 0;
}

static int cmd_file_info(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t instance;
    const char *pathname;
    const char *name;
    const char *mime_type;
    int err;

    if (argc != 2U) {
        shell_error(sh, "Usage: bacnet file info <instance>");
        return -EINVAL;
    }
    err = file_instance_parse(sh, argv[1], &instance);
    if (err) {
        return err;
    }

    pathname = bacfile_pathname(instance);
    name = bacfile_name_ansi(instance);
    mime_type = bacfile_file_type(instance);
    shell_print(sh, "instance: %u", (unsigned)instance);
    shell_print(sh, "name: %s", name ? name : "");
    shell_print(sh, "mime-type: %s", mime_type ? mime_type : "");
    shell_print(sh, "size: %u", (unsigned)bacfile_file_size(instance));
    shell_print(
        sh, "read-only: %s", bacfile_read_only(instance) ? "true" : "false");
    shell_print(sh, "pathname: %s", pathname ? pathname : "");
    shell_print(
        sh, "stream-access: %s",
        bacfile_file_access_stream(instance) ? "true" : "false");

    return 0;
}

static int cmd_file_read(const struct shell *sh, size_t argc, char **argv)
{
    uint8_t data[FILE_CHUNK_SIZE];
    char hex[FILE_HEX_BUFFER_SIZE];
    uint32_t instance;
    uint32_t offset;
    uint32_t requested = FILE_CHUNK_SIZE;
    uint32_t bytes_read;
    const char *pathname;
    int err;

    if ((argc < 3U) || (argc > 4U)) {
        shell_error(sh, "Usage: bacnet file read <instance> <offset> [len]");
        return -EINVAL;
    }
    err = file_instance_parse(sh, argv[1], &instance);
    if (err) {
        return err;
    }
    err = file_offset_parse(sh, argv[2], &offset);
    if (err) {
        return err;
    }
    if (argc == 4U) {
        if (!bacnet_string_to_uint32(argv[3], &requested) || requested == 0U ||
            requested > FILE_CHUNK_SIZE) {
            shell_error(
                sh, "Read length must be between 1 and %u",
                (unsigned)FILE_CHUNK_SIZE);
            return -EINVAL;
        }
    }

    err = file_path_require(sh, instance, &pathname);
    if (err) {
        return err;
    }
    err = file_stream_require(sh, instance);
    if (err) {
        return err;
    }

    bytes_read =
        bacfile_read_offset(instance, (int32_t)offset, data, requested);
    hex[0] = '\0';
    if ((bytes_read > 0U) &&
        (bin2hex(data, bytes_read, hex, sizeof(hex)) != 2U * bytes_read)) {
        shell_error(sh, "Unable to encode read data as hex");
        return -EIO;
    }
    shell_print(
        sh, "%u %u %s%s", (unsigned)offset, (unsigned)bytes_read, hex,
        bytes_read < requested ? " EOF" : "");

    return 0;
}

static int cmd_file_write(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t instance;
    uint32_t offset;
    int err;

    if (argc != 4U) {
        shell_error(sh, "Usage: bacnet file write <instance> <offset> <hex>");
        return -EINVAL;
    }
    err = file_instance_parse(sh, argv[1], &instance);
    if (err) {
        return err;
    }
    err = file_offset_parse(sh, argv[2], &offset);
    if (err) {
        return err;
    }

    return file_write_hex(sh, instance, offset, argv[3]);
}

static void file_dump_line(
    const struct shell *sh, uint32_t offset, const uint8_t *data, size_t len)
{
    size_t index;

    shell_fprintf(sh, SHELL_NORMAL, "%08x: ", (unsigned)offset);
    for (index = 0; index < 16U; index += 2U) {
        if (index < len) {
            shell_fprintf(sh, SHELL_NORMAL, "%02x", (unsigned)data[index]);
        } else {
            shell_fprintf(sh, SHELL_NORMAL, "  ");
        }
        if ((index + 1U) < len) {
            shell_fprintf(sh, SHELL_NORMAL, "%02x", (unsigned)data[index + 1U]);
        } else {
            shell_fprintf(sh, SHELL_NORMAL, "  ");
        }
        shell_fprintf(sh, SHELL_NORMAL, " ");
        if (index == 6U) {
            shell_fprintf(sh, SHELL_NORMAL, " ");
        }
    }
    shell_fprintf(sh, SHELL_NORMAL, " |");
    for (index = 0; index < 16U; index++) {
        char ascii = '.';

        if ((index < len) && (data[index] >= 0x20U) && (data[index] <= 0x7eU)) {
            ascii = (char)data[index];
        }
        shell_fprintf(sh, SHELL_NORMAL, "%c", ascii);
    }
    shell_print(sh, "|");
}

static int cmd_file_dump(const struct shell *sh, size_t argc, char **argv)
{
    uint8_t data[16];
    uint32_t instance;
    uint32_t offset = 0;
    uint32_t remaining;
    uint32_t file_size;
    uint32_t requested;
    uint32_t bytes_read;
    bool length_set = false;
    const char *pathname;
    int err;

    if ((argc < 2U) || (argc > 4U)) {
        shell_error(sh, "Usage: bacnet file dump <instance> [offset] [len]");
        return -EINVAL;
    }
    err = file_instance_parse(sh, argv[1], &instance);
    if (err) {
        return err;
    }
    if (argc >= 3U) {
        err = file_offset_parse(sh, argv[2], &offset);
        if (err) {
            return err;
        }
    }
    if (argc == 4U) {
        if (!bacnet_string_to_uint32(argv[3], &remaining)) {
            shell_error(sh, "Invalid dump length: %s", argv[3]);
            return -EINVAL;
        }
        length_set = true;
    }

    err = file_path_require(sh, instance, &pathname);
    if (err) {
        return err;
    }
    err = file_stream_require(sh, instance);
    if (err) {
        return err;
    }

    file_size = bacfile_file_size(instance);
    if (file_size > INT32_MAX) {
        shell_error(sh, "File is too large for stream offsets");
        return -EOVERFLOW;
    }
    if (offset > file_size) {
        shell_error(
            sh, "Offset %u is past end of file (%u)", offset, file_size);
        return -EINVAL;
    }
    if (!length_set) {
        remaining = file_size - offset;
    } else if (remaining > (uint32_t)INT32_MAX - offset) {
        shell_error(sh, "Dump range exceeds the stream offset limit");
        return -EOVERFLOW;
    }
    while (remaining > 0U) {
        requested = MIN(remaining, sizeof(data));
        bytes_read =
            bacfile_read_offset(instance, (int32_t)offset, data, requested);
        if (bytes_read == 0U) {
            break;
        }
        file_dump_line(sh, offset, data, bytes_read);
        offset += bytes_read;
        remaining -= bytes_read;
        if (bytes_read < requested) {
            break;
        }
    }

    return 0;
}

static void file_json_string_data(
    const struct shell *sh, const uint8_t *value, size_t value_len)
{
    size_t index;

    shell_fprintf(sh, SHELL_NORMAL, "\"");
    for (index = 0; index < value_len; index++) {
        uint8_t character = value[index];

        switch (character) {
            case '"':
            case '\\':
                shell_fprintf(sh, SHELL_NORMAL, "\\%c", (int)character);
                break;
            case '\b':
                shell_fprintf(sh, SHELL_NORMAL, "\\b");
                break;
            case '\f':
                shell_fprintf(sh, SHELL_NORMAL, "\\f");
                break;
            case '\n':
                shell_fprintf(sh, SHELL_NORMAL, "\\n");
                break;
            case '\r':
                shell_fprintf(sh, SHELL_NORMAL, "\\r");
                break;
            case '\t':
                shell_fprintf(sh, SHELL_NORMAL, "\\t");
                break;
            default:
                if (character < 0x20U) {
                    shell_fprintf(
                        sh, SHELL_NORMAL, "\\u%04x", (unsigned)character);
                } else {
                    shell_fprintf(sh, SHELL_NORMAL, "%c", (int)character);
                }
                break;
        }
    }
    shell_fprintf(sh, SHELL_NORMAL, "\"");
}

static void file_json_string(const struct shell *sh, const char *value)
{
    file_json_string_data(sh, (const uint8_t *)value, strlen(value));
}

static void
file_json_hex(const struct shell *sh, const uint8_t *value, size_t value_len)
{
    static const char Hex_Digits[] = "0123456789abcdef";
    size_t index;

    shell_fprintf(sh, SHELL_NORMAL, "\"");
    for (index = 0; index < value_len; index++) {
        shell_fprintf(
            sh, SHELL_NORMAL, "%c%c", Hex_Digits[value[index] >> 4],
            Hex_Digits[value[index] & 0x0fU]);
    }
    shell_fprintf(sh, SHELL_NORMAL, "\"");
}

static void file_json_unsigned_or_null(
    const struct shell *sh, uint32_t value, uint32_t unspecified)
{
    if (value == unspecified) {
        shell_fprintf(sh, SHELL_NORMAL, "null");
    } else {
        shell_fprintf(sh, SHELL_NORMAL, "%u", (unsigned)value);
    }
}

static void file_json_real(
    const struct shell *sh, const char *type, double value, bool is_single)
{
    char number[48];
    int length;

    if (!isfinite(value)) {
        shell_fprintf(
            sh, SHELL_NORMAL,
            "{\"type\":\"%s\",\"value\":null,\"special\":", type);
        file_json_string(
            sh,
            isnan(value) ? "nan" : (value < 0.0 ? "-infinity" : "infinity"));
        shell_fprintf(sh, SHELL_NORMAL, "}");
        return;
    }
    length = bacapp_snprintf(
        number, sizeof(number), is_single ? "%.9g" : "%.17g", value);
    if ((length < 0) || ((size_t)length >= sizeof(number))) {
        shell_fprintf(
            sh, SHELL_NORMAL, "{\"type\":\"%s\",\"value\":null}", type);
        return;
    }
    shell_fprintf(
        sh, SHELL_NORMAL, "{\"type\":\"%s\",\"value\":%s}", type, number);
}

static void file_json_application_value(
    const struct shell *sh, const BACNET_APPLICATION_DATA_VALUE *value)
{
    switch (value->tag) {
#if defined(BACAPP_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"null\",\"value\":null}");
            break;
#endif
#if defined(BACAPP_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"boolean\",\"value\":%s}",
                value->type.Boolean ? "true" : "false");
            break;
#endif
#if defined(BACAPP_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"unsigned\",\"value\":%llu}",
                (unsigned long long)value->type.Unsigned_Int);
            break;
#endif
#if defined(BACAPP_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"signed\",\"value\":%ld}",
                (long)value->type.Signed_Int);
            break;
#endif
#if defined(BACAPP_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            file_json_real(sh, "real", value->type.Real, true);
            break;
#endif
#if defined(BACAPP_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            file_json_real(sh, "double", value->type.Double, false);
            break;
#endif
#if defined(BACAPP_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"octet-string\",\"value_hex\":");
            file_json_hex(
                sh, octetstring_value_const(&value->type.Octet_String),
                octetstring_length(&value->type.Octet_String));
            shell_fprintf(sh, SHELL_NORMAL, "}");
            break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            shell_fprintf(
                sh, SHELL_NORMAL,
                "{\"type\":\"character-string\",\"encoding\":%u,",
                (unsigned)characterstring_encoding(
                    &value->type.Character_String));
            if (characterstring_utf8_valid(&value->type.Character_String)) {
                shell_fprintf(sh, SHELL_NORMAL, "\"value\":");
                file_json_string_data(
                    sh,
                    (const uint8_t *)characterstring_value_const(
                        &value->type.Character_String),
                    characterstring_length(&value->type.Character_String));
            } else {
                shell_fprintf(sh, SHELL_NORMAL, "\"value_hex\":");
                file_json_hex(
                    sh,
                    (const uint8_t *)characterstring_value_const(
                        &value->type.Character_String),
                    characterstring_length(&value->type.Character_String));
            }
            shell_fprintf(sh, SHELL_NORMAL, "}");
            break;
#endif
#if defined(BACAPP_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING: {
            uint8_t bit_index;
            uint8_t bits_used = bitstring_bits_used(&value->type.Bit_String);

            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"bit-string\",\"bits\":\"");
            for (bit_index = 0; bit_index < bits_used; bit_index++) {
                shell_fprintf(
                    sh, SHELL_NORMAL, "%c",
                    bitstring_bit(&value->type.Bit_String, bit_index) ? '1'
                                                                      : '0');
            }
            shell_fprintf(sh, SHELL_NORMAL, "\"}");
        } break;
#endif
#if defined(BACAPP_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"enumerated\",\"value\":%u}",
                (unsigned)value->type.Enumerated);
            break;
#endif
#if defined(BACAPP_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            shell_fprintf(sh, SHELL_NORMAL, "{\"type\":\"date\",\"year\":");
            file_json_unsigned_or_null(
                sh, value->type.Date.year, BACNET_DATE_YEAR_EPOCH + UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, ",\"month\":");
            file_json_unsigned_or_null(sh, value->type.Date.month, UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, ",\"day\":");
            file_json_unsigned_or_null(sh, value->type.Date.day, UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, ",\"weekday\":");
            file_json_unsigned_or_null(sh, value->type.Date.wday, UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, "}");
            break;
#endif
#if defined(BACAPP_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            shell_fprintf(sh, SHELL_NORMAL, "{\"type\":\"time\",\"hour\":");
            file_json_unsigned_or_null(sh, value->type.Time.hour, UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, ",\"minute\":");
            file_json_unsigned_or_null(sh, value->type.Time.min, UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, ",\"second\":");
            file_json_unsigned_or_null(sh, value->type.Time.sec, UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, ",\"hundredths\":");
            file_json_unsigned_or_null(
                sh, value->type.Time.hundredths, UINT8_MAX);
            shell_fprintf(sh, SHELL_NORMAL, "}");
            break;
#endif
#if defined(BACAPP_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID: {
            const char *object_type_name = bactext_object_type_name_default(
                value->type.Object_Id.type, "unknown");

            shell_fprintf(
                sh, SHELL_NORMAL,
                "{\"type\":\"object-identifier\",\"object_type\":%u,"
                "\"object_type_name\":",
                (unsigned)value->type.Object_Id.type);
            file_json_string(sh, object_type_name);
            shell_fprintf(
                sh, SHELL_NORMAL, ",\"instance\":%u}",
                (unsigned)value->type.Object_Id.instance);
        } break;
#endif
        default:
            shell_fprintf(
                sh, SHELL_NORMAL, "{\"type\":\"unsupported\",\"tag\":%u}",
                (unsigned)value->tag);
            break;
    }
}

static bool
file_application_data_is_primitive(const uint8_t *data, uint32_t data_len)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    uint32_t offset = 0;
    int decoded_len;

    while (offset < data_len) {
        decoded_len = bacapp_decode_application_data(
            &data[offset], data_len - offset, &value);
        if ((decoded_len <= 0) || ((uint32_t)decoded_len > data_len - offset)) {
            return false;
        }
        offset += (uint32_t)decoded_len;
    }

    return true;
}

static void file_json_application_values(
    const struct shell *sh, const uint8_t *data, uint32_t data_len)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    uint32_t offset = 0;
    int decoded_len;
    bool first = true;

    shell_fprintf(sh, SHELL_NORMAL, "[");
    if (!file_application_data_is_primitive(data, data_len)) {
        shell_fprintf(sh, SHELL_NORMAL, "{\"type\":\"encoded\",\"data_hex\":");
        file_json_hex(sh, data, data_len);
        shell_fprintf(sh, SHELL_NORMAL, "}]");
        return;
    }
    while (offset < data_len) {
        decoded_len = bacapp_decode_application_data(
            &data[offset], data_len - offset, &value);
        if (!first) {
            shell_fprintf(sh, SHELL_NORMAL, ",");
        }
        file_json_application_value(sh, &value);
        offset += (uint32_t)decoded_len;
        first = false;
    }
    shell_fprintf(sh, SHELL_NORMAL, "]");
}

static int file_export_crc(
    const struct shell *sh,
    uint32_t instance,
    uint32_t file_size,
    uint32_t *crc)
{
    uint8_t data[FILE_CHUNK_SIZE];
    uint32_t offset = 0;
    uint32_t requested;
    uint32_t bytes_read;

    *crc = 0;
    while (offset < file_size) {
        requested = MIN(file_size - offset, FILE_CHUNK_SIZE);
        bytes_read =
            bacfile_read_offset(instance, (int32_t)offset, data, requested);
        if (bytes_read != requested) {
            shell_error(
                sh, "Short read while computing CRC at offset %u", offset);
            return -EIO;
        }
        *crc = crc32_ieee_update(*crc, data, bytes_read);
        offset += bytes_read;
    }

    return 0;
}

static int cmd_file_export(const struct shell *sh, size_t argc, char **argv)
{
    uint8_t data[FILE_CHUNK_SIZE];
    char hex[FILE_HEX_BUFFER_SIZE];
    uint32_t instance;
    uint32_t file_size;
    uint32_t offset;
    uint32_t requested;
    uint32_t bytes_read;
    uint32_t crc;
    const char *pathname;
    const char *name;
    const char *mime_type;
    int err;

    if (argc != 2U) {
        shell_error(sh, "Usage: bacnet file export <instance>");
        return -EINVAL;
    }
    err = file_instance_parse(sh, argv[1], &instance);
    if (err) {
        return err;
    }
    err = file_path_require(sh, instance, &pathname);
    if (err) {
        return err;
    }
    err = file_stream_require(sh, instance);
    if (err) {
        return err;
    }

    file_size = bacfile_file_size(instance);
    if (file_size > INT32_MAX) {
        shell_error(sh, "File is too large for stream offsets");
        return -EOVERFLOW;
    }
    err = file_export_crc(sh, instance, file_size, &crc);
    if (err) {
        return err;
    }

    name = bacfile_name_ansi(instance);
    mime_type = bacfile_file_type(instance);
    shell_print(sh, "{");
    shell_print(sh, "\"instance\":%u,", (unsigned)instance);
    shell_fprintf(sh, SHELL_NORMAL, "\"name\":");
    file_json_string(sh, name ? name : "");
    shell_print(sh, ",");
    shell_fprintf(sh, SHELL_NORMAL, "\"mime_type\":");
    file_json_string(sh, mime_type ? mime_type : "");
    shell_print(sh, ",");
    shell_print(sh, "\"size\":%u,", (unsigned)file_size);
    shell_print(
        sh, "\"read_only\":%s,",
        bacfile_read_only(instance) ? "true" : "false");
    shell_print(sh, "\"crc32\":\"%08x\",", (unsigned)crc);
    shell_print(sh, "\"data\":[");

    offset = 0;
    while (offset < file_size) {
        requested = MIN(file_size - offset, FILE_CHUNK_SIZE);
        bytes_read =
            bacfile_read_offset(instance, (int32_t)offset, data, requested);
        if (bytes_read != requested) {
            shell_error(sh, "Short read while exporting at offset %u", offset);
            return -EIO;
        }
        if (bin2hex(data, bytes_read, hex, sizeof(hex)) != 2U * bytes_read) {
            shell_error(sh, "Unable to encode export data as hex");
            return -EIO;
        }
        shell_print(
            sh, "{\"instance\":%u,\"offset\":%u,\"data\":\"%s\"}%s",
            (unsigned)instance, (unsigned)offset, hex,
            (offset + bytes_read < file_size) ? "," : "");
        offset += bytes_read;
    }
    shell_print(sh, "]");
    shell_print(sh, "}");

    return 0;
}

static int cmd_file_import(const struct shell *sh, size_t argc, char **argv)
{
    struct bacnet_file_import_chunk chunk = { 0 };
    uint32_t instance;
    uint32_t offset;
    int64_t parsed_fields;

    if (argc != 2U) {
        shell_error(
            sh,
            "Usage: bacnet file import "
            "'{\"instance\":N,\"offset\":N,\"data\":\"<hex>\"}'");
        return -EINVAL;
    }
    parsed_fields = json_obj_parse(
        argv[1], strlen(argv[1]), file_import_chunk_descr,
        ARRAY_SIZE(file_import_chunk_descr), &chunk);
    if (parsed_fields != 0x7) {
        shell_error(
            sh, "Import requires instance, offset, and data fields: %lld",
            (long long)parsed_fields);
        return -EINVAL;
    }
    if (!file_import_uint32_parse(&chunk.instance, &instance) ||
        !file_import_uint32_parse(&chunk.offset, &offset) || !chunk.data) {
        shell_error(sh, "Import instance, offset, and data must be valid");
        return -EINVAL;
    }
    if (!bacfile_valid_instance(instance)) {
        shell_error(sh, "File instance %u does not exist", (unsigned)instance);
        return -EINVAL;
    }
    if (offset > INT32_MAX) {
        shell_error(sh, "Import offset is too large");
        return -EINVAL;
    }

    return file_write_hex(sh, instance, offset, chunk.data);
}

static int file_initial_values_validate(const BACNET_CREATE_OBJECT_DATA *data)
{
    BACNET_CREATE_OBJECT_PROPERTY_VALUE value = { 0 };
    uint32_t offset = 0;
    uint32_t data_len;
    int decoded_len;

    if ((data->application_data_len < 0) ||
        ((size_t)data->application_data_len > sizeof(data->application_data))) {
        return -EBADMSG;
    }
    data_len = (uint32_t)data->application_data_len;
    while (offset < data_len) {
        decoded_len = create_object_decode_initial_value(
            &data->application_data[offset], data_len - offset, &value);
        if ((decoded_len <= 0) || ((uint32_t)decoded_len > data_len - offset)) {
            return -EBADMSG;
        }
        offset += (uint32_t)decoded_len;
    }

    return 0;
}

static void file_json_initial_values(
    const struct shell *sh, const BACNET_CREATE_OBJECT_DATA *data)
{
    BACNET_CREATE_OBJECT_PROPERTY_VALUE value = { 0 };
    uint32_t offset = 0;
    bool first = true;
    int decoded_len;

    shell_fprintf(sh, SHELL_NORMAL, "[");
    while (offset < (uint32_t)data->application_data_len) {
        decoded_len = create_object_decode_initial_value(
            &data->application_data[offset],
            (uint32_t)data->application_data_len - offset, &value);
        if (!first) {
            shell_fprintf(sh, SHELL_NORMAL, ",");
        }
        shell_fprintf(
            sh, SHELL_NORMAL, "{\"property_identifier\":%u,\"property_name\":",
            (unsigned)value.propertyIdentifier);
        file_json_string(
            sh,
            bactext_property_name_default(value.propertyIdentifier, "unknown"));
        shell_fprintf(sh, SHELL_NORMAL, ",\"array_index\":");
        if (value.propertyArrayIndex == BACNET_ARRAY_ALL) {
            shell_fprintf(sh, SHELL_NORMAL, "null");
        } else {
            shell_fprintf(
                sh, SHELL_NORMAL, "%u", (unsigned)value.propertyArrayIndex);
        }
        shell_fprintf(sh, SHELL_NORMAL, ",\"priority\":");
        if (value.priority == BACNET_NO_PRIORITY) {
            shell_fprintf(sh, SHELL_NORMAL, "null");
        } else {
            shell_fprintf(sh, SHELL_NORMAL, "%u", (unsigned)value.priority);
        }
        shell_fprintf(sh, SHELL_NORMAL, ",\"value\":");
        file_json_application_values(
            sh, value.application_data, (uint32_t)value.application_data_len);
        shell_fprintf(sh, SHELL_NORMAL, "}");
        offset += (uint32_t)decoded_len;
        first = false;
    }
    shell_fprintf(sh, SHELL_NORMAL, "]");
}

static void file_json_create_object(
    const struct shell *sh,
    uint32_t offset,
    const BACNET_CREATE_OBJECT_DATA *data)
{
    shell_fprintf(
        sh, SHELL_NORMAL,
        "{\"offset\":%u,\"object_type\":%u,\"object_type_name\":",
        (unsigned)offset, (unsigned)data->object_type);
    file_json_string(
        sh, bactext_object_type_name_default(data->object_type, "unknown"));
    shell_fprintf(
        sh, SHELL_NORMAL, ",\"object_instance\":%u,\"initial_values\":",
        (unsigned)data->object_instance);
    file_json_initial_values(sh, data);
    shell_print(sh, "}");
}

static int cmd_file_decode(const struct shell *sh, size_t argc, char **argv)
{
    uint8_t apdu[MAX_APDU];
    BACNET_CREATE_OBJECT_DATA data = { 0 };
    uint32_t instance;
    uint32_t file_size;
    uint32_t offset = 0;
    uint32_t bytes_read;
    int decoded_len;
    const char *pathname;
    int err;

    if (argc != 2U) {
        shell_error(sh, "Usage: bacnet file decode <backup-instance>");
        return -EINVAL;
    }
    err = file_instance_parse(sh, argv[1], &instance);
    if (err) {
        return err;
    }
    if (!Device_Is_Configuration_File(instance)) {
        shell_error(sh, "File %u is not a configured backup file", instance);
        return -EPERM;
    }
    err = file_path_require(sh, instance, &pathname);
    if (err) {
        return err;
    }
    err = file_stream_require(sh, instance);
    if (err) {
        return err;
    }

    file_size = bacfile_file_size(instance);
    if (file_size > INT32_MAX) {
        shell_error(sh, "Backup file is too large for stream offsets");
        return -EOVERFLOW;
    }
    while (offset < file_size) {
        bytes_read =
            bacfile_read_offset(instance, (int32_t)offset, apdu, sizeof(apdu));
        if (bytes_read == 0U) {
            shell_error(
                sh, "Unable to read backup record at offset %u", offset);
            return -EIO;
        }
        memset(&data, 0, sizeof(data));
        decoded_len =
            create_object_decode_service_request(apdu, bytes_read, &data);
        if ((decoded_len <= 0) || ((uint32_t)decoded_len > bytes_read)) {
            shell_error(
                sh, "Malformed or truncated CreateObject record at offset %u",
                offset);
            return -EBADMSG;
        }
        if (!Device_Object_Functions_Find(data.object_type)) {
            shell_error(
                sh, "Unsupported object type %s at offset %u",
                bactext_object_type_name(data.object_type), offset);
            return -ENOTSUP;
        }
        err = file_initial_values_validate(&data);
        if (err) {
            shell_error(
                sh, "Malformed CreateObject initial values at offset %u",
                offset);
            return err;
        }
        file_json_create_object(sh, offset, &data);
        offset += (uint32_t)decoded_len;
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    file_sub_cmd,
    SHELL_CMD(list, NULL, "list BACnet File objects", cmd_file_list),
    SHELL_CMD(info, NULL, "<instance> show File object details", cmd_file_info),
    SHELL_CMD(
        read,
        NULL,
        "<instance> <offset> [len] read a hex chunk",
        cmd_file_read),
    SHELL_CMD(
        write,
        NULL,
        "<instance> <offset> <hex> write a hex chunk",
        cmd_file_write),
    SHELL_CMD(
        dump,
        NULL,
        "<instance> [offset] [len] xxd -r compatible hexdump",
        cmd_file_dump),
    SHELL_CMD(
        export,
        NULL,
        "<instance> export a JSON document with chunk data",
        cmd_file_export),
    SHELL_CMD(
        import, NULL, "'<json>' import one JSON hex chunk", cmd_file_import),
    SHELL_CMD(
        decode,
        NULL,
        "<instance> inspect configured CreateObject backup data",
        cmd_file_decode),
    SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD(
    (bacnet), file, &file_sub_cmd, "BACnet File object commands", NULL, 1, 0);
