/**
 * @file
 * @brief Datalink for BACnet MS/TP
 * @author Steve Karg
 * @date August 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
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

/* MS/TP port */
static struct mstp_port_struct_t MSTP_Port;

/** Initialize the driver hardware */
static void rs485_init(void)
{

}

/** Prepare & transmit a packet. */
void rs485_send(const uint8_t *payload, uint16_t payload_len)
{

}

/** Check if one received byte is available */
bool rs485_read(uint8_t *buf)
{
    return false;
}

/** true if the driver is transmitting */
bool rs485_transmitting(void)
{
    return false;
}

/** Get the current baud rate */
uint32_t rs485_baud_rate(void)
{
    return 0;
}

/** Set the current baud rate */
bool rs485_baud_rate_set(uint32_t baud)
{
    return false;
}

/** Get the current silence time */
uint32_t rs485_silence_milliseconds(void)
{
    return 0;
}

/** Reset the silence time */
void rs485_silence_reset(void);
{

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
void mstp_init_uuid(const uint8_t *new_uuid, size_t length);
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
