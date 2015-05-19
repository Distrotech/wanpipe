/* prot_trace.c */
#if defined(__WINDOWS__)
# include <conio.h>				/* for _kbhit */
# include "wanpipe_includes.h"
# include "wanpipe_defines.h"	/* for 'wan_udp_hdr_t' */
# include "wanpipe_time.h"		/* for 'struct timeval' */
# include "wanpipe_common.h"	/* for 'strcasecmp' */
#else
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
# include <netinet/ip.h>
# include <netinet/tcp.h>
# include <linux/if_wanpipe.h>
# include <linux/if_ether.h>
#else
# include <netinet/in_systm.h>
# include <netinet/ip.h>
# include <netinet/tcp.h>
#endif
#endif

#include "wanpipe_api.h"
#include "fe_lib.h"
#include "wanpipemon.h"

#define X25_MOD8	0x01
#define X25_MOD128	0x10

#define	PCAP_MAGIC			0xa1b2c3d4

extern trace_prot_t trace_prot_opt;
extern trace_prot_t trace_x25_prot_opt;
extern unsigned int TRACE_DLCI;
extern unsigned char TRACE_PROTOCOL;
extern unsigned int  DECODE_PROTOCOL;
extern unsigned int TRACE_X25_LCN;
extern unsigned char TRACE_X25_OPT;
extern char TRACE_EBCDIC;
extern char TRACE_ASCII;
extern char TRACE_HEX;
extern int sys_timestamp;


 
#ifndef ETH_P_LAPD
#define ETH_P_LAPD 0x0030
#endif

#define LAPD_SLL_PKTTYPE_OFFSET         0       /* packet type - 2 bytes */
#define LAPD_SLL_HATYPE_OFFSET          2       /* hardware address type - 2 bytes */
#define LAPD_SLL_HALEN_OFFSET           4       /* hardware address length - 2 bytes */
#define LAPD_SLL_ADDR_OFFSET            6       /* address - 8 bytes */
#define LAPD_SLL_PROTOCOL_OFFSET        14      /* protocol, should be ETH_P_LAPD - 2 bytes */
#define LAPD_SLL_LEN                    16      /* length of the header */

#define MTP2_SENT_OFFSET		0	/* 1 byte */
#define MTP2_ANNEX_A_USED_OFFSET	1	/* 1 byte */
#define MTP2_LINK_NUMBER_OFFSET		2	/* 2 bytes */
#define MTP2_HDR_LEN			4	/* length of the header */


#undef phtons
#define phtons(p, v) \
        {                               \
        ((uint8_t*)(p))[0] = (uint8_t)((v) >> 8); \
        ((uint8_t*)(p))[1] = (uint8_t)((v) >> 0); \
        }

#undef pntohs
#define pntohs(p)   ((guint16)                       \
                     ((guint16)*((const uint8_t *)(p)+0)<<8|  \
                      (guint16)*((const uint8_t *)(p)+1)<<0))



extern wanpipe_hdlc_engine_t *rx_hdlc_eng;  

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
unsigned char hexdig(unsigned char n);
unsigned char dighex(unsigned char n);
int x25_call_request_decode (unsigned char *data, int len);
void trace_banner (wp_trace_output_iface_t *trace_iface, int *trace_started);
int trace_hdlc_data(wanpipe_hdlc_engine_t *hdlc_eng, void *data, int len);

static void decode_chdlc_ip_transaction(cisco_slarp_t *cisco_slarp);
static void print_ipv4_address(unsigned int address);
static int decode_data_ipv4(unsigned char* data, unsigned short data_len);
static char* decode_tcp_level_protocol(unsigned short port);
static void print_data_in_hex(unsigned char* data, unsigned short data_len);

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

void trace_banner (wp_trace_output_iface_t *trace_iface, int *trace_started)
{
	int len 		=trace_iface->len;
	unsigned char status	=trace_iface->status;
	unsigned int timestamp	=trace_iface->timestamp;

	if (!(*trace_started)){
		char date_string[100];
		
		date_string[0] = '\0';

		snprintf(date_string, 100, "%5d", timestamp);
		
		if (trace_iface->sec){
			char tmp_time[50];
			time_t time_val=trace_iface->sec;
			struct tm *time_tm;

			/* Parse time and date */
			time_tm = localtime(&time_val);
   
			strftime(tmp_time,sizeof(tmp_time),"  %b",time_tm);
			snprintf(date_string+strlen(date_string), 100-strlen(date_string), " %s ",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%d",time_tm);
			snprintf(date_string+strlen(date_string), 100-strlen(date_string), "%s ",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%H",time_tm);
			snprintf(date_string+strlen(date_string), 100-strlen(date_string), "%s:",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%M",time_tm);
			snprintf(date_string+strlen(date_string), 100-strlen(date_string), "%s:",tmp_time);

			strftime(tmp_time,sizeof(tmp_time),"%S",time_tm);
			snprintf(date_string+strlen(date_string), 100-strlen(date_string), "%s",tmp_time);

			snprintf(date_string+strlen(date_string), 100-strlen(date_string), " %-6lu [1/100s]",(unsigned long)trace_iface->usec );
		}
		
		printf("\n%s\tLen=%d\tTimeStamp=%s\n",
			(status & 0x01) ? 
			"OUTGOING" : "INCOMING",
			len,
			date_string);

		trace_iface->status&=(~WP_TRACE_OUTGOING);
		if (trace_iface->status){
			printf("\t\tReceive error - ");

			if(trace_iface->status & WP_TRACE_ABORT){
				printf("\t\tAbort");
			}else{
				printf((trace_iface->status & WP_TRACE_CRC) ? "\t\tBad CRC" : "\t\tOverrun");
			}	
		}

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

    snprintf(buffer, 25, "Unknown %02X", code);

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

    snprintf(buffer, 25, "Unknown %d", code);

    return buffer;
}

#ifdef __WINDOWS__
static int decode_chdlc(wp_trace_output_iface_t *trace_iface,
		        int *trace_started)
{
	WIN_DBG_FUNC_NOT_IMPLEMENTED();
	return 1;
}
#else
static int decode_chdlc(wp_trace_output_iface_t *trace_iface,
		        int *trace_started)
{
	unsigned char *data= trace_iface->data;

	int inf_frame=0;
	cisco_header_t *cisco_header = (cisco_header_t *)&data[0];
	u_int32_t time_alive;
	
	trace_banner(trace_iface,trace_started);

	
	//print the data (including Cisco header)
      	print_data_in_hex(data, trace_iface->len);

	printf("CHDLC decode\t");

	switch(ntohs(cisco_header->packet_type)) {
			
		case CISCO_PACKET_IP:
			printf("CISCO Packet IP-v4\n\n");
			//decode data past Cisco header
		     	decode_data_ipv4((unsigned char*)(cisco_header + 1),
				trace_iface->len - sizeof(cisco_header_t));
      			break;

		case CISCO_PACKET_SLARP: 
			{ 
			cisco_slarp_t *cisco_slarp =
				(cisco_slarp_t*)(cisco_header+1);

			switch(ntohl(cisco_slarp->code)) {
			case SLARP_REQUEST:
				  printf("SLARP Request packet\n");
				  decode_chdlc_ip_transaction(cisco_slarp);
				  break;

			case SLARP_REPLY:
				  printf("SLARP Reply packet\n");
				  decode_chdlc_ip_transaction(cisco_slarp);
		 		  break;

			case SLARP_KEEPALIVE:
				  printf("SLARP Keepalive packet\n");
				  printf("\t\tlocal sequence number: %lu\tremote sequence number: %lu\n",
					(unsigned long)ntohl(cisco_slarp->un.keepalive.my_sequence),
					(unsigned long)ntohl(cisco_slarp->un.keepalive.your_sequence));

				  {
					unsigned short t1;
					unsigned short t2;

					t1 = (cisco_slarp->un.keepalive.t1 << 8) | (cisco_slarp->un.keepalive.t1  >> 8);
					t2 = (cisco_slarp->un.keepalive.t2 << 8) | (cisco_slarp->un.keepalive.t2  >> 8);
				 
				  	time_alive = t1;
				  	time_alive = (time_alive << 16) | t2;

				  }
				  printf("\t\treliability number: 0x%X\t\ttime alive: %u",
					(unsigned short)ntohs(cisco_slarp->un.keepalive.reliability),
					time_alive);
				  break;

			default:
				  printf("Unrecognized SLARP packet");
				  break;
			} /* Slarp code switch */
			
			break;
			
			} /* slarp case */

		case CISCO_PACKET_CDP: 
			printf("Cisco Discover Protocol packet");
			break;

		default:
			printf("Unknown Cisco packet");
			break;
	}

	return inf_frame;
}
#endif

static void decode_chdlc_ip_transaction(cisco_slarp_t *cisco_slarp)
{
	printf("\t\tIP address:  ");
       	print_ipv4_address(cisco_slarp->un.address.address);

	printf("\tIP netmask:  ");
       	print_ipv4_address(cisco_slarp->un.address.mask);
}

#define IP_V4		4
#define IP_V4_IHLEN  	5

#ifdef __WINDOWS__
static int decode_data_ipv4(unsigned char* data, unsigned short data_len)
{
	WIN_DBG_FUNC_NOT_IMPLEMENTED();
	return 1;
}
#else
static int decode_data_ipv4(unsigned char* data, unsigned short data_len)
{
	iphdr_t *ip_hdr = (iphdr_t*)data;
	struct tcphdr * tcp_hdr;
  	char is_port_valid_for_protocol = 1;

	printf("TCP/IP decode\t");

	if(data_len < sizeof(iphdr_t) + sizeof(struct tcphdr)){
		printf("Data length is invalid for IP V4.\n");
		return 1;
  	}

	if(ip_hdr->w_ip_v != IP_V4){
		printf("Invalid IP version: %d.\n",
				ip_hdr->w_ip_v);
		return 1;
	}

	if(ip_hdr->w_ip_hl != IP_V4_IHLEN){
		printf("Invalid IP header length: %d.\n",
				ip_hdr->w_ip_hl);
		return 1;
	}

	printf("IP Level Protocol: ");
	
	switch(ip_hdr->w_ip_p)
	{
	case IPPROTO_IP:	/* 0 - Dummy protocol for TCP		 */
		printf("TCP");
		break;
		
	case IPPROTO_ICMP:	/* 1 */
		printf("Internet Control Message Protocol (ICMP)");
    		is_port_valid_for_protocol = 0;
		break;

	case IPPROTO_IGMP:	/* 2 */
		printf("Internet Group Management Protocol (IGMP)");
		break;
	
	case IPPROTO_IPIP:	/* 4 */
		printf("IPIP tunnels");
		break;

	case IPPROTO_TCP:	/* 6 */
		printf("Transmission Control Protocol (TCP)");
		break;

	case IPPROTO_EGP:	/* 8 */
		printf("Exterior Gateway Protocol (EGP)");
		break;

	case IPPROTO_PUP:	/* 12 */
		printf("PUP protocol");
		break;

	case IPPROTO_UDP:	/* 17 */
		printf("User Datagram Protocol (UDP)");
		break;

	case IPPROTO_IDP:	/* 22 */
		printf("XNS IDP protocol");
		break;

	case IPPROTO_RSVP:	/* 46 */
		printf("RSVP protocol");
		break;

	case IPPROTO_GRE:	/* 47 */
		printf("Cisco GRE tunnels (rfc 1701,1702)");
		break;

	case IPPROTO_IPV6:	/* 41 */
		printf("IPv6-in-IPv4 tunnelling");
		break;

	case IPPROTO_PIM:	/* 103 */
		printf("Protocol Independent Multicast");
		break;

	case IPPROTO_ESP:      /* 50 */
		printf("Encapsulation Security Payload protocol");
		break;

	case IPPROTO_AH:       /* 51 */
		printf("Authentication Header protocol");
		break;

#if defined(__LINUX__)
	case IPPROTO_COMP:     /* 108 */
#else
	case IPPROTO_IPCOMP:     /* 108 */
#endif
		printf("Compression Header protocol");
		break;

	case IPPROTO_RAW:      /* 255  */
		printf("Raw IP packets");
		break;

#if 0	
	case IPPROTO_MAX:
		printf("IPPROTO_MAX");
		break;
#endif
	
	default:
		printf("Unknown (%d)\n", ip_hdr->w_ip_p);
		//not a serious error, but stop here anyway
		return 0;
	}

	printf(", TTL: %d.\n", ip_hdr->w_ip_ttl);

	printf("\t\tSource IP: ");
	print_ipv4_address(ip_hdr->w_ip_src);
	printf(", Destination IP: ");
	print_ipv4_address(ip_hdr->w_ip_dst);
	printf("\n");

	//not all protocols actually using a port
  	if(is_port_valid_for_protocol == 1){
  		//now decode TCP part
		tcp_hdr = (struct tcphdr*)&data[sizeof(iphdr_t)];
  		printf("\t\tSource Port: %u (%s), Destination Port: %u (%s)\n",
	  		ntohs(tcp_hdr->w_tcp_sport),
		  	decode_tcp_level_protocol(ntohs(tcp_hdr->w_tcp_sport)),
	         	ntohs(tcp_hdr->w_tcp_dport),
  			decode_tcp_level_protocol(ntohs(tcp_hdr->w_tcp_dport)));
  	}
	return 0;
}
#endif

//define here most used protocols
#define ECHO		7
#define FTP_DATA	20
#define FTP		21
#define TELNET		23
#define SMTP		24
#define FINGER		79
#define SNMP		161

static char* decode_tcp_level_protocol(unsigned short port)
{
	switch(port)
	{
	case ECHO:
		return "ECHO";
		
	case FTP_DATA:
		return "FTP-DATA";
		
	case FTP:
		return "FTP";

	case TELNET:
		return "TELNET";

	case SMTP:
		return "SMTP";

	case FINGER:
		return "FINGER";
	
	case SNMP:
		return "SNMP";

	default:
		return "Unknown";
	}
}

static void print_ipv4_address(unsigned int address)
{
	printf("%u.%u.%u.%u",(u_int32_t)address & 0x000000FF,
				(unsigned int)(address & 0x0000FF00) >> 8,
				(unsigned int)(address & 0x00FF0000) >>16,
				(unsigned int)(address & 0xFF000000) >>24);
}

static void print_data_in_hex(unsigned char* data, unsigned short data_len)
{
  	int i;

	printf("Raw (HEX)\t");
	
	for(i = 0; i < data_len; i++){
		if(i && i%16 == 0){
			printf("\n\t\t");
		}
		printf("%02X ", (unsigned char)data[i]);
		
		//can be too much to print. limit to 256
		if (!trace_all_data){
			if(i == 256){
				printf("...\n");
				break;
			}
		}
		
	}
	printf("\n\n");
}

#define SIZEOF_FR_HEADER  4

/*
ANSI
OUTGOING     : Len=16   TimeStamp=15210
               00 01 03 08 00 75 95 01 01 01 03 02 08 01 FF FF

INCOMING     : Len=16   TimeStamp=15211
               00 01 03 08 00 7D 95 01 01 01 03 02 02 08 53 CC


OUTGOING     : Len=16   TimeStamp=65210
               00 01 03 08 00 75 95 01 01 00 03 02 0D 06 FF FF

INCOMING     : Len=21   TimeStamp=65211
               00 01 03 08 00 7D 95 01 01 00 03 02 07 0D 07 03 01 80 82 2C
               A7
	       
*/

/*
Q.933
OUTGOING     : Len=15   TimeStamp=40746
               00 01 03 08 00 75 51 01 01 53 02 04 01 FF FF

INCOMING     : Len=15   TimeStamp=40747
               00 01 03 08 00 7D 51 01 01 53 02 02 04 02 3D


OUTGOING     : Len=15   TimeStamp=25210
               00 01 03 08 00 75 51 01 00 53 02 09 06 FF FF

INCOMING     : Len=20   TimeStamp=25211
               00 01 03 08 00 7D 51 01 00 53 02 07 09 57 03 01 80 82 A0 E6	       
*/

/*
LMI
OUTGOING     : Len=15   TimeStamp=60746
               FC F1 03 09 00 75 01 01 01 03 02 06 05 FF FF

INCOMING     : Len=15   TimeStamp=60747
               FC F1 03 09 00 7D 01 01 01 03 02 06 06 19 44

OUTGOING     : Len=15   TimeStamp= 5210
               FC F1 03 09 00 75 01 01 00 03 02 07 06 FF FF

INCOMING     : Len=31   TimeStamp= 5211
               FC F1 03 09 00 7D 01 01 00 03 02 07 07 07 06 00 10 02 00 00
               00 07 06 00 12 02 00 00 00 C2 B1
*/

#define ANSI_LINK_VERIFICATION_LEN_WCRC	16
#define ANSI_LINK_VERIFICATION_LEN	14
#define ANSI_FULL_STATUS_LEN_WCRC	21
#define ANSI_FULL_STATUS_LEN		19
#define ANSI_OFFSET			9

#define Q933_LINK_VERIFICATION_LEN_WCRC	15
#define Q933_LINK_VERIFICATION_LEN	13
#define Q933_FULL_STATUS_LEN_WCRC	20
#define Q933_FULL_STATUS_LEN		18
#define Q933_OFFSET			8


#define LMI_LINK_VERIFICATION_LEN 	Q933_LINK_VERIFICATION_LEN
#define LMI_OFFSET			8

#ifdef __WINDOWS__
static int decode_fr(wp_trace_output_iface_t *trace_iface,
		 int *trace_started,int dlci)
{
	WIN_DBG_FUNC_NOT_IMPLEMENTED();
	return 1;
}
#else
static int decode_fr(wp_trace_output_iface_t *trace_iface,
		 int *trace_started,int dlci)
{
	unsigned char *data	=(unsigned char*)trace_iface->data;
	int len 		=trace_iface->len;
	
	int inf_frame=1;
	int j;

	static int signalling_offset = 0;
	static char lmi = 0;

	trace_banner(trace_iface,trace_started);
	
      	//print the data (including FR header)
	print_data_in_hex(data, len);

	//printf("Frame Relay decode");
	printf("FR decode\t");
	
	//printf("\tFR   : ");
	printf("DLCI=%i ", dlci);
	printf("C/R=%i ", (data[0]&0x2)>>1);
	printf("EA=%i ", data[0]&0x1);
	printf("FECN=%i ", (data[1]&0x8)>>3);
	printf("BECN=%i ", (data[1]&0x4)>>2);
	printf("DE=%i ", (data[1]&0x2)>>1);
	printf("EA=%i ", data[1]&0x1);
	printf("\n");

	if (len <= SIZEOF_FR_HEADER){
		return inf_frame;
	}

	if (!dlci || dlci==1023){

		//printf("len: %d\n", len);
		if(signalling_offset == 0){
			//need at least one signalling frame to detect signalling type.
			switch(len)
			{
			case ANSI_LINK_VERIFICATION_LEN:
			case ANSI_LINK_VERIFICATION_LEN_WCRC:
				signalling_offset = ANSI_OFFSET;
				break;

			case Q933_LINK_VERIFICATION_LEN:
				signalling_offset = Q933_OFFSET;

				if(dlci == 1023){
					lmi = 1;
				}else{
					lmi = 0;
				}
				break;
			
			default:
				printf("Unknow Signalling: Wait for signalling frame...\n");
				return 0;
			}//switch()
		}//if()
		
		//printf("signalling_offset: %d\n", signalling_offset);

		printf("\t\tSignalling ");
		switch(signalling_offset)
		{
		case ANSI_OFFSET:
			printf("ANSI");
			break;

		case Q933_OFFSET:
			if(lmi == 0){
				printf("Q.933");
			}else{
				printf("LMI");
			}
			break;
		default:
			printf("Unknown: 0x%X",signalling_offset);
		}
		printf("\n");
			
		printf("\t\t");
	     	if(data[signalling_offset]){
	    		printf("Link Verification ");
		}else{
	 	    	printf("Full Status ");
		}	
		
		if (data[5] == 0x75){
			printf("Req");
		}else{
			printf("Reply");
		}
			
		printf("\tSx=0x%02X  Rx=0x%02X  ", 
			(unsigned char)data[signalling_offset + 3],
		       	(unsigned char)data[signalling_offset + 4]);

	     		
		if (!data[signalling_offset] && (data[5]==0x7D) ) {
			/* full status reply */
			if(lmi == 0){
#if 0
				printf("\nFull Rep: Sig off=%i, Len=%i\n",
					signalling_offset,len);
#endif
				for( j=signalling_offset + 5; (j<len); j+=5){
					dlci = (unsigned short)data[j+2];
					dlci = (dlci << 4) | ((data[j+3]>>3) & 0x0F);

					printf("\n\t");
					printf("\tDLCI=%-3u ",dlci);
		
					if (data[j+4] & 0x08){
		       				printf("New, ");
					}else{
		       		   		printf("Present, ");
					}
					
			   		if (data[j+4] & 0x02){
			       		   	printf("Active ");
					}else{
		       			   	printf("Inactive ");
					}
				}//for()
			}else{
				
				for(j=signalling_offset + 5; ((j+8)<len); j+=8){
					dlci = ntohs(*(unsigned short*)&data[j+2]);
					
					printf("\n\t     :  ");
					printf("\tDLCI=%-3u ",dlci);

					if (data[j+4] & 0x08){
		       				printf("New, ");
					}else{
		       		   		printf("Present, ");
					}
					
			   		if (data[j+4] & 0x02){
			       		   	printf("Active ");
					}else{
		       			   	printf("Inactive ");
					}
				}
			}//if()
	 	}
		inf_frame=0;
		return inf_frame;
	}

	if ((data[2]==0x03 && data[3]==0xCC) || (data[2]==0x08 && data[3]==0x00)){
		iphdr_t *ip_hdr;
		inf_frame=1;
		
		ip_hdr = (iphdr_t*)&data[SIZEOF_FR_HEADER];	
		
		printf("\t\t%s encapsulated data packet.\n\n",	data[3] == 0xCC ? "IETF" : "Cisco");

		//decode data past FR header
		if(trace_iface->sub_type == WP_OUT_TRACE_INTERP_IPV4){
			decode_data_ipv4(data + SIZEOF_FR_HEADER, len - SIZEOF_FR_HEADER);
      		}

			
	}else if (data[2] == 0x03 &&
	          data[3] == 0x00 &&
	          data[4] == 0x80 &&
	          data[8] == 0x08 &&
	          data[9] == 0x06) {
	    
		/* Request packet */
		printf("\t     : ");
		if(data[17] == 0x08){
			printf("Inverse Arp Request on DLCI %u", dlci);
		}else if(data[17] == 0x09){
			printf("Inverse Arp Reply on DLCI %u", dlci);
		} 
	}else{
		printf("\t\tUnknown frame type on on DLCI %u", dlci);
	}

	return inf_frame;
}
#endif

static int decode_lapb(wp_trace_output_iface_t *trace_iface,
		       int *trace_started)
{
	unsigned char *data	=trace_iface->data;
	int len 		=trace_iface->len;

	int inf_frame=0;
	trace_banner(trace_iface,trace_started);
	
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

			case 0x83:
			        printf("SNRM P=%1i",
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
	
	return inf_frame;
}

static int decode_x25(wp_trace_output_iface_t *trace_iface,
		      int *trace_started)
{
	unsigned char *data	=trace_iface->data;
	int len 		=trace_iface->len;

	unsigned int lcn = ((data[0]&0x0F)<<8)|(data[1]&0xFF);
	unsigned char mod = (data[0]>>4)&0x03;
	unsigned char pkt_type;
	unsigned char pr,ps,mbit;
	unsigned char inf_frame=0;

	if (TRACE_X25_LCN){
		if (lcn != 0 && TRACE_X25_LCN != lcn){
			return inf_frame;
		}
	}

	pkt_type=data[2];

	if (!(pkt_type&0x01)){
		if (!(TRACE_X25_OPT & DATA)){
			return inf_frame;
		}
	}else{	
		if (!(TRACE_X25_OPT & PROT)){
			return inf_frame;
		}
	}
	
	trace_banner(trace_iface,trace_started);
	
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

	return inf_frame;
}


static void decode_annexg_x25_pkt (wp_trace_output_iface_t *trace_iface,
		                   int annexg)
{

	unsigned char *data	=trace_iface->data;
	int len 		=trace_iface->len;

	int inf_frame=0;
	int trace_started=0;
	int dlci;

	if (!TRACE_EBCDIC && !TRACE_ASCII && !TRACE_HEX){
		TRACE_HEX=1;
	}
	
	if (annexg){

		if (match_trace_criteria(data,len,&dlci) != 0)
			return;
		
		if (TRACE_PROTOCOL & FRAME){

			inf_frame=decode_fr(trace_iface,&trace_started,dlci);
			if (!inf_frame){
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
		inf_frame = decode_lapb(trace_iface,&trace_started);	
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
		decode_x25(trace_iface,&trace_started);
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
 	ph.ts_sec = trace_iface->sec;
        ph.ts_usec = trace_iface->usec;
        ph.incl_len =  trace_iface->len;
        ph.orig_len = trace_iface->len;

	fwrite(&ph, sizeof(ph), 1, trace_iface->output_file);
}

static void
get_ppp_magic_number(unsigned char* data, int len, char* outstr, int max_len, char offset_flag)
{
	if(offset_flag == 0){

		if(len < 12){
			printf("%s invalid buffer length. Line: %d\n", __FUNCTION__, __LINE__);
			return;
		}
		snprintf(outstr, max_len, "\tMagic# %02X%02X%02X%02X",
			(unsigned char)data[8], (unsigned char)data[9],
		       	(unsigned char)data[10], (unsigned char)data[11]);
	}else{

		if(len < 14){
			printf("%s invalid buffer length. Line: %d\n", __FUNCTION__, __LINE__);
			return;
		}
		snprintf(outstr, max_len, "\tMagic# %02X%02X%02X%02X", 
			(unsigned char)data[8+2], (unsigned char)data[8+3],
		       	(unsigned char)data[8+4], (unsigned char)data[8+5]);
	}
}

#define MAX_OUTSTR_LEN	1024
static int decode_ppp(wp_trace_output_iface_t *trace_iface,
		        int *trace_started)
{
	unsigned char *data = trace_iface->data;
	int len = trace_iface->len;
	int inf_frame=0;
	unsigned short tmp_ushort;
	char outstr[MAX_OUTSTR_LEN];
	
	trace_banner(trace_iface,trace_started);
	memset(outstr, 0x00, MAX_OUTSTR_LEN);
	
	//print the data (including PPP header)
	print_data_in_hex(data, len);

	printf("PPP decode\t");

	tmp_ushort = *(unsigned short*)&data[2];
	switch(tmp_ushort) {
	case 0x2100: // IP packet
		printf("IP-v4 data packet\n\n");
		decode_data_ipv4(&data[4], len - 4);
		break;
	case 0x2B00: // IPX packet
		printf("IPX data packet\n\n");
		break;
	case 0x5700:
		printf("IP-v6 data packet\n\n");
		break;
	case 0x2180: // IPCP-v4
	case 0x5780: // IPCP-v6
	case 0x2B80: // IPXCP
	case 0x21C0: // LCP

		//pre-decodeing
		if (tmp_ushort == 0x2180){
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "IPCP-v4 packet - ");
		}

		if (tmp_ushort == 0x5780){
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "IPCP-v6 packet - ");
		}

		if (tmp_ushort == 0x2B80){
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "IPXCP packet - ");
		}

		if (tmp_ushort == 0x21C0){
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "LCP packet\tID 0x%02X - ", data[5]);
		}

		//common decoding
		switch(data[4]) {
		case 1: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Configure Request");
			break;
		case 2: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Configure Ack\t");
			break;
		case 3: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Configure Nack\t");
			break;
		case 4: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Configure Reject");
			break;
		case 5: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Terminate Request");
			break;
		case 6: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Terminate Ack\t");
			break;
		case 7: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Code Reject\t");
			break;
		case 8: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Protocol Reject");
			break;
		case 9: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Echo Request\t");
			break;
		case 10: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Echo Reply\t");
			break;
		case 11: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Discard Request");
			break;
		default:
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Unknown type");
			break;
		}

		//post-decoding
		if (tmp_ushort == 0x21C0){
			//LCP
			switch(data[4]) {
			case 1: 
				get_ppp_magic_number(
					data, len, 
					outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), 1);
				break;
			case 2: 
				get_ppp_magic_number(
					data, len, 
					outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), 1);
				break;
			case 3: 
				get_ppp_magic_number(
					data, len, 
					outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), 1);
				break;
			case 4: 
				get_ppp_magic_number(
					data, len, 
					outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), 1);
				break;
			case 5: 
			case 6: 
			case 7: 
			case 8: 
				;//do nothing
				break;
			case 9: 
				get_ppp_magic_number(
					data, len, 
					outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), 0);
				break;
			case 10: 
				get_ppp_magic_number(
					data, len, 
					outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), 0);
				break;
			case 11: 
				get_ppp_magic_number(
					data, len, 
					outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), 0);
				break;
			default:
				break;
			}
		}
		break;

	case 0x23C0: // PAP
		snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "PAP packet - ");
		switch(data[4]) {
		case 1: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Authenticate Request");
			break;
		case 2: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Authenticate Ack");
			break;
		case 3: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Authenticate Nack");
			break;
		default:
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Unknown type");
			break;
		}
		break;
	case 0x25C0: // LQR
		snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Link Quality Report");
		break;
	case 0x23C2: // CHAP
		snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "CHAP packet - ");
		switch(data[4]) {
		case 1: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Challenge");
			break;
		case 2: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Responce");
			break;
		case 3: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Success");
			break;
		case 4: 
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Failure");
			break;
		default:
			snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Unknown type");
			break;
		}
		break;
	default:	// unknown
		snprintf(outstr+strlen(outstr), MAX_OUTSTR_LEN-strlen(outstr), "Unknown type");
		break;
	}

	printf(outstr);
	printf("\n\n");
	return inf_frame;
}


void wp_trace_output(wp_trace_output_iface_t *trace_iface)
{
	//int num_chars;
	//int j;
	int trace_started=0;
	uint8_t lapd_hdr[LAPD_SLL_LEN];
	uint8_t mtp2_hdr[MTP2_HDR_LEN];

	memset(&lapd_hdr, 0, sizeof(lapd_hdr));
	memset(&mtp2_hdr, 0, sizeof(mtp2_hdr));

try_trace_again:

	switch (trace_iface->type){

	case WP_OUT_TRACE_INTERP:

		switch (trace_iface->link_type){
		
		case 0:
		case WANCONFIG_AFT:
		case WANCONFIG_AFT_TE1:
		case WANCONFIG_AFT_TE3:
			if (trace_iface->data[0] == 0x8F || trace_iface->data[0] == 0x0F){
				trace_iface->link_type=WANCONFIG_CHDLC;
				goto try_trace_again;
			}else if (trace_iface->data[0] == 0xFF && trace_iface->data[1] == 0x03){
				trace_iface->link_type=WANCONFIG_PPP;
				goto try_trace_again;
			}else if (trace_iface->data[0] != 0x45){
				trace_iface->link_type=WANCONFIG_FR;
				goto try_trace_again;
			}else{
				printf("\nFailed to autodetect protocol (use -p option)\n");
			}
			
		case WANCONFIG_CHDLC://104

			if (trace_iface->data[0] == 0x8F || trace_iface->data[0] == 0x0F){
				//this is CHDLC
				decode_chdlc(trace_iface,&trace_started);
				printf("\n");
			}else if (trace_iface->data[0] == 0xFF && trace_iface->data[1] == 0x03){
				//on S514 LIP layer runs above CHDLC firmware
				trace_iface->link_type=WANCONFIG_PPP;
				goto try_trace_again;
			}else if (trace_iface->data[0] != 0x45){
				//on S514, LIP layer runs above CHDLC firmware
				trace_iface->link_type=WANCONFIG_FR;
				goto try_trace_again;
			}else{
				decode_annexg_x25_pkt(trace_iface,0);
			}
			break;

		case WANCONFIG_MPROT://107
		case WANCONFIG_FR://107
			{
			int dlci;
			match_trace_criteria(trace_iface->data,trace_iface->len,&dlci);
			decode_fr(trace_iface,&trace_started,dlci);
			printf("\n");
			}
			break;

		case 9:
		case WANCONFIG_PPP:
			/* ppp decoding */
			decode_ppp(trace_iface,&trace_started);
			break;

		case 200:
			decode_annexg_x25_pkt(trace_iface,0);
			break;

		case WANCONFIG_ATM://115
			printf("\n");
      			//print the data (past ATM header. header discarded on the card)
			print_data_in_hex(trace_iface->data, trace_iface->len);

      			if(trace_iface->sub_type == WP_OUT_TRACE_INTERP_IPV4){
				//decode data past ATM header
  				decode_data_ipv4(trace_iface->data, trace_iface->len);
      			}
      			break;
		
		default:
			printf("\nUnknown Configured Wanpipe Protocol 0x%X\n",trace_iface->link_type);
			break;
		}
		
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

			if (pcap_prot == 177) {
				printf("Starting PCAP Prot=ISDN Reference=%s File Trace in: %s\n\n",
					pcap_isdn_network?"Net":"CPE", pcap_output_file_name);
			} else if (pcap_prot == 139) {
				printf("Starting PCAP Prot=MTP2 File Trace in: %s\n\n",
					pcap_output_file_name);
			}else{				
				printf("Starting PCAP Prot=%i File Trace in: %s\n\n",
					pcap_prot, pcap_output_file_name);
			}
	
			trace_iface->init=1;
		}


		if (pcap_prot == 177) {
			 if (trace_iface->status) {
				/* Outgoing */
			 	phtons(&lapd_hdr[LAPD_SLL_PKTTYPE_OFFSET], 4); //pseudo_header->lapd.pkttype);
		 	 } else {
				/* Incoming */
			 	phtons(&lapd_hdr[LAPD_SLL_PKTTYPE_OFFSET], 0); //pseudo_header->lapd.pkttype);
			 }	
	               	 phtons(&lapd_hdr[LAPD_SLL_PROTOCOL_OFFSET], ETH_P_LAPD);

			 /* User specified Network or CPE */
                	 lapd_hdr[LAPD_SLL_ADDR_OFFSET + 0] = pcap_isdn_network; //pseudo_header->lapd.we_network?0x01:0x00;
	
			 trace_iface->len += sizeof(lapd_hdr);

		} else if (pcap_prot == 139) {
			if (trace_iface->status) {
				/* Outgoing */
				mtp2_hdr[MTP2_SENT_OFFSET] = 1;
		 	} else {
				/* Incoming */
				mtp2_hdr[MTP2_SENT_OFFSET] = 0;
			}	
			mtp2_hdr[MTP2_ANNEX_A_USED_OFFSET]=0;
			phtons(&mtp2_hdr[MTP2_LINK_NUMBER_OFFSET],
		    			0);
			 trace_iface->len += sizeof(mtp2_hdr);
		}

		print_pcap_record_header(trace_iface);

		if (pcap_prot == 177) {
			fwrite(&lapd_hdr[0], sizeof(lapd_hdr), 1, trace_iface->output_file);
			trace_iface->len -= sizeof(lapd_hdr);
		}	

		if (pcap_prot == 139) {
			fwrite(&mtp2_hdr[0], sizeof(mtp2_hdr), 1, trace_iface->output_file);
			trace_iface->len -= sizeof(mtp2_hdr);
		}

		fwrite(&trace_iface->data[0], trace_iface->len, 1, trace_iface->output_file);

		printf("\rTotal Captured=%-5u (Len=%i)",trace_iface->pkts_written+1,trace_iface->len);
		
		break;

	case WP_OUT_TRACE_HDLC:
		
                trace_banner(trace_iface,&trace_started);

		if (trace_iface->len == 0){
			printf("the frame data is not available" );
			return;
		}

		if (rx_hdlc_eng) {
			wanpipe_hdlc_decode(rx_hdlc_eng,trace_iface->data,trace_iface->len);
		}

		break;                                                
		

  case WP_OUT_TRACE_BIN:
  
   
#if 0
   if (trace_iface->len != 4800) {
    printf("ERROR: Trace Len %i\n",trace_iface->len );
   } 
#endif
	if (trace_iface->status) {
		  fwrite(&trace_iface->data[0], trace_iface->len, 1, trace_bin_out);
    } else {
		  fwrite(&trace_iface->data[0], trace_iface->len, 1, trace_bin_in);
    }

    break;
		
	case WP_OUT_TRACE_RAW:
	default:

		trace_banner(trace_iface,&trace_started);

		if (trace_iface->len == 0){
			printf("the frame data is not available" );
			return;
		}

		print_data_in_hex(trace_iface->data, trace_iface->len);

		break;     
	}

	trace_iface->pkts_written++;
	trace_iface->status=0;
}

int trace_hdlc_data(wanpipe_hdlc_engine_t *hdlc_eng, void *data, int len)
{
	print_data_in_hex(data, len);			
	return 0;
}
