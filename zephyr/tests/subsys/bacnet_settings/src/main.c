#include <zephyr/ztest.h>
#include <string.h>
#include <stdint.h>

#include <bacnet_settings/bacnet_storage.h>

static struct {
    bool called;
    BACNET_STORAGE_KEY key;
    uint8_t data[8];
    size_t len;
} restore_state;

static int restore_cb(
    BACNET_STORAGE_KEY *key, const void *data, size_t data_size, void *context)
{
    ARG_UNUSED(context);

    restore_state.called = true;
    restore_state.key = *key;
    restore_state.len = data_size;
    memcpy(restore_state.data, data, data_size);

    return 0;
}

static int read_cb(void *cb_arg, void *data, size_t len)
{
    const uint8_t *source = cb_arg;
    memcpy(data, source, len);
    return (int)len;
}

ZTEST(bacnet_storage, test_numeric_helpers)
{
    unsigned long uvalue = 0;
    long svalue = 0;
    float fvalue = 0.0f;

    zassert_true(bacnet_storage_strtoul("123", &uvalue), NULL);
    zassert_equal(uvalue, 123UL, NULL);
    zassert_false(bacnet_storage_strtoul("12x", &uvalue), NULL);

    zassert_true(bacnet_storage_strtol("-77", &svalue), NULL);
    zassert_equal(svalue, -77L, NULL);
    zassert_false(bacnet_storage_strtol("x77", &svalue), NULL);

    zassert_true(bacnet_storage_strtof("1.25", &fvalue), NULL);
    zassert_within(fvalue, 1.25f, 0.0001f, NULL);
    zassert_false(bacnet_storage_strtof("abc", &fvalue), NULL);
}

ZTEST(bacnet_storage, test_key_encode_decode_roundtrip)
{
    char path[128];
    BACNET_STORAGE_KEY in_key;
    BACNET_STORAGE_KEY out_key;
    int rc;

    bacnet_storage_key_init(&in_key, 2, 1001, 85, 3);
    rc = bacnet_storage_key_encode(path, sizeof(path), &in_key);
    zassert_true(rc > 0, NULL);

    memset(&out_key, 0, sizeof(out_key));
    rc = bacnet_storage_key_decode(path, &out_key);
    zassert_equal(rc, 0, NULL);
    zassert_mem_equal(&in_key, &out_key, sizeof(BACNET_STORAGE_KEY), NULL);
}

ZTEST(bacnet_storage, test_key_parse)
{
    BACNET_STORAGE_KEY key;
    char *argv[] = { "set", "5", "100", "85[2]" };
    int rc;

    rc = bacnet_storage_key_parse(&key, ARRAY_SIZE(argv), argv);
    zassert_equal(rc, 0, NULL);
    zassert_equal(key.object_type, 5, NULL);
    zassert_equal(key.object_instance, 100, NULL);
    zassert_equal(key.property_id, 85, NULL);
    zassert_equal(key.array_index, 2, NULL);
}

ZTEST(bacnet_storage, test_handler_set_invokes_restore_callback)
{
    BACNET_STORAGE_KEY key;
    char path[128];
    const uint8_t sample[] = { 1, 2, 3, 4 };
    int rc;

    memset(&restore_state, 0, sizeof(restore_state));
    bacnet_storage_load_callback_set(restore_cb, NULL);

    bacnet_storage_key_init(&key, 8, 200, 111, BACNET_STORAGE_ARRAY_INDEX_NONE);
    bacnet_storage_key_encode(path, sizeof(path), &key);

    rc = bacnet_storage_handler_set(
        path, sizeof(sample), read_cb, (void *)sample);
    zassert_equal(rc, 0, NULL);
    zassert_true(restore_state.called, NULL);
    zassert_mem_equal(
        &restore_state.key, &key, sizeof(BACNET_STORAGE_KEY), NULL);
    zassert_equal(restore_state.len, sizeof(sample), NULL);
    zassert_mem_equal(restore_state.data, sample, sizeof(sample), NULL);
}

ZTEST_SUITE(bacnet_storage, NULL, NULL, NULL, NULL, NULL);
