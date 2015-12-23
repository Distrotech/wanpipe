/*
 * These are the public elements of the Linux sppp module.
 */

#ifndef	_WP_SPPP_IFACE_H
#define	_WP_SPPP_IFACE_H

#ifdef WANLIP_DRIVER

enum sppp_ioctl_cmds {
	SIOCG_SPPP_CONF,
	SIOCS_SPPP_NEW_X25_CONF,
	SIOCS_SPPP_DEL_X25_CONF,
	SIOCG_SPPP_STATUS,
};

extern void *wp_register_chdlc_prot(void *, 
				   char *, 
		                   void *, 
				   wplip_prot_reg_t *);
extern void *wp_register_ppp_prot(void *, 
				   char *, 
		                   void *, 
				   wplip_prot_reg_t *);

extern int wp_unregister_sppp_prot(void *);

extern void *wp_register_sppp_chan(void *if_ptr, 
			    void *prot_ptr, 
		            char *devname, 
			    void *cfg_ptr,
			    unsigned char type);

extern int wp_unregister_sppp_chan(void *chan_ptr);

extern int wp_sppp_open(void *sppp_ptr);
extern int wp_sppp_close(void *sppp_ptr);
extern int wp_sppp_rx(void *sppp_ptr, void *skb);
extern int wp_sppp_bh(void *sppp_ptr);
extern int wp_sppp_tx(void *sppp_ptr, void *skb, int type);

#if defined(__WINDOWS__)
extern void *wp_register_sppp_prot(void *, char *, void *, wplip_prot_reg_t *);
#endif
extern int wp_sppp_timer(void *sppp_ptr, unsigned int *period, unsigned int carrier_reliable);
extern int wp_sppp_pipemon(void *sppp, int cmd, int addr, unsigned char* data, unsigned int *len);
extern int wp_sppp_task(void *sppp_ptr);



#endif


/*=====================================================
 * PPP Commands and Statistics
 *====================================================*/

/* 'command' field defines */
#define PPP_READ_CODE_VERSION	0x10	/* configuration commands */
#define PPP_SET_CONFIG		0x05
#define PPP_READ_CONFIG		0x06
#define	PPP_SET_INTR_FLAGS	0x20
#define	PPP_READ_INTR_FLAGS	0x21
#define	PPP_SET_INBOUND_AUTH	0x30
#define	PPP_SET_OUTBOUND_AUTH	0x31
#define	PPP_GET_CONNECTION_INFO	0x32
#define	PPP_GET_LINK_STATUS	0x34

#define PPP_COMM_ENABLE		0x03	/* operational commands */
#define PPP_COMM_DISABLE	0x04
#define	PPP_SEND_SIGN_FRAME	0x23
#define	PPP_READ_SIGN_RESPONSE	0x24
#define	PPP_DATALINE_MONITOR	0x33

#define PPP_READ_STATISTICS	0x07	/* statistics commands */
#define PPP_FLUSH_STATISTICS	0x08
#define PPP_READ_ERROR_STATS	0x09
#define PPP_FLUSH_ERROR_STATS	0x0A
#define PPP_READ_PACKET_STATS	0x12
#define PPP_FLUSH_PACKET_STATS	0x13
#define PPP_READ_LCP_STATS	0x14
#define PPP_FLUSH_LCP_STATS	0x15
#define PPP_READ_LPBK_STATS	0x16
#define PPP_FLUSH_LPBK_STATS	0x17
#define PPP_READ_IPCP_STATS	0x18
#define PPP_FLUSH_IPCP_STATS	0x19
#define PPP_READ_IPXCP_STATS	0x1A
#define PPP_FLUSH_IPXCP_STATS	0x1B
#define PPP_READ_PAP_STATS	0x1C
#define PPP_FLUSH_PAP_STATS	0x1D
#define PPP_READ_CHAP_STATS	0x1E
#define PPP_FLUSH_CHAP_STATS	0x1F
#define PPP_READ_AUTH		0x22

#define PPP_PAP			0xc023
#define PPP_CHAP		0xc223

#pragma pack(1)
/*----------------------------------------------------------------------------
 * Packet Statistics (returned by the PPP_READ_PACKET_STATS command).
 */
typedef struct	ppp_pkt_stats
{
	unsigned short rx_bad_header	;	/* 00: */
	unsigned short rx_prot_unknwn	;	/* 02: */
	unsigned short rx_too_large	;	/* 04: */
	unsigned short rx_lcp		;	/* 06: */
	unsigned short tx_lcp		;	/* 08: */
	unsigned short rx_ipcp		;	/* 0A: */
	unsigned short tx_ipcp		;	/* 0C: */
	unsigned short rx_ipxcp		;	/* 0E: */
	unsigned short tx_ipxcp		;	/* 10: */
	unsigned short rx_pap		;	/* 12: */
	unsigned short tx_pap		;	/* 14: */
	unsigned short rx_chap		;	/* 16: */
	unsigned short tx_chap		;	/* 18: */
	unsigned short rx_lqr		;	/* 1A: */
	unsigned short tx_lqr		;	/* 1C: */
	unsigned short rx_ip		;	/* 1E: */
	unsigned short tx_ip		;	/* 20: */
	unsigned short rx_ipx		;	/* 22: */
	unsigned short tx_ipx		;	/* 24: */
	unsigned short rx_ipv6cp	;	/* 26: */
	unsigned short tx_ipv6cp	;	/* 28: */
	unsigned short rx_ipv6		;	/* 2A: */
	unsigned short tx_ipv6		;	/* 2C: */
} ppp_pkt_stats_t;

/*----------------------------------------------------------------------------
 * LCP Statistics (returned by the PPP_READ_LCP_STATS command).
 */
typedef struct	ppp_lcp_stats
{
	unsigned short rx_unknown	;	/* 00: unknown LCP type */
	unsigned short rx_conf_rqst	;	/* 02: Configure-Request */
	unsigned short rx_conf_ack	;	/* 04: Configure-Ack */
	unsigned short rx_conf_nak	;	/* 06: Configure-Nak */
	unsigned short rx_conf_rej	;	/* 08: Configure-Reject */
	unsigned short rx_term_rqst	;	/* 0A: Terminate-Request */
	unsigned short rx_term_ack	;	/* 0C: Terminate-Ack */
	unsigned short rx_code_rej	;	/* 0E: Code-Reject */
	unsigned short rx_proto_rej	;	/* 10: Protocol-Reject */
	unsigned short rx_echo_rqst	;	/* 12: Echo-Request */
	unsigned short rx_echo_reply	;	/* 14: Echo-Reply */
	unsigned short rx_disc_rqst	;	/* 16: Discard-Request */
	unsigned short tx_conf_rqst	;	/* 18: Configure-Request */
	unsigned short tx_conf_ack	;	/* 1A: Configure-Ack */
	unsigned short tx_conf_nak	;	/* 1C: Configure-Nak */
	unsigned short tx_conf_rej	;	/* 1E: Configure-Reject */
	unsigned short tx_term_rqst	;	/* 20: Terminate-Request */
	unsigned short tx_term_ack	;	/* 22: Terminate-Ack */
	unsigned short tx_code_rej	;	/* 24: Code-Reject */
	unsigned short tx_proto_rej	;	/* 26: Protocol-Reject */
	unsigned short tx_echo_rqst	;	/* 28: Echo-Request */
	unsigned short tx_echo_reply	;	/* 2A: Echo-Reply */
	unsigned short tx_disc_rqst	;	/* 2E: Discard-Request */
	unsigned short rx_too_large	;	/* 30: packets too large */
	unsigned short rx_ack_inval	;	/* 32: invalid Conf-Ack */
	unsigned short rx_rej_inval	;	/* 34: invalid Conf-Reject */
	unsigned short rx_rej_badid	;	/* 36: Conf-Reject w/bad ID */
} ppp_lcp_stats_t;

/*----------------------------------------------------------------------------
 * Loopback Error Statistics (returned by the PPP_READ_LPBK_STATS command).
 */
typedef struct	ppp_lpbk_stats
{
	unsigned short conf_magic	;	/* 00:  */
	unsigned short loc_echo_rqst	;	/* 02:  */
	unsigned short rem_echo_rqst	;	/* 04:  */
	unsigned short loc_echo_reply	;	/* 06:  */
	unsigned short rem_echo_reply	;	/* 08:  */
	unsigned short loc_disc_rqst	;	/* 0A:  */
	unsigned short rem_disc_rqst	;	/* 0C:  */
	unsigned short echo_tx_collsn	;	/* 0E:  */
	unsigned short echo_rx_collsn	;	/* 10:  */
} ppp_lpbk_stats_t;

/*----------------------------------------------------------------------------
 * Protocol Statistics (returned by the PPP_READ_IPCP_STATS and
 * PPP_READ_IPXCP_STATS commands).
 */
typedef struct	ppp_prot_stats
{
	unsigned short rx_unknown	;	/* 00: unknown type */
	unsigned short rx_conf_rqst	;	/* 02: Configure-Request */
	unsigned short rx_conf_ack	;	/* 04: Configure-Ack */
	unsigned short rx_conf_nak	;	/* 06: Configure-Nak */
	unsigned short rx_conf_rej	;	/* 08: Configure-Reject */
	unsigned short rx_term_rqst	;	/* 0A: Terminate-Request */
	unsigned short rx_term_ack	;	/* 0C: Terminate-Ack */
	unsigned short rx_code_rej	;	/* 0E: Code-Reject */
	unsigned short reserved		;	/* 10: */
	unsigned short tx_conf_rqst	;	/* 12: Configure-Request */
	unsigned short tx_conf_ack	;	/* 14: Configure-Ack */
	unsigned short tx_conf_nak	;	/* 16: Configure-Nak */
	unsigned short tx_conf_rej	;	/* 18: Configure-Reject */
	unsigned short tx_term_rqst	;	/* 1A: Terminate-Request */
	unsigned short tx_term_ack	;	/* 1C: Terminate-Ack */
	unsigned short tx_code_rej	;	/* 1E: Code-Reject */
	unsigned short rx_too_large	;	/* 20: packets too large */
	unsigned short rx_ack_inval	;	/* 22: invalid Conf-Ack */
	unsigned short rx_rej_inval	;	/* 24: invalid Conf-Reject */
	unsigned short rx_rej_badid	;	/* 26: Conf-Reject w/bad ID */
} ppp_prot_stats_t;

/*----------------------------------------------------------------------------
 * PAP Statistics (returned by the PPP_READ_PAP_STATS command).
 */
typedef struct	ppp_pap_stats
{
	unsigned short rx_unknown	;	/* 00: unknown type */
	unsigned short rx_auth_rqst	;	/* 02: Authenticate-Request */
	unsigned short rx_auth_ack	;	/* 04: Authenticate-Ack */
	unsigned short rx_auth_nak	;	/* 06: Authenticate-Nak */
	unsigned short reserved		;	/* 08: */
	unsigned short tx_auth_rqst	;	/* 0A: Authenticate-Request */
	unsigned short tx_auth_ack	;	/* 0C: Authenticate-Ack */
	unsigned short tx_auth_nak	;	/* 0E: Authenticate-Nak */
	unsigned short rx_too_large	;	/* 10: packets too large */
	unsigned short rx_bad_peerid	;	/* 12: invalid peer ID */
	unsigned short rx_bad_passwd	;	/* 14: invalid password */
} ppp_pap_stats_t;

/*----------------------------------------------------------------------------
 * CHAP Statistics (returned by the PPP_READ_CHAP_STATS command).
 */
typedef struct	ppp_chap_stats
{
	unsigned short rx_unknown	;	/* 00: unknown type */
	unsigned short rx_challenge	;	/* 02: Authenticate-Request */
	unsigned short rx_response	;	/* 04: Authenticate-Ack */
	unsigned short rx_success	;	/* 06: Authenticate-Nak */
	unsigned short rx_failure	;	/* 08: Authenticate-Nak */
	unsigned short reserved		;	/* 0A: */
	unsigned short tx_challenge	;	/* 0C: Authenticate-Request */
	unsigned short tx_response	;	/* 0E: Authenticate-Ack */
	unsigned short tx_success	;	/* 10: Authenticate-Nak */
	unsigned short tx_failure	;	/* 12: Authenticate-Nak */
	unsigned short rx_too_large	;	/* 14: packets too large */
	unsigned short rx_bad_peerid	;	/* 16: invalid peer ID */
	unsigned short rx_bad_passwd	;	/* 18: invalid password */
	unsigned short rx_bad_md5	;	/* 1A: invalid MD5 format */
	unsigned short rx_bad_resp	;	/* 1C: invalid response */
} ppp_chap_stats_t;

/*----------------------------------------------------------------------------
 * Connection Information (returned by the PPP_GET_CONNECTION_INFO command).
 */
typedef struct	ppp_conn_info
{
	unsigned short remote_mru	;	/* 00:  */
	unsigned char  ip_options	;	/* 02:  */
	unsigned char  ip_local[4]	;	/* 03:  */
	unsigned char  ip_remote[4]	;	/* 07:  */
	unsigned char  ipx_options	;	/* 0B:  */
	unsigned char  ipx_network[4]	;	/* 0C:  */
	unsigned char  ipx_local[6]	;	/* 10:  */
	unsigned char  ipx_remote[6]	;	/* 16:  */
	unsigned char  ipx_router[48]	;	/* 1C:  */
	unsigned char  auth_status	;	/* 4C:  */
#if defined(__WINDOWS__)/* zero-sized array does not comply to ANSI 'C' standard! */
	unsigned char  peer_id[1]	;	/* 4D:  */
#else
	unsigned char  peer_id[0]	;	/* 4D:  */
#endif
} ppp_conn_info_t;


typedef struct s_auth {
	unsigned short 	proto;			/* authentication protocol to use */
	unsigned short	flags;
	unsigned char	name[64];	/* system identification name */
	unsigned char	secret[16];	/* secret password */
	unsigned char	challenge[16];	/* random challenge */
	unsigned short	authenticated; /* 1 when authenticated, 0 when not*/
} s_auth_t;

#pragma pack()

/*=====================================================
 * CHDLC Commands and Statistics
 *====================================================*/

#define READ_CHDLC_OPERATIONAL_STATS	0x27
#define FLUSH_CHDLC_OPERATIONAL_STATS	0x28
#define READ_GLOBAL_CONFIGURATION	0x03
#define READ_CHDLC_CODE_VERSION		0x20
#define READ_CHDLC_CONFIGURATION	0x23


/* ----------------------------------------------------------------------------
 *           Constants for the READ_CHDLC_OPERATIONAL_STATS command
 * --------------------------------------------------------------------------*/

#pragma pack(1)

/* the CHDLC operational statistics structure */
typedef struct {

	/* Data frame transmission statistics */
	unsigned int Data_frames_Tx_count ;	
	/* # of frames transmitted */
	unsigned int Data_bytes_Tx_count ; 	
	/* # of bytes transmitted */
	unsigned int Data_Tx_throughput ;	
	/* transmit throughput */
	unsigned int no_ms_for_Data_Tx_thruput_comp ;	
	/* millisecond time used for the Tx throughput computation */
	unsigned int Tx_Data_discard_lgth_err_count ;	
	/* number of Data frames discarded (length error) */
	unsigned int reserved_Data_frm_Tx_stat1 ;	
	/* reserved for later */
	unsigned int reserved_Data_frm_Tx_stat2 ;
	/* reserved for later */
	unsigned int reserved_Data_frm_Tx_stat3 ;	
	/* reserved for later */

	/* Data frame reception statistics */
	unsigned int Data_frames_Rx_count ;	
	/* number of frames received */
	unsigned int Data_bytes_Rx_count ;	
	/* number of bytes received */
	unsigned int Data_Rx_throughput ;	
	/* receive throughput */
	unsigned int no_ms_for_Data_Rx_thruput_comp ;	
	/* millisecond time used for the Rx throughput computation */
	unsigned int Rx_Data_discard_short_count ;	
	/* received Data frames discarded (too short) */
	unsigned int Rx_Data_discard_long_count ;	
	/* received Data frames discarded (too long) */
	unsigned int Rx_Data_discard_inactive_count ;	
	/* received Data frames discarded (link inactive) */
	unsigned int reserved_Data_frm_Rx_stat1 ;	
	/* reserved for later */

	/* SLARP frame transmission/reception statistics */
	/* number of SLARP Request frames transmitted */
	unsigned int CHDLC_SLARP_REQ_Tx_count ;		
	/* number of SLARP Request frames received */
	unsigned int CHDLC_SLARP_REQ_Rx_count ;		
	/* number of SLARP Reply frames transmitted */
	unsigned int CHDLC_SLARP_REPLY_Tx_count ;	
	/* number of SLARP Reply frames received */
	unsigned int CHDLC_SLARP_REPLY_Rx_count ;	
	/* number of SLARP keepalive frames transmitted */
	unsigned int CHDLC_SLARP_KPALV_Tx_count ;	
	/* number of SLARP keepalive frames received */
	unsigned int CHDLC_SLARP_KPALV_Rx_count ;	
	/* reserved for later */
	unsigned int reserved_SLARP_stat1 ;		
	/* reserved for later */
	unsigned int reserved_SLARP_stat2 ;		

	/* CDP frame transmission/reception statistics */
	unsigned int CHDLC_CDP_Tx_count ;		
	/* number of CDP frames transmitted */
	unsigned int CHDLC_CDP_Rx_count ;		
	/* number of CDP frames received */
	unsigned int reserved_CDP_stat1 ;		
	/* reserved for later */
	unsigned int reserved_CDP_stat2 ;		
	/* reserved for later */
	unsigned int reserved_CDP_stat3 ;		
	/* reserved for later */
	unsigned int reserved_CDP_stat4 ;		
	/* reserved for later */
	unsigned int reserved_CDP_stat5 ;		
	/* reserved for later */
	unsigned int reserved_CDP_stat6 ;		
	/* reserved for later */

	/* Incomming frames with a format error statistics */
	unsigned short Rx_frm_incomp_CHDLC_hdr_count ;	
	/* frames received of with incomplete Cisco HDLC header */
	unsigned short Rx_frms_too_long_count ;		
	/* frames received of excessive length count */
	unsigned short Rx_invalid_CHDLC_addr_count ;	
	/* frames received with an invalid CHDLC address count */
	unsigned short Rx_invalid_CHDLC_ctrl_count ;	
	/* frames received with an invalid CHDLC control field count */
	unsigned short Rx_invalid_CHDLC_type_count ;	
	/* frames received of an invalid CHDLC frame type count */
	unsigned short Rx_SLARP_invalid_code_count ;	
	/* SLARP frame received with an invalid packet code */
	unsigned short Rx_SLARP_Reply_bad_IP_addr ;	
	/* SLARP Reply received - bad IP address */
	unsigned short Rx_SLARP_Reply_bad_netmask ;	
	/* SLARP Reply received - bad netmask */
	unsigned int reserved_frm_format_err1 ;		
	/* reserved for later */
	unsigned int reserved_frm_format_err2 ;		
	/* reserved for later */
	unsigned int reserved_frm_format_err3 ;		
	/* reserved for later */
	unsigned int reserved_frm_format_err4 ;		
	/* reserved for later */

	/* CHDLC timeout/retry statistics */
	unsigned short SLARP_Rx_keepalive_TO_count ;	
	/* timeout count for incomming SLARP frames */
	unsigned short SLARP_Request_TO_count ;		
	/* timeout count for SLARP Request frames */
	unsigned int To_retry_reserved_stat1 ;		
	/* reserved for later */
	unsigned int To_retry_reserved_stat2 ;		
	/* reserved for later */
	unsigned int To_retry_reserved_stat3 ;		
	/* reserved for later */

	/* CHDLC link active/inactive and loopback statistics */
	unsigned short link_active_count ;		
	/* number of times that the link went active */
	unsigned short link_inactive_modem_count ;	
	/* number of times that the link went inactive (modem failure) */
	unsigned short link_inactive_keepalive_count ;	
	/* number of times that the link went inactive (keepalive failure) */
	unsigned short link_looped_count ;		
	/* link looped count */
	unsigned int link_status_reserved_stat1 ;	
	/* reserved for later use */
	unsigned int link_status_reserved_stat2 ;	
	/* reserved for later use */

	/* miscellaneous statistics */
	unsigned int reserved_misc_stat1 ;		
	/* reserved for later */
	unsigned int reserved_misc_stat2 ;		
	/* reserved for later */
	unsigned int reserved_misc_stat3 ;		
	/* reserved for later */
	unsigned int reserved_misc_stat4 ;		
	/* reserved for later */

} CHDLC_OPERATIONAL_STATS_STRUCT, chdlc_stat_t;

#pragma pack()


#endif
