/*******************************************************************************
 * ** sdla_b800_remora.h
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




#ifndef __SDLA_B800_REMORA_H
#define __SDLA_B800_REMORA_H


#define AFT_B800_BASE_REG_OFF   0x0000
#define B800_REG_OFF(reg) reg+AFT_B800_BASE_REG_OFF

#define B800_SPI_REG_RESET_BIT					28
#define B800_SPI_REG_READ_ENABLE_BIT    29 /* 1 for read, 0 for write */
#define B800_SPI_REG_CHAN_TYPE_FXS_BIT  30 /* 0 for FXO, 1 for FXS */
#define B800_SPI_REG_SPI_BUSY_BIT       31 /* 1 busy, 0 ready */
#define B800_SPI_REG_START_BIT          31

#define NUM_B800_ANALOG_PORTS           16

#define IS_B800(fe) (((sdla_t*)(fe->card))->adptr_type == AFT_ADPTR_B800)
#define IS_B800_CARD(card) (card->adptr_type == AFT_ADPTR_B800)

#endif /* __SDLA_B800_REMORA_H */
