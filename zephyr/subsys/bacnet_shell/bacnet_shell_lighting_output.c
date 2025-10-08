/*
 * Copyright (c) 2025 Legrand North America, LLC., as an unpublished work.
 * All Rights Reserved.
 *
 * The information contained herein is confidential property of Legrand.
 * The use, copying, transfer, or disclosure of such information is
 * prohibited except by the express written agreement with Legrand.
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/basic/object/lo.h"

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
    present_value = Lighting_Output_Present_Value(instance);
    tracking_value = Lighting_Output_Tracking_Value(instance);
    priority = Lighting_Output_Present_Value_Priority(instance);
    overridden = Lighting_Output_Overridden_Status(instance);
    out_of_service = Lighting_Output_Out_Of_Service(instance);
    shell_print(
        shell, "lighting-output:%u %.1f%%@%d->%.1f %s %s", instance,
        present_value, priority, tracking_value,
        (overridden ? "overridden" : ""),
        (out_of_service ? "out-of-service" : ""));
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
    unsigned long long_value;
    bool overridden = false;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance", argv[1]);
            return -EINVAL;
        }
        instance = (uint32_t)long_value;
        if (Lighting_Output_Valid_Instance(instance) == false) {
            shell_error(shell, "argv[1]=%s invalid instance", argv[1]);
            return -EINVAL;
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
        tracking_value = (double)Lighting_Output_Tracking_Value(instance);
        shell_print(
            shell,
            "lighting-output-%d %.1f "
            "{%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,"
            "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f} %s",
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
            (overridden ? "overridden" : ""));
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    return 0;
}

static int
cmd_lighting_output_track(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    unsigned long long_value;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance", argv[1]);
            return -EINVAL;
        }
        instance = (uint32_t)long_value;
        if (Lighting_Output_Valid_Instance(instance) == false) {
            shell_error(shell, "argv[1]=%s invalid instance", argv[1]);
            return -EINVAL;
        }
        shell_print(
            shell, "lighting-output-%d %.1f", instance,
            (double)Lighting_Output_Tracking_Value(instance));
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    return 0;
}

static int
cmd_lighting_output_override(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    unsigned long long_value;
    float float_value = 0.0f;
    double double_value;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance", argv[1]);
            return -EINVAL;
        }
        instance = (uint32_t)long_value;
        if (Lighting_Output_Valid_Instance(instance) == false) {
            shell_error(shell, "argv[1]=%s invalid instance", argv[1]);
            return -EINVAL;
        }
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    if (argc > 2) {
        if (bacnet_strnicmp(argv[2], "clear", 5) == 0) {
            /* clear the override */
            Lighting_Output_Overridden_Clear(instance);
            return cmd_lighting_output_value_print(shell, instance);
        } else {
            /* <level in percent>*/
            double_value = strtod(argv[2], &end);
            if (end == argv[2]) {
                shell_error(shell, "argv[2]=%s invalid percentage", argv[2]);
                return -EINVAL;
            }
            float_value = (float)double_value;
        }
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
        /* set the override */
        Lighting_Output_Overridden_Set(instance, float_value);
    }
    return cmd_lighting_output_value_print(shell, instance);
}

static int
cmd_lighting_output_value_set(const struct shell *shell, int argc, char **argv)
{
    uint32_t instance = 0;
    unsigned priority = 0;
    float float_value = 0.0f;
    double double_value;
    unsigned long long_value;
    bool relinquish = false;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance number", argv[1]);
            return -EINVAL;
        }
        instance = long_value;
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    if (argc > 2) {
        if (bacnet_strnicmp(argv[2], "NULL", 4) == 0) {
            relinquish = true;
        } else {
            /* <level in percent>*/
            double_value = strtod(argv[2], &end);
            if (end == argv[2]) {
                shell_error(shell, "argv[2]=%s invalid percentage", argv[2]);
                return -EINVAL;
            }
            float_value = (float)double_value;
        }
        priority = Lighting_Output_Default_Priority(instance);
        if (argc > 3) {
            /* [priority] */
            long_value = strtoul(argv[3], &end, 0);
            if (end == argv[3]) {
                shell_error(shell, "argv[3]=%s invalid priority", argv[3]);
                return -EINVAL;
            }
            priority = (unsigned)long_value;
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
    unsigned long long_value;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance number", argv[1]);
            return -EINVAL;
        }
        instance = long_value;
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    priority = Lighting_Output_Default_Priority(instance);
    if (argc > 2) {
        /* [priority] */
        long_value = strtoul(argv[2], &end, 0);
        if (end == argv[2]) {
            shell_error(shell, "argv[2]=%s invalid priority", argv[2]);
            return -EINVAL;
        }
        priority = (unsigned)long_value;
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
    double double_value;
    uint32_t fade_time = 0;
    unsigned long long_value;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance number", argv[1]);
            return -EINVAL;
        }
        instance = long_value;
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    if (argc > 2) {
        /* <level in percent>*/
        double_value = strtod(argv[2], &end);
        if (end == argv[2]) {
            shell_error(shell, "argv[2]=%s invalid percentage", argv[2]);
            return -EINVAL;
        }
        float_value = (float)double_value;
        /* [fade in milliseconds] */
        if (argc > 3) {
            long_value = strtoul(argv[3], &end, 0);
            if (end == argv[3]) {
                shell_error(shell, "argv[3]=%s invalid fade time", argv[2]);
                return -EINVAL;
            }
            fade_time = (uint32_t)long_value;
        }
        if (argc > 4) {
            /* [priority] */
            long_value = strtoul(argv[4], &end, 0);
            if (end == argv[4]) {
                shell_error(shell, "argv[4]=%s invalid priority", argv[4]);
                return -EINVAL;
            }
            priority = (unsigned)long_value;
        }
        data.operation = BACNET_LIGHTS_FADE_TO;
        data.use_fade_time = true;
        data.fade_time = fade_time;
        data.use_target_level = true;
        data.target_level = float_value;
        if ((priority >= BACNET_MIN_PRIORITY) &&
            (priority <= BACNET_MAX_PRIORITY)) {
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
    double double_value;
    unsigned long long_value;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance number", argv[1]);
            return -EINVAL;
        }
        instance = long_value;
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    if (argc > 2) {
        /* <level in percent>*/
        double_value = strtod(argv[2], &end);
        if (end == argv[2]) {
            shell_error(shell, "argv[2]=%s invalid percentage", argv[2]);
            return -EINVAL;
        }
        float_value = (float)double_value;
        /* [ramp percent] */
        if (argc > 3) {
            double_value = strtod(argv[3], &end);
            if (end == argv[3]) {
                shell_error(shell, "argv[3]=%s invalid percentage", argv[3]);
                return -EINVAL;
            }
            ramp_rate = (float)double_value;
        }
        if (argc > 4) {
            /* [priority] */
            long_value = strtoul(argv[4], &end, 0);
            if (end == argv[4]) {
                shell_error(shell, "argv[4]=%s invalid priority", argv[4]);
                return -EINVAL;
            }
            priority = (unsigned)long_value;
        }
        data.operation = BACNET_LIGHTS_RAMP_TO;
        if (isgreater(ramp_rate, 0.0f)) {
            data.use_ramp_rate = true;
            data.ramp_rate = ramp_rate;
        }
        data.use_target_level = true;
        data.target_level = float_value;
        if ((priority >= BACNET_MIN_PRIORITY) &&
            (priority <= BACNET_MAX_PRIORITY)) {
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
    unsigned long long_value;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance number", argv[1]);
            return -EINVAL;
        }
        instance = long_value;
        if (argc > 2) {
            /* [priority] */
            long_value = strtoul(argv[2], &end, 0);
            if (end == argv[2]) {
                shell_error(shell, "argv[2]=%s invalid priority", argv[2]);
                return -EINVAL;
            }
            priority = (unsigned)long_value;
        }
        data.operation = BACNET_LIGHTS_STOP;
        if ((priority >= BACNET_MIN_PRIORITY) &&
            (priority <= BACNET_MAX_PRIORITY)) {
            data.priority = priority;
            data.use_priority = true;
        }
        (void)Lighting_Output_Lighting_Command_Set(instance, &data);
        return cmd_lighting_output_value_print(shell, instance);
    } else {
        shell_help(shell);
        return -EINVAL;
    }
}

static int cmd_lighting_output_step(
    const struct shell *shell,
    int argc,
    char **argv,
    BACNET_LIGHTING_OPERATION operation)
{
    uint32_t instance = 0;
    unsigned priority = 0;
    unsigned long long_value;
    double double_value;
    char *end;
    float step_increment;
    BACNET_LIGHTING_COMMAND data = { 0 };

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance number", argv[1]);
            return -EINVAL;
        }
        instance = long_value;
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    if (argc > 2) {
        /* <level in percent>*/
        double_value = strtod(argv[2], &end);
        if (end == argv[2]) {
            shell_error(shell, "argv[2]=%s invalid percentage", argv[2]);
            return -EINVAL;
        }
        step_increment = (float)double_value;
        if (argc > 3) {
            /* [priority] */
            long_value = strtoul(argv[3], &end, 0);
            if (end == argv[3]) {
                shell_error(shell, "argv[3]=%s invalid priority", argv[3]);
                return -EINVAL;
            }
            priority = (unsigned)long_value;
        }
        data.operation = operation;
        data.use_step_increment = true;
        data.step_increment = step_increment;
        if ((priority >= BACNET_MIN_PRIORITY) &&
            (priority <= BACNET_MAX_PRIORITY)) {
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

static int cmd_lighting_output_blink(
    const struct shell *shell,
    int argc,
    char **argv,
    BACNET_LIGHTING_OPERATION operation)
{
    BACNET_LIGHTING_COMMAND data = { 0 };
    uint32_t instance = 0;
    float off_value = 0.0;
    uint16_t interval = 0;
    uint16_t count = 0;
    uint32_t egress_seconds = 60 * 5;
    unsigned priority = 0;
    unsigned long long_value;
    double double_value;
    char *end;

    if (argc > 1) {
        /* <instance> */
        long_value = strtoul(argv[1], &end, 0);
        if (end == argv[1]) {
            shell_error(shell, "argv[1]=%s invalid instance number", argv[1]);
            return -EINVAL;
        }
        instance = long_value;
    } else {
        shell_help(shell);
        return -EINVAL;
    }
    if (argc > 2) {
        /* <egress seconds> */
        long_value = strtoul(argv[2], &end, 0);
        if (end == argv[2]) {
            shell_error(shell, "argv[2]=%s invalid egress seconds", argv[2]);
            return -EINVAL;
        }
        egress_seconds = long_value;
    }
    if (argc > 3) {
        /* <interval milliseconds> */
        long_value = strtoul(argv[3], &end, 0);
        if (end == argv[3]) {
            shell_error(shell, "argv[3]=%s invalid interval ms", argv[3]);
            return -EINVAL;
        }
        interval = long_value;
    }
    if (argc > 4) {
        /* <off percent> */
        double_value = strtod(argv[4], &end);
        if (end == argv[4]) {
            shell_error(shell, "argv[4]=%s invalid off percentage", argv[4]);
            return -EINVAL;
        }
        off_value = (float)double_value;
    }
    if (argc > 5) {
        /* <priority> */
        long_value = strtoul(argv[5], &end, 0);
        if (end == argv[5]) {
            shell_error(shell, "argv[5]=%s invalid priority", argv[5]);
            return -EINVAL;
        }
        priority = (unsigned)long_value;
    }
    Lighting_Output_Blink_Warn_Feature_Set(
        instance, off_value, interval, count);
    Lighting_Output_Egress_Time_Set(instance, egress_seconds);
    data.operation = operation;
    if ((priority >= BACNET_MIN_PRIORITY) &&
        (priority <= BACNET_MAX_PRIORITY)) {
        data.priority = priority;
        data.use_priority = true;
    }
    (void)Lighting_Output_Lighting_Command_Set(instance, &data);
    return cmd_lighting_output_value_print(shell, instance);
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

SHELL_STATIC_SUBCMD_SET_CREATE(
    lighting_output_step_sub_cmd,
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        up,
        NULL,
        "<up> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_up),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        down,
        NULL,
        "<down> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_down),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        on,
        NULL,
        "<on> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_on),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        off,
        NULL,
        "<off> <instance> "
        "<step increment percent> <priority>",
        cmd_lighting_output_step_off),
    SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
    lighting_output_blink_sub_cmd,
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        warn,
        NULL,
        "<instance> <egress seconds> "
        "<interval milliseconds> <off percent> <priority>",
        cmd_lighting_output_blink_warn),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        warn_off,
        NULL,
        "<instance> <egress seconds> "
        "<interval milliseconds> <off percent> <priority>",
        cmd_lighting_output_blink_warn_off),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        warn_relinquish,
        NULL,
        "<instance> <egress seconds> "
        "<interval milliseconds> <off percent> <priority>",
        cmd_lighting_output_blink_warn_relinquish),
    SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
    lighting_output_sub_cmd,
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        list,
        NULL,
        "list all the lights",
        cmd_lighting_output_list),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        get,
        NULL,
        "<instance>",
        cmd_lighting_output_value_get),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        set,
        NULL,
        "<instance> <level percent> [priority]",
        cmd_lighting_output_value_set),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        relinquish,
        NULL,
        "<instance> [priority]",
        cmd_lighting_output_value_relinquish),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        fade,
        NULL,
        "<instance> <level percent> "
        "[fade milliseconds] [priority]",
        cmd_lighting_output_fade),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        ramp,
        NULL,
        "<instance> <level percent> "
        "[ramp percent] [priority]",
        cmd_lighting_output_ramp),
    SHELL_COND_CMD(
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
        "<warn|warn_off|warn_relinquish>",
        NULL),
    SHELL_COND_CMD(
        CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT,
        override,
        NULL,
        "<instance> <clear|<level percent> [momentary]>",
        cmd_lighting_output_override),
    SHELL_COND_CMD(
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
