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
# include <linux/sdlapci.h>
#elif defined(__WINDOWS__)
# include <windows.h>
# include <winioctl.h>
# include <conio.h>
# include <stddef.h>		//for offsetof()
# include <sdlasfm.h>
# include <sdlapci.h>

# define strncasecmp	_strnicmp

#else
# include <net/if.h>
# include <wanpipe_defines.h>
# include <sdlasfm.h>
# include <wanpipe_cfg.h>
# include <sdlapci.h>
#endif

#include "wan_aft_prg.h"
#include "wanpipe_api.h"

#define JP8_VALUE               0x02
#define JP7_VALUE               0x01
#define SW0_VALUE               0x04
#define SW1_VALUE               0x08   

#define WRITE_DEF_SECTOR_DSBL   0x01
#define FRONT_END_TYPE_MASK     0x38

#define AFT_BIT_DEV_ADDR_CLEAR	0x600
#define AFT_BIT_DEV_ADDR_CPLD	0x200

#define AFT4_BIT_DEV_ADDR_CLEAR	0x800 /* QUADR */
#define AFT4_BIT_DEV_ADDR_CPLD	0x800 /* QUADR */

#define BIT_A18_SECTOR_SA4_SA7	0x20
#define USER_SECTOR_START_ADDR	0x40000

/* Manufacturer code */
#define MCODE_ST	0x20

/* Device code */
#define DCODE_M29W040B	0xE3
#define DCODE_M29W800DT	0xD7
#define DCODE_M29W800DB	0x5B

#define M29W040B_FID	0
#define M29W800DT_FID	1
#define M29W800DB_FID	2

typedef struct {
	unsigned long	saddr;
	unsigned long	len;
	unsigned int	sector_type;
} block_addr_t;

static block_addr_t 
block_addr_M29W040B[19] = 
	{
		{ 0x00000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x10000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x20000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x30000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x40000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x50000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x60000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x70000, 0xFFFF, USER_SECTOR_FLASH }
	};

static block_addr_t 
block_addr_M29W800DT[19] = 
	{
		{ 0x00000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x10000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x20000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x30000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x40000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x50000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x60000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x70000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x80000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x90000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xA0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xB0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xC0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xD0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xE0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xF0000, 0x7FFF, USER_SECTOR_FLASH },
		{ 0xF8000, 0x1FFF, USER_SECTOR_FLASH },
		{ 0xFA000, 0x1FFF, USER_SECTOR_FLASH },
		{ 0xFC000, 0x3FFF, USER_SECTOR_FLASH }
	};

static block_addr_t 
block_addr_M29W800DB[19] = 
	{
		{ 0x00000, 0x3FFF, DEF_SECTOR_FLASH },
		{ 0x04000, 0x1FFF, DEF_SECTOR_FLASH },
		{ 0x06000, 0x1FFF, DEF_SECTOR_FLASH },
		{ 0x08000, 0x7FFF, DEF_SECTOR_FLASH },
		{ 0x10000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x20000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x30000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x40000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x50000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x60000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x70000, 0xFFFF, DEF_SECTOR_FLASH },
		{ 0x80000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0x90000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xA0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xB0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xC0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xD0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xE0000, 0xFFFF, USER_SECTOR_FLASH },
		{ 0xF0000, 0xFFFF, USER_SECTOR_FLASH },
	};

#define FLASH_NAME_LEN	20

struct flash_spec_t {
	char		name[FLASH_NAME_LEN];
	unsigned short	data_reg;
	unsigned short	addr1_reg;
	unsigned char	addr1_mask;
	unsigned short	addr2_reg;
	unsigned char	addr2_mask;
	unsigned short	addr3_reg;
	unsigned char	addr3_mask;
	unsigned long	def_start_adr;
	unsigned long	user_start_adr;
	unsigned short	user_mask_sector_flash;
	int		sectors_no;
	block_addr_t	*block_addr;
} flash_spec[] = 
	{
		{ 
			"M29W040B",
			0x04, 0x05, 0xFF, 0x06, 0xFF, 0x07, 0x3,
			0x00000, 0x40000, 0x04, 
			8, block_addr_M29W040B
		},
		{ 
			"M29W800DT",
			0x04, 0x05, 0xFF, 0x06, 0xFF, 0x07, 0x7,
			0x00000, 0x80000, 0x08, 
			19, block_addr_M29W800DT
		},
		{
			"M29W800DB",
			0x04, 0x05, 0xFF, 0x06, 0xFF, 0x07, 0x7,
			0x00000, 0x80000, 0x08, 
			19, block_addr_M29W800DB
		}
	};

extern int	verbose;
extern int	card_type;

extern int	progress_bar(char*,int,int);

extern int exec_read_cmd(void*,unsigned int, unsigned int, unsigned int*);
extern int exec_write_cmd(void*,unsigned int, unsigned int, unsigned int);


int board_reset(wan_aft_cpld_t *cpld, int clear);
int update_flash(wan_aft_cpld_t *cpld, int stype, int mtype, char* filename);

/*
 ******************************************************************************
			  FUNCTION PROTOTYPES
 ******************************************************************************
*/

static unsigned long filesize(FILE* f)
{
	unsigned long size = 0;
	unsigned long cur_pos = 0;

	cur_pos = ftell(f);
	if ((cur_pos != -1l) && !fseek(f, 0, SEEK_END)){
		size = ftell(f);
		fseek(f, cur_pos, SEEK_SET);
	}
	return size;
}

#define UNKNOWN_FILE	0
#define HEX_FILE	1
#define BIN_FILE	2
static int get_file_type(char *filename)
{
	char	*ext;
	int	type = UNKNOWN_FILE;

	ext = strstr(filename, ".");
	if (ext != NULL){
		ext++;
		if (!strncasecmp(ext, "BIN", 3)){
			type = BIN_FILE;
		}else if (!strncasecmp(ext, "HEX", 3)){
			type = HEX_FILE;
		}else if (!strncasecmp(ext, "MCS", 3)){
			type = HEX_FILE;
		}else{
			type = BIN_FILE;
		}
	}else{
		/* By default, file format is Binary */
		type = BIN_FILE;
	}
	return type;

}
static long read_bin_data_file(char* filename, char** data)
{
	FILE		*f = NULL;
	char		*buf = NULL;
	unsigned long	fsize = 0;

	f = fopen(filename, "rb");
	if (f == NULL){
		printf("read_bin_data_file: Can't open data file %s!\n",
					filename);
		return -EINVAL;
	}
	fsize = filesize(f);
	if (!fsize){
		printf("read_bin_data_file: Data file %s is empty!\n",
					filename);
		return -EINVAL;
	}
	buf = malloc(fsize);
	if (buf == NULL){
		printf("read_bin_data_file: Can't allocate memory for data!\n");
		return -ENOMEM;
	}
	if (fread(buf, 1, fsize, f) < fsize){
		printf("read_bin_data_file: Can't copy all data to local buffer!\n");
		free(buf);
		return -EINVAL;
	}
	*data = buf;
	fclose(f);
	return fsize;
	
}

#define CMD_FLASH_UNKNOWN		0x00
#define CMD_FLASH_VERIFY		0x01
#define CMD_FLASH_PRG			0x02

#define MIN_HEX_LINE_LEN		11
#define MAX_HEX_LINE_LEN		200
#define MAX_HEXNUM_LEN			5

#define HEX_START_LINE			':'
#define HEX_DATA_RECORD			0x00
#define HEX_END_FILE_RECORD		0x01
#define HEX_EXT_SEG_ADDR_RECORD		0x02
#define HEX_START_SEG_ADDR_RECORD	0x03
#define HEX_EXT_LIN_ADDR_RECORD		0x04
#define HEX_START_LIN_ADDR_RECORD	0x05
static int parse_hex_line(char *line, int data[], int *addr, int *num, int *type)
{
	int	sum, len, cksum;
	char	*ptr = line;
	
	*num = 0;
	if (*ptr++ != HEX_START_LINE){
		printf("Wrong HEX file format!\n");
		return -EINVAL;
	}
	if (strlen(line) < MIN_HEX_LINE_LEN){
		printf("Wrong HEX line length (too small)!\n");
		return -EINVAL;
	}
	if (!sscanf(ptr, "%02x", &len)){
		printf("Failed to read record-length field\n");
	       	return -EINVAL;
	}
	ptr += 2;
	if (strlen(line) < (unsigned int)(MIN_HEX_LINE_LEN + (len * 2))){
		printf("Wrong HEX line length (too small)!\n");
	       	return -EINVAL;
	}
	if (!sscanf(ptr, "%04x", addr)){
		printf("Failed to read record-addr field\n");
	       	return -EINVAL;
	}
	ptr += 4;
	if (!sscanf(ptr, "%02x", type)){
		printf("Failed to read record-type field\n");
		return -EINVAL;
	}
	ptr += 2;
	sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*type & 255);
	while(*num != len){
		if (!sscanf(ptr, "%02x", &data[*num])){
			printf("Failed to read record-data field (%d)\n",
					*num);
		       	return -EINVAL;
		}
		ptr += 2;
		sum += data[*num] & 255;
		(*num)++;
		if (*num >= MAX_HEX_LINE_LEN){
			printf("Wrong HEX line length (too long)!\n");
		       	return -EINVAL;
		}
	}
	if (!sscanf(ptr, "%02x", &cksum)) return -EINVAL;
	if (((sum & 255) + (cksum & 255)) & 255){
	       	printf("Wrong hex checksum value!\n");
	       	return -EINVAL;
	}
	return 0;
}


static int
aft_flash_hex_file(wan_aft_cpld_t *cpld, int stype, char *filename, int cmd)
{
	FILE		*f;
	char		line[MAX_HEX_LINE_LEN];
	int		data[MAX_HEX_LINE_LEN];
	int		seg = 0, addr;
	int		len, type, i;
	unsigned char*	val;
	int 		page_size;
	
	if (cmd == CMD_FLASH_PRG){
		cpld->iface->erase(cpld, stype, 0);
		//erase_sector_flash(cpld, stype, 0);
	}
	f = fopen(filename, "r");
	if (f == NULL){
		printf("Can't open data file %s!\n",
					filename);
		return -EINVAL;
	}


	while(!feof(f)){
		if (!fgets(line, MAX_HEX_LINE_LEN, f)){
			if (feof(f)){
				break;
			}
			fclose(f);
			return -EINVAL;
		}
		if (parse_hex_line(line, data, &addr, &len, &type)){
			printf("Failed to parse HEX line!\n");
			fclose(f);
			return -EINVAL;
		}
		switch(type){
		case HEX_EXT_LIN_ADDR_RECORD:
			/* Save extended address */
			seg = data[0] << 24;
			seg |= data[1] << 16;
			continue;
		case HEX_EXT_SEG_ADDR_RECORD:
			seg = data[0] << 12;
			seg = data[1] << 4;
			continue;
		}
		i = 0;

		while(i < len){
			if (cmd == CMD_FLASH_PRG){
				page_size = cpld->iface->prg(cpld, stype, seg+addr+i, (u8*) &data[i]);
				if (page_size < 0){
					printf("\r\tUpdating flash\t\t\t\tFailed(%x)\n",
								addr+i);
					fclose(f);
					return -EINVAL;
				}
			}else{
				page_size = cpld->iface->read(cpld, stype, MEMORY_TYPE_FLASH, seg+addr+i, &val);

				if (memcmp(&data[i], val, page_size)) {
					printf("\r\tVerification\t\t\t\tFailed(%x)\n",
						addr+i);
					fclose(f);
					return -EINVAL;
				}
			}
			i+=page_size;
		}
		if (cmd == CMD_FLASH_PRG){
			progress_bar("\tUpdating flash\t\t\t\t",0,0);
		}else{
			progress_bar("\tVerification\t\t\t\t",0,0);
		}
	}
	if (cmd == CMD_FLASH_PRG){
		printf("\r\tUpdating flash\t\t\t\tPassed\n");
	}else{
		printf("\r\tVerification\t\t\t\tPassed\n");
	}

	fflush(stdout);
	fclose(f);
	return 0;
}


static int
aft_flash_bin_file (wan_aft_cpld_t *cpld, int stype, char *filename, int cmd)
{
	char*		data = NULL;
	u8*		tmp_data_read;
	long	findex = 0;
	long	fsize = 0;
	int 	page_size;
	int		i;

	fsize = read_bin_data_file(filename, &data);
	if (fsize < 0){
		return -EINVAL;
	}
#if 1
	if (cmd == CMD_FLASH_PRG){
		cpld->iface->erase(cpld, stype, 0);
	}
#endif	
#if 1
	while (findex < fsize) {
		if (cmd == CMD_FLASH_PRG){
			page_size = cpld->iface->prg(cpld, stype, findex, (u8*)&data[findex]);
		} else {
			page_size = cpld->iface->read(cpld, stype, MEMORY_TYPE_FLASH, findex, &tmp_data_read);
#if 1
			//printf("&data[findex]=0x%x,tmp_data_read=0x%x\n",data[findex], *tmp_data_read);
			if (memcmp(&data[findex], tmp_data_read, page_size)) {
				printf("\r\taft_flash_bin_file Verification\t\t\t\tFailed(%lx)\n",
					findex);
				free(data);
				return -EINVAL;
			}
#endif
		}
		
		if (page_size < 0) {
			if (cmd == CMD_FLASH_PRG) {
				printf("\r\taft_flash_bin_file Updating\t\t\t\tFailed(%lx)\n",
						findex);
			} else {
				printf("\r\taft_flash_bin_file Verification\t\t\t\tFailed(%lx)\n",
						findex);
			}
			free(data);
			return -EINVAL;
		}
		findex += page_size;
		//findex += 4;
		for(i=0;i<1000;i++);
		if ((findex & 0x1FFF) == 0x1000){
			if (cmd == CMD_FLASH_PRG){
				progress_bar("\tUpdating flash\t\t\t\t",
							findex, fsize);
			}else{
				progress_bar("\tVerification\t\t\t\t",
							findex, fsize);
			}
		}
	}	

	if (cmd == CMD_FLASH_PRG){
		printf("\n\r\tUpdating flash\t\t\t\tPassed\n");
	}else{
		printf("\r\tVerification\t\t\t\tPassed\n");
	}
#endif
	fflush(stdout);
	free(data);
	return 0;
	
}

static int
prg_flash_data(wan_aft_cpld_t *cpld, int stype, char *filename, int type)
{
	switch(type){
		case HEX_FILE:
			return aft_flash_hex_file(cpld, stype, filename, CMD_FLASH_PRG);
		case BIN_FILE:
			return aft_flash_bin_file(cpld, stype, filename, CMD_FLASH_PRG);
	}
	return -EINVAL;
}

static int
verify_flash_data(wan_aft_cpld_t *cpld, int stype, char *filename, int type)
{
	switch(type){
		case HEX_FILE:
			return aft_flash_hex_file(cpld, stype, filename, CMD_FLASH_VERIFY);
		case BIN_FILE:
			return aft_flash_bin_file(cpld, stype, filename, CMD_FLASH_VERIFY);
	}
	return -EINVAL;	
}

int update_flash(wan_aft_cpld_t *cpld, int stype, int mtype, char* filename)
{
	int	type, err;

	if (cpld->iface == NULL){
		printf("%s: Internal Error (line %d)\n",
				__FUNCTION__,__LINE__);
		return -EINVAL;
	}
#if 1
	if (cpld->iface->reset){
		cpld->iface->reset(cpld);
	}
#endif
	// Checking write enable to the Default Boot Flash Sector 
	if (cpld->iface->is_protected){
		if (cpld->iface->is_protected(cpld, stype)){
			printf("update_flash: Default sector protected!\n");
			return -EINVAL;
		}
	}

	type = get_file_type(filename);
	if (!(err = prg_flash_data(cpld, stype, filename, type))){
		cpld->iface->reset(cpld);
		err = verify_flash_data(cpld, stype, filename, type);
	}
	return err;
}

int board_reset(wan_aft_cpld_t *cpld, int clear)
{
	unsigned int	data;
	unsigned int 	iface_reg_off;

	switch(cpld->core_info->board_id) {
		case AFT_A600_SUBSYS_VENDOR:
		case AFT_B601_SUBSYS_VENDOR:
		case AFT_W400_SUBSYS_VENDOR:
			iface_reg_off = 0x1040;	
			break;
		default:
			iface_reg_off = 0x40;	
			break;
	}
	
	/* Release board internal reset (AFT-T1/E1/T3/E3 */
	if (exec_read_cmd(cpld->private, iface_reg_off, 4, &data)){
		printf("Failed access (read) to the board!\n");
		return -EINVAL;
	}

	switch(cpld->core_info->board_id){
	case A101_1TE1_SUBSYS_VENDOR:
	case A101_2TE1_SUBSYS_VENDOR:
	case A104_4TE1_SUBSYS_VENDOR:
	case A300_UTE3_SUBSYS_VENDOR:
		switch(cpld->adptr_type){
		case A104_ADPTR_4TE1:
		case A108_ADPTR_8TE1:
		case A116_ADPTR_16TE1:
       			if (clear) data &= ~0x06;
       	       	       	else data |= 0x06;
			break;
		case A101_ADPTR_1TE1:
		case A101_ADPTR_2TE1:
       			if (clear) data &= ~0x20;
       		       	else data |= 0x20;
			break;
		case A300_ADPTR_U_1TE3:
       			if (clear) data &= ~0x20;
       		       	else data |= 0x20;
			break;
		default:
			return -EINVAL;
		}
		break;
	case AFT_1TE1_SHARK_SUBSYS_VENDOR:
	case AFT_2TE1_SHARK_SUBSYS_VENDOR:
	case AFT_4TE1_SHARK_SUBSYS_VENDOR:
	case AFT_8TE1_SHARK_SUBSYS_VENDOR:
	case AFT_16TE1_SHARK_SUBSYS_VENDOR:
		switch(cpld->core_info->core_id){
		case AFT_PMC_FE_CORE_ID:
			switch(cpld->adptr_type){
       		 	case A104_ADPTR_4TE1:
       				if (clear) data &= ~0x06;
		       	       	else data |= 0x06;
				break;
			case A101_ADPTR_2TE1:
       				if (clear) data &= ~0x20;
			       	else data |= 0x20;
				break;
			default:
				return -EINVAL;
			}
		case AFT_DS_FE_CORE_ID:
       			if (clear) data &= ~0x06;
		       	else data |= 0x06;
			break;
		default:
			return -EINVAL;
		}
		break;
	case A200_REMORA_SHARK_SUBSYS_VENDOR:
	case A400_REMORA_SHARK_SUBSYS_VENDOR:
	case AFT_B800_SUBSYS_VENDOR:
		if (clear) data &= ~0x06;
		else data |= 0x06;
		break;
	case A300_UTE3_SHARK_SUBSYS_VENDOR:
		if (clear) data &= ~0x60;
		else data |= 0x60;
		break; 
	case AFT_56K_SHARK_SUBSYS_VENDOR:
		if (clear) data &= ~0x04;
	       	else data |= 0x04;
		break;
	case AFT_ISDN_BRI_SHARK_SUBSYS_VENDOR:
	case A700_SHARK_SUBSYS_VENDOR:
	case B500_SHARK_SUBSYS_VENDOR:
		if (clear) data &= ~0x06;
	       	else data |= 0x06;
		break;
	case AFT_2SERIAL_RS232_SUBSYS_VENDOR:
	case AFT_4SERIAL_RS232_SUBSYS_VENDOR:
	case AFT_2SERIAL_V35X21_SUBSYS_VENDOR:
	case AFT_4SERIAL_V35X21_SUBSYS_VENDOR:
		if (clear) data &= ~0x06;
	       	else data |= 0x06;
		break;
	case AFT_T116_SUBSYS_VENDOR:
		exec_read_cmd(cpld->private, 0x40, 4, &data);
		data &= ~0x06;
		exec_write_cmd(cpld->private,0x40,4,data);
		//exec_read_cmd(cpld->private, 0x40, 4, &data);
		//printf("New data 0f 0x40 = 0x%x\n",data);
		exec_write_cmd(cpld->private,0x44,2,0x4000);	
		exec_read_cmd(cpld->private, iface_reg_off, 4, &data);
		exec_write_cmd(cpld->private,0x44,4,0x00054000);	
		break;
	case AFT_A600_SUBSYS_VENDOR:
	case AFT_B601_SUBSYS_VENDOR:
	case AFT_B610_SUBSYS_VENDOR:
	case AFT_W400_SUBSYS_VENDOR:
		if (clear) data &= ~0x06;
		else data |= 0x06;
		break;
	default:
		printf("Unsupported card type (board_id=%X)!\n", cpld->core_info->board_id);		
		return -EINVAL;
	}

	if (exec_write_cmd(cpld->private, iface_reg_off,4,data)){
		printf("Failed access (write) to the board!\n");
		return -EINVAL;
	}
	
	return 0;
}

