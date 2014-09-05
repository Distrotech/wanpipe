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
# include <stddef.h>		//for offsetof()
# include <string.h>

# include "wanpipe_includes.h"
# include "wanpipe_common.h"
# include "sdlasfm.h"
# include "sdlapci.h"

#else
# include <net/if.h>
# include <wanpipe_defines.h>
# include <sdlasfm.h>
# include <wanpipe_cfg.h>
#endif

#include "wan_aftup.h"
#include "wan_aft_prg.h"
#include "wan_aft_flash_a600.h"

aftup_flash_t	aft_a600_flash = {  0x014F, AFT_CORE_X250_SIZE };

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

static int aft_reset_flash_a600(wan_aft_cpld_t *cpld);
static int aft_is_protected_a600(wan_aft_cpld_t *cpld, int stype);
static int aft_flash_id_a600(wan_aft_cpld_t *cpld, int mtype, int stype, int *flash_id);
static int aft_reload_flash_a600(wan_aft_cpld_t *cpld, int unused);
static int aft_write_flash_a600(wan_aft_cpld_t*,int,unsigned long,u8*);
static int aft_read_flash_a600(wan_aft_cpld_t*,int,int,unsigned long,u8**);


static int aft_erase_flash_a600(wan_aft_cpld_t*,int,int);

int exec_sflash_instruction_cmd(wan_aft_cpld_t *cpld,
					int instruction, 
					u32 data_write, 
					u32 data_write_size, 
					u32 offset, 
					u32* data_read);

int exec_sflash_wait_write_done (wan_aft_cpld_t *cpld, int typical_time);
int exec_sflash_wait_ready (wan_aft_cpld_t *cpld);

void cp_data_from_bin(u8* dest, u8* src, size_t size) ;


aftup_flash_iface_t aftup_a600_flash_iface = 
{
	aft_reset_flash_a600,
	aft_is_protected_a600,
	aft_flash_id_a600,
	aft_reload_flash_a600,
	aft_write_flash_a600,
	aft_read_flash_a600,
	aft_erase_flash_a600
};


static int aft_reset_flash_a600(wan_aft_cpld_t *cpld)
{
	int err = 0;
	u32 reg_1040 = 0;

	exec_read_cmd(cpld->private, 0x1040, 4,	&reg_1040);

	if (reg_1040 & 0x06) {
		reg_1040 &=~0x06;

		err = exec_write_cmd(cpld->private, 
				0x1040, 
				4,
				reg_1040);
		if (err) {
			printf("Failed to reset card\n");
		}
	}
	return err;
}

static int aft_is_protected_a600(wan_aft_cpld_t *cpld, int stype)
{
	return 0;
}

static int aft_flash_id_a600(wan_aft_cpld_t *cpld, int unused_1, int unused_2, int *flash_id)
{
	int err;
	u32 data_read; 
	u32 tmp; 
	u8 mem_type;
	u8 man_id;
	u8 dev_id;

	data_read = 0x00;
	tmp = 0x00;
	err = 0;

	err = exec_sflash_instruction_cmd(cpld ,M25P40_COMMAND_RDID, 0, 0, 0, &data_read);
	if (err) {
		return err;
	}

	man_id = (data_read >> 16) & 0xFF;
	dev_id = (data_read) & 0xFF;
	mem_type = (data_read >> 8) & 0xFF;
	
	if (mem_type != M25P40_ID_MT) {
		printf ("Invalid flash memory type 0x%X\n", mem_type);
		return -1;
	}

	if (man_id != M25P40_ID_MF) {
		printf("Invalid flash Manufacturer ID 0x%X\n", man_id);
		return -1;
	}

	if (dev_id != M25P40_ID_MC) {
		printf("Invalid flash Device ID 0x%X\n", dev_id);
		return -1;
	}
		
	*flash_id = data_read & 0xFFFFFF;
	return 0;
}

static int aft_reload_flash_a600(wan_aft_cpld_t *cpld, int unused)
{
	unsigned int data_read = 0x00;
	u32 firm_version;
	firm_version = ((wan_aftup_t*) cpld->private)->flash_rev;

	exec_read_cmd(cpld->private, 0x1040, 4, &data_read); 

	if (firm_version >=3) {
		data_read |= (1 << 19);
	} else {
		data_read |= (1 << 18);
	}

	exec_write_cmd(cpld->private, 0x1040, 4, data_read);
	wp_usleep(3000000);
	return 0;
}

static int aft_erase_flash_a600(wan_aft_cpld_t *cpld, int stype, int verify)
{
	int err;

	err = exec_sflash_instruction_cmd(cpld, M25P40_COMMAND_WREN, 0, 0, 0, NULL);
	if (err) {
		return err;
	}
	err = exec_sflash_instruction_cmd(cpld, M25P40_COMMAND_BE, 0, 0, 0, NULL);
	if (err) {
		return err;
	}
	
	if (exec_sflash_wait_write_done(cpld, 4000000)){
		printf("Flash erase timeout \n");
		return -1;
	}
	return 0;	
}

static int
aft_read_flash_a600(wan_aft_cpld_t *cpld, int unused_1, int unused_2, unsigned long offset, u8** data)
{
	int num_bytes = 4;
	int err = 0;
	u8* tmp_data = NULL;
	u32 data_read = 0x00;


	tmp_data = malloc(num_bytes);
	if (tmp_data == NULL) {
		printf("Failed to allocate memory (%s:%d)\n", 
				__FUNCTION__,__LINE__);
		return -ENOMEM;
	}

	memset(data, 0, num_bytes);
	err = exec_sflash_instruction_cmd(cpld, M25P40_COMMAND_READ, 0, 0, offset, &data_read);
	if (err) {
		printf("Failed to send read EEPROM\n");
		return -EINVAL;
	}
	
	cp_data_from_bin(tmp_data, (u8*) &data_read, num_bytes);
	*data = tmp_data;
	return num_bytes;
}


static int
aft_write_flash_a600(wan_aft_cpld_t *cpld, int unused_1, unsigned long offset, u8* data)
{
	int num_bytes = 4;
	u32 data_write;
	int err;

	data_write = 0x00;

	cp_data_from_bin((u8*) &data_write, data, num_bytes);
	
	err = exec_sflash_instruction_cmd(cpld, M25P40_COMMAND_WREN, 0, 0, 0, NULL);
	if (err) {
		printf ("Failed to send write enable command to flash\n");
		return -EINVAL;
	}
	
	err = exec_sflash_instruction_cmd(cpld, M25P40_COMMAND_PP, data_write, num_bytes, offset, NULL);
	if (err) {
		printf ("Failed to send write page program command to flash\n");
		return -EINVAL;
	}

	
	err = exec_sflash_wait_write_done(cpld, 1500);
	if (err) {
		printf ("Flash program timeout\n");
		return -EINVAL;
	}

	return num_bytes;
}



int exec_sflash_instruction_cmd(wan_aft_cpld_t *cpld, int instruction, u32 data_write, u32 data_write_size, u32 offset, u32* data_read)
{
	unsigned int tmp_data = 0;
	unsigned int tmp_data_read = 0;

	int return_val = 0;
	int err = 0;

        if (offset > M25P40_FLASH_SIZE) {
                printf("Error: address too big, max:%x\n", M25P40_FLASH_SIZE);
		return_val = -1;
	}
	
	
	switch(instruction) {
		case M25P40_COMMAND_WREN:
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_WREN << 24;
			tmp_data &= 0xFF000000;

			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA WREN\n");
				return_val = -1;
				break;
			}
			break;
		case M25P40_COMMAND_WRDI:
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_WRDI << 24;
			tmp_data &= 0xFF000000;
			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA WRDI");
				return_val = -1;
				break;
			}
			break;
		case M25P40_COMMAND_BE:
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_BE << 24;
			tmp_data &= 0xFF000000;
			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA BE");
				return_val = -1;
				break;
			}
			break;
		case M25P40_COMMAND_RDSR:
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_RDSR << 24;
			tmp_data &= 0xFF000000;

			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA RDSR");
				return_val = -1;
				break;
			}
			
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_read_cmd(cpld->private, EEPROM_DATA_REG_IN, 4, &tmp_data_read); 
			if (err) {
				printf("FAILED TO READ DATA RDSR");
				return_val = -1;
				break;
			}
	
			memset(data_read, 0, sizeof(u32));
			memcpy(data_read, &tmp_data_read, sizeof(u32));
			break;
		case M25P40_COMMAND_WRSR:
			/* set data register */
			tmp_data = 0;
			
			tmp_data |= data_write << 24;
			tmp_data &= 0xFF000000;
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_write_cmd(cpld->private, EEPROM_DATA_REG_OUT, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA WRSR EEPROM_OUT");
				return_val = -1;
				break;
			}

			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_WRSR << 24;
			tmp_data &= 0xFF000000;
			
			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
 
			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA WRSR");
				return_val = -1;
				break;
			}
			break;
		case M25P40_COMMAND_READ:
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_READ << 24;
			tmp_data &= 0xFF000000;
                        
			/* set address */
			tmp_data |=offset;		

			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
   	 
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}

			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA READ");
				return_val = -1;
				break;
			}

			tmp_data_read = 0;
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			
			err = exec_read_cmd(cpld->private, EEPROM_DATA_REG_IN, 4, &tmp_data_read); 
			if (err) {
				printf("FAILED TO READ DATA READ");
				return_val = -1;
				break;
			}
			
			memset(data_read, 0, sizeof(u32));
			memcpy(data_read, &tmp_data_read, sizeof(u32));
			break;
		case M25P40_COMMAND_PP:
                        switch (data_write_size) {
				case 1:
					/* set data register */
					tmp_data = 0;
					tmp_data |= (data_write & 0xFF);

					if(exec_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					err = exec_write_cmd(cpld->private, (EEPROM_DATA_REG_OUT+3), 1, tmp_data);
					if (err) { 
						printf("FAILED TO WRITE DATA PP EEPROM_OUT");
						return_val = -1;
						break;
					}       
					
					/* set instruction byte */
					tmp_data = 0;
					tmp_data |= M25P40_COMMAND_PP << 24;
					tmp_data &= 0xFF000000;
          
					/* set up start condition */
					/* set bit 22 */
					tmp_data |= 0x00400000;
					tmp_data &= 0xFFFC0000;

					/* set address */
                                        tmp_data |=offset;

					err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
					if (err) {
						printf("FAILED TO WRITE DATA PP");
						return_val = -1;
						break;
					}

				       break;
				       /* endof 1 byte write */
				case 2:
					/* set data register */
					tmp_data = 0;
                                  
					tmp_data |= (data_write & 0xFFFF);
					
					if(exec_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					err = exec_write_cmd(cpld->private, (EEPROM_DATA_REG_OUT+2), 2, tmp_data);
					if (err) { 
						printf("FAILED TO WRITE DATA PP EEPROM_OUT");
						return_val = -1;
						break;
					}       

					/* set instruction byte */
					tmp_data = 0;
					tmp_data |= M25P40_COMMAND_PP << 24;
					tmp_data &= 0xFF000000;
          
					/* set up start condition */
					/* set bit 22 */
					tmp_data |= 0x00400000;

					tmp_data &= 0xFFFC0000;

					/* set address */
					tmp_data |=offset;

					err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
					if (err) {
						printf("FAILED TO WRITE DATA PP");
						return_val = -1;
					}
					tmp_data = 0x00;
					if(exec_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					err = exec_read_cmd(cpld->private, EEPROM_CNTRL_REG, 4, &tmp_data);
					break;
					/* endof 2 byte write */
				case 4:	
					/* set data register */
					tmp_data = 0;
					tmp_data |= data_write;
					
					if(exec_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					err = exec_write_cmd(cpld->private, EEPROM_DATA_REG_OUT, 4, tmp_data);
					if (err) {
						printf("FAILED TO WRITE DATA PP EEPROM_OUT");
						return_val = -1;
					}

					/* set instruction byte */
					tmp_data = 0;
					tmp_data |= M25P40_COMMAND_PP << 24;
					tmp_data &= 0xFF000000;
          
					/* set up start condition */
					/* set bit 22 */
					tmp_data |= 0x00400000;
	  
					/* set address */
					tmp_data |=offset;

					err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
					if (err) {
						printf("FAILED TO WRITE DATA PP");
						return_val = -1;
					}
					break;
					/* endof 4 byte write */
				default:
					printf("Invalid write size (%d)\n", data_write_size);
					return_val = -1;
					break;
			} /* switch write-size */ 
			break;
		case M25P40_COMMAND_RDID:
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_RDID << 24;
			tmp_data &= 0xFF000000;
			
			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
	
			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA RDID");
				exit(1);
			}

			if(exec_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}

			err = exec_read_cmd(cpld->private, EEPROM_DATA_REG_IN, 4, &tmp_data_read); 
			if (err) {
				printf("FAILED TO READ DATA RDID");
				return_val = -1;
				break;
			}
	
			memset(data_read, 0, sizeof(u32));
			memcpy(data_read, &tmp_data_read, sizeof(u32));
			break;
		default:
			printf("Error: instruction 0x%02X not implemented\n", instruction);
			break;
	}
   
	return return_val;
}

int exec_sflash_wait_ready (wan_aft_cpld_t *cpld)
{
	u32 data_read;
	int cnt = 0;
	
	data_read=0x00;
	exec_read_cmd(cpld->private, EEPROM_CNTRL_REG, 4, &data_read); 
	
	while (data_read & EEPROM_READY_BIT) {
		wp_usleep(5);
		exec_read_cmd(cpld->private, EEPROM_CNTRL_REG, 4, &data_read);
		if (cnt++ > 200000) {
			return -1;
		}
	}
	return 0;
}

int exec_sflash_wait_write_done (wan_aft_cpld_t *cpld, int typical_time)
{
	int err;
	u32 data_read = 0x00;
	int timeout = 0;
	
	err = exec_sflash_instruction_cmd(cpld, M25P40_COMMAND_RDSR, 0, 0, 0, &data_read);
	if (err) {
		return err;
	}
 	
	data_read = (data_read >> 8) & 0xFF;
		
	while (data_read & M25P40_SR_BIT_WIP) {
		wp_usleep(typical_time/5);
		
		err = exec_sflash_instruction_cmd(cpld, 
					M25P40_COMMAND_RDSR, 
					0, 0, 0, &data_read);
		if (err) {
			return err;
		}
	
		data_read = (data_read >> 8) & 0xFF;
		if (timeout++ > 50 ) {
			return -1;
		}
	}
	return 0;
}


void cp_data_from_bin(u8* dest, u8* src, size_t size) 
 {
	u8* tmp_src = NULL;
	u8* tmp_dest = NULL;
	int i;
	
	//tmp_src = src+3;
	tmp_src = src+(size-1);
	tmp_dest = dest;

	for (i=0; i < size; i++) {
		*tmp_dest = *tmp_src;
		 tmp_dest++;
		 tmp_src--;	
	}
}





