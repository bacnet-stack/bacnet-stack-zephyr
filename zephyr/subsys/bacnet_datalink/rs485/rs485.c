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

/**
 * @file
 * @brief RS485 Driver interface for MS/TP
 */
static void uart_rx_rdy_cb(struct bacnet_driver_rs485 *context, uint8_t rx_byte)
{
    uint32_t length;
    int32_t result;
    uint8_t tx_byte;
    const uint32_t tx_timeout = 0;

    if (context->transmitting) {
        if (ring_buf_is_empty(&context->rb_tx)) {
            context->transmitting = false;
        } else {
            context->transmitted++;
            tx_byte = ring_buf_peek(&context->rb_tx, &tx_byte, sizeof(tx_byte));
            context->buffer_tx[0] = tx_byte;
            result =
                uart_tx(context->uart_dev, context->buffer_tx, 1, tx_timeout);
            if (result == 0) {
                context->silence_timer = k_uptime_get();
                context->transmitted++;
            } else {
                LOG_DBG("%s failed to transmit!", context->iface_name);
            }
        }
    } else {
        context->received++;
        ring_buf_put(&context->rb_rx, &rx_byte, sizeof(rx_byte));
        context->silence_timer = k_uptime_get();
    }
}

static void uart_cb(const struct device *dev, struct uart_event *evt,
                    void *user_data)
{
    __ASSERT_NO_MSG(evt != NULL);
    __ASSERT_NO_MSG(user_data != NULL);
    struct bacnet_driver_rs485 *context = user_data;
    uint8_t *p;
    int len;

    switch (evt->type) {
    case UART_TX_DONE:
        LOG_DBG("%s TX done", context->iface_name);
        break;
    case UART_TX_ABORTED:
        LOG_DBG("%s TX aborted", context->iface_name);
        break;
    case UART_RX_RDY:
        LOG_DBG("%s RX ready", context->iface_name);
        len = evt->data.rx.len;
        p = &evt->data.rx.buf[evt->data.rx.offset];
        for (int i = 0; i < len; i++) {
            uart_rx_rdy_cb(context, p[i]);
        }
        break;
    case UART_RX_BUF_REQUEST:
        LOG_DBG("%s RX buf request", context->iface_name);
        uart_rx_buf_rsp(dev, context->buffer_rx, sizeof(context->buffer_rx));
        break;
    case UART_RX_BUF_RELEASED:
        LOG_DBG("%s RX buf released", context->iface_name);
        /* not using a buffer that needs freed */
        break;
    case UART_RX_DISABLED:
        LOG_DBG("%s RX disabled", context->iface_name);
        break;
    case UART_RX_STOPPED:
        LOG_DBG("%s RX stopped", context->iface_name);
        break;
    default:
        LOG_ERR("%s unsupported event (%d)", context->iface_name, evt->type);
        return;
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
    const uint32_t tx_timeout = 0;

    if ((context != NULL) && (context->enabled)) {
        if (nbytes) {
            context->transmitting = true;
            ring_buf_reset(&context->rb_tx);
            ring_buf_put(&context->rb_tx, buffer, nbytes);
            /* send first byte */
            context->buffer_tx[0] = *buffer;
            result =
                uart_tx(context->uart_dev, context->buffer_tx, 1, tx_timeout);
            context->silence_timer = k_uptime_get();
            context->transmitted++;
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

int32_t bacnet_driver_rs485_enable(struct bacnet_driver_rs485 *context)
{
    int32_t result = -EINVAL;

    if ((context != NULL) && (!context->enabled)) {
        ring_buf_init(&context->rb_tx, sizeof(context->rb_tx_buffer),
                      context->rb_tx_buffer);
        ring_buf_init(&context->rb_rx, sizeof(context->rb_rx_buffer),
                      context->rb_rx_buffer);
        context->transmitting = false;
        context->silence_timer = k_uptime_get();
        context->transmitted = 0;
        context->received = 0;
        context->enabled = true;

        /* TODO: Configure pins for UART */
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
            goto cleanup;
        }
        result = uart_callback_set(context->uart_dev, uart_cb, context);
        if (result != 0) {
            LOG_ERR("UART %s failed set callback (%d)", context->iface_name,
                    result);
            goto cleanup;
        }
        result = uart_rx_enable(context->uart_dev, context->buffer_rx, 1,
                                SYS_FOREVER_US);
        if (result != 0) {
            LOG_ERR("UART %s failed enable rx (%d)", context->iface_name,
                    result);
        }
    }

cleanup:
    if (result != 0) {
        (void)bacnet_driver_rs485_disable(context);
    }

    return result;
}
