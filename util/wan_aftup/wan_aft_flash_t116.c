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
#include "wan_aft_flash_t116.h"

aftup_flash_t	aft_t116_flash = {  0x014F, AFT_CORE_X250_SIZE };

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

static int aft_reset_flash_t116(wan_aft_cpld_t *cpld);
static int aft_is_protected_t116(wan_aft_cpld_t *cpld, int stype);
static int aft_flash_id_t116(wan_aft_cpld_t *cpld, int mtype, int stype, int *flash_id);
static int aft_reload_flash_t116(wan_aft_cpld_t *cpld, int unused);
static int aft_write_flash_t116(wan_aft_cpld_t*,int,unsigned long,u8*);
static int aft_read_flash_t116(wan_aft_cpld_t*,int,int,unsigned long,u8**);
int check_cleared_fcr(wan_aft_cpld_t *cpld);


static int aft_erase_flash_t116(wan_aft_cpld_t*,int,int);

int exec_t116_sflash_instruction_cmd(wan_aft_cpld_t *cpld,
					int instruction, 
					u32 data_write, 
					u32 data_write_size, 
					u32 offset, 
					u32* data_read);

int exec_t116_sflash_wait_write_done (wan_aft_cpld_t *cpld, int typical_time);
int exec_t116_sflash_wait_ready (wan_aft_cpld_t *cpld);

void cp_t116_data_from_bin(u8* dest, u8* src, size_t size) ;


aftup_flash_iface_t aftup_t116_flash_iface = 
{
	aft_reset_flash_t116,
	aft_is_protected_t116,
	aft_flash_id_t116,
	aft_reload_flash_t116,
	aft_write_flash_t116,
	aft_read_flash_t116,
	aft_erase_flash_t116
};

int read_reg_ds26519_fpga (wan_aft_cpld_t *cpld, int addr){
	//int data= 0;
	u32 data= 0;
	addr = addr & 0X3FFF; //Clearing Bits 31-14
	addr = addr | 0x8000; //Setting 10 for bit 14-15
	exec_write_cmd(cpld->private,0x44,2,addr);
	exec_read_cmd(cpld->private,0x44, 4, &data);
	data = (data >> 16) & 0xFF; //Shifting data to lower bytes first then Clearing the unused bits
	//printf ("The value read = 0x%X\n",data);
	return data;
}

int write_reg_ds26519_fpga (wan_aft_cpld_t *cpld,int addr,int data){
	int shifted_data = 0;
	//int data_dummy;
	u32 data_dummy;
	int tmp;
	addr = addr & 0X3FFF; //Clearing Bits 31-14
	addr = addr | 0x8000; //Setting 10 for bit 14-15
	//printf("In write_reg_ds26519_fpga addr=0x%x,data = 0x%x\n",addr,data);
	//getchar();
	//getchar();
	exec_write_cmd(cpld->private,0x44,2,addr); //Set the address
	exec_read_cmd(cpld->private,0x44, 4, &data_dummy);
	shifted_data = (data << 16) & 0x00FF0000; //shifting the entered data to bits 16-23 + clearing the other bits
	exec_write_cmd(cpld->private,0x44, 4, shifted_data); //writing the data to the specified address
	//exec_write_cmd(cpld->private,0x44, 1, shifted_data); //writing the data to the specified address
	tmp = read_reg_ds26519_fpga(cpld,addr);
	if (tmp != data){
		//printf ("Written Data 0x%X is different from read data 0x%X on addr 0x%X\\n",data,tmp,addr);
		//return 1;
	}
	return 0;
}

int write_reg_ds26519_fpga2 (wan_aft_cpld_t *cpld,int addr,int data){
	int shifted_data = 0;
	//int data_dummy;
	u32 data_dummy;
	int tmp;
	addr = addr & 0X3FFF; //Clearing Bits 31-14
	addr = addr | 0x8000; //Setting 10 for bit 14-15
	exec_write_cmd(cpld->private,0x44,2,addr); //Set the address
	//getchar();
	//getchar();
	exec_read_cmd(cpld->private,0x44, 4, &data_dummy);
	shifted_data = (data << 16) & 0x00FF0000; //shifting the entered data to bits 16-23 + clearing the other bits
	exec_write_cmd(cpld->private,0x44, 1, shifted_data); //writing the data to the specified address
	//exec_write_cmd(cpld->private,0x44, 1, shifted_data); //writing the data to the specified address
	tmp = read_reg_ds26519_fpga(cpld,addr);
	if (tmp != data){
		//printf ("Written Data 0x%X is different from read data 0x%X on addr 0x%X\\n",data,tmp,addr);
		//return 1;
	}
	return 0;
}

int flash_chip_select_low(wan_aft_cpld_t *cpld){
	int write_val;

	write_val = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, 0x00);
	return write_val;
}

int enable_flash_write(wan_aft_cpld_t *cpld){
	int write_val;

	write_val = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, 0x01);
	return write_val;
}

int flash_chip_select_high(wan_aft_cpld_t *cpld){
	int write_val;

	write_val = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, 0x02);
	return write_val;
}

int read_flash_ctrl_reg(wan_aft_cpld_t *cpld){
	int read_val;

	read_val = read_reg_ds26519_fpga(cpld, FLASH_CTRL_REG);
	return read_val;
}
#if 1
int M25P40_clockout(wan_aft_cpld_t *cpld, int x){
	//printf("cpld = %p\n",cpld);
	write_reg_ds26519_fpga(cpld, FLASH_DATA_REG, x);
	enable_flash_write(cpld);
	check_cleared_fcr(cpld);

	return 0;
	//return read_reg_ds26519_fpga(cpld, FLASH_DATA_REG);
}
#endif

int M25P40_clockout2(wan_aft_cpld_t *cpld, int x){
	write_reg_ds26519_fpga2(cpld, FLASH_DATA_REG, x);
	enable_flash_write(cpld);
	check_cleared_fcr(cpld);

	return 0;
	//return read_reg_ds26519_fpga(cpld, FLASH_DATA_REG);
}
//Wait for Data Interface Reset to be cleared by HW in FPGA CONTROL/STATUS
int check_cleared_fcr(wan_aft_cpld_t *cpld){
	int loop = 1;
	int tmp;
	while (loop){
		tmp = read_flash_ctrl_reg(cpld);
		if ((tmp & 0x1) == 0){
			loop = 0;
		}
	}
	return loop;
}


static int aft_reset_flash_t116(wan_aft_cpld_t *cpld)
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

static int aft_is_protected_t116(wan_aft_cpld_t *cpld, int stype)
{
	return 0;
}

static int aft_flash_id_t116(wan_aft_cpld_t *cpld, int unused_1, int unused_2, int *flash_id)
{
	int err;
	u32 data_read; 
	u32 tmp; 
	u8 mem_type;
	u8 man_id;
	u8 dev_id;
	u32 data_dummy;

	data_read = 0x00;
	tmp = 0x00;
	err = 0;

	exec_write_cmd(cpld->private,0x44,2,0x4000); //Set the address
	exec_read_cmd(cpld->private,0x44, 4, &data_dummy);
	exec_write_cmd(cpld->private,0x44, 4, 0x00054000); //writing the data to the specified address
	wp_usleep(2000000);

	err = exec_t116_sflash_instruction_cmd(cpld ,M25P40_COMMAND_RDID, 0, 0, 0, &data_read);
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
	
	printf("mem_type=0x%x, man_id=0x%x, dev_id=0x%x\n",mem_type,man_id,dev_id);
	*flash_id = data_read & 0xFFFFFF;
	
	return 0;
}

static int aft_reload_flash_t116(wan_aft_cpld_t *cpld, int unused)
{
	//getchar();
#if 1
	exec_write_cmd(cpld->private,0x44,2,0x4000);
	exec_write_cmd(cpld->private,0x44,4,0x00004000);
	//getchar();
	wp_usleep(10000);
	exec_write_cmd(cpld->private,0x44,2,0x4000);
	exec_write_cmd(cpld->private,0x44,4,0x00054000);
#endif
	return 0;
}

static int aft_erase_flash_t116(wan_aft_cpld_t *cpld, int stype, int verify)
{
	int err;

	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_WREN, 0, 0, 0, NULL);
	if (err) {
		return err;
	}
	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_BE, 0, 0, 0, NULL);
	if (err) {
		return err;
	}
	
	if (exec_t116_sflash_wait_write_done(cpld, 4000000)){
		printf("Flash erase timeout \n");
		return -1;
	}
	return 0;	
}

static int
aft_read_flash_t116(wan_aft_cpld_t *cpld, int unused_1, int unused_2, unsigned long offset, u8** data)
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
	//printf("offset = 0x%lx\n",offset);
	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_READ, 0, 0, offset, &data_read);
	if (err) {
		printf("Failed to send read EEPROM\n");
		return -EINVAL;
	}
	err = exec_t116_sflash_wait_write_done(cpld, 1500);
	if (err) {
		printf ("Flash program timeout\n");
		return -EINVAL;
	}
	//

	cp_t116_data_from_bin(tmp_data, (u8*) &data_read, num_bytes); 
	*data = tmp_data;
	return num_bytes;
}


static int
aft_write_flash_t116(wan_aft_cpld_t *cpld, int unused_1, unsigned long offset, u8* data)
{
	int num_bytes = 4;
	u32 data_write;
	int err;

	data_write = 0x00;

	cp_t116_data_from_bin((u8*) &data_write, data, num_bytes);
#if 0	
	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_WREN, 0, 0, 0, NULL);
	if (err) {
		printf ("Failed to send write enable command to flash\n");
		return -EINVAL;
	}

	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_SE, data_write, num_bytes, offset, NULL);
#endif

	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_WREN, 0, 0, 0, NULL);
	if (err) {
		printf ("Failed to send write enable command to flash\n");
		return -EINVAL;
	}

	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_PP, data_write, num_bytes, offset, NULL);
	if (err) {
		printf ("Failed to send write page program command to flash\n");
		return -EINVAL;
	}

	flash_chip_select_high(cpld);
#if 1	
	err = exec_t116_sflash_wait_write_done(cpld, 1500);
	if (err) {
		printf ("Flash program timeout\n");
		return -EINVAL;
	}
#endif
	return num_bytes;
}



int exec_t116_sflash_instruction_cmd(wan_aft_cpld_t *cpld, int instruction, u32 data_write, u32 data_write_size, u32 offset, u32* data_read)
{
	unsigned int tmp_data = 0;
	unsigned int tmp_data1 = 0;
	unsigned int tmp_data2 = 0;
	unsigned int tmp_data3 = 0;
	unsigned int tmp_data4 = 0;
	unsigned int tmp_data_read = 0;

	unsigned int tmp_offset1=0;
	unsigned int tmp_offset2=0;
	unsigned int tmp_offset3=0;

	int return_val = 0;
	int i;

    if (offset > M25P40_FLASH_SIZE) {
        printf("Error: address too big, max:%x\n", M25P40_FLASH_SIZE);
		return_val = -1;
	}
	
	
	switch(instruction) {
		case M25P40_COMMAND_WREN:
#if 1
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_WREN);
			flash_chip_select_high(cpld);
#endif		
#if 0
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_WREN << 24;
			tmp_data &= 0xFF000000;

			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			//if(exec_t116_sflash_wait_ready(cpld)) {
			if(check_cleared_fcr(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			//err = write_reg_ds26519_fpga(cpld, FLASH_DATA_REG, tmp_data);
			err = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA WREN\n");
				return_val = -1;
				break;
			}
#endif
			break;
		case M25P40_COMMAND_WRDI:
#if 1
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld,M25P40_COMMAND_WRDI);
			flash_chip_select_high(cpld);	
#endif
#if 0
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_WRDI << 24;
			tmp_data &= 0xFF000000;
			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			err =  write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);//exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA WRDI");
				return_val = -1;
				break;
			}
#endif
			break;
		case M25P40_COMMAND_BE:
#if 1
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_BE);
			flash_chip_select_high(cpld);
#endif
#if 0
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_BE << 24;
			tmp_data &= 0xFF000000;
			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			err =  write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA BE");
				return_val = -1;
				break;
			}
#endif
			break;
		case M25P40_COMMAND_RDSR:
#if 1
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_RDSR);

			//tmp_data_read = M25P40_clockout(cpld, FLASH_DATA_REG);
			
			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data_read = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);
			flash_chip_select_high(cpld);
#endif
#if 0
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_RDSR << 24;
			tmp_data &= 0xFF000000;

			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
    
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			err =  write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA RDSR");
				return_val = -1;
				break;
			}
			
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			//err = exec_read_cmd(cpld->private, EEPROM_DATA_REG_IN, 4, &tmp_data_read); 
			tmp_data_read =  read_reg_ds26519_fpga(cpld, FLASH_DATA_REG);
			if (err) {
				printf("FAILED TO READ DATA RDSR");
				return_val = -1;
				break;
			}
#endif	
			memset(data_read, 0, sizeof(u32));
			memcpy(data_read, &tmp_data_read, sizeof(u32));
			break;
		case M25P40_COMMAND_WRSR:
#if 1
			tmp_data = 0;
			
			//tmp_data |= data_write << 24;
			//tmp_data &= 0xFF000000;
			tmp_data = data_write;
			tmp_data &= 0x000000FF;

			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_WRSR);

			M25P40_clockout(cpld, tmp_data);

			flash_chip_select_high(cpld);
#endif
#if 0
			/* set data register */
			tmp_data = 0;
			
			tmp_data |= data_write << 24;
			tmp_data &= 0xFF000000;
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			//err = exec_write_cmd(cpld->private, EEPROM_DATA_REG_OUT, 4, tmp_data);
			err =  write_reg_ds26519_fpga(cpld, FLASH_DATA_REG, tmp_data);

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
 
			//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			err =  write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA WRSR");
				return_val = -1;
				break;
			}
#endif
			break;
		case M25P40_COMMAND_READ:
#if 1 
			tmp_data = 0;
			
			//tmp_data |= data_write << 8;
			//tmp_data &= 0xFFFFFF00;
			tmp_data = data_write;
			tmp_data &= 0x00FFFFFF;

			flash_chip_select_low(cpld);
			M25P40_clockout(cpld,M25P40_COMMAND_READ);
#if 0
			tmp_data = (offset >> 16) & 0xFF;
			M25P40_clockout(cpld,tmp_data);

			tmp_data = (offset >> 8) & 0xFF;
			M25P40_clockout(cpld,tmp_data);
#endif
			//tmp_data = offset & 0xFF;
			//M25P40_clockout2(cpld,tmp_data);

			//tmp_data = (offset >> 8) & 0xFF;
			//M25P40_clockout2(cpld,tmp_data);

			tmp_data = (offset >> 16) & 0xFF;
			M25P40_clockout(cpld,tmp_data);

			tmp_data = (offset >> 8) & 0xFF;
			M25P40_clockout(cpld,tmp_data);

			tmp_data = offset & 0xFF;
			M25P40_clockout(cpld,tmp_data);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data1 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data2 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data3 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);

			
			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data4 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);
#if 0
			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data5 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data6 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data7 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data8 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);
#endif

			//printf("tmp_data_read in M25P40_COMMAND_READ = 0x%x\n",tmp_data_read);
			//printf("tmp_data1=0x%x, tmp_data2=0x%x, tmp_data3=0x%x, tmp_data4=0x%x\n",tmp_data1,tmp_data2,tmp_data3,tmp_data4);
			//tmp_data_read = tmp_data_read | tmp_data1 << 24 | tmp_data2 << 16 | tmp_data3 << 8 | tmp_data4 << 0;
			//tmp_data_read = tmp_data1 << 24 | tmp_data2 << 16 | tmp_data3 << 8 | tmp_data4 << 0;
			tmp_data_read = tmp_data1 << 0 | tmp_data2 << 8 | tmp_data3 << 16 | tmp_data4 << 24;
			//tmp_data_read = M25P40_clockout(cpld, FLASH_DATA_REG);

#endif
#if 0
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_READ << 24;
			tmp_data &= 0xFF000000;
                        
			/* set address */
			tmp_data |=offset;		

			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
   	 
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}

			//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			err =  write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA READ");
				return_val = -1;
				break;
			}

			tmp_data_read = 0;
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			
			//err = exec_read_cmd(cpld->private, EEPROM_DATA_REG_IN, 4, &tmp_data_read); 
			err =  write_reg_ds26519_fpga(cpld, FLASH_DATA_REG, tmp_data);
			if (err) {
				printf("FAILED TO READ DATA READ");
				return_val = -1;
				break;
			}
#endif			
			memset(data_read, 0, sizeof(u32));
			memcpy(data_read, &tmp_data_read, sizeof(u32));
			break;
		case M25P40_COMMAND_SE:
			//////// SECTOR ERASE ////////
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_SE);

			tmp_offset3 = (offset >> 16) & 0xFF;
			M25P40_clockout(cpld,tmp_offset3);
			printf("ERASE offset=0x%x, tmp_offset3=0x%x\n",offset,tmp_offset3);

			tmp_offset2 = (offset >> 8) & 0xFF;
			M25P40_clockout(cpld,tmp_offset2);
			printf("ERASE offset=0x%x, tmp_offset2=0x%x\n",offset,tmp_offset2);

			tmp_offset1 = offset & 0xFF;
			M25P40_clockout(cpld,tmp_offset1);
			printf("ERASE offset=0x%x, tmp_offset1=0x%x\n",offset,tmp_offset1);

			flash_chip_select_high(cpld);

			break;
		case M25P40_COMMAND_PP:
#if 1
			tmp_data = 0;
#if 0
			//////// SECTOR ERASE ////////
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_SE);

			tmp_offset3 = (offset >> 16) & 0xFF;
			M25P40_clockout(cpld,tmp_offset3);
			printf("ERASE offset=0x%x, tmp_offset3=0x%x\n",offset,tmp_offset3);

			tmp_offset2 = (offset >> 8) & 0xFF;
			M25P40_clockout(cpld,tmp_offset2);
			printf("ERASE offset=0x%x, tmp_offset2=0x%x\n",offset,tmp_offset2);

			tmp_offset1 = offset & 0xFF;
			M25P40_clockout(cpld,tmp_offset1);
			printf("ERASE offset=0x%x, tmp_offset1=0x%x\n",offset,tmp_offset1);

			flash_chip_select_high(cpld);

			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_WREN);
			flash_chip_select_high(cpld);
			//////////////////////////////
#endif
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_WREN);
			flash_chip_select_high(cpld);

			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_PP);
			
			//for (i=0;i<data_write_size;i++){
                        switch (data_write_size) {
				case 1:
					tmp_data = data_write & 0xFF;
					printf("data_read=0x%x, tmp_data=0x%x\n",data_write,tmp_data);
					M25P40_clockout2(cpld,tmp_data);
					//data_write = data_wirite >> 8;
					break;
				case 2:
					tmp_data1 = data_write & 0x00FF;
					printf("data_read=0x%x, tmp_data1=0x%x\n",data_write,tmp_data1);
					M25P40_clockout2(cpld,tmp_data1);
					tmp_data2 = data_write & 0xFF00;
					tmp_data2 = tmp_data2 >> 8;
					printf("data_read=0x%x, tmp_data2=0x%x\n",data_write,tmp_data2);
					M25P40_clockout2(cpld,tmp_data2);
					break;
				case 4: 
					/*
					tmp_data1 = data_write & 0x00FF;
					printf("data_read=0x%x, tmp_data1=0x%x\n",data_write,tmp_data1);
					M25P40_clockout(cpld,offset);
					
					M25P40_clockout(cpld,tmp_data1);

					tmp_data2 = data_write & 0xFF00;
					tmp_data2 = tmp_data2 >> 8;
					printf("data_read=0x%x, tmp_data2=0x%x\n",data_write,tmp_data2);
					M25P40_clockout(cpld,tmp_data2);

					tmp_data3 = data_write & 0xFF0000;
					tmp_data3 = tmp_data3 >> 16;
					printf("data_read=0x%x, tmp_data3=0x%x\n",data_write,tmp_data3);
					M25P40_clockout(cpld,tmp_data3);
					*/

					//offset = 0;
					//data_write = 0xABCDEF10;

					tmp_offset3 = (offset >> 16) & 0xFF;
					M25P40_clockout(cpld,tmp_offset3);
					//printf("offset=0x%x, tmp_offset3=0x%x\n",offset,tmp_offset3);

					tmp_offset2 = (offset >> 8) & 0xFF;
					M25P40_clockout(cpld,tmp_offset2);
					//printf("offset=0x%x, tmp_offset2=0x%x\n",offset,tmp_offset2);

					tmp_offset1 = offset & 0xFF;
					M25P40_clockout(cpld,tmp_offset1);
					//printf("offset=0x%x, tmp_offset1=0x%x\n",offset,tmp_offset1);

					/*
					tmp_data1 = data_write & 0xFF000000;
					tmp_data1 = tmp_data1 >> 24;
					printf("data_read=0x%x, tmp_data4=0x%x\n",data_write,tmp_data1);
					M25P40_clockout(cpld,tmp_data1);

					tmp_data3 = data_write & 0xFF0000;
					tmp_data3 = tmp_data3 >> 16;
					printf("data_read=0x%x, tmp_data3=0x%x\n",data_write,tmp_data3);
					M25P40_clockout(cpld,tmp_data3);

					tmp_data2 = data_write & 0xFF00;
					tmp_data2 = tmp_data2 >> 8;
					printf("data_read=0x%x, tmp_data2=0x%x\n",data_write,tmp_data2);
					M25P40_clockout(cpld,tmp_data2);
					
					tmp_data1 = data_write & 0x00FF;
					printf("data_read=0x%x, tmp_data1=0x%x\n",data_write,tmp_data1);
					M25P40_clockout(cpld,tmp_data1);
					*/
#if 1
					//ORIGINAL
					for (i=0;i<4;i++){
						tmp_data = data_write & 0xFF;
						//printf("tmp_data entered 0x%X for byte %d\n",tmp_data, i);
						M25P40_clockout(cpld,tmp_data);
						data_write = data_write >> 8;
					}
#endif
#if 0		
					for (i=0;i<4;i++){
						tmp_data = (data_write >> 24) & 0xFF;
						printf("tmp_data entered 0x%X for byte %d\n",tmp_data, i);
						M25P40_clockout(cpld,tmp_data);
						data_write = data_write << 8;
					}
#endif
					flash_chip_select_high(cpld);
					/*
					tmp_data = data_write;
					printf("data_read=0x%x, tmp_data=0x%x\n",data_write,tmp_data);
					M25P40_clockout(cpld,tmp_data);
					M25P40_clockout(cpld,tmp_data);
					M25P40_clockout(cpld,tmp_data);
					*/
					break;
			}
			
			flash_chip_select_high(cpld);
#endif


#if 0
                        switch (data_write_size) {
				case 1:
					/* set data register */
					tmp_data = 0;
					tmp_data |= (data_write & 0xFF);

					if(exec_t116_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					//err = exec_write_cmd(cpld->private, (EEPROM_DATA_REG_OUT+3), 1, tmp_data);
					err = write_reg_ds26519_fpga(cpld, FLASH_DATA_REG, tmp_data);
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

					//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
					err = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
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
					
					if(exec_t116_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					//err = exec_write_cmd(cpld->private, (EEPROM_DATA_REG_OUT+2), 2, tmp_data);
					err = write_reg_ds26519_fpga(cpld, FLASH_DATA_REG, tmp_data);
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

					//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
					err = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
					if (err) {
						printf("FAILED TO WRITE DATA PP");
						return_val = -1;
					}
					tmp_data = 0x00;
					if(exec_t116_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					//err = exec_read_cmd(cpld->private, EEPROM_CNTRL_REG, 4, &tmp_data);
					err = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
					break;
					/* endof 2 byte write */
				case 4:	
					/* set data register */
					tmp_data = 0;
					tmp_data |= data_write;
					
					if(exec_t116_sflash_wait_ready(cpld)) {
						printf("ERROR: EEPROM timeout\n");
						return -1;
					}
					//err = exec_write_cmd(cpld->private, EEPROM_DATA_REG_OUT, 4, tmp_data);
					err = write_reg_ds26519_fpga(cpld, FLASH_DATA_REG, tmp_data);
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

					//err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
					err = write_reg_ds26519_fpga(cpld, FLASH_CTRL_REG, tmp_data);
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
#endif
			break;
		case M25P40_COMMAND_RDID:
#if 1
			flash_chip_select_low(cpld);
			M25P40_clockout(cpld, M25P40_COMMAND_RDID);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			
			tmp_data1 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);
			printf("Device Identification: Manufacture ID = 0x%X\n",tmp_data1);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data2 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);
			printf ("Device Identification: Memory Type= 0x%X\n",tmp_data2);

			enable_flash_write(cpld);
			check_cleared_fcr(cpld);
			tmp_data3 = read_reg_ds26519_fpga(cpld,FLASH_DATA_REG);
			printf ("Device Identification: Memory Capacity = 0x%X\n",tmp_data3);
			flash_chip_select_high(cpld);

			tmp_data = tmp_data | tmp_data1 << 16 | tmp_data2 << 8 | tmp_data3 << 0;
			printf ("Device Identification: Final = 0x%x\n",tmp_data);

#endif


#if 0
			/* set instruction byte */
			tmp_data = 0;
			tmp_data |= M25P40_COMMAND_RDID << 24;
			tmp_data &= 0xFF000000;
			
			/* set up start condition */
			/* set bit 22 */
			tmp_data |= 0x00400000;
	
			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}
			err = exec_write_cmd(cpld->private, EEPROM_CNTRL_REG, 4, tmp_data);
			if (err) {
				printf("FAILED TO WRITE DATA2RDID");
				exit(1);
			}

			if(exec_t116_sflash_wait_ready(cpld)) {
				printf("ERROR: EEPROM timeout\n");
				return -1;
			}

			err = exec_read_cmd(cpld->private, EEPROM_DATA_REG_IN, 4, &tmp_data_read); 
			if (err) {
				printf("FAILED TO READ DATA RDID");
				return_val = -1;
				break;
			}
	
#endif
			memset(data_read, 0, sizeof(u32));
			//memcpy(data_read, &tmp_data_read, sizeof(u32));
			memcpy(data_read, &tmp_data, sizeof(u32));
			break;
		default:
			printf("Error: instruction 0x%02X not implemented\n", instruction);
			break;
	}
   
	return return_val;
}

int exec_t116_sflash_wait_ready (wan_aft_cpld_t *cpld)
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

int exec_t116_sflash_wait_write_done (wan_aft_cpld_t *cpld, int typical_time)
{
	int err;
	u32 data_read = 0x00;
	int timeout = 0;
	
	err = exec_t116_sflash_instruction_cmd(cpld, M25P40_COMMAND_RDSR, 0, 0, 0, &data_read);
	if (err) {
		return err;
	}
 	
	//data_read = (data_read >> 8) & 0xFF;
		
	while (data_read & M25P40_SR_BIT_WIP) {
		//printf ("STILL BUSY\n\n");
		wp_usleep(typical_time/5);
		
		err = exec_t116_sflash_instruction_cmd(cpld, 
					M25P40_COMMAND_RDSR, 
					0, 0, 0, &data_read);
		if (err) {
			return err;
		}
	
		//data_read = (data_read >> 8) & 0xFF;
		if (timeout++ > 50 ) {
			return -1;
		}
	}
	return 0;
}


void cp_t116_data_from_bin(u8* dest, u8* src, size_t size) 
 {
	 //WE DO NOT NEED THIS FOR THE T116 Daughter card
	int i;
#if 0
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
#endif
	
	for (i=0; i < size; i++) {
		*dest = *src;
		 dest++;
		 src++;	
	}
}





