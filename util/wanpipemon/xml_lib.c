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
#if defined(__LINUX__)
# include <linux/version.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/if_wanpipe.h>
# include <linux/if_ether.h>
#endif

#include "wanpipe_api.h"
#include "fe_lib.h"
#include "wanpipemon.h"


int output_start_xml_router (void)
{
	FILE* pipe_fd;
	char host[50];
	char tmp_time[10];
	time_t time_val;
	struct tm *time_tm;

	if (start_xml_router)
		return 0;
	
	/* Get the host name of the router */
	pipe_fd = popen("cat /etc/hostname","r");
	if (!pipe_fd)
		return 1;
	
	fgets(host,sizeof(host)-1,pipe_fd);
	pclose(pipe_fd);
	host[strlen(host)-1]='\0';

	/* Parse time and date */
	time(&time_val);
	time_tm = localtime(&time_val);
	
	printf("<router name=\"%s\">\n",host);

	printf("<date>\n");
	
	strftime(tmp_time,sizeof(tmp_time),"%Y",time_tm);
	printf("<year>%s</year>\n",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%m",time_tm);
	printf("<month>%s</month>\n",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%d",time_tm);
	printf("<day>%s</day>\n",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%H",time_tm);
	printf("<hour>%s</hour>\n",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%M",time_tm);
	printf("<minute>%s</minute>\n",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%S",time_tm);
	printf("<sec>%s</sec>\n",tmp_time);
	
	printf("</date>\n<output>\n");
	
	return 0;
}

int output_stop_xml_router(void)
{
	if (stop_xml_router)
		return 0;

	printf("</output>\n</router>\n");
	return 0;
}

int output_start_xml_header(char *hdr)
{
	printf(" <header name=\"%s\">\n",hdr);
	return 0;		
}

int output_stop_xml_header(void)
{
	printf(" </header>\n");
	return 0;
}

void output_xml_val_data (char *value_name, int value)
{
	printf("    <value name=\"%s\">\n      %i\n    </value>\n",value_name,value);
}

void output_xml_val_asc(char *value_name, char *value)
{
	printf("    <value name=\"%s\">\n      %s\n    </value>\n",value_name,value);
}

void output_error(char *value)
{
	if (xml_output){
		output_start_xml_router();	
		printf("    <error>\n      %s\n    </error>\n",value);
		output_stop_xml_router();
	}else{
		printf("ERROR: %s\n",value);
	}
}
