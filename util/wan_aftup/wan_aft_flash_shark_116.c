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

#define MASK_DEF_SECTOR_FLASH	0x00
#define MASK_USER_SECTOR_FLASH	0x08


/* Manufacturer code */
#define MCODE_ST	0x20

/* Device code */
#define DCODE_M29W040B	0xE3
#define DCODE_M29W800DT	0xD7
#define DCODE_M29W800DB	0x5B

#define M29W800DT_FID	0
#define M29W800DB_FID	1

struct {
	unsigned long	start_addr;
	unsigned long	start_off;
	unsigned long	len;
	unsigned int	sector_type;
} aft_shark_flash_spec_116 [2][19] = 
	{
		{ 
			{ 0x00000, 0x00000, 0xFFFF, DEF_SECTOR_FLASH }, 
			{ 0x10000, 0x10000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x20000, 0x20000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x30000, 0x30000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x40000, 0x40000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x50000, 0x50000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x60000, 0x60000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x70000, 0x70000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x80000, 0x00000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0x90000, 0x10000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xA0000, 0x20000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xB0000, 0x30000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xC0000, 0x40000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xD0000, 0x50000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xE0000, 0x60000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xF0000, 0x70000, 0x7FFF, USER_SECTOR_FLASH },
			{ 0xF8000, 0x78000, 0x1FFF, USER_SECTOR_FLASH },
			{ 0xFA000, 0x7A000, 0x1FFF, USER_SECTOR_FLASH },
			{ 0xFC000, 0x7C000, 0x3FFF, USER_SECTOR_FLASH },
		},

		{
			{ 0x00000, 0x00000, 0x3FFF, DEF_SECTOR_FLASH },
			{ 0x04000, 0x40000, 0x1FFF, DEF_SECTOR_FLASH },
			{ 0x06000, 0x60000, 0x1FFF, DEF_SECTOR_FLASH },
			{ 0x08000, 0x80000, 0x7FFF, DEF_SECTOR_FLASH },
			{ 0x10000, 0x10000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x20000, 0x20000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x30000, 0x30000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x40000, 0x40000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x50000, 0x50000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x60000, 0x60000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x70000, 0x70000, 0xFFFF, DEF_SECTOR_FLASH },
			{ 0x80000, 0x00000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0x90000, 0x10000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xA0000, 0x20000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xB0000, 0x30000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xC0000, 0x40000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xD0000, 0x50000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xE0000, 0x60000, 0xFFFF, USER_SECTOR_FLASH },
			{ 0xF0000, 0x70000, 0xFFFF, USER_SECTOR_FLASH },
		}
	};

aftup_flash_t	aft_shark_flash_116 = {  0x014F, AFT_CORE_X1600_SIZE };

extern int	verbose;

extern int	progress_bar(char*,int,int);
/*
 ******************************************************************************
			  FUNCTION PROTOTYPES
 ******************************************************************************
*/
		
extern int exec_read_cmd(void*,unsigned int, unsigned int, unsigned int*);
extern int exec_write_cmd(void*,unsigned int, unsigned int, unsigned int);
extern void hit_any_key(void);

static int aft_reset_flash_shark(wan_aft_cpld_t *cpld);
static int aft_is_protected_shark(wan_aft_cpld_t *cpld, int stype);
static int aft_flash_id_shark(wan_aft_cpld_t *cpld, int mtype, int stype, int *flash_id);
static int aft_reload_flash_shark(wan_aft_cpld_t *cpld, int sector_type);
static int aft_write_flash_shark(wan_aft_cpld_t*,int, unsigned long,unsigned char*);
static int aft_read_flash_shark(wan_aft_cpld_t*,int,int,unsigned long, unsigned char**);
static unsigned char aft_read_flash_byte_shark(wan_aft_cpld_t*,int,int,unsigned long);
static int aft_erase_flash_shark(wan_aft_cpld_t*,int,int);

aftup_flash_iface_t aftup_shark_flash_116_iface = 
{
	aft_reset_flash_shark,
	aft_is_protected_shark,
	aft_flash_id_shark,
	aft_reload_flash_shark,
	aft_write_flash_shark,
	aft_read_flash_shark,
	aft_erase_flash_shark
};

/******************************************************************************
*			  FUNCTION DEFINITION	
******************************************************************************/
static unsigned short get_cpld_off(int adptr_type, unsigned short cpld_off)
{

	switch(adptr_type){
	case A300_ADPTR_U_1TE3:
		cpld_off &= ~AFT_BIT_DEV_ADDR_CLEAR; 
		cpld_off |= AFT_BIT_DEV_ADDR_CPLD; 
		break;
	case AFT_ADPTR_56K:
		cpld_off &= ~AFT8_BIT_DEV_ADDR_CLEAR; 
		cpld_off |= AFT8_BIT_DEV_ADDR_CPLD; 
		break;
	default:
		cpld_off |= AFT4_BIT_DEV_ADDR_CPLD; 
		break;
	}
	return cpld_off;

}
static unsigned int
write_cpld(wan_aft_cpld_t *cpld, unsigned short cpld_off,unsigned short cpld_data)
{
	cpld_off = get_cpld_off(cpld->adptr_type, cpld_off);
	exec_write_cmd(cpld->private, 0x46, 2, cpld_off);
	exec_write_cmd(cpld->private, 0x44, 2, cpld_data);
	return 0;
}

static unsigned int
read_cpld(wan_aft_cpld_t *cpld, unsigned short cpld_off)
{
	unsigned int cpld_data;
		
	cpld_off = get_cpld_off(cpld->adptr_type, cpld_off);
	exec_write_cmd(cpld->private, 0x46, 2, cpld_off);
	if (exec_read_cmd(cpld->private, 0x44, 4, &cpld_data) == 0){
		return cpld_data;
	}else{
		return 0;
	}
}

static unsigned int
__aft_write_flash_shark_byte(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off,unsigned char data)
{
	unsigned char	offset;

	//Writing flash address to cpld
	offset = off & 0xFF;
	write_cpld(cpld, 0x05, offset);
	offset = (off >> 8) & 0xFF;
	write_cpld(cpld, 0x06, offset);
	offset = (off >> 16) & 0xF;

	write_cpld(cpld, 0x07, offset);
	write_cpld(cpld, 0x04, data);
        write_cpld(cpld, 0x07, 0x00);  // disable CS signal for the Boot FLASH/SRAM

	return 0;
}

static unsigned int
aft_write_flash_shark_byte(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off,unsigned char data)
{
	unsigned long	sec_off = 0x00;

	return __aft_write_flash_shark_byte(cpld, stype, mtype, sec_off + off, data);
}

static unsigned char
__aft_read_flash_byte_shark(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off)
{
	unsigned char offset;
        unsigned char data;

	//Writing flash address to cpld
	offset = off & 0xFF;
	write_cpld(cpld, 0x05, offset);
	offset = (off >> 8) & 0xFF;
	write_cpld(cpld, 0x06, offset);
	offset = (off >> 16) & 0xF;
        offset |= MASK_MEMORY_TYPE_FLASH;

	write_cpld(cpld, 0x07, offset);
        data = read_cpld(cpld, 0x04);
        write_cpld(cpld, 0x07, 0x00); // Disable CS for the Boot FLASH/SRAM
	return data;
}

static unsigned char
aft_read_flash_byte_shark(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off)
{
	unsigned long	sec_off = 0x00;

	return __aft_read_flash_byte_shark(cpld, stype, mtype, sec_off + off);
}

static int
aft_erase_flash_shark(wan_aft_cpld_t *cpld, int stype, int verify)
{
	unsigned long offset = 0x00;
	unsigned char data = 0x00;
	int sector_no = 0;

	for(sector_no = 0; sector_no < 19; sector_no++){
		offset = aft_shark_flash_spec_116[cpld->flash_index][sector_no].start_addr;
		__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0xAAA, 0xAA);
		__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x555, 0x55);
		__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0xAAA, 0x80);
		__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0xAAA, 0xAA);
		__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x555, 0x55);
		aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, offset, 0x30);

		do{
			data = aft_read_flash_byte_shark(
					cpld, stype,
					MEMORY_TYPE_FLASH,
					offset);
			if (data & 0x80){
				break;
			}else if (data & 0x20){
				data = aft_read_flash_byte_shark(
						cpld, stype, 
						MEMORY_TYPE_FLASH, 
						offset);
				if (data & 0x80){
					break;
				}else{
					printf("%s: Failed!\n", __FUNCTION__);
					printf("%s: Sector=%d!\n",
						__FUNCTION__,
						(stype == USER_SECTOR_FLASH) ? 
								sector_no+4:sector_no);
					return -EINVAL;
				}
			}
		} while(1);

		progress_bar("\tErasing sectors\t\t\t\t", 
					sector_no, 19);
	}
	printf("\r\tErasing sectors\t\t\t\tPassed\n");
	if (!verify) return 0;
	
	// Verify that flash is 0xFF
	for(offset = 0; offset < 0x100000; offset++){
	//	for(i=0;i<1000;i++);
		data = aft_read_flash_byte_shark(cpld, stype, MEMORY_TYPE_FLASH, offset);	
		if (data != 0xFF){
			printf(" Failed to compare! %05lx -> %02x \n",
						offset,data);
			return -EINVAL;
		}
		if ((offset & 0x3FFF) == 0x2000){
			progress_bar("\tErasing sectors (verification)\t\t",
							offset, 0x100000);
		}
	}
	printf("\r\tErasing sectors (verification)\t\tPassed\n");
	return 0;
}

static int
aft_write_flash_shark(wan_aft_cpld_t *cpld, int stype, unsigned long off32, unsigned char* pdata)
{
	unsigned char data;
	unsigned char data1 = 0x00;
	int num_bytes = 1;
	int i;
	
	data = *pdata;
	
	__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0xAAA, 0xAA);
	__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0x555, 0x55);
	__aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, 0xAAA, 0xA0);
	aft_write_flash_shark_byte(cpld, stype, MEMORY_TYPE_FLASH, off32, data);

	do{
		for(i=0;i<1000;i++);
		data1 = aft_read_flash_byte_shark(cpld, stype, MEMORY_TYPE_FLASH, off32);
		if ((data1 & 0x80) == (data & 0x80)){
			break;
		}else if (data1 & 0x20){
			data1 = aft_read_flash_byte_shark(
					cpld, stype, 
					MEMORY_TYPE_FLASH, 
					off32);
			if ((data1 & 0x80) == (data & 0x80)){
				break;
			}else{
				printf("prg_flash_byte: Failed Addr=(%lX)!\n",
							off32);
				return -EINVAL;
			}
		}
	} while(1);
	return num_bytes;
}


static int
aft_read_flash_shark(wan_aft_cpld_t *cpld, int stype, int mtype, unsigned long off, unsigned char** ppdata)
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

	*pdata =  __aft_read_flash_byte_shark(cpld, stype, mtype, sec_off + off);
	*ppdata = pdata;
	return num_bytes;

}


static int aft_reset_flash_shark(wan_aft_cpld_t *cpld)
{

	__aft_write_flash_shark_byte(cpld, DEF_SECTOR_FLASH, MEMORY_TYPE_FLASH, 0x00, 0xF0);
	return 0;
}

static int aft_is_protected_shark(wan_aft_cpld_t *cpld, int stype)
{
	return 0;
}

static int aft_flash_id_shark(wan_aft_cpld_t *cpld, int mtype, int stype, int *flash_id)
{
	unsigned char	man_code, device_code;

	aft_reset_flash_shark(cpld);
	__aft_write_flash_shark_byte(cpld, stype, mtype, 0xAAA, 0xAA);
	__aft_write_flash_shark_byte(cpld, stype, mtype, 0x555, 0x55);
	__aft_write_flash_shark_byte(cpld, stype, mtype, 0xAAA, 0x90);

	man_code = aft_read_flash_byte_shark(cpld, stype, mtype, 0x00);
	if (man_code != MCODE_ST){
		printf("The current shark flash is not supported (man id %02X)!\n",
				man_code);
		return -EINVAL;
	}
	*flash_id = man_code << 8;

	device_code = aft_read_flash_byte_shark(cpld, stype, mtype, 0x02);
	switch(device_code){
	case DCODE_M29W800DT:
		cpld->flash_index = M29W800DT_FID;
		break;
	case DCODE_M29W800DB:
		cpld->flash_index = M29W800DB_FID;
		break;
	default:
		printf("The current flash is not supported (dev id %02X)!\n",
					device_code);
		return -EINVAL;
		break;
	}
	*flash_id |= device_code;
	aft_reset_flash_shark(cpld);
	return 0;
}

static int aft_reload_flash_shark(wan_aft_cpld_t *cpld, int sector_type)
{
	/* Reload new code in to Xilinx from
	** Flash Default sector */   
	write_cpld(cpld, 0x07,0x80);   
	return 0;
}

