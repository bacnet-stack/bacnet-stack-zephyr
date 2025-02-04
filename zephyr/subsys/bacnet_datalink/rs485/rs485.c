/**
 * @file
 * @brief RS485 Driver interface for MS/TP
 * @author Steve Karg
 * @date January 2025
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#if !defined(CONFIG_UART_ASYNC_API)
#error "RS485 driver requires CONFIG_UART_ASYNC_API to be defined!"
#endif

#if !defined(CONFIG_RING_BUFFER)
#error "RS485 driver requires CONFIG_RING_BUFFER to be defined!"
#endif

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <bacnet_datalink/rs485.h>
/* Logging module registration is already done */
#include "bacnet_osif/bacnet_log.h"
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

static void uart_isr_cb(const struct device *dev, void *user_data)
{
	size_t len;
	uint8_t buf[64];
	__ASSERT_NO_MSG(user_data != NULL);
	struct bacnet_driver_rs485 *context = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			len = uart_fifo_read(dev, buf, sizeof(buf));
			if (len > 0) {
				if (!context->transmitting) {
					ring_buf_put(&context->rb_rx, buf, len);
					context->received++;
					context->silence_timer = k_uptime_get();
				}
			}
		}
		if (uart_irq_tx_ready(dev)) {
      		if (!context->transmitting) {
     			uart_irq_tx_disable(dev);
			}
			len = ring_buf_get(&context->rb_tx, buf, 1);
			if (!len) {
				uart_irq_tx_disable(dev);
				context->silence_timer = k_uptime_get();
				context->transmitting = false;
			} else {
				uart_fifo_fill(dev, buf, 1);
				context->transmitted++;
			}
		}
	}
}

/*
 * API functions
 */

int32_t bacnet_driver_rs485_config_get(struct bacnet_driver_rs485 *context,
				       struct bacnet_driver_rs485_config *cfg)
{
	int32_t result = -EINVAL;

	if ((context != NULL) && (cfg != NULL)) {
		memmove(cfg, &context->config, sizeof(context->config));
		result = 0;
	}

	return result;
}

int64_t
bacnet_driver_rs485_silence_milliseconds(struct bacnet_driver_rs485 *context)
{
	int64_t delta = 0;

	if (context) {
		delta = k_uptime_delta(&context->silence_timer);
	}

	return delta;
}

void bacnet_driver_rs485_silence_reset(struct bacnet_driver_rs485 *context)
{
	if (context) {
		context->silence_timer = k_uptime_get();
	}
}

bool bacnet_driver_rs485_transmitting(struct bacnet_driver_rs485 *context)
{
	bool transmitting = false;

	if (context) {
		transmitting = context->transmitting;
	}

	return transmitting;
}

uint32_t bacnet_driver_rs485_transmitted(struct bacnet_driver_rs485 *context)
{
	uint32_t transmitted = 0;

	if (context) {
		transmitted = context->transmitted;
	}

	return transmitted;
}

uint32_t bacnet_driver_rs485_received(struct bacnet_driver_rs485 *context)
{
	uint32_t received = 0;

	if (context) {
		received = context->received;
	}

	return received;
}

bool bacnet_driver_rs485_byte_available(struct bacnet_driver_rs485 *context,
					uint8_t *data_register)
{
	bool available = false;

	if (!ring_buf_is_empty(&context->rb_rx)) {
		if (data_register) {
			(void)ring_buf_get(&context->rb_rx, data_register, 1);
		}
		available = true;
	}

	return available;
}

int32_t bacnet_driver_rs485_transmit(struct bacnet_driver_rs485 *context,
				     const uint8_t *buffer, uint32_t nbytes)
{
	int32_t result = -EINVAL;

	if ((context != NULL) && (context->enabled)) {
		if (nbytes) {
			context->transmitting = true;
			(void)ring_buf_put(&context->rb_tx, buffer, nbytes);
			uart_irq_tx_enable(context->uart_dev);
			context->silence_timer = k_uptime_get();
			result = 0;
		}
	}

	return result;
}

int32_t bacnet_driver_rs485_transmit_cancel(struct bacnet_driver_rs485 *context)
{
	int32_t result = -EINVAL;

	if (context) {
		context->transmitting = false;
		ring_buf_reset(&context->rb_tx);
		context->silence_timer = k_uptime_get();
		result = 0;
	}

	return result;
}

int32_t bacnet_driver_rs485_disable(struct bacnet_driver_rs485 *context)
{
	int32_t result = -EINVAL;

	if (context != NULL) {
		result = 0;
		uart_irq_rx_disable(context->uart_dev);
		uart_irq_tx_disable(context->uart_dev);
		/* Reject further input requests */
		context->enabled = false;
		/* Clean up internal resources */
		context->transmitting = false;
		context->silence_timer = 0;
		context->transmitted = 0;
		context->received = 0;
		context->enabled = true;

		/*TODO: cancel UART actions? unregister UART callback? */
	}

	return result;
}

int32_t bacnet_driver_rs485_configure(struct bacnet_driver_rs485 *context)
{
	int32_t result = -EINVAL;
	struct uart_config uart_config = {
		.baudrate = context->config.uart_baud,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	result = uart_configure(context->uart_dev, &uart_config);
	if (result != 0) {
		LOG_ERR("UART %s failed to configure (%d)", context->iface_name,
			result);
	}

	return result;
}

int32_t bacnet_driver_rs485_enable(struct bacnet_driver_rs485 *context)
{
	int32_t result = -EINVAL;
	uint8_t c;

	if ((context != NULL) && (!context->enabled)) {
		ring_buf_init(&context->rb_rx, sizeof(context->rb_rx_buffer),
			      context->rb_rx_buffer);
		context->transmitting = false;
		context->silence_timer = k_uptime_get();
		context->transmitted = 0;
		context->received = 0;
		context->enabled = true;

		result = bacnet_driver_rs485_configure(context);
		if (result != 0) {
			goto cleanup;
		}
		LOG_INF("UART %s configured baud rate (%lu)",
			context->iface_name,
			(unsigned long)context->config.uart_baud);

		uart_irq_rx_disable(context->uart_dev);
		uart_irq_tx_disable(context->uart_dev);
		result = uart_irq_callback_user_data_set(context->uart_dev, uart_isr_cb, context);
		if (result != 0) {
			LOG_ERR("UART %s failed set callback (%d)",
				context->iface_name, result);
			goto cleanup;
		}
		/* flush UART fifo */
		while (uart_irq_rx_ready(context->uart_dev)) {
			uart_fifo_read(context->uart_dev, &c, 1);
		}
        /* Both TX and RX are interrupt driven */
		uart_irq_rx_enable(context->uart_dev);
	}

cleanup:
	if (result != 0) {
		(void)bacnet_driver_rs485_disable(context);
	}

	return result;
}
