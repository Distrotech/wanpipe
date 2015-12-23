/*
 * These are the public elements of the Linux XDLC module.
 */

#ifndef	_WP_XDLC_IFACE_H
#define	_WP_XDLC_IFACE_H

enum xdlc_error_codes{
	EXCEP_LINK_CONNECTED 	  = 0,
	EXCEP_LINK_CONNECTING,
	EXCEP_LINK_DISCONNECTED,
	EXCEP_LINK_DISCONNECTING,

	EXCEP_OOB_EXCEPTIONS,
	
	EXCEP_NO_RESPONSE_COUNTER,
	EXCEP_FRM_RETRANS_COUNTER,
	
	EXCEP_RNR_COUNTER,
	EXCEP_SEC_NRM_TIMEOUT,
	
	EXCEP_PRI_RD_FRAME_RECEIVED,
	EXCEP_FRM_DM_FRAME_RECEIVED,
	EXCEP_SEC_SNRM_FRAME_RECEIVED,
	EXCEP_SEC_DISC_FRAME_RECEIVED,
	
	EXCEP_FRMR_FRAME_RECEIVED,
	EXCEP_FRMR_FRAME_TRANSMITTED
};

#define XDLC_STATE_DECODE(_state_) (\
	(_state_==EXCEP_LINK_CONNECTED)?"Connected": \
	(_state_==EXCEP_LINK_CONNECTING)?"Connecting": \
	(_state_==EXCEP_LINK_DISCONNECTED)?"Disconnected": \
	(_state_==EXCEP_LINK_DISCONNECTING)?"Disconnecting": "Unknown" )


#define XDLC_EXCEP_DECODE(_error_) (\
	(_error_==EXCEP_NO_RESPONSE_COUNTER)?"No Response Timeout": \
	(_error_==EXCEP_FRM_RETRANS_COUNTER)?"Frame Re-Transmit Timeout": \
	\
	(_error_==EXCEP_RNR_COUNTER)?"Consec RNR Rx Timeout": \
	(_error_==EXCEP_SEC_NRM_TIMEOUT)?"Sec NRM Timeout": \
	\
	(_error_==EXCEP_PRI_RD_FRAME_RECEIVED)?"Pri RD Received": \
	(_error_==EXCEP_FRM_DM_FRAME_RECEIVED)?"DM Frame Received": \
	(_error_==EXCEP_SEC_SNRM_FRAME_RECEIVED)?"Sec SNRM Received": \
	(_error_==EXCEP_SEC_DISC_FRAME_RECEIVED)?"Sec DISC Received": \
	\
	(_error_==EXCEP_FRMR_FRAME_RECEIVED)?"Frame Reject Received": \
	(_error_==EXCEP_FRMR_FRAME_TRANSMITTED)?"Frame Reject Transmiteed" : "None")

enum xdlc_ioctl_cmds {
	SIOCS_XDLC_SET_CONF,
	SIOCS_XDLC_GET_CONF,

	SIOCS_XDLC_START,
	SIOCS_XDLC_STOP,
	
	SIOCS_XDLC_ENABLE_IFRAMES,
	SIOCS_XDLC_DISABLE_IFRAMES,

	SIOCS_XDLC_CLEAR_POLL_TMR,
	SIOCS_XDLC_FLUSH_TX_BUF,

	SIOCS_XDLC_GET_STATS,
	SIOCS_XDLC_FLUSH_STATS
};

typedef struct{
	unsigned char state;
	unsigned char address;
	unsigned short exception;
}wp_xdlc_excep_t;

#if 0
typedef struct wp_xdlc_reg
{
	int (*prot_state_change) (void *, int, unsigned char*, int);
	int (*chan_state_change) (void *, int, unsigned char*, int);
	int (*tx_down)		 (void *, void *);
	int (*rx_up) 	    	 (void *, void *);
}wp_xdlc_reg_t;
#endif

extern void 	*wp_reg_xdlc_prot(void *link_ptr, 
				  char *devname,
				  void *conf,
				  wplip_prot_reg_t *prot_reg);

extern int 	wp_unreg_xdlc_prot(void *xdlc_link_ptr);

extern void 	*wp_reg_xdlc_addr(void *,
				  void *, 
		 	          char *, 
		                  void *,
				  unsigned char);

extern int 	wp_unreg_xdlc_addr(void *);

extern int 	wp_xdlc_open(void *);
extern int 	wp_xdlc_close(void *);
extern int 	wp_xdlc_rx(void *, void *);
extern int 	wp_xdlc_rx_kick(void *);
extern int 	wp_xdlc_bh(void *);
extern int 	wp_xdlc_tx(void *, void *, int type);
extern int 	wp_xdlc_timer(void *, unsigned int *period, unsigned int);
extern int	wp_xdlc_ioctl(void *, int, void *);


#endif
