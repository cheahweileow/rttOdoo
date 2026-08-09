/*
 * Copyright (c) 2024-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2025-05-20     Roo           first version (Titan RA8P1 board)
 */

#ifndef WIFI_CONFIG_H_
#define WIFI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_pin_cfg.h"

/* RW007 WiFi module pin definitions for Titan RA8P1 board */
#define RA_RW007_RST_PIN        (BSP_IO_PORT_07_PIN_08)  /* P708 - Reset */
#define RA_RW007_CS_PIN         (BSP_IO_PORT_07_PIN_05)  /* P705 - SPI Chip Select */
#define RA_RW007_INT_BUSY_PIN   (BSP_IO_PORT_04_PIN_07)  /* P407 - Interrupt/Busy */
#define RA_RW007_SPI_BUS_NAME   "sci9"                    /* SCI9 in SPI mode */

/* WiFi credentials */
#define WIFI_SSID               "Castcoding"
#define WIFI_PASSWORD           "0707697768"

/* HTTP streaming server port */
#define STREAM_SERVER_PORT      8080

#ifdef __cplusplus
}
#endif

#endif /* WIFI_CONFIG_H_ */
