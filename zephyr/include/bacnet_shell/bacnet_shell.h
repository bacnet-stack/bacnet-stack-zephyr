/**
 * @file
 * @brief BACnet shell registration helpers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_SHELL_BACNET_SHELL_H
#define BACNET_SHELL_BACNET_SHELL_H

#include <zephyr/shell/shell.h>
#include <bacnet_osif/bacnet_lock.h>

/**
 * @brief Define a shell handler that holds the BACnet task lock.
 * @param _handler Existing handler. It must already be defined.
 */
#define BACNET_SHELL_LOCK_HANDLER(_handler)               \
    static int _handler##_locked(                         \
        const struct shell *sh, size_t argc, char **argv) \
    {                                                     \
        int status;                                       \
                                                          \
        bacnet_lock();                                    \
        status = _handler(sh, argc, argv);                \
        bacnet_unlock();                                  \
        return status;                                    \
    }

/**
 * @brief Register a locked handler in a static subcommand set.
 * @note Invoke BACNET_SHELL_LOCK_HANDLER(_handler) first, at file scope.
 */
#define BACNET_SHELL_CMD(_syntax, _subcmd, _help, _handler) \
    SHELL_CMD(_syntax, _subcmd, _help, _handler##_locked)

/**
 * @brief Register a conditional locked handler in a static subcommand set.
 * @note Invoke BACNET_SHELL_LOCK_HANDLER(_handler) first, at file scope.
 */
#define BACNET_SHELL_COND_CMD(_flag, _syntax, _subcmd, _help, _handler) \
    SHELL_COND_CMD(_flag, _syntax, _subcmd, _help, _handler##_locked)

/**
 * @brief Add a BACnet shell command whose handler holds the task lock.
 */
#define BACNET_SHELL_CMD_ADD(                                \
    _parent, _syntax, _subcmd, _help, _handler, _mand, _opt) \
    BACNET_SHELL_LOCK_HANDLER(_handler)                      \
    SHELL_SUBCMD_ADD(                                        \
        _parent, _syntax, _subcmd, _help, _handler##_locked, _mand, _opt)

#endif /* BACNET_SHELL_BACNET_SHELL_H */
