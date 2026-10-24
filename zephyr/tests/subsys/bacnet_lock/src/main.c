/**
 * @file
 * @brief BACnet task lock tests for the clock store
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "bacnet/bacdef.h"
#include "bacnet/datetime.h"
#include "bacnet/basic/server/bacnet_basic.h"
#include <bacnet_osif/bacnet_lock.h>

#define CLOCK_PAUSE_MS 100
#define SHELL_WRITES 4

static struct k_thread Reader_Thread;
static K_THREAD_STACK_DEFINE(Reader_Stack, 2048);
static struct k_thread Bacnet_Thread;
static K_THREAD_STACK_DEFINE(Bacnet_Stack, 8192);

static atomic_t Reader_Run;
static atomic_t Bacnet_Run;
static atomic_t Saw_Tear;
static atomic_t Read_Count;
static atomic_t Bad_Year;
static atomic_t Bad_Month;
static atomic_t Bad_Day;
static atomic_t Bad_Hour;

static void clock_test_pause(void)
{
    k_msleep(CLOCK_PAUSE_MS);
}

static bool
clock_pair_is_consistent(const BACNET_DATE *date, const BACNET_TIME *time)
{
    if ((date->year == BACNET_DATE_YEAR_EPOCH) && (date->month == 1) &&
        (date->day == 1) && (time->hour == 0)) {
        return true;
    }
    if ((date->year == 2000) && (date->month == 1) && (date->day == 1) &&
        (time->hour == 1)) {
        return true;
    }
    if ((date->year == 2099) && (date->month == 12) && (date->day == 31) &&
        (time->hour == 23)) {
        return true;
    }

    return false;
}

static void record_bad_pair(const BACNET_DATE *date, const BACNET_TIME *time)
{
    atomic_set(&Bad_Year, date->year);
    atomic_set(&Bad_Month, date->month);
    atomic_set(&Bad_Day, date->day);
    atomic_set(&Bad_Hour, time->hour);
    atomic_set(&Saw_Tear, 1);
}

/**
 * Lower priority than the test thread. It runs while that thread sleeps
 * inside the clock pause.
 */
static void reader_entry(void *p1, void *p2, void *p3)
{
    BACNET_DATE date;
    BACNET_TIME time;

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    while (atomic_get(&Reader_Run)) {
        datetime_local(&date, &time, NULL, NULL);
        if ((date.year == BACNET_DATE_YEAR_EPOCH) && (time.hour == 23)) {
            atomic_set(&Saw_Tear, 1);
        }
        /* Sleep so this thread cannot starve the writer that is inside
           the clock pause. */
        k_msleep(1);
    }
}

static void bacnet_entry(void *p1, void *p2, void *p3)
{
    BACNET_DATE date;
    BACNET_TIME time;

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    while (atomic_get(&Bacnet_Run)) {
        bacnet_lock();
        bacnet_basic_task();
        datetime_local(&date, &time, NULL, NULL);
        if (!clock_pair_is_consistent(&date, &time)) {
            record_bad_pair(&date, &time);
        }
        atomic_inc(&Read_Count);
        bacnet_unlock();
        k_msleep(1);
    }
}

static void *lock_setup(void)
{
    const struct shell *sh = shell_backend_dummy_get_ptr();

    WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
    zassert_true(shell_ready(sh), "dummy shell did not become ready");
    datetime_timesync_test_pause_set(clock_test_pause);
    bacnet_lock();
    bacnet_basic_init();
    bacnet_unlock();

    return NULL;
}

ZTEST(bacnet_lock, test_clock_tear_without_lock)
{
    BACNET_DATE date;
    BACNET_TIME time;
    k_tid_t tid;

    datetime_init_defaults();
    atomic_set(&Saw_Tear, 0);
    atomic_set(&Reader_Run, 1);
    tid = k_thread_create(
        &Reader_Thread, Reader_Stack, K_THREAD_STACK_SIZEOF(Reader_Stack),
        reader_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
    datetime_set_date(&date, 2099, 12, 31);
    datetime_set_time(&time, 23, 0, 0, 0);
    datetime_timesync(&date, &time, false);
    atomic_set(&Reader_Run, 0);
    k_thread_join(tid, K_FOREVER);
    zassert_equal(
        atomic_get(&Saw_Tear), 1,
        "reader did not observe the new time with the previous date");
}

ZTEST(bacnet_lock, test_clock_consistent_with_lock)
{
    const struct shell *sh = shell_backend_dummy_get_ptr();
    const char *commands[] = {
        "bacnet time 2000/01/01 01:00:00",
        "bacnet time 2099/12/31 23:00:00",
    };
    k_tid_t tid;
    unsigned int i;
    int err = 0;

    datetime_init_defaults();
    atomic_set(&Saw_Tear, 0);
    atomic_set(&Read_Count, 0);
    atomic_set(&Bacnet_Run, 1);
    tid = k_thread_create(
        &Bacnet_Thread, Bacnet_Stack, K_THREAD_STACK_SIZEOF(Bacnet_Stack),
        bacnet_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
    for (i = 0; (i < SHELL_WRITES) && (err == 0); i++) {
        err = shell_execute_cmd(sh, commands[0]);
        if (err == 0) {
            k_msleep(1);
            err = shell_execute_cmd(sh, commands[1]);
        }
        if (err == 0) {
            k_msleep(1);
        }
    }
    atomic_set(&Bacnet_Run, 0);
    k_thread_join(tid, K_FOREVER);
    zassert_ok(err, "bacnet time command failed");
    zassert_true(atomic_get(&Read_Count) > 0, "BACnet thread did not read");
    zassert_equal(
        atomic_get(&Saw_Tear), 0, "torn clock %ld-%ld-%ld hour %ld",
        (long)atomic_get(&Bad_Year), (long)atomic_get(&Bad_Month),
        (long)atomic_get(&Bad_Day), (long)atomic_get(&Bad_Hour));
}

ZTEST_SUITE(bacnet_lock, NULL, lock_setup, NULL, NULL, NULL);
