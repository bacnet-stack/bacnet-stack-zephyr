/**
 * @file
 * @brief The Zephyr RTOS interface for ReinitializeDevice services
 * @details If the request is valid and 'Reinitialized State of Device'
 *  is WARMSTART or COLDSTART, then the responding BACnet-user shall
 *  immediately proceed to perform any applicable shut-down procedures
 *  prior to reinitializing the device as specified by the requesting
 *  BACnet-user in the request.
 *
 *  WARMSTART shall mean to reboot the device and start over,
 *  retaining all data and programs that would normally be
 *  retained during a brief power outage.
 *  The precise interpretation of COLDSTART shall be defined by the vendor.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <stdint.h>
#include <stdlib.h>
#include <bacnet_osif/bacnet_reinit.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack core API */
#include "bacnet/version.h"
#include "bacnet/basic/sys/mstimer.h"
/* BACnet Stack basic device API -
   see bacnet/basic/server/bacnet_device.c for details */
#include "bacnet/basic/object/device.h"
/* Logging module registration is already done in ports/zephyr/main.c */
#include <bacnet_osif/bacnet_log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

/* timer for ReinitializeDevice service */
static struct mstimer Reinitialize_Timer;

/**
 * @brief Process a ReinitializeDevice request and trigger warm/cold restart.
 * @param coldstart_callback Callback invoked before a cold start reboot.
 * @param context Context passed to the cold start callback.
 */
void bacnet_reinitialize_device_task(
    bacnet_reinitialize_device_coldstart_callback coldstart_callback,
    void *context)
{
    BACNET_REINITIALIZED_STATE state;

    state = Device_Reinitialized_State();
    switch (state) {
        case BACNET_REINIT_COLDSTART:
            if (mstimer_expired(&Reinitialize_Timer)) {
                /* disable the interval timer - one shot */
                mstimer_set(&Reinitialize_Timer, 0);
                LOG_INF("ReinitializeDevice COLDSTART requested. REBOOT.");
                if (coldstart_callback != NULL) {
                    coldstart_callback(context);
                }
#if defined(CONFIG_REBOOT)
                sys_reboot(SYS_REBOOT_COLD);
#else
                LOG_ERR("Reboot not supported on this platform");
#endif
            }
            break;
        case BACNET_REINIT_WARMSTART:
            if (mstimer_expired(&Reinitialize_Timer)) {
                /* disable the interval timer - one shot */
                mstimer_set(&Reinitialize_Timer, 0);
                LOG_INF("ReinitializeDevice WARMSTART requested. REBOOT.");
#if defined(CONFIG_REBOOT)
                sys_reboot(SYS_REBOOT_WARM);
#else
                LOG_ERR("Reboot not supported on this platform");
#endif
            }
            break;
        case BACNET_REINIT_IDLE:
            mstimer_reset(&Reinitialize_Timer);
            break;
        default:
            break;
    }
}

/**
 * @brief Initialize the ReinitializeDevice service timer.
 * @param timeout_ms The timeout in milliseconds for the ReinitializeDevice
 * service.
 * @note This function should be called during system initialization to set up
 * the timer.
 */
void bacnet_reinitialize_device_init(uint32_t timeout_ms)
{
    mstimer_set(&Reinitialize_Timer, timeout_ms);
}
