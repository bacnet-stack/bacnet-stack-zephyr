/**
 * @file
 * @brief The BACnet shell commands for ReinitializeDevice
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2026
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
#include "bacnet/bacapp.h"
/* BACnet Device API */
#include "bacnet/basic/object/device.h"

/**
 * @brief Print BACnet Reinitialized_State
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_reinit(const struct shell *sh, size_t argc, char **argv)
{
    BACNET_REINITIALIZE_DEVICE_DATA rd_data = { 0 };
    BACNET_REINITIALIZED_STATE state = BACNET_REINIT_IDLE;
    uint32_t found_index = 0;
    bool status = false;
    const char *name = NULL, *password = NULL, *error_name = NULL;
    const char *service_name = "ReinitializeDevice";
    const char *param_name = "state";
    const char *result_name = "error-code";

    rd_data.state = BACNET_REINIT_IDLE;
    if ((argc == 3) || (argc == 2) || (argc == 1)) {
        if ((argc == 3) || (argc == 2)) {
            /* Request ReinitializeDevice without password */
            if (bactext_reinitialized_state_strtol(argv[1], &found_index)) {
                rd_data.state = found_index;
            }
        }
        if (argc == 3) {
            /* Request ReinitializeDevice with password */
            password = argv[2];
            characterstring_init_ansi(&rd_data.password, password);
        }
        if (rd_data.state == BACNET_REINIT_IDLE) {
            /* Print current ReinitializeDevice state */
            state = Device_Reinitialized_State();
            name = bactext_reinitialized_state_name_default(state, "Unknown");
            shell_print(
                sh, "{\"%s\":{\"%s\":\"%s\"}}", service_name, param_name, name);
        } else {
            /* Request ReinitializeDevice */
            status = Device_Reinitialize(&rd_data);
            name =
                bactext_reinitialized_state_name_default(rd_data.state, NULL);
            if (status) {
                error_name = bactext_error_code_name(ERROR_CODE_SUCCESS);
            } else {
                error_name = bactext_error_code_name(rd_data.error_code);
            }
            service_name = "ReinitializeDevice-Request";
            if (name) {
                shell_print(
                    sh, "{\"%s\":{\"%s\":\"%s\",\"%s\":\"%s\"}}", service_name,
                    param_name, name, result_name, error_name);
            } else {
                shell_print(
                    sh, "{\"%s\":{\"%s\":%d,\"%s\":\"%s\"}}", service_name,
                    param_name, rd_data.state, result_name, error_name);
            }
        }
        return 0;
    }
    shell_help(sh);
    return -EINVAL;
}

SHELL_SUBCMD_ADD(
    (bacnet), reinit, NULL, "[state] [password]", cmd_reinit, 0, 0);
