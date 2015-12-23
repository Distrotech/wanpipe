/*******************************************************************************
 * ** sdla_remora_a600.h
 * **
 * ** Author:      David Yat Sin  <dyatsin@sangoma.com>
 * **
 * ** Copyright:   (c) 2005 Sangoma Technologies Inc.
 * **
 * **              This program is free software; you can redistribute it and/or
 * **              modify it under the terms of the GNU General Public License
 * **              as published by the Free Software Foundation; either version
 * **              2 of the License, or (at your option) any later version.
 * ** ============================================================================
 * ** Nov , 2008  David Yat Sin    Initial version
 * *******************************************************************************/




#ifndef __SDLA_A600_H
#define __SDLA_A600_H


#define AFT_A600_BASE_REG_OFF   0x1000
#define A600_REG_OFF(reg) reg+AFT_A600_BASE_REG_OFF

#define A600_SERIALNUM_REG_HI   0x109C
#define A600_SERIALNUM_REG_LO   0x1098

#define NUM_A600_ANALOG_PORTS 5
#define NUM_A600_ANALOG_FXO_PORTS 4


#define A600_SPI_REG_BROADCAST_MODE_BIT 26
#define A600_SPI_REG_READ_ENABLE_BIT    27 /* 1 for read, 0 for write */
#define A600_SPI_REG_CHAN_TYPE_FXS_BIT  28 /* 0 for FXO, 1 for FXS */
#define A600_SPI_REG_FXS_RESET_BIT      29 /* reset FXS channel */
#define A600_SPI_REG_FX0_RESET_BIT      30 /* reset FX0 channel */
#define A600_SPI_REG_SPI_BUSY_BIT       31 /* 1 busy, 0 ready */
#define A600_SPI_REG_START_BIT          31




#define A600_SPI_REG_CTRL_BYTE_MASK     0xFF000000
#define A600_SPI_REG_ADDR_BYTE_MASK     0x0000FF00
#define A600_SPI_REG_DATA_BYTE_MASK     0x000000FF

extern int wp_a600_iface_init(void*, void*);


#define IS_A600(fe) (((sdla_t*)(fe->card))->adptr_type == AFT_ADPTR_A600 || ((sdla_t*)(fe->card))->adptr_type == AFT_ADPTR_B610)
#define IS_A600_CARD(card) (card->adptr_type == AFT_ADPTR_A600 || card->adptr_type == AFT_ADPTR_B610)

#define IS_B610(fe) (((sdla_t*)(fe->card))->adptr_type == AFT_ADPTR_B610)

#define IS_B601(fe) (((sdla_t*)(fe->card))->adptr_type == AFT_ADPTR_B601)
#define IS_B601_CARD(card) (card->adptr_type == AFT_ADPTR_B601)
#define IS_B601_TE1_CARD(card) (card->adptr_type == AFT_ADPTR_B601 && card->wandev.comm_port == 1)

#define A600_MAXIM_INTERFACE_REG_ADD_LO 0x1044
#define A600_MAXIM_INTERFACE_REG_ADD_HI 0x1046

#define A600_FIRST_LINE_CFG_ADD  0x2100

#define A600_FIRST_LINE_CFG_RESET_BIT 0x01

#endif /* SDLA_A600_H */

