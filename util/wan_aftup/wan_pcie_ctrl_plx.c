
#include <stdlib.h>
#include <stdio.h>

#if !defined(__WINDOWS__)
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

#if defined(__LINUX__)
# include <linux/if.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_common.h>
# include <linux/sdlapci.h>
# include <linux/sdlasfm.h>
# include <linux/if_wanpipe.h>

#elif defined(__WINDOWS__)
# include <windows.h>
# include <winioctl.h>
# include <conio.h>
# include <stddef.h>		//for offsetof()
# include <sdlasfm.h>
# include <sdlapci.h>

#else
# include <net/if.h>
# include <wanpipe_defines.h>
# include <wanpipe_common.h>
# include <sdlapci.h>
# include <sdlasfm.h>
# include <wanpipe.h>
#endif

#include "wan_aftup.h"
#include "wan_pcie_ctrl.h"
#include "wan_pcie_ctrl_plx.h"

int aft_bridge_gen_image_plx(pcie_eeprom_info_t* eeprom_info) ;
int aft_bridge_read_byte_plx(void* aft, u8 off, u8 *val);
int aft_bridge_write_byte_plx(void* aft, u8 off, u8 val);
int aft_bridge_reload_plx(void* aft);

extern int exec_bridge_write_cmd(void *, unsigned int off, unsigned int len, unsigned int data);
extern int exec_bridge_read_cmd(void *, unsigned int off, unsigned int len, unsigned int *data);


pcie_bridge_iface_t aft_pci_bridge_iface_plx = 
{
	aft_bridge_gen_image_plx,
	aft_bridge_write_byte_plx,
	aft_bridge_read_byte_plx,
	aft_bridge_reload_plx,
};

unsigned int aft_plx_read(void* aft, uint32_t reg)
{
	unsigned int	value = 0;
	
	
	if (reg < 0x1000){
		exec_bridge_read_cmd(aft, reg, 4, &value);
	}else if (reg >= 0x1000 && reg <= 0x1FFF){
		exec_bridge_write_cmd(aft, 0x84, 4, reg);
		exec_bridge_read_cmd(aft, 0x88, 4, &value);
	}
	return value;
}

int aft_plx_write(void* aft, unsigned short reg, uint32_t value)
{
	if (reg < 0x1000){
		exec_bridge_write_cmd(aft, reg, 4, value);
	}else if (reg >= 0x1000 && reg <= 0x1FFF){
		exec_bridge_write_cmd(aft, 0x84, 4, reg);
		exec_bridge_write_cmd(aft, 0x88, 4, value);
	}
	return 0;
}

void PEX_8111Read(void* aft, int addr, int *data)
{
	*data = aft_plx_read(aft, addr);
}


int EE_WaitIdle(void* aft)
{	
	int eeCtl, ii;
	for (ii = 0; ii < 100; ii++)
	{
	       	/* read current value in EECTL */
		PEX_8111Read(aft, EECTL, &eeCtl);
		/* loop until idle */
		if ((eeCtl & (1 << EEPROM_BUSY)) == 0)
			return(eeCtl);
		wp_usleep(100);
	}
	printf("ERROR: EEPROM Busy timeout!\n");
	return(eeCtl);
}

void PEX_8111Write(void* aft, int addr, int data)
{
	int	i,j;
	aft_plx_write(aft, addr, data);
	for(i=0;i<1000;i++)
		for(j=0;j<1000;j++);
}

void EE_Off(void* aft)
{	
	int t = 0;
	
	EE_WaitIdle(aft); /* make sure EEPROM is idle */
	PEX_8111Write(aft, EECTL, t); /* turn off everything (especially EEPROM_CS_ENABLE)*/
}

unsigned char EE_ReadByte(void* aft)
{
       	/* make sure EEPROM is idle */
	int eeCtl = EE_WaitIdle(aft);

	eeCtl |=	(1 << EEPROM_CS_ENABLE) |
	       		(1 << EEPROM_BYTE_READ_START);
	PEX_8111Write(aft, EECTL, eeCtl); /* start reading */
	eeCtl = EE_WaitIdle(aft); /* wait until read is done */
	
	/* extract read data from EECTL */
	return (unsigned char)((eeCtl >> EEPROM_READ_DATA) & 0xff);
}

void EE_WriteByte(void* aft, unsigned char val)
{
	int eeCtl = EE_WaitIdle(aft); /* make sure EEPROM is idle */	
	
	/* clear current WRITE value */
	eeCtl &= ~(0xff << EEPROM_WRITE_DATA);
	eeCtl |= 	(1 << EEPROM_CS_ENABLE) |
			(1 << EEPROM_BYTE_WRITE_START) |
			((val & 0xff) << EEPROM_WRITE_DATA);

	PEX_8111Write(aft, EECTL, eeCtl);
}


unsigned char read_eeprom_status(void* aft)
{	
	unsigned char status = 0;

    	EE_WriteByte(aft, READ_STATUS_EE_OPCODE); /* read status opcode */
	status = EE_ReadByte(aft); /* get EEPROM status */
	EE_Off(aft); /* turn off EEPROM */

	return status;
}





int aft_bridge_gen_image_plx(pcie_eeprom_info_t* eeprom_info)
{
	int i;
	int current_image_index;
	
	if (((eeprom_info->num_regs*6)+4) > MAX_EEPROM_SIZE) {
		printf("Error: Number of registers to be written to exceed EEPROM size (regs:%d, eeprom size:%d)\n", eeprom_info->num_regs, MAX_EEPROM_SIZE);
		return -1;
	}
	
	/* set signatures */
	eeprom_info->image_vals[0].off = 0x00;
	eeprom_info->image_vals[0].value = 0x5A;
	
	eeprom_info->image_vals[1].off = 0x01;
	eeprom_info->image_vals[1].value = 0x01; /* Format */
	
	/* Set size of eeprom */
	eeprom_info->image_vals[2].off = 0x02;
	eeprom_info->image_vals[2].value = (eeprom_info->num_regs*6) & 0xFF;
	
	eeprom_info->image_vals[3].off = 0x03;
	eeprom_info->image_vals[3].value = ((eeprom_info->num_regs*6) >> 8) & 0xFF;
	
	current_image_index = 4;
	
	for (i = 0; i < eeprom_info->num_regs; i++) {
		eeprom_info->image_vals[current_image_index].value = 
						(eeprom_info->reg_vals[i].off & 0xFF);
		eeprom_info->image_vals[current_image_index+1].value =
						((eeprom_info->reg_vals[i].off>>8) & 0xFF);
		
		eeprom_info->image_vals[current_image_index+2].value = 
						(eeprom_info->reg_vals[i].value & 0xFF);
		eeprom_info->image_vals[current_image_index+3].value =
						((eeprom_info->reg_vals[i].value >> 8) & 0xFF);
		eeprom_info->image_vals[current_image_index+4].value =
						((eeprom_info->reg_vals[i].value >> 16) & 0xFF);
		eeprom_info->image_vals[current_image_index+5].value =
						((eeprom_info->reg_vals[i].value >> 24) & 0xFF);
		

		eeprom_info->image_vals[current_image_index].off = current_image_index;
		eeprom_info->image_vals[current_image_index+1].off = current_image_index+1;
		eeprom_info->image_vals[current_image_index+2].off = current_image_index+2;
		eeprom_info->image_vals[current_image_index+3].off = current_image_index+3;
		eeprom_info->image_vals[current_image_index+4].off = current_image_index+4;
		eeprom_info->image_vals[current_image_index+5].off = current_image_index+5;
		
		current_image_index +=6;
	}

	/* These values do not affect the behavior of pcie bridge, they are only used for reference purposes */
	eeprom_info->image_vals[current_image_index].off   = 0xFC;	/* Sangoma vendor ID		*/
	eeprom_info->image_vals[current_image_index].value = 0x19;

	current_image_index++;
	eeprom_info->image_vals[current_image_index].off   = 0xFD;	/* Sangoma vendor ID		*/
	eeprom_info->image_vals[current_image_index].value = 0x23;

	current_image_index++;
	eeprom_info->image_vals[current_image_index].off   = 0xFE;	/* Sangoma PLX config verion	*/
	eeprom_info->image_vals[current_image_index].value = 0x00;

	current_image_index++;
	eeprom_info->image_vals[current_image_index].off   = 0xFF;	
	eeprom_info->image_vals[current_image_index].value = eeprom_info->version;	/* Sangoma PLX config verion	*/
	current_image_index++;

	eeprom_info->num_eeprom_bytes = current_image_index;
	printf("PLX PEX8111/2 Image generated (%d bytes)\n",
				eeprom_info->num_eeprom_bytes);
	return 0;
}

int aft_bridge_read_byte_plx(void* aft, u8 addr, u8 *val)
{
	int	i = 0, eeCtl = 0;
	unsigned char	data;
	
	/* busy verification */
#if 1
	eeCtl=EE_WaitIdle(aft);
#else
	PEX_8111Read(aft, EECTL, &eeCtl);
#endif
	if ((eeCtl & (1 << 19)) != 0){
	 	printf("ERROR: EEPROM is busy!\n");
		return 0xFF;
	}
	eeCtl = 0x00000000;
	eeCtl |= (1 << EEPROM_CS_ENABLE);
	PEX_8111Write(aft, EECTL, eeCtl);
		
	eeCtl &= ~(0xff << EEPROM_WRITE_DATA);
	eeCtl |= 	(1 << EEPROM_CS_ENABLE) |
		  	(1 << EEPROM_BYTE_WRITE_START) |
			(3 << EEPROM_WRITE_DATA);
	PEX_8111Write(aft, EECTL, eeCtl);
	for(i=0; i<1000;i++){
		PEX_8111Read(aft, EECTL, &eeCtl);
		if ((eeCtl & (1 << 16)) == 0){
			break;
		}
	}	
	if ((eeCtl & (1 << 16)) != 0){
	 	printf("ERROR: EEPROM timeout (write cmd)!\n");
		return 0xFF;
	}
	
	eeCtl = 0x00000000;
	eeCtl |= (1 << EEPROM_CS_ENABLE);
	PEX_8111Write(aft, EECTL, eeCtl);
		
	eeCtl &= ~(0xff << EEPROM_WRITE_DATA);
	eeCtl |= 	(1 << EEPROM_CS_ENABLE) |
		  	(1 << EEPROM_BYTE_WRITE_START) |
			(addr << EEPROM_WRITE_DATA);
	PEX_8111Write(aft, EECTL, eeCtl);
	for(i=0; i<1000;i++){
		PEX_8111Read(aft, EECTL, &eeCtl);
		if ((eeCtl & (1 << 16)) == 0){
			break;
		}
	}	
	if ((eeCtl & (1 << 16)) != 0){
	 	printf("ERROR: EEPROM timeout (addr)!\n");
		return 0xFF;
	}
	
	eeCtl = 0x00000000;
	eeCtl |= (1 << EEPROM_CS_ENABLE) |
		 (1 << EEPROM_BYTE_READ_START);
	PEX_8111Write(aft, EECTL, eeCtl);
	for(i=0; i<1000;i++){
		PEX_8111Read(aft, EECTL, &eeCtl);
		if ((eeCtl & (1 << 17)) == 0){
			break;
		}
	}	
	if ((eeCtl & (1 << 17)) != 0){
	 	printf("ERROR: EEPROM timeout (read)!\n");
		return 0xFF;
	}
	EE_WaitIdle(aft);
	PEX_8111Read(aft, EECTL, &eeCtl);
	data = (eeCtl >> 8) & 0xFF;
	EE_Off(aft); /* turn off EEPROM */

	*val = data;
	return 0;
}

int aft_bridge_write_byte_plx(void* aft, u8 addr, u8 data)
{
	int	i = 0, eeCtl = 0;
	
	eeCtl=EE_WaitIdle(aft);

	PEX_8111Read(aft, EECTL, &eeCtl);
	if ((eeCtl & (1 << 19)) != 0){
	 	printf("ERROR: EEPROM is busy %08X (init)!\n", eeCtl);
		return -1;
	}

	eeCtl = 0x00000000;
	eeCtl |= (1 << EEPROM_CS_ENABLE);
	PEX_8111Write(aft, EECTL, eeCtl);
		
	eeCtl &= ~(0xff << EEPROM_WRITE_DATA);
	eeCtl |= 	(1 << EEPROM_CS_ENABLE) |
			(1 << EEPROM_BYTE_WRITE_START) |
			(6 << EEPROM_WRITE_DATA);
	PEX_8111Write(aft, EECTL, eeCtl);
	
	for(i=0; i<1000;i++){
		PEX_8111Read(aft, EECTL, &eeCtl);
		if ((eeCtl & (1 << 16)) == 0){
			break;
		}
	}	
	if ((eeCtl & (1 << 16)) != 0){
	 	printf("ERROR: EEPROM timeout (wren cmd)!\n");
		return -1;
	}
	
	EE_Off(aft); /* turn off EEPROM */

	/* busy verification */
#if 1
	eeCtl=EE_WaitIdle(aft);
#else
	PEX_8111Read(aft, EECTL, &eeCtl);
#endif
	if ((eeCtl & (1 << 19)) != 0){
	 	printf("ERROR: EEPROM is busy!\n");
		return -1;
	}
	eeCtl = 0x00000000;
	eeCtl |= (1 << EEPROM_CS_ENABLE);
	PEX_8111Write(aft, EECTL, eeCtl);
		
	eeCtl &= ~(0xff << EEPROM_WRITE_DATA);
	eeCtl |= 	(1 << EEPROM_CS_ENABLE) |
		  	(1 << EEPROM_BYTE_WRITE_START) |
			(2 << EEPROM_WRITE_DATA);
	PEX_8111Write(aft, EECTL, eeCtl);
	for(i=0; i<1000;i++){
		PEX_8111Read(aft, EECTL, &eeCtl);
		if ((eeCtl & (1 << 16)) == 0){
			break;
		}
	}	
	if ((eeCtl & (1 << 16)) != 0){
	 	printf("ERROR: EEPROM timeout (write cmd)!\n");
		return -1;
	}
	
	eeCtl = 0x00000000;
	eeCtl |= (1 << EEPROM_CS_ENABLE);
	PEX_8111Write(aft, EECTL, eeCtl);
		
	eeCtl &= ~(0xff << EEPROM_WRITE_DATA);
	eeCtl |= 	(1 << EEPROM_CS_ENABLE) |
		  	(1 << EEPROM_BYTE_WRITE_START) |
			(addr << EEPROM_WRITE_DATA);
	PEX_8111Write(aft, EECTL, eeCtl);
	for(i=0; i<1000;i++){
		PEX_8111Read(aft, EECTL, &eeCtl);
		if ((eeCtl & (1 << 16)) == 0){
			break;
		}
	}	
	if ((eeCtl & (1 << 16)) != 0){
	 	printf("ERROR: EEPROM timeout (addr)!\n");
		return -1;
	}
	
	eeCtl = 0x00000000;
	eeCtl |= (1 << EEPROM_CS_ENABLE);
	PEX_8111Write(aft, EECTL, eeCtl);
		
	eeCtl &= ~(0xff << EEPROM_WRITE_DATA);
	eeCtl |= 	(1 << EEPROM_CS_ENABLE) |
		  	(1 << EEPROM_BYTE_WRITE_START) |
			(data << EEPROM_WRITE_DATA);
	PEX_8111Write(aft, EECTL, eeCtl);
	for(i=0; i<1000;i++){
		PEX_8111Read(aft, EECTL, &eeCtl);
		if ((eeCtl & (1 << 16)) == 0){
			break;
		}
	}	
	if ((eeCtl & (1 << 16)) != 0){
	 	printf("ERROR: EEPROM timeout (0x5A)!\n");
		return -1;
	}
	
	EE_Off(aft); /* turn off EEPROM */
	wp_usleep(100000);
	return 0;
}

int aft_bridge_reload_plx(void* aft)
{
	int	cnt = 0, eeCtl = 0;
	
	PEX_8111Read(aft, EECTL, &eeCtl);
	eeCtl |= BIT_EECTL_RELOAD;
	PEX_8111Write(aft, EECTL, eeCtl);
	
	do {
		PEX_8111Read(aft, EECTL, &eeCtl);
		cnt++;
	} while(!(eeCtl & BIT_EECTL_RELOAD) && (cnt < 100));
	
	return (!(eeCtl & BIT_EECTL_RELOAD));
}


