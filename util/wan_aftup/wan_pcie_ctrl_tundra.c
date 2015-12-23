
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
#include "wan_pcie_ctrl_tundra.h"


int aft_bridge_gen_image_tundra(pcie_eeprom_info_t* eeprom_info) ;
int aft_bridge_read_byte_tundra(void* aft, u8 off, u8 *val);
int aft_bridge_write_byte_tundra(void* aft, u8 off, u8 val);
int aft_bridge_reload_tundra(void* aft);

extern int exec_bridge_write_cmd(void *, unsigned int off, unsigned int len, unsigned int data);

extern int exec_bridge_read_cmd(void *, unsigned int off, unsigned int len, unsigned int *data);


pcie_bridge_iface_t aft_pci_bridge_iface_tundra = 
{
	aft_bridge_gen_image_tundra,
	aft_bridge_write_byte_tundra,
	aft_bridge_read_byte_tundra,
	aft_bridge_reload_tundra,
};


int exec_tundra_wait_eeprom_ready(void* aft)
{
	u32 data_read;
	int cnt = 0;
	
	data_read = 0x00;
	exec_bridge_read_cmd(aft, TUNDRA_EECTRL_REG_OFF, 4, &data_read);
	while (data_read & TUNDRA_EECTRL_BIT_BUSY) {
		wp_usleep(100);
		exec_bridge_read_cmd(aft, TUNDRA_EECTRL_REG_OFF, 4, &data_read);
		if(cnt++ > 1000) {
			return -1;
		}
	}
	return 0;
}


int aft_bridge_gen_image_tundra(pcie_eeprom_info_t* eeprom_info)
{
	int i;
	int current_image_index;
	
	if (((eeprom_info->num_regs*6)+4) > MAX_EEPROM_SIZE) {
		printf("Error: Number of registers to be written to exceed EEPROM size (regs:%d, eeprom size:%d)\n", eeprom_info->num_regs, MAX_EEPROM_SIZE);
		return -1;
	}
	
	/* set signatures */
	eeprom_info->image_vals[0].off = 0x00;
	eeprom_info->image_vals[0].value = 0xAB;
	
	eeprom_info->image_vals[1].off = 0x01;
	eeprom_info->image_vals[1].value = 0x28;
	
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
						((eeprom_info->reg_vals[i].off>>8) & 0xF) | 0xF0;
		
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

	printf("Tundra Tsi381 Image generated (%d bytes)\n",
				eeprom_info->num_eeprom_bytes);
	return 0;
}

int aft_bridge_read_byte_tundra(void* aft, u8 off, u8 *val)
{
	unsigned int cmd;
	unsigned int data_read;
		
	int err;
	
	cmd = 0x00;
	data_read = 0x00;
	*val = 0x00;
	
	if(exec_tundra_wait_eeprom_ready(aft)) {
		printf("PCIe Bridge EEPROM read timeout\n");
		return 1;
	}
	
	/* set addr width to 9-bit */
	cmd |= TUNDRA_EECTRL_BIT_ADD_WIDTH;
	cmd |= TUNDRA_EECTRL_BIT_CMD_READ;
	cmd |= TUNDRA_EECTRL_ADD_BIT_SHIFT(off);
	
	err = exec_bridge_write_cmd(aft, TUNDRA_EECTRL_REG_OFF, 4, cmd);
	
	cmd |= TUNDRA_EECTRL_BIT_CMD_VLD;
	err = exec_bridge_write_cmd(aft, TUNDRA_EECTRL_REG_OFF, 4, cmd);

	if(exec_tundra_wait_eeprom_ready(aft)) {
		printf("PCIe Bridge EEPROM read timeout\n");
		return 1;
	}

	err = exec_bridge_read_cmd(aft, TUNDRA_EECTRL_REG_OFF, 4, &data_read);

	*val = (data_read & 0xFF);
	return 0;
}

int aft_bridge_write_byte_tundra(void* aft, u8 off, u8 val)
{
	unsigned int cmd;
	int err;
	cmd = 0x00;
	
	if(exec_tundra_wait_eeprom_ready(aft)) {
		printf("PCIe Bridge EEPROM read timeout\n");
		return 1;
	}
	
	cmd |= TUNDRA_EECTRL_BIT_ADD_WIDTH;
	cmd |= TUNDRA_EECTRL_BIT_CMD_WRITE;
	cmd |= TUNDRA_EECTRL_ADD_BIT_SHIFT(off);
	
	cmd |= val;
	
	cmd |= TUNDRA_EECTRL_BIT_CMD_VLD;

	err = exec_bridge_write_cmd(aft, TUNDRA_EECTRL_REG_OFF, 4, cmd);
	if(exec_tundra_wait_eeprom_ready(aft)) {
		printf("PCIe Bridge EEPROM write timeout\n");
		return 1;
	}
	return 0;
}

int aft_bridge_reload_tundra(void* aft)
{
	return 0;
}



