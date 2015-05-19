#ifndef __WAN_PCIE_CTRL_H
#define __WAN_PCIE_CTRL_H

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

#define MAX_EEPROM_SIZE 0xFF

typedef struct  
{
	u8  off;
	u8  value;
} pcie_eeprom_image_val;

typedef struct  
{
	u16  off;
	u32  value;
} pcie_reg_val;

typedef struct pcie_eeprom_info_
{
	u16 vendor_id;	/* Vendor ID for PCIE-Bridge */
	u16 device_id;	/* Device ID for PCIE-Bridge */
	
	u16 format;	/* EEPROM Address format */
	u16 version;	/* Version */

	/* Number of registers on PCIE-Bridge that will be configured */
	u32 num_regs;	
	
	/* Number of bytes that will be written to EEPROM */
	u32 num_eeprom_bytes;
	
	/* List of registers and values that will be updated on PCIE-Bridge */
	pcie_reg_val reg_vals[MAX_EEPROM_SIZE];
	
	/* List of address and values that will be written to on EEPROM */
	pcie_eeprom_image_val image_vals[MAX_EEPROM_SIZE];
} pcie_eeprom_info_t;


typedef struct pcie_bridge_iface_
{	
	int (*gen_eeprom_image) (pcie_eeprom_info_t* eeprom_info);
	int (*write_byte) (void* aft, u8 off, u8 val);
	int (*read_byte) (void* aft, u8 off, u8 *val);
	int (*reload) (void* aft);		
} pcie_bridge_iface_t;


int parse_pcie_reg_dat(pcie_eeprom_info_t* eeprom_info, char* fname);

#endif /*__WAN_PCIE_CTRL_H */
