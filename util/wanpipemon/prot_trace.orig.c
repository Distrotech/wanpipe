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
# include <linux/wanpipe.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/if_wanpipe.h>
# include <linux/if_ether.h>
#else
# include <wanpipe_abstr.h>
# include <wanpipe.h>
# include <sdla_chdlc.h>
#endif
#include "fe_lib.h"
#include "wanpipemon.h"

#define X25_MOD8	0x01
#define X25_MOD128	0x10

#define	PCAP_MAGIC			0xa1b2c3d4

extern trace_prot_t trace_prot_opt;
extern trace_prot_t trace_x25_prot_opt;
extern unsigned int TRACE_DLCI;
extern unsigned char TRACE_PROTOCOL;
extern unsigned int TRACE_X25_LCN;
extern unsigned char TRACE_X25_OPT;
extern char TRACE_EBCDIC;
extern char TRACE_ASCII;
extern char TRACE_HEX;
extern int sys_timestamp;

static char ebcdic[] =
{
/*        0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F  */
/* 40 */ ' ','.','.','.','.','.','.','.','.','.','[','.','<','(','+','!',
/* 50 */ '&','.','.','.','.','.','.','.','.','.',']','$','*',')',';','^',
/* 60 */ '-','/','.','.','.','.','.','.','.','.','|',',','%','_','>','?',
/* 70 */ '.','.','.','.','.','.','.','.','.','`',':','#','@','\'','=','"',
/* 80 */ '.','a','b','c','d','e','f','g','h','i','.','.','.','.','.','.',
/* 90 */ '.','j','k','l','m','n','o','p','q','r','.','.','.','.','.','.',
/* A0 */ '.','~','s','t','u','v','w','x','y','z','.','.','.','.','.','.',
/* B0 */ '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
/* C0 */ '{','A','B','C','D','E','F','G','H','I','.','.','.','.','.','.',
/* D0 */ '}','J','K','L','M','N','O','P','Q','R','.','.','.','.','.','.',
/* E0 */ '\\','.','S','T','U','V','W','X','Y','Z','.','.','.','.','.','.',
/* F0 */ '0','1','2','3','4','5','6','7','8','9','.','.','.','.','.','.',
};


int match_trace_criteria(unsigned char *pkt, int len, int *dlci);
void trace_banner (int len, unsigned char status, unsigned int timestamp, int *trace_started);
unsigned char hexdig(unsigned char n);
unsigned char dighex(unsigned char n);
int x25_call_request_decode (unsigned char *data, int len);


int match_trace_criteria(unsigned char *pkt, int len, int *dlci)
{
	unsigned int tmp_val;
	tmp_val=(pkt[0] >> 2)&0x3F;
	tmp_val = tmp_val<<4;
	tmp_val|=(pkt[1]>>4)&0x0F;

	*dlci=tmp_val;
	
	if (TRACE_DLCI  > 0){
		if (TRACE_DLCI == tmp_val){
			return 0;
		}
	}else if (TRACE_DLCI == 0){
		return 0;
	}

	return 1;
}
void trace_banner (int len, unsigned char status, unsigned int timestamp, int *trace_started)
{
	if (!(*trace_started)){
		unsigned char date_string[50];
		
		date_string[0] = '\0';

		sprintf(date_string, "%5d", timestamp);
		
		if (sys_timestamp){
			char tmp_time[10];
			time_t time_val;
			struct tm *time_tm;

			/* Parse time and date */
			time(&time_val);
			time_tm = localtime(&time_val);
   
			strftime(tmp_time,sizeof(tmp_time),"%m",time_tm);
			sprintf(date_string+strlen(date_string), " | %s ",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%d",time_tm);
			sprintf(date_string+strlen(date_string), "%s ",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%H",time_tm);
			sprintf(date_string+strlen(date_string), "%s:",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%M",time_tm);
			sprintf(date_string+strlen(date_string), "%s:",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%S",time_tm);
			sprintf(date_string+strlen(date_string), "%s\t",tmp_time);
		}
		
		printf("\n%s     : Len=%d\tTime Stamp=%s\n",
			(status & 0x01) ? 
			"OUTGOING" : "INCOMING",
			len,
			date_string);

		*trace_started=1;
	}
}


unsigned char hexdig(unsigned char n)
{
	n &= 0x0F;														
	/* only operate on the low four bits 
	 * return a single hexadecmal ASCII digit */
	return((n >= 10) ? (n - 10 + 'A') : (n + '0'));
}


unsigned char dighex(unsigned char n)
{

	if (n >= 'a')	
		/* provide the adjustment if the passed ASCII value is greater */
		return(n + 10 - 'a') ;	/* than 'a' */
	else 
		/* provide the adjustment if the passed ASCII value is greater */
		/* than 'A' or is a numeral */
		return((n >= 'A') ? (n + 10 - 'A') : (n - '0'));
}


int x25_call_request_decode (unsigned char *data, int len)
{
    int len1, len2;
    int i;
    char addr1[16], addr2[16];
    char udata[200];
    char *first, *second;
    char byte;
    int localoffset;
    int offset=3;
    int facil_len;
    unsigned char tmp_val;

    byte = data[offset];
    len2 = (byte >> 4) & 0x0F;
    len1 = (byte >> 0) & 0x0F;

    if ((len1+len2) > len){
	    return 1;
    }
    
    
    localoffset = ++offset;
    byte = data[localoffset];

    first=addr1;
    second=addr2;
    for (i = 0; i < (len1+len2); i++) {
	if (i < len1) {
	    if (i % 2 != 0) {
		*first++ = ((byte >> 0) & 0x0F) + '0';
		localoffset++;
		byte = data[localoffset];
	    } else {
		*first++ = ((byte >> 4) & 0x0F) + '0';
	    }
	} else {
	    if (i % 2 != 0) {
		*second++ = ((byte >> 0) & 0x0F) + '0';
		localoffset++;
		byte = data[localoffset];
	    } else {
		*second++ = ((byte >> 4) & 0x0F) + '0';
	    }
	}
    }

    *first  = '\0';
    *second = '\0';

    if (len1) {
	printf("Called=%s ",addr1);
    }
    if (len2) {
	printf("Calling=%s ",addr2);
    }

    localoffset = 4;
    localoffset += ((len1 + len2 + 1) / 2);	
   
    if (localoffset >= len)
	    return 0;

    /* Look for facilities */
    facil_len=data[localoffset];
    
    /* Skip facilites for now */
    if (facil_len){ 
	printf ("Facil=");
	localoffset++;
	
	for (i=0;i<facil_len ;i++){
		printf("%02X",data[localoffset++]);
	}
	printf (" ");
    }else{
	localoffset++;
    }

    if (localoffset >= len)
	    return 0;

    len1=localoffset;

    first=udata;
    byte=data[localoffset];
    for (i=0;i<((len-len1)*2) && i<sizeof(udata);i++){
	    if (i % 2 != 0) {
		tmp_val=(byte >> 0) & 0x0F;
		*first++ = hexdig(tmp_val);
#if 0
		if (tmp_val >= 0 && tmp_val <= 9){
			*first++ = tmp_val + '0';
		}else{
			*first++ = tmp_val + 55;
		}
#endif
		localoffset++;
		byte = data[localoffset];
	    } else {
		tmp_val=(byte >> 4) & 0x0F;
		*first++ = hexdig(tmp_val);
	    }
    }

    *first='\0';
    printf("Data=%s",udata);
    
    /* Look for user data */
    return 0;
}

static char *clear_code(unsigned char code)
{
    static char buffer[25];

    if (code == 0x00 || (code & 0x80) == 0x80)
	return "DTE Originated";
    if (code == 0x01)
	return "Number Busy";
    if (code == 0x03)
	return "Invalid Facility Requested";
    if (code == 0x05)
	return "Network Congestion";
    if (code == 0x09)
	return "Out Of Order";
    if (code == 0x0B)
	return "Access Barred";
    if (code == 0x0D)
	return "Not Obtainable";
    if (code == 0x11)
	return "Remote Procedure Error";
    if (code == 0x13)
	return "Local Procedure Error";
    if (code == 0x15)
	return "RPOA Out Of Order";
    if (code == 0x19)
	return "Reverse Charging Acceptance Not Subscribed";
    if (code == 0x21)
	return "Incompatible Destination";
    if (code == 0x29)
	return "Fast Select Acceptance Not Subscribed";
    if (code == 0x39)
	return "Destination Absent";

    sprintf(buffer, "Unknown %02X", code);

    return buffer;
}

static char *clear_diag(unsigned char code)
{
    static char buffer[25];

    if (code == 0)
	return "No additional information";
    if (code == 1)
	return "Invalid P(S)";
    if (code == 2)
	return "Invalid P(R)";
    if (code == 16)
	return "Packet type invalid";
    if (code == 17)
	return "Packet type invalid for state r1";
    if (code == 18)
	return "Packet type invalid for state r2";
    if (code == 19)
	return "Packet type invalid for state r3";
    if (code == 20)
	return "Packet type invalid for state p1";
    if (code == 21)
	return "Packet type invalid for state p2";
    if (code == 22)
	return "Packet type invalid for state p3";
    if (code == 23)
	return "Packet type invalid for state p4";
    if (code == 24)
	return "Packet type invalid for state p5";
    if (code == 25)
	return "Packet type invalid for state p6";
    if (code == 26)
	return "Packet type invalid for state p7";
    if (code == 27)
	return "Packet type invalid for state d1";
    if (code == 28)
	return "Packet type invalid for state d2";
    if (code == 29)
	return "Packet type invalid for state d3";
    if (code == 32)
	return "Packet not allowed";
    if (code == 33)
	return "Unidentifiable packet";
    if (code == 34)
	return "Call on one-way logical channel";
    if (code == 35)
	return "Invalid packet type on a PVC";
    if (code == 36)
	return "Packet on unassigned LC";
    if (code == 37)
	return "Reject not subscribed to";
    if (code == 38)
	return "Packet too short";
    if (code == 39)
	return "Packet too long";
    if (code == 40)
	return "Invalid general format identifier";
    if (code == 41)
	return "Restart/registration packet with nonzero bits";
    if (code == 42)
	return "Packet type not compatible with facility";
    if (code == 43)
	return "Unauthorised interrupt confirmation";
    if (code == 44)
	return "Unauthorised interrupt";
    if (code == 45)
	return "Unauthorised reject";
    if (code == 48)
	return "Time expired";
    if (code == 49)
	return "Time expired for incoming call";
    if (code == 50)
	return "Time expired for clear indication";
    if (code == 51)
	return "Time expired for reset indication";
    if (code == 52)
	return "Time expired for restart indication";
    if (code == 53)
	return "Time expired for call deflection";
    if (code == 64)
	return "Call set-up/clearing or registration pb.";
    if (code == 65)
	return "Facility/registration code not allowed";
    if (code == 66)
	return "Facility parameter not allowed";
    if (code == 67)
	return "Invalid called DTE address";
    if (code == 68)
	return "Invalid calling DTE address";
    if (code == 69)
	return "Invalid facility/registration length";
    if (code == 70)
	return "Incoming call barred";
    if (code == 71)
	return "No logical channel available";
    if (code == 72)
	return "Call collision";
    if (code == 73)
	return "Duplicate facility requested";
    if (code == 74)
	return "Non zero address length";
    if (code == 75)
	return "Non zero facility length";
    if (code == 76)
	return "Facility not provided when expected";
    if (code == 77)
	return "Invalid CCITT-specified DTE facility";
    if (code == 78)
	return "Max. nb of call redir/defl. exceeded";
    if (code == 80)
	return "Miscellaneous";
    if (code == 81)
	return "Improper cause code from DTE";
    if (code == 82)
	return "Not aligned octet";
    if (code == 83)
	return "Inconsistent Q bit setting";
    if (code == 84)
	return "NUI problem";
    if (code == 112)
	return "International problem";
    if (code == 113)
	return "Remote network problem";
    if (code == 114)
	return "International protocol problem";
    if (code == 115)
	return "International link out of order";
    if (code == 116)
	return "International link busy";
    if (code == 117)
	return "Transit network facility problem";
    if (code == 118)
	return "Remote network facility problem";
    if (code == 119)
	return "International routing problem";
    if (code == 120)
	return "Temporary routing problem";
    if (code == 121)
	return "Unknown called DNIC";
    if (code == 122)
	return "Maintenance action";
    if (code == 144)
	return "Timer expired or retransmission count surpassed";
    if (code == 145)
	return "Timer expired or retransmission count surpassed for INTERRUPT";
    if (code == 146)
	return "Timer expired or retransmission count surpassed for DATA "
	       "packet transmission";
    if (code == 147)
	return "Timer expired or retransmission count surpassed for REJECT";
    if (code == 160)
	return "DTE-specific signals";
    if (code == 161)
	return "DTE operational";
    if (code == 162)
	return "DTE not operational";
    if (code == 163)
	return "DTE resource constraint";
    if (code == 164)
	return "Fast select not subscribed";
    if (code == 165)
	return "Invalid partially full DATA packet";
    if (code == 166)
	return "D-bit procedure not supported";
    if (code == 167)
	return "Registration/Cancellation confirmed";
    if (code == 224)
	return "OSI network service problem";
    if (code == 225)
	return "Disconnection (transient condition)";
    if (code == 226)
	return "Disconnection (permanent condition)";
    if (code == 227)
	return "Connection rejection - reason unspecified (transient "
	       "condition)";
    if (code == 228)
	return "Connection rejection - reason unspecified (permanent "
	       "condition)";
    if (code == 229)
	return "Connection rejection - quality of service not available "
               "transient condition)";
    if (code == 230)
	return "Connection rejection - quality of service not available "
               "permanent condition)";
    if (code == 231)
	return "Connection rejection - NSAP unreachable (transient condition)";
    if (code == 232)
	return "Connection rejection - NSAP unreachable (permanent condition)";
    if (code == 233)
	return "reset - reason unspecified";
    if (code == 234)
	return "reset - congestion";
    if (code == 235)
	return "Connection rejection - NSAP address unknown (permanent "
               "condition)";
    if (code == 240)
	return "Higher layer initiated";
    if (code == 241)
	return "Disconnection - normal";
    if (code == 242)
	return "Disconnection - abnormal";
    if (code == 243)
	return "Disconnection - incompatible information in user data";
    if (code == 244)
	return "Connection rejection - reason unspecified (transient "
               "condition)";
    if (code == 245)
	return "Connection rejection - reason unspecified (permanent "
               "condition)";
    if (code == 246)
	return "Connection rejection - quality of service not available "
               "(transient condition)";
    if (code == 247)
	return "Connection rejection - quality of service not available "
               "(permanent condition)";
    if (code == 248)
	return "Connection rejection - incompatible information in user data";
    if (code == 249)
	return "Connection rejection - unrecognizable protocol indentifier "
               "in user data";
    if (code == 250)
	return "Reset - user resynchronization";

    sprintf(buffer, "Unknown %d", code);

    return buffer;
}


void decode_pkt (unsigned char *data,int len,
		 unsigned char status, unsigned int timestamp,
		 int protocol)
{
	int inf_frame=0;
	int trace_started=0;
	int dlci;

	if (match_trace_criteria(data,len,&dlci) != 0)
		return;

	if (!TRACE_EBCDIC && !TRACE_ASCII && !TRACE_HEX){
		TRACE_HEX=1;
	}
	
	if (protocol){
		if (TRACE_PROTOCOL & FRAME){

			trace_banner(len,status,timestamp,&trace_started);
			
			printf("\tFR   : ");
			printf("DLCI=%i ", dlci);
			printf("C/R=%i ", (data[0]&0x2)>>1);
			printf("EA=%i ", data[0]&0x1);
			printf("FECN=%i ", (data[1]&0x8)>>3);
			printf("BECN=%i ", (data[1]&0x4)>>2);
			printf("DE=%i ", (data[1]&0x2)>>1);
			printf("EA=%i ", data[1]&0x1);
			printf("\n");
		
			if (!dlci || dlci==1023){
				printf("\n");
				return;
			}
		}
		data+=2;
		len-=2;

		if (!dlci || dlci==1023){
			return;
		}
	}

	
	if (len<2){
		return;
	}
	
	if (TRACE_PROTOCOL & LAPB){

		trace_banner(len,status,timestamp,&trace_started);
		
		inf_frame=0;
		printf("\tLAPB : %02i | ",data[0]);
		
		if (!(data[1]&0x01)){
			if (len >= 5){
				printf("INF  Ns=%1i Nr=%1i P=%1i : <data, len=%i >",
					(data[1]>>1)&0x07,
					data[1]>>5,
					(data[1]>>4)&0x01,(len-2));
				inf_frame=1;
			}else{
				printf("INF  Ns=%1i Nr=%1i P=%1i ",
					(data[1]>>1)&0x07,
					data[1]>>5,
					(data[1]>>4)&0x01);
			}
			goto end_lapb_decode;
		}
			
		switch ((data[1]&0x0F)){
				
			case 0x01: 
			      printf("RR   Pr=%1i P=%1i",
					(data[1]>>5),
					(data[1]>>4)&0x01);
			      break;

			case 0x05:
			      printf("RNR  Pr=%1i P=%1i",
					(data[1]>>5),
					(data[1]>>4)&0x01);

			      break;

			case 0x09:
			      printf("REJ  Pr=%1i P=%1i",
					(data[1]>>5),
					(data[1]>>4)&0x01);
			      break;
			
			default:
			      
				switch (data[1]&0xEF){
				
				case 0x2F:
			      		printf("SBM  P=%1i",
						(data[1]>>4)&0x01);
				      break;
				case 0x0F:
					printf("DM   F=%1i",
						(data[1]>>4)&0x01);
				      break;

				case 0x43:
					printf("DSC  P=%1i",
						(data[1]>>4)&0x01);
					break;
				case 0x63:
					printf("UA   F=%1i",
						(data[1]>>4)&0x01);
					break;
				
				case 0x87:
				      	printf("FMR  F=%1i",
						(data[1]>>4)&0x01);
				      break;

				default:
				      printf("Unknown lapb pkt : 0x%02X ", data[1]);
				      break;
				}
		}
		
end_lapb_decode:	
		printf("\n");
		
		if (!inf_frame){
			return;
		}
		
	}else{
		/* Do not pass non information frames to X25 */
		if ((data[1]&0x01)){
			return;
		}
	}

	data+=2;
	len-=2;

	if (len<3){
		return;
	}


	if (TRACE_PROTOCOL & X25){
	
		unsigned int lcn = ((data[0]&0x0F)<<8)|(data[1]&0xFF);
		unsigned char mod = (data[0]>>4)&0x03;
		unsigned char pkt_type;
		unsigned char pr,ps,mbit;
		
		inf_frame=0;
		if (TRACE_X25_LCN){
			if (lcn != 0 && TRACE_X25_LCN != lcn){
				return;
			}
		}

		pkt_type=data[2];

		if (!(pkt_type&0x01)){
			if (!(TRACE_X25_OPT & DATA)){
				return;
			}
		}else{	
			if (!(TRACE_X25_OPT & PROT)){
				return;
			}
		}
		
		trace_banner(len,status,timestamp,&trace_started);
		
		printf("\tX25  : LCN=%i  Q=%i  D=%i  Modulo=%i : ",
				lcn,
				data[0]&0x80,
				data[0]&0x40,
				data[0]&0x10 ? 8 : 128);

		if (!(pkt_type&0x01)){

			if (mod==X25_MOD8){
				pr=data[2]>>5;
				ps=(data[2]>>1)&0x07;
				mbit=(data[2]&0x10)>>4;
			}else{
				ps=data[2]>>1;
				pr=data[3]>>1;
				mbit=data[3]&0x01;
			}

			printf("DATA Pr=%02i Ps=%02i M=%i",
				pr,ps,mbit);
			inf_frame=1;
			goto end_x25_decode; 
		}else{
			if (mod==X25_MOD8){
				pr=data[2]>>5;
			}else{
				pr=data[3]>>1;
			}
		}

		switch(pkt_type&0x1F){

		case 0x01:
			printf("\n\t       ");
			printf("RR Pr=%02i",pr);
			break;

		case 0x05:
			printf("\n\t       ");
			printf("RNR Pr=%02i",pr);
			break;
			
		case 0x09:	
			printf("\n\t       ");
			printf("REJ Pr=%02i",pr);
			break;

		default:
			
			printf("\n\t     * ");
			
			switch(pkt_type){
		
			case 0x0B:
				printf("CALL REQUEST :\n\t       ");
				x25_call_request_decode(data,len);	
				break;

			case 0x0F:
				printf("CALL ACCEPT ");
				break;

			case 0x13:
				printf("CLEAR REQUEST :\n\t       ");
				printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
					data[3],clear_code(data[3]),
					data[4],clear_diag(data[4]));
				break;

			case 0x17:
				printf("CLEAR CONFIRM :\n\t       ");
				printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
					data[3],clear_code(data[3]),
					data[4],clear_diag(data[4]));
				break;
				
			case 0x23:
				printf("INTERRUPT :\n\t       ");
				printf("Data=(0x%02X)",data[3]);
				break;

			case 0x27:
				printf("INTERRUPT CONFIRM");
				break;

			case 0x1B:	
				printf("RESET REQUEST :\n\t       ");
				printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
					data[3],clear_code(data[3]),
					data[4],clear_diag(data[4]));
				break;

			case 0x1F:	
				printf("RESET CONFIRM");
				break;

			case 0xFB:
				printf("RESTART REQUEST :\n\t       ");
				printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
					data[3],clear_code(data[3]),
					data[4],clear_diag(data[4]));
				break;

			case 0xFF:
				printf("RESTART CONFIRM");
				break;
					
			case 0xF1:
				printf("\nDIAGNOSTIC");
				break;

			default:
				printf("\nUnknown x25 cmd 0x%2X",data[2]);
				break;
			}
			break;
		}
end_x25_decode:

		if (inf_frame){
			int i;
			char ch;
			char *specials = "\b\f\n\r\t";
	    		char *escapes = "bfnrt";
    			char *cp;

			printf("\n");

			if (TRACE_HEX){
				printf("\n\thex  : ");
				for (i=3;i<len;i++){
					printf("%02X ",data[i]);

					if (((i-2) % 20) == 0){
						printf("\n\t       ");
					}
				}
				printf("\n");	
			}

			
			if (TRACE_ASCII){		/* we're dumping ASCII */
				printf("\n\tascii: ");
				for (i=3;i<len;i++){
					
					ch = (isprint (data[i]) || data[i] == ' ') ? data[i] : '.';

					if ((isprint(data[i]) || data[i] == ' '))
					    (void) printf("%c", ch);
					else if (data[i] && (cp = strchr(specials, data[i])))
					    (void) printf("\\%c", escapes[cp - specials]);
					else
					    (void) printf("%02X", ch);

					//if (((i-2) % 25) == 0){
					//	printf("\n\t       ");
					//}
				}
				printf("\n");
			}

			if (TRACE_EBCDIC){		/* we're dumping EBCDIC (blecch!) */

				printf("\n\tebcdic:");
				for (i=3;i<len;i++){

					ch = (data[i] >= 0x40) ? ebcdic[data[i] - 0x40] : '.';
					
					if ((ch != '.' || data[i] == ebcdic['.']))
						(void) printf("%c", ch);
					else if (data[i] && (cp = strchr(specials, data[i])))
						(void) printf("\\%c", escapes[cp - specials]);
					else
						(void) printf("%02X", data[i]);
				
					//if (((i-2) % 25) == 0){
					//	printf("\n\t       ");
					//}

				}
				printf("\n");
			}

		}else{ //end of inf frame
			printf("\n");
		}
	}
}

static void decode_x25(unsigned char *data)
{
	unsigned int lcn = ((data[0]&0x0F)<<8)|(data[1]&0xFF);
	unsigned char mod = (data[0]>>4)&0x03;
	unsigned char pkt_type;
	unsigned char pr,ps,mbit;
	unsigned char inf_frame=0;

	if (TRACE_X25_LCN){
		if (lcn != 0 && TRACE_X25_LCN != lcn){
			return;
		}
	}

	pkt_type=data[2];

	if (!(pkt_type&0x01)){
		if (!(TRACE_X25_OPT & DATA)){
			return;
		}
	}else{	
		if (!(TRACE_X25_OPT & PROT)){
			return;
		}
	}
	
	trace_banner(len,status,timestamp,&trace_started);
	
	printf("\tX25  : LCN=%i  Q=%i  D=%i  Modulo=%i : ",
			lcn,
			data[0]&0x80,
			data[0]&0x40,
			data[0]&0x10 ? 8 : 128);

	if (!(pkt_type&0x01)){

		if (mod==X25_MOD8){
			pr=data[2]>>5;
			ps=(data[2]>>1)&0x07;
			mbit=(data[2]&0x10)>>4;
		}else{
			ps=data[2]>>1;
			pr=data[3]>>1;
			mbit=data[3]&0x01;
		}

		printf("DATA Pr=%02i Ps=%02i M=%i",
			pr,ps,mbit);
		inf_frame=1;
		goto end_x25_decode; 
	}else{
		if (mod==X25_MOD8){
			pr=data[2]>>5;
		}else{
			pr=data[3]>>1;
		}
	}

	switch(pkt_type&0x1F){

	case 0x01:
		printf("\n\t       ");
		printf("RR Pr=%02i",pr);
		break;

	case 0x05:
		printf("\n\t       ");
		printf("RNR Pr=%02i",pr);
		break;
		
	case 0x09:	
		printf("\n\t       ");
		printf("REJ Pr=%02i",pr);
		break;

	default:
		
		printf("\n\t     * ");
		
		switch(pkt_type){
	
		case 0x0B:
			printf("CALL REQUEST :\n\t       ");
			x25_call_request_decode(data,len);	
			break;

		case 0x0F:
			printf("CALL ACCEPT ");
			break;

		case 0x13:
			printf("CLEAR REQUEST :\n\t       ");
			printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
				data[3],clear_code(data[3]),
				data[4],clear_diag(data[4]));
			break;

		case 0x17:
			printf("CLEAR CONFIRM :\n\t       ");
			printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
				data[3],clear_code(data[3]),
				data[4],clear_diag(data[4]));
			break;
			
		case 0x23:
			printf("INTERRUPT :\n\t       ");
			printf("Data=(0x%02X)",data[3]);
			break;

		case 0x27:
			printf("INTERRUPT CONFIRM");
			break;

		case 0x1B:	
			printf("RESET REQUEST :\n\t       ");
			printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
				data[3],clear_code(data[3]),
				data[4],clear_diag(data[4]));
			break;

		case 0x1F:	
			printf("RESET CONFIRM");
			break;

		case 0xFB:
			printf("RESTART REQUEST :\n\t       ");
			printf("Code=(0x%02X)%s : Diag=(0x%02X)%s",
				data[3],clear_code(data[3]),
				data[4],clear_diag(data[4]));
			break;

		case 0xFF:
			printf("RESTART CONFIRM");
			break;
				
		case 0xF1:
			printf("\nDIAGNOSTIC");
			break;

		default:
			printf("\nUnknown x25 cmd 0x%2X",data[2]);
			break;
		}
		break;
	}
end_x25_decode:

	if (inf_frame){
		int i;
		char ch;
		char *specials = "\b\f\n\r\t";
		char *escapes = "bfnrt";
		char *cp;

		printf("\n");

		if (TRACE_HEX){
			printf("\n\thex  : ");
			for (i=3;i<len;i++){
				printf("%02X ",data[i]);

				if (((i-2) % 20) == 0){
					printf("\n\t       ");
				}
			}
			printf("\n");	
		}

		
		if (TRACE_ASCII){		/* we're dumping ASCII */
			printf("\n\tascii: ");
			for (i=3;i<len;i++){
				
				ch = (isprint (data[i]) || data[i] == ' ') ? data[i] : '.';

				if ((isprint(data[i]) || data[i] == ' '))
				    (void) printf("%c", ch);
				else if (data[i] && (cp = strchr(specials, data[i])))
				    (void) printf("\\%c", escapes[cp - specials]);
				else
				    (void) printf("%02X", ch);

				//if (((i-2) % 25) == 0){
				//	printf("\n\t       ");
				//}
			}
			printf("\n");
		}

		if (TRACE_EBCDIC){		/* we're dumping EBCDIC (blecch!) */

			printf("\n\tebcdic:");
			for (i=3;i<len;i++){

				ch = (data[i] >= 0x40) ? ebcdic[data[i] - 0x40] : '.';
				
				if ((ch != '.' || data[i] == ebcdic['.']))
					(void) printf("%c", ch);
				else if (data[i] && (cp = strchr(specials, data[i])))
					(void) printf("\\%c", escapes[cp - specials]);
				else
					(void) printf("%02X", data[i]);
			
				//if (((i-2) % 25) == 0){
				//	printf("\n\t       ");
				//}

			}
			printf("\n");
		}

	}else{ //end of inf frame
		printf("\n");
	}
}



static void print_pcap_file_header (wp_trace_output_iface_t *trace_iface)
{
    struct pcap_hdr fh;

    fh.magic = PCAP_MAGIC;
    fh.version_major = 2;
    fh.version_minor = 4;
    fh.thiszone = 0;
    fh.sigfigs = 0;
    fh.snaplen = 102400;
    fh.network = trace_iface->link_type;

    fwrite(&fh, sizeof(fh), 1, trace_iface->output_file);
}

static void print_pcap_record_header(wp_trace_output_iface_t *trace_iface)
{
	struct pcaprec_hdr ph;
	
 	/* Write PCap header */
        ph.ts_sec = trace_iface->pkts_written;
        ph.ts_usec = trace_iface->pkts_written;
        ph.incl_len =  trace_iface->len;
        ph.orig_len = trace_iface->len;

	fwrite(&ph, sizeof(ph), 1, trace_iface->output_file);
}

void wp_trace_output(wp_trace_output_iface_t *trace_iface)
{
	int num_chars;
	int j;

	switch (trace_iface->type){


	case WP_OUT_TRACE_INTERP:

		decode_pkt(trace_iface->data,
			   trace_iface->len,
			   trace_iface->status,
			   trace_iface->timestamp,
			   0);
		
		break;

	case WP_OUT_TRACE_PCAP:

		trace_iface->output_file = pcap_output_file;
		if (trace_iface->output_file == NULL){
			printf("Error: No output binary file specified!\n");
			return;
		}
			
		if (trace_iface->init==0){

			if (pcap_prot){
				trace_iface->link_type=pcap_prot;
			}
			
			print_pcap_file_header(trace_iface);		
			printf("Staring PCAP File Trace in: %s\n\n",
					pcap_output_file_name);
			trace_iface->init=1;
		}
		
		print_pcap_record_header(trace_iface);

		fwrite(&trace_iface->data[0], trace_iface->len, 1, trace_iface->output_file);

		printf("\rTotal Captured=%-5u (Len=%i)",trace_iface->pkts_written+1,trace_iface->len);
		
		break;

	case WP_OUT_TRACE_RAW:
	default:

		if (trace_iface->status & WP_TRACE_OUTGOING){
			printf("\nOUTGOING\t");
		}else{
			printf("\nINCOMING\t");
		}
	
		printf("Len=%-5u\tTimestamp=%-5u\t", 
				   trace_iface->len, 
				   trace_iface->timestamp);

		trace_iface->status&=(~WP_TRACE_OUTGOING);
		
		if (trace_iface->status){
			printf("Receive error - ");

			if(trace_iface->status & WP_TRACE_ABORT){
				printf("Abort");
			}else{
				printf((trace_iface->status & WP_TRACE_CRC) ? "Bad CRC" : "Overrun");
			}	
		}
		
		if (trace_iface->len == 0){
			printf("the frame data is not available" );
			return;
		}

		if (!trace_iface->trace_all_data){
       			num_chars = ((trace_iface->len <= DEFAULT_TRACE_LEN)? 
						trace_iface->len:DEFAULT_TRACE_LEN);
		}else{
			num_chars = trace_iface->len;
		}

		 printf("\n\t\t");
	 	 for( j=0; j<num_chars; j++ ) {
		 	if (j!=0 && !(j%20)){
				printf("\n\t\t");
			}
	     		printf("%02X ", trace_iface->data[j]);
		 }

		if (num_chars < trace_iface->len){
			printf("..");
		}

		printf("\n");
		
		break;
	}

	trace_iface->pkts_written++;
	trace_iface->status=0;
}



