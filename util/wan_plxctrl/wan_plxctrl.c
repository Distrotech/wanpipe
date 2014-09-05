
/***********************************************************************
* wan_plxctr.c	Sangoma PLX Control Utility.
*
* Copyright:	(c) 2005 AMFELTEC Corp.
*
* ----------------------------------------------------------------------
* Nov 1, 2006	Alex Feldman	Initial version.
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

//This is the address of the ECCTL register , page 149 of PEX8111 datasheet.
#define EECTL			0x1004	

//Change this accordingly.!!!
#define PLXE_SIZE		0xFF

#define PLXE_SHIFT_READ_DATA		8
#define PLXE_SHIFT_WRITE_DATA		0

#define PLXE_MASK_BUSY			(1<<19)
#define PLXE_MASK_CS_ENABLE		(1<<18)
#define PLXE_MASK_BYTE_READ_START	(1<<17)
#define PLXE_MASK_BYTE_WRITE_START	(1<<16)

//EEPROM COMMANDS
#define PLXE_OPCODE_READ_STATUS	0x05
#define PLXE_OPCODE_WREN 		0x06
#define PLXE_OPCODE_WRITE		0x02
#define PLXE_OPCODE_READ		0x03


/************************************************************************/
/*			GLOBAL VARIABLES				*/
/************************************************************************/

/************************************************************************/
/*			FUNCTION PROTOTYPES				*/
/************************************************************************/
void PEX_8111Read(void *info, int addr, int *data);
void PEX_8111Write(void *info, int addr, int data);
int	EE_WaitIdle(void *info);
int	EE_Off(void *info);
int	EE_ReadByte(void *info, unsigned char*);
int	EE_WriteByte(void *info, unsigned char val);

int	wan_plxctrl_status(void *info, unsigned char *);
int	wan_plxctrl_write8(void *info, unsigned char addr, unsigned char data);
int	wan_plxctrl_read8(void *info, unsigned char, unsigned char*);
int	wan_plxctrl_erase(void *info);

extern int exec_read_cmd(void*, unsigned int, unsigned int, unsigned int*);
extern int exec_write_cmd(void*, unsigned int, unsigned int, unsigned int);

/***********************************************
****************FUNCTION PROTOTYPES*************
************************************************/
//These functions are taken from page 37 of PEX8111 datasheet.


//Write the value read into the dst pointer.
void PEX_8111Read(void *info, int addr, int *data)
{	
	if (addr < 0x1000){
		exec_read_cmd(info, addr, 4, (unsigned int*)data);
	}else if (addr >= 0x1000 && addr <= 0x1FFF){
			
		exec_write_cmd(info, 0x84, 4, addr);
		exec_read_cmd(info, 0x88, 4, (unsigned int*)data);
	}	
}
 

void PEX_8111Write(void *info, int addr, int data)
{
	if (addr < 0x1000){
		exec_write_cmd(info, addr, 4, data);
	}else if (addr >= 0x1000 && addr <= 0x1FFF){
			
		exec_write_cmd(info, 0x84, 4, addr);
		exec_write_cmd(info, 0x88, 4, data);
	}
}

///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


int EE_WaitIdle(void *info)
{	
	int eeCtl, ii;
	for (ii = 0; ii < 1000; ii++)
	{
	       	/* read current value in EECTL */
		PEX_8111Read(info, EECTL, &eeCtl);
		/* loop until idle */
		if ((eeCtl & PLXE_MASK_BUSY) == 0)
			return(eeCtl);
	}
	printf("ERROR: EEPROM Busy timeout!\n");
	return PLXE_MASK_BUSY;
}


///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////



int EE_Off(void *info)
{	
	int t = 0;
	
	/* make sure EEPROM is idle */
	if (EE_WaitIdle(info) & PLXE_MASK_BUSY){
		return -EINVAL;
	}
	/* turn off everything (especially PLXE_MASK_CS_ENABLE)*/
	PEX_8111Write(info, EECTL, t);
	return 0;
}

///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


int EE_ReadByte(void *info, unsigned char *data)
{
       	/* make sure EEPROM is idle */
	int		eeCtl,i;

	*data = 0x00;
	if (EE_WaitIdle(info) & PLXE_MASK_BUSY){
		return -EINVAL;
	}

	eeCtl = (PLXE_MASK_CS_ENABLE | PLXE_MASK_BYTE_READ_START);
	PEX_8111Write(info, EECTL, eeCtl); /* start reading */
	
	for (i=0;i<1000;i++){
		PEX_8111Read(info, EECTL, &eeCtl);
		if ((eeCtl & PLXE_MASK_BYTE_READ_START) == 0){
			break;
		}
	}
	if ((eeCtl & PLXE_MASK_BYTE_READ_START) != 0){
		printf("ERROR: EEPROM is still reading byte (busy)!\n");
		return -EBUSY;
	}

	EE_WaitIdle(info); /* wait until read is done */
	PEX_8111Read(info, EECTL, &eeCtl);
	*data = (eeCtl >> PLXE_SHIFT_READ_DATA) & 0xFF;
	return 0;
}

///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

int EE_WriteByte(void *info, unsigned char val)
{
	int eeCtl,i;	
	
	if (EE_WaitIdle(info) & PLXE_MASK_BUSY){
		return -EINVAL;
	}
	
	/* clear current WRITE value */
	eeCtl = (PLXE_MASK_CS_ENABLE | PLXE_MASK_BYTE_WRITE_START);
	eeCtl |= ((val & 0xff) << PLXE_SHIFT_WRITE_DATA);
	PEX_8111Write(info, EECTL, eeCtl);
	
	for (i=0;i<1000;i++){
		PEX_8111Read(info, EECTL, &eeCtl);
		if ((eeCtl & PLXE_MASK_BYTE_WRITE_START) == 0){
			break;
		}
	}
	if (eeCtl & PLXE_MASK_BYTE_WRITE_START){
		printf("ERROR: EEPROM is still writting byte (busy)!\n");
		return -EBUSY;
	}
	return 0;
}
///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

//These are the high level functions

int wan_plxctrl_status(void *info, unsigned char *status)
{	

    	EE_WriteByte(info, PLXE_OPCODE_READ_STATUS); /* read status opcode */
	EE_ReadByte(info, status); /* get EEPROM status */
	EE_Off(info); /* turn off EEPROM */

	return 0;
}

int wan_plxctrl_write8(void *info, unsigned char addr, unsigned char data)
{
	EE_WriteByte(info, PLXE_OPCODE_WREN); /* must first write-enable */
	EE_Off(info); /* turn off EEPROM */
	EE_WriteByte(info, PLXE_OPCODE_WRITE); /* opcode to write bytes */

	/* Send low byte of address */
	EE_WriteByte(info, (unsigned char)(addr & 0xFF));
	
	EE_WriteByte(info, data & 0xFF); /* send data to be written */
	
	EE_Off(info); /* turn off EEPROM */
	usleep(10000);
	return 0;
}

///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

int wan_plxctrl_read8(void *info, unsigned char addr, unsigned char *data)
{	

	*data = 0x00;    
    	EE_WriteByte(info, PLXE_OPCODE_READ);
	EE_WriteByte(info, (unsigned char)(addr & 0xFF));

	EE_ReadByte(info, data);
    	EE_Off(info);
	return 0;
}

int wan_plxctrl_erase(void *info)
{	
	int t;

	for(t = 0; t < PLXE_SIZE; t++){
		wan_plxctrl_write8(info, t, 0xFF);
	}
	return 0;
}
