/******************************************************************************
 * sdla_gsm.h	
 *
 * Author: 	Moises Silva <moy@sangoma.com>
 *
 * Copyright:	(c) 2011 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Oct 06, 2011 Moises Silva   Initial Version
 ******************************************************************************
 */

#ifndef __SDLA_GSM_H
#define __SDLA_GSM_H

#if (defined __WINDOWS__)
# define inline __inline
#endif
/* W400 has 4 GSM modules. Each module has one voice channel and one fake D-channel */
#define MAX_GSM_MODULES 4

/* All modules masked (4 modules, 1111) */
#define AFT_GSM_ALL_MODULES_MASK 0x000F

/* Assumed the last channel is the HDLC channel */
#define MAX_GSM_CHANNELS 2
#define MAX_GSM_TIMESLOTS 32

/* where in the span channel array indexes we have voice and data */
#define GSM_VOICE_CHANNEL 0
#define GSM_UART_CHANNEL 1

/* 31th chan (zero-based) will have the dchan */
#define GSM_DCHAN_LOGIC_CHAN 31

#define IS_GSM_CARD(card) IS_GSM_FEMEDIA(&(card)->fe)

/* Global register */
#define AFT_GSM_GLOBAL_REG 0x1400

/* Starting configuration register (incremented by AFT_GSM_REG_OFFSET for each module) */
#define AFT_GSM_CONFIG_REG 0x1200

/* UART tx/rx data and status registers (incremented by AFT_GSM_REG_OFFSET for each module) */
#define AFT_GSM_UART_TX_REG 0x1300
#define AFT_GSM_UART_RX_REG 0x1310
#define AFT_GSM_UART_STAT_REG 0x1320

/* Per module register offset */
#define AFT_GSM_REG_OFFSET 0x0004

/* Our FIFO size for the UART */
#define AFT_GSM_UART_TX_FIFO_SIZE 15
#define AFT_GSM_UART_RX_FIFO_SIZE 66

/* UART status */
#define AFT_GSM_UART_TX_FIFO_DATA_COUNT_MASK 0x0F
#define AFT_GSM_UART_RX_FIFO_DATA_COUNT_MASK 0x7F
#define AFT_GSM_UART_RX_FIFO_DATA_COUNT_OFFSET 8
#define AFT_GSM_UART_TX_FIFO_OVERFLOW_BIT 16
#define AFT_GSM_UART_RX_FIFO_OVERFLOW_BIT 17

/* PLL status bits in the global register */
#define AFT_GSM_PLL_RESET_BIT 0
#define AFT_GSM_PLL_PHASE_SHIFT_OVERFLOW_BIT 1
#define AFT_GSM_PLL_INPUT_CLOCK_LOST_BIT 2
#define AFT_GSM_PLL_OUTPUT_CLOCK_STOPPED_BIT 3
#define AFT_GSM_PLL_LOCKED_BIT 4

/* SIM muxing error in the global register */
#define AFT_GSM_SIM_MUXING_ERROR_BIT 5

/* PCM audio loop, all audio going towards the Digital Voiceband Interface (DVI) will be looped back,
 * this is only useful for debugging purposes */
#define AFT_GSM_GLOBAL_PCM_LOOPBACK_BIT 30

/* Shutdown 3.8v for all modules (active 0) in the global register */
#define AFT_GSM_GLOBAL_SHUTDOWN_BIT 31

/*!< Retrieve the register address for a module */
#define AFT_GSM_MOD_REG(mod_no, reg) (reg + ((mod_no-1) * AFT_GSM_REG_OFFSET))

/*!< GSM module configuration (bit numbers are zero-based) */
#define AFT_GSM_MOD_POWER_BIT 0x0
#define AFT_GSM_MOD_RESET_BIT 0x1
#define AFT_GSM_MOD_POWER_MONITOR_BIT 0x2
#define AFT_GSM_MOD_TX_MONITOR_BIT 0x3
#define AFT_GSM_MOD_SIM_INSERTED_BIT 0x4

/*!< The high order last 3 bits of first byte of the module configuration register select the SIM */
#define AFT_GSM_MOD_SIM_BIT_OFFSET 5
#define AFT_GSM_MOD_SIM_MASK (0x7 << AFT_GSM_MOD_SIM_BIT_OFFSET)

/*!< The first 3 bits of the second byte select the UART baud rate */
#define AFT_GSM_MOD_UART_BAUD_RATE_BIT_OFFSET 0x8
#define AFT_GSM_MOD_UART_BAUD_RATE_MASK (0x7 << AFT_GSM_MOD_UART_BAUD_RATE_BIT_OFFSET)

/*!< The fourth bit of the second byte report if there is service or not */
#define AFT_GSM_MOD_SERVICE_BIT 0xB

/*!< The fifth bit of the second byte report if there is a module (cell phone) present or not */
#define AFT_GSM_MOD_PRESENT_BIT 0xC

/*! Absolute timeout to power on/off a module 
 *  Telit data sheet suggests 2 seconds, in real life for W400
 *  I've seen power off take more than that, 3-4 seconds, so
 *  I decided to boost this to 10 seconds to be ultra-safe
 *  The only down-side of this will be if we ever need to
 *  use the "slow hwprobe" mechanism that attempts to power on/off
 *  the modules the first time hwprobe is run, because a timeout
 *  of 10 seconds will slow down a lot the first hwprobe in cases where
 *  the module is not present (see sdladrv.c slow hw probe define)
 *  since currently we do not use the "slow hwprobe" mode and rely
 *  instead on the GSM hw FPGA register to tell us if the module
 *  is present or not, we should be fine ...
 *  */
#define AFT_GSM_MODULE_POWER_TOGGLE_TIMEOUT_MS 10000

/*!< How often to check if the module is already on/off during startup/shutdown */
#define AFT_GSM_MODULE_POWER_TOGGLE_CHECK_INTERVAL_MS 10

/*! Absolute timeout to enable PLL */
#define AFT_GSM_PLL_ENABLE_TIMEOUT_MS 2000

/*!< How often to check if the PLL is already enabled on startup */
#define AFT_GSM_PLL_ENABLE_CHECK_INTERVAL_MS 10

/*! How much to wait between power registry writes 
 *  Telit data sheet says input command for switch off/on must be equal or bigger to 1 second,
 *  we give it 100ms more in case the host scheduling is somewhat faster */
#define AFT_GSM_MODULE_POWER_TOGGLE_DELAY_MS 1100

/* Transform from module index to bit index in bit map */
#define AFT_GSM_MOD_BIT(mod_no) (mod_no - 1)

static inline int aft_gsm_get_sim_no(int reg)
{
	reg &= AFT_GSM_MOD_SIM_MASK;
	return (reg >> AFT_GSM_MOD_SIM_BIT_OFFSET);
}

static inline void aft_gsm_set_sim_no(int *reg, int sim_no)
{
	sim_no <<= AFT_GSM_MOD_SIM_BIT_OFFSET;
	*reg |= (sim_no & AFT_GSM_MOD_SIM_MASK);
}

static inline int aft_gsm_get_uart_baud_rate(int reg)
{
	reg &= AFT_GSM_MOD_UART_BAUD_RATE_MASK;
	return (reg >> AFT_GSM_MOD_UART_BAUD_RATE_BIT_OFFSET);
}

int wp_gsm_iface_init(void*, void*);
int wp_gsm_uart_rx_fifo(void *card, unsigned char *rx_buff, int reqlen);
int wp_gsm_uart_check_tx_fifo(void *card);
int wp_gsm_uart_tx_fifo(void *card, unsigned char *tx_buff, int len);
int wp_gsm_pll_reset(void *card);
int wp_gsm_update_sim_status(void *card);

typedef struct wan_gsm_udp {
	union {
		char uart_debug;
	} u;
} wan_gsm_udp_t;

/* used by gsm front end code to store GSM-specific data */
#define GSM_MAX_UART_FRAME_LEN 300
typedef struct sdla_gsm_param {
	unsigned char uart_txbuf[GSM_MAX_UART_FRAME_LEN];
	int uart_txbuf_len;
	int uart_tx_cnt;
} sdla_gsm_param_t;

/* Front-End UDP command */
#define WAN_GSM_REGDUMP     (WAN_FE_UDP_CMD_START + 0)
#define WAN_GSM_UART_DEBUG  (WAN_FE_UDP_CMD_START + 1)
#define WAN_GSM_AUDIO_DEBUG (WAN_FE_UDP_CMD_START + 2)
#define WAN_GSM_PLL_RESET   (WAN_FE_UDP_CMD_START + 3)
#define WAN_GSM_POWER_TOGGLE (WAN_FE_UDP_CMD_START + 4)
#define WAN_GSM_UPDATE_SIM_STATUS (WAN_FE_UDP_CMD_START + 5)
#define WAN_GSM_PCM_LOOPBACK (WAN_FE_UDP_CMD_START + 6)

#define AFT_GSM_UART_DEBUG_ENABLED_BIT 1
#define AFT_GSM_AUDIO_DEBUG_TOGGLE_BIT 2

#endif	/* __SDLA_GSM_H */

