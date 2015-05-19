#if defined(__WINDOWS__)
# include <windows.h>
# include <winioctl.h>
# include <conio.h>
# include <stddef.h>		//for offsetof()
# include <string.h>
# include <stdlib.h>
# include "wanpipe_includes.h"
# include "wanpipe_api.h"
#else
# include <ctype.h>
# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
#endif

#include "mem.h"

#define BUFFER_SIZE         4096
#define READ_BUFFER_TIMEOUT 1000

#define PM_SIZE 1536 /* Max: 144KB/3/32=1536 PM rows for 30F. */
#define EE_SIZE 128 /* 4KB/2/16=128 EE rows */
#define CM_SIZE 8


extern "C" {
int MakeConnection(char *ifname);
int CloseConnection(char *ifname); 
int wan_usbfxo_fwupdate(void*, char *ifname, int);
int exec_usb_data_write(void*, int off, char *data, int len);
int exec_usb_data_read(void*, char *data, int len);
int progress_bar(char *msg, int ci, int mi);
void hit_any_key(void);
}

int ReceiveData (void*, char *pBuffer ,  int BytesToRead);
int WriteCommBlock (void*, char *pBuffer ,  int BytesToWrite);
static int SendHexFile(void*, FILE * fin, eFamily Family, char*);

#if defined(__WINDOWS__)
int wan_usbfxo_fwupdate(void *arg, char *ifname, int multiupdate)
{
	printf("%s(): Error: function not implemented!\n", __FUNCTION__);
	return -EINVAL;
}
#else
int wan_usbfxo_fwupdate(void *arg, char *ifname, int multiupdate)
{
	FILE	*fin = NULL;
	int	err = 0;
	char	filename[50], ch = 0;
	eFamily	family = dsPIC33F;

filename_again:
	printf("Enter new USB-FXO firmware file [q=exit] > ");
	scanf("%s", filename);
	if (strlen(filename) == 1 && toupper(filename[0]) == 'Q'){
		return -EINVAL;
	}
	if ((fin = fopen(filename, "r")) == NULL){
		printf("ERROR: Failed to find specified firmware file %s!\n",
				filename);
		printf("\n");
		goto filename_again;
	}
	fclose(fin);

update_again:

	system("modprobe -r wan_aften");

	printf("\n");
	printf(
"We are ready to start USB-FXO firmware update. In order to perform USB-FXO\n" 
"firmware update, you will need to execute the following steps:\n"
"1. If USB-FXO device is connected to the computer, please disconnect it now\n"
"2. Now re-connect USB-FXO device\n"
"3. Wait for alternating RED/GREEN leds, then press Enter to begin USB-FXO\n"
"   firmware update\n"
"\n"
"(NOTE: You must press Enter while alternating RED/GREEN leds are flashing\n"
"       (10 secs).\n"
	);

	printf("\n");
	hit_any_key();

	printf("Connect USB-FXO device and press Enter to update firmware ...");
	getchar();	

	system("modprobe wan_aften");
	sleep(3);
	if (MakeConnection(ifname)){
		printf("%s: Failed to create socket to the driver!\n", ifname);
		return -EINVAL;
	}
	
	fin = fopen(filename, "r");
	if (fin == NULL){
		printf("ERROR: Failed to open firmware file %s!\n",
				filename);
		return -EINVAL;
	}

	err = SendHexFile(arg, fin, family, filename);
	if (fin) fclose(fin);

	CloseConnection(ifname);

	if (multiupdate){
		printf("Would you like to update firmware for another USB-FXO device (Y/N) > ");
		ch = getchar();
		if (toupper(ch) == 'Y'){
			goto update_again;
		}
	}

	return err;
}
#endif

static int SendHexFile(void *arg, FILE *fin, eFamily Family, char *filename)
{
	char	Buffer[BUFFER_SIZE];
	int	ExtAddr = 0, Row;
	int	err = 0, progress_cnt = 0;

	/* Initialize Memory */
	mem_cMemRow ** ppMemory = (mem_cMemRow **)malloc(sizeof(mem_cMemRow *) * PM_SIZE + sizeof(mem_cMemRow *) * EE_SIZE + sizeof(mem_cMemRow *) * CM_SIZE);

	for(Row = 0; Row < PM_SIZE; Row++)
	{
		ppMemory[Row] = new mem_cMemRow(mem_cMemRow::Program, 0x000000, Row, Family);
	}

	for(Row = 0; Row < EE_SIZE; Row++)
	{
		ppMemory[Row + PM_SIZE] = new mem_cMemRow(mem_cMemRow::EEProm, 0x7FF000, Row, Family);
	}

	for(Row = 0; Row < CM_SIZE; Row++)
	{
		ppMemory[Row + PM_SIZE + EE_SIZE] = new mem_cMemRow(mem_cMemRow::Configuration, 0xF80000, Row, Family);
	}
	
	printf("\n");
	progress_cnt = 0;
	progress_bar("Loading USB-FXO firmware ...\t", 0, 0);
	while(fgets(Buffer, sizeof(Buffer), fin) != NULL)
	{
		int ByteCount;
		int Address;
		int RecordType;

		if (++progress_cnt % 100 == 0) progress_bar("Loading USB-FXO firmware ...\t", 0, 0);

		sscanf(Buffer+1, "%2x%4x%2x", &ByteCount, &Address, &RecordType);

		if(RecordType == 0)
		{
			Address = (Address + ExtAddr) / 2;
			
			for(int CharCount = 0; CharCount < ByteCount*2; CharCount += 4, Address++)
			{
				bool bInserted = 0;

				for(Row = 0; Row < (PM_SIZE + EE_SIZE + CM_SIZE); Row++)
				{
					if((bInserted = ppMemory[Row]->InsertData(Address, Buffer + 9 + CharCount)) == 1)
					{
						break;
					}
				}

				if(bInserted != 1)
				{
					printf("Failed (Bad Hex file)!\n");
					printf("ERROR: Address 0x%x out of range\n", Address);
					return -EINVAL;
				}
			}
		}
		else if(RecordType == 1)
		{
		}
		else if(RecordType == 4)
		{
			sscanf(Buffer+9, "%4x", &ExtAddr);

			ExtAddr = ExtAddr << 16;
		}
		else
		{
			printf("Failed (Unknown hex record type)!\n");
			return -EINVAL;
		}
	}

	for(Row = 0; Row < (PM_SIZE + EE_SIZE + CM_SIZE); Row++)
	{
		ppMemory[Row]->FormatData();
	}
	printf("\tDone\n");

	progress_cnt = 0;
	progress_bar("Programming USB-FXO Device ...\t", 0, 0);
	for(Row = 0; Row < (PM_SIZE + EE_SIZE + CM_SIZE); Row++)
	{
		if (++progress_cnt % 10 == 0) progress_bar("Programming USB-FXO Device ...\t", 0, 0);
		err = ppMemory[Row]->SendData(arg);
		if (err){
			switch(err){
			case -EINVAL:
				printf("Failed (Unknown memory type)!\n");
				break;
			case -EBUSY:
				printf("Failed (Timeout)!\n");
				printf("ERROR: Failed to receive ACK command!\n");
				break;
			case -EIO:
				break;
			default:
				printf("Failed (Unknown error code %d)!\n", err);
				break;
			}
			printf("\n\n");
			return -EINVAL;
		}
	}

	Buffer[0] = COMMAND_RESET;
	WriteCommBlock(arg, Buffer , 1);

	printf("\tDone\n\n");
	return 0;
}

int WriteCommBlock(void* arg, char *pBuffer , int BytesToWrite)
{
	int	len = 0, cnt = 0;
 
	while(cnt < BytesToWrite){

		if ((cnt + 32) < BytesToWrite){
			len = 32;
		}else{
			len = BytesToWrite - cnt;
		}
		if (exec_usb_data_write(arg, cnt, &pBuffer[cnt], len)){
			return -EINVAL;
		}
		cnt += len;
	}
	return 0;
}

int ReceiveData (void *arg, char *pBuffer, int BytesToRead)
{
	if (exec_usb_data_read(arg, pBuffer, BytesToRead)){
		return -EINVAL;
	}
	return 0;
}

