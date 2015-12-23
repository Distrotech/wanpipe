
#include <stdlib.h>
#include <stdio.h>

#if !defined(__WINDOWS__)
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif

#if defined(__LINUX__)
# include <linux/if.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/wanpipe_defines.h>
# include <linux/sdlasfm.h>
# include <linux/wanpipe_cfg.h>
#elif defined(__WINDOWS__)
# include <windows.h>
# include <winioctl.h>
# include <conio.h>
# include <stddef.h>	/* offsetof() */
# include <string.h>

# include "wanpipe_includes.h"
# include "wanpipe_common.h"
# include "wanpipe_time.h"	/* wp_usleep() */
# include "sdlasfm.h"
# include "sdlapci.h"

#else
# include <net/if.h>
# include <wanpipe_defines.h>
# include <sdlasfm.h>
# include <wanpipe_cfg.h>
#endif

#include "wan_aft_prg.h"

#define JP8_VALUE               0x02
#define JP7_VALUE               0x01
#define SW0_VALUE               0x04
#define SW1_VALUE               0x08   

#define MASK_DEF_SECTOR_FLASH	0x00
#define MASK_USER_SECTOR_FLASH	0x04

#define WRITE_DEF_SECTOR_DSBL   0x01
#define FRONT_END_TYPE_MASK     0x38

#define AFT_BIT_DEV_ADDR_CLEAR	0x600
#define AFT_BIT_DEV_ADDR_CPLD	0x200

#define AFT4_BIT_DEV_ADDR_CLEAR	0x800 /* QUADR */
#define AFT4_BIT_DEV_ADDR_CPLD	0x800 /* QUADR */

#define BIT_A18_SECTOR_SA4_SA7	0x20
#define USER_SECTOR_START_ADDR	0x40000

extern int	verbose;
extern int	card_type;

/******************************************************************************
			  FUNCTION PROTOTYPES
******************************************************************************/

extern int	exec_read_cmd(void*,unsigned int, unsigned int, unsigned int*);
extern int	exec_write_cmd(void*,unsigned int, unsigned int, unsigned int);
extern void	hit_any_key(void);
extern int	progress_bar(char*,int,int);

static int aft_reset_flash(wan_aft_cpld_t *cpld);
static int aft_is_protected(wan_aft_cpld_t *cpld, int stype);
static int aft_flash_id(wan_aft_cpld_t *cpld, int mtype, int stype, int *flash_id);
static int aft_reload_flash(wan_aft_cpld_t *cpld, int sector_type);
static int aft_write_flash(wan_aft_cpld_t*,int,unsigned long,unsigned char*);
static int aft_read_flash(wan_aft_cpld_t*,int,int,unsigned long, unsigned char**);
static unsigned char aft_read_flash_byte(wan_aft_cpld_t*,int,int,unsigned long);
static int aft_erase_flash_sector(wan_aft_cpld_t*,int,int);

aftup_flash_iface_t aftup_flash_iface = 
{
	aft_reset_flash,
	aft_is_protected,
	aft_flash_id,
	aft_reload_flash,
	aft_write_flash,
	aft_read_flash,
	aft_erase_flash_sector
};

struct {
	unsigned long	start_addr;
	unsigned long	start_off;
	unsigned long	len;
	unsigned int	sector_type;
} aft_flash_spec [8] = 
	{
		{ 0x00000, 0x00000, 0xFFFF, DEF_SECTOR_FLASH }, 
		{ 0x10000, 0x10000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x20000, 0x20000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x30000, 0x30000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x40000, 0x00000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x50000, 0x10000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x60000, 0x20000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x70000, 0x30000, 0xFFFF, USER_SECTOR_FLASH },
	};

/******************************************************************************
*			  FUNCTION DEFINITION	
******************************************************************************/
int do_wait(int sec)
{
#if defined(__WINDOWS__)
	/* convert seconds to milliseconds */
	Sleep(sec*1000);
#else
	struct timeval	timeout;
	int		err;
	timeout.tv_sec = sec;
	timeout.tv_usec = 0;
	err=select(0,NULL, NULL, NULL, &timeout);
#endif
	return 0;
}

static unsigned int
write_cpld(wan_aft_cpld_t *cpld, unsigned short cpld_off,unsigned short cpld_data)
{
	if (cpld->adptr_type == A104_ADPTR_4TE1){
		cpld_off |= AFT4_BIT_DEV_ADDR_CPLD; 
	}else{
		cpld_off &= ~AFT_BIT_DEV_ADDR_CLEAR; 
		cpld_off |= AFT_BIT_DEV_ADDR_CPLD; 
	}

	exec_write_cmd(cpld->private, 0x46, 2, cpld_off);
	exec_write_cmd(cpld->private, 0x44, 2, cpld_data);
	return 0;
}

static unsigned int
read_cpld(wan_aft_cpld_t *cpld, unsigned short cpld_off)
{
	unsigned int cpld_data;
		
	if (cpld->adptr_type == A104_ADPTR_4TE1){
		cpld_off |= AFT4_BIT_DEV_ADDR_CPLD; 
	}else{
		cpld_off &= ~AFT_BIT_DEV_ADDR_CLEAR; 
		cpld_off |= AFT_BIT_DEV_ADDR_CPLD; 
	}

	exec_write_cmd(cpld->private, 0x46, 2, cpld_off);
	if (exec_read_cmd(cpld->private, 0x44, 4, &cpld_data) == 0){
		return cpld_data;
	}else{
		return 0;
	}
}

static unsigned int
__aft_write_flash_byte(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off,unsigned char data)
{
	unsigned char	offset;

	//Writing flash address to cpld
	offset = off & 0xFF;
	write_cpld(cpld, 0x05, offset);
	offset = (off >> 8) & 0xFF;
	write_cpld(cpld, 0x06, offset);
	offset = (off >> 16) & 0x3;
	if (mtype == MEMORY_TYPE_SRAM){
		offset |= MASK_MEMORY_TYPE_SRAM;
	}else if (mtype == MEMORY_TYPE_FLASH){
		offset |= MASK_MEMORY_TYPE_FLASH;
	}else{
		return -EINVAL;
	}
	if (stype == USER_SECTOR_FLASH){
		offset |= MASK_USER_SECTOR_FLASH;
	}else if (stype == DEF_SECTOR_FLASH){
		;
	}else{
		return -EINVAL;
	}
	write_cpld(cpld, 0x07, offset);
	write_cpld(cpld, 0x04, data);
    write_cpld(cpld, 0x07, 0x00);  // disable CS signal for the Boot FLASH/SRAM

	return 0;
}

static unsigned int
aft_write_flash_byte(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off,unsigned char data)
{
	unsigned long	sec_off = 0x00;

	if (stype == USER_SECTOR_FLASH){
		sec_off = USER_SECTOR_START_ADDR;
	}
	return __aft_write_flash_byte(cpld, stype, mtype, sec_off + off, data);
}

static unsigned char
__aft_read_flash_byte(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off)
{
	unsigned char offset;
        unsigned char data;

	//Writing flash address to cpld
	offset = off & 0xFF;
	write_cpld(cpld, 0x05, offset);
	offset = (off >> 8) & 0xFF;
	write_cpld(cpld, 0x06, offset);
	offset = (off >> 16) & 0x3;
        offset |= MASK_MEMORY_TYPE_FLASH;
//           
//	if (memory_type == MEMORY_TYPE_SRAM){
//		offset |= MASK_MEMORY_TYPE_SRAM;
//	}else if (memory_type == MEMORY_TYPE_FLASH){
//		offset |= MASK_MEMORY_TYPE_FLASH;
//	}else{
//		return -EINVAL;
//	}
	if (stype == USER_SECTOR_FLASH){
		offset |= MASK_USER_SECTOR_FLASH;
	}else if (stype == DEF_SECTOR_FLASH){
		;
	}else{
		return -EINVAL;
	}

	write_cpld(cpld, 0x07, offset);
        data = read_cpld(cpld, 0x04);
        write_cpld(cpld, 0x07, 0x00); // Disable CS for the Boot FLASH/SRAM
	return data;
}

static unsigned char
aft_read_flash_byte(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off)
{
	unsigned long	sec_off = 0x00;

	if (stype == USER_SECTOR_FLASH){
		sec_off = USER_SECTOR_START_ADDR;
       	}
	return __aft_read_flash_byte(cpld, stype, mtype, sec_off + off);
}

static int
aft_read_flash(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off, unsigned char** ppdata)
{
	int num_bytes = 1;
	unsigned char* pdata = NULL;
	unsigned long	sec_off = 0x00;
	
	pdata = malloc(num_bytes);
	if (pdata == NULL) {
		printf("Failed to allocate memory (%s:%d)\n", 
				__FUNCTION__,__LINE__);
		return -ENOMEM;
	}
	memset(pdata, 0, num_bytes);
	
	if (stype == USER_SECTOR_FLASH){
		sec_off = USER_SECTOR_START_ADDR;
       	}
	*pdata =  __aft_read_flash_byte(cpld, stype, mtype, sec_off + off);
	*ppdata = pdata;
	return num_bytes;

}

static int
aft_erase_flash_sector(wan_aft_cpld_t *cpld, int stype, int verify)
{
	unsigned long offset = 0x00;
	unsigned char data = 0x00;
	int sector_no = 0;
	int cnt=0;

	// Checking write enable to the Default Boot Flash Sector 
	if (aft_is_protected(cpld, stype)){
		printf("%s: Default sector protected (%s:%d)!\n",
				__FUNCTION__,__FILE__,__LINE__);
		return -EINVAL;
	}

	for(sector_no = 0; sector_no < 8; sector_no++){
		if (aft_flash_spec[sector_no].sector_type != stype){
			continue;
		}
		offset = aft_flash_spec[sector_no].start_off; 
		__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x5555, 0xAA);
		__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x2AAA, 0x55);
		__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x5555, 0x80);
		__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x5555, 0xAA);
		__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x2AAA, 0x55);
		aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, offset, 0x30);
		do{
//MF			for(i=0;i<100000;i++);
			wp_usleep(1);
			data = aft_read_flash_byte(
						cpld, stype,
						MEMORY_TYPE_FLASH,
						offset);
			if (data & 0x80){
				break;
			}else if (data & 0x20){
				data = aft_read_flash_byte(
						cpld, stype, 
						MEMORY_TYPE_FLASH, 
						offset);
				if (data & 0x80){
					break;
				}else{
					printf("erase_sector_flash: Failed!\n");
					printf("erase_sector_flash: Sector=%d!\n",
					   (stype == USER_SECTOR_FLASH) ? 
							sector_no+4:sector_no);
					return -EINVAL;
				}
			}
		} while(1);

		progress_bar("\tErasing sectors\t\t\t\t",
					sector_no, 8);
	}
	printf("\r\tErasing sectors\t\t\t\tPassed\n");
	if (!verify) return 0;

	// Verify that flash is 0xFF
	for(offset = 0; offset < 0x40000; offset++){
//MF		for(i=0;i<10000;i++);
		data = aft_read_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, offset);	
		if (data != 0xFF){
			printf(" Failed to compare! %05lx -> %02x \n",
						offset+cnt,data);
			return -EINVAL;
		}
		if ((offset & 0x1FFF) == 0x1000){
			progress_bar("\tErasing sectors (verification)\t\t",
						offset, 0x40000);
		}
	}
	printf("\r\tErasing sectors (verification)\t\tPassed\n");
	return 0;
}

static int
aft_write_flash(	wan_aft_cpld_t	*cpld,
			int		stype,
			unsigned long	off32,
			unsigned char*	pdata)
{
	unsigned char	data;
	unsigned char	data1 = 0x00;
	int num_bytes = 1;

	data = *pdata;

	// Checking write enable to the Default Boot Flash Sector 
	if (aft_is_protected(cpld, stype)){
		printf("%s: Default sector protected (%s:%d)!\n",
				__FUNCTION__,__FILE__,__LINE__);
		return -EINVAL;
	}
	__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x5555, 0xAA);
	__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x2AAA, 0x55);
	__aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x5555, 0xA0);
	aft_write_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, off32, data);
	do{
#if 0
		for(i=0;i<100000;i++);
#endif
		data1 = aft_read_flash_byte(cpld, stype, MEMORY_TYPE_FLASH, off32);
		if ((data1 & 0x80) == (data & 0x80)){
			break;
		}else if (data1 & 0x20){
			data1 = aft_read_flash_byte(
					cpld, stype, 
					MEMORY_TYPE_FLASH, 
					off32);
			if ((data1 & 0x80) == (data & 0x80)){
				break;
			}else{
				printf("prg_flash_byte: Failed!\n");
				return -EINVAL;
			}
		}
	} while(1);
	return num_bytes;
}

static int aft_reset_flash(wan_aft_cpld_t *cpld)
{
	aft_write_flash_byte(cpld, DEF_SECTOR_FLASH, MEMORY_TYPE_FLASH, 0x00, 0xF0);
	return 0;
}

static int aft_is_protected(wan_aft_cpld_t *cpld, int stype)
{
	unsigned char	val;

	if(stype == DEF_SECTOR_FLASH){
		val = read_cpld(cpld, 0x09);
		if (val & WRITE_DEF_SECTOR_DSBL){
			return 1;
		}
	}    
	return 0;
}

static int aft_flash_id(wan_aft_cpld_t *cpld, int mtype, int stype, int *flash_id)
{
	unsigned char	man_code, device_code;

	__aft_write_flash_byte(cpld, stype, mtype, 0x5555, 0xAA);
	__aft_write_flash_byte(cpld, stype, mtype, 0x2AAA, 0x55);
	__aft_write_flash_byte(cpld, stype, mtype, 0x5555, 0x90);
	man_code = aft_read_flash_byte(cpld, stype, mtype, 0x00);
	*flash_id = man_code << 8;
	device_code = aft_read_flash_byte(cpld, stype, mtype, 0x01);
	*flash_id |= device_code;
	aft_reset_flash(cpld);
	if (*flash_id != 0x014F && *flash_id != 0x20E3){
		printf("The current flash is not supported (flash id %02X)!\n",
					*flash_id);
		return -EINVAL;
	}
	return 0;
}

static int aft_reload_flash(wan_aft_cpld_t *cpld, int sector_type)
{
	if (sector_type == USER_SECTOR_FLASH){
		/* Reload new code in to Xilinx from
                ** Flash User sector */  
		write_cpld(cpld, 0x07,0xC0); 
	}else{
		/* Reload new code in to Xilinx from
		** Flash Default sector */   
		write_cpld(cpld, 0x07,0x80);   
	}
	return 0;
}

