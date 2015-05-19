/**
 * \file sdladrv_usb.h
 * \brief SDLADRV USB Interface
 *
 * Authors: Alex Feldman <alex@sangoma.com>
 *
 * Copyright (c) 2008-09, Sangoma Technologies
 * All rights reserved.
 *
 * * Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sangoma Technologies nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sangoma Technologies ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sangoma Technologies BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * ===============================================================================
 */

#if !defined(__SDLADRV_USB_H)
# define __SDLADRV_USB_H

/* Internal USB-FXO CPU registers */
#define SDLA_USB_CPU_REG_DEVICEID		0x00
#define SDLA_USB_DEVICEID			0x55

#define SDLA_USB_CPU_REG_HARDWAREVER		0x01

#define SDLA_USB_CPU_REG_FIRMWAREVER		0x02

#define SDLA_USB_CPU_REG_CTRL			0x03
#define SDLA_USB_CPU_BIT_CTRL_RESET		0x80
#define SDLA_USB_CPU_BIT_CTRL_FWUPDATE		0x40
#define SDLA_USB_CPU_BIT_CTRL_TS1_HWEC_EN	0x08
#define SDLA_USB_CPU_BIT_CTRL_TS0_HWEC_EN	0x04
#define SDLA_USB_CPU_BIT_CTRL_TS1_EVENT_EN	0x02
#define SDLA_USB_CPU_BIT_CTRL_TS0_EVENT_EN	0x01

#define SDLA_USB_CPU_REG_FIFO_STATUS		0x04
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS1_TX_UF	0x80
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS1_TX_OF	0x40
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS0_TX_UF	0x20
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS0_TX_OF	0x10
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS1_RX_UF	0x08
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS1_RX_OF	0x04
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS0_RX_UF	0x02
#define SDLA_USB_CPU_BIT_FIFO_STATUS_TS0_RX_OF	0x01

#define SDLA_USB_CPU_REG_UART_STATUS		0x05
#define SDLA_USB_CPU_BIT_UART_STATUS_LOST_SYNC	0x10
#define SDLA_USB_CPU_BIT_UART_STATUS_CMD_UNKNOWN	0x10
#define SDLA_USB_CPU_BIT_UART_STATUS_RX_UF	0x08
#define SDLA_USB_CPU_BIT_UART_STATUS_RX_OF	0x04
#define SDLA_USB_CPU_BIT_UART_STATUS_TX_UF	0x02
#define SDLA_USB_CPU_BIT_UART_STATUS_TX_OF	0x01

#define SDLA_USB_CPU_REG_HOSTIF_STATUS		0x06
#define SDLA_USB_CPU_BIT_HOSTIF_STATUS_RX_UF	0x08
#define SDLA_USB_CPU_BIT_HOSTIF_STATUS_RX_OF	0x04
#define SDLA_USB_CPU_BIT_HOSTIF_STATUS_TX_UF	0x02
#define SDLA_USB_CPU_BIT_HOSTIF_STATUS_TX_OF	0x01

#define SDLA_USB_CPU_REG_LED_CONTROL		0x07
#define SDLA_USB_CPU_BIT_LED_CONTROL_TS1_GRN	0x08
#define SDLA_USB_CPU_BIT_LED_CONTROL_TS1_RED	0x04
#define SDLA_USB_CPU_BIT_LED_CONTROL_TS0_GRN	0x02
#define SDLA_USB_CPU_BIT_LED_CONTROL_TS0_RED	0x01

#define SDLA_USB_CPU_REG_DEBUG			0x08
#define SDLA_USB_CPU_BIT_DEBUG_WEN_ACK		0x08
#define SDLA_USB_CPU_BIT_DEBUG_DTMF		0x04
#define SDLA_USB_CPU_BIT_DEBUG_LOCAL_LB		0x02
#define SDLA_USB_CPU_BIT_DEBUG_LINE_LB		0x01

#define SDLA_USB_CPU_REG_EC_NUM			0x09

#define SDLA_USB_CPU_REG_FWUPDATE_MAGIC		0x0A
#define SDLA_USB_CPU_BITS_FWUPDATE_MAGIC	0x5A


#if defined(WAN_KERNEL)

# include "sdladrv.h"

extern int sdla_usb_init(void);
extern int sdla_usb_exit(void);
extern int sdla_usb_setup(sdlahw_card_t*, int);
extern int sdla_usb_down(sdlahw_card_t*, int force);

/*usb interface */
extern int	sdla_usb_cpu_read(void *phw, unsigned char off, unsigned char *data);
extern int	sdla_usb_cpu_write(void *phw, unsigned char off, unsigned char data);
extern int	sdla_usb_write_poll(void *phw, unsigned char off, unsigned char data);
extern int	sdla_usb_read_poll(void *phw, unsigned char off, unsigned char *data);
extern int	sdla_usb_hwec_enable(void *phw, int mod_no, int enable);
extern int	sdla_usb_rxevent_enable(void *phw, int mod_no, int enable);
extern int	sdla_usb_rxevent(void *phw, int mod_no, u8 *regs, int);
extern int	sdla_usb_rxtx_data_init(void *phw, int, unsigned char **, unsigned char **);
extern int	sdla_usb_rxdata_enable(void *phw, int enable);
extern int 	sdla_usb_fwupdate_enable(void *phw);
extern int	sdla_usb_txdata_raw(void *phw, unsigned char*, int);
extern int	sdla_usb_txdata_raw_ready(void *phw);
extern int	sdla_usb_rxdata_raw(void *phw, unsigned char*, int);
extern int	sdla_usb_set_intrhand(void*, wan_pci_ifunc_t*, void*, int);
extern int	sdla_usb_restore_intrhand(void*, int);
extern int 	sdla_usb_err_stats(void*,void*,int);
extern int 	sdla_usb_flush_err_stats(void*);

#endif /* WAN_KERNEL */

#endif	/* __SDLADRV_USB_H */
