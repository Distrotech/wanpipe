/*
 * These are the public elements of the Linux DSP module.
 */

#ifndef	_WANPIPE_DSP_KERNEL_H
#define	_WANPIPE_DSP_KERNEL_H

/*
 * ******************************************************************
 * 			INCLUDES				    *
 * ******************************************************************
 */
#include <linux/if_wanpipe.h>
#include <linux/wanpipe_cfg.h>
#include "wanpipe_x25_kernel.h"

/*
 * ******************************************************************
 * 			DEFINES/MACROS				    *
 * ******************************************************************
 */

/* DSP Messages */
#define DSP_CALL_REQ_HOST_MSG		0x56
#define DSP_CALL_REQ_TERM_MSG		0x57
#define DSP_INV_TO_CLEAR_MSG		0x01
#define DSP_CMD_RES_UNDELIVERED_MSG	0x10
#define DSP_CMD_RES_ABORTED_MSG		0x11
#define DSP_STATUS_MSG			0x12
#define DSP_ACK_MSG			0x14
#define DSP_CIRCUIT_ENABLED_MSG		0x20
#define DSP_CIRCUIT_RESET_MSG		0x21
#define DSP_CIRCUIT_REQUEST_MSG		0x22
#define DSP_CIRCUIT_DISCONNECT_MSG	0x24

#define DSP_DATA_MSG			0x40
#define DSP_DATA_ACK_MSG		0x41


#define DSP_FALSE	0
#define DSP_TRUE	1

#define DSP_OK		0

/* 'dsp_result' values */
#define DSP_LINK_DOWN	0x01
#define DSP_LINK_READY	0x02
#define DSP_LINK_RESET	0x03

/* Options for 'DSP_pad' */
#define DSP_HPAD	0x00
#define DSP_TPAD	0x01

#define DSP_MAX_DATA	1024

#define DSP_CALL_STR_SZ	512

#ifndef DSP_PROT
# define DSP_PROT	0x20
#endif

/* Control Byte */
#define DSP_CR_INC_INF_FOR_SE	0x01	/* Further information for session 
					 * establishment is contained in a
					 * CIRCUIT REQEUST message
					 */
#define DSP_SND_MULT_USER_CR	0x02	/* Sends a Multiple User Circuit
					 * Request message.
					 */
#define DSP_TT_SUPPORTS		0x04	/* Request is for a 3270 device that
					 * supports Transparent Text.
					 */
#define DSP_REQ_FOR_ASCII_DEV	0x08	/* Request is for an ASCII device.
					 */

/* Connection Request Mode */
#define DSP_CRM_TYPE1		0x01	/* Referred to as fixed class */
#define DSP_CRM_TYPE2		0x02	/* Referred to as specific class */
#define DSP_CRM_TYPE3		0x03	/* Referred to as non-specific class */
#define DSP_CRM_TYPE4		0x04	/* Referred to as associated class */

/*
 * *************
 * Reason Code *
 * *************
 */
/* Reason code for CIRCUIT_RESET */
#define DSP_RC_RESET			0x00
#define DSP_RC_DATA_ERROR		0x11
/* Reason code for INVITATION_TO_CLEAR */
#define DSP_RC_UNDEFINED		0x00
#define DSP_RC_USER			0x01
#define DSP_RC_INVALID_DQ_MSG		0x10
#define DSP_RC_INVALID_STATE_TRANS	0x11
#define DSP_RC_INVALID_DQ_FORMAT	0x12
#define DSP_RC_INVALID_DATA_FORMAT	0x13
#define DSP_RC_TIMEOUT			0x20
#define DSP_RC_FACILITY_FAILURE		0x21

/*
 * ******************************************************************
 * 			ENUM/TYPEDEF/STRCTURE			    *
 * ******************************************************************
 */
enum { 
	SIOC_DSP_PLACE_CALL	= (SIOC_ANNEXG_PLACE_CALL),
	SIOC_DSP_CLEAR_CALL 	= (SIOC_ANNEXG_CLEAR_CALL),
	SIOC_DSP_API_RESERVED 	= (SIOC_WANPIPE_DEVPRIVATE),

	SIOC_DSP_GET_CALL_DATA,
	SIOC_DSP_SET_CALL_DATA,

 	SIOC_DSP_ACCEPT_CALL,
	SIOC_DSP_RESET_CALL,
	SIOC_DSP_DEBUG,	

	SIOC_DSP_INVITATION_TO_CLEAR,
	SIOC_DSP_CIRCUIT_ENABLED,
	SIOC_DSP_CIRCUIT_RESET,
	SIOC_DSP_CIRCUIT_DISCONNECT,
	SIOC_DSP_CIRCUIT_REQUEST,
	SIOC_DSP_STATUS,
	SIOC_DSP_ACK,
	SIOC_DSP_CMD_RES_UNDELIVERED,
	SIOC_DSP_CMD_RES_ABORTED,

	SIOC_DSP_SEND_DATA,
	SIOC_DSP_RECEIVE_DATA,

	SIOC_DSP_SET_PID,
	SIOC_DSP_SET_LCN_LABEL
};



#pragma pack(1)
typedef struct {
	unsigned char  dsp_pkt_type;	/* DSP packet type */
	unsigned char  dsp_resp;	/* 1 - sending response */
	unsigned char  dsp_user_cn;	/* user circuit number */
	unsigned short dsp_length;	/* Data length */
	unsigned short dsp_lcn;
	unsigned char  dsp_result;
	union {
		unsigned char status;		/* Status */
		unsigned char pad_type;		/* PAD type */
#if 0
		ALEX 10.4.2001
		ETX_ETB_ESC
		unsigned char et_byte;		/* ET byte (1-ETX,0-ETB)*/
#endif
	} dspapi_hdr_dspu1;
	union {
		struct {
			unsigned char  src_cu_addr;	/* Src Ctrl Unit addr */
			unsigned char  src_dev_addr;	/* Source Device addr */
			unsigned char  ctrl_byte;	/* control byte */
			unsigned char  dev_info;	/* Device information */
			unsigned char  crm_type;	/* Con. Request mode */
		} dspapi_hdr_dsppc1;
		unsigned char error_code;		/* Sense byte */
		unsigned char reason_code;		/* reason code */
		unsigned char transp;			/* Transparent data */
	} dspapi_hdr_dspu2;
	union {
		struct {
			unsigned char  dst_cu_addr;	/* Dst Ctrl Unit addr */
			unsigned char  dst_dev_addr;	/* Dst Device addr */
		}dspapi_hdr_dsppc2;
		unsigned char  sn;			/* seq. num */
		unsigned char  sense;			/* Sense byte */
	}dspapi_hdr_dspu3;
}dspapi_hdr_t;

#define dsp_status	 dspapi_hdr_dspu1.status
#define dsp_pad_type	 dspapi_hdr_dspu1.pad_type
#define dsp_et_byte	 dspapi_hdr_dspu1.et_byte

#define dsp_src_cu_addr	 dspapi_hdr_dspu2.dspapi_hdr_dsppc1.src_cu_addr
#define dsp_src_dev_addr dspapi_hdr_dspu2.dspapi_hdr_dsppc1.src_dev_addr
#define dsp_ctrl_byte	 dspapi_hdr_dspu2.dspapi_hdr_dsppc1.ctrl_byte
#define dsp_dev_info	 dspapi_hdr_dspu2.dspapi_hdr_dsppc1.dev_info
#define dsp_crm_type	 dspapi_hdr_dspu2.dspapi_hdr_dsppc1.crm_type
#define dsp_error_code	 dspapi_hdr_dspu2.error_code
#define dsp_reason_code	 dspapi_hdr_dspu2.reason_code
#define dsp_data_transp	 dspapi_hdr_dspu2.transp

#define dsp_dst_cu_addr	 dspapi_hdr_dspu3.dspapi_hdr_dsppc2.dst_cu_addr
#define dsp_dst_dev_addr dspapi_hdr_dspu3.dspapi_hdr_dsppc2.dst_dev_addr
#define dsp_sense	 dspapi_hdr_dspu3.sense
#define dsp_sn	 	 dspapi_hdr_dspu3.sn

typedef struct {
	dspapi_hdr_t hdr;
	char data[DSP_MAX_DATA];
}dspapi_t;

#pragma pack()


#ifdef __KERNEL__

struct wanpipe_dsp_register_struct{
	unsigned char init;
	int (*dsp_bind_api)(struct net_device *master_dev, void *sk_id);
};


/* ----------------------------------------------------------------------------
 *                     Return codes from interface commands
 * --------------------------------------------------------------------------*/


extern int register_wanpipe_dsp_protocol(struct wanpipe_dsp_register_struct*);
extern void unregister_wanpipe_dsp_protocol(void);

extern int register_x25_dsp_protocol(struct x25_dsp_register_struct*);
extern void unregister_x25_dsp_protocol(void);
extern struct net_device* x25_get_svc_dev(struct net_device* master_dev);
extern int x25_get_call_info(struct net_device* master_dev, unsigned short*, unsigned short*, unsigned short*);

#endif /* __KERNEL__ */

#endif

