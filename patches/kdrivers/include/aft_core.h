/*****************************************************************************
* aft_core.h
*	
*		WANPIPE(tm) AFT CORE Hardware Support
* 		
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 07, 2003	Nenad Corbic	Initial version.
*****************************************************************************/


#ifndef _SDLA_AFT_TE1_H
#define _SDLA_AFT_TE1_H

#include "wanpipe_api.h"
#include "aft_core_user.h"

#ifdef WAN_KERNEL

#include "aft_core_private.h"
#include "aft_core_utils.h"
#include "aft_core_prot.h"

#include "aft_a104.h"
#include "aft_t116.h"

#if defined(CONFIG_WANPIPE_PRODUCT_AFT_RM)
# include "aft_analog.h"
#endif

#if defined(CONFIG_WANPIPE_PRODUCT_AFT_BRI)
# include "aft_bri.h"
#endif

#include "aft_a104.h"
#include "aft_analog.h"
#include "aft_bri.h"
#include "aft_gsm.h"
#include "aft_t116.h"


#if defined(__WINDOWS__)
# include <sdladrv_private.h>
# define AFT_MAX_CHIP_SECURITY_CNT 100
#endif

/*======================================================
 * GLOBAL  (All Ports)
 * 
 * AFT Chip PCI Registers
 *
 * Global Configuration Area for All Ports
 *
 *=====================================================*/

#define AFT_CHIP_CFG_REG		0x40

#define AFT_ANALOG_MCPU_IFACE_RW	0x44

#define AFT_WDT_CTRL_REG		0x48 
#define AFT_WDT_CTRL_REG2		0x64 
#define AFT_WDT_1TO4_CTRL_REG		AFT_WDT_CTRL_REG
#define AFT_WDT_1TO4_CTRL_REG2		AFT_WDT_CTRL_REG2

#define AFT_DATA_MUX_REG		0x4C 

#define AFT_FIFO_MARK_REG		0x50

#define AFT_MCPU_INTERFACE_RW		0x54

#define AFT_ANALOG_SPI_IFACE_RW		0x54

#define AFT_CHIP_STAT_REG		0x58
#define AFT_CHIP_STAT_REG2		0x60

#define AFT_WDT_4TO8_CTRL_REG		0x5C
#define AFT_WDT_4TO8_CTRL_REG2		0x68

#define AFT_FREE_RUN_TIMER_CTRL_REG		0x98

#define AFT_FREE_RUN_TIMER_PENDING_REG	0x9C


/*================================================= 
  A104 CHIP CFG REGISTERS  
*/

# define AFT_CHIPCFG_TE1_CFG_BIT	0
# define AFT_CHIPCFG_56K_CFG_BIT	0
# define AFT_CHIPCFG_ANALOG_CLOCK_SELECT_BIT 0
# define AFT_CHIPCFG_A500_NET_SYNC_CLOCK_SELECT_BIT 0
# define AFT_CHIPCFG_SFR_EX_BIT		1
# define AFT_CHIPCFG_SFR_IN_BIT		2
# define AFT_CHIPCFG_FE_INTR_CFG_BIT	3

# define AFT_CHIPCFG_A104D_EC_SEC_KEY_MASK	0x7
# define AFT_CHIPCFG_A104D_EC_SEC_KEY_SHIFT 	4

# define AFT_CHIPCFG_A200_EC_SEC_KEY_MASK	0x3
# define AFT_CHIPCFG_A200_EC_SEC_KEY_SHIFT  	16 

# define AFT_CHIPCFG_A600_EC_SEC_KEY_MASK       0x3
# define AFT_CHIPCFG_A600_EC_SEC_KEY_SHIFT      4

# define AFT_CHIPCFG_A700_EC_SEC_KEY_MASK		0x1
# define AFT_CHIPCFG_A700_EC_SEC_KEY_SHIFT 		4

# define AFT_CHIPCFG_SPI_SLOW_BIT	5	/* Slow down SPI */
#if 0
# define AFT_CHIPCFG_EC_INTR_CFG_BIT	4	/* Shark or Analog */
# define AFT_CHIPCFG_SECURITY_CFG_BIT	6
#endif

# define AFT_CHIPCFG_RAM_READY_BIT	7
# define AFT_CHIPCFG_HDLC_CTRL_RDY_BIT  8

# define AFT_CHIPCFG_ANALOG_INTR_MASK	0x0F		/* Analog */
# define AFT_CHIPCFG_ANALOG_INTR_SHIFT	9

# define AFT_CHIPCFG_A500_INTR_MASK	0x0F		/* A500 BRI - interrupt pending from upto 4 remoras.
							 *	bit 9 - remora 1
							 *	bit 10- remora 2
							 *	bit 11- remora 3
							 *	bit 12- remora 4
							 */
# define AFT_CHIPCFG_A500_INTR_SHIFT	9
# define A500_LINE_SYNC_MASTER_BIT	31


# define AFT_CHIPCFG_A108_EC_CLOCK_SRC_MASK	0x07	/* A108 */
# define AFT_CHIPCFG_A108_EC_CLOCK_SRC_SHIFT	9	

# define AFT_CHIPCFG_A104D_EC_SECURITY_BIT 12
# define AFT_CHIPCFG_A108_EC_INTR_ENABLE_BIT 12		/* A108 */


# define AFT_CHIPCFG_A500_EC_INTR_ENABLE_BIT 14		/* A500 - BRI not used for now */


# define AFT_CHIPCFG_EC_INTR_STAT_BIT						13
#	define AFT_CHIPCFG_B600_EC_RESET_BIT					25
# define AFT_CHIPCFG_B600_EC_CHIP_PRESENT_BIT		20              /* B600 and B601 only */

/* A104 A200 A108 Differ Here 
 * By default any register without device name is
 * common to all all.
 */

# define AFT_CHIPCFG_P1_TDMV_INTR_BIT	14	

# define AFT_CHIPCFG_P2_TDMV_INTR_BIT	15	
# define AFT_CHIPCFG_A104_TDM_ACK_BIT 15

# define AFT_CHIPCFG_A200_EC_SECURITY_BIT 15		/* Analog */
# define AFT_CHIPCFG_A108_EC_SECURITY_BIT 15		/* A108 */
# define AFT_CHIPCFG_A500_EC_SECURITY_BIT 15		/* A500/BRI */

# define AFT_CHIPCFG_P3_TDMV_INTR_BIT	16	
# define AFT_CHIPCFG_A108_A104_TDM_FIFO_SYNC_BIT	16 	/* A108 Global Fifo Sync Bit */
# define AFT_CHIPCFG_A700_ANALOG_TDM_FIFO_SYNC_BIT	18      /* A700 Analog Global Fifo Sync Bit */


# define AFT_CHIPCFG_P4_TDMV_INTR_BIT	17	
# define AFT_CHIPCFG_A108_A104_TDM_DMA_RINGBUF_BIT	17   	/* A108 Quad DMA Ring buf enable */

# define AFT_CHIPCFG_P1_WDT_INTR_BIT	18
# define AFT_CHIPCFG_P2_WDT_INTR_BIT	19
# define AFT_CHIPCFG_P3_WDT_INTR_BIT	20
# define AFT_CHIPCFG_P4_WDT_INTR_BIT	21

# define AFT_CHIPCFG_FREE_RUN_INTR_BIT	20		/* AFT FREE Run timer bit */

# define AFT_CHIPCFG_A108_EC_INTER_STAT_BIT	21	/* A108 */

# define AFT_CHIPCFG_FE_INTR_STAT_BIT	22
# define AFT_CHIPCFG_SECURITY_STAT_BIT	23


/* A104 & A104D IRQ Status Bits */

# define AFT_CHIPCFG_HDLC_INTR_MASK	0x0F
# define AFT_CHIPCFG_HDLC_INTR_SHIFT	24

# define AFT_CHIPCFG_DMA_INTR_MASK	0x0F
# define AFT_CHIPCFG_DMA_INTR_SHIFT	28

# define AFT_CHIPCFG_A108_TDM_GLOBAL_RX_INTR_ACK 30
# define AFT_CHIPCFG_A108_TDM_GLOBAL_TX_INTR_ACK 31

# define AFT_CHIPCFG_A700_TDM_ANALOG_RX_INTR_ACK 28
# define AFT_CHIPCFG_A700_TDM_ANALOG_TX_INTR_ACK 29

# define AFT_CHIPCFG_WDT_INTR_MASK	0x0F
# define AFT_CHIPCFG_WDT_INTR_SHIFT	18

# define AFT_CHIPCFG_TDMV_INTR_MASK	0x0F
# define AFT_CHIPCFG_TDMV_INTR_SHIFT	14

#  define AFT_CHIPCFG_WDT_FE_INTR_STAT	0
#  define AFT_CHIPCFG_WDT_TX_INTR_STAT  1
#  define AFT_CHIPCFG_WDT_RX_INTR_STAT	2


#define AFT_CLKCFG_A600_REG 0x1090
#define AFT_CLKCFG_A600_CLK_OUTPUT_BIT         0
#define AFT_CLKCFG_A600_CLK_EXT_CLK_SRC_BIT    4
#define AFT_CLKCFG_A600_CLK_SRC_BIT_MASK       0x6
#define AFT_CLKCFG_A600_CLK_SRC_BIT_SHIFT      1
#define AFT_CLKCFG_A600_CLK_OUT_BIT_MASK       0x7
#define AFT_CLKCFG_A600_CLK_OUT_BIT_SHIFT      5


/* Use the onboard oscillator clk 8.192 Mhz */
# define AFT_CLKCFG_A600_CLK_SRC_OSC		0x00	

/* Use the clock from front end no pll */
# define AFT_CLKCFG_A600_CLK_SRC_EXT_NO_PLL	0x01

/* Use the clock from front end with pll */
# define AFT_CLKCFG_A600_CLK_SRC_EXT_PLL	0x02

/* use the board system clock */
# define AFT_CLKCFG_A600_CLK_OUT_BOARD		0x04


#define AFT_CLKCFG_B601_EC_SRC_MUX_MASK					0x03
#define AFT_CLKCFG_B601_EC_SRC_MUX_SHIFT				1

#define AFT_CLKCFG_B601_EC_SRC_MUX_OSC_CLK				0x0
#define AFT_CLKCFG_B601_EC_SRC_MUX_EXT_CLK				0x1
#define AFT_CLKCFG_B601_EC_SRC_MUX_TRISTATE				0x2

#define AFT_CLKCFG_B601_EXT_CLK_VAL_MASK				0x1
#define AFT_CLKCFG_B601_EXT_CLK_VAL_SHIFT				4

#define AFT_CLKCFG_B601_EXT_CLK_MUX_MASK				0x7
#define AFT_CLKCFG_B601_EXT_CLK_MUX_SHIFT				5

#define AFT_CLKCFG_B601_EXT_CLK_MUX_LINE_0				0x00
#define AFT_CLKCFG_B601_EXT_CLK_MUX_LINE_1				0x01
#define AFT_CLKCFG_B601_EXT_CLK_MUX_OSC_2000HZ			0x02
#define AFT_CLKCFG_B601_EXT_CLK_MUX_OSC_1500HZ			0x03
#define AFT_CLKCFG_B601_EXT_CLK_MUX_H100_CLK			0x04

#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_MASK           0x7
#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_SHIFT          8

#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_LINE_0         0x00
#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_LINE_1         0x01
#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_OSC_2000HZ     0x02
#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_OSC_1500HZ     0x03
#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_EXT_8000HZ     0x04
#define AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_EXT_1500HZ     0x05





static __inline u32
aft_chipcfg_get_fifo_reset_bit(sdla_t* card)
{
	if (card->adptr_type == AFT_ADPTR_FLEXBRI &&
		card->wandev.comm_port == 1) {

		return AFT_CHIPCFG_A700_ANALOG_TDM_FIFO_SYNC_BIT;
	}

	return AFT_CHIPCFG_A108_A104_TDM_FIFO_SYNC_BIT;
}


static __inline u32
aft_chipcfg_get_rx_intr_ack_bit(sdla_t* card)
{
	if (card->adptr_type == AFT_ADPTR_FLEXBRI &&
		card->wandev.comm_port == 1) {

		return AFT_CHIPCFG_A700_TDM_ANALOG_RX_INTR_ACK;
	}

	return AFT_CHIPCFG_A108_TDM_GLOBAL_RX_INTR_ACK;
}

static __inline u32
aft_chipcfg_get_tx_intr_ack_bit(sdla_t* card)
{
	if (card->adptr_type == AFT_ADPTR_FLEXBRI &&
		card->wandev.comm_port == 1) {

		return AFT_CHIPCFG_A700_TDM_ANALOG_TX_INTR_ACK;
	}

	return AFT_CHIPCFG_A108_TDM_GLOBAL_TX_INTR_ACK;
}



static __inline u32
aft_chipcfg_get_rx_intr_ack_bit_bymap(u32 comm_port)
{
	if (comm_port == 1) {
		return AFT_CHIPCFG_A700_TDM_ANALOG_RX_INTR_ACK;
	}

	return AFT_CHIPCFG_A108_TDM_GLOBAL_RX_INTR_ACK;
}

static __inline u32
aft_chipcfg_get_tx_intr_ack_bit_bymap(u32 comm_port)
{
	if (comm_port == 1) {
		return AFT_CHIPCFG_A700_TDM_ANALOG_TX_INTR_ACK;
	}

	return AFT_CHIPCFG_A108_TDM_GLOBAL_TX_INTR_ACK;
}

/* A104 & A104D Interrupt Status Funcitons */

static __inline u32
aft_chipcfg_get_hdlc_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_HDLC_INTR_SHIFT;
	reg&=AFT_CHIPCFG_HDLC_INTR_MASK;
	return reg;
}

static __inline u32
aft_chipcfg_get_analog_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_ANALOG_INTR_SHIFT;
	reg&=AFT_CHIPCFG_ANALOG_INTR_MASK;
	return reg;
}

static __inline void
aft_chipcfg_set_oct_clk_src(u32 *reg, u32 src)
{ 
 	*reg&=~(AFT_CHIPCFG_A108_EC_CLOCK_SRC_MASK<<AFT_CHIPCFG_A108_EC_CLOCK_SRC_SHIFT);
	src&=AFT_CHIPCFG_A108_EC_CLOCK_SRC_MASK;
	*reg|=(src<<AFT_CHIPCFG_A108_EC_CLOCK_SRC_SHIFT);       
}


static __inline u32
aft_chipcfg_get_dma_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_DMA_INTR_SHIFT;
	reg&=AFT_CHIPCFG_DMA_INTR_MASK;
	return reg;
}

static __inline u32
aft_chipcfg_get_wdt_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_WDT_INTR_SHIFT;
	reg&=AFT_CHIPCFG_WDT_INTR_MASK;
	return reg;
}

static __inline u32
aft_chipcfg_get_tdmv_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_TDMV_INTR_SHIFT;
	reg&=AFT_CHIPCFG_TDMV_INTR_MASK;
	return reg;
}




/* A108 IRQ Status Bits on CHIP_STAT_REG */

# define AFT_CHIPCFG_A108_WDT_INTR_MASK		0xFF
# define AFT_CHIPCFG_A108_WDT_INTR_SHIFT	0

# define AFT_CHIPCFG_A108_TDMV_INTR_MASK	0xFF
# define AFT_CHIPCFG_A108_TDMV_INTR_SHIFT	8


# define AFT_CHIPCFG_A108_FIFO_INTR_MASK	0xFF
# define AFT_CHIPCFG_A108_FIFO_INTR_SHIFT	16

# define AFT_CHIPCFG_A108_DMA_INTR_MASK		0xFF
# define AFT_CHIPCFG_A108_DMA_INTR_SHIFT	24




/* A108 Interrupt Status Functions */

static __inline u32
aft_chipcfg_a108_get_fifo_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_A108_FIFO_INTR_SHIFT;
	reg&=AFT_CHIPCFG_A108_FIFO_INTR_MASK;
	return reg;
}

static __inline u32
aft_chipcfg_a108_get_dma_intr_stats(u32 reg)
{
	DEBUG_TEST("INSIDE aft_chipcfg_a108_get_dma_intr_stats, reg= 0x%X\n",reg);
	reg=reg>>AFT_CHIPCFG_A108_DMA_INTR_SHIFT;
	reg&=AFT_CHIPCFG_A108_DMA_INTR_MASK;
	return reg;
}

static __inline u32
aft_chipcfg_a108_get_wdt_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_A108_WDT_INTR_SHIFT;
	reg&=AFT_CHIPCFG_A108_WDT_INTR_MASK;
	return reg;
}

static __inline u32
aft_chipcfg_a108_get_tdmv_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_A108_TDMV_INTR_SHIFT;
	reg&=AFT_CHIPCFG_A108_TDMV_INTR_MASK;
	return reg;
}


/* AFT Serial specific bits */

# define AFT_CHIPCFG_SERIAL_WDT_INTR_MASK	0xF
# define AFT_CHIPCFG_SERIAL_WDT_INTR_SHIFT	0

# define AFT_CHIPCFG_SERIAL_STATUS_INTR_MASK	0xFFF
# define AFT_CHIPCFG_SERIAL_STATUS_INTR_SHIFT	4

# define AFT_CHIPCFG_SERIAL_CTS_STATUS_INTR_BIT 0
# define AFT_CHIPCFG_SERIAL_DCD_STATUS_INTR_BIT 1
# define AFT_CHIPCFG_SERIAL_RTS_STATUS_INTR_BIT 2

/* Serial specific functions */

static __inline u32
aft_chipcfg_serial_get_status_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_SERIAL_STATUS_INTR_SHIFT;
	reg&=AFT_CHIPCFG_SERIAL_STATUS_INTR_MASK;

	return reg;
}


static __inline u32
aft_chipcfg_serial_get_wdt_intr_stats(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_SERIAL_WDT_INTR_SHIFT;
	reg&=AFT_CHIPCFG_SERIAL_WDT_INTR_MASK;
	return reg;
}



/* 56k IRQ status bits */
# define AFT_CHIPCFG_A56K_WDT_INTR_BIT		0
# define AFT_CHIPCFG_A56K_DMA_INTR_BIT		24
# define AFT_CHIPCFG_A56K_FIFO_INTR_BIT		16

# define AFT_CHIPCFG_A56K_FE_MASK			0x7F
# define AFT_CHIPCFG_A56K_FE_SHIFT			9

static __inline u32
aft_chipcfg_a56k_read_fe(u32 reg)
{
	reg=reg>>AFT_CHIPCFG_A56K_FE_SHIFT;
	reg&=AFT_CHIPCFG_A56K_FE_MASK;
	return reg;
}

static __inline u32
aft_chipcfg_a56k_write_fe(u32 reg, u32 val)
{
	val&=AFT_CHIPCFG_A56K_FE_MASK;

	reg &= ~(AFT_CHIPCFG_A56K_FE_MASK<<AFT_CHIPCFG_A56K_FE_SHIFT);

	reg |= (val<<AFT_CHIPCFG_A56K_FE_SHIFT);

	return reg;
}


static __inline u32
aft_chipcfg_get_ec_channels(u32 reg)
{
	switch ((reg>>AFT_CHIPCFG_A104D_EC_SEC_KEY_SHIFT)&AFT_CHIPCFG_A104D_EC_SEC_KEY_MASK){
    
	case 0x00:
		return 0;
	case 0x01:
		return 32;
	case 0x02:
		return 64;
	case 0x03:
		return 96;
	case 0x04:
		return 128;
	case 0x05:
		return 256;
   	default:
		return 0;
	}
	
	return 0;
}


static __inline u32
aft_chipcfg_get_a200_ec_channels(u32 reg)
{
	switch ((reg>>AFT_CHIPCFG_A200_EC_SEC_KEY_SHIFT)&AFT_CHIPCFG_A200_EC_SEC_KEY_MASK){
    
	case 0x00:
		return 0;
	case 0x01:
		return 16;
	case 0x02:
		return 32;
   	default:
		return 0;
	}
	
	return 0;
}

static __inline u32
aft_chipcfg_get_a600_ec_channels(u32 reg, u8 core_rev)
{
	if (core_rev >= 3) {
		if (wan_test_bit(AFT_CHIPCFG_B600_EC_CHIP_PRESENT_BIT, &reg)) {
			return 5;
		}
		return 0;
	} else {
		switch ((reg>>AFT_CHIPCFG_A600_EC_SEC_KEY_SHIFT) & AFT_CHIPCFG_A600_EC_SEC_KEY_MASK){
			case 0x00:
				return 0;
			case 0x01:
				return 5;
			default:
				return 0;
		}
	}
	
	return 0;
}

static __inline u32
aft_chipcfg_get_b601_ec_channels(u32 reg)
{
	if (wan_test_bit(AFT_CHIPCFG_B600_EC_CHIP_PRESENT_BIT, &reg)) {
		return 64;
	} 
	return 0;
}


static __inline u32
aft_chipcfg_get_b601_security(u32 reg)
{
	return((reg >> AFT_CHIPCFG_A600_EC_SEC_KEY_SHIFT) & AFT_CHIPCFG_A600_EC_SEC_KEY_MASK);
}

static __inline u32
aft_chipcfg_get_a700_ec_channels(u32 reg)
{
	switch ((reg>>AFT_CHIPCFG_A700_EC_SEC_KEY_SHIFT)&AFT_CHIPCFG_A700_EC_SEC_KEY_MASK){
		case 0x0:
			return 0;
		case 0x1:
			return 16;
		default:
			return 0;
	}

	return 0;
}


# define AFT_WDTCTRL_MASK		0xFF
# define AFT_WDTCTRL_TIMEOUT 		75	/* ms */

static __inline void 
aft_wdt_ctrl_reset(u8 *reg)
{
	*reg=0xFF;
}
static __inline void 
aft_wdt_ctrl_set(u8 *reg, u8 timeout)
{
	timeout&=AFT_WDTCTRL_MASK;
	*reg=timeout;
}


#define AFT_FIFO_MARK_32_MASK	0x3F
#define AFT_FIFO_MARK_32_SHIFT	0

#define AFT_FIFO_MARK_64_MASK	0x3F
#define AFT_FIFO_MARK_64_SHIFT	6

#define AFT_FIFO_MARK_128_MASK	0x3F
#define AFT_FIFO_MARK_128_SHIFT	12

#define AFT_FIFO_MARK_256_MASK	0x3F
#define AFT_FIFO_MARK_256_SHIFT	18

#define AFT_FIFO_MARK_512_MASK	0x3F
#define AFT_FIFO_MARK_512_SHIFT	24

static __inline void
aft_fifo_mark_gset(u32 *reg, u8 mark)
{	
	mark&=AFT_FIFO_MARK_32_MASK;
	
	*reg=0;
	*reg=(mark<<AFT_FIFO_MARK_32_SHIFT)|
	     (mark<<AFT_FIFO_MARK_64_SHIFT)|
	     (mark<<AFT_FIFO_MARK_128_SHIFT)|
	     (mark<<AFT_FIFO_MARK_256_SHIFT)|
	     (mark<<AFT_FIFO_MARK_512_SHIFT);
}



/*======================================================
 * 
 * AFT AFT_CHIP_STAT_REG 
 *
 *=====================================================*/





/*======================================================
 * 
 * AFT_LINE_CFG_REG 
 *
 *=====================================================*/

#define AFT_LINE_CFG_REG		0x100

#define AFT_TX_DMA_INTR_MASK_REG	0x104

#define AFT_TX_DMA_INTR_PENDING_REG	0x108

#define AFT_TX_FIFO_INTR_PENDING_REG	0x10C

#define AFT_RX_DMA_INTR_MASK_REG	0x110

#define AFT_RX_DMA_INTR_PENDING_REG	0x114

#define AFT_RX_FIFO_INTR_PENDING_REG	0x118

#define AFT_SERIAL_LINE_CFG_REG		0x210

# define AFT_LCFG_FE_IFACE_RESET_BIT	0
# define AFT_LCFG_TX_FE_SYNC_STAT_BIT	1
# define AFT_LCFG_TX_FE_FIFO_STAT_BIT	2
# define AFT_LCFG_RX_FE_SYNC_STAT_BIT	3
# define AFT_LCFG_RX_FE_FIFO_STAT_BIT	4

# define AFT_LCFG_SS7_MODE_BIT		5
# define AFT_LCFG_SS7_LSSU_FRMS_SZ_BIT	6

# define AFT_LCFG_DMA_INTR_BIT		8
# define AFT_LCFG_FIFO_INTR_BIT		9

# define AFT_LCFG_TDMV_CH_NUM_MASK	0x1F
# define AFT_LCFG_TDMV_CH_NUM_SHIFT	10

# define AFT_LCFG_TDMV_INTR_BIT		15

# define AFT_LCFG_A108_CLK_ROUTE_MASK	0x0F
# define AFT_LCFG_A108_CLK_ROUTE_SHIFT	16


# define AFT_LCFG_FE_SYNC_CNT_MASK		0xFF
# define AFT_LCFG_FE_SYNC_CNT_SHIFT		20

/* Not Digital */
# define AFT_LCFG_CLR_CHNL_EN		26
# define AFT_LCFG_FE_CLK_ROUTE_BIT	27

# define AFT_LCFG_FE_CLK_SOURCE_MASK	0x03
# define AFT_LCFG_FE_CLK_SOURCE_SHIFT	28


# define AFT_LCFG_GREEN_LED_BIT		30
# define AFT_LCFG_A108_FE_CLOCK_MODE_BIT 31	/* A108 */

# define AFT_LCFG_RED_LED_BIT		31
# define AFT_LCFG_A108_FE_TE1_MODE_BIT	30	/* A108 */

/*A116 TAP*/
#define AFT_LCFG_T116_PORT_REG		0x06
#define AFT_LCFG_T116_FE_FIFO_WRITE_ERR	0
#define AFT_LCFG_T116_FE_FIFO_UNDERFLOW	1
#define AFT_LCFG_T116_FE_FIFO_OVERFLOW	2
#define AFT_LCFG_T116_FE_RX_SYNC	3
#define AFT_LCFG_T116_FE_RESET		6
#define AFT_LCFG_T116_FE_MODE		7

#define AFT_DCM_T116_REG			0x03
#define AFT_DCM_T116_PHASE_ERR_BIT		0
#define AFT_DCM_T116_LOSS_OF_CLOCK_BIT	1
#define AFT_DCM_T116_CLKFX_ERROR_BIT	2


#define AFT_LCFG_B601_CLK_ROUTE_MASK		0x07
#define AFT_LCFG_B601_CLK_ROUTE_SHIFT		16

#define AFT_LCFG_B601_CLK_SRC_LINE_0		0x00    /* Receive clock from line 0 is source */
#define AFT_LCFG_B601_CLK_SRC_LINE_1		0x01    /* Receive clock from line 1 is source , Not used for now */
#define AFT_LCFG_B601_CLK_SRC_EXT_2000HZ	0x02       /* External clock in case 2.048 Mhz is the source */
#define AFT_LCFG_B601_CLK_SRC_EXT_1500HZ	0x03       /* External clock in case 1.544 Mhz is the source */
#define AFT_LCFG_B601_CLK_SRC_OSC_2000HZ	0x04    /* Master Board oscillator 2.048 Mhz is the source */
#define AFT_LCFG_B601_CLK_SRC_OSC_1500HZ	0x05    /* Master Board oscillator 1.544 Mhz is the source */

#define AFT_LCFG_B601_FE_CHIP_RESET_BIT       5 /* For B601 only */
#define AFT_LCFG_B601_GREEN_LED_BIT           6
#define AFT_LCFG_B601_RED_LED_BIT                     7

#define AFT_B601_ECHO_EN_CTRL_REG                     0x210
#define AFT_B601_DATA_MUX_EN_CTRL_REG         0x20C

static __inline void
aft_lcfg_b601_fe_clk_source(u32 *reg, u32 src)
{
       *reg&=~(AFT_LCFG_B601_CLK_ROUTE_MASK<<AFT_LCFG_B601_CLK_ROUTE_SHIFT);
       *reg|=(src&AFT_LCFG_B601_CLK_ROUTE_MASK)<<AFT_LCFG_B601_CLK_ROUTE_SHIFT;
}

static __inline void
aft_lcfg_fe_clk_source(u32 *reg, u32 src)
{
	*reg&=~(AFT_LCFG_FE_CLK_SOURCE_MASK<<AFT_LCFG_FE_CLK_SOURCE_SHIFT);
	*reg|=(src&AFT_LCFG_FE_CLK_SOURCE_MASK)<<AFT_LCFG_FE_CLK_SOURCE_SHIFT;
}

static __inline void
aft_lcfg_a108_fe_clk_source(u32 *reg, u32 src)
{
	*reg&=~(AFT_LCFG_A108_CLK_ROUTE_MASK<<AFT_LCFG_A108_CLK_ROUTE_SHIFT);
	*reg|=(src&AFT_LCFG_A108_CLK_ROUTE_MASK)<<AFT_LCFG_A108_CLK_ROUTE_SHIFT;
}


static __inline void
aft_lcfg_tdmv_cnt_inc(u32 *reg)
{
	u32 cnt = (*reg>>AFT_LCFG_TDMV_CH_NUM_SHIFT)&AFT_LCFG_TDMV_CH_NUM_MASK;
	if (cnt < 32){
		cnt++;
	}
	*reg&=~(AFT_LCFG_TDMV_CH_NUM_MASK<<AFT_LCFG_TDMV_CH_NUM_SHIFT);
	*reg|=(cnt&AFT_LCFG_TDMV_CH_NUM_MASK)<<AFT_LCFG_TDMV_CH_NUM_SHIFT;
}
static __inline void
aft_lcfg_tdmv_cnt_dec(u32 *reg)
{
	u32 cnt = (*reg>>AFT_LCFG_TDMV_CH_NUM_SHIFT)&AFT_LCFG_TDMV_CH_NUM_MASK;
	if (cnt > 0){
		cnt--;
	}
	*reg&=~(AFT_LCFG_TDMV_CH_NUM_MASK<<AFT_LCFG_TDMV_CH_NUM_SHIFT);
	*reg|=(cnt&AFT_LCFG_TDMV_CH_NUM_MASK)<<AFT_LCFG_TDMV_CH_NUM_SHIFT;
}

static __inline u32
aft_lcfg_get_fe_sync_cnt(u32 reg)
{
	return (reg>>AFT_LCFG_FE_SYNC_CNT_SHIFT)&AFT_LCFG_FE_SYNC_CNT_MASK;
}

static __inline void
aft_lcfg_set_fe_sync_cnt(u32 *reg, int cnt)
{
	*reg&=~(AFT_LCFG_FE_SYNC_CNT_MASK<<AFT_LCFG_FE_SYNC_CNT_SHIFT);
	*reg|=((cnt&AFT_LCFG_FE_SYNC_CNT_MASK)<<AFT_LCFG_FE_SYNC_CNT_SHIFT);
}


/* SS7 LCFG MODE
 * 0: 128
 * 1: 4096
 *
 * SS7 LSSU FRM SZ
 * 0: 4byte (if 128) || 7byte (if 4096)
 * 1: 5byte (if 128) || 8byte (if 4096)
 */

static __inline void 
aft_lcfg_ss7_mode128_cfg(u32 *reg, int lssu_size)
{
	wan_clear_bit(AFT_LCFG_SS7_MODE_BIT,reg);
	if (lssu_size == 4){
		wan_clear_bit(AFT_LCFG_SS7_LSSU_FRMS_SZ_BIT,reg);
	}else{
		wan_set_bit(AFT_LCFG_SS7_LSSU_FRMS_SZ_BIT,reg);
	}
}

static __inline void 
aft_lcfg_ss7_mode4096_cfg(u32 *reg, int lssu_size)
{
	wan_set_bit(AFT_LCFG_SS7_MODE_BIT,reg);
	if (lssu_size == 7){
		wan_clear_bit(AFT_LCFG_SS7_LSSU_FRMS_SZ_BIT,reg);
	}else{
		wan_set_bit(AFT_LCFG_SS7_LSSU_FRMS_SZ_BIT,reg);
	}
}


/*======================================================
 * PER PORT 
 * 
 * AFT General DMA Registers
 *
 *=====================================================*/

#define AFT_DMA_CTRL_REG		0x200

#define AFT_TX_DMA_CTRL_REG		0x204

#define AFT_RX_DMA_CTRL_REG		0x208

#define AFT_ANALOG_DATA_MUX_CTRL_REG	0x20C

#define AFT_ANALOG_EC_CTRL_REG		0x210

# define AFT_DMACTRL_TDMV_RX_TOGGLE	14
# define AFT_DMACTRL_TDMV_TX_TOGGLE	15

# define AFT_DMACTRL_MAX_LOGIC_CH_SHIFT 16
# define AFT_DMACTRL_MAX_LOGIC_CH_MASK  0x1F

# define AFT_DMACTRL_GLOBAL_INTR_BIT	31

static __inline void 
aft_dmactrl_set_max_logic_ch(u32 *reg, int max_logic_ch)
{
	*reg&=~(AFT_DMACTRL_MAX_LOGIC_CH_MASK<<AFT_DMACTRL_MAX_LOGIC_CH_SHIFT);
	max_logic_ch&=AFT_DMACTRL_MAX_LOGIC_CH_MASK;
	*reg|=(max_logic_ch<<AFT_DMACTRL_MAX_LOGIC_CH_SHIFT);
}

static __inline u32 
aft_dmactrl_get_max_logic_ch(u32 reg)
{
	u32 max_logic_ch;
	reg&=(AFT_DMACTRL_MAX_LOGIC_CH_MASK<<AFT_DMACTRL_MAX_LOGIC_CH_SHIFT);
	max_logic_ch = reg >> AFT_DMACTRL_MAX_LOGIC_CH_SHIFT;
	return max_logic_ch;
}


/*======================================================
 * PER PORT 
 * 
 * AFT Control RAM DMA Registers
 *
 *=====================================================*/

#define AFT_CONTROL_RAM_ACCESS_BASE_REG	0x1000


# define AFT_CTRLRAM_LOGIC_CH_NUM_MASK	0x1F
# define AFT_CTRLRAM_LOGIC_CH_NUM_SHIFT	0

# define AFT_CTRLRAM_HDLC_MODE_BIT	  7
# define AFT_CTRLRAM_HDLC_CRC_SIZE_BIT	  8
# define AFT_CTRLRAM_HDLC_TXCH_RESET_BIT  9
# define AFT_CTRLRAM_HDLC_RXCH_RESET_BIT 10

# define AFT_CTRLRAM_SYNC_FST_TSLOT_BIT	 11

# define AFT_CTRLRAM_SS7_ENABLE_BIT	 12

# define AFT_CTRLRAM_DATA_MUX_ENABLE_BIT 13
# define AFT_CTRLRAM_SS7_FORCE_RX_BIT	 14

# define AFT_CTRLRAM_FIFO_SIZE_SHIFT	 16
# define AFT_CTRLRAM_FIFO_SIZE_MASK	 0x1F

# define AFT_CTRLRAM_FIFO_BASE_SHIFT	 24
# define AFT_CTRLRAM_FIFO_BASE_MASK	 0x1F


static __inline void
aft_ctrlram_set_logic_ch(u32 *reg, int logic_ch)
{
	*reg&=~(AFT_CTRLRAM_LOGIC_CH_NUM_MASK<<AFT_CTRLRAM_LOGIC_CH_NUM_SHIFT);
	logic_ch&=AFT_CTRLRAM_LOGIC_CH_NUM_MASK;
	*reg|=logic_ch;
}

static __inline void
aft_ctrlram_set_fifo_size(u32 *reg, int size)
{
	*reg&=~(AFT_CTRLRAM_FIFO_SIZE_MASK<<AFT_CTRLRAM_FIFO_SIZE_SHIFT);
	size&=AFT_CTRLRAM_FIFO_SIZE_MASK;
	*reg|=(size<<AFT_CTRLRAM_FIFO_SIZE_SHIFT);
}

static __inline void
aft_ctrlram_set_fifo_base(u32 *reg, int base)
{
	*reg&=~(AFT_CTRLRAM_FIFO_BASE_MASK<<AFT_CTRLRAM_FIFO_BASE_SHIFT);
	base&=AFT_CTRLRAM_FIFO_BASE_MASK;
	*reg|=(base<<AFT_CTRLRAM_FIFO_BASE_SHIFT);
}


/*======================================================
 * PER PORT 
 * 
 * AFT DMA Chain RAM Registers
 *
 *=====================================================*/

#define AFT_DMA_CHAIN_RAM_BASE_REG 	0x1800

# define AFT_DMACHAIN_TX_ADDR_CNT_MASK	0x0F
# define AFT_DMACHAIN_TX_ADDR_CNT_SHIFT 0

# define AFT_DMACHAIN_RX_ADDR_CNT_MASK	0x0F
# define AFT_DMACHAIN_RX_ADDR_CNT_SHIFT 8

# define AFT_DMACHAIN_FIFO_SIZE_MASK	0x1F
# define AFT_DMACHAIN_FIFO_SIZE_SHIFT	16

/* AFT_DMACHAIN_TDMV_SIZE Bit Map
 * 00=  8byte size
 * 01= 16byte size
 * 10= 32byte size
 * 11= 64byte size 
 */
# define AFT_DMACHAIN_TDMV_SIZE_MASK	0x03
# define AFT_DMACHAIN_TDMV_SIZE_SHIFT	21

# define AFT_DMACHAIN_TDMV_ENABLE_BIT	23

# define AFT_DMACHAIN_FIFO_BASE_MASK	0x1F
# define AFT_DMACHAIN_FIFO_BASE_SHIFT	24

# define AFT_DMACHAIN_SS7_FORCE_RX	29

# define AFT_DMACHAIN_SS7_ENABLE_BIT	30

# define AFT_DMACHAIN_RX_SYNC_BIT 	31

static __inline u32
aft_dmachain_get_tx_dma_addr(u32 reg)
{
	reg=reg>>AFT_DMACHAIN_TX_ADDR_CNT_SHIFT;
	reg&=AFT_DMACHAIN_TX_ADDR_CNT_MASK;
	return reg;
}
static __inline void
aft_dmachain_set_tx_dma_addr(u32 *reg, int addr)
{
	*reg&=~(AFT_DMACHAIN_TX_ADDR_CNT_MASK<<AFT_DMACHAIN_TX_ADDR_CNT_SHIFT);
	addr&=AFT_DMACHAIN_TX_ADDR_CNT_MASK;
	*reg|=addr<<AFT_DMACHAIN_TX_ADDR_CNT_SHIFT;
	return;
}

static __inline u32
aft_dmachain_get_rx_dma_addr(u32 reg)
{
	reg=reg>>AFT_DMACHAIN_RX_ADDR_CNT_SHIFT;
	reg&=AFT_DMACHAIN_RX_ADDR_CNT_MASK;
	return reg;
}


static __inline void
aft_dmachain_set_rx_dma_addr(u32 *reg, int addr)
{
	*reg&=~(AFT_DMACHAIN_RX_ADDR_CNT_MASK<<AFT_DMACHAIN_RX_ADDR_CNT_SHIFT);
	addr&=AFT_DMACHAIN_RX_ADDR_CNT_MASK;
	*reg|=addr<<AFT_DMACHAIN_RX_ADDR_CNT_SHIFT;
	return;
}


static __inline void
aft_dmachain_set_fifo_size(u32 *reg, int size)
{
	*reg&=~(AFT_DMACHAIN_FIFO_SIZE_MASK<<AFT_DMACHAIN_FIFO_SIZE_SHIFT);
	size&=AFT_DMACHAIN_FIFO_SIZE_MASK;
	*reg|=(size<<AFT_DMACHAIN_FIFO_SIZE_SHIFT);
}

static __inline void
aft_dmachain_set_fifo_base(u32 *reg, int base)
{
	*reg&=~(AFT_DMACHAIN_FIFO_BASE_MASK<<AFT_DMACHAIN_FIFO_BASE_SHIFT);
	base&=AFT_DMACHAIN_FIFO_BASE_MASK;
	*reg|=(base<<AFT_DMACHAIN_FIFO_BASE_SHIFT);
}

static __inline void
aft_dmachain_enable_tdmv_and_mtu_size(u32 *reg, int size)
{
	*reg&=~(AFT_DMACHAIN_TDMV_SIZE_MASK<<AFT_DMACHAIN_TDMV_SIZE_SHIFT);

	switch(size){
	case 8:
		size=0x00;
		break;
	case 16:
		size=0x01;
		break;
	case 32:
		size=0x02;
		break;
	case 40:
		size=0x02;
		break;
	case 80:
		size=0x03;
		break;
	default:
		size=0x00;
		break;
	}
	
	size&=AFT_DMACHAIN_TDMV_SIZE_MASK;
	*reg|=(size<<AFT_DMACHAIN_TDMV_SIZE_SHIFT);

	wan_set_bit(AFT_DMACHAIN_TDMV_ENABLE_BIT,reg);
}



/*======================================================
 * PER PORT 
 * 
 * AFT DMA Descr RAM Registers
 * AFT_SERIAL_LINE_CFG_REG
 *=====================================================*/

#define AFT_SERIAL_LCFG_RTS_BIT  	0
#define AFT_SERIAL_LCFG_DTR_BIT  	1
#define AFT_SERIAL_LCFG_CTS_BIT  	2
#define AFT_SERIAL_LCFG_DCD_BIT  	3
#define AFT_SERIAL_LCFG_POLARITY_BIT 	4
#define AFT_SERIAL_LCFG_SWMODE_BIT	5
#define AFT_SERIAL_LCFG_X21_MODE_BIT    6

#define AFT_SERIAL_LCFG_CTRL_BIT_MASK 0x0F

#define AFT_SERIAL_LCFG_BAUD_SHIFT	8
#define AFT_SERIAL_LCFG_BAUD_ORIG_MASK  0xFFFF
#define AFT_SERIAL_LCFG_BAUD_MASK	0xFFF

#define AFT_SERIAL_LCFG_CLK_OSC_SEL_BIT 23
#define AFT_SERIAL_LCFG_CLK_SRC_BIT	24
#define AFT_SERIAL_LCFG_CTS_INTR_EN_BIT	25
#define AFT_SERIAL_LCFG_DCD_INTR_EN_BIT 26
#define AFT_SERIAL_LCFG_IDLE_DET_BIT	27

#define AFT_SERIAL_LCFG_LCODING_MASK	0x07
#define AFT_SERIAL_LCFG_LCODING_SHIFT	28

#define AFT_SERIAL_LCFG_IFACE_TYPE_BIT	31

//This aft_serial_set_baud_rate wzs the original one before the implementation of RECOVERY CLOCK
//We are keeping it here just to be safe, in case of complaints from customers
#if 0 
static __inline void
aft_serial_set_baud_rate(u32 *reg, u32 rate)
{
	u32 div= (14745600/(2*rate));
	u32 remain= (14745600%(2*rate));
	if (!remain || (remain/2 > rate)) {
		div=div-1;	
	}
	}
	if (div > 0) {
		div=div-1;
	}
	
	if (div==0) {
		DEBUG_EVENT("wanpipe: Error: Baud Rate Requested %i too high! Cannot be generated by hardware!\n",rate);		
		return -1;
	}
	*reg&=~(AFT_SERIAL_LCFG_BAUD_MASK<<AFT_SERIAL_LCFG_BAUD_SHIFT);
	*reg|= ((div)&AFT_SERIAL_LCFG_BAUD_MASK)<<AFT_SERIAL_LCFG_BAUD_SHIFT;

	wan_clear_bit(AFT_SERIAL_LCFG_CLK_OSC_SEL_BIT,&reg);
}
#endif

static __inline void
aft_serial_set_lcoding(u32 *reg, u32 coding)
{
	*reg&=~(AFT_SERIAL_LCFG_LCODING_MASK<<AFT_SERIAL_LCFG_LCODING_SHIFT);
	*reg|= (coding&AFT_SERIAL_LCFG_LCODING_MASK)<<AFT_SERIAL_LCFG_LCODING_SHIFT;
}

static __inline int
aft_serial_set_legacy_baud_rate(u32 *reg, u32 rate)
{
	u32 div,remain;
	div=14745600/(2*rate);
	remain=14745600%(2*rate);
	if (remain > rate) {
		div=div+1;
	}
	if (div > 0) {
		div=div-1;
	}
	
	if (div==0) {
		DEBUG_ERROR("wanpipe: Error: Baud Rate Requested %i too high! Cannot be generated by hardware!\n",rate);
		return -1;
	}

	*reg&=~(AFT_SERIAL_LCFG_BAUD_ORIG_MASK<<AFT_SERIAL_LCFG_BAUD_SHIFT);
	*reg|= ((div)&AFT_SERIAL_LCFG_BAUD_ORIG_MASK)<<AFT_SERIAL_LCFG_BAUD_SHIFT;

	wan_clear_bit(AFT_SERIAL_LCFG_CLK_OSC_SEL_BIT,reg);

	return 0;
}

static __inline int
aft_serial_set_baud_rate(u32 *reg, u32 rate, u32 recovery)
{
	u32 div,remain,freq,percent;
	u32 acc1,acc2,div1,div2,round1,round2;

	round1=round2=0;

	if (rate == 0) {
		return -1;
	}

	div=14745600/(2*rate);
	remain=14745600%(2*rate);
	DEBUG_TEST("DIV=%u REMAIN1 %u  rate=%u\n",div,remain,rate);
	if (remain > rate) {	
		div=div+1;
		round1=1;
	}
	if (div > 0) {
		div=div-1;
	}	
	div1=div;

	if (remain > rate) {
		acc1= 2*rate - remain;
	} else {
		acc1=remain;
	}

	div=(12352000)/(2*rate);
	remain=(12352000)%(2*rate);
	DEBUG_TEST("DIV=%u REMAIN2 %u  rate=%u\n",div,remain,rate);
	if (remain > rate) {
		div=div+1;
		round2=1;
	}

	if (div > 0) {
		div=div-1;
	}
	div2=div;

	if (remain > rate) {
		acc2= 2*rate - remain;
	} else {
		acc2=remain;
	}
	
	if (acc1 > acc2) {
		wan_set_bit(AFT_SERIAL_LCFG_CLK_OSC_SEL_BIT,reg);
		div=div2;

		if (div == 0) {
			DEBUG_ERROR("wanpipe: Error: Baud Rate Requested %i too high! Cannot be generated by hardware!\n", recovery?rate/32:rate);
		return -1;
		}
		
		DEBUG_EVENT("NEW OSC: DIV=%i (DIV1=%i DIV2=%i) (ACC1=%i ACC2=%i)\n",div,div1,div2,acc1,acc2);
		freq=12352000/(2*(div+round2+1));
		percent=abs(freq-rate)*100/rate;

		if (recovery) {
			freq=freq/32;
			rate=rate/32;
		}
		
		if (percent > 2) {
			DEBUG_ERROR("wanpipe: Error: Baud Rate Requested %i cannot be generated by hardware! Actual=%i Tolerance=%i\n", rate,freq,percent);
			return -EINVAL;
		}

		DEBUG_EVENT("wanpipe: Baud Rate Requested %i, Baud Rate Generated %i, Tolerance=%i\n",rate,freq,percent);
	} else {
		wan_clear_bit(AFT_SERIAL_LCFG_CLK_OSC_SEL_BIT,reg);
		div=div1;
		
		if (div == 0) {
			DEBUG_ERROR("wanpipe: Error: Baud Rate Requested %i too high! Cannot be generated by hardware!\n", recovery?rate/32:rate);
			return -1;
		}
	
		DEBUG_EVENT("ORIG OSC: DIV=%i (DIV1=%i DIV2=%i) (ACC1=%i ACC2=%i)\n",div,div1,div2,acc1,acc2);
		freq=14745600/(2*(div+round1+1));
		percent=abs(freq-rate)*100/rate;

		if (recovery) {
			freq=freq/32;
			rate=rate/32;
		}

		if (percent > 2) {
			DEBUG_ERROR("wanpipe: Error: Baud Rate Requested %i cannot be generated by hardware! Actual=%i Tolerance=%i\n", rate,freq,percent);
			return -EINVAL;
		}

		DEBUG_EVENT("wanpipe: Baud Rate Requested %i, Baud Rate Generated %i, Tolerance=%i\n", rate,freq,percent);
	}

	*reg&=~(AFT_SERIAL_LCFG_BAUD_MASK<<AFT_SERIAL_LCFG_BAUD_SHIFT);
	*reg|= ((div)&AFT_SERIAL_LCFG_BAUD_MASK)<<AFT_SERIAL_LCFG_BAUD_SHIFT;
	DEBUG_EVENT(" OSC: DIV=%i REG=0x%08X \n",div,*reg);

	return 0;
}
/*======================================================
 * FREE RUNNING TIMER
 * 
 * AFT_FREE_RUN_TIMER_CTRL_REG
 *=====================================================*/

#define AFT_FREE_RUN_TIMER_DIVIDER_SHIFT		0
#define AFT_FREE_RUN_TIMER_DIVIDER_MASK			0x3F

#define AFT_FREE_RUN_TIMER_INTER_ENABLE_BIT		7

static __inline void
aft_free_running_timer_ctrl_set(u32 *reg, u32 divider)
{
	*reg&=~(AFT_FREE_RUN_TIMER_DIVIDER_MASK<<AFT_FREE_RUN_TIMER_DIVIDER_SHIFT);
	*reg|=(divider&AFT_FREE_RUN_TIMER_DIVIDER_MASK)<<AFT_FREE_RUN_TIMER_DIVIDER_SHIFT;
}



/*======================================================
 * PER PORT 
 * 
 * AFT DMA Descr RAM Registers
 *
 *=====================================================*/

#define AFT_TX_DMA_LO_DESCR_BASE_REG	0x2000
#define AFT_TX_DMA_HI_DESCR_BASE_REG	0x2004

#define AFT_RX_DMA_LO_DESCR_BASE_REG	0x2008
#define AFT_RX_DMA_HI_DESCR_BASE_REG	0x200C


# define AFT_TXDMA_LO_ALIGN_MASK	0x03
# define AFT_TXDMA_LO_ALIGN_SHIFT	0

# define AFT_TXDMA_HI_DMA_LENGTH_MASK	0xFFF
# define AFT_TXDMA_HI_DMA_LENGTH_SHIFT	0

# define AFT_TXDMA_HI_DMA_STATUS_MASK	0x0F
# define AFT_TXDMA_HI_DMA_STATUS_SHIFT	11

#  define AFT_TXDMA_HIDMASTATUS_PCI_M_ABRT	0
#  define AFT_TXDMA_HIDMASTATUS_PCI_T_ABRT	1
#  define AFT_TXDMA_HIDMASTATUS_PCI_DS_TOUT	2
#  define AFT_TXDMA_HIDMASTATUS_PCI_RETRY	3

# define AFT_TXDMA_HI_SS7_FI_LS_TX_STATUS_BIT	19
# define AFT_TXDMA_HI_SS7_FISU_OR_LSSU_BIT	20
# define AFT_TXDMA_HI_SS7_FI_LS_FORCE_TX_BIT	21

# define AFT_TXDMA_HI_LAST_DESC_BIT	24
# define AFT_TXDMA_HI_INTR_DISABLE_BIT	27
# define AFT_TXDMA_HI_DMA_CMD_BIT	28
# define AFT_TXDMA_HI_EOF_BIT		29
# define AFT_TXDMA_HI_START_BIT		30
# define AFT_TXDMA_HI_GO_BIT		31

# define AFT_RXDMA_LO_ALIGN_MASK	0x03
# define AFT_RXDMA_LO_ALIGN_SHIFT	0

# define AFT_RXDMA_HI_DMA_LENGTH_MASK	0xFFF
# define AFT_RXDMA_HI_DMA_LENGTH_SHIFT	0

# define AFT_RXDMA_HI_DMA_STATUS_MASK	0x0F
# define AFT_RXDMA_HI_DMA_STATUS_SHIFT	11

#  define AFT_RXDMA_HIDMASTATUS_PCI_M_ABRT	0
#  define AFT_RXDMA_HIDMASTATUS_PCI_T_ABRT	1
#  define AFT_RXDMA_HIDMASTATUS_PCI_DS_TOUT	2
#  define AFT_RXDMA_HIDMASTATUS_PCI_RETRY	3

# define AFT_RXDMA_HI_IFT_INTR_ENB_BIT	15
# define AFT_RXDMA_HI_LAST_DESC_BIT	24
# define AFT_RXDMA_HI_FCS_ERR_BIT	25
# define AFT_RXDMA_HI_FRM_ABORT_BIT	26
# define AFT_RXDMA_HI_INTR_DISABLE_BIT	27
# define AFT_RXDMA_HI_DMA_CMD_BIT	28
# define AFT_RXDMA_HI_EOF_BIT		29
# define AFT_RXDMA_HI_START_BIT		30
# define AFT_RXDMA_HI_GO_BIT		31


static __inline void
aft_txdma_lo_set_alignment(u32 *reg, int align)
{
	*reg&=~(AFT_TXDMA_LO_ALIGN_MASK<<AFT_TXDMA_LO_ALIGN_SHIFT);
	align&=AFT_TXDMA_LO_ALIGN_MASK;
	*reg|=(align<<AFT_TXDMA_LO_ALIGN_SHIFT);
}
static __inline u32
aft_txdma_lo_get_alignment(u32 reg)
{
	reg=reg>>AFT_TXDMA_LO_ALIGN_SHIFT;
	reg&=AFT_TXDMA_LO_ALIGN_MASK;
	return reg;
}

static __inline void
aft_txdma_hi_set_dma_length(u32 *reg, int len, int align)
{
	*reg&=~(AFT_TXDMA_HI_DMA_LENGTH_MASK<<AFT_TXDMA_HI_DMA_LENGTH_SHIFT);
	len=(len>>2)+align;
	len&=AFT_TXDMA_HI_DMA_LENGTH_MASK;
	*reg|=(len<<AFT_TXDMA_HI_DMA_LENGTH_SHIFT);
}

static __inline u32
aft_txdma_hi_get_dma_length(u32 reg)
{
	reg=reg>>AFT_TXDMA_HI_DMA_LENGTH_SHIFT;
	reg&=AFT_TXDMA_HI_DMA_LENGTH_MASK;
	reg=reg<<2;
	return reg;	
}

static __inline u32
aft_txdma_hi_get_dma_status(u32 reg)
{
	reg=reg>>AFT_TXDMA_HI_DMA_STATUS_SHIFT;
	reg&=AFT_TXDMA_HI_DMA_STATUS_MASK;
	return reg;	
}

static __inline void
aft_rxdma_lo_set_alignment(u32 *reg, int align)
{
	*reg&=~(AFT_RXDMA_LO_ALIGN_MASK<<AFT_RXDMA_LO_ALIGN_SHIFT);
	align&=AFT_RXDMA_LO_ALIGN_MASK;
	*reg|=(align<<AFT_RXDMA_LO_ALIGN_SHIFT);
}
static __inline u32
aft_rxdma_lo_get_alignment(u32 reg)
{
	reg=reg>>AFT_RXDMA_LO_ALIGN_SHIFT;
	reg&=AFT_RXDMA_LO_ALIGN_MASK;
	return reg;
}

static __inline void
aft_rxdma_hi_set_dma_length(u32 *reg, int len, int align)
{
	*reg&=~(AFT_RXDMA_HI_DMA_LENGTH_MASK<<AFT_RXDMA_HI_DMA_LENGTH_SHIFT);
	len=(len>>2)-align;
	len&=AFT_RXDMA_HI_DMA_LENGTH_MASK;
	*reg|=(len<<AFT_RXDMA_HI_DMA_LENGTH_SHIFT);
}

static __inline u32
aft_rxdma_hi_get_dma_length(u32 reg)
{
	reg=reg>>AFT_RXDMA_HI_DMA_LENGTH_SHIFT;
	reg&=AFT_RXDMA_HI_DMA_LENGTH_MASK;
	reg=reg<<2;
	return reg;	
}

static __inline u32
aft_rxdma_hi_get_dma_status(u32 reg)
{
	reg=reg>>AFT_RXDMA_HI_DMA_STATUS_SHIFT;
	reg&=AFT_RXDMA_HI_DMA_STATUS_MASK;
	return reg;	
}


#define FIFO_32B			0x00
#define FIFO_64B			0x01
#define FIFO_128B			0x03
#define FIFO_256B			0x07
#define FIFO_512B			0x0F
#define FIFO_1024B			0x1F



/*===============================================

*/

/* Default Active DMA channel used by the
 * DMA Engine */
#define AFT_DEFLT_ACTIVE_CH  0

#define MAX_AFT_TX_DMA_SIZE  0xFFFF

#define MIN_WP_PRI_MTU		8	
#define DEFAULT_WP_PRI_MTU 	1500

/* Maximum MTU for AFT card
 * 8192-4=8188.  This is a hardware
 * limitation. 
 */
#define MAX_WP_PRI_MTU		8188

#define MAX_DMA_PER_CH		20
#define MIN_DMA_PER_CH		2

#define WP_MAX_FIFO_FRAMES      7

#define AFT_DEFAULT_DATA_MUX_MAP 0x01234567
enum {
	WAN_AFT_RED,
	WAN_AFT_GREEN
};

enum {
	WAN_AFT_OFF,
	WAN_AFT_ON
};



/*==========================================
 * Board CPLD Interface Section
 *
 *=========================================*/


#define PMC_CONTROL_REG		0x00

/* Used to reset the pcm 
 * front end 
 *    0: Reset Enable 
 *    1: Normal Operation 
 */
#define PMC_RESET_BIT		0	

/* Used to set the pmc clock
 * source:
 *   0 = E1
 *   1 = T1 
 */
#define PMC_CLOCK_SELECT	1

#define LED_CONTROL_REG		0x01

#define JP8_VALUE               0x02
#define JP7_VALUE               0x01
#define SW0_VALUE               0x04
#define SW1_VALUE               0x08


#define AFT_CUSTOMER_CPLD_ID_REG				0xA0
#define AFT_CUSTOMER_CPLD_ID_TE1_SERIAL_OFFSET	0x02
#define AFT_CUSTOMER_CPLD_ID_ANALOG_OFFSET		0x0A


#define BRI_CPLD0_ECHO_RESET_BIT		0
#define BRI_CPLD0_NETWORK_SYNC_OUT_BIT		2

/* -------------------------------------- */

#define WRITE_DEF_SECTOR_DSBL   0x01
#define FRONT_END_TYPE_MASK     0x38


#define MEMORY_TYPE_SRAM        0x00
#define MEMORY_TYPE_FLASH       0x01
#define MASK_MEMORY_TYPE_SRAM   0x10
#define MASK_MEMORY_TYPE_FLASH  0x20

#define BIT_A18_SECTOR_SA4_SA7  0x20
#define USER_SECTOR_START_ADDR  0x40000

#define MAX_TRACE_QUEUE         500

#define	HDLC_FREE_LOGIC_CH	0x1F					
#define AFT_DEFLT_ACTIVE_CH	0
						
static __inline unsigned short aft_valid_mtu(unsigned short mtu)
{
	unsigned short	new_mtu;

	if (mtu <= 128){
		new_mtu = 256;

	}else if (mtu <= 256){
		new_mtu = 512;

	}else if (mtu <= 512){
		new_mtu = 1024;

	}else if (mtu <= 1024){
		new_mtu = 2048;

	}else if (mtu <= 2048){
		new_mtu = 4096;

	}else if (mtu <= 4096){
		new_mtu = 8188;

	}else if (mtu <= 8188){
		new_mtu = 8188;
	}else{
		return 0;
	}	

#if defined(__FreeBSD__)
	if (new_mtu > MCLBYTES - 16){
		new_mtu = MCLBYTES-16;
	}
#endif
	return new_mtu;
}

static __inline unsigned short aft_dma_buf_bits(unsigned short dma_bufs)
{
	if (dma_bufs < 2){
		return 0;
	}else if (dma_bufs < 3){
		return 1;
	}else if (dma_bufs < 5){
		return 2;
	}else if (dma_bufs < 9){
		return 3;
	}else if (dma_bufs < 17){
		return 4;
	}else{
		return 0;
	}	
}

static __inline void
aft_set_led_b601(unsigned int color, int led_pos, int on, u32 *reg)
{
	if (color == WAN_AFT_RED){
		if (on == WAN_AFT_OFF){
			wan_clear_bit(AFT_LCFG_B601_RED_LED_BIT,reg);
		}else{
			wan_set_bit(AFT_LCFG_B601_RED_LED_BIT,reg);
		}
	}else{
		if (on == WAN_AFT_OFF){
			wan_clear_bit(AFT_LCFG_B601_GREEN_LED_BIT,reg);
		}else{
			wan_set_bit(AFT_LCFG_B601_GREEN_LED_BIT,reg);
		}
	}
}


static __inline void
aft_set_led(unsigned int color, int led_pos, int on, u32 *reg)
{
	if (color == WAN_AFT_RED){
		if (on == WAN_AFT_OFF){
			wan_clear_bit(AFT_LCFG_RED_LED_BIT,reg);
		}else{
			wan_set_bit(AFT_LCFG_RED_LED_BIT,reg);
		}
	}else{
		if (on == WAN_AFT_OFF){
			wan_clear_bit(AFT_LCFG_GREEN_LED_BIT,reg);
		}else{
			wan_set_bit(AFT_LCFG_GREEN_LED_BIT,reg);
		}
	}
}

static __inline int
aft_get_num_of_slots(u32 total_slots, u32 chan_slots)
{	
	int num_of_slots=0;
	u32 i;
	for (i=0;i<total_slots;i++){
		if (wan_test_bit(i,&chan_slots)){
			num_of_slots++;
		}
	}

	return num_of_slots;
}

#define MAX_AFT_HW_DEV 22

typedef struct aft_hw_dev{

	int init;

	int (*aft_global_chip_config)(sdla_t *card);
	int (*aft_global_chip_unconfig)(sdla_t *card);

	int (*aft_chip_config)(sdla_t *card, wandev_conf_t *);
	int (*aft_chip_unconfig)(sdla_t *card);

	int (*aft_chan_config)(sdla_t *card, void *chan);
	int (*aft_chan_unconfig)(sdla_t *card, void *chan);

	int (*aft_led_ctrl)(sdla_t *card, int, int, int);
	int (*aft_test_sync)(sdla_t *card, int);

	unsigned char (*aft_read_fe)(sdla_t* card, unsigned short off);
	int (*aft_write_fe)(sdla_t *card, unsigned short off, unsigned char value);

	unsigned char (*aft_read_cpld)(sdla_t *card, unsigned short cpld_off);
	int (*aft_write_cpld)(sdla_t *card, unsigned short cpld_off, u_int16_t cpld_data);

	void (*aft_fifo_adjust)(sdla_t *card, u32 level);

	int (*aft_check_ec_security)(sdla_t *card);

	int (*aft_set_fe_clock)(sdla_t *card);

}aft_hw_dev_t;

extern aft_hw_dev_t aft_hwdev[MAX_AFT_HW_DEV];

#define ASSERT_AFT_HWDEV(type) \
	if (type >= MAX_AFT_HW_DEV){ \
		DEBUG_EVENT("%s:%d: Critical Invalid AFT HW DEV Type %d\n", \
				__FUNCTION__,__LINE__,type); \
		return -EINVAL;	\
	}\
	if (!wan_test_bit(0,&aft_hwdev[type].init)) { \
		DEBUG_EVENT("%s:%d: Critical AFT HW DEV Type %d not initialized\n", \
				__FUNCTION__,__LINE__,type); \
		return -EINVAL; \
	}

#define ASSERT_AFT_HWDEV_VOID(type) \
	if (type >= MAX_AFT_HW_DEV){ \
		DEBUG_EVENT("%s:%d: Critical Invalid AFT HW DEV Type %d\n", \
				__FUNCTION__,__LINE__,type); \
		return;	\
	}\
	if (!aft_hwdev[type] || !wan_test_bit(0,&aft_hwdev[type].init)) { \
		DEBUG_EVENT("%s:%d: Critical AFT HW DEV Type %d not initialized\n", \
				__FUNCTION__,__LINE__,type); \
		return; \
	}


#endif /* WAN_KERNEL */



#endif
