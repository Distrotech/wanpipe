#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <linux/wanpipe_wanrouter.h>	


#define WANCONFIG_SOCKET "/etc/wanpipe/wanconfig_socket"

int main (int argc, char **argv)
{
	struct sockaddr_un address;
	int sock;
	size_t addrLength;
	unsigned char data[100], rx_data[100];
	int err,len;
	fd_set	readset;

	err=1;

	if (argc < 2){
		printf("Usage wp_client <cmd>");
		exit(1);
	}
			
	memset(&address,0,sizeof(struct sockaddr_un));
	
	if ((sock=socket(PF_UNIX,SOCK_STREAM,0)) < 0){
		perror("socket: ");
		return -EIO;
	}

	address.sun_family=AF_UNIX;
	strcpy(address.sun_path, WANCONFIG_SOCKET);

	addrLength = sizeof(address.sun_family) + strlen(address.sun_path);

	if(connect(sock,(struct sockaddr*)&address,addrLength)){
		perror("connect: ");
		close(sock);
		return errno;
	}

	len=sprintf(data,argv[1]);
	
	err=send(sock,data,len,0);
	if (err != len){
		printf("Clinet failed to send the whole packet");
		if (err<=0){
			perror("send: ");
		}
		close(sock);
		return errno;
	}

	FD_ZERO(&readset);
	FD_SET(sock,&readset);

	if (select(sock+1,&readset,NULL,NULL,NULL)){
		if (FD_ISSET(sock,&readset)){
			
			err=recv(sock,rx_data,100,0);
			if (err>0){
				err=*(unsigned short*)&rx_data[0];
				printf("%s :  err=%i\n",data,err);
			}else{
				perror("recv: ");	
			}
			
		}else{
			printf("Select ok but not read ????\n");
		}
	}else{
		perror("Select: ");
	}
	
	close(sock);
	return err;
}
