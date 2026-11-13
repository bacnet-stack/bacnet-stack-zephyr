/**
 * @file
 * @brief BACnet shell commands for debugging and testing lighting output
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2025
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
#include <bacnet_shell/bacnet_shell.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/basic/object/lo.h"

/**
 * @brief Parse an unsigned integer shell argument with range checking
 * @param shell [in] shell instance used for error reporting
 * @param argv [in] shell argument vector
 * @param index [in] index of the argument to parse
 * @param max [in] maximum allowed value
 * @param name [in] argument name used for error reporting
 * @param value [out] parsed value
 * @return 0 on success, or negative errno on failure
 */
static int cmd_arg_unsigned(
    const struct shell *shell,
    char **argv,
    int index,
    unsigned long max,
    const char *name,
    unsigned long *value)
{
    unsigned long long_value;
    int err = 0;

    long_value = shell_strtoul(argv[index], 0, &err);
    if ((err == 0) && (long_value > max)) {
        err = -ERANGE;
    }
    if (err != 0) {
        shell_error(shell, "argv[%d]=%s invalid %s", index, argv[index], name);
        return err;
    }
    *value = long_value;

    return 0;
}

/**
 * @brief Parse a floating point shell argument
 * @param shell [in] shell instance used for error reporting
 * @param argv [in] shell argument vector
 * @param index [in] index of the argument to parse
 * @param name [in] argument name used for error reporting
 * @param value [out] parsed value
 * @return 0 on success, or negative errno on failure
 */
static int cmd_arg_float(
    const struct shell *shell,
    char **argv,
    int index,
    const char *name,
    float *value)
{
    double double_value;
    char *end = NULL;
    int err = 0;

    /* shell has no strtod helper, so mirror the shell_strtoul checks */
    errno = 0;
    double_value = strtod(argv[index], &end);
    if (errno == ERANGE) {
        err = -ERANGE;
    } else if ((errno != 0) || (end == argv[index]) || (*end != '\0')) {
        err = -EINVAL;
    }
    if (err != 0) {
        shell_error(shell, "argv[%d]=%s invalid %s", index, argv[index], name);
        return err;
    }
    *value = (float)double_value;

    return 0;
}

/**
 * @brief Parse a BACnet object instance shell argument
 * @param shell [in] shell instance used for error reporting
 * @param argv [in] shell argument vector
 * @param index [in] index of the argument to parse
 * @param instance [out] parsed object instance
 * @return 0 on success, or negative errno on failure
 */
static int cmd_arg_instance(
    const struct shell *shell, char **argv, int index, uint32_t *instance)
{
    unsigned long long_value = 0;
    int err;

    err = cmd_arg_unsigned(
        shell, argv, index, BACNET_MAX_INSTANCE, "instance", &long_value);
    if (err == 0) {
        *instance = (uint32_t)long_value;
    }

    return err;
}

/**
 * @brief Parse a BACnet priority shell argument
 * @param shell [in] shell instance used for error reporting
 * @param argv [in] shell argument vector
 * @param index [in] index of the argument to parse
 * @param priority [out] parsed priority, 0..16
 * @return 0 on success, or negative errno on failure
 */
static int cmd_arg_priority(
    const struct shell *shell, char **argv, int index, unsigned *priority)
{
    unsigned long long_value = 0;
    int err;

    err = cmd_arg_unsigned(
        shell, argv, index, BACNET_MAX_PRIORITY, "priority", &long_value);
    if (err == 0) {
        *priority = (unsigned)long_value;
    }

    return err;
}

/**
 * @brief Parse a lighting output instance argument and validate it exists
 * @param shell [in] shell instance used for error reporting
 * @param argv [in] shell argument vector
 * @param index [in] index of the argument to parse
 * @param instance [out] parsed object instance
 * @return 0 on success, or negative errno on failure
 */
static int cmd_arg_valid_instance(
    const struct shell *shell, char **argv, int index, uint32_t *instance)
{
    int err;

    err = cmd_arg_instance(shell, argv, index, instance);
    if (err != 0) {
        return err;
    }
    if (Lighting_Output_Valid_Instance(*instance) == false) {
        shell_error(shell, "argv[%d]=%s invalid instance", index, argv[index]);
        return -EINVAL;
    }

    return 0;
}

static int
cmd_lighting_output_value_print(const struct shell *shell, uint32_t instance)
{
    double present_value, tracking_value;
    unsigned priority = 0;
    bool overridden = false;
    bool out_of_service = false;

    if (Lighting_Output_Valid_Instance(instance) == false) {
        return -EINVAL;
    }
    present_value = (double)Lighting_Output_Present_Value(instance);
    tracking_value = (double)Lighting_Output_Tracking_Value(instance);
    priority = Lighting_Output_Present_Value_Priority(instance);
    overridden = Lighting_Output_Overridden_Status(instance);
    out_of_service = Lighting_Output_Out_Of_Service(instance);
    shell_print(
        shell, "lighting-output:%u %.1f%%@%d->%.1f %s %s", instance,
        present_value, priority, tracking_value,
        (overridden ? "overridden" : "tracking"),
        (out_of_service ? "out-of-service" : "in-service"));
    return 0;
}

static int
cmd_lighting_output_list(const struct shell *shell, int argc, char **argv)
{
    unsigned count, index;
    uint32_t instance = 0;

    ARG_UNUSED(argv);
    ARG_UNUSED(argc);
    count = Lighting_Output_Count();
    for (index = 0; index < count; index++) {
        instance = Lighting_Output_Index_To_Instance(index);
        cmd_lighting_output_value_print(shell, instance);
    }
    return 0;
}

static double cmd_priority_array_value(bool is_relinquished, float value)
{
    if (is_relinquished) {
        return INFINITY;
    }
    return (double)value;
}

static int
cmd_lighting_output_value_get(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    unsigned priority;
    float priority_array[BACNET_MAX_PRIORITY + 1] = { 0.0f };
    double tracking_value;
    bool is_relinquished[BACNET_MAX_PRIORITY + 1] = { false };
    bool overridden = false;
    bool out_of_service = false;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    priority_array[BACNET_MAX_PRIORITY] =
        Lighting_Output_Relinquish_Default(instance);
    for (priority = BACNET_MIN_PRIORITY; priority <= BACNET_MAX_PRIORITY;
         priority++) {
        priority_array[priority - 1] =
            Lighting_Output_Priority_Array_Value(instance, priority);
        is_relinquished[priority - 1] =
            Lighting_Output_Priority_Array_Relinquished(instance, priority);
    }
    overridden = Lighting_Output_Overridden_Status(instance);
    out_of_service = Lighting_Output_Out_Of_Service(instance);
    tracking_value = (double)Lighting_Output_Tracking_Value(instance);
    shell_print(
        shell,
        "lighting-output-%d %.1f "
        "{%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,"
        "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f} %s %s",
        instance, tracking_value,
        cmd_priority_array_value(is_relinquished[0], priority_array[0]),
        cmd_priority_array_value(is_relinquished[1], priority_array[1]),
        cmd_priority_array_value(is_relinquished[2], priority_array[2]),
        cmd_priority_array_value(is_relinquished[3], priority_array[3]),
        cmd_priority_array_value(is_relinquished[4], priority_array[4]),
        cmd_priority_array_value(is_relinquished[5], priority_array[5]),
        cmd_priority_array_value(is_relinquished[6], priority_array[6]),
        cmd_priority_array_value(is_relinquished[7], priority_array[7]),
        cmd_priority_array_value(is_relinquished[8], priority_array[8]),
        cmd_priority_array_value(is_relinquished[9], priority_array[9]),
        cmd_priority_array_value(is_relinquished[10], priority_array[10]),
        cmd_priority_array_value(is_relinquished[11], priority_array[11]),
        cmd_priority_array_value(is_relinquished[12], priority_array[12]),
        cmd_priority_array_value(is_relinquished[13], priority_array[13]),
        cmd_priority_array_value(is_relinquished[14], priority_array[14]),
        cmd_priority_array_value(is_relinquished[15], priority_array[15]),
        (double)priority_array[BACNET_MAX_PRIORITY],
        (overridden ? "overridden" : "tracking"),
        (out_of_service ? "out-of-service" : "in-service"));

    return 0;
}

static int
cmd_lighting_output_track(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    shell_print(
        shell, "lighting-output-%d %.1f", instance,
        (double)Lighting_Output_Tracking_Value(instance));

    return 0;
}

static int
cmd_lighting_output_override(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    float float_value = 0.0f;
    float ramp_value = 0.0f;
    int err;

    /* <instance> <clear|ramp <percent> <rate>|<percent> [momentary] */
    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        if (bacnet_strnicmp(argv[2], "clear", 5) == 0) {
            /* clear the override */
            Lighting_Output_Overridden_Clear(instance);
            return cmd_lighting_output_value_print(shell, instance);
        } else if (bacnet_strnicmp(argv[2], "ramp", 4) == 0) {
            if (argc < 5) {
                shell_help(shell);
                return -EINVAL;
            }
            /* <level in percent>*/
            err = cmd_arg_float(shell, argv, 3, "percentage", &float_value);
            if (err != 0) {
                return err;
            }
            /* <ramp in percent per second>*/
            err = cmd_arg_float(shell, argv, 4, "ramp rate", &ramp_value);
            if (err != 0) {
                return err;
            }
            /* ramp the override */
            Lighting_Output_Overridden_Ramp(instance, float_value, ramp_value);
        } else {
            /* <level in percent>*/
            err = cmd_arg_float(shell, argv, 2, "percentage", &float_value);
            if (err != 0) {
                return err;
            }
            if (argc > 3) {
                if (bacnet_strnicmp(argv[3], "momentary", 9) == 0) {
                    /* momentarily override the value */
                    Lighting_Output_Overridden_Momentary(instance, float_value);
                } else {
                    shell_help(shell);
                    return -EINVAL;
                }
            } else {
                /* set the override without momentary */
                Lighting_Output_Overridden_Set(instance, float_value);
            }
        }
    }
    return cmd_lighting_output_value_print(shell, instance);
}

static int
cmd_lighting_output_value_set(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    unsigned priority = 0;
    float float_value = 0.0f;
    bool relinquish = false;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        if (bacnet_strnicmp(argv[2], "NULL", 4) == 0) {
            relinquish = true;
        } else {
            /* <level in percent>*/
            err = cmd_arg_float(shell, argv, 2, "percentage", &float_value);
            if (err != 0) {
                return err;
            }
        }
        priority = Lighting_Output_Default_Priority(instance);
        if (argc > 3) {
            /* [priority] */
            err = cmd_arg_priority(shell, argv, 3, &priority);
            if (err != 0) {
                return err;
            }
        }
        if (relinquish) {
            Lighting_Output_Present_Value_Relinquish(instance, priority);
        } else {
            Lighting_Output_Present_Value_Set(instance, float_value, priority);
        }
    }
    return cmd_lighting_output_value_print(shell, instance);
}

static int cmd_lighting_output_value_relinquish(
    const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    unsigned priority = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    priority = Lighting_Output_Default_Priority(instance);
    if (argc > 2) {
        /* [priority] */
        err = cmd_arg_priority(shell, argv, 2, &priority);
        if (err != 0) {
            return err;
        }
    }
    Lighting_Output_Present_Value_Relinquish(instance, priority);
    return cmd_lighting_output_value_print(shell, instance);
}

static int
cmd_lighting_output_fade(const struct shell *shell, int argc, char **argv)
{
    BACNET_LIGHTING_COMMAND data = { 0 };
    uint32_t instance = 0;
    float float_value = 0.0f;
    unsigned priority = 0;
    unsigned long long_value = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        /* <level in percent>*/
        err = cmd_arg_float(shell, argv, 2, "percentage", &float_value);
        if (err != 0) {
            return err;
        }
        /* [fade in milliseconds] */
        if (argc > 3) {
            err = cmd_arg_unsigned(
                shell, argv, 3, UINT32_MAX, "fade time", &long_value);
            if (err != 0) {
                return err;
            }
        }
        if (argc > 4) {
            /* [priority] */
            err = cmd_arg_priority(shell, argv, 4, &priority);
            if (err != 0) {
                return err;
            }
        }
        data.operation = BACNET_LIGHTS_FADE_TO;
        data.use_fade_time = true;
        data.fade_time = (uint32_t)long_value;
        data.use_target_level = true;
        data.target_level = float_value;
        if (priority >= BACNET_MIN_PRIORITY) {
            data.priority = priority;
            data.use_priority = true;
        }
        (void)Lighting_Output_Lighting_Command_Set(instance, &data);
    }
    return cmd_lighting_output_value_print(shell, instance);
}

static int
cmd_lighting_output_ramp(const struct shell *shell, int argc, char **argv)
{
    BACNET_LIGHTING_COMMAND data = { 0 };
    uint32_t instance = 0;
    float float_value = 0.0f;
    float ramp_rate = 100.0f;
    unsigned priority = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        /* <level in percent>*/
        err = cmd_arg_float(shell, argv, 2, "percentage", &float_value);
        if (err != 0) {
            return err;
        }
        /* [ramp percent] */
        if (argc > 3) {
            err = cmd_arg_float(shell, argv, 3, "ramp rate", &ramp_rate);
            if (err != 0) {
                return err;
            }
        }
        if (argc > 4) {
            /* [priority] */
            err = cmd_arg_priority(shell, argv, 4, &priority);
            if (err != 0) {
                return err;
            }
        }
        data.operation = BACNET_LIGHTS_RAMP_TO;
        if (isgreater(ramp_rate, 0.0f)) {
            data.use_ramp_rate = true;
            data.ramp_rate = ramp_rate;
        }
        data.use_target_level = true;
        data.target_level = float_value;
        if (priority >= BACNET_MIN_PRIORITY) {
            data.priority = priority;
            data.use_priority = true;
        }
        (void)Lighting_Output_Lighting_Command_Set(instance, &data);
    }
    return cmd_lighting_output_value_print(shell, instance);
}

static int
cmd_lighting_output_stop(const struct shell *shell, int argc, char **argv)
{
    BACNET_LIGHTING_COMMAND data = { 0 };
    uint32_t instance = 0;
    unsigned priority = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        /* [priority] */
        err = cmd_arg_priority(shell, argv, 2, &priority);
        if (err != 0) {
            return err;
        }
    }
    data.operation = BACNET_LIGHTS_STOP;
    if (priority >= BACNET_MIN_PRIORITY) {
        data.priority = priority;
        data.use_priority = true;
    }
    (void)Lighting_Output_Lighting_Command_Set(instance, &data);
    return cmd_lighting_output_value_print(shell, instance);
}

static int cmd_lighting_output_step(
    const struct shell *shell,
    int argc,
    char **argv,
    BACNET_LIGHTING_OPERATION operation)
{
    uint32_t instance = 0;
    unsigned priority = 0;
    float step_increment = 0.0f;
    BACNET_LIGHTING_COMMAND data = { 0 };
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    /* <instance> */
    err = cmd_arg_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        /* <level in percent>*/
        err = cmd_arg_float(shell, argv, 2, "percentage", &step_increment);
        if (err != 0) {
            return err;
        }
        if (argc > 3) {
            /* [priority] */
            err = cmd_arg_priority(shell, argv, 3, &priority);
            if (err != 0) {
                return err;
            }
        }
        data.operation = operation;
        data.use_step_increment = true;
        data.step_increment = step_increment;
        if (priority >= BACNET_MIN_PRIORITY) {
            data.priority = priority;
            data.use_priority = true;
        }
        (void)Lighting_Output_Lighting_Command_Set(instance, &data);
    }
    return cmd_lighting_output_value_print(shell, instance);
}

static int
cmd_lighting_output_step_up(const struct shell *shell, int argc, char **argv)
{
    const BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_STEP_UP;

    return cmd_lighting_output_step(shell, argc, argv, operation);
}

static int
cmd_lighting_output_step_down(const struct shell *shell, int argc, char **argv)
{
    const BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_STEP_DOWN;

    return cmd_lighting_output_step(shell, argc, argv, operation);
}

static int
cmd_lighting_output_step_on(const struct shell *shell, int argc, char **argv)
{
    const BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_STEP_ON;

    return cmd_lighting_output_step(shell, argc, argv, operation);
}

static int
cmd_lighting_output_step_off(const struct shell *shell, int argc, char **argv)
{
    const BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_STEP_OFF;

    return cmd_lighting_output_step(shell, argc, argv, operation);
}

/**
 * @brief Command a blink-warn lighting operation using the configured
 *  blink and egress properties
 * @note <instance> [priority]
 */
static int cmd_lighting_output_blink(
    const struct shell *shell,
    int argc,
    char **argv,
    BACNET_LIGHTING_OPERATION operation)
{
    BACNET_LIGHTING_COMMAND data = { 0 };
    uint32_t instance = 0;
    unsigned priority = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        err = cmd_arg_priority(shell, argv, 2, &priority);
        if (err != 0) {
            return err;
        }
    }
    data.operation = operation;
    if (priority >= BACNET_MIN_PRIORITY) {
        data.priority = priority;
        data.use_priority = true;
    }
    (void)Lighting_Output_Lighting_Command_Set(instance, &data);
    return cmd_lighting_output_value_print(shell, instance);
}

static int
cmd_lighting_output_blink_print(const struct shell *shell, uint32_t instance)
{
    float off_value = 0.0f;
    uint16_t interval = 0;
    uint16_t count = 0;

    (void)Lighting_Output_Blink_Warn_Feature(
        instance, &off_value, &interval, &count);
    shell_print(
        shell,
        "lighting-output-%u blink-warn %s off=%.1f%% interval=%ums count=%u",
        instance, Lighting_Output_Blink_Warn_Enable(instance) ? "on" : "off",
        (double)off_value, interval, count);

    return 0;
}

/**
 * @brief Set the blink-warn off value, interval, and count
 * @note <instance> <off percent> <interval milliseconds> [count]
 */
static int
cmd_lighting_output_blink_set(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    float off_value = 0.0f;
    uint16_t interval = 0;
    uint16_t count = UINT16_MAX;
    unsigned long long_value = 0;
    int err;

    if (argc < 4) {
        shell_help(shell);
        return -EINVAL;
    }
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    err = cmd_arg_float(shell, argv, 2, "off percentage", &off_value);
    if (err != 0) {
        return err;
    }
    err = cmd_arg_unsigned(
        shell, argv, 3, UINT16_MAX, "interval ms", &long_value);
    if (err != 0) {
        return err;
    }
    interval = (uint16_t)long_value;
    if (argc > 4) {
        /* 65535 blinks for the whole egress time */
        err =
            cmd_arg_unsigned(shell, argv, 4, UINT16_MAX, "count", &long_value);
        if (err != 0) {
            return err;
        }
        count = (uint16_t)long_value;
    }
    (void)Lighting_Output_Blink_Warn_Feature_Set(
        instance, off_value, interval, count);

    return cmd_lighting_output_blink_print(shell, instance);
}

/**
 * @note <instance>
 */
static int
cmd_lighting_output_blink_get(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }

    return cmd_lighting_output_blink_print(shell, instance);
}

/**
 * @brief Get or set the blink-warn-enable property
 * @note <instance> [on|off]
 */
static int cmd_lighting_output_blink_enable(
    const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    if (argc > 2) {
        if (bacnet_stricmp(argv[2], "on") == 0) {
            (void)Lighting_Output_Blink_Warn_Enable_Set(instance, true);
        } else if (bacnet_stricmp(argv[2], "off") == 0) {
            (void)Lighting_Output_Blink_Warn_Enable_Set(instance, false);
        } else {
            shell_error(shell, "argv[2]=%s invalid enable", argv[2]);
            return -EINVAL;
        }
    }

    return cmd_lighting_output_blink_print(shell, instance);
}

static int
cmd_lighting_output_egress_print(const struct shell *shell, uint32_t instance)
{
    shell_print(
        shell, "lighting-output-%u egress-time=%us egress-active=%s", instance,
        Lighting_Output_Egress_Time(instance),
        Lighting_Output_Egress_Active(instance) ? "true" : "false");

    return 0;
}

/**
 * @note <instance> <egress seconds>
 */
static int
cmd_lighting_output_egress_set(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    unsigned long long_value = 0;
    int err;

    if (argc < 3) {
        shell_help(shell);
        return -EINVAL;
    }
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }
    err = cmd_arg_unsigned(
        shell, argv, 2, UINT32_MAX, "egress seconds", &long_value);
    if (err != 0) {
        return err;
    }
    (void)Lighting_Output_Egress_Time_Set(instance, (uint32_t)long_value);

    return cmd_lighting_output_egress_print(shell, instance);
}

/**
 * @note <instance>
 */
static int
cmd_lighting_output_egress_get(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    int err;

    if (argc < 2) {
        shell_help(shell);
        return -EINVAL;
    }
    err = cmd_arg_valid_instance(shell, argv, 1, &instance);
    if (err != 0) {
        return err;
    }

    return cmd_lighting_output_egress_print(shell, instance);
}

static int
cmd_lighting_output_blink_warn(const struct shell *shell, int argc, char **argv)
{
    const BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_WARN;

    return cmd_lighting_output_blink(shell, argc, argv, operation);
}

static int cmd_lighting_output_blink_warn_off(
    const struct shell *shell, int argc, char **argv)
{
    const BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_WARN_OFF;

    return cmd_lighting_output_blink(shell, argc, argv, operation);
}

static int cmd_lighting_output_blink_warn_relinquish(
    const struct shell *shell, int argc, char **argv)
{
    const BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_WARN_RELINQUISH;

    return cmd_lighting_output_blink(shell, argc, argv, operation);
}

BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_step_up)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_step_down)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_step_on)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_step_off)

SHELL_STATIC_SUBCMD_SET_CREATE(
    lighting_output_step_sub_cmd,
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        up,
        NULL,
        "<up> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_up),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        down,
        NULL,
        "<down> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_down),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        on,
        NULL,
        "<on> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_on),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        off,
        NULL,
        "<off> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_off),
    SHELL_SUBCMD_SET_END);

BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_blink_set)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_blink_get)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_blink_enable)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_blink_warn)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_blink_warn_off)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_blink_warn_relinquish)

SHELL_STATIC_SUBCMD_SET_CREATE(
    lighting_output_blink_sub_cmd,
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        set,
        NULL,
        "<instance> <off percent> <interval milliseconds> [count]",
        cmd_lighting_output_blink_set),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        get,
        NULL,
        "<instance>",
        cmd_lighting_output_blink_get),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        enable,
        NULL,
        "<instance> [on|off]",
        cmd_lighting_output_blink_enable),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        warn,
        NULL,
        "<instance> [priority]",
        cmd_lighting_output_blink_warn),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        warn_off,
        NULL,
        "<instance> [priority]",
        cmd_lighting_output_blink_warn_off),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        warn_relinquish,
        NULL,
        "<instance> [priority]",
        cmd_lighting_output_blink_warn_relinquish),
    SHELL_SUBCMD_SET_END);

BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_egress_set)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_egress_get)

SHELL_STATIC_SUBCMD_SET_CREATE(
    lighting_output_egress_sub_cmd,
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        set,
        NULL,
        "<instance> <egress seconds>",
        cmd_lighting_output_egress_set),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        get,
        NULL,
        "<instance>",
        cmd_lighting_output_egress_get),
    SHELL_SUBCMD_SET_END);

BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_list)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_value_get)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_value_set)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_value_relinquish)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_fade)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_ramp)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_stop)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_override)
BACNET_SHELL_LOCK_HANDLER(cmd_lighting_output_track)

SHELL_STATIC_SUBCMD_SET_CREATE(
    lighting_output_sub_cmd,
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        list,
        NULL,
        "list all the lights",
        cmd_lighting_output_list),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        get,
        NULL,
        "<instance>",
        cmd_lighting_output_value_get),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        set,
        NULL,
        "<instance> <level percent> [priority]",
        cmd_lighting_output_value_set),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        relinquish,
        NULL,
        "<instance> [priority]",
        cmd_lighting_output_value_relinquish),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        fade,
        NULL,
        "<instance> <level percent> "
        "[fade milliseconds] [priority]",
        cmd_lighting_output_fade),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        ramp,
        NULL,
        "<instance> <level percent> "
        "[ramp percent] [priority]",
        cmd_lighting_output_ramp),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        stop,
        NULL,
        "<instance> [priority]",
        cmd_lighting_output_stop),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        step,
        &lighting_output_step_sub_cmd,
        "<up|down|on|off>",
        NULL),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        blink,
        &lighting_output_blink_sub_cmd,
        "<set|get|enable|warn|warn_off|warn_relinquish>",
        NULL),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        egress,
        &lighting_output_egress_sub_cmd,
        "<set|get>",
        NULL),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        override,
        NULL,
        "<instance> <clear|ramp <percent> <rate>|<percent> [momentary]",
        cmd_lighting_output_override),
    BACNET_SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        track,
        NULL,
        "<instance>",
        cmd_lighting_output_track),
    SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD(
    (bacnet),
    light,
    &lighting_output_sub_cmd,
    "BACnet Lighting Output Commands",
    NULL,
    1,
    0);
