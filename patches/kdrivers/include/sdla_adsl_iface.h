#ifndef _ADSL_OS_ABSTR_H_
#define _ADSL_OS_ABSTR_H_

/*
**		DEFINED/MACROS	
*/
#ifdef __ADSL_IFACE
# define EXTERN_ADSL	extern
# define EXTERN_WAN
#else
# define EXTERN_ADSL
# define EXTERN_WAN	extern
#endif

#define TRC_INCOMING_FRM		0x00
#define TRC_OUTGOING_FRM		0x01

#define	ADSL_BAUD_RATE			0x04
#define	ADSL_ATM_CONF			0x05
#define	ADSL_OP_STATE			0x09
#define	ADSL_COUNTERS			0x0A
#define	ADSL_LAST_FAILED_STATUS		0x0B
#define	ADSL_LCL_SNR_MARGIN		0x0C
#define	ADSL_UPSTREAM_MARGIN_STATUS	0x0D
#define	ADSL_FAILURES			0x0E			
#define	ADSL_ATTENUATION_STATUS		0x0F		
#define ADSL_RMT_VENDOR_ID_STATUS	0x10
#define ADSL_GET_MODULATION		0x11
#define ADSL_START_PROGRESS		0x12
#define ADSL_LCL_XMIT_POWER_STATUS	0x13
#define ADSL_RMT_TX_POWER_STATUS	0x14
#define ADSL_ACTUAL_CONFIGURATION	0x15
#define ADSL_ACTUAL_INTERLEAVE_STATUS	0x16
#define ADSL_ATM_CELL_COUNTER		0x17
#define ADSL_EEPROM_WRITE		0x18
#define ADSL_EEPROM_READ		0x19


typedef struct adsl_failures
{
	unsigned long	CrcErrorsPerMin;
	unsigned long	ExcessiveConsecutiveCrcErrorsPerTickCount;
	unsigned long	ExcessiveConsecutiveCrcErrorsPerMinCount;
	unsigned long	ExcessiveConsecutiveOverallFailureCount;
}adsl_failures_t;

/*
**		FUNCTION PROTOTYPES
*/

EXTERN_WAN  int		adsl_can_tx(void*);
EXTERN_WAN  int 	adsl_send(void* adapter_id, void* tx_skb, unsigned int Flag);
EXTERN_ADSL void	adsl_lan_rx(void*,void*,unsigned long,unsigned char*, int);

EXTERN_ADSL void	adsl_tx_complete(void*, int, int);
EXTERN_WAN  void	adsl_rx_complete(void*);

EXTERN_ADSL void 	adsl_tty_receive(void *, unsigned char *, 
		             		 unsigned char *,unsigned int);

EXTERN_ADSL void	adsl_wan_soft_intr(void *, unsigned int, unsigned long*);
EXTERN_ADSL void* 	adsl_ttydriver_alloc(void);
EXTERN_ADSL void 	adsl_ttydriver_free(void*);
EXTERN_ADSL void* 	adsl_termios_alloc(void);

EXTERN_ADSL int 	adsl_wan_register(void *,
					  char *,
		             	  	unsigned char);

EXTERN_ADSL void 	adsl_wan_unregister(unsigned char);


EXTERN_ADSL int		adsl_tracing_enabled(void*);
EXTERN_ADSL int		adsl_trace_enqueue(void*, void*);
EXTERN_ADSL int		adsl_trace_purge(void*);
EXTERN_ADSL void* 	adsl_trace_info_alloc(void);
EXTERN_ADSL void 	adsl_trace_info_init(void *trace_ptr);


EXTERN_WAN  void* 	adsl_create(void*, void*, char*);
EXTERN_WAN  void* 	adsl_new_if(void*, unsigned char*, void*);
EXTERN_WAN  int 	adsl_wan_init(void*);
EXTERN_WAN  int		adsl_del_if(void*);
EXTERN_WAN  void	adsl_timeout(void*);
EXTERN_WAN  void	adsl_disable_comm(void*);
EXTERN_WAN  int		adsl_isr(void*);
EXTERN_WAN  void	adsl_udp_cmd(void*, unsigned char, unsigned char*, unsigned short*);
EXTERN_WAN  void* 	adsl_get_trace_ptr(void *pAdapter_ptr);
EXTERN_WAN  int 	adsl_wan_interface_type(void *);

EXTERN_WAN  int	 	GpWanOpen(void *pAdapter_ptr, unsigned char line, void *tty, void **data);
EXTERN_WAN  int 	GpWanClose(void *pAdapter_ptr, void *pChan_ptr);
EXTERN_WAN  int 	GpWanTx(void *pChan_ptr, int fromUser, const unsigned char *buffer, int bufferLen);
EXTERN_WAN  int 	GpWanWriteRoom(void *pChan_ptr);

EXTERN_ADSL int		adsl_detect_prot_header(unsigned char*,int,char*);
EXTERN_ADSL void	adsl_tty_hangup(void*);
 
#endif
