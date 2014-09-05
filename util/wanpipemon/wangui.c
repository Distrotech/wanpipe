#include <stdio.h>
#include <stddef.h>	/* offsetof(), etc. */
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#ifdef LOCALE
#include <locale.h>
#endif

#if defined(__LINUX__)
# include <wait.h>
# include <netinet/in.h>
#else
# include <sys/wait.h>
# include <netinet/in.h>
#endif

#include "wanpipe_api.h"
#include "unixio.h"
#include "fe_lib.h"
#include "wanpipemon.h"

#define SELECT_OK 	0
#define SELECT_EXIT	256
#define SELECT_BACK	SELECT_EXIT
#define SELECT_HELP	512
#define MAX_TOKENS	32

#define UNKNOWN_SYS	0
#define LOCAL_SYS	1
#define REMOTE_SYS	2
#define MAX_IF_NUM	100

#if 0
#undef system
#define system(a)
#endif

/******************************************************************************
 * 			TYPEDEF/STRUCTURE				      *
 *****************************************************************************/

#if 0
#define GET_MENU(func,rc, arg) {switch (wan_protocol){ \
				case WANCONFIG_CHDLC: \
					rc = CHDLC##func arg; \
					break; \
				case WANCONFIG_FR: \
					rc = FR##func arg; \
					break; \
				case WANCONFIG_PPP: \
					rc = PPP##func arg; \
					break; \
				case WANCONFIG_X25: \
					rc = X25##func arg; \
					break; \
				case WANCONFIG_ADSL: \
					rc = ADSL##func arg; \
					break; \
				default:\
					rc=NULL;\
			        	printf("Unknown protocol type %i!\n",wan_protocol); \
					break;\
				}\
				}
#endif
				

static char* str_strip (char*, char*);
static int tokenize (char*, char **, char *, char *, char *);
static int wan_if_ip_menu(void);
static int wan_ip_menu(void);
static int wan_iface_menu(void);
static void wan_main_menu(void);

/*============================================================================
 * Strip leading and trailing spaces off the string str.
 */
static char* str_strip (char* str, char* s)
{
	char* eos = str + strlen(str);		/* -> end of string */

	while (*str && strchr(s, *str))
		++str;				/* strip leading spaces */
	while ((eos > str) && strchr(s, *(eos - 1)))
		--eos;				/* strip trailing spaces */
	*eos = '\0';
	return str;
}


/*============================================================================
 * Tokenize string.
 *	Parse a string of the following syntax:
 *		<tag>=<arg1>,<arg2>,...
 *	and fill array of tokens with pointers to string elements.
 *
 *	Return number of tokens.
 */
static int tokenize (char* str, char **tokens, char *arg1, char *arg2, char *arg3)
{
	int cnt = 0;

	tokens[0] = strtok(str, arg1);
	while (tokens[cnt] && (cnt < MAX_TOKENS - 1)) {
		tokens[cnt] = str_strip(tokens[cnt], arg2);
		tokens[++cnt] = strtok(NULL, arg3);
	}
	return cnt;
}


static int hit_enter_or_space_key(void)
{
	int ch;
	printf("\nHit ENTER to return to menu / SPACE to repeat\n");

	for (;;){
		kbdhit(&ch);

		if (ch==10)
			return 0;

		if (ch==32)
			return 1;
		
	}

	fflush(stdout);
	return 0;
}


static int wan_if_ip_menu(void)
{
	int err=0;
	char data[100];
	char current[100];
	char title[100];
	char prompt[100];
	int pipefds[2];	
	pid_t pid;
	int stat;
	char *menu[]={"local", "Local System Debugging", "remote", "Remote System Debugging"};

	if (pipe(pipefds) == -1){
		printf("Error pipe() failed!\n");
		return -1;
	}

	snprintf(current,100,"lo");
	snprintf(title,100,"Operation Mode Seleciton");
	snprintf(prompt,100,"Local System: device resides on the server, if name needed.  Remote System: device resides on the network, thus ip address needed");

	memset(data,0,100);

	if (!(pid=fork())){
		close(pipefds[0]);
		err=dup2(pipefds[1],2);
		if (!err){
			exit(SELECT_EXIT);
		}

		init_dialog();
		err=dialog_menu(title, prompt, 20, 50, 2, current, 2, (const char *const*)menu); 
		end_dialog();
		exit(err);
	}
	
	pid=waitpid(pid,&stat,WUNTRACED);
	
	read(pipefds[0],data,50);
	data[strlen(data)-1]='\0';

	close(pipefds[0]);
	close(pipefds[1]);

	if (stat == SELECT_OK){
		if (strcmp(data,"remote") == 0){
			return REMOTE_SYS;
		}else{
			return LOCAL_SYS;
		}
	}

	return -1;
}

static int wan_ip_menu(void)
{
	int err=0;
	char data[100];
	char title[100];
	char prompt[100];
	int pipefds[2];	
	pid_t pid;
	int stat;

	memset(data,0,100);

	if (pipe(pipefds) == -1){
		printf("Error pipe() failed!\n");
		return -1;
	}

	snprintf(title,100,"IP Address Setup");
	snprintf(prompt,100,"Specify a remote IP address");

	if (!(pid=fork())){
		
		close(pipefds[0]);
		err=dup2(pipefds[1],2);
		if (!err){
			exit(SELECT_EXIT);
		}
		
		init_dialog();
		err=dialog_inputbox (title, prompt, 10, 50, "201.1.1.2");
		end_dialog();

		fprintf(stderr,"%s",dialog_input_result);
		exit(err);
	}

	pid=waitpid(pid,&stat,WUNTRACED);
	
	read(pipefds[0],data,99);
	
	if (stat==SELECT_OK){
		struct in_addr *ip_str = NULL;
		strlcpy(ipaddress,data,16);
		if (inet_aton(ipaddress,ip_str) != 0 ){
			ip_addr = WAN_TRUE;
		}else{
			printf("Invalid IP address %s!\n",ipaddress);
		}
	}else{
		return -1;
	}

	snprintf(title,100,"UPD Port Setup");
	snprintf(prompt,100,"Specify remote UPD port");

	if (!(pid=fork())){
		
		close(pipefds[0]);
		err=dup2(pipefds[1],2);
		if (!err){
			exit(SELECT_EXIT);
		}
		
		init_dialog();
		err=dialog_inputbox (title, prompt, 10, 50, "9000");
		end_dialog();

		fprintf(stderr,"%s",dialog_input_result);
		exit(err);
	}

	pid=waitpid(pid,&stat,WUNTRACED);

	memset(data,0,100);
	
	read(pipefds[0],data,99);

	close(pipefds[0]);
	close(pipefds[1]);

	if (err==SELECT_OK){
		udp_port=atoi(data);
		return 0;
	}

	return -1;
}


static int wan_iface_menu(void)
{
	int err=0;
	char data[100];
	char current[100];
	char title[100];
	char prompt[100];
	int pipefds[2];	
	pid_t pid;
	int stat;

	int i=0;
	int j=0;
	int len=0;
	char *menu[MAX_IF_NUM];
	int toknum;
	char line[1000];
	char* token[MAX_TOKENS];
	FILE* pfd;


	memset(menu,0,sizeof(menu));

	if (pipe(pipefds) == -1){
		printf("Error pipe() failed!\n");
		return -1;
	}

#if defined(__LINUX__)
	pfd = popen("cat /proc/net/dev","r");
	if (pfd == NULL)
		return -1;

	while (fgets(line,sizeof(line)-1,pfd)){
		if (++i<3)
			continue;
		toknum = tokenize(line, token, ":", " \t\n", ":");
		if (toknum>1){
			if (j>=MAX_IF_NUM)
				break;
			menu[j++]=strdup(token[0]);
			menu[j++]=strdup(token[0]);
		}
	}	
	len = j/2;

	pclose(pfd);
#elif defined(__FreeBSD__)
	pfd = popen("ifconfig -l","r");
	i=0;
	fgets(line,sizeof(line)-1,pfd);
	toknum = tokenize(line, token, " ", " \t\n", " ");
	for (j=0;j<toknum;j++){
		if (j>=MAX_IF_NUM)
			break;
		menu[i++]=strdup(token[j]);
		menu[i++]=strdup(token[j]);
	}
	len = i/2;
	pclose(pfd);
#else
	pfd = popen("ifconfig -a","r");
	i=0;
	while(fgets(line,sizeof(line)-1,pfd)){
		if (!isalpha(line[0])){
			continue;
		}
		toknum = tokenize(line, token, ":", " \t\n", ":");
		if (toknum != 2){
			continue;
		}
		if (j>=MAX_IF_NUM)
			break;
		menu[i++]=strdup(token[0]);
		menu[i++]=strdup(token[0]);


	}
	len = i/2;
	pclose(pfd);
#endif

	if (len==0){
		return -1;
	}

	snprintf(current, 100,"lo");
	snprintf(title,100,"Interface Selection");
	snprintf(prompt,100,"Please select a WANPIPE interface using UP and DOWN keys");

	
	memset(data,0,100);

	if (!(pid=fork())){
		close(pipefds[0]);
		err=dup2(pipefds[1],2);
		if (!err){
			exit(SELECT_EXIT);
		}

		init_dialog();
		err=dialog_menu(title, prompt, 20, 50, 10, current, len, (const char *const*)menu); 
		end_dialog();

		for (i=0;i<MAX_IF_NUM;i++){
			if (menu[i]){
				free(menu[i]);
				menu[i]=0;
			}
		}
		
		exit(err);
	}

	for (i=0;i<MAX_IF_NUM;i++){
		if (menu[i]){
			free(menu[i]);
			menu[i]=0;
		}
	}

	pid=waitpid(pid,&stat,WUNTRACED);
	
	read(pipefds[0],data,50);
	data[strlen(data)-1]='\0';

	close(pipefds[0]);
	close(pipefds[1]);

	if (stat == SELECT_OK){
		strlcpy(if_name,data,WAN_IFNAME_SZ);
		ip_addr=WAN_FALSE;
		return 0;
	}
	return -1;
}


static void wan_main_menu(void)
{
	int err=0;
	char data[100];
	char cmd_data[100];
	char current[100];
	char title[100];
	char prompt[100];
	int pipefds[2];	
	pid_t pid;
	int stat;
	
	if (pipe(pipefds) == -1){
		printf("Error pipe() failed!\n");
		return;
	}
	
	snprintf(current,100,"A");
	snprintf(title,100,"Command Main Menu");
	snprintf(prompt,100,"Select one of the command sections");

	for (;;){
		memset(data,0,100);


		if (!(pid=fork())){
			int len=0;
			char **menu=NULL;
			
			close(pipefds[0]);
			err=dup2(pipefds[1],2);
			if (!err){
				exit(SELECT_EXIT);
			}

			EXEC_PROT_FUNC(get_main_menu,wan_protocol,menu,(&len));
			if (!menu){
				exit(SELECT_EXIT);
			}
			
			init_dialog();
			err=dialog_menu(title, prompt, 20, 50, 10, current, len, (const char *const*)menu); 
			end_dialog();
			exit(err);
		}//End of Child
	
		pid=waitpid(pid,&stat,WUNTRACED);
		
		read(pipefds[0],data,50);
		data[strlen(data)-1]='\0';

		if (stat == SELECT_OK){
			for (;;){

				if (!(pid=fork())){
					int len=0;
					char **menu=NULL;

					close(pipefds[0]);
					err=dup2(pipefds[1],2);
					if (!err){
						printf("DUP2 Failed!\n");
						exit(SELECT_EXIT);
					}

					EXEC_PROT_FUNC(get_cmd_menu,wan_protocol,menu,(data,&len));
					if (!menu){
						printf("NO CMD MENU FOUND %s!\n",data);
						exit(SELECT_EXIT);
					}
					
					init_dialog();
					err=dialog_menu(title, prompt, 20, 50, 10, 
							current, len, (const char *const*)menu); 
					end_dialog();
					exit(err);
					
				}//End of child
				

				memset(cmd_data,0,100);
				pid=waitpid(pid,&stat,WUNTRACED);
				read(pipefds[0],cmd_data,50);
				cmd_data[strlen(cmd_data)-1]='\0';
				
				if (stat == SELECT_OK){

exec_cmd_again:
					system("clear");	
					EXEC_PROT_FUNC(main,wan_protocol,err,(cmd_data, 0, NULL));
					
					hit_enter_or_space_key();
					if (hit_enter_or_space_key() == 0){
						continue;
					}else{
						goto exec_cmd_again;
					}
				}
				break;
			}//for
		}else{
			break;
		}
		
	}//for
	close(pipefds[0]);
	close(pipefds[1]);
}


static int if_system = UNKNOWN_SYS;
int wan_main_gui(void)
{
	int local=0;
#ifdef LOCALE
    	(void) setlocale (LC_ALL, "");
#endif	
	
	system("clear");

if_ip_menu:
	if (if_system == UNKNOWN_SYS){
		if_system = wan_if_ip_menu();
		local = 0;
	}else{
		goto main_menu;	
	}

if_local_remote_sys:
	switch(if_system){
	case LOCAL_SYS:
		if (wan_iface_menu() != 0){
			system("clear");
			if_system = UNKNOWN_SYS;
			goto if_ip_menu;
		}
		if (!local){
			return WAN_TRUE;
		}
		break;

	case REMOTE_SYS:
		if (wan_ip_menu() != 0){
			system("clear");
			if_system = UNKNOWN_SYS;
			goto if_ip_menu;
		}
		if (!local){
			return WAN_TRUE;
		}
		break;

	case UNKNOWN_SYS:
	default:
		/* Exit from wanpipemon */
		return WAN_FALSE;
		break;
	}
main_menu:
	wan_main_menu();
	local = 0;
	goto if_local_remote_sys;

	system("clear");
	return WAN_TRUE;
}
