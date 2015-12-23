/*****************************************************************************
* sdladrv_utils.c	SDLADRV utils functions.
*
*
* Author:	Alex Feldman
*
* Copyright:	(c) 2007 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 20, 2007	Alex Feldman	Initial version
*
*****************************************************************************/


/***************************************************************************
****		I N C L U D E  		F I L E S			****
***************************************************************************/

#if defined(__LINUX__)||defined(__KERNEL__)
# define _K22X_MODULE_FIX_
#endif

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"

#include "sdlasfm.h"	/* SDLA firmware module definitions */
#include "sdlapci.h"	/* SDLA PCI hardware definitions */
#include "sdladrv.h"	/* API definitions */


/***************************************************************************
****                     M A C R O S / D E F I N E S                    ****
***************************************************************************/
//This is the address of the ECCTL register , page 149 of PEX8111 datasheet.
#define SDLA_PLXE_CTL			0x1004	

//Change this accordingly.!!!
#define SDLA_PLXE_SIZE			0x7F

#define SDLA_PLXE_SHIFT_WRITE_DATA	0
#define SDLA_PLXE_SHIFT_READ_DATA	8

#define SDLA_PLXE_MASK_BYTE_WRITE_START (1<<16)
#define SDLA_PLXE_MASK_BYTE_READ_START	(1<<17)
#define SDLA_PLXE_MASK_CS_ENABLE	(1<<18)
#define SDLA_PLXE_MASK_BUSY		(1<<19)

//EEPROM COMMANDS
#define SDLA_PLXE_OPCODE_WRITE		0x02
#define SDLA_PLXE_OPCODE_READ		0x03
#define SDLA_PLXE_OPCODE_READ_STATUS	0x05
#define SDLA_PLXE_OPCODE_WREN 		0x06


/***************************************************************************
****               F U N C T I O N   P R O T O T Y P E S                ****
***************************************************************************/

/***************************************************************************
****                      G L O B A L  D A T A                          ****
***************************************************************************/

/***************************************************************************
****               F U N C T I O N   D E F I N I T I O N                ****
***************************************************************************/
static void	sdla_plx_8111Read(void *phw, int addr, int *data);
static void	sdla_plx_8111Write(void *phw, int addr, int data);
static int	sdla_plx_EE_waitidle(void *phw);
static int	sdla_plx_EE_off(void *phw);
static int 	sdla_plx_EE_readbyte(void *phw,unsigned char*);
static int	sdla_plx_EE_writebyte(void *phw, unsigned char);

int	sdla_plxctrl_status(void *phw, unsigned char*);
int	sdla_plxctrl_read8(void *phw, short, unsigned char*);
int	sdla_plxctrl_write8(void *phw, short, unsigned char);
int	sdla_plxctrl_erase(void *phw);

extern int	sdla_pci_bridge_read_config_dword(void*, int, u_int32_t*);
extern int	sdla_pci_bridge_write_config_dword(void*, int, u_int32_t);

/************************************************************************/
/*			GLOBAL VARIABLES				*/
/************************************************************************/

/************************************************************************/
/*			FUNCTION PROTOTYPES				*/
/************************************************************************/

///////////////////////////////////////////////////////////////////
//Write the value read into the dst pointer.
static void sdla_plx_8111Read(void *phw, int addr, int *data)
{	
	WAN_ASSERT_VOID(phw == NULL);
	if (addr < 0x1000){
		sdla_pci_bridge_read_config_dword(phw, addr, data);
	}else if (addr >= 0x1000 && addr <= 0x1FFF){
		sdla_pci_bridge_write_config_dword(phw, 0x84, addr);
		sdla_pci_bridge_read_config_dword(phw, 0x88, data);
	}	
}

///////////////////////////////////////////////////////////////////
static void sdla_plx_8111Write(void *phw, int addr, int data)
{
	WAN_ASSERT_VOID(phw == NULL);
	if (addr < 0x1000){
		sdla_pci_bridge_write_config_dword(phw, addr, data);
	}else if (addr >= 0x1000 && addr <= 0x1FFF){			
		sdla_pci_bridge_write_config_dword(phw, 0x84, addr);
		sdla_pci_bridge_write_config_dword(phw, 0x88, data);
	}
}

///////////////////////////////////////////////////////////////////
static int sdla_plx_EE_waitidle(void *phw)
{	
	sdlahw_t	*hw = (sdlahw_t*)phw;
	int 		eeCtl, ii;

	WAN_ASSERT(phw == NULL);
	for (ii = 0; ii < 100; ii++){
	       	/* read current value in EECTL */
		sdla_plx_8111Read(phw, SDLA_PLXE_CTL, &eeCtl);
		/* loop until idle */
		if ((eeCtl & SDLA_PLXE_MASK_BUSY) == 0){
			return(eeCtl);
		}
		WP_DELAY(1000);
	}
	DEBUG_ERROR("%s: ERROR: EEPROM Busy timeout!\n",
				hw->devname);
	return SDLA_PLXE_MASK_BUSY;
}

///////////////////////////////////////////////////////////////////
static int sdla_plx_EE_off(void *phw)
{	
	
	WAN_ASSERT(phw == NULL);
	 /* make sure EEPROM is idle */
	sdla_plx_EE_waitidle(phw);
	/* turn off everything (especially SDLA_PLXE_MASK_CS_ENABLE)*/
	sdla_plx_8111Write(phw, SDLA_PLXE_CTL, 0);
	return 0; 
}

///////////////////////////////////////////////////////////////////
static int sdla_plx_EE_readbyte(void *phw, unsigned char *data)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	int		i, eeCtl = 0x00;

	WAN_ASSERT(phw == NULL);

	if (sdla_plx_EE_waitidle(phw) & SDLA_PLXE_MASK_BUSY){
		return -EINVAL;
	}

	eeCtl = (SDLA_PLXE_MASK_CS_ENABLE | SDLA_PLXE_MASK_BYTE_READ_START);
	sdla_plx_8111Write(phw, SDLA_PLXE_CTL, eeCtl); /* start reading */
	for (i=0;i<1000;i++){
		sdla_plx_8111Read(phw, SDLA_PLXE_CTL, &eeCtl);
		if ((eeCtl & SDLA_PLXE_MASK_BYTE_READ_START) == 0){
			break;
		}
	}
	if (eeCtl & SDLA_PLXE_MASK_BYTE_READ_START){
		DEBUG_ERROR("%s: ERROR: Timeout on PLX READ!\n",
					hw->devname);
		return -EINVAL;
	}
	
	eeCtl = sdla_plx_EE_waitidle(phw); /* wait until read is done */
	sdla_plx_8111Read(phw, SDLA_PLXE_CTL, &eeCtl);
	*data = (eeCtl >> SDLA_PLXE_SHIFT_READ_DATA) & 0xFF;
	return 0;
}

///////////////////////////////////////////////////////////////////
static int sdla_plx_EE_writebyte(void *phw, unsigned char val)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	int 		i, eeCtl = 0; /* make sure EEPROM is idle */	
	
	WAN_ASSERT(phw == NULL);
	if (sdla_plx_EE_waitidle(phw) & SDLA_PLXE_MASK_BUSY){
		return -EINVAL;
	}

	/* clear current WRITE value */
	eeCtl = (SDLA_PLXE_MASK_CS_ENABLE | SDLA_PLXE_MASK_BYTE_WRITE_START);
	eeCtl |= ((val & 0xff) << SDLA_PLXE_SHIFT_WRITE_DATA);
	sdla_plx_8111Write(phw, SDLA_PLXE_CTL, eeCtl);
	
	for (i=0;i<1000;i++){
		sdla_plx_8111Read(phw, SDLA_PLXE_CTL, &eeCtl);
		if ((eeCtl & SDLA_PLXE_MASK_BYTE_WRITE_START) == 0){
			break;
		}
	}
	if (eeCtl & SDLA_PLXE_MASK_BYTE_WRITE_START){
		DEBUG_ERROR("%s: ERROR: Timeout on PLX write!\n",
						hw->devname);
		return -EINVAL;
	}
	return 0;
}


/***************************************************************************
****			These are the high level functions		****
***************************************************************************/

///////////////////////////////////////////////////////////////////
int sdla_plxctrl_status(void *phw, unsigned char *status)
{	
	WAN_ASSERT(phw == NULL);

    	sdla_plx_EE_writebyte(phw, SDLA_PLXE_OPCODE_READ_STATUS);
	if (sdla_plx_EE_readbyte(phw, status)){
		return -EINVAL;
	}
	sdla_plx_EE_off(phw); /* turn off EEPROM */

	return 0;
}

int sdla_plxctrl_write8(void *phw, short addr, unsigned char data)
{
	WAN_ASSERT(phw == NULL);

	/* must first write-enable */
	sdla_plx_EE_writebyte(phw, SDLA_PLXE_OPCODE_WREN);
	/* turn off EEPROM */
	sdla_plx_EE_off(phw);
	/* opcode to write bytes */
	sdla_plx_EE_writebyte(phw, SDLA_PLXE_OPCODE_WRITE);

	/* Send low byte of address */
	sdla_plx_EE_writebyte(phw, (unsigned char)(addr & 0xFF));
	/* send data to be written */
	sdla_plx_EE_writebyte(phw, (data & 0xFF));
	
	sdla_plx_EE_off(phw); /* turn off EEPROM */
	WP_DELAY(10000);
	return 0;
}

///////////////////////////////////////////////////////////////////
int sdla_plxctrl_read8(void *phw, short addr, unsigned char *data)
{	
	WAN_ASSERT(phw == NULL);

    sdla_plx_EE_writebyte(phw, SDLA_PLXE_OPCODE_READ);
	sdla_plx_EE_writebyte(phw, (unsigned char)(addr & 0xFF));

	if (sdla_plx_EE_readbyte(phw, data)){
		return -EINVAL;
	}
    	sdla_plx_EE_off(phw);
	return 0;
}

int sdla_plxctrl_erase(void *phw)
{	
	int t;

	WAN_ASSERT(phw == NULL);
	for(t = 0; t < SDLA_PLXE_SIZE; t++){
		sdla_plxctrl_write8(phw, t, 0xFF);
	}
	return 0;
}

