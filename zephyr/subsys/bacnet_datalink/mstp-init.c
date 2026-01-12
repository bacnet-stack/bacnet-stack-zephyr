/**
 * @file
 * @brief Datalink for BACnet MS/TP
 * @author Steve Karg
 * @date August 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <bacnet_datalink/rs485.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
/* me! */
#include "bacnet_datalink/mstp_init.h"

/* note: Logging module registration is done elsewhere */
#include "bacnet_osif/bacnet_log.h"
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

static struct bacnet_driver_rs485 rs485_context;
/* MS/TP port */
static struct mstp_port_struct_t MSTP_Port;
/* MS/TP Thread */
static struct k_thread mstp_thread_data;
K_THREAD_STACK_DEFINE(
    mstp_thread_stack, CONFIG_BACNET_DATALINK_MSTP_STACK_SIZE);

/* gather the RS485 UART peripherals */
struct rs485_instance {
    const struct device *dev;
};
#define RS485_COMPAT zephyr_modbus_serial
#define RS485_DEV_INFO(node_id) { .dev = DEVICE_DT_GET(node_id) },
static struct rs485_instance rs485_instances[] = { DT_FOREACH_STATUS_OKAY(
    RS485_COMPAT, RS485_DEV_INFO) };

#define RS485_COUNT ARRAY_SIZE(rs485_instances)

/** Get the RS485 device instance by index */
const struct device *rs485_device(uint8_t rs485_index)
{
    if (rs485_index < RS485_COUNT) {
        if (!device_is_ready(rs485_instances[rs485_index].dev)) {
            return NULL;
        }
        return rs485_instances[rs485_index].dev;
    }

    return NULL;
}

/** Initialize the driver hardware */
static void rs485_init(void)
{
}

/** Prepare & transmit a packet. */
void rs485_send(const uint8_t *payload, uint16_t payload_len)
{
    int32_t result;

    result = bacnet_driver_rs485_transmit(&rs485_context, payload, payload_len);
    if (result < 0) {
        LOG_ERR("Failed to transmit RS485 packet: result=%d", result);
    }
}

/** Check if one received byte is available */
bool rs485_read(uint8_t *data_register)
{
    return bacnet_driver_rs485_byte_available(&rs485_context, data_register);
}

/** true if the driver is transmitting */
bool rs485_transmitting(void)
{
    return bacnet_driver_rs485_transmitting(&rs485_context);
}

/** Get the current baud rate */
uint32_t rs485_baud_rate(void)
{
    return rs485_context.config.uart_baud;
}

/** Set the current baud rate */
bool rs485_baud_rate_set(uint32_t baud)
{
    int32_t result;

    rs485_context.config.uart_baud = baud;
    result = bacnet_driver_rs485_configure(&rs485_context);
    if (result < 0) {
        LOG_ERR("Failed to set RS485 baud to %lu: result=%d",
            (unsigned long)baud, result);
    }

    return result == 0;
}

/** Get the current silence time */
uint32_t rs485_silence_milliseconds(void)
{
    return bacnet_driver_rs485_silence_milliseconds(&rs485_context);
}

/** Reset the silence time */
void rs485_silence_reset(void)
{
    bacnet_driver_rs485_silence_reset(&rs485_context);
}

static struct dlmstp_rs485_driver RS485_Driver = {
    .init = rs485_init,
    .send = rs485_send,
    .read = rs485_read,
    .transmitting = rs485_transmitting,
    .baud_rate = rs485_baud_rate,
    .baud_rate_set = rs485_baud_rate_set,
    .silence_milliseconds = rs485_silence_milliseconds,
    .silence_reset = rs485_silence_reset
};
static struct dlmstp_user_data_t MSTP_User_Data;
static uint8_t Input_Buffer[DLMSTP_MPDU_MAX];
static uint8_t Output_Buffer[DLMSTP_MPDU_MAX];

/**
 * @brief Initialize the MSTP port UUID used for Zero Config
 * @param new_uuid - UUID to be set
 * @param length - length of the UUID
 */
void mstp_init_uuid(const uint8_t *new_uuid, size_t length)
{
    if (new_uuid && length) {
        memcpy(MSTP_Port.UUID, new_uuid, length);
    }
}

/**
 * @brief Initialize the MSTP port MAC address
 * @param mac - MAC address of this node. Possible value include
 * 0..127 - Master Node, 128..254 - Slave Node, 255 - Zero Config
 */
void mstp_init_mac(uint8_t mac)
{
    if (mac == 255) {
        MSTP_Port.ZeroConfigEnabled = true;
        MSTP_Port.Zero_Config_Preferred_Station = 255;
        MSTP_Port.This_Station = 255;
        MSTP_Port.SlaveNodeEnabled = false;
    } else if (mac <= 127) {
        MSTP_Port.ZeroConfigEnabled = false;
        MSTP_Port.Zero_Config_Preferred_Station = 255;
        MSTP_Port.This_Station = mac;
        MSTP_Port.SlaveNodeEnabled = false;
    } else {
        MSTP_Port.ZeroConfigEnabled = false;
        MSTP_Port.Zero_Config_Preferred_Station = 255;
        MSTP_Port.This_Station = mac;
        MSTP_Port.SlaveNodeEnabled = true;
    }
}

/**
 * @brief Initialize the MSTP port baud rate
 * @param baud - Baud rate for the RS-485 port
 */
void mstp_init_baud(uint32_t baud)
{
    dlmstp_set_baud_rate(baud);
}

/**
 * @brief Initialize the MSTP port max-master configuration value 0..127
 * @param max_master - value to be set (default=127)
 */
void mstp_init_max_master(uint8_t max_master)
{
    dlmstp_set_max_master(max_master);
}

/**
 * @brief Initialize the MSTP port
 * @param mac - MAC address of this node
 * @param baud - Baud rate for the RS-485 port
 * @param max_master - Maximum master address
 */
void mstp_init_port(uint8_t mac, uint32_t baud, uint8_t max_master)
{
    int32_t result;

    rs485_context.uart_dev = rs485_device(0);
    rs485_context.iface_name = "RS485";
    rs485_context.config.uart_baud = baud;
    result = bacnet_driver_rs485_enable(&rs485_context);
    if (result < 0) {
        LOG_ERR("Failed to enable RS485 driver: result=%d", result);
    }
    /* initialize MSTP datalink layer */
    MSTP_Port.Nmax_info_frames = DLMSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master = max_master;
    MSTP_Port.InputBuffer = Input_Buffer;
    MSTP_Port.InputBufferSize = sizeof(Input_Buffer);
    MSTP_Port.OutputBuffer = Output_Buffer;
    mstp_init_mac(mac);
    /* user data */
    MSTP_User_Data.RS485_Driver = &RS485_Driver;
    MSTP_Port.UserData = &MSTP_User_Data;
    dlmstp_init((char *)&MSTP_Port);
}

/**
 * @brief handles recurring strictly timed task
 * @brief timeout - number of milliseconds for datalink to wait for packet
 * @note called by ISR or RTOS every timeout milliseconds
 */
static void mstp_task(unsigned int timeout)
{
    struct dlmstp_packet *pkt = NULL;
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 };

    pdu_len = dlmstp_receive(
        &src, &Receive_Buffer[0], sizeof(Receive_Buffer), timeout);
    if (pdu_len) {
        pkt = (void *)Ringbuf_Data_Peek(&Receive_PDU_Queue);
        if (pkt) {
            memcpy(pkt->pdu, Receive_Buffer, MAX_MPDU);
            bacnet_address_copy(&pkt->address, &src);
            pkt->pdu_len = pdu_len;
            if (Ringbuf_Data_Put(&Receive_PDU_Queue, (volatile uint8_t *)pkt)) {
                xSemaphoreGive(BACnet_PDU_Available);
            }
        }
    }
}

/**
 * @brief BACnet MS/TP Thread
 */
static void mstp_thread(void)
{
	LOG_INF("MS/TP: started");
    for (;;) {
        mstp_task(1);
        k_sleep(K_MSEC(1));
    }
}

static int mstp_init(struct device *dev)
{
    ARG_UNUSED(dev);

    k_thread_create(
        &mstp_thread_data, mstp_thread_stack,
        K_THREAD_STACK_SIZEOF(mstp_thread_stack),
        (k_thread_entry_t)mstp_thread, NULL, NULL, NULL,
        K_PRIO_PREEMPT(CONFIG_BACNET_DATALINK_MSTP_PRIO), 0, K_NO_WAIT);
    k_thread_name_set(&mstp_thread_data, "MS/TP");
    return 0;
}

SYS_INIT(
    mstp_init, APPLICATION, CONFIG_BACNET_DATALINK_MSTP_APP_PRIORITY);
