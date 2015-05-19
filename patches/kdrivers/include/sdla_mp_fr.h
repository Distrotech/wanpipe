#ifndef __WANPIPE_MFR__
#define __WANPIPE_MFR__

#define HDLC_PROT_ONLY
#include <linux/sdla_chdlc.h>
#include <linux/sdla_fr.h>

#undef MAX_FR_CHANNELS
#undef HIGHEST_VALID_DLCI

#define MAX_FR_CHANNELS 1023
#define HIGHEST_VALID_DLCI  MAX_FR_CHANNELS-1

typedef struct {
	unsigned ea1  : 1;
	unsigned cr   : 1;
	unsigned dlcih: 6;
  
	unsigned ea2  : 1;
	unsigned de   : 1;
	unsigned becn : 1;
	unsigned fecn : 1;
	unsigned dlcil: 4;
}__attribute__ ((packed)) fr_hdr;


#define	FR_HEADER_LEN	8

#define LINK_STATE_RELIABLE 0x01
#define LINK_STATE_REQUEST  0x02 /* full stat sent (DCE) / req pending (DTE) */
#define LINK_STATE_CHANGED  0x04 /* change in PVCs state, send full report */
#define LINK_STATE_FULLREP_SENT 0x08 /* full report sent */

#define FR_UI              0x03
#define FR_PAD             0x00

#define NLPID_IP           0xCC
#define NLPID_IPV6         0x8E
#define NLPID_SNAP         0x80
#define NLPID_PAD          0x00
#define NLPID_Q933         0x08



/* 'status' defines */
#define	FR_LINK_INOPER	0x00		/* for global status (DLCI == 0) */
#define	FR_LINK_OPER	0x01

#if 0
#define FR_DLCI_INOPER  0x00
#define	FR_DLCI_DELETED	0x01		/* for circuit status (DLCI != 0) */
#define	FR_DLCI_ACTIVE	0x02
#define	FR_DLCI_WAITING	0x04
#define	FR_DLCI_NEW	0x08
#define	FR_DLCI_REPORT	0x40
#endif

#define PVC_STATE_NEW       0x01
#define PVC_STATE_ACTIVE    0x02
#define PVC_STATE_FECN	    0x08 /* FECN condition */
#define PVC_STATE_BECN      0x10 /* BECN condition */


#define LMI_ANSI_DLCI	0
#define LMI_LMI_DLCI	1023

#define LMI_PROTO               0x08
#define LMI_CALLREF             0x00 /* Call Reference */
#define LMI_ANSI_LOCKSHIFT      0x95 /* ANSI lockshift */
#define LMI_REPTYPE                1 /* report type */
#define LMI_CCITT_REPTYPE       0x51
#define LMI_ALIVE                  3 /* keep alive */
#define LMI_CCITT_ALIVE         0x53
#define LMI_PVCSTAT                7 /* pvc status */
#define LMI_CCITT_PVCSTAT       0x57
#define LMI_FULLREP                0 /* full report  */
#define LMI_INTEGRITY              1 /* link integrity report */
#define LMI_SINGLE                 2 /* single pvc report */
#define LMI_STATUS_ENQUIRY      0x75
#define LMI_STATUS              0x7D /* reply */

#define LMI_REPT_LEN               1 /* report type element length */
#define LMI_INTEG_LEN              2 /* link integrity element length */

#define LMI_LENGTH                13 /* standard LMI frame length */
#define LMI_ANSI_LENGTH           14

#define HDLC_MAX_MTU 1500	/* Ethernet 1500 bytes */
#define HDLC_MAX_MRU (HDLC_MAX_MTU + 10) /* max 10 bytes for FR */

#define MAX_TRACE_QUEUE 	  100
#define TRACE_QUEUE_LIMIT	  1001
#define	MAX_TRACE_TIMEOUT	  (HZ*10)

static __inline__ u16 status_to_dlci(u8 *status, u8 *state)
{
	*state &= ~(PVC_STATE_ACTIVE | PVC_STATE_NEW);
	if (status[2] & 0x08)
		*state |= PVC_STATE_NEW;
	else if (status[2] & 0x02)
		*state |= PVC_STATE_ACTIVE;

	return ((status[0] & 0x3F)<<4) | ((status[1] & 0x78)>>3);
}



static __inline__ u16 q922_to_dlci(u8 *hdr)
{
        return ((hdr[0] & 0xFC)<<2) | ((hdr[1] & 0xF0)>>4);
}


static inline u8 fr_lmi_nextseq(u8 x)
{
	x++;
	return x ? x : 1;
}

static __inline__ void dlci_to_q922(u8 *hdr, u16 dlci)
{
        hdr[0] = (dlci>>2) & 0xFC;
        hdr[1] = ((dlci<<4) & 0xF0) | 0x01;
}


typedef struct {

	struct tasklet_struct wanpipe_task;
	unsigned long 	tq_working;
	netdevice_t	*dlci_to_dev_map[MAX_FR_CHANNELS];
	unsigned char   global_dlci_map[MAX_FR_CHANNELS];
	wan_fr_conf_t 	cfg;	
	unsigned char 	station;
	struct sk_buff_head rx_free;
	struct sk_buff_head rx_used;
	struct sk_buff_head lmi_queue;
	struct sk_buff_head trace_queue;
	unsigned long 	last_rx_poll;
	unsigned int 	txseq, rxseq;
	unsigned char 	state;
	unsigned long 	n391cnt;
	unsigned int 	last_errors;
	struct timer_list timer;
	unsigned short  lmi_dlci;
	netdevice_t 	*tx_dev;

	fr_link_stat_t  link_stats;
	void 		*update_dlci;
	unsigned long	tracing_enabled;
	int		max_trace_queue;
	unsigned long   trace_timeout;

	unsigned int	max_rx_queue;
} fr_prot_t;

#endif
