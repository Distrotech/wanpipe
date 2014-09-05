/***********************************************************
 * wanpipe_fr_iface.h  Wanpipe Multi Protocol Interface
 *
 *
 *
 *
 */

#ifndef _WANPIPE_FR_IFACE_H_
#define _WANPIPE_FR_IFACE_H_

#ifdef WANLIP_DRIVER

#if 0
typedef struct wp_fr_reg
{
	int (*prot_set_state)(void* link_dev, int state, unsigned char *, int reason);
	int (*chan_set_state)(void* chan_dev, int state, unsigned char *, int reason);
	int (*tx_down)	     (void *, void *);
	int (*rx_up) 	     (void *, void *, int);
	int mtu;
}wp_fr_reg_t;
#endif

#define wplist_insert_dev(dev, list)	do{\
	                                   dev->next = list;\
					   list = dev;\
                                        }while(0)

#define wplist_remove_dev(dev,cur,prev,list)\
					do{\
	                                   if ((cur=list) == dev){\
						list=cur->next;\
						break;\
					   }else{\
						while (cur!=NULL && cur->next!=NULL){\
							prev=cur;\
							cur=cur->next;\
							if (cur==dev){\
								prev->next=cur->next;\
								break;\
							}\
						}\
					   }\
                                        }while(0)



extern void *wp_register_fr_prot(void *link_ptr, 
				 char *devname, 
				 void *cfg, 
				 wplip_prot_reg_t *fr_reg);

extern int wp_unregister_fr_prot(void *prot_ptr);

extern void *wp_register_fr_chan(void *if_ptr, 
		                 void *prot_ptr, 
				 char *devname,
				 void *cfg,
				 unsigned char type);

extern int wp_unregister_fr_chan(void *chan_ptr);

extern int wp_fr_open_chan (void *chan_ptr);
extern int wp_fr_close_chan(void *chan_ptr);
extern int wp_fr_ioctl     (void *chan_ptr, int cmd, void *arg);

extern int wp_fr_rx     (void * prot_ptr, void *rx_pkt);
extern int wp_fr_timer  (void *prot_ptr, unsigned int *period, unsigned int carrier_reliable);
extern int wp_fr_tx 	(void * chan_ptr, void *skb, int type);
extern int wp_fr_pipemon(void *chan, int cmd, int dlci, unsigned char* data, unsigned int *len);
extern int wp_fr_snmp(void* chan_ptr, void* data);

#endif


/* 'command' field defines */
#define	FR_WRITE		0x01
#define	FR_READ			0x02
#define	FR_ISSUE_IS_FRAME	0x03
#define FR_SET_CONFIG		0x10
#define FR_READ_CONFIG		0x11
#define FR_COMM_DISABLE		0x12
#define FR_COMM_ENABLE		0x13
#define FR_READ_STATUS		0x14
#define FR_READ_STATISTICS	0x15
#define FR_FLUSH_STATISTICS	0x16
#define	FR_LIST_ACTIVE_DLCI	0x17
#define FR_FLUSH_DATA_BUFFERS	0x18
#define FR_READ_ADD_DLC_STATS	0x19
#define	FR_ADD_DLCI		0x20
#define	FR_DELETE_DLCI		0x21
#define	FR_ACTIVATE_DLCI	0x22
#define	FR_DEACTIVATE_DLCI	0x22
#define FR_READ_MODEM_STATUS	0x30
#define FR_SET_MODEM_STATUS	0x31
#define FR_READ_ERROR_STATS	0x32
#define FR_FLUSH_ERROR_STATS	0x33
#define FR_READ_DLCI_IB_MAPPING 0x34
#define FR_READ_CODE_VERSION	0x40
#define	FR_SET_INTR_MODE	0x50
#define	FR_READ_INTR_MODE	0x51
#define FR_SET_TRACE_CONFIG	0x60
#define FR_FT1_STATUS_CTRL 	0x80
#define FR_SET_FT1_MODE		0x81
#define	FR_LIST_CONFIGURED_DLCIS 0x82


#pragma pack(1)

/*----------------------------------------------------------------------------
 * Global Statistics Block.
 *	This structure is returned by the FR_READ_STATISTICS command when
 *	dcli == 0.
 */
typedef struct	fr_link_stat
{
	unsigned short rx_too_long	;	/* 00h:  */
	unsigned short rx_dropped	;	/* 02h:  */
	unsigned short rx_dropped2	;	/* 04h:  */
	unsigned short rx_bad_dlci	;	/* 06h:  */
	unsigned short rx_bad_format	;	/* 08h:  */
	unsigned short retransmitted	;	/* 0Ah:  */
	unsigned short cpe_tx_FSE	;	/* 0Ch:  */
	unsigned short cpe_tx_LIV	;	/* 0Eh:  */
	unsigned short cpe_rx_FSR	;	/* 10h:  */
	unsigned short cpe_rx_LIV	;	/* 12h:  */
	unsigned short node_rx_FSE	;	/* 14h:  */
	unsigned short node_rx_LIV	;	/* 16h:  */
	unsigned short node_tx_FSR	;	/* 18h:  */
	unsigned short node_tx_LIV	;	/* 1Ah:  */
	unsigned short rx_ISF_err	;	/* 1Ch:  */
	unsigned short rx_unsolicited	;	/* 1Eh:  */
	unsigned short rx_SSN_err	;	/* 20h:  */
	unsigned short rx_RSN_err	;	/* 22h:  */
	unsigned short T391_timeouts	;	/* 24h:  */
	unsigned short T392_timeouts	;	/* 26h:  */
	unsigned short N392_reached	;	/* 28h:  */
	unsigned short cpe_SSN_RSN	;	/* 2Ah:  */
	unsigned short current_SSN	;	/* 2Ch:  */
	unsigned short current_RSN	;	/* 2Eh:  */
	unsigned short curreny_T391	;	/* 30h:  */
	unsigned short current_T392	;	/* 32h:  */
	unsigned short current_N392	;	/* 34h:  */
	unsigned short current_N393	;	/* 36h:  */
} fr_link_stat_t;

/*----------------------------------------------------------------------------
 * DLCI statistics.
 *	This structure is returned by the FR_READ_STATISTICS command when
 *	dlci != 0.
 */
typedef struct	fr_dlci_stat
{
	unsigned int tx_frames		;	/* 00h:  */
	unsigned int tx_bytes		;	/* 04h:  */
	unsigned int rx_frames		;	/* 08h:  */
	unsigned int rx_bytes		;	/* 0Ch:  */
	unsigned int rx_dropped	;	/* 10h:  */
	unsigned int rx_inactive	;	/* 14h:  */
	unsigned int rx_exceed_CIR	;	/* 18h:  */
	unsigned int rx_DE_set		;	/* 1Ch:  */
	unsigned int tx_throughput	;	/* 20h:  */
	unsigned int tx_calc_timer	;	/* 24h:  */
	unsigned int rx_throughput	;	/* 28h:  */
	unsigned int rx_calc_timer	;	/* 2Ch:  */
} fr_dlci_stat_t;

#pragma pack()

#endif
