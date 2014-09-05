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

# define strncasecmp	_strnicmp

#else
# include <net/if.h>
# include <wanpipe_defines.h>
# include <wanpipe_common.h>
# include <sdlapci.h>
# include <sdlasfm.h>
# include <wanpipe.h>
#endif

#include "wan_pcie_ctrl.h"


void trim(char *line){
	while(strlen(line) && (line[strlen(line)-1] < 33)){
		line[strlen(line)-1] = '\0';
	}
}


int parse_pcie_reg_dat_line(pcie_eeprom_info_t* eeprom_info, char* line, int* reg_val_index)
{
	char *key;
	char *val;
	u32 tmp;

	trim(line);
	if(strlen(line)==0){
		/* empty line */
		return 0;
	}
	
	key = val = line;	
	
	if (strchr(line, ':') != NULL) {
		while(val && *val && (*val != ':')) val++;
	
		if(val){
			*val='\0';
			val++;
		}
	
		while(val && *val && (*val < 33)) val++;
		if(*val){
			trim(key);
			if(!strncmp(key, "VENDOR_ID", 9)) {
				sscanf(val, "%X", &tmp);
				eeprom_info->vendor_id = tmp;
			} else if(!strncmp(key, "DEVICE_ID", 9)) {
				sscanf(val, "%X", &tmp);
				eeprom_info->device_id = tmp;
			} else if(!strncmp(key, "FORMAT", 9)) {
				sscanf(val, "%X", &tmp);
				eeprom_info->format = tmp;
			} else if(!strncmp(key, "VERSION", 9)) {
				sscanf(val, "%d", &tmp);
				eeprom_info->version = tmp;
			}
		} 
	} else if (strchr(line, '[') != NULL &&
		   strchr(line, ']')) {	
		
		u32 off;
		u32 value;
		
		key++;
		while(val && *val && (*val != ']')) val++;
		if(val){
			*val='\0';
			val++;
		}
		
		trim(val);
		
		sscanf(key, "%X", &off);
		sscanf(val, "%X", &value);
		
		eeprom_info->reg_vals[*reg_val_index].off = off;
		eeprom_info->reg_vals[*reg_val_index].value = value;
		*reg_val_index = *reg_val_index+1;
	}
	return 0;
}

int parse_pcie_reg_dat(pcie_eeprom_info_t* eeprom_info, char* fname)
{
	FILE * file;
	char line_buf[256];
	int line_num=0;
	
	int reg_val_index = 0;
	
	file = fopen(fname, "r");
	if (file == NULL) {
		printf("Error: failed to open %s\n", fname);
		return -1;
	}
	
	for (;;) {
		if(fgets(&line_buf[0], 256, file)){
			/* ignore lines that contain ';' */
			if(strchr(&line_buf[0],';')==NULL){
				trim(&line_buf[0]);
				if (parse_pcie_reg_dat_line(eeprom_info, &line_buf[0], &reg_val_index)) {
					printf("Could not parse line:%d: %s\n", line_num++, &line_buf[0]);
					return -1;
				}
			}
			line_num++;
		} else {
			/* EOF reached:*/
			if (!reg_val_index) {
				printf("Error: no registers defined\n");
				return -1;
			}
			eeprom_info->num_regs = reg_val_index;
			return 0;
		}
	}
	return 0;
}
