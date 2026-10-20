/**
 * @file
 * @brief Regression tests for bacnet_storage_erase()
 * @copyright SPDX-License-Identifier: Apache-2.0
 *
 * bacnet_storage_erase() wipes the whole settings partition to recover from
 * a corrupt or foreign storage partition, and must never erase the flash
 * that holds the running firmware (CONFIG_XIP).
 *
 * The flash_area_open() and flash_area_erase() calls made by
 * bacnet_storage.c are redirected (see CMakeLists.txt) to the functions below
 * so that the tests can count the erase calls, inject flash errors, and
 * prove that a refused erase never reaches the flash driver.
 * Everything else uses the real flash map and the flash simulator.
 *
 * Scenarios (selected by the test CMakeLists.txt):
 * - default (native_sim): no XIP, full recovery and error tests
 * - BACNET_TEST_XIP_OTHER_DEVICE: XIP image on a different flash device
 *   than the settings partition, so the guard must not refuse the erase
 * - BACNET_TEST_XIP_OVERLAP: the settings partition lives on the flash
 *   device that holds the firmware and overlaps the image, so the guard
 *   must refuse the erase
 */
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <bacnet_settings/bacnet_storage.h>

#if defined(BACNET_TEST_XIP_OVERLAP) || defined(BACNET_TEST_XIP_OTHER_DEVICE)
#if !defined(CONFIG_XIP)
#error "the XIP guard scenarios must be built for an XIP target"
#endif
#endif

/* same partition selection as bacnet_storage.c */
#if DT_HAS_CHOSEN(zephyr_settings_partition)
#define SETTINGS_PARTITION_NODE DT_CHOSEN(zephyr_settings_partition)
#else
#define SETTINGS_PARTITION_NODE DT_NODELABEL(storage_partition)
#endif
#define SETTINGS_PARTITION_ID DT_FIXED_PARTITION_ID(SETTINGS_PARTITION_NODE)

#define CORRUPT_BYTE 0xA5
#define SENTINEL_BYTE 0x5A
#define CHUNK_SIZE 64

/* simulates a reboot: the settings subsystem forgets its backend */
extern bool settings_subsys_initialized;

static struct {
    unsigned int erase_calls;
    off_t erase_off;
    size_t erase_len;
    int erase_error;
    int open_error;
} mock;

int bacnet_test_flash_area_open(uint8_t id, const struct flash_area **fap);
int bacnet_test_flash_area_erase(
    const struct flash_area *fa, off_t off, size_t len);

int bacnet_test_flash_area_open(uint8_t id, const struct flash_area **fap)
{
    if (mock.open_error) {
        return mock.open_error;
    }

    return flash_area_open(id, fap);
}

int bacnet_test_flash_area_erase(
    const struct flash_area *fa, off_t off, size_t len)
{
    mock.erase_calls++;
    mock.erase_off = off;
    mock.erase_len = len;
    if (mock.erase_error) {
        return mock.erase_error;
    }

    return flash_area_erase(fa, off, len);
}

static const struct flash_area *partition_open(void)
{
    const struct flash_area *fa = NULL;

    zassert_equal(flash_area_open(SETTINGS_PARTITION_ID, &fa), 0, NULL);

    return fa;
}

static void simulate_reboot(void)
{
    settings_subsys_initialized = false;
}

static void partition_erase(const struct flash_area *fa)
{
    zassert_equal(flash_area_erase(fa, 0, fa->fa_size), 0, NULL);
}

static void partition_fill(const struct flash_area *fa, uint8_t value)
{
    uint8_t chunk[CHUNK_SIZE];
    off_t off;

    memset(chunk, value, sizeof(chunk));
    for (off = 0; off < fa->fa_size; off += sizeof(chunk)) {
        zassert_equal(flash_area_write(fa, off, chunk, sizeof(chunk)), 0, NULL);
    }
}

/* true when every byte of the partition equals the value */
static bool partition_is_all(const struct flash_area *fa, uint8_t value)
{
    uint8_t chunk[CHUNK_SIZE];
    off_t off;
    size_t i;

    for (off = 0; off < fa->fa_size; off += sizeof(chunk)) {
        zassert_equal(flash_area_read(fa, off, chunk, sizeof(chunk)), 0, NULL);
        for (i = 0; i < sizeof(chunk); i++) {
            if (chunk[i] != value) {
                return false;
            }
        }
    }

    return true;
}

static void corrupt_partition(const struct flash_area *fa)
{
    partition_erase(fa);
    partition_fill(fa, CORRUPT_BYTE);
}

static void erase_suite_reset(void *fixture)
{
    const struct flash_area *fa;

    ARG_UNUSED(fixture);

    memset(&mock, 0, sizeof(mock));
    fa = partition_open();
    partition_erase(fa);
    simulate_reboot();
}

static void erase_suite_cleanup(void *fixture)
{
    erase_suite_reset(fixture);
}

#if !defined(BACNET_TEST_XIP_OVERLAP)

/* flash bytes just outside the partition, used to detect an over-erase */
struct neighbor {
    bool present;
    off_t off;
    uint8_t data[CHUNK_SIZE];
};

/* mark the CHUNK_SIZE bytes at absolute flash offset 'off' */
static void
neighbor_mark(const struct device *dev, off_t off, struct neighbor *n)
{
    struct flash_pages_info info;

    memset(n, 0, sizeof(*n));
    if (flash_get_page_info_by_offs(dev, off, &info) != 0) {
        /* partition is at the edge of the flash device */
        return;
    }
    zassert_equal(flash_erase(dev, info.start_offset, info.size), 0, NULL);
    memset(n->data, SENTINEL_BYTE, sizeof(n->data));
    zassert_equal(flash_write(dev, off, n->data, sizeof(n->data)), 0, NULL);
    n->present = true;
    n->off = off;
}

static void neighbor_check(const struct device *dev, const struct neighbor *n)
{
    uint8_t chunk[CHUNK_SIZE];

    if (!n->present) {
        return;
    }
    zassert_equal(flash_read(dev, n->off, chunk, sizeof(chunk)), 0, NULL);
    zassert_mem_equal(chunk, n->data, sizeof(chunk), NULL);
}

/* A corrupt partition makes settings init fail with -EDEADLK. The erase
   must recover it so that init succeeds after the reboot. */
ZTEST(bacnet_storage_erase, test_erase_recovers_corrupt_storage)
{
    const struct flash_area *fa = partition_open();
    BACNET_STORAGE_KEY key;
    const char *value = "recovered";
    char readback[16] = { 0 };

    corrupt_partition(fa);
    simulate_reboot();
    zassert_equal(bacnet_storage_init(), -EDEADLK, "corrupt init must fail");
    /* a failed init must not be latched as initialized */
    zassert_false(settings_subsys_initialized, NULL);

    zassert_equal(bacnet_storage_erase(), 0, NULL);
    zassert_equal(mock.erase_calls, 1U, NULL);
    zassert_equal(mock.erase_off, 0, NULL);
    zassert_equal(mock.erase_len, fa->fa_size, NULL);
    zassert_true(
        partition_is_all(fa, flash_area_erased_val(fa)), "not fully erased");

    simulate_reboot();
    zassert_equal(bacnet_storage_init(), 0, "init after erase must succeed");

    bacnet_storage_key_init(&key, 1, 2, 3, BACNET_STORAGE_ARRAY_INDEX_NONE);
    zassert_equal(bacnet_storage_set(&key, value, strlen(value) + 1), 0, NULL);
    zassert_equal(
        bacnet_storage_get(&key, readback, sizeof(readback)),
        (int)(strlen(value) + 1), NULL);
    zassert_str_equal(readback, value, NULL);
}

/* The erase also wipes healthy data: nothing survives the reboot. */
ZTEST(bacnet_storage_erase, test_erase_discards_valid_storage)
{
    BACNET_STORAGE_KEY key;
    const char *value = "stale";
    char readback[16] = { 0 };

    zassert_equal(bacnet_storage_init(), 0, NULL);
    bacnet_storage_key_init(&key, 4, 5, 6, BACNET_STORAGE_ARRAY_INDEX_NONE);
    zassert_equal(bacnet_storage_set(&key, value, strlen(value) + 1), 0, NULL);
    zassert_true(
        bacnet_storage_get(&key, readback, sizeof(readback)) > 0, NULL);

    zassert_equal(bacnet_storage_erase(), 0, NULL);
    simulate_reboot();
    zassert_equal(bacnet_storage_init(), 0, NULL);
    zassert_equal(
        bacnet_storage_get(&key, readback, sizeof(readback)), -ENOENT, NULL);
}

/* Erasing a blank partition is harmless and repeatable. */
ZTEST(bacnet_storage_erase, test_erase_blank_partition)
{
    const struct flash_area *fa = partition_open();

    zassert_equal(bacnet_storage_erase(), 0, NULL);
    zassert_equal(bacnet_storage_erase(), 0, NULL);
    zassert_equal(mock.erase_calls, 2U, NULL);
    zassert_true(partition_is_all(fa, flash_area_erased_val(fa)), NULL);
}

/* Only the selected partition is erased: the flash right before and right
   after it must keep its bytes. */
ZTEST(bacnet_storage_erase, test_erase_preserves_bytes_outside_partition)
{
    const struct flash_area *fa = partition_open();
    const struct device *dev = fa->fa_dev;
    struct neighbor before;
    struct neighbor after;

    neighbor_mark(dev, fa->fa_off - CHUNK_SIZE, &before);
    neighbor_mark(dev, fa->fa_off + fa->fa_size, &after);
    zassert_true(
        before.present || after.present,
        "no flash outside the partition to check");
    corrupt_partition(fa);
    neighbor_check(dev, &before);
    neighbor_check(dev, &after);

    zassert_equal(bacnet_storage_erase(), 0, NULL);

    zassert_true(
        partition_is_all(fa, flash_area_erased_val(fa)), "not fully erased");
    neighbor_check(dev, &before);
    neighbor_check(dev, &after);
}

/* A flash driver error is returned unchanged, and storage stays as is. */
ZTEST(bacnet_storage_erase, test_erase_propagates_flash_erase_error)
{
    const struct flash_area *fa = partition_open();
    const int errors[] = { -EIO, -EACCES, -ENODEV };
    size_t i;

    for (i = 0; i < ARRAY_SIZE(errors); i++) {
        memset(&mock, 0, sizeof(mock));
        mock.erase_error = errors[i];
        corrupt_partition(fa);

        zassert_equal(bacnet_storage_erase(), errors[i], "error %d", errors[i]);
        zassert_equal(mock.erase_calls, 1U, NULL);
        zassert_true(
            partition_is_all(fa, CORRUPT_BYTE), "partition changed on error");
    }

    /* once the flash recovers, the same call succeeds */
    mock.erase_error = 0;
    zassert_equal(bacnet_storage_erase(), 0, NULL);
    zassert_true(partition_is_all(fa, flash_area_erased_val(fa)), NULL);
}

/* Failing to open the partition is returned and nothing is erased. */
ZTEST(bacnet_storage_erase, test_erase_propagates_flash_open_error)
{
    const struct flash_area *fa = partition_open();

    corrupt_partition(fa);
    mock.open_error = -ENOENT;

    zassert_equal(bacnet_storage_erase(), -ENOENT, NULL);
    zassert_equal(mock.erase_calls, 0U, NULL);
    zassert_true(partition_is_all(fa, CORRUPT_BYTE), NULL);
}

#endif /* !BACNET_TEST_XIP_OVERLAP */

#if defined(BACNET_TEST_XIP_OVERLAP)

/* The settings partition shares the flash device with the running image and
   overlaps it: the erase must be refused before reaching the flash driver. */
ZTEST(bacnet_storage_erase, test_erase_refuses_firmware_overlap)
{
    const struct flash_area *fa = partition_open();

    corrupt_partition(fa);
    /* an erase that reached the driver would fail differently */
    mock.erase_error = -EIO;

    zassert_equal(bacnet_storage_erase(), -EPERM, NULL);
    zassert_equal(mock.erase_calls, 0U, "flash erase must not be invoked");
    zassert_true(
        partition_is_all(fa, CORRUPT_BYTE), "partition changed despite guard");
}

/* The refusal is stable, and does not depend on the partition content. */
ZTEST(bacnet_storage_erase, test_erase_refuses_firmware_overlap_repeatedly)
{
    const struct flash_area *fa = partition_open();

    partition_fill(fa, flash_area_erased_val(fa));
    zassert_equal(bacnet_storage_erase(), -EPERM, NULL);
    zassert_equal(bacnet_storage_erase(), -EPERM, NULL);
    zassert_equal(mock.erase_calls, 0U, NULL);
}

/* An open failure still wins over the guard and is reported unchanged. */
ZTEST(bacnet_storage_erase, test_erase_overlap_open_error_propagates)
{
    mock.open_error = -ENODEV;
    zassert_equal(bacnet_storage_erase(), -ENODEV, NULL);
    zassert_equal(mock.erase_calls, 0U, NULL);
}

#endif /* BACNET_TEST_XIP_OVERLAP */

ZTEST_SUITE(
    bacnet_storage_erase,
    NULL,
    NULL,
    erase_suite_reset,
    erase_suite_cleanup,
    NULL);
