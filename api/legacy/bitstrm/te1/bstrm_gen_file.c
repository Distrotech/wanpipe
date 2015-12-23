
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/wanpipe.h>
#include <linux/sdla_chdlc.h>

#define TX_FILE_NAME   	"tx_ch_file.b"
#define FILE_DIR	"test_file"
#define MAX_CH_NUM	31

#define CH_1_DATA	0x20
#define DIFF_BETWEEN_CH 3

#define NUM_OF_PCK_GENERATED 10000	//Equals 2.4 MB each packet is 24 bytes

unsigned char ch_data[MAX_CH_NUM];

#if 1
unsigned char ch_scratch_data[MAX_CH_NUM];
#else
//unsigned char ch_scratch_data[]={0x7E,0x7E,0x7E,0x01,0xF1,0xC1,0x00,0xFC,0x7E,0x7E,0xFF,0xFF,0x7F,0x7F};
unsigned char ch_scratch_data[]={0x7E,0x7E,0x7E,0x01,0xFF,0xC1,0xFD,0xFC,0x7E,0x7E,0x01,0x7F,0xFF,0x7F,0x7F};
#endif

int main(int argc, char* argv[])
{
	FILE *file;
	unsigned char tx_file_name[100];
	int i,x;
	int ferr;
	
	sprintf(tx_file_name, "%s/%s",FILE_DIR,TX_FILE_NAME);
	
	file = fopen(tx_file_name,"wb");
	if (!file){
		printf("Failed to open TX File: %s\n",
				tx_file_name);
		return 1;
	}

#if 1
	for (i=0;i<MAX_CH_NUM;i++){
		ch_data[i]=CH_1_DATA+(i*DIFF_BETWEEN_CH);
		//ch_data[i]=0x10;
		printf("Data CH=%i = 0x%x\n",
				(i+1),ch_data[i]);
	}
#endif



	for (i=0;i<NUM_OF_PCK_GENERATED;i++){
#if 1
		for (x=0;x<MAX_CH_NUM;x++){
			ch_scratch_data[x] = ch_data[x]+(i%3);
			//ch_scratch_data[x] = 0x10;
		}
#endif
		ferr=fwrite((void *)&ch_scratch_data[0],
			     sizeof(char), sizeof(ch_scratch_data), file);
		
		if (ferr != sizeof(ch_scratch_data)){
			printf("Failed to write to %s file: Invalid write len=%i instead of %i\n",
					tx_file_name,ferr,sizeof(ch_scratch_data));
			fclose(file);
			return -1;
		}
	}

	fclose(file);
	return 0;
}
