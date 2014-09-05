
/***********************************************************************
* wan_plxctr.c	Sangoma PLX Control Utility.
*
* Copyright:	(c) 2005 Sangoma Corp.
*
* ----------------------------------------------------------------------
* Nov 1, 2006	Alex Feldman	Initial version.
***********************************************************************/

/***********************************************************************
**			I N C L U D E  F I L E S
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
#include <netinet/in.h>

#include "wanpipe_defines.h"
#include "wanpipe_common.h"
#include "wanpipe_cfg_def.h"
#include "sdlapci.h"
#include "sdlasfm.h"
#include "wanpipe.h"

#if defined(__LINUX__)
# include <linux/if.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/if_wanpipe.h>
#else
# include <net/if.h>
#endif

#include "wan_plxup.h"

/***********************************************************************
**			D E F I N E S / M A C R O S
***********************************************************************/
#define MAX_FILE_LINE_LEN	100

/***********************************************************************
**			G L O B A L  V A R I A B L E S
***********************************************************************/
static wan_cmd_api_t	api_cmd;


/***********************************************************************
**		F U N C T I O N  P R O T O T Y P E S
***********************************************************************/	
extern int wan_plxctrl_read8(void*, unsigned char, unsigned char*);
extern int wan_plxctrl_write8(void*, unsigned char, unsigned char);

int exec_read_cmd(void*, unsigned int, unsigned int, unsigned int*);
int exec_write_cmd(void*, unsigned int, unsigned int, unsigned int);

/***********************************************************************
**		F U N C T I O N  D E F I N I T I O N S
***********************************************************************/	
static int MakeConnection(wan_plxup_t *info, char *ifname) 
{
#if defined(__LINUX__)
	char			error_msg[100];
	struct ifreq		ifr;
	struct sockaddr_ll	sa;
	
	memset(&sa,0,sizeof(struct sockaddr_ll));
	memset(&ifr,0,sizeof(struct ifreq));
	errno = 0;
   	info->sock = socket(AF_PACKET, SOCK_DGRAM, 0);
   	if(info->sock < 0 ) {
      		perror("Socket");
      		return -EIO;
   	} /* if */
  
	strncpy(ifr.ifr_name, ifname, strlen(ifname));
	if (ioctl(info->sock,SIOCGIFINDEX,&ifr)){
		snprintf(error_msg, 100, "Get index: %s", ifname);
		perror(error_msg);
		close(info->sock);
		return -EIO;
	}

	sa.sll_protocol = htons(0x17);
	sa.sll_family=AF_PACKET;
	sa.sll_ifindex = ifr.ifr_ifindex;

        if(bind(info->sock, (struct sockaddr *)&sa, sizeof(struct sockaddr_ll)) < 0){
		snprintf(error_msg, 100, "Bind: %s", ifname);
                perror(error_msg);
		close(info->sock);
                return -EINVAL;
        }
#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)

 	info->sock = socket(AF_INET, SOCK_DGRAM, 0);
   	if( info->sock < 0 ) {
      		perror("Socket");
      		return -EIO;
   	} /* if */
#endif
	return 0;

}

static int CloseConnection(wan_plxup_t *info)
{
	close(info->sock);
	return 0;
}

int exec_read_cmd(void *arg, unsigned int off, unsigned int len, unsigned int *data)
{
	wan_plxup_t	*info = (wan_plxup_t*)arg;
	int		err;
	struct ifreq	ifr;
	char		msg[100];

	memset(api_cmd.data, 0, WAN_MAX_CMD_DATA_SIZE);
	memset(ifr.ifr_name, 0, WAN_IFNAME_SZ);
	strncpy(ifr.ifr_name, info->if_name, strlen(info->if_name));
	ifr.ifr_data = (char*)&api_cmd;

	api_cmd.cmd	= SIOC_WAN_READ_PCIBRIDGE_REG;
	api_cmd.bar	= 0;
	api_cmd.len	= len;
	api_cmd.offset	= off;

	err = ioctl(info->sock,SIOC_WAN_DEVEL_IOCTL,&ifr);
	if (err){
		snprintf(msg, 100, "Read Bridge Cmd Exec: %s: ",
					ifr.ifr_name);
		perror(msg);
	}

	if (len==4){
		*(unsigned int*)data = *(unsigned int*)api_cmd.data;
	}else{
		return -EINVAL;
	}
	return err;
}

int exec_write_cmd(void *arg, unsigned int off, unsigned int len, unsigned int data)
{
	wan_plxup_t	*info = (wan_plxup_t*)arg;
	int		err;
	struct ifreq	ifr;
	char		msg[100];

	memset(api_cmd.data, 0, WAN_MAX_CMD_DATA_SIZE);
	memset(ifr.ifr_name, 0, WAN_IFNAME_SZ);
	strncpy(ifr.ifr_name, info->if_name, strlen(info->if_name));
	ifr.ifr_data = (char*)&api_cmd;

	api_cmd.cmd	= SIOC_WAN_WRITE_PCIBRIDGE_REG;
	api_cmd.bar	= 0;
	api_cmd.len	= len;
	api_cmd.offset	= off;

	if (len==4){
		*(unsigned int*)api_cmd.data=(unsigned int)data;
	}else{
		return -EINVAL;
	}

	err = ioctl(info->sock,SIOC_WAN_DEVEL_IOCTL,&ifr);
	if (err){
		snprintf(msg, 100, "Write Bridge Cmd Exec: %s: ",
					ifr.ifr_name);
		perror(msg);
	}
	return err;
}

static int wan_plxup_read_cmd(wan_plxup_t *info)
{
	FILE		*f;
	unsigned char	off, value;
	int		err = 0;

	if (info->file){	
		f = fopen(info->file, "w");//open the .HEX file for reading
		if(f == NULL){
			fprintf(stderr, "ERROR: Failed to open %s file!\n", info->file);
		}else{
			printf("Uploading to file: %s\n", info->file);
		}
		for(off=0; off <= WAN_PLXUP_MAX_ESIZE; off++){
			printf("Reading from %02X  = ", off);
			err = wan_plxctrl_read8(info, off, &value);
			if (err){
				printf("Failed (err=%d)\n", err);
				break;
			}
			fprintf(f, "%02X:%02X\n", off, value);
			printf("%02X\n", value);
		}
		if (f) fclose(f);
	}else{
		printf("Reading from %02X  = ", (unsigned char)info->addr);
		err = wan_plxctrl_read8(info, (unsigned char)info->addr, &value);
		if (!err){
			printf("%02X\n", value);
		}else{
			printf("Failed (err=%d)\n", err);
		}
	}
	return err;
}


char	buffer[WAN_PLXUP_MAX_ESIZE];
static int wan_plxup_write_cmd(wan_plxup_t *info)
{
	FILE		*f;
	unsigned int	offset, value;
	int		err;
	
	if (info->file){
		f = fopen(info->file, "r");//open the .HEX file for reading
		if(f == NULL){
			fprintf(stderr, "ERROR: Failed to open %s file!\n",
						info->file);
		}else{
			printf("Uploading file: %s\n", info->file);
		}
		while(fgets(buffer, 99 , f) != NULL){
		
			sscanf(buffer, "%X:%X", &offset, &value);
			printf("Writting %02X to %02X ...", 
					(unsigned char)value, 
					(unsigned char)offset);
			err = wan_plxctrl_write8(info, 
					(unsigned char)offset, 
					(unsigned char)value);
			if (err){
				printf("Failed (err=%d)\n", err);
				break;
			}
			printf("OK\n");			
		}
		if (f) fclose(f);
	}else{
		printf("Writting %02X to %02X ...", 
					(unsigned char)info->value, 
					(unsigned char)info->addr);
		err = wan_plxctrl_write8(info, 
					(unsigned char)info->addr, 
					(unsigned char)info->value);
		if (err){
			printf("Failed (err=%d)\n", err);
		}else{
			printf("OK!\n");
		}		
	}
	return err;
}

static int wan_plxup_cmd(int cmd, wan_plxup_t *info)
{

	switch(cmd){
	case WAN_PLXUP_READ_EFILE_CMD:
	case WAN_PLXUP_READ_E_CMD:
		wan_plxup_read_cmd(info);
		break;
	case WAN_PLXUP_WRITE_EFILE_CMD:
	case WAN_PLXUP_WRITE_E_CMD:
		wan_plxup_write_cmd(info);
		break;
	default:
		printf("ERROR: Unknown WAN_PLXUP command (%X)!\n", cmd);
		return -EINVAL;
	}
	return 0;
}


static int print_help(void)
{
	printf("\nwan_plxup Usage (version %d.%d):\n\n", 
			WAN_PLXUP_MAJOR_VERSION, WAN_PLXUP_MINOR_VERSION);
	printf("\
  wan_plxup -i <if name>  -rf <file>        - Read contents of PLX EEPROM into file\n\
  wan_plxup -i <if name>  -r  <off>         - Read PLX EEPROM value at Off\n\
  wan_plxup -i <ife name> -wf <file>        - Write PLX EEPROM contents from file\n\
  wan_plxup -i <ife name> -w  <off> <value> - Write Value to PLX EEPROM at Off\n\
  wan_plxup help                            - Print help message\n\
");
  
	printf("\n");
	printf("\n");
	return 0;
}

static int parse_args(int argc, char *argv[], wan_plxup_t *info)
{
	int	cmd = 0;
	int	i = 0;

	for(i = 0; i < argc; i++){
		if (!strcmp(argv[i], "help")){
			print_help();
			exit(0);
		}else if (!strcmp(argv[i], "-i")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid Interface Name parameter!\n");
				return -EINVAL;
			}
			strcpy(info->if_name, argv[i+1]);
		}else if (!strcmp(argv[i], "-rf")){
			cmd = WAN_PLXUP_READ_EFILE_CMD;
			if (i+1 > argc-1){
				printf("ERROR: Invalid Read file parameter!\n");
				return -EINVAL;
			}
			info->file = argv[i+1];
		}else if (!strcmp(argv[i], "-r")){
			cmd = WAN_PLXUP_READ_E_CMD;
			if (i+1 > argc-1){
				printf("ERROR: Invalid Read parameter!\n");
				return -EINVAL;
			}
			sscanf(argv[i+1], "%08X", &info->addr);
		}else if (!strcmp(argv[i], "-wf")){
			cmd = WAN_PLXUP_WRITE_EFILE_CMD;
			if (i+1 > argc-1){
				printf("ERROR: Invalid Write file parameter!\n");
				return -EINVAL;
			}
			info->file = argv[i+1];
		}else if (!strcmp(argv[i], "-w")){
			cmd = WAN_PLXUP_WRITE_E_CMD;
			if (i+2 > argc-1){
				printf("ERROR: Invalid Write parameter!\n");
				return -EINVAL;
			}
			sscanf(argv[i+1], "%08X", &info->addr);
			sscanf(argv[i+2], "%08X", &info->value);
		}
	}
	if (!cmd){
		print_help();
		exit(0);
	}
	return cmd;
}

int main (int argc, char *argv[])
{
	wan_plxup_t	info;
	int		cmd, err;

	memset(&info, 0, sizeof(wan_plxup_t));
	cmd = parse_args(argc, argv, &info);
	
	if (MakeConnection(&info, info.if_name)){
		printf("%s: Failed to create connection\n", info.if_name);
		return -EINVAL;
	}
	
	err = wan_plxup_cmd(cmd, &info);
	CloseConnection(&info);

	return err;
}


