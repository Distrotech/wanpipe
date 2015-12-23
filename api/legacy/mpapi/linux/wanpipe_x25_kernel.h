/*
 * These are the public elements of the Linux X25 module.
 */

#ifndef	WANPIPE_X25_KERNEL_H
#define	WANPIPE_X25_KERNEL_H

#include <linux/if_wanpipe.h>
#include <linux/wanpipe_debug.h>
#include <linux/wanpipe_cfg.h>
#include <linux/if_wanpipe_common.h>

#define	X25_OK			0
#define	X25_BADTOKEN		1
#define	X25_INVALUE		2
#define	X25_CONNECTED		3
#define	X25_NOTCONNECTED	4
#define	X25_REFUSED		5
#define	X25_TIMEDOUT		6
#define	X25_NOMEM		7

#define	X25_STANDARD		0x00
#define	X25_EXTENDED		0x01

#define	X25_SLP		0x00
#define	X25_MLP		0x02

#define	X25_DTE		0x00
#define	X25_DCE		0x04

#define	WAN_ADDRESS_SZ	31
#define X25_CALL_STR_SZ 512
#define	WAN_IF_LABEL_SZ	15

#define M_BIT	1
#define CLEAR_NO_WAIT 0x00
#define CLEAR_WAIT_FOR_DATA 0x80

/* X.25 packet types */
#define DATA_PKT				0x00
#define RR_PKT					0x01
#define RNR_PKT					0x05
#define REJ_PKT					0x09
#define CALL_REQUEST_PKT		  	0x0B
#define CALL_ACCEPT_PKT				0x0F
#define CLEAR_REQUEST_PKT			0x13
#define CLEAR_CONF_PKT				0x17
#define RESET_REQUEST_PKT			0x1B
#define RESET_CONF_PKT				0x1F
#define RESTART_REQUEST_PKT	  		0xFB
#define RESTART_CONF_PKT			0xFF
#define INTERRUPT_PKT			 	0x23
#define INTERRUPT_CONF_PKT			0x27
#define DIAGNOSTIC_PKT				0xF1
#define REGISTRATION_REQUEST_PKT 		0xF3
#define REGISTRATION_CONF_PKT 			0xF7



enum { 
	SIOC_X25_PLACE_CALL	= (SIOC_ANNEXG_PLACE_CALL),
	SIOC_X25_CLEAR_CALL	= (SIOC_ANNEXG_CLEAR_CALL),
	SIOC_X25_API_RESERVED 	= (SIOC_WANPIPE_DEVPRIVATE),

	SIOC_X25_GET_CALL_DATA,
	SIOC_X25_SET_CALL_DATA,

 	SIOC_X25_ACCEPT_CALL,
	SIOC_X25_RESET_CALL,
	SIOC_X25_DEBUG,
	SIOC_X25_SET_LCN_PID,
	SIOC_X25_SET_LCN_LABEL,
	SIOC_X25_INTERRUPT,	
	SIOC_X25_HDLC_LINK_STATUS,
	SIOC_X25_CLEAR_CONF,
	SIOC_X25_RESET_CONF,
	SIOC_X25_RESTART_CONF
};


#define SIOC_WANPIPE_GET_CALL_DATA SIOC_X25_GET_CALL_DATA
#define SIOC_WANPIPE_SET_CALL_DATA SIOC_X25_SET_CALL_DATA
#define SIOC_WANPIPE_ACCEPT_CALL   SIOC_X25_ACCEPT_CALL
#define SIOC_WANPIPE_CLEAR_CALL	   SIOC_X25_CLEAR_CALL
#define SIOC_WANPIPE_RESET_CALL	   SIOC_X25_RESET_CALL
#define SIOC_WANPIPE_INTERRUPT	   SIOC_X25_INTERRUPT
#define SIOC_WANPIPE_GET_LAPB_STATUS SIOC_X25_HDLC_LINK_STATUS


#define DECODE_X25_CMD(cmd) ((cmd==SIOC_X25_PLACE_CALL)?"X25 PLACE CALL" : \
		             (cmd==SIOC_X25_CLEAR_CALL)?"X25 CLEAR CALL" : \
			     (cmd==SIOC_X25_GET_CALL_DATA)?"X25 GET CALL" : \
			     (cmd==SIOC_X25_SET_CALL_DATA)?"X25 SET CALL" : \
			     (cmd==SIOC_X25_ACCEPT_CALL)?"X25 ACCEPT CALL" : \
			     (cmd==SIOC_X25_RESET_CALL)?"X25 RESET CALL" : \
			     (cmd==SIOC_X25_DEBUG)?"X25 DEBUG" : \
			     (cmd==SIOC_X25_INTERRUPT)?"X25 INTERRUPT" : \
			     (cmd==SIOC_X25_SET_LCN_PID)?"X25 SET PID": \
		  	     (cmd==SIOC_X25_SET_LCN_LABEL)?"X25 SET LABEL" : DECODE_API_CMD(cmd) )
			    


/*============ bit settings for the 'X25_API_options' ============ */

/* don't check the transmit window of the LCN when transmitting data */
#define DONT_CHK_TX_WIN_ON_DATA_TX		0x0001	

/*========== bit settings for the 'X25_protocol_options' byte =========*/

/* registration pragmatics supported */
#define REGISTRATION_PRAGMATICS_SUPP		0x0001	
/* a station configured as a DCE will issue Diagnostic packets */
#define NO_DIAG_PKTS_ISSUED_BY_DCE		0x0002
/* a Restart Request packet is not issued when entering the ABM */
#define NO_RESTART_REQ_ON_ENTER_ABM		0x0004	
/* an asynchronous packet does not include a diagnostic field */
#define NO_DIAG_FIELD_IN_ASYNC_PKTS		0x0008
/* D-bit pragmatics are not supported */
#define D_BIT_PRAGMATICS_NOT_SUPPORTED		0x0010
/* flow control facilities are automatically inserted in call */
#define AUTO_FLOW_CTRL_PARM_FACIL_INS 		0x0020	
																	/* setup packets */
#define CALLAC_DOES_NOT_INCL_ADDR_LGTH 	0x0040	/* a transmitted or received Call Accept packet does not include */
																	/* an address and facilities length field */
#define HANDLE_ALL_INCOMING_FACILS			0x0080	/* all incoming facilities are supported */
#define NO_CHK_PROC_INCOMING_FACILS			0x0100	/* incoming facilities are not checked and processed */

/* bit settings for the 'X25_response_options' byte */
#define ALL_DATA_PKTS_ACKED_WITH_RR			0x0001	/* all received Data packets are acknowledged with an RR */
#define DISABLE_AUTO_CLEAR_CONF				0x0002	/* disable the automatic issing of Clear Confirmation packets */
#define DISABLE_AUTO_RESET_CONF				0x0004	/* disable the automatic issing of Reset Confirmation packets */
#define DISABLE_AUTO_RESTART_CONF			0x0008	/* disable the automatic issing of Restart Confirmation packets */
#define DISABLE_AUTO_INT_CONF					0x0010	/* disable the automatic issing of Interrupt Confirmation packets */

/* 'genl_facilities_supported_1' */
#define FLOW_CTRL_PARM_NEG_SUPP				0x0001
#define THROUGHPUT_CLASS_NEG_SUPP			0x0002
#define REV_CHARGING_SUPP						0x0004
#define FAST_SELECT_SUPP						0x0008
#define NUI_SELECTION_SUPP						0x0010
#define CUG_SELECT_BASIC_SUPP					0x0020
#define CUG_SELECT_EXT_SUPP					0x0040
#define CUG_OUT_ACC_SEL_BASIC_SUPP			0x0080
#define CUG_OUT_ACC_SEL_EXT_SUPP				0x0100
#define BI_CUG_SEL_SUPP							0x0200
#define RPOA_SEL_BASIC_FORMAT_SUPP			0x0400
#define RPOA_SEL_EXT_FORMAT_SUPP				0x0800
#define CALL_DEFLEC_SEL_SUPP					0x1000
#define CALL_REDIR_DEFLEC_NOTIF_SUPP		0x2000
#define CALLED_LNE_ADDR_MOD_NOTIF_SUPP		0x4000
#define TRANSIT_DELAY_SELECT_IND_SUPP		0x8000

/* 'genl_facilities_supported_2' */
#define CHRG_INFO_REQ_SERVICE_SUPP			0x0001
#define CHRG_INFO_IND_MON_UNIT_SUPP			0x0002
#define CHRG_INFO_IND_SEG_COUNT_SUPP		0x0004
#define CHRG_INFO_IND_CALL_DUR_SUPP			0x0008

/* 'CCITT_facilities_supported' */
#define CCITT_CALLING_ADDR_EXT_SUPP			0x0001
#define CCITT_CALLED_ADDR_EXT_SUPP			0x0002
#define CCITT_MIN_THROUGH_CLS_NEG_SUPP		0x0004
#define CCITT_ETE_TX_DELAY_NEG_SUPP			0x0008
#define CCITT_PRIORITY_SUPP					0x0010
#define CCITT_PROTECTION_SUPP					0x0020
#define CCITT_EXPIDITED_DATA_NEG_SUPP		0x0040

/* CCITT compatibility levels */
#define CCITT_1980_COMPATIBILITY				1980	/* 1980 compatibility */
#define CCITT_1984_COMPATIBILITY				1984	/* 1984 compatibility */
#define CCITT_1988_COMPATIBILITY				1988	/* 1988 compatibility */

/* permitted minimum and maximum values for setting the X.25 configuration */
#define MIN_PACKET_WINDOW						1		/* the minimum packet window size */
#define MAX_PACKET_WINDOW						7		/* the maximum packet window size */
#define MIN_DATA_PACKET_SIZE					16		/* the minimum Data packet size */
#define MAX_DATA_PACKET_SIZE					4096	/* the maximum Data packet size */
#define MIN_X25_ASYNC_TIMEOUT					1000	/* the minimum asynchronous packet timeout value */
#define MAX_X25_ASYNC_TIMEOUT					60000	/* the maximum asynchronous packet timeout value */
#define MIN_X25_ASYNC_RETRY_COUNT			0		/* the minimum asynchronous packet retry count */
#define MAX_X25_ASYNC_RETRY_COUNT			250	/* the maximum asynchronous packet retry count */












#define	X25_MAX_DATA	4096	/* max length of X.25 data buffer */
#pragma pack(1)

typedef struct {
	unsigned char  qdm;	/* Q/D/M bits */
	unsigned char  cause;	/* cause field */
	unsigned char  diagn;	/* diagnostics */
	unsigned char  pktType;
	unsigned short length;
	unsigned char  result;
	unsigned short lcn;
	unsigned short mtu;
	unsigned short mru;
	char reserved[3];
}x25api_hdr_t;

typedef struct {
	x25api_hdr_t hdr;
	char data[X25_MAX_DATA];
}x25api_t;

#pragma pack()


/* ----------------------------------------------------------------------------
 *                     Return codes from interface commands
 * --------------------------------------------------------------------------*/

#define NO_X25_EXCEP_COND_TO_REPORT			0x51	/* there is no X.25 exception condition to report */
#define X25_COMMS_DISABLED						0x51	/* X.25 communications are not currently enabled */
#define X25_COMMS_ENABLED						0x51	/* X.25 communications are currently enabled */
#define DISABLE_X25_COMMS_BEFORE_CFG		0x51	/* X.25 communications must be disabled before setting the configuration */
#define X25_CFG_BEFORE_COMMS_ENABLED		0x52	/* perform a SET_X25_CONFIGURATION before enabling X.25 comms */
#define LGTH_X25_CFG_DATA_INVALID 			0x52	/* the length of the passed X.25 configuration data is invalid */
#define LGTH_X25_INT_DATA_INVALID			0x52	/* the length of the passed interrupt trigger data is invalid */
#define LINK_LEVEL_NOT_CONNECTED				0x52	/* the HDLC link is not currently in the ABM */
#define INVALID_X25_CFG_DATA					0x53	/* the passed X.25 configuration data is invalid */
#define INVALID_X25_APP_IRQ_SELECTED		0x53	/* in invalid IRQ was selected in the SET_X25_INTERRUPT_TRIGGERS */
#define X25_IRQ_TMR_VALUE_INVALID			0x54	/* an invalid application IRQ timer value was selected */
#define INVALID_X25_PACKET_SIZE				0x55	/* the X.25 packet size is invalid */
#define INVALID_X25_LCN_CONFIG				0x56	/* the X.25 logical channel configuration is invalid */
#define SET_HDLC_CFG_BEFORE_X25_CFG			0x58	/* the HDLC configuration must be set before the X.25 configuration */
#define HDLC_CFG_INVALID_FOR_X25				0x59	/* the HDLC configuration is invalid for X.25 usage */
#define INVALID_LCN_SELECTION					0x60	/* the LCN selected is invalid */
#define NO_LCN_AVAILABLE_CALL_REQ			0x60	/* no LCN is available for an outgoing Call Request */
#define LCN_NOT_IN_DATA_XFER_ST				0x61 	/* the LCN is not in the data transfer mode */
#define NO_INCOMING_CALL_PENDING				0x61 	/* no Call Request/Indication has been received */
#define NO_CLEAR_REQ_PKT_RECEIVED		 	0x61	/* no Clear Request/Indication has been received */
#define NO_RESET_REQ_PKT_RECEIVED		 	0x61 	/* no Reset Request/Indication has been received */
#define NO_RESTART_REQ_PKT_RECEIVED	  		0x61 	/* no Restart Request/Indication has been received */
#define NO_INTERRUPT_PKT_RECEIVED			0x61 	/* no Interrupt packet has been received */
#define NO_REGSTR_REQ_PKT_RECEIVED			0x61 	/* no Registration Request has been received */
#define LCN_STATE_INVALID_CLEAR_REQ	 		0x61	/* the LCN state is invalid for a Clear Request to be issued */
#define LCN_STATE_INVALID_RESET_REQ			0x61	/* the LCN state is invalid for a Reset Request to be issued */
#define LCN_STATE_INVALID_INTERRUPT			0x61	/* the LCN state is invalid for an Interrupt packet to be issued */
#define ASYNC_BFR_IN_USE						0x62	/* the storage buffer used for asynchronous packet formatting is */
																/* currently in use */
#define DEFINED_WINDOW_SIZE_INVALID			0x62	/* SET_PVC_CONFIGURATION - the selected window size is invalid */
#define DEFINED_PACKET_SIZE_INVALID			0x63	/* SET_PVC_CONFIGURATION - the selected packet size is invalid */
#define DATA_TRANSMIT_WINDOW_CLOSED			0x63	/* the transmit packet window is closed */
#define NO_X25_DATA_PKT_AVAIL			  		0x63	/* no Data packet is available for reception by the */
																/* application on the selected LCN */
#define INVALID_X25_PACKET_TYPE				0x63	/* the X25 packet type is invalid */
#define INVALID_CALL_CONTROL_DEFN 			0x63 	/* INCOMING_CALL_CONTROL - the call control definition is invalid */
#define D_BIT_ACK_OUTSTANDING					0x63	/* a response to a transmitted D-bit Data packet has not yet been */
																/* received */
#define PREV_DATA_RX_NOT_COMPLETE			0x64	/* the application has not completed the previous RECEIVE_X25_DATA_PACKET command */
#define PREV_DATA_TX_NOT_COMPLETE			0x64	/* the application has not completed the previous SEND_X25_DATA_PACKET command */
#define INVALID_X25_PKT_LGTH					0x64	/* the length of the outgoing packet is invalid */
#define CALL_ARGS_LGTH_EXCESSIVE				0x65	/* the outgoing call arguments are of excessive length */
#define NO_CALLAC_FAST_SEL_RESTR_RESP 		0x66	/* an incoming call received with Fast Select */
																/* facilities with restriction on response may not be accepted */
#define REG_PRAGMATICS_NOT_SUPP 				0x67	/* Registration pragmatics are not supported */
#define PKT_NOT_PERMITTED_AT_DTE				0x68	/* the asynchronous packet may not be transmitted */
																/* at a station configured as a DTE */
#define PKT_NOT_PERMITTED_AT_DCE				0x68	/* the asynchronous packet may not be transmitted */
																/* at a station configured as a DCE */
#define USR_DATA_NOT_ALWD_ASYNC_PKT			0x69	/* a user data field is not permitted in the asynchronous packet */
#define D_BIT_NOT_ALWD_CALL_SETUP_PKT 		0x6A	/* the D-bit may not be set in the asynchronous packet */
#define FACIL_NOT_ALWD_OUTGOING_PKT 		0x6B	/* a facilities field is not permitted in the asynchronous packet */
#define NO_CALL_DESTINATION_ADDR				0x6C	/* - no '-d' definition found in the passed arguments */
#define INVALID_ARGS_IN_ASYNC_PKT			0x6D	/* there are invalid arguments included in the asynchronous packet */
#define USER_DATA_FIELD_LGTH_ODD 			0x6E	/* the length of the passed user data is odd */
#define ASYNC_PKT_RECEIVED						0x70	/* an asynchronous packet has been received */
#define PROTOCOL_VIOLATION						0x71	/* a protocol violation has occurred */
#define X25_TIMEOUT								0x72 	/* an X.25 timeout has occured */
#define ASYNC_PKT_RETRY_LIMIT_EXCEEDED	 	0x73	/* an X.25 asynchronous packet retry limit has been exceeded */
#define INVALID_X25_COMMAND					0x7F	/* the defined X25 interface command is invalid */



#ifdef __KERNEL__


struct lapb_x25_register_struct {

	unsigned long init;
 	void (*x25_lapb_connect_indication)(void *x25_id,int reason);
	int  (*x25_setparms)(struct net_device *dev, struct x25_parms_struct *conf);
	void (*x25_lapb_bh_kick)(void *x25_id);
	int (*x25_receive_pkt)(void *x25_id, struct sk_buff *skb);
	
	int (*x25_register)(void **x25_lnk, struct net_device *lapb_dev, char *dev_name, struct net_device **new_dev);
	int (*x25_unregister_link)(void *x25_id);
	int (*x25_unregister)(struct net_device *x25_dev);

	void (*x25_lapb_disconnect_confirm)(void *x25_id,int reason);
	void (*x25_lapb_disconnect_indication)(void *x25_id,int reason);
	void (*x25_lapb_connect_confirm)(void *x25_id,int reason);
	int  (*x25_get_map)(void*, struct seq_file *);
	void (*x25_get_active_inactive)(void*,wp_stack_stats_t*);
};

struct x25_dsp_register_struct {
	unsigned char init;
        int (*dsp_register)(struct net_device* x25_dev, char* dev_name, struct net_device **new_dev);
        int (*dsp_unregister)(struct net_device** x25_dev, void* dsp_id);
        int  (*dsp_setparms)(void* dsp_id, void*);

        int  (*dsp_call_request)(void* dsp_id, void* sk_id, struct sk_buff* skb);
        int  (*dsp_call_accept)(void* dsp_id, struct sk_buff* skb);
        void  (*dsp_svc_up)(void* dsp_id);
        void  (*dsp_svc_down)(void* dsp_id);
        void  (*dsp_svc_ready)(void* dsp_id);
	void  (*dsp_svc_reset)(void* dsp_id);
	
        int  (*dsp_receive_pkt)(void* dsp_id, struct sk_buff *skb);
	void (*dsp_x25_bh_kick)(void *dsp_id);
	int (*dsp_rx_buf_check)(void *dsp_id, int len);
	int (*dsp_oob_msg)(void *dsp_id, struct sk_buff* skb);
	int (*dsp_check_dev)(void *dsp_id);
	void (*dsp_get_active_inactive)(void*dsp_id,wp_stack_stats_t *);

};


struct wanpipe_x25_register_struct{
	
	unsigned char init;
	
	int (*x25_bind_api_to_svc)(struct net_device *master_dev, void *sk_id);
	int (*x25_bind_listen_to_link)(struct net_device *master_dev, void *sk_id, unsigned short protocol);
	int (*x25_unbind_listen_from_link)(void *sk_id,unsigned short protocol);
	
	int (*x25_dsp_register)(struct net_device *x25_dev,
				char *dev_name,
				struct net_device **new_dev);

	int (*x25_dsp_unregister)(void* dsp_id);
	int (*x25_dsp_setparms) (struct net_device *x25_dev, 
			void* dsp_id, 
			void *dsp_parms);


};

extern int register_wanpipe_x25_protocol (struct wanpipe_x25_register_struct *x25_api_reg);
extern void unregister_wanpipe_x25_protocol (void);

extern int x25_dsp_register(struct net_device *x25_dev,
			    char *dev_name,
			    struct net_device **new_dev);

extern int x25_dsp_unregister(void* dsp_id);
extern int x25_dsp_setparms (struct net_device *x25_dev, 
			void* dsp_id, 
			void *dsp_parms);

extern void lapb_kickstart_tx (struct net_device *dev);

extern void lapb_set_mtu (struct net_device *dev, unsigned int mtu);

extern void lapb_link_unregister (struct net_device *dev);

extern void x25_dsp_id_unregister(struct net_device *dev);

extern int x25_dsp_is_svc_ready(struct net_device *dev);

extern void wan_skb_destructor (struct sk_buff *skb);
#define CALL_LOG(x25_link,format,msg...) { if (x25_link && x25_link->cfg.call_logging){ \
					      DEBUG_EVENT(format,##msg); \
					   }else if(!x25_link){\
					      DEBUG_EVENT(format,##msg); \
					   }}

#endif
#endif

