#ifdef WANPIPEMON_GUI 
#include "../lxdialog/dialog.h"
#endif

extern int 	output_start_xml_router (void);
extern int 	output_stop_xml_router (void);
extern int 	output_start_xml_header (char *hdr);
extern int 	output_stop_xml_header (void);

extern void 	output_xml_val_data (char *value_name, int value);
extern void 	output_xml_val_asc (char *value_name, char * value);
extern void 	output_error(char *value);

extern void 	flush_te1_pmon(void);
extern void 	read_te1_56k_stat(void);
extern void	banner(char *, int);
extern int DoCommand(wan_udp_hdr_t*);

extern void CHDLC_set_FT1_mode(void);
extern void ATM_set_FT1_mode(void);
extern void FR_set_FT1_mode(void);
extern void PPP_set_FT1_mode(void);
extern void CHDLC_read_FT1_status(void);
extern void FR_read_FT1_status(void);
extern void PPP_read_FT1_status(void);
extern void ATM_read_FT1_status(void);

extern int sock;
extern char i_name[];
extern FT1_LED_STATUS FT1_LED;
extern int raw_data;
extern int fail;
extern int xml_output;
extern int trace_all_data;
extern short dlci_number;
extern int start_xml_router;
extern int stop_xml_router;
extern int is_508;
extern wan_udp_hdr_t wan_udp;
extern unsigned char par_port_A_byte, par_port_B_byte;
extern int lcn_number;
extern void ResetWanUdp(wan_udp_hdr_t *wan);
extern int protocol_cb_size;
extern char if_name[];
extern int ip_addr;
extern char ipaddress[];
extern int udp_port;
extern int wan_protocol;
extern int annexg_trace;

extern char *csudsu_menu[];

extern int pcap_output;
extern int pcap_prot;
extern FILE *pcap_output_file;
extern char pcap_output_file_name[];


#define MAX_CMD_ARG 10
#define DEFAULT_TRACE_LEN 39
#define DO_COMMAND(packet)	DoCommand((wan_udp_hdr_t*)&packet)

#define UDP_CHDLC_SIGNATURE	"CTPIPEAB"
#define UDP_PPP_SIGNATURE 	"PTPIPEAB"
#define UDP_FR_SIGNATURE        "FPIPE8ND"
#define UDP_X25_SIGNATURE       "XLINK8ND"

/* Trace delay value in micro seconds */
#define WAN_TRACE_DELAY		20000  

struct cmd_menu_lookup_t
{
	char *cmd_menu_name;
	char **cmd_menu_ptr;
};

extern char ** FRget_main_menu(int *len);
extern char ** FRget_cmd_menu(char *cmd_name,int *len);
extern char ** CHDLCget_main_menu(int *len);
extern char ** CHDLCget_cmd_menu(char *cmd_name,int *len);
extern char ** PPPget_main_menu(int *len);
extern char ** PPPget_cmd_menu(char *cmd_name,int *len);
#if defined(__LINUX__)
extern char ** X25get_main_menu(int *len);
extern char ** X25get_cmd_menu(char *cmd_name,int *len);

extern char ** SS7get_main_menu(int *len);
extern char ** SS7get_cmd_menu(char *cmd_name,int *len);

extern char ** BITSTRMget_main_menu(int *len);
extern char ** BITSTRMget_cmd_menu(char *cmd_name,int *len);


#endif
extern char ** ADSLget_main_menu(int *len);
extern char ** ATMget_main_menu(int *len);
extern char ** AFTget_main_menu(int *len);
extern char ** ADSLget_cmd_menu(char *cmd_name,int *len);
extern char ** ATMget_cmd_menu(char *cmd_name,int *len);
extern char ** AFTget_cmd_menu(char *cmd_name,int *len);

typedef int	config_t(void);
typedef int 	usage_t(void);
typedef int 	main_t(char*,int,char**);
typedef int 	dis_trace_t(void);
typedef char **	main_menu_t(int *len);
typedef char ** cmd_menu_t(char *cmd_name,int *len);	
typedef void	ft1_cfg_t(void);

struct fun_protocol {
	int		protocol_id;
	char   prot_name[10];
	config_t*	config;
	usage_t*	usage;
	main_t*		main;
	dis_trace_t*	disable_trace;
	main_menu_t*	get_main_menu;
	cmd_menu_t*	get_cmd_menu;
	ft1_cfg_t*	read_FT1_status;
	ft1_cfg_t*	set_FT1_mode;
	unsigned char	mbox_offset;
};

extern int 		wan_main_gui(void);
extern config_t		CHDLCConfig;
extern config_t 	FRConfig;
extern config_t 	PPPConfig;
extern config_t 	ADSLConfig;
extern config_t 	ATMConfig;
extern config_t 	AFTConfig;

#if defined(__LINUX__)
extern config_t		X25Config;
extern config_t		SS7Config;
extern config_t		BITSTRMConfig;
#endif

extern usage_t		ATMUsage;
extern usage_t		AFTUsage;
extern usage_t		CHDLCUsage;
extern usage_t		FRUsage;
extern usage_t		PPPUsage;
extern usage_t		ADSLUsage;
#if defined(__LINUX__)
extern usage_t		X25Usage;
extern usage_t		SS7Usage;
extern usage_t		BITSTRMUsage;
#endif

extern main_t		CHDLCMain;
extern main_t		ATMMain;
extern main_t		AFTMain;
extern main_t		FRMain;
extern main_t		PPPMain;
extern main_t		ADSLMain;
#if defined(__LINUX__)
extern main_t	 	SS7Main;
extern main_t		X25Main;
extern main_t		BITSTRMMain;
#endif

extern dis_trace_t	CHDLCDisableTrace;
extern dis_trace_t	FRDisableTrace;
extern dis_trace_t	PPPDisableTrace;
extern dis_trace_t	ADSLDisableTrace;
extern dis_trace_t	ATMDisableTrace;
extern dis_trace_t	AFTDisableTrace;
#if defined(__LINUX__)
extern dis_trace_t	SS7DisableTrace;
extern dis_trace_t	X25DisableTrace;
#endif

extern struct fun_protocol function_lookup[];

#define DECODE_PROT(prot)((prot==WANCONFIG_FR)?"Frame Relay": \
			  (prot==WANCONFIG_MFR)?"Frame Relay": \
		          (prot==WANCONFIG_PPP)?"PPP Point-to-Point":\
		          (prot==WANCONFIG_CHDLC)?"Cisco HDLC": \
		          (prot==WANCONFIG_X25)?"X25": \
		          (prot==WANCONFIG_ADSL)?"ADSL: Eth,IP,PPP/ATM":\
			  (prot==WANCONFIG_ATM)?"ATM":\
			  (prot==WANCONFIG_AFT)?"AFT":\
			  (prot==WANCONFIG_AFT_TE1)?"AFT_TE1":\
			  (prot==WANCONFIG_AFT_TE3)?"AFT_TE3":\
							"Unknown")

#define EXEC_PROT_FUNC(func,prot,err,arg) {\
	int i=0; \
	err=WAN_FALSE; \
	for(;;){ \
		if (function_lookup[i].protocol_id == 0){\
			printf("\nProtocol: %s support not compiled in!\n\n", \
					DECODE_PROT(prot)); \
			break;\
		}\
		\
		if (function_lookup[i].protocol_id == prot){\
			if (function_lookup[i].func){\
				err=function_lookup[i].func arg;\
				break; \
			}\
			printf("\nProtocol: %s support not compiled in!\n\n", \
					DECODE_PROT(prot)); \
			break;\
		}\
		i++;\
	}\
}

#define EXEC_PROT_VOID_FUNC(func,prot,arg) {\
	int i=0; \
	for(;;){ \
		if (function_lookup[i].protocol_id == 0){\
			printf("\nProtocol: %s support not compiled in!\n\n", \
					DECODE_PROT(prot)); \
			break;\
		}\
		\
		if (function_lookup[i].protocol_id == prot){\
			if (function_lookup[i].func){\
				function_lookup[i].func arg;\
				break; \
			}\
			printf("\nProtocol: %s support not compiled in!\n\n", \
					DECODE_PROT(prot)); \
			break;\
		}\
		i++;\
	}\
}

#define EXEC_NAME_FUNC(func,name,arg) {\
	int i=0; \
	for(;;){ \
		if (function_lookup[i].protocol_id == 0){\
			printf("\nProtocol: '%s' support not compiled in!\n\n", \
					name); \
			break;\
		}\
		\
		if (strcmp(function_lookup[i].prot_name,name)==0){\
			if (function_lookup[i].func){\
				function_lookup[i].func arg;\
				break;\
			}\
			printf("\nProtocol: '%s' support not compiled in!\n\n", \
					name); \
			break;\
		}\
		i++;\
	}\
}


static inline unsigned char get_wan_udphdr_data_byte(unsigned char off) 
{
	int i=0; 
	for (;;){
		if (function_lookup[i].protocol_id == 0){
			printf("\nProtocol: '%s' support not compiled in!\n\n", 
					DECODE_PROT(wan_protocol)); 
			break;
		};
	
		if (function_lookup[i].protocol_id == wan_protocol){

#if 0
			printf("Ptr %p  Off %i New Ptr %p Chdlc %p\n",
					&wan_udp.wan_udphdr_data[0],
				function_lookup[i].mbox_offset,
				(((unsigned char*)&wan_udp.wan_udphdr_data[0])+
				function_lookup[i].mbox_offset),
				wan_udp.wan_udphdr_chdlc_data);
#endif			
			return (((unsigned char*)&wan_udp.wan_udphdr_data[0])+
				function_lookup[i].mbox_offset)[off];
		}
		i++;
	}

	return 0;
}

static inline unsigned char *get_wan_udphdr_data_ptr(unsigned char off) 
{
	int i=0; 
	for (;;){
		if (function_lookup[i].protocol_id == 0){
			printf("\nProtocol: '%s' support not compiled in!\n\n", 
					DECODE_PROT(wan_protocol)); 
			break;
		};
	
		if (function_lookup[i].protocol_id == wan_protocol){

			return &(((unsigned char*)&wan_udp.wan_udphdr_data[0])+
				function_lookup[i].mbox_offset)[off];
		}
		i++;
	}

	return 0;
}



static inline unsigned char set_wan_udphdr_data_byte(unsigned char off, unsigned char data) 
{
	int i=0; 
	for (;;){
		if (function_lookup[i].protocol_id == 0){
			printf("\nProtocol: '%s' support not compiled in!\n\n", 
					DECODE_PROT(wan_protocol)); 
			break;
		};
	
		if (function_lookup[i].protocol_id == wan_protocol){

			(((unsigned char*)&wan_udp.wan_udphdr_data[0])+
				function_lookup[i].mbox_offset)[off] = data;

			return 0;
		}
		i++;
	}

	return -1;
}
		
#define FRAME 0x01
#define LAPB  0x02
#define X25   0x04
#define PPP   0x08
#define CHDLC 0x10
#define ETH   0x20
#define IP    0x40
#define ALL_PROT (FRAME|LAPB|X25|PPP|CHDLC);

#define DATA 0x1
#define PROT 0x2
#define ALL_X25 (DATA|PROT)

typedef struct {
	unsigned char prot_name[15];
	signed char prot_index;
	unsigned int  pcap_prot;
} trace_prot_t;


/* "libpcap" file header (minus magic number). */
struct pcap_hdr {
    unsigned long    	magic;          /* magic */
    unsigned short	version_major;	/* major version number */
    unsigned short	version_minor;	/* minor version number */
    unsigned long	thiszone;	/* GMT to local correction */
    unsigned long	sigfigs;	/* accuracy of timestamps */
    unsigned long	snaplen;	/* max length of captured packets, in octets */
    unsigned long	network;	/* data link type */
};

/* "libpcap" record header. */
struct pcaprec_hdr {
    unsigned long	ts_sec;		/* timestamp seconds */
    unsigned long	ts_usec;	/* timestamp microseconds */
    unsigned long	incl_len;	/* number of octets of packet saved in file */
    unsigned long	orig_len;	/* actual length of packet */
};



enum {
	WP_OUT_TRACE_RAW,
	WP_OUT_TRACE_INTERP,
	WP_OUT_TRACE_PCAP,
	WP_OUT_TRACE_INTERP_IPV4,
	WP_OUT_TRACE_ATM_RAW_PHY,
	WP_OUT_TRACE_ATM_INTERPRETED_PHY
};

#define WP_TRACE_OUTGOING 0x01
#define WP_TRACE_INCOMING 0x02
#define WP_TRACE_ABORT    0x04
#define WP_TRACE_CRC	  0x08
#define WP_TRACE_OVERRUN  0x10


typedef struct {
	unsigned char *data;
	unsigned int len;
	unsigned int status;
	unsigned short timestamp;
	unsigned long  systimestamp;
	unsigned int type;
	unsigned int link_type;
	unsigned int prot_criteria;
	unsigned char trace_all_data;
	unsigned int pkts_written;
	unsigned char init;
	time_t	 sec,usec;
	FILE *output_file;
	unsigned int sub_type;
} wp_trace_output_iface_t;



#pragma pack(1)
typedef struct {
        unsigned char link_address;     /* Broadcast or Unicast */
        unsigned char control;
        unsigned short packet_type;     /* IP/SLARP/CDP */
} cisco_header_t;

typedef struct {
        unsigned long code;     /* Slarp Control Packet Code */
        union {
                struct {
                        unsigned long address;  /* IP address */
                        unsigned long mask;     /* IP mask    */
                        unsigned short reserved[3];
                } address;
                struct {
                        unsigned long my_sequence;
                        unsigned long your_sequence;
                        unsigned short reliability;
                        unsigned short t1;      /* time alive (upper) */
                        unsigned short t2;      /* time alive (lower) */
                } keepalive;
        } un;
} cisco_slarp_t;
#pragma pack()

/* Link addresses */
#define CISCO_ADDR_UNICAST      0x0F
#define CISCO_ADDR_BCAST        0x8F

/* Packet Types */
#define CISCO_PACKET_IP         0x0800
#define CISCO_PACKET_SLARP      0x8035
#define CISCO_PACKET_CDP        0x2000

/* Packet Codes */
#define SLARP_REQUEST   0
#define SLARP_REPLY     1
#define SLARP_KEEPALIVE 2




//extern void decode_pkt (unsigned char *data,int len,
//		        unsigned char status, unsigned int timestamp, int protocol);


extern void wp_trace_output(wp_trace_output_iface_t *trace_iface);


	
