#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "bacnet/bacdef.h"
#include "bacnet/create_object.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/device.h"

#define TEST_FILE_INSTANCE 7U
#define OTHER_FILE_INSTANCE 8U
#define MOCK_FILE_CAPACITY 4096U

static uint8_t Mock_File_Data[MOCK_FILE_CAPACITY];
static size_t Mock_File_Size;

static size_t test_file_read(
    const char *pathname, int32_t offset, uint8_t *buffer, size_t buffer_size)
{
    size_t position;
    size_t bytes_to_read;

    ARG_UNUSED(pathname);
    if (offset < 0) {
        return 0;
    }
    position = (size_t)offset;
    if (position >= Mock_File_Size) {
        return 0;
    }
    bytes_to_read = MIN(buffer_size, Mock_File_Size - position);
    memcpy(buffer, &Mock_File_Data[position], bytes_to_read);

    return bytes_to_read;
}

static size_t test_file_write(
    const char *pathname,
    int32_t offset,
    const uint8_t *buffer,
    size_t buffer_size)
{
    size_t position;

    ARG_UNUSED(pathname);
    if (offset < 0) {
        return 0;
    }
    position = (size_t)offset;
    if ((position > sizeof(Mock_File_Data)) ||
        (buffer_size > sizeof(Mock_File_Data) - position)) {
        return 0;
    }
    if (position == 0U) {
        Mock_File_Size = 0;
    }
    if (position > Mock_File_Size) {
        memset(&Mock_File_Data[Mock_File_Size], 0, position - Mock_File_Size);
    }
    memcpy(&Mock_File_Data[position], buffer, buffer_size);
    if (position + buffer_size > Mock_File_Size) {
        Mock_File_Size = position + buffer_size;
    }

    return buffer_size;
}

static size_t test_file_size(const char *pathname)
{
    ARG_UNUSED(pathname);
    return Mock_File_Size;
}

static void *file_shell_setup(void)
{
    const struct shell *sh = shell_backend_dummy_get_ptr();

    WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
    zassert_true(shell_ready(sh), "dummy shell did not become ready");

    Device_Init(NULL);
    zassert_equal(
        bacfile_create(TEST_FILE_INSTANCE), TEST_FILE_INSTANCE,
        "failed to create test File object");
    zassert_true(
        bacfile_object_name_set(TEST_FILE_INSTANCE, "shell-test.bin"), NULL);
    zassert_true(
        bacfile_file_type_set(TEST_FILE_INSTANCE, "application/test"), NULL);
    zassert_true(
        bacfile_pathname_set(TEST_FILE_INSTANCE, "shell-test.bin"), NULL);
    zassert_true(
        bacfile_file_access_stream_set(TEST_FILE_INSTANCE, true), NULL);

    bacfile_read_stream_data_callback_set(test_file_read);
    bacfile_write_stream_data_callback_set(test_file_write);
    bacfile_file_size_callback_set(test_file_size);
    zassert_true(Device_Configuration_File_Set(0, TEST_FILE_INSTANCE), NULL);

    return NULL;
}

static void file_shell_before(void *fixture)
{
    const struct shell *sh = shell_backend_dummy_get_ptr();

    ARG_UNUSED(fixture);
    zassert_true(
        bacfile_valid_instance(TEST_FILE_INSTANCE),
        "test File object disappeared before the next case");
    memset(Mock_File_Data, 0, sizeof(Mock_File_Data));
    Mock_File_Size = 0;
    bacfile_read_only_set(TEST_FILE_INSTANCE, false);
    bacfile_file_access_stream_set(TEST_FILE_INSTANCE, true);
    Device_Configuration_File_Set(0, TEST_FILE_INSTANCE);
    shell_backend_dummy_clear_output(sh);
}

static int shell_run(const char *command, const char **output)
{
    const struct shell *sh = shell_backend_dummy_get_ptr();
    size_t output_size;
    int err;

    shell_backend_dummy_clear_output(sh);
    err = shell_execute_cmd(sh, command);
    *output = shell_backend_dummy_get_output(sh, &output_size);
    ARG_UNUSED(output_size);

    return err;
}

static size_t encode_create_request(
    uint8_t *buffer, BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    BACNET_CREATE_OBJECT_DATA data = { 0 };

    data.object_type = object_type;
    data.object_instance = object_instance;
    return create_object_service_request_encode(buffer, MAX_APDU, &data);
}

static void store_backup_data(const uint8_t *data, size_t data_size)
{
    zassert_true(data_size <= sizeof(Mock_File_Data), NULL);
    memcpy(Mock_File_Data, data, data_size);
    Mock_File_Size = data_size;
}

ZTEST(bacnet_shell_file, test_write_read_roundtrip)
{
    const char *output;
    int err;

    err = shell_run("bacnet file write 7 0 48656c6c6f", &output);
    zassert_equal(err, 0, "write command failed: %d (%s)", err, output);
    err = shell_run("bacnet file read 7 0 8", &output);
    zassert_equal(err, 0, "read command failed: %d (%s)", err, output);
    zassert_not_null(strstr(output, "0 5 48656c6c6f EOF"), "%s", output);
}

ZTEST(bacnet_shell_file, test_invalid_hex_and_instance)
{
    const char *output;
    int err;

    err = shell_run("bacnet file write 7 0 abc", &output);
    zassert_not_equal(err, 0, "odd-length hex was accepted");
    zassert_not_null(strstr(output, "even number"), "%s", output);

    err = shell_run("bacnet file read 99 0", &output);
    zassert_not_equal(err, 0, "invalid instance was accepted");
    zassert_not_null(strstr(output, "does not exist"), "%s", output);
}

ZTEST(bacnet_shell_file, test_read_only_file)
{
    const char *output;
    int err;

    zassert_true(bacfile_read_only_set(TEST_FILE_INSTANCE, true), NULL);
    err = shell_run("bacnet file write 7 0 41", &output);
    zassert_not_equal(err, 0, "write to read-only File object succeeded");
    zassert_not_null(strstr(output, "read-only"), "%s", output);
}

ZTEST(bacnet_shell_file, test_chunk_limit)
{
    char hex[2U * CONFIG_BACNETSTACK_BACNET_FILE_SHELL_CHUNK_SIZE + 3U];
    char command[CONFIG_SHELL_CMD_BUFF_SIZE];
    const char *output;
    size_t hex_size = sizeof(hex) - 1U;
    int err;

    memset(hex, '0', hex_size);
    hex[hex_size] = '\0';
    snprintk(command, sizeof(command), "bacnet file write 7 0 %s", hex);
    err = shell_run(command, &output);
    zassert_not_equal(err, 0, "oversized chunk was accepted");
    zassert_not_null(strstr(output, "chunk limit"), "%s", output);
}

ZTEST(bacnet_shell_file, test_info_list_and_dump)
{
    const char *output;
    int err;

    err = shell_run("bacnet file list", &output);
    zassert_equal(err, 0, "list command failed: %d (%s)", err, output);
    zassert_not_null(
        strstr(output, "7 shell-test.bin application/test 0 read-write"), "%s",
        output);

    err = shell_run("bacnet file info 7", &output);
    zassert_equal(err, 0, "info command failed: %d (%s)", err, output);
    zassert_not_null(strstr(output, "pathname: shell-test.bin"), "%s", output);
    zassert_not_null(strstr(output, "stream-access: true"), "%s", output);

    err = shell_run("bacnet file write 7 0 48656c6c6f", &output);
    zassert_equal(err, 0, "write command failed: %d (%s)", err, output);
    err = shell_run("bacnet file dump 7", &output);
    zassert_equal(err, 0, "dump command failed: %d (%s)", err, output);
    zassert_not_null(strstr(output, "00000000: 4865 6c6c 6f"), "%s", output);
    zassert_not_null(strstr(output, "|Hello"), "%s", output);
}

ZTEST(bacnet_shell_file, test_export_import_roundtrip)
{
    char command[CONFIG_SHELL_CMD_BUFF_SIZE];
    const char *output;
    const char *hex_start;
    const char *hex_end;
    size_t hex_len;
    int err;

    err = shell_run("bacnet file write 7 0 48656c6c6f", &output);
    zassert_equal(err, 0, "write command failed: %d (%s)", err, output);
    err = shell_run("bacnet file export 7", &output);
    zassert_equal(err, 0, "export command failed: %d (%s)", err, output);
    zassert_not_null(strstr(output, "\"crc32\":"), "%s", output);
    hex_start = strstr(output, "\"data\":\"");
    zassert_not_null(hex_start, "export chunk missing: %s", output);
    hex_start += strlen("\"data\":\"");
    hex_end = strchr(hex_start, '"');
    zassert_not_null(hex_end, "export chunk is unterminated: %s", output);
    hex_len = (size_t)(hex_end - hex_start);
    zassert_true(hex_len > 0U, "exported chunk is empty");

    snprintk(
        command, sizeof(command),
        "bacnet file import '{\"instance\":7,\"offset\":0,\"data\":\"%.*s\"}'",
        (int)hex_len, hex_start);
    err = shell_run(command, &output);
    zassert_equal(err, 0, "import command failed: %d (%s)", err, output);
    err = shell_run("bacnet file read 7 0 8", &output);
    zassert_equal(err, 0, "read command failed: %d (%s)", err, output);
    zassert_not_null(strstr(output, "0 5 48656c6c6f EOF"), "%s", output);
}

ZTEST(bacnet_shell_file, test_non_stream_file)
{
    const char *output;
    int err;

    zassert_true(
        bacfile_file_access_stream_set(TEST_FILE_INSTANCE, false), NULL);
    err = shell_run("bacnet file read 7 0", &output);
    zassert_not_equal(err, 0, "record-access file was accepted");
    zassert_not_null(strstr(output, "stream access"), "%s", output);
}

ZTEST(bacnet_shell_file, test_decode_create_object_backup)
{
    uint8_t request[MAX_APDU];
    const char *output;
    size_t request_size;
    int err;

    request_size =
        encode_create_request(request, OBJECT_FILE, OTHER_FILE_INSTANCE);
    zassert_true(
        request_size > 0U, "failed to encode test CreateObject request");
    store_backup_data(request, request_size);

    err = shell_run("bacnet file decode 7", &output);
    zassert_equal(err, 0, "decode command failed: %d (%s)", err, output);
    zassert_not_null(strstr(output, "offset=0"), "%s", output);
    zassert_not_null(strstr(output, "instance=8"), "%s", output);
}

ZTEST(bacnet_shell_file, test_decode_malformed_and_truncated_records)
{
    uint8_t request[MAX_APDU];
    const uint8_t malformed[] = { 0xff };
    const char *output;
    size_t request_size;
    int err;

    store_backup_data(malformed, sizeof(malformed));
    err = shell_run("bacnet file decode 7", &output);
    zassert_not_equal(err, 0, "malformed record was accepted");
    zassert_not_null(strstr(output, "offset 0"), "%s", output);

    request_size =
        encode_create_request(request, OBJECT_FILE, TEST_FILE_INSTANCE);
    zassert_true(
        request_size > 1U, "failed to encode test CreateObject request");
    store_backup_data(request, request_size - 1U);
    err = shell_run("bacnet file decode 7", &output);
    zassert_not_equal(err, 0, "truncated record was accepted");
    zassert_not_null(strstr(output, "offset 0"), "%s", output);
}

ZTEST(bacnet_shell_file, test_decode_unsupported_and_non_backup_files)
{
    uint8_t request[MAX_APDU];
    const char *output;
    size_t request_size;
    int err;

    request_size = encode_create_request(
        request, (BACNET_OBJECT_TYPE)(MAX_BACNET_OBJECT_TYPE - 1U),
        TEST_FILE_INSTANCE);
    zassert_true(request_size > 0U, "failed to encode unsupported request");
    store_backup_data(request, request_size);
    err = shell_run("bacnet file decode 7", &output);
    zassert_not_equal(err, 0, "unsupported object was accepted");
    zassert_not_null(strstr(output, "offset 0"), "%s", output);

    Device_Configuration_File_Set(0, OTHER_FILE_INSTANCE);
    err = shell_run("bacnet file decode 7", &output);
    zassert_not_equal(err, 0, "non-configuration File object was accepted");
    zassert_not_null(
        strstr(output, "not a configured backup file"), "%s", output);
}

ZTEST_SUITE(
    bacnet_shell_file, NULL, file_shell_setup, file_shell_before, NULL, NULL);
