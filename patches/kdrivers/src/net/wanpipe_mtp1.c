/*****************************************************************************
* wanpipe_mtp1.c
*
* 		WANPIPE(tm) MTP1 Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2009 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================*/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe.h"
#include "wanpipe_mtp1.h"

#define BITS_IN_OCTET   8
#define FLAG        	0x7e
#define BIT_7  			0x80
#define TX_BUF_SIZE 	1024*10
#define RX_BUF_SIZE 	1024*8
#define TX_IDLE_BUF_SIZE 128
#define WP_MTP1_MAX_TX_Q 10

#ifdef __WINDOWS__
# define wp_mtp1_os_lock_irq(card,flags)   wan_spin_lock_irq(card,flags)
# define wp_mtp1_os_unlock_irq(card,flags) wan_spin_unlock_irq(card,flags)
#else

#if 0
#warning "WANPIPE MTP1 - OS LOCK DEFINED -- DEBUGGING"
#define wp_mtp1_os_lock_irq(card,flags)	wan_spin_lock_irq(card,flags)
#define wp_mtp1_os_unlock_irq(card,flags) wan_spin_unlock_irq(card,flags)
#else
#define wp_mtp1_os_lock_irq(card,flags)
#define wp_mtp1_os_unlock_irq(card,flags)
#endif
#endif       



//#define DEBUG 1

enum  e_mtp2_daed_rx_states
{
	INIT,
	DAEDRX_0,
	DAEDRX_1,
	DAEDRX_2,
}; 


typedef struct wp_tbs {
	
	u32	cur_1s_bit_count;
	u32	cur_bit_pos;
	u32	cur_index;
	u32	card_index;
	u8  buf[TX_BUF_SIZE];
	u8  idle_buf[TX_IDLE_BUF_SIZE];
	//u32 idle_buf_len;
	wan_skb_queue_t tx_q;
	wan_skb_queue_t tx_r_q;
	wan_skb_queue_t tx_q_dealloc;
} wp_tbs_t;

typedef struct wp_mtp1_link{
	
	int init;
	char name[100];
	
	u8  state_daedr;
	u8  prev_state_daedr;
	u32 r_total_octet_count;
	u32 r_su_accepted;
	u32 r_su_rejected;
	u32 r_fisu_count;
	u32 r_lssu_1_count;
	u32 r_lssu_2_count;
	u32 r_msu_count;
	
	u32 r_flag_accum;
	u32 r_flag_count;
	u32 r_accum;
	u32 r_1s_bit_count;
	u32 r_octet_counting_mode;
	u32 r_octet_counting_count;
	u32 r_octet_bit_count;
	u32 r_bit_gate_count;
	
	wp_tbs_t tbs;
	
	wp_mtp1_reg_t reg;
	wp_mtp1_cfg_t cfg;
	wp_mtp1_stats_t stats;
	
	u8  rx_buf[RX_BUF_SIZE];
	u32 rx_buf_len;
	
	wan_skb_queue_t rx_list;
	wan_skb_queue_t tx_list;

} wp_mtp1_link_t;

static uint16_t  crc_tss_rev_table[256] =	
{
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
	0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
	0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
	0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
	0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
	0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
	0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
	0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
	0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
	0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
	0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
	0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
	0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
	0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
	0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
	0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
	0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
	0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
	0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
	0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
	0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
	0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
	0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
	0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
	0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
	0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
	0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
	0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
	0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
	0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
	0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
};


/*===============================================
  PRIVATE FUNCTIONS
  ==============================================*/
  
static uint16_t crc_tss_rev (uint16_t crc, unsigned int len, uint8_t * p_buf)
		/******************************************************************************/
{
	/* the relected bits version of the algorithm */
	unsigned int i = 0;

	for (i=0; i<len; i++) {
		crc = (crc >> 8) ^ crc_tss_rev_table [(crc ^ *p_buf++) & 0xff];
	}
	
	return (crc);
}

  
static __inline int su_accept(wp_mtp1_link_t *p_lnk, int err_code)
{
	int err=-1;

	if (p_lnk->rx_buf_len < 3) {
		if (p_lnk->reg.rx_suerm) {
			err=p_lnk->reg.rx_suerm(p_lnk->reg.priv_ptr);
		}
		return err;
	}
	

	if (p_lnk->reg.trace && err_code == 0) {
		p_lnk->reg.trace(p_lnk->reg.priv_ptr, &p_lnk->rx_buf[0], p_lnk->rx_buf_len-2, 1);
	}
	
	if (p_lnk->reg.rx_data) {		
		err=p_lnk->reg.rx_data(p_lnk->reg.priv_ptr,&p_lnk->rx_buf[0],p_lnk->rx_buf_len, err_code);		
	}
	
	return err;
}	



static __inline int suerm (wp_mtp1_link_t *p_lnk)
{
	int err=-1;
	
	/* Pass suerm up the stack */
	if (p_lnk->reg.rx_suerm) {
		err=p_lnk->reg.rx_suerm(p_lnk->reg.priv_ptr);
	}
	
	return err;
}

static void octet_count_inc (wp_mtp1_link_t * p_lnk)
		/******************************************************************************/
{
	p_lnk->r_octet_bit_count++;
	if (p_lnk->r_octet_bit_count >= 8)
	{
		p_lnk->r_octet_counting_count++;
		if (p_lnk->r_octet_counting_count >= p_lnk->cfg.param_N)
		{
			p_lnk->r_octet_counting_count = 0;
			suerm (p_lnk);
		}
	}
}


static void octet_count_start (wp_mtp1_link_t * p_lnk)
		/******************************************************************************/
{

#if 0
	if (p_lnk->r_octet_counting_mode)
	{
#ifdef DEBUG
		DEBUG_EVENT("CONTINUE OCTET COUNTING MODE\n");
#endif
		//putchar ('.');
		octet_count_inc (p_lnk);
		p_lnk->state_daedr = DAEDRX_0;
	}
	else
#endif

	{
#ifdef DEBUG
		DEBUG_EVENT("START OCTET COUNTING MODE\n");
#endif
		p_lnk->r_octet_counting_mode = 1;
		p_lnk->r_octet_counting_count = 0;
		octet_count_inc (p_lnk);
		p_lnk->state_daedr = DAEDRX_0;
	}
}


static void rx_bit_to_su (wp_mtp1_link_t * p_lnk, u8 bit)
		/******************************************************************************/
{
	uint16_t crc;
	uint8_t	su_bit;

#ifdef DEBUG
		if (p_lnk->prev_state_daedr != p_lnk->state_daedr) {
			DEBUG_EVENT("%s: Old State %i New State = %i\n",
					p_lnk->name,p_lnk->prev_state_daedr,p_lnk->state_daedr);
			p_lnk->prev_state_daedr = p_lnk->state_daedr;
		}
#endif

	switch (p_lnk->state_daedr)
	{
	case INIT:
		/* come here once when link (re)starts */
		p_lnk->r_su_accepted = 0;
		p_lnk->r_su_rejected = 0;
		p_lnk->r_fisu_count = 0;
		p_lnk->r_lssu_1_count = 0;
		p_lnk->r_lssu_2_count = 0;
		p_lnk->r_msu_count = 0;
		
		p_lnk->r_flag_accum = 0;
		p_lnk->r_octet_counting_mode = 0;
		p_lnk->state_daedr = DAEDRX_0;
		
	case DAEDRX_0:
		/* sync-up; find the first flag in the bitstream */
		if (p_lnk->r_octet_counting_mode) octet_count_inc (p_lnk);
		p_lnk->r_flag_accum = (p_lnk->r_flag_accum >> 1) | bit;
		switch (p_lnk->r_flag_accum)
		{
			case FLAG: 
				/* first flag found */
				p_lnk->r_flag_count++;
				p_lnk->r_bit_gate_count = 7;
				p_lnk->state_daedr = DAEDRX_1;
				break;
			case 0x7f:
			case 0xfe:
				/* too many consecutive 1's */
				su_accept(p_lnk,1<<WP_MTP1_ABORT_ERROR_BIT);
				octet_count_start (p_lnk);
				break;
			default:
				break;
		}
		break;
		
	case DAEDRX_1:
		/* push the last flag out of the flag accumulator */
		p_lnk->r_flag_accum = (p_lnk->r_flag_accum >> 1) | bit;
		if (p_lnk->r_octet_counting_mode) octet_count_inc (p_lnk);
		if (p_lnk->r_bit_gate_count > 0)
		{
			/* getting rid of the flag a bit at a time */
			p_lnk->r_bit_gate_count--;
			break;
		}
		else
		{
			/* get ready to collect SU bits */
			p_lnk->r_accum = 0;
			p_lnk->r_octet_bit_count = 0;
			
			memset(&p_lnk->rx_buf,0,sizeof(p_lnk->rx_buf));
			p_lnk->rx_buf_len=0;
			
			p_lnk->r_1s_bit_count = 0;
			p_lnk->state_daedr = DAEDRX_2;
		}

		switch (p_lnk->r_flag_accum)
		{
			case FLAG: 
				/* back-to-back flag found */
				p_lnk->r_flag_count++;
				p_lnk->r_bit_gate_count = 7;
				p_lnk->state_daedr = DAEDRX_1;
				break;
			case 0x7f:
			case 0xfe:
				/* too many consecutive 1's */
				su_accept(p_lnk,1<<WP_MTP1_ABORT_ERROR_BIT);
				octet_count_start (p_lnk);
				break;
			default:
				break;
		}
		break;
		
	case DAEDRX_2:
		/* collect and process SU bits */
		if (p_lnk->r_octet_counting_mode) octet_count_inc (p_lnk);
			/* bit 0 in flag accumulator is bit to be shifted into the
			* SU octet accumulator */
		su_bit = (p_lnk->r_flag_accum & 0x01) ? 0x80 : 0x00;
		/* remove stuffed 0-bits */
		if (su_bit > 0) /* bit is 1 */
		{
			p_lnk->r_1s_bit_count++;
			if (p_lnk->r_1s_bit_count >= 7)
			{
				octet_count_start (p_lnk);
				/* things are screwed up; bail */
				return;
			}
		}
		else /* bit is 0 */
		{
			if (p_lnk->r_1s_bit_count == 5)
			{
					/* stuffed 0-bit found; ignore it 
				* so that it is removed from 
					* the stream */
#ifdef DEBUG
					DEBUG_EVENT("\t\t\t\t\t\tdeleted a 0 bit\n");
#endif
					p_lnk->r_1s_bit_count = 0;
					goto DAEDRX_2_CHECK_FLAG;
			}
			p_lnk->r_1s_bit_count = 0;
		}

		/* found a bit that we can keep */
		p_lnk->r_accum = (p_lnk->r_accum >> 1) | su_bit;
		p_lnk->r_octet_bit_count++;
			
		/* have we accumulated an octet? */
		if (p_lnk->r_octet_bit_count < 8)
		{
			/* still collecting bits for this octet */
			goto DAEDRX_2_CHECK_FLAG;
		}
		if (p_lnk->r_octet_bit_count == 8)
		{
			/* we have a signal unit octet; how many do we 
			* have so far for this SU? */
			if (p_lnk->rx_buf_len > p_lnk->cfg.max_mru)
			{
				su_accept(p_lnk,1<<WP_MTP1_FRAME_ERROR_BIT);
				
				memset(&p_lnk->rx_buf,0,sizeof(p_lnk->rx_buf));
				p_lnk->rx_buf_len=0;
		
				octet_count_start (p_lnk);
			
				/* things are wacky; bail */
				return;
			}
			
			/* a reasonable SU octet has been recovered; store it in a
			* signal unit buffer; TODO: this buffer should be allocated
			* mem, right? */
			p_lnk->rx_buf[p_lnk->rx_buf_len]=p_lnk->r_accum;
			p_lnk->rx_buf_len++;

			/* reset the counters */
			p_lnk->r_octet_bit_count = 0;
			
			/* TODO: do we need to constantly zero the SU octet
			* accumulator? */
			p_lnk->r_accum = 0;
		}
		else
		{
#ifdef DEBUG
			/* program is borked */
			DEBUG_EVENT ("%s: octet bit count < 8:octet bit count follow",
						 p_lnk->name,p_lnk->r_octet_bit_count);
#endif

			p_lnk->state_daedr = INIT;
		}

		/* intentional fall-through to DAEDRX_2_CHECK_FLAG */
DAEDRX_2_CHECK_FLAG:
		p_lnk->r_flag_accum = (p_lnk->r_flag_accum >> 1) | bit;
		switch (p_lnk->r_flag_accum)
		{
			case FLAG: 
				/* closing flag found; we'll find out
				* if this is an opener 8 bits later;
				* we should be stopped on an octet
				* boundry */
				if (p_lnk->r_octet_bit_count != 0)
				{
					/* leftover bits exist; signal unit bit
					* count is not evenly divisable by 8 */
					suerm (p_lnk);
					su_accept(p_lnk,1<<WP_MTP1_FRAME_ERROR_BIT);
					goto DAEDRX_2_RESET_VARS;
				}
				if (p_lnk->rx_buf_len < 3)
				{
					suerm (p_lnk);
					su_accept(p_lnk,1<<WP_MTP1_FRAME_ERROR_BIT);
					goto DAEDRX_2_RESET_VARS;
				} 
				
				crc = crc_tss_rev(
						0xffff, p_lnk->rx_buf_len , &p_lnk->rx_buf[0]);
#ifdef DEBUG
				int i;
				uint8_t * p_octet;

				if (p_lnk->rx_buf_len >= 7)
				{
					DEBUG_EVENT("su crc: 0x%04x\n", crc);
					p_octet = (uint8_t *)&p_lnk->rx_buf[0];
					for (i=0; i < p_lnk->rx_buf_len; i++) 
						DEBUG_EVENT("msu octet %d: 0x%02x\n",
							i, 
							*p_octet++);
				}
#endif
				/* if (crc != 0x1d0f) */
				if (crc != 0xf0b8)
				{
					suerm(p_lnk);
					su_accept(p_lnk,1<<WP_MTP1_CRC_ERROR_BIT);
					goto DAEDRX_2_RESET_VARS;
				}			
				
				/* get a good SU */
				su_accept(p_lnk,0);
				
				/* intentional fall-through to DAEDRX_2_RESET_VARS */
DAEDRX_2_RESET_VARS:
				p_lnk->r_flag_count++;
				p_lnk->r_bit_gate_count = 7;
				p_lnk->state_daedr = DAEDRX_1;
				
				memset(p_lnk->rx_buf,0,sizeof(p_lnk->rx_buf));
				p_lnk->rx_buf_len=0;
				
				break;
			case 0x7f:
			case 0xfe:
				/* too many consecutive 1's */
				su_accept(p_lnk,1<<WP_MTP1_ABORT_ERROR_BIT);
				octet_count_start (p_lnk);
				break;
			default:
				break;
		}
		break;
			
	default:
		break;
	}
}



int daed_rx (wp_mtp1_link_t * mtp1_link, u8 octet)
{
	mtp1_link->r_total_octet_count++;

	
	if (mtp1_link->cfg.hdlc_bit_endian == WP_MTP1_HDLC_BIT_0_FIRST)	{
		/*
		rx_bit_to_su ((octet & 0x01) << 7);
		rx_bit_to_su ((octet & 0x02) << 6);
		rx_bit_to_su ((octet & 0x04) << 5);
		rx_bit_to_su ((octet & 0x08) << 4);
		rx_bit_to_su ((octet & 0x10) << 3);
		rx_bit_to_su ((octet & 0x20) << 2);
		rx_bit_to_su ((octet & 0x40) << 1);
		if (clear_channel_DS0) rx_bit_to_su (octet & 0x80);
		*/
		rx_bit_to_su (mtp1_link, (octet & 0x01) ? 0x80 : 0);
		rx_bit_to_su (mtp1_link, (octet & 0x02) ? 0x80 : 0);
		rx_bit_to_su (mtp1_link, (octet & 0x04) ? 0x80 : 0);
		rx_bit_to_su (mtp1_link, (octet & 0x08) ? 0x80 : 0);
		rx_bit_to_su (mtp1_link, (octet & 0x10) ? 0x80 : 0);
		rx_bit_to_su (mtp1_link, (octet & 0x20) ? 0x80 : 0);
		rx_bit_to_su (mtp1_link, (octet & 0x40) ? 0x80 : 0);
		
		if (mtp1_link->cfg.hdlc_op_mode == WP_MTP1_HDLC_8BIT) {
			rx_bit_to_su (mtp1_link, octet & 0x80);
		}
		
	} else	{
		
		if (mtp1_link->cfg.hdlc_op_mode == WP_MTP1_HDLC_8BIT) {
			rx_bit_to_su (mtp1_link, octet & 0x80);
		}
		rx_bit_to_su (mtp1_link, (octet & 0x40) << 1);
		rx_bit_to_su (mtp1_link, (octet & 0x20) << 2);
		rx_bit_to_su (mtp1_link, (octet & 0x10) << 3);
		rx_bit_to_su (mtp1_link, (octet & 0x08) << 4);
		rx_bit_to_su (mtp1_link, (octet & 0x04) << 5);
		rx_bit_to_su (mtp1_link, (octet & 0x02) << 6);
		rx_bit_to_su (mtp1_link, (octet & 0x01) << 7);
	}
	
	return 0;
}



/*===============================================
  DAED TX FUNCTIONS
  ==============================================*/

void su_bit_to_tbs(wp_mtp1_link_t * p_lnk, const unsigned int bit)
{
	/* don't use bit 7 of a 56kb/s link */
	if (p_lnk->tbs.cur_bit_pos == 7) 
	{
		if (p_lnk->cfg.hdlc_op_mode == WP_MTP1_HDLC_7BIT)
		{
			p_lnk->tbs.buf[p_lnk->tbs.cur_index] |= BIT_7;
			p_lnk->tbs.cur_bit_pos++;
			if (p_lnk->tbs.cur_bit_pos >= BITS_IN_OCTET)
			{
#ifdef DEBUG
				DEBUG_EVENT("56k octet to txb: 0x%02x at offset %u\n", 
					   p_lnk->tbs.buf[p_lnk->tbs.cur_index], p_lnk->tbs.cur_index);
#endif
				p_lnk->tbs.cur_bit_pos = 0;
				p_lnk->tbs.cur_index++;
				if (p_lnk->tbs.cur_index >= TX_BUF_SIZE) {
					p_lnk->tbs.cur_index = 0;
				}
				p_lnk->tbs.buf[p_lnk->tbs.cur_index] = 0;
			}
		}
	}
	/* add the bit to the octet being built */
	p_lnk->tbs.buf[p_lnk->tbs.cur_index] |= 
			(bit << p_lnk->tbs.cur_bit_pos);
	
	p_lnk->tbs.cur_bit_pos++;

	if (p_lnk->tbs.cur_bit_pos >= BITS_IN_OCTET)
	{
#ifdef DEBUG
		DEBUG_EVENT("octet to txb: 0x%02x at offset %u\n",
			   p_lnk->tbs.buf[p_lnk->tbs.cur_index],
			   p_lnk->tbs.cur_index
			  );
#endif
		p_lnk->tbs.cur_bit_pos = 0;
		p_lnk->tbs.cur_index++;
		if (p_lnk->tbs.cur_index >= TX_BUF_SIZE) { 
			p_lnk->tbs.cur_index = 0;
		}
		p_lnk->tbs.buf[p_lnk->tbs.cur_index] = 0;
	}

	/* here is the 0-bit insertion procedure; if 5 sequential 1's have occured 
	* in the bitstream, then we must add a 0 to the bit stream 
	*/
	if (bit == 1) {
		p_lnk->tbs.cur_1s_bit_count++;
		if (p_lnk->tbs.cur_1s_bit_count == 5) {
#ifdef DEBUG
			DEBUG_EVENT("\t\t\t\tinserting a 0 bit!\n");
#endif
			su_bit_to_tbs (p_lnk, 0); /* recursion */
		}
	} else {
		p_lnk->tbs.cur_1s_bit_count = 0;
	}
}

void flag_bit_to_txb (wp_mtp1_link_t * p_lnk)
{
	unsigned int flag_bits[8] = {0,1,1,1,1,1,1,0};
	int i;

	for (i=0; i < BITS_IN_OCTET; i++)
	{
		/* don't use bit 7 of a 56kb/s link */
		if (p_lnk->tbs.cur_bit_pos == 7) 
		{
			if (p_lnk->cfg.hdlc_op_mode == WP_MTP1_HDLC_7BIT)
			{
				p_lnk->tbs.buf[p_lnk->tbs.cur_index] |= BIT_7;
				p_lnk->tbs.cur_bit_pos++;
				if (p_lnk->tbs.cur_bit_pos >= BITS_IN_OCTET)
				{
#ifdef DEBUG
					DEBUG_EVENT("56k flag octet to txb: 0x%02x at offset %u\n", 
						   p_lnk->tbs.buf[p_lnk->tbs.cur_index], p_lnk->tbs.cur_index);
#endif
					p_lnk->tbs.cur_bit_pos = 0;
					p_lnk->tbs.cur_index++;
					if (p_lnk->tbs.cur_index >= TX_BUF_SIZE) {
						p_lnk->tbs.cur_index = 0;
					}
					p_lnk->tbs.buf[p_lnk->tbs.cur_index] = 0;
				}
			}
		}
		/* add the bit to the octet being built */
		p_lnk->tbs.buf[p_lnk->tbs.cur_index] |= 
				(flag_bits[i] << p_lnk->tbs.cur_bit_pos);
	
		p_lnk->tbs.cur_bit_pos++;

		if (p_lnk->tbs.cur_bit_pos >= BITS_IN_OCTET)
		{
			
#ifdef DEBUG
			DEBUG_EVENT("flag octet to txb: 0x%02x at offset %u\n", 
				   p_lnk->tbs.buf[p_lnk->tbs.cur_index], p_lnk->tbs.cur_index);
#endif
			p_lnk->tbs.cur_bit_pos = 0;
			p_lnk->tbs.cur_index++;
			if (p_lnk->tbs.cur_index >= TX_BUF_SIZE) {
				p_lnk->tbs.cur_index = 0;
			}
			p_lnk->tbs.buf[p_lnk->tbs.cur_index] = 0;
		}
	}
}

int daed_tx (wp_mtp1_link_t * mtp1_link, u8* buf, int len, u16 *crc)			
{
	int err=0,i;
	
	mtp1_link->tbs.cur_1s_bit_count = 0;
	flag_bit_to_txb(mtp1_link);

	for (i=0; i < len; i++){
		if (mtp1_link->cfg.hdlc_bit_endian == WP_MTP1_HDLC_BIT_0_FIRST)	{
			
			su_bit_to_tbs (mtp1_link, buf[i] & 0x01);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x02) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x04) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x08) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x10) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x20) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x40) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x80) ? 1 : 0);
			
		} else {
			
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x80) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x40) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x20) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x10) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x08) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x04) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, (buf[i] & 0x02) ? 1 : 0);
			su_bit_to_tbs (mtp1_link, buf[i] & 0x01);
		}
	}

	if (crc) {
		u8 cbuf[2];

		cbuf[0]=*crc&0xFF;
		cbuf[1]=(*crc&0xFF00)>>8;
		
		for (i=0; i < 2; i++){
			if (mtp1_link->cfg.hdlc_bit_endian == WP_MTP1_HDLC_BIT_0_FIRST)	{
			
				su_bit_to_tbs (mtp1_link, cbuf[i] & 0x01);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x02) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x04) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x08) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x10) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x20) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x40) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x80) ? 1 : 0);
			
			} else {
			
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x80) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x40) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x20) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x10) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x08) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x04) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, (cbuf[i] & 0x02) ? 1 : 0);
				su_bit_to_tbs (mtp1_link, cbuf[i] & 0x01);
			}
		}
	}
	
	return err;
}


/*===============================================
  PUBLIC FUNCTIONS
  ==============================================*/
static int mtp1_gcnt=0;
void *wp_mtp1_register(wp_mtp1_reg_t *reg)
{
	wp_mtp1_link_t *mtp1_link;
	
	if (!reg->cfg.max_mru) {
		return NULL;
	}
	
	if (reg->cfg.max_mru > RX_BUF_SIZE) {
		reg->cfg.max_mru = RX_BUF_SIZE;
	}
	
	mtp1_link=wan_malloc(sizeof(wp_mtp1_link_t));
	
	if (mtp1_link == NULL) {
		DEBUG_ERROR("%s() Failed to alloc memory size=%i\n",__FUNCTION__,sizeof(wp_mtp1_link_t));
		return NULL;	
	}
	
	memset(mtp1_link,0,sizeof(wp_mtp1_link_t));
	
	memcpy(&mtp1_link->reg,reg,sizeof(mtp1_link->reg));
	memcpy(&mtp1_link->cfg,&reg->cfg,sizeof(mtp1_link->cfg));

	wan_skb_queue_init(&mtp1_link->tbs.tx_q_dealloc);
	wan_skb_queue_init(&mtp1_link->tbs.tx_q);
	wan_skb_queue_init(&mtp1_link->tbs.tx_r_q);
	wan_skb_queue_init(&mtp1_link->rx_list);
	wan_skb_queue_init(&mtp1_link->tx_list);

	memset(mtp1_link->tbs.idle_buf,0x7E,sizeof(mtp1_link->tbs.idle_buf));

	sprintf(mtp1_link->name,"mtp1-%i",++mtp1_gcnt);
	
	return mtp1_link;
}

int wp_mtp1_free(void *mtp1)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;
	netskb_t *skb;	
	
	wan_set_bit(0,&mtp1_link->init);
	
	while((skb=wan_skb_dequeue(&mtp1_link->tbs.tx_q_dealloc)) != NULL) {
		wan_skb_free(skb);	
	}
	while((skb=wan_skb_dequeue(&mtp1_link->tbs.tx_r_q)) != NULL) {
		wan_skb_free(skb);	
	}
	while((skb=wan_skb_dequeue(&mtp1_link->tbs.tx_q)) != NULL) {
		wan_skb_free(skb);	
	}
	
	wan_free(mtp1_link);
	
	return 0;
}

int wp_mtp1_rx_handler(void *mtp1, u8 *data, int len)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;
	int i;
	int err=0;
	
	for (i=0;i<len;i++) {
		err=daed_rx(mtp1_link,data[i]);
		if (err) {
			break;	
		}
	}
	
	return err;	
}

int wp_mtp1_reset(void *mtp1)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;
	mtp1_link->state_daedr = INIT;
	return 0;
}

int wp_mtp1_tx_check(void *mtp1)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;
	
	if (wan_skb_queue_len(&mtp1_link->tbs.tx_q) > WP_MTP1_MAX_TX_Q) {
		return 1;
	}
	
	return 0;
}

int wp_mtp1_tx_data(void *mtp1, netskb_t *skb, wp_api_hdr_t *hdr, netskb_t *rskb)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;
	u16 crc;
	
	if (wan_skb_queue_len(&mtp1_link->tbs.tx_q) > WP_MTP1_MAX_TX_Q) {
		return 1;
	}

	crc=~crc_tss_rev (0xffff, wan_skb_len(skb), wan_skb_data(skb));
	wan_skb_set_csum(skb,crc);
	wan_skb_queue_tail(&mtp1_link->tbs.tx_q,skb);
	
	if (mtp1_link->reg.trace) {
		mtp1_link->reg.trace(mtp1_link->reg.priv_ptr, wan_skb_data(skb), wan_skb_len(skb), 0);
	}
	
	if (rskb) {
		crc=~crc_tss_rev (0xffff, wan_skb_len(rskb), wan_skb_data(rskb));
		wan_skb_set_csum(rskb,crc);
		wan_skb_queue_tail(&mtp1_link->tbs.tx_r_q,rskb);
	}
	
	return 0;
}

int wp_mtp1_poll_check(void *mtp1)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;

	
	if (wan_skb_queue_len(&mtp1_link->tbs.tx_q_dealloc) > 20) {
		return 1;
	}

#if 0
	if (wan_skb_queue_len(&mtp1_link->rx_list) < mtp1_link->cfg.max_rx_q/2) {
		return 1;
	}
#endif

	return 0;
}

/* This function must periodically to freeup
   discarded frames */
int wp_mtp1_poll(void *mtp1, wan_spinlock_t *lock)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;
	wan_smp_flag_t flags;
	netskb_t *skb=NULL;
	int retry=0;
	int loops=0;

	flags=0;

	if (!mtp1_link) {
		return -1;
	}

	
	
	do {

		retry=0;
		
		loops++;
		if (loops > 200) {
			break;
		}
		
		wp_mtp1_os_lock_irq(lock,&flags);
		skb=wan_skb_dequeue(&mtp1_link->tbs.tx_q_dealloc);
		wp_mtp1_os_unlock_irq(lock,&flags);
		if (skb) {
			wan_skb_free(skb);
			retry++;
		}

		skb=NULL;
#if 0
		wp_mtp1_os_lock_irq(lock,&flags);
		qlen=wan_skb_queue_len(&mtp1_link->rx_list);
		wp_mtp1_os_unlock_irq(lock,&flags);

		if (qlen < mtp1_link->cfg.max_rx_q) {
			skb=wan_skb_alloc(mtp1_link->cfg.max_mru+128);
			if (skb) {
				wp_mtp1_os_lock_irq(lock,&flags);
				wan_skb_queue_tail(&mtp1_link->rx_list,skb);
				wp_mtp1_os_unlock_irq(lock,&flags);
			}
			retry++;
		}
#endif

	}while (retry);

	DEBUG_TEST("mtp1 poll loops=%i!\n",loops);
	
	return 0;
}

int wp_mtp1_tx_bh_handler(void *mtp1, u8 *data, int mtu)
{
	wp_mtp1_link_t *mtp1_link = (wp_mtp1_link_t*)mtp1;
	wp_mtp1_link_t *p_lnk = (wp_mtp1_link_t*)mtp1;
	netskb_t *skb;
	int i;
	int err=0;
	int r;
	int k;
	u16 crc;
	
	while ((p_lnk->tbs.cur_index - p_lnk->tbs.card_index) < mtu) {
		skb=wan_skb_dequeue(&p_lnk->tbs.tx_q);
		if (skb) {
			crc = wan_skb_csum(skb);
			daed_tx(mtp1_link, wan_skb_data(skb), wan_skb_len(skb), &crc);
			wan_skb_queue_tail(&p_lnk->tbs.tx_q_dealloc,skb);
			if (wan_skb_queue_len(&p_lnk->tbs.tx_q) < WP_MTP1_MAX_TX_Q) {
				p_lnk->reg.wakeup(p_lnk->reg.priv_ptr);	
			}
		} else if (wan_skb_queue_len(&p_lnk->tbs.tx_r_q)) {
			for (;;) {
				skb=wan_skb_dequeue(&p_lnk->tbs.tx_r_q);
				if (!skb) {
					break;
				}
				if (wan_skb_queue_len(&p_lnk->tbs.tx_r_q)){
					wan_skb_queue_tail(&p_lnk->tbs.tx_q_dealloc,skb);
				} else {
					break;
				}
			}
			if (skb) {
				crc = wan_skb_csum(skb);
				daed_tx(mtp1_link, wan_skb_data(skb), wan_skb_len(skb), &crc);
				if (wan_skb_queue_len(&p_lnk->tbs.tx_r_q)){
					wan_skb_queue_tail(&p_lnk->tbs.tx_q_dealloc,skb);
				} else {
					wan_skb_queue_tail(&p_lnk->tbs.tx_r_q,skb);
				}
			} else {
				daed_tx(mtp1_link,mtp1_link->tbs.idle_buf,0,NULL);
			}

		} else {
			daed_tx(mtp1_link,mtp1_link->tbs.idle_buf,0,NULL);
		}
	}
	
	/* put the tx octets for this DS0 into the outbound bitstream buffer */
	for (i = 0; i < mtu; i++){
		//p_lnk->w_total_octet_count++;
		data[i] = p_lnk->tbs.buf[p_lnk->tbs.card_index++];
	}
	
	/* move contents at the bottom of the buffer to the top if needed */
	r = p_lnk->tbs.cur_index - p_lnk->tbs.card_index;
	if (r < mtu) {
		/* move octets at the bottom of the buffer to the top; we must get 
		* r + 1 octets because there might be a partially built octet in
		* the r-th octet */
		k = p_lnk->tbs.card_index;
		for (i = 0; i <= r; i++, k++){
			p_lnk->tbs.buf[i] = p_lnk->tbs.buf[k];
		}
	
		/* adjust indicies to the top of the buffer */
		p_lnk->tbs.cur_index = r;
		p_lnk->tbs.card_index = 0;
	}
	
	return err;
}

