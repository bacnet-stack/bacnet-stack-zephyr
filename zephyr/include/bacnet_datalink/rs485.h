/**
 * @file
 * @brief The BACnet RS485 driver initialization functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2025
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_DRIVER_RS485_INIT_H
#define BACNET_DRIVER_RS485_INIT_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/uart.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/datalink/dlmstp.h"

/** @brief Config structure
 *
 *  Define the public structure used for configuring the driver.
 *  Configuration must be done while the driver is disabled.
 */
struct bacnet_driver_rs485_config {
	uint32_t uart_baud;
	const struct gpio_dt_spec rts;
};

/** @brief Context structure
 *
 *  Define the public structure used for operating the driver.
 */
struct bacnet_driver_rs485 {
    /* driver */
    const char *iface_name;
    struct bacnet_driver_rs485_config config;
    const struct device *uart_dev;
    /* flags for operation */
    bool enabled;
    bool transmitting;
    /* data stores for receiving */
    struct ring_buf rb_tx;
    uint8_t rb_tx_buffer[DLMSTP_MPDU_MAX];
    uint8_t buffer_rx[1];
    /* data stores for transmitting */
    struct ring_buf rb_rx;
    uint8_t rb_rx_buffer[DLMSTP_MPDU_MAX];
    uint8_t buffer_tx[1];
    /* timer for tracking line silence */
    int64_t silence_timer;
    /* counters for statistics */
    uint32_t transmitted;
    uint32_t collisions;
    uint32_t received;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @brief Get current driver configuration.
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *  @param cfg Reference to structure for copy of current configuration.
 *
 *  @return Zero upon success.
 *  @return -EINVAL if invalid parameter(s).
 *  @return -ENOSYS if not supported.
 */
int32_t bacnet_driver_rs485_config_get(struct bacnet_driver_rs485 *context,
                                     struct bacnet_driver_rs485_config *cfg);

/** @brief driver silence time in milliseconds
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return number of milliseconds that the driver has been silent
 */
int64_t
bacnet_driver_rs485_silence_milliseconds(struct bacnet_driver_rs485 *context);

/** @brief set the driver silence time to current time
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 */
void
bacnet_driver_rs485_silence_reset(struct bacnet_driver_rs485 *context);


/** @brief driver transmission collision state
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return true if the driver is transmitting and collided
 */
bool bacnet_driver_rs485_collision_detected(struct bacnet_driver_rs485 *context);

/** @brief driver transmission state
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return true if the driver is transmitting
 */
bool bacnet_driver_rs485_transmitting(struct bacnet_driver_rs485 *context);

/** @brief driver statistics - number of transmit collisions
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return number of transmit collisions
 */
uint32_t bacnet_driver_rs485_collisions(struct bacnet_driver_rs485 *context);

/** @brief driver statistics - number of bytes transmitted
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return number of bytes transmitted
 */
uint32_t bacnet_driver_rs485_transmitted(struct bacnet_driver_rs485 *context);

/** @brief driver statistics - number of bytes received
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return number of bytes received
 */
uint32_t bacnet_driver_rs485_received(struct bacnet_driver_rs485 *context);

/** @brief Get a byte from the receive buffer of the driver
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *  @param data_register place to store the byte, or
 *   NULL if checking to see if a byte is available
 *
 *  @return true if there is a byte available
 */
bool bacnet_driver_rs485_byte_available(struct bacnet_driver_rs485 *context,
    uint8_t *data_register);


/** @brief Start the Transmission of a buffer to the driver
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *  @param buffer The buffer of bytes to send
 *  @param nbytes The number of bytes in the buffer to send
 *
 *  @return Zero if succesfully enqueued and transmision started
 *  @return -EINVAL if invalid parameter(s).
 *  @return -ENOSYS if not supported.
 */
int32_t bacnet_driver_rs485_transmit(struct bacnet_driver_rs485 *context,
                                   const uint8_t *buffer, uint32_t nbytes);

/** @brief Cancel the transmission in progress on the driver
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return Zero if successful
 *  @return -EINVAL if invalid parameter(s).
 *  @return -ENOSYS if not supported.
 */
int32_t bacnet_driver_rs485_transmit_cancel(struct bacnet_driver_rs485 *context);

/** @brief Disable driver and return resources.
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return Zero if successful
 *  @return -EINVAL if invalid parameter(s).
 *  @return -ENOSYS if not supported.
 */
int32_t bacnet_driver_rs485_disable(struct bacnet_driver_rs485 *context);


/** @brief Configure driver to with current configuration
 *  @note Use this function to reconfigure the baud rate
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return Zero upon success.
 *  @return -EINVAL if invalid parameter(s).
 *  @return -ENOSYS if not supported.
 */
int32_t bacnet_driver_rs485_configure(struct bacnet_driver_rs485 *context);

/** @brief Enable driver to run with current configuration
 *
 *  @param bacnet_driver_rs485 The handle of the protocol processor.
 *
 *  @return Zero upon success.
 *  @return -EINVAL if invalid parameter(s).
 *  @return -ENOSYS if not supported.
 */
int32_t bacnet_driver_rs485_enable(struct bacnet_driver_rs485 *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
