
#ifndef _BSC_HEADER_
#define _BSC_HEADER_

#pragma pack(1)


/*========== MAILBOX COMMANDS AND RETURN CODES ==========*/
#define BSC_READ                       0x00
#define BSC_WRITE                      0x01
#define OPEN_LINK                      0x02
#define CLOSE_LINK                     0x03
#define CAM_WRITE                      0x04
#define CAM_READ                       0x05
#define LINK_STATUS                    0x06
#define READ_OPERATIONAL_STATISTICS    0x07
#define FLUSH_OPERATIONAL_STATISTICS   0x08
#define READ_COMMS_ERROR_STATISTICS    0x09
#define FLUSH_COMMS_ERROR_STATISTICS   0x0A
#define READ_BSC_ERROR_STATISTICS      0x0B
#define FLUSH_BSC_ERROR_STATISTICS     0x0C
#define FLUSH_BSC_TEXT_BUFFERS         0x0D
#define SET_CONFIGURATION              0x0E
#undef READ_CONFIGURATION
#define READ_CONFIGURATION             0x0F
#define SET_MODEM_STATUS               0x10
#define READ_MODEM_STATUS              0x11
#undef READ_CODE_VERSION         
#define READ_CODE_VERSION              0x12
#define ADD_STATION						0x20
#define DELETE_STATION					0x21
#define DELETE_ALL_STATIONS				0x22
#define LIST_STATIONS					0x23
#define SET_GENERAL_OR_SPECIFIC_POLL	0x24
#define SET_STATION_STATUS				0x25

#define READ_STATE_DIAGNOSTICS         0x30

#define	UNUSED_CMD_FOR_EVENTS		0x7e

#define	Z80_TIMEOUT_ERROR	0x0a
#define	DATA_LENGTH_TOO_BIG	0x03

#define BSC_SENDBOX     0xF000  /* send mailbox */

#define	MDATALEN	4000
#define MBOX_HEADER_SZ	15

/* for point-to-point, ignore station_number and address fields in CBLOCK */

/* note: structure must be packed on 1-byte boundaries
   and for a block this size, it is not wise to allocate it on
   the stack - should be a static global
*/
/* control block */
typedef struct {
	unsigned char 	opp_flag 		;
	unsigned char 	command			;
	unsigned short 	buffer_length		;
	unsigned char 	return_code		;
	unsigned char 	misc_tx_rx_bits		;
	unsigned short 	heading_length		;
	unsigned short 	notify			;
	unsigned char 	station			;
	unsigned char 	poll_address		;
	unsigned char 	select_address		;
	unsigned char 	device_address		;
	unsigned char 	notify_extended		;
	unsigned char 	reserved		;
	unsigned char 	data[MDATALEN]		;
} BSC_MAILBOX_STRUCT;



typedef struct {
	unsigned char 	line_speed_number	;
	unsigned short 	max_data_frame_size	;
	unsigned char 	secondary_station	;
	unsigned char 	num_consec_PAD_eof	;
	unsigned char 	num_add_lead_SYN	;
	unsigned char 	conversational_mode	;
	unsigned char 	pp_dial_up_operation	;
	unsigned char 	switched_CTS_RTS	;
	unsigned char 	EBCDIC_encoding		;
	unsigned char 	auto_open		;
	unsigned char 	misc_bits		;
	unsigned char 	protocol_options1	;
	unsigned char 	protocol_options2	;
	unsigned short 	reserved_pp		;
	unsigned char 	max_retransmissions	;
	unsigned short 	fast_poll_retries	;
	unsigned short 	TTD_retries		;
	unsigned short 	restart_timer		;
	unsigned short 	pp_slow_restart_timer	;
	unsigned short 	TTD_timer		;
	unsigned short 	pp_delay_between_EOT_ENQ ;
	unsigned short 	response_timer		;
	unsigned short 	rx_data_timer		;
	unsigned short 	NAK_retrans_delay_timer ;
	unsigned short 	wait_CTS_timer		;
	unsigned char 	mp_max_consec_ETX	;
	unsigned char 	mp_general_poll_address ;
	unsigned short 	sec_poll_timeout	;
	unsigned char 	pri_poll_skips_inactive ;
	unsigned char 	sec_additional_stn_send_gpoll ;
	unsigned char 	pri_select_retries 	;
	unsigned char 	mp_multipoint_options	;
	unsigned short 	reserved		;
} BSC_CONFIG_STRUCT;


typedef struct {
	unsigned char max_tx_queue	;
	unsigned char max_rx_queue	;
	unsigned char station_flags	;
}ADD_STATION_STRUCT;


#define SIOC_WANPIPE_EXEC_CMD	SIOC_WANPIPE_DEVPRIVATE

#pragma pack()

#endif
