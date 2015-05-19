#include <linux/version.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <linux/wanpipe.h>
#include <linux/sdla_fr.h>
#include <linux/types.h>
#include <linux/if_packet.h>
#include <linux/if_wanpipe.h>
#include <linux/if_ether.h>
#include <sys/stat.h>

void usage(void)
{

	printf("\nUsage: wp_x25_event_read <x25 link name>\n\neg: wp_x25_event_read link0000\n\n");

}

int main (int argc,char **argv)
{
	time_t time_val;
	struct tm *time_tm;
	unsigned char tmp_time[50];
	unsigned char *time_ptr,*time_ptr1;
	FILE *ifp;
	unsigned char filename[100], action[100], line[1000];
	struct stat file_stat;

	if (argc != 2){
		usage();
		exit(1);
	}
	
	if (strlen(argv[1]) > 15){
		printf("Link name exceed maximum size 15\n");
		exit(1);
	}	

	sprintf(filename,"/proc/net/wanrouter/x25/%s/events",argv[1]);

	if (stat(filename,&file_stat) < 0){
		printf("Link %s not found in /proc/net/wanrouter/x25 directory!\n",
			argv[1]);
		exit(1);
	}
	
	sprintf(action,"cat %s | sort",filename);
	
	ifp=popen(action, "r");

	if (ifp == NULL){
		printf("Failed to open %s\n",filename);
		exit(1);
	}
	
	while (fgets(line,sizeof(line)-1,ifp)){
		time_ptr=strpbrk(line,":");
		if (time_ptr){

			time_ptr1=strpbrk(time_ptr+1,":");
			time_ptr[0]='\0';
		
			time_val=atoi(line);
			time_tm = localtime(&time_val);

			strftime(tmp_time,sizeof(tmp_time),"%a",time_tm);
			printf("%s ",tmp_time);
			strftime(tmp_time,sizeof(tmp_time),"%b",time_tm);
			printf("%s ",tmp_time);
			strftime(tmp_time,sizeof(tmp_time),"%d",time_tm);
			printf("%s ",tmp_time);
			strftime(tmp_time,sizeof(tmp_time),"%H",time_tm);
			printf("%s:",tmp_time);
			strftime(tmp_time,sizeof(tmp_time),"%M",time_tm);
			printf("%s:",tmp_time);
			strftime(tmp_time,sizeof(tmp_time),"%S",time_tm);
			printf("%s ",tmp_time);
			printf("%s",time_ptr1);
			continue;
		}
		printf("%s",line);
		
	}
	fflush(stdout);
	return 0;
}
