/**
 * @file
 * @brief BACnet shell commands for debugging and testing date and time
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date September 2025
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/datetime.h"

static int cmd_date_time(const struct shell *shell, int argc, char **argv)
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };
    int16_t utc_offset_minutes = 0;
    bool dst_active = false;
    char date_string[40] = { 0 };
    char time_string[40] = { 0 };

    if ((argc == 3) || (argc == 1)) {
        if (argc == 3) {
            /* Set new time */
            if (datetime_date_init_ascii(&bdate, argv[1]) &&
                datetime_time_init_ascii(&btime, argv[2])) {
                datetime_timesync(&bdate, &btime, false);
            } else {
                shell_print(shell, " date time format: YYYY/MM/DD HH:MM:SS.hh");
                return -EINVAL;
            }
        }
        datetime_local(&bdate, &btime, &utc_offset_minutes, &dst_active);
        datetime_date_to_ascii(&bdate, date_string, sizeof(date_string));
        datetime_time_to_ascii(&btime, time_string, sizeof(time_string));
        shell_print(shell, "%s %s", date_string, time_string);
        return 0;
    } else {
        shell_help(shell);
        return -EINVAL;
    }
}

SHELL_SUBCMD_ADD(
    (bacnet), time, NULL, "BACnet Date Time Command", cmd_date_time, 1, 2);
