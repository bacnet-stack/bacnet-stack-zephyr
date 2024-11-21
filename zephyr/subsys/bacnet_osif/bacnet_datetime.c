/**
 * @file
 * @brief Operating system interface for calendar time
 * @author Steve Karg
 * @date August 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/basic/sys/mstimer.h"
/* BACnet Stack API */
#include "bacnet/datetime.h"

/* local time */
static BACNET_DATE_TIME BACnet_Date_Time;
static int16_t UTC_Offset_Minutes;
/* note: need starting and stopping dates/times to determine DST*/
static bool DST_Active;
static struct mstimer Date_Timer;

/**
 * @brief Synchronize the local time from the millisecond timer
 */
void datetime_sync(void)
{
	bacnet_time_t seconds, elapsed_seconds;
	unsigned long milliseconds;

	milliseconds = mstimer_elapsed(&Date_Timer);
	elapsed_seconds = milliseconds / 1000UL;
	if (elapsed_seconds) {
		mstimer_restart(&Date_Timer);
		seconds = datetime_seconds_since_epoch(&BACnet_Date_Time);
		seconds += elapsed_seconds;
		datetime_since_epoch_seconds(&BACnet_Date_Time, seconds);
		/* generate a hundredths value */
		milliseconds -= (elapsed_seconds * 1000UL);
		BACnet_Date_Time.time.hundredths = milliseconds / 10;
	}
}

/**
 * @brief Get the local date and time
 * @param bdate [out] The date to get
 * @param btime [out] The time to get
 * @param utc_offset_minutes [out] The UTC offset in minutes
 * @param dst_active [out] The DST flag
 * @return true if successful, false on error
 */
bool datetime_local(BACNET_DATE *bdate, BACNET_TIME *btime,
		    int16_t *utc_offset_minutes, bool *dst_active)
{
	datetime_sync();
	if (bdate) {
		datetime_copy_date(bdate, &BACnet_Date_Time.date);
	}
	if (btime) {
		datetime_copy_time(btime, &BACnet_Date_Time.time);
	}
	if (utc_offset_minutes) {
		*utc_offset_minutes = UTC_Offset_Minutes;
	}
	if (dst_active) {
		*dst_active = DST_Active;
	}

	return true;
}

/**
 * @brief Set the local date and time from a BACnet TimeSynchronization request
 * @param bdate [in] The date to set
 * @param btime [in] The time to set
 * @param utc [in] true if originating from an UTCTimeSynchronization request
 */
void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
	BACNET_DATE_TIME utc_time = { 0 };
	int8_t dst_adjust_minutes = 0;

	if (utc) {
		datetime_copy_date(&utc_time.date, bdate);
		datetime_copy_time(&utc_time.time, btime);
		if (DST_Active) {
			dst_adjust_minutes = 60;
		}
		datetime_utc_to_local(&BACnet_Date_Time, &utc_time,
				      UTC_Offset_Minutes, dst_adjust_minutes);
	} else {
		datetime_copy_date(&BACnet_Date_Time.date, bdate);
		datetime_copy_time(&BACnet_Date_Time.time, btime);
	}
	mstimer_restart(&Date_Timer);
	(void)utc;
}

/**
 * @brief Initialize the local date and time timer
 */
void datetime_init(void)
{
	mstimer_set(&Date_Timer, 0);
}
