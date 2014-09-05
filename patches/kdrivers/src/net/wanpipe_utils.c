/*
 * Copyright (c) 1997, 1998, 1999
 *	Alex Feldman <al.feldman@sangoma.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Alex Feldman.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Alex Feldman AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Alex Feldman OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: wanpipe_utils.c,v 1.101 2008-02-04 18:02:20 sangoma Exp $
 */

/*
 ****************************************************************************
 * wan_utils.c	WANPIPE(tm) Multiprotocol WAN Utilities.
 *
 * Author:	Alex Feldman    <al.feldman@sangoma.com> 
 *
 * ===========================================================================
 * July 29, 2002	Alex Feldman	Initial version.
 ****************************************************************************
*/


#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_snmp.h"
#include "wanpipe.h"
#include "if_wanpipe_common.h"


#ifdef __OpenBSD__
# define read_eflags()	({ register u_long ef;		\
		__asm("pushfl; popl %0" : "=r" (ef));	\
		ef;})
# define write_eflags(x)({register u_long ef = (x);	\
		__asm("pushl %0; popfl" : : "r" (ef));})
#endif

/*
** Function prototypes.
*/
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void wanpipe_debug_timer(void* arg);
#elif defined(__WINDOWS__)
static void wanpipe_debug_timer(IN PKDPC Dpc, void* arg, void* arg2, void* arg3);
extern int set_netdev_state(sdla_t* card, netdevice_t* sdla_net_dev, int state);
#else
static void wanpipe_debug_timer(unsigned long arg);
#endif

#ifdef __WINDOWS__
#define wp_os_lock_irq(card,flags)   wan_spin_lock_irq(card,flags)
#define wp_os_unlock_irq(card,flags) wan_spin_unlock_irq(card,flags)
#else
#define wp_os_lock_irq(card,flags)
#define wp_os_unlock_irq(card,flags)
#endif

extern sdla_t*	wanpipe_debug;
int wan_get_dbg_msg(wan_device_t* wandev, void* u_dbg_msg);
void wanpipe_set_dev_carrier_state(sdla_t* card, int state);

unsigned char wp_brt[256];

static void wan_generate_bit_rev_table(void)
{
	unsigned char util_char;
	unsigned char misc_status_byte;
	int i;

	/* generate the bit-reversing table for all unsigned characters */
	for(util_char = 0;; util_char ++) {
		misc_status_byte = 0;			/* zero the character to be 'built' */
		/* process all 8 bits of the source byte and generate the */
		for(i = 0; i <= 7; i ++) {
		      	/* corresponding 'bit-flipped' character */
			if(util_char & (1 << i)) {
				misc_status_byte |= (1 << (7 - i));
			}
		}
		/* insert the 'bit-flipped' character into the table 
		 * at the appropriate location */
		wp_brt[util_char] = misc_status_byte; 

		/* exit when all unsigned characters have been processed */
		if(util_char == 0xFF) {
			break;
		}
	}
}



/*============================================================================
 * Convert decimal string to unsigned integer.
 * If len != 0 then only 'len' characters of the string are converted.
 */
unsigned int wan_dec2uint (unsigned char* str, int len)
{
	unsigned val;

	if (!len) 
		len = strlen(str);

	for (val = 0; len && is_digit(*str); ++str, --len)
		val = (val * 10) + (*str - (unsigned)'0');

	return val;
}

/* 
 * ============================================================================
 * Get WAN device state.
 */
char* wanpipe_get_state_string (void* card_id)
{
	sdla_t*		card = (sdla_t*)card_id;

	switch(card->wandev.state){
	case WAN_UNCONFIGURED:
		return "Unconfigured";
	case WAN_DISCONNECTED:
		return "Disconnected";
	case WAN_CONNECTING:
		return "Connecting";
	case WAN_CONNECTED:
		return "Connected";
	case WAN_DISCONNECTING:
		return "disconnecting";
	case WAN_FT1_READY:			
		return "FT1 Ready";
	}
	return "Unknown";
}

/* 
 * ============================================================================
 * Get WAN device state.
 */
char wanpipe_get_state (void* card_id)
{
	sdla_t*	card = (sdla_t*)card_id;
	
	return card->wandev.state;
}

/* 
 * ============================================================================
 * Get WAN device state.
 */
void wanpipe_set_baud (void* card_id, unsigned int baud)
{
	sdla_t*	card = (sdla_t*)card_id;

	/* Setup for bps */
	card->wandev.bps=baud*1000;
}

void wanpipe_set_dev_carrier_state(sdla_t* card, int state)
{

#if defined(__WINDOWS__)
	{
		int i;
		for (i = 0; i < NUM_OF_E1_CHANNELS; i++){
			if(card->sdla_net_device[i] != NULL && wan_test_bit(0,&card->up[i]) ){
				set_netdev_state(card, card->sdla_net_device[i], state);
			}
		}
	}
#else
	netdevice_t *dev;
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
        if (dev && WAN_NETIF_UP(dev)) {
		if (state == WAN_CONNECTED) {
                 	WAN_NETIF_CARRIER_ON(dev);
	       		WAN_NETIF_WAKE_QUEUE(dev);       		
		} else {
                	WAN_NETIF_CARRIER_OFF(dev);
	       		WAN_NETIF_STOP_QUEUE(dev); 
		}			
	}
#endif
}
           

/* 
 * ============================================================================
 * Set WAN device state.
 */
void wanpipe_set_state (void* card_id, int state)
{
	sdla_t*	card = (sdla_t*)card_id;
	if (card->wandev.state != state){ 
		switch (state){ 
		case WAN_CONNECTED:
			DEBUG_EVENT("%s: Link connected!\n", card->devname);
			card->wan_debugging_state = WAN_DEBUGGING_NONE;
			break;

		case WAN_CONNECTING:
			DEBUG_EVENT("%s: Link connecting...\n", card->devname);
			break;

		case WAN_DISCONNECTED:
			DEBUG_EVENT("%s: Link disconnected!\n", card->devname);
			break;
		}
		card->wandev.state = (char)state;

		if (card->wandev.config_id == WANCONFIG_ADSL) {
			wanpipe_set_dev_carrier_state(card,state);
		}   
	}
	card->state_tick = SYSTEM_TICKS;
}

/*
 * ============================================================================
 * Check if packet is UDP packet or not.
 */
int wan_udp_pkt_type(void* card_id, caddr_t ptr)
{
	sdla_t*		card = (sdla_t*)card_id;
	wan_udp_pkt_t* 	wan_udp_pkt = (wan_udp_pkt_t*)ptr; 
	unsigned int 	udp_port = card->wandev.udp_port;

	DEBUG_UDP("%s: Ver 0x%x Prot 0x%x UDP type %d Req 0x%x\n",
					card->devname,
					wan_udp_pkt->wan_ip_v,
					wan_udp_pkt->wan_ip_p,
					wan_udp_pkt->wan_udp_dport,
					wan_udp_pkt->wan_udp_request_reply);

	if (wan_udp_pkt->wan_ip_v == 0x04 &&
	    wan_udp_pkt->wan_ip_p == UDPMGMT_UDP_PROTOCOL &&
	    wan_udp_pkt->wan_udp_dport == ntohs((u16)udp_port) &&  
	    wan_udp_pkt->wan_udp_request_reply == UDPMGMT_REQUEST){

		if (!strncmp(wan_udp_pkt->wan_udp_signature,
			     GLOBAL_UDP_SIGNATURE,
			     GLOBAL_UDP_SIGNATURE_LEN)){
			return 0;
		}
	}
    	return UDP_INVALID_TYPE;
}

/*
 * ============================================================================
 * Calculate packet checksum.
 */
unsigned short wan_calc_checksum(char *data, int len)
{
	unsigned short temp;
	u_int32_t sum = 0;
	int i;
  
	for(i=0; i < len; i += 2){
		memcpy(&temp, &data[i], 2);
		sum += (u_int32_t)temp;
	}

	while(sum >> 16){
		sum = (sum & 0xffffUL) + (sum >> 16);
	}    

	temp = (unsigned short)sum;
	temp = ~temp;
 
	if (temp == 0){
		temp = 0xFFFF;
	}

	return temp;
} 

/*
 * ===========================================================================
 * Reply to UDP Management system.
 * Agruments:
 *	mbox_len - includes data length and trace_info_t (chdlc and dsl).
 * Return length of reply.
 */
int wan_reply_udp(void* card_id, unsigned char *data, unsigned int mbox_len)
{
	sdla_t*		card = (sdla_t*)card_id;
	wan_udp_pkt_t*	udp_pkt = (wan_udp_pkt_t*)data;
	u_int32_t 	ip_temp;
	unsigned short 	len, 
			udp_length, 
			temp, 
			ip_length;
	int 		even_bound = 0;
 

	/* fill in UDP reply */
	udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;

	/* fill in UDP length */
	udp_length = (unsigned short)(sizeof(struct udphdr) + 
			sizeof(wan_mgmt_t) + 
			sizeof(wan_cmd_t) +
			mbox_len);

	if (card->wandev.config_id == WANCONFIG_CHDLC || 
	    card->wandev.config_id == WANCONFIG_SDLC){
	       	udp_length += sizeof(wan_trace_info_t);
	}

	/* Set length of packet */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	len = sizeof(struct ip) + udp_length;
#else
	len = sizeof(struct iphdr) + udp_length;
#endif

	/* put it on an even boundary */
	if (udp_length & 0x0001){ 
		udp_length += 1;
		len += 1;
		even_bound = 1;
	} 

	temp = (udp_length<<8)|(udp_length>>8);
	udp_pkt->wan_udp_len = temp;

	/* swap UDP ports */
	temp = udp_pkt->wan_udp_sport;
	udp_pkt->wan_udp_sport = udp_pkt->wan_udp_dport;
	udp_pkt->wan_udp_dport = temp;

	/* add UDP pseudo header */
	temp = 0x1100;
	*((unsigned short*)(udp_pkt->wan_udp_data + mbox_len + even_bound)) = temp;
	temp = (udp_length<<8)|(udp_length>>8);
	*((unsigned short*)(udp_pkt->wan_udp_data + mbox_len + even_bound + 2)) = temp;

	/* calculate UDP checksum */
	udp_pkt->wan_udp_sum = 0;
	udp_pkt->wan_udp_sum = 
		wan_calc_checksum(&data[UDP_OFFSET],
				  udp_length + UDP_OFFSET);

	/* fill in IP length */
	ip_length = len;
	temp = (ip_length<<8)|(ip_length>>8);
	udp_pkt->wan_ip_len = temp;

	/* swap IP addresses */
	ip_temp = udp_pkt->wan_ip_src;
	udp_pkt->wan_ip_src = udp_pkt->wan_ip_dst;
	udp_pkt->wan_ip_dst = ip_temp;
    
	/* fill in IP checksum */
	udp_pkt->wan_ip_sum = 0;
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	udp_pkt->wan_ip_sum = wan_calc_checksum(data, sizeof(struct ip)); 
#else
	udp_pkt->wan_ip_sum = wan_calc_checksum(data, sizeof(struct iphdr)); 
#endif

	return len;
} /* wan_reply_udp */



#if defined(__LINUX__)
/*
 * ===========================================================================
 * Reply to UDP Management system.
 * Agruments:
 *	mbox_len - includes data length and trace_info_t (chdlc and dsl).
 * Return length of reply.
 */
int wan_ip_udp_setup(void* card_id, wan_rtp_conf_t *rtp_conf, 
		     u32 chan,
		     unsigned char *data, unsigned int mbox_len)
{
	wan_rtp_pkt_t*	rtp_pkt = (wan_rtp_pkt_t*)data;
	unsigned short 	len, 
			udp_length, 
			temp, 
			ip_length;
	int 		even_bound = 0;


	/* fill in UDP length */
	udp_length =  sizeof(struct udphdr) + 
		      sizeof(wan_rtp_hdr_t) + 
		      mbox_len;


	/* Set length of packet */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	len = sizeof(struct ip) + udp_length;
#else
	len = sizeof(struct iphdr) + udp_length;
#endif

	/* put it on an even boundary */
#if 1
	if (udp_length & 0x0001){ 
		udp_length += 1;
		data[sizeof(ethhdr_t)+sizeof(iphdr_t)+udp_length]=0;
		len += 1;
		even_bound = 1;
	} 
#endif

	temp = (udp_length<<8)|(udp_length>>8);
	rtp_pkt->wan_udp_len = temp;

	temp = rtp_pkt->wan_udp_sport;
	rtp_pkt->wan_udp_sport = htons(rtp_conf->rtp_port+chan);
	rtp_pkt->wan_udp_dport = htons(rtp_conf->rtp_port+chan);

	/* calculate UDP checksum */
	rtp_pkt->wan_udp_sum = 0;
	rtp_pkt->wan_udp_sum = 0;
	#if 0
		wan_calc_checksum(&data[sizeof(ethhdr_t)+UDP_OFFSET],
				  udp_length+UDP_OFFSET);
	#endif
	
	/* fill in IP length */
	ip_length = len;
	temp = (ip_length<<8)|(ip_length>>8);
	rtp_pkt->wan_ip_len = temp;

	/* IP addresses */
	rtp_pkt->wan_ip_src = rtp_conf->rtp_local_ip;
	rtp_pkt->wan_ip_dst = rtp_conf->rtp_ip;
	rtp_pkt->wan_ip_v  = 4;
	rtp_pkt->wan_ip_hl = 5;
	rtp_pkt->wan_ip_len = htons(ip_length);
	rtp_pkt->wan_ip_tos = 1;
	rtp_pkt->wan_ip_ttl = 255;
	rtp_pkt->wan_ip_p = IPPROTO_UDP;

	memcpy(rtp_pkt->wan_eth_dest,rtp_conf->rtp_mac,ETH_ALEN);
	memcpy(rtp_pkt->wan_eth_src,rtp_conf->rtp_local_mac,ETH_ALEN);
	rtp_pkt->wan_eth_proto=0x0008;
    
	/* fill in IP checksum */
	rtp_pkt->wan_ip_sum = 0;
	rtp_pkt->wan_ip_sum = wan_calc_checksum(&data[sizeof(ethhdr_t)], sizeof(iphdr_t)); 

	return len+sizeof(ethhdr_t);
} /* wan_ip_udp_setup */
#endif

/*
*****************************************************************************
**
**
*/

/*
** Initialize WANPIPE debug timer 
*/
void wanpipe_debug_timer_init(void* card_id)
{
	sdla_t*	card = (sdla_t*)card_id;
	wan_init_timer(&card->debug_timer, 
		       wanpipe_debug_timer, 
		       (wan_timer_arg_t)card);	
	return;
}


#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void wanpipe_debug_timer(void* arg)
#elif defined(__WINDOWS__)
static void wanpipe_debug_timer(IN PKDPC Dpc, void* arg, void* arg2, void* arg3)
#else
static void wanpipe_debug_timer(unsigned long arg)
#endif
{
	sdla_t*		card = (sdla_t*)arg;
	wan_tasklet_t*	debug_task = NULL;
	/*
	** I should set running flag to zero in order to 
	** schedule this task. I don't care if after setting 
	** this flag to zero, the same task can be scheduled from
	** another place before I'll try to do this here, only 
	** one task will go through and the result will be same.
	*/
	WAN_ASSERT1(card == NULL);
	debug_task = &card->debug_task;
	if (card->wandev.state == WAN_UNCONFIGURED){
		return;
	}
	WAN_TASKLET_END(debug_task);
	WAN_TASKLET_SCHEDULE(debug_task);
}

/*
*****************************************************************************
**
**
*/
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
void wanpipe_debugging (void* data, int pending)
#else
void wanpipe_debugging (ulong_ptr_t data)
#endif
{
	sdla_t*			card = (sdla_t*)data;
	wan_tasklet_t*		debug_task = NULL;
	static u_int32_t	CRC_frames = 0, abort_frames = 0;
	static u_int32_t	tx_underun_frames = 0;
	int			delay = 0;

	WAN_ASSERT1(card == NULL);
	if (card->wandev.state == WAN_UNCONFIGURED){
		goto wanpipe_debugging_end;
	}
	if (card->wandev.state == WAN_CONNECTED){
		/*
		** Terminate sequence because link comes UP.
		*/
		DEBUG_DBG("%s: WAN_DBG: Link came UP before the end of the DEBUG sequence!\n",
					card->devname);
		goto wanpipe_debugging_end;
	}
	if (card->wan_debugging_state == WAN_DEBUGGING_NONE ||
	    card->wan_debugging_state == WAN_DEBUGGING_AGAIN){
		DEBUG_DBG("%s: WAN_DBG: Debugging state is %s!\n",
					card->devname,
					(card->wan_debugging_state == WAN_DEBUGGING_NONE) ? "Start":"Again");
		card->wan_debugging_state = WAN_DEBUGGING_START;
	}
	if (IS_TE1_CARD(card) || IS_56K_CARD(card)){ 
		u_int32_t status;
		/* Sangoma T1/E1/56K cards */
		/* Check Tv attributes */
		if (card->wandev.fe_iface.read_alarm){ 
			card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
		}
		if (card->wandev.fe_iface.read_alarm && 
		    (status = card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE))){
			DEBUG_DBG("%s: WAN_DBG: T1/E1/56K is not in Service, print Alarms!\n",
						card->devname);
			/* Not in service, Alarms */
			if (card->wan_debug_last_msg != WAN_DEBUG_ALARM_MSG){
				card->wandev.fe_iface.print_fe_alarm(&card->fe, status);
				DEBUG_EVENT("%s: Line for WANPIPEn is down.\n",
						card->devname);
				DEBUG_EVENT("%s:    All WANPIPEn interfaces down.\n",
						card->devname);
				DEBUG_EVENT("%s:    Tell your Telco about your\n",
						card->devname);
				DEBUG_EVENT("%s:    ALARM condition.\n",
						card->devname);
			}
			card->wan_debug_last_msg = WAN_DEBUG_ALARM_MSG;
		}else{
			/* In service, no Alarms */
			switch(card->wan_debugging_state){
			case WAN_DEBUGGING_START:
				DEBUG_DBG("%s: WAN_DBG: T1/E1/56K in Service, reading statistics...\n",
						card->devname);
				/* Read number of CRC, abort and tx frames */
				CRC_frames		= card->get_crc_frames(card);
				abort_frames		= card->get_abort_frames(card);
				tx_underun_frames 	= card->get_tx_underun_frames(card);
				card->wan_debugging_state = WAN_DEBUGGING_CONT;
				wan_add_timer(&card->debug_timer, 15*HZ);
				return;
				break;

			case WAN_DEBUGGING_CONT:
				if (IS_TE1_CARD(card) &&
				    ((card->get_crc_frames(card) != CRC_frames) ||
				     (card->get_abort_frames(card) != abort_frames))){
					DEBUG_DBG("%s: WAN_DBG: T1/E1/56K in Service, RX CRC/Abort frames!\n",
						card->devname);
					/* CRC/Abort frames, Probably incorrect channel config */
					if (card->wan_debug_last_msg != WAN_DEBUG_TE1_MSG){
						DEBUG_EVENT("%s: T1/E1 channel set-up is wrong!\n",
								card->devname);
						DEBUG_EVENT("%s:    Your current channel configuration is %s!\n",
								card->devname,
								card->wandev.fe_iface.print_fe_act_channels(&card->fe));
						DEBUG_EVENT("%s:    Have your Telco check the \n",
								card->devname);
						DEBUG_EVENT("%s:    active DS0 channels on your line.\n",
								card->devname);
					}
					card->wan_debug_last_msg = WAN_DEBUG_TE1_MSG;
				}else{
					/* no CRC/Abort frames */
					goto line_down_without_DSU_CSU_error;
				}
				break;

			case WAN_DEBUGGING_PROTOCOL:
				DEBUG_DBG("%s: WAN_DBG: Debugging protocol level...\n",
							card->devname);
				if (card->wan_debugging){
					delay = card->wan_debugging(card);
				}
				break;
			}
		}		
	}else{
		/* Sangoma Serial cards */
		switch(card->wan_debugging_state){
		case WAN_DEBUGGING_START:
			DEBUG_DBG("%s: WAN_DBG: Serial card, reading statistics...\n",
						card->devname);
			/* Check CRC and abort stats for 15 second */
			CRC_frames	= card->get_crc_frames(card);
			abort_frames	= card->get_abort_frames(card);
			tx_underun_frames = card->get_tx_underun_frames(card);
			card->wan_debugging_state = WAN_DEBUGGING_CONT;
			wan_add_timer(&card->debug_timer, 15*HZ);
			return;	
			break;

		case WAN_DEBUGGING_CONT:
			if ((card->get_crc_frames(card) != CRC_frames) ||
			    (card->get_abort_frames(card) != abort_frames)){
				DEBUG_DBG("%s: WAN_DBG: Serial card, RX CRC/Abort frames!\n",
						card->devname);
				/* CRC/Abort frames, Probably incorrect channel config */
				if (card->wandev.clocking == WANOPT_EXTERNAL){
					if (card->wan_debug_last_msg != WAN_DEBUG_LINERROR_MSG){
						/* External clocking */
						DEBUG_ERROR("%s: Your line is experiencing errors!\n",
									card->devname);
						DEBUG_EVENT("%s:    Check your DSU/CSU and line configuration\n",
									card->devname);
						DEBUG_EVENT("%s:    with your Telco.\n",
									card->devname);
					}
					card->wan_debug_last_msg = WAN_DEBUG_LINERROR_MSG;
				}else{
					/* Internal clocking */
					if (card->wan_debug_last_msg != WAN_DEBUG_CLK_MSG){
						DEBUG_EVENT("%s: You are set for Internal clocking, \n",
									card->devname);
						DEBUG_ERROR("%s: causing line errors!\n",
									card->devname);
						DEBUG_EVENT("%s:    Set to External clocking except for\n",
									card->devname);
						DEBUG_EVENT("%s:    back-to-back testing!\n",
									card->devname);
					}
					card->wan_debug_last_msg = WAN_DEBUG_CLK_MSG;
				}
			}else{
				/* no CRC/Abort frames */
				goto line_down_without_DSU_CSU_error;
			}
			break;
		
		case WAN_DEBUGGING_PROTOCOL:
			DEBUG_DBG("%s: WAN_DBG: Debugging protocol level...\n",
							card->devname);
			if (card->wan_debugging){
				delay = card->wan_debugging(card);
			}
			break;
		}
	}
	/* Link is down because DSU/CSU errors */	
	goto wanpipe_debugging_end;

line_down_without_DSU_CSU_error:
	/* Link is down (no DSU/CSU errors) - may be protocol problem */	
	if (card->get_tx_underun_frames(card) != tx_underun_frames){ 
		DEBUG_DBG("%s: WAN_DBG: No TX frames!\n",
					card->devname);
		if (card->wan_debug_last_msg != WAN_DEBUG_TX_MSG){
			DEBUG_EVENT("%s: WAN port is not transmitting!\n",
						card->devname);
			DEBUG_EVENT("%s:    No TX clock from DSU/CSU.\n",
						card->devname);
			DEBUG_EVENT("%s:    Check your cable with your DSU/CSU supplier.\n",
						card->devname);
		}
		card->wan_debug_last_msg = WAN_DEBUG_TX_MSG;
	}else{
		if (card->wan_debugging){
			delay = card->wan_debugging(card);
		}
		if (card->wan_debugging_state == WAN_DEBUGGING_PROTOCOL){
			wan_add_timer(&card->debug_timer, delay*HZ);
			return;
		}
	}
	goto wanpipe_debugging_end;	

wanpipe_debugging_end:
	/*
	** WANPIPE debugging task is finished. Clear debugging flag!
 	*/
	card->wan_debugging_state = WAN_DEBUGGING_NONE;
	debug_task = &card->debug_task;
	WAN_TASKLET_END(debug_task);
	if (card->wandev.state != WAN_CONNECTED && card->wandev.state != WAN_UNCONFIGURED){
		/*
		** Repeat the WANPIPE debugging task in next 60 seconds.
		*/
		card->wan_debugging_state = WAN_DEBUGGING_AGAIN;
		wan_add_timer(&card->debug_timer, WAN_DEBUGGING_DELAY*HZ);
	}else{
		/* 
		** Link is connected, We don't need to remember the last message 
		*/
		card->wan_debug_last_msg = WAN_DEBUG_NONE_MSG;
		WAN_DEBUG_STOP(card);
	}
	return;
} 

/* ALEX_DEBUG */ 
void wan_debug_trigger(int flag)
{
	if (wanpipe_debug == NULL){
		return;
	}
	switch(flag){
	case WAN_DEBUG_SET_TRIGGER:
		wan_set_bit(WAN_DEBUG_TRIGGER, &wanpipe_debug->u.debug.status);
		break;

	case WAN_DEBUG_CLEAR_TRIGGER:
		wan_clear_bit(WAN_DEBUG_TRIGGER, &wanpipe_debug->u.debug.status);
		break;
	}
	return;
}

void wan_debug_write(char* msg)
{
	wanpipe_kernel_msg_info_t	wan_debug_info;
	wanpipe_kernel_msg_hdr_t	wan_dbg;
	int				len = strlen(msg);	
	wan_smp_flag_t			smp_flags;

	if (wanpipe_debug == NULL || !wanpipe_debug->configured){
		return;
	}
	if (!wan_test_bit(WAN_DEBUG_TRIGGER, &wanpipe_debug->u.debug.status)){
		/* No Debug Trigger */
		return;  
	}
	if (wan_test_bit(WAN_DEBUG_READING, &wanpipe_debug->u.debug.status) ||
	    wan_test_bit(WAN_DEBUG_FULL, &wanpipe_debug->u.debug.status)){
		/* We cannot add new message! */
	       	return;
	}
	if (len == 0){
		DEBUG_EVENT("wan_debug_write: Debug message is empty!\n");
		return;
	}
	wan_spin_lock_irq(&wanpipe_debug->wandev.lock, &smp_flags);

	wanpipe_debug->hw_iface.peek(wanpipe_debug->hw, 0, &wan_debug_info, sizeof(wanpipe_kernel_msg_info_t));
	if (wan_debug_info.magic != ROUTER_MAGIC || wanpipe_debug->u.debug.total_len == 0){
		wan_debug_info.magic	= ROUTER_MAGIC;
		wan_debug_info.total_len = 0;
		wanpipe_debug->hw_iface.poke(wanpipe_debug->hw, 0, 
			  &wan_debug_info, sizeof(wanpipe_kernel_msg_info_t));
		wanpipe_debug->u.debug.total_len = 0;
		wanpipe_debug->u.debug.total_num = 0;
		wanpipe_debug->u.debug.current_offset = sizeof(wanpipe_kernel_msg_info_t);
	}
	if (wan_debug_info.total_len + sizeof(wanpipe_kernel_msg_info_t) + len > (256 * 1024)){
		DEBUG_EVENT("wan_debug_write: There are no enought memory (%ld)!\n",
				wan_debug_info.total_len);
		DEBUG_EVENT("wan_debug_write: Disable debug message output!\n");
		wan_set_bit(WAN_DEBUG_FULL, &wanpipe_debug->u.debug.status);
		goto wan_debug_write_done;
	}
	wan_dbg.len = len;
	wan_getcurrenttime(&wan_dbg.time, NULL);
	wanpipe_debug->hw_iface.poke(wanpipe_debug->hw, 
		  wanpipe_debug->u.debug.total_len+sizeof(wanpipe_kernel_msg_info_t), 
		  &wan_dbg, 
		  sizeof(wanpipe_kernel_msg_hdr_t));
	wanpipe_debug->u.debug.total_len += sizeof(wanpipe_kernel_msg_hdr_t);
	wanpipe_debug->hw_iface.poke(wanpipe_debug->hw, 
		  wanpipe_debug->u.debug.total_len+sizeof(wanpipe_kernel_msg_info_t), 
		  msg, 
		  len);
	wanpipe_debug->u.debug.total_len += len;
	wanpipe_debug->u.debug.total_num ++;
	wan_debug_info.total_len = wanpipe_debug->u.debug.total_len;
	wanpipe_debug->hw_iface.poke(wanpipe_debug->hw, 
		  0, 
		  &wan_debug_info, 
		  sizeof(wanpipe_kernel_msg_info_t));
wan_debug_write_done:
	wan_spin_unlock_irq(&wanpipe_debug->wandev.lock, &smp_flags);
	return;
}

int wan_debug_read(void* device, void* u_dbg_msg)
{
	wanpipe_kernel_msg_info_t	wan_debug_info;
	wan_kernel_msg_t		dbg_msg;
	wanpipe_kernel_msg_hdr_t*	wan_dbg;
	char*				data = NULL;
	unsigned long			next_offset = 0, offset = 0;
	int 				read_done = 0, err = 0;
	wan_smp_flag_t			smp_flags;

	if (!wanpipe_debug || wanpipe_debug->wandev.state == WAN_UNCONFIGURED){
		return -EINVAL;
	}
	wan_spin_lock_irq(&wanpipe_debug->wandev.lock, &smp_flags);
	/* We are busy to read all debug message. Do not add new messages yet! */
	if (!wan_test_bit(WAN_DEBUG_READING, &wanpipe_debug->u.debug.status)){
		wan_set_bit(WAN_DEBUG_READING, &wanpipe_debug->u.debug.status);
		DEBUG_DBG("wan_debug_read: Number of debug messages %ld\n", 
				wanpipe_debug->u.debug.total_num);
	}
	if (WAN_COPY_FROM_USER((void*)&dbg_msg, (void*)u_dbg_msg, sizeof(wan_kernel_msg_t))){
		wan_clear_bit(WAN_DEBUG_READING, &wanpipe_debug->u.debug.status);
		err = -EFAULT;
		goto wan_get_dbg_msg_error;
	}
		
	if (dbg_msg.magic != ROUTER_MAGIC){
		wan_clear_bit(WAN_DEBUG_READING, &wanpipe_debug->u.debug.status);
		err = -EINVAL;
		goto wan_get_dbg_msg_error;
	}
	if ((data = wan_malloc(dbg_msg.max_len)) == NULL){
		wan_clear_bit(WAN_DEBUG_READING, &wanpipe_debug->u.debug.status);
		err = -ENOMEM;
		goto wan_get_dbg_msg_error;
	}

	wanpipe_debug->hw_iface.peek(wanpipe_debug->hw, 0, 
		  &wan_debug_info, sizeof(wanpipe_kernel_msg_info_t));
	if (wan_debug_info.total_len == 0 || wan_debug_info.magic != ROUTER_MAGIC){
		DEBUG_DBG("%s: There are no debug messages!\n", 
					wanpipe_debug->devname);
		dbg_msg.len = 0;
		goto wan_get_dbg_msg_done;
	}
	next_offset = wanpipe_debug->u.debug.current_offset;
	if (wan_debug_info.total_len-next_offset > dbg_msg.max_len){
		dbg_msg.is_more = 1;
	}
 
	do {
		wanpipe_debug->hw_iface.peek(wanpipe_debug->hw, next_offset, 
			  &data[offset], sizeof(wanpipe_kernel_msg_hdr_t));
		wan_dbg = (wanpipe_kernel_msg_hdr_t*)&data[offset];
		if (wan_dbg->len == 0){
			DEBUG_DBG("wan_debug_read: Debug msg is empty!\n");
		}
		if (offset + sizeof(wanpipe_kernel_msg_hdr_t) + wan_dbg->len < dbg_msg.max_len){
			next_offset += sizeof(wanpipe_kernel_msg_hdr_t);
			offset += sizeof(wanpipe_kernel_msg_hdr_t);
			wanpipe_debug->hw_iface.peek(wanpipe_debug->hw, next_offset, &data[offset], wan_dbg->len);
			next_offset += wan_dbg->len;
			offset += wan_dbg->len;
			dbg_msg.num ++;
		}else{
			read_done = 1;
		}
		if (next_offset >= wan_debug_info.total_len){
			read_done = 1;
		}
	}while(!read_done);
	wanpipe_debug->u.debug.current_offset = next_offset;

wan_get_dbg_msg_done:
	if (offset){
		if (WAN_COPY_TO_USER((void*)dbg_msg.data, (void*)data, offset)){
			err = -EFAULT;
			goto wan_get_dbg_msg_done;
		}
	}
	dbg_msg.len = offset;
	if (!dbg_msg.is_more){
		DEBUG_DBG("%s: There are no more debug messages!\n", 
					wanpipe_debug->devname);
		wan_debug_info.magic	= ROUTER_MAGIC;
		wan_debug_info.total_len	= 0;
		wanpipe_debug->u.debug.current_offset = 
				sizeof(wanpipe_kernel_msg_info_t);
		wanpipe_debug->u.debug.total_len = 0;
		wanpipe_debug->u.debug.total_num = 0;
		wanpipe_debug->hw_iface.poke(wanpipe_debug->hw, 0, 
			  &wan_debug_info, sizeof(wanpipe_kernel_msg_info_t));
		if (wan_test_bit(WAN_DEBUG_FULL, &wanpipe_debug->u.debug.status)){
			DEBUG_EVENT("wan_debug_write: Enable debug message output!\n");
			wan_clear_bit(WAN_DEBUG_FULL, &wanpipe_debug->u.debug.status);
		}
		wan_clear_bit(WAN_DEBUG_READING, &wanpipe_debug->u.debug.status);
	}
	if (WAN_COPY_TO_USER((void*)u_dbg_msg, (void*)&dbg_msg, sizeof(wan_kernel_msg_t))){
		err = -EFAULT;
		goto wan_get_dbg_msg_done;
	}
wan_get_dbg_msg_error:
	if (data){
		wan_free(data);
	}
	wan_spin_unlock_irq(&wanpipe_debug->wandev.lock, &smp_flags);
	return err;

}

int wan_tracing_enabled(wan_trace_t *trace_info)
{
	WAN_ASSERT(trace_info == NULL);
	
	if (wan_test_bit(0,&trace_info->tracing_enabled)){

		if ((SYSTEM_TICKS - trace_info->trace_timeout) > WAN_MAX_TRACE_TIMEOUT){
			DEBUG_EVENT("wanpipe: Disabling trace, timeout!\n");
			wan_clear_bit(0,&trace_info->tracing_enabled);
			wan_clear_bit(1,&trace_info->tracing_enabled);
			return -EINVAL;
		}

		if (wan_skb_queue_len(&trace_info->trace_queue) < (int)trace_info->max_trace_queue){

			if(wan_test_bit(3,&trace_info->tracing_enabled)){
				/* Trace ALL cells, including Idle.
				** Check that trace queue is NOT more than half full. */
				if (wan_skb_queue_len(&trace_info->trace_queue) < (int)trace_info->max_trace_queue / 2){
					/*it is ok to enqueue an idle cell. */
					return 3;
				}else{
					return -ENOBUFS; 
				}
			}

			if (wan_test_bit(1,&trace_info->tracing_enabled)){
				return 1;
			}
		
			if (wan_test_bit(2,&trace_info->tracing_enabled)){
				return 2;
			}
			
			return 0;
		}

		if (WAN_NET_RATELIMIT()){
			DEBUG_WARNING("wanpipe: Warning: trace queue overflow %d (max=%d)!\n",
					wan_skb_queue_len(&trace_info->trace_queue),
					trace_info->max_trace_queue);
		}
		return -ENOBUFS;
	}
	return -EINVAL;

}


int wan_capture_trace_packet_buffer(sdla_t *card, wan_trace_t* trace_info, char *data, int len, char direction, int channel)
{
	void* new_skb = NULL;
	wan_trace_pkt_t trc_el;	
	int		flag = 0;
	int 	total_len=0;
	char	*buf;

	/* The tracing is done by wan_capture_trace_packet function */
	if (!wan_test_bit(8,&trace_info->tracing_enabled)){
		return 0;
	}

	if ((flag = wan_tracing_enabled(trace_info)) >= 0){

		/* Allocate a header mbuf */
		total_len = len + sizeof(wan_trace_pkt_t);

		new_skb = wan_skb_alloc(total_len);
		if (new_skb == NULL) 
			return -1;

		wan_getcurrenttime(&trc_el.sec, &trc_el.usec);
		trc_el.status		= direction;
		trc_el.data_avail	= 1;
		trc_el.time_stamp	= 
			(unsigned short)((((trc_el.sec * 1000000) + trc_el.usec) / 1000) % 0xFFFF);
		trc_el.real_length	= len;
		trc_el.channel=channel;

		buf=wan_skb_put(new_skb, sizeof(wan_trace_pkt_t));
		memcpy(buf,(caddr_t)&trc_el,sizeof(wan_trace_pkt_t));

		buf=wan_skb_put(new_skb,len);
		memcpy(buf,data,len);
		
		wan_skb_queue_tail(&trace_info->trace_queue, new_skb);
	}

	return flag;
}


int wan_capture_trace_packet(sdla_t *card, wan_trace_t* trace_info, netskb_t *skb, char direction)
{
	int		len = 0;
	void*		new_skb = NULL;
	wan_trace_pkt_t trc_el;	
	int		flag = 0;
	wan_smp_flag_t smp_flags;

	smp_flags=0;
	
	/* 8 is a spacial buffer trace */
	if (wan_test_bit(8,&trace_info->tracing_enabled)){
		return 0;
	}

	if ((flag = wan_tracing_enabled(trace_info)) >= 0){

		/* Allocate a header mbuf */
		len = 	wan_skb_len(skb) + sizeof(wan_trace_pkt_t);

		new_skb = wan_skb_alloc(len);
		if (new_skb == NULL) 
			return -1;

		wan_getcurrenttime(&trc_el.sec, &trc_el.usec);
		trc_el.status		= direction;
		trc_el.data_avail	= 1;
		trc_el.time_stamp	= 
			(unsigned short)((((trc_el.sec * 1000000) + trc_el.usec) / 1000) % 0xFFFF);
		trc_el.real_length	= (u16)wan_skb_len(skb);
		
		wan_skb_copyback(new_skb, 
				 wan_skb_len(new_skb), 
			   	 sizeof(wan_trace_pkt_t), 
				 (caddr_t)&trc_el);
	
		wan_skb_copyback(new_skb, 
				 wan_skb_len(new_skb), 
				 wan_skb_len(skb),
				 (caddr_t)wan_skb_data(skb));
		
		wp_os_lock_irq(&card->wandev.lock,&smp_flags);
		wan_skb_queue_tail(&trace_info->trace_queue, new_skb);
		wp_os_unlock_irq(&card->wandev.lock,&smp_flags);
	}

	return 0;
}



int wan_capture_trace_packet_offset(sdla_t *card, wan_trace_t* trace_info, netskb_t *skb, int off,char direction)
{
    int	flag = 0;
	
	/* 8 is a spacial buffer trace */
	if (wan_test_bit(8,&trace_info->tracing_enabled)){
		return -EBUSY;
	}

	if ((flag = wan_tracing_enabled(trace_info)) >= 1){

		int             len = 0;
	        void*           new_skb = NULL;
        	wan_trace_pkt_t trc_el, *trc_el_ptr; 
       	 	int             i,drop=1;
        	unsigned char   diff=0,*src_ptr,*dest_ptr;

		if (off < 1){
			return -EINVAL;
		}

		/* Allocate a header mbuf */
		len = 	wan_skb_len(skb) + sizeof(wan_trace_pkt_t) + 16;

		new_skb = wan_skb_alloc(len);
		if (new_skb == NULL) 
			return -1;

		wan_getcurrenttime(&trc_el.sec, &trc_el.usec);
		trc_el.status		= direction;
		trc_el.data_avail	= 1;
		trc_el.time_stamp	= 
			(unsigned short)((((trc_el.sec * 1000000) + trc_el.usec) / 1000) % 0xFFFF);
		trc_el.real_length	= (u16)wan_skb_len(skb);
		
		wan_skb_copyback(new_skb, 
				 wan_skb_len(new_skb), 
			   	 sizeof(wan_trace_pkt_t), 
				 (caddr_t)&trc_el);

		trc_el.real_length=0;
		src_ptr=wan_skb_data(skb);
		for (i=(off-1);i<wan_skb_len(skb);){	
			dest_ptr=wan_skb_put(new_skb,1);
			dest_ptr[0]=src_ptr[i];
			trc_el.real_length++;

			if (i==(off-1)){
				diff=dest_ptr[0];
			}

			if (diff != dest_ptr[0]){
				drop=0;
			}

			if (IS_T1_CARD(card)){
				i+=24;
			}else{
				i+=31;
			}
		}

		if (drop){
			wan_skb_free(new_skb);
			return 0;
		}

		trc_el_ptr=(wan_trace_pkt_t*)wan_skb_data(new_skb);
		trc_el_ptr->real_length = trc_el.real_length;		
	
		wan_skb_queue_tail(&trace_info->trace_queue, new_skb);
	}

	return 0;
}


void wan_trace_info_init(wan_trace_t *trace, int max_trace_queue)
{
	wan_skb_queue_t*	trace_queue = NULL;

	trace_queue = &trace->trace_queue;
	WAN_IFQ_INIT(trace_queue, max_trace_queue);
	trace->trace_timeout	= SYSTEM_TICKS;
	trace->tracing_enabled	= 0;
	trace->max_trace_queue	= max_trace_queue;
}

int wan_trace_purge (wan_trace_t *trace)
{
	wan_skb_queue_t*	trace_queue = NULL;

	WAN_ASSERT(trace == NULL);
	trace_queue = &trace->trace_queue;
	WAN_IFQ_PURGE(trace_queue);
	return 0;
}

int wan_trace_enqueue(wan_trace_t *trace, void *skb_ptr)
{
	wan_skb_queue_t*	trace_queue = NULL;
	netskb_t*		skb = (netskb_t*)skb_ptr;
	int			err = 0;

	WAN_ASSERT(trace == NULL);
	trace_queue = &trace->trace_queue;
	WAN_IFQ_ENQUEUE(trace_queue, skb, NULL, err);
	return err;
}


/*
*****************************************************************************
**
**
*/
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
u_char ds1_variables_oid[] = { 1,3,6,1,2,1,10,18 };
u_char fr_variables_oid[] =  { 1,3,6,1,2,1,10,32 };
u_char ppp_variables_oid[] = { 1,3,6,1,2,1,10,23 };
u_char x25_variables_oid[] = { 1,3,6,1,2,1,10,5  };

int wan_snmp_data(sdla_t* card, netdevice_t* dev, int cmd, struct ifreq* ifr)
{
#if !defined(__WINDOWS__)
	wanpipe_snmp_t	snmp;
	switch(cmd){
	case SIOC_WANPIPE_SNMP:
		break;

	case SIOC_WANPIPE_SNMP_IFSPEED:
		if (WAN_COPY_TO_USER(ifr->ifr_data, &card->wandev.bps, sizeof(card->wandev.bps))){
			DEBUG_EVENT("%s: %s(): Line: %d: Failed to copy kernel space to user snmp data!\n",
				card->devname, __FUNCTION__, __LINE__);
			return -EFAULT;
		}
		return 0;
	default:
		DEBUG_EVENT("%s: Unknown cmd: %d (0x%X) \n",__FUNCTION__,cmd,cmd);
	}

	if (WAN_COPY_FROM_USER(&snmp, ifr->ifr_data, sizeof(wanpipe_snmp_t))){
		DEBUG_EVENT("%s: Failed to copy user snmp data to kernel space!\n",
				card->devname);
		return -EFAULT;
	}

	if (strncmp((char *)ds1_variables_oid,(char *)snmp.snmp_name, sizeof(ds1_variables_oid)) == 0){
		/* SNMP call for ds1 */
		DEBUG_SNMP("%s: Get T1/E1 SNMP data\n", 
					card->devname);
		if(card->wandev.fe_iface.get_snmp_data){
			card->wandev.fe_iface.get_snmp_data(&card->fe, dev, &snmp);
		}else{
			DEBUG_EVENT("%s: Failed to get Front End SNMP data! Request is invalid for Serial Card.\n",
				card->devname);
		}
	}else{
		if (strncmp((char *)fr_variables_oid,(char *)snmp.snmp_name, sizeof(fr_variables_oid)) == 0){
			/* SNMP call for frame relay */
			DEBUG_SNMP("%s: Get Frame Relay SNMP data\n", card->devname);
		}

		if (strncmp((char *)ppp_variables_oid,(char *)snmp.snmp_name, sizeof(ppp_variables_oid)) == 0){
			/* SNMP call for PPP */
			DEBUG_SNMP("%s: Get PPP SNMP data\n", card->devname);
		}
	
		if (strncmp((char *)x25_variables_oid,(char *)snmp.snmp_name, sizeof(x25_variables_oid)) == 0){
			/* SNMP call for x251 */
			DEBUG_SNMP("%s: Get X.25 SNMP data\n", card->devname);
		}
		if (card->get_snmp_data){
			card->get_snmp_data(card, dev, &snmp);
		}else{
			DEBUG_SNMP("%s: get_snmp_data() ptr is NULL!!\n", card->devname);
		}
	}

	if (WAN_COPY_TO_USER(ifr->ifr_data, &snmp, sizeof(wanpipe_snmp_t))){
		DEBUG_EVENT("%s: Failed to copy kernel space to user snmp data!\n",
				card->devname);
		return -EFAULT;
	}
#endif	
	return 0;
}
#endif

void wanpipe_card_lock_irq(void *card_id, unsigned long *flags)
{
#define card  ((sdla_t*)card_id)
	wan_spin_lock_irq(&card->wandev.lock, (wan_smp_flag_t*)flags);	
#undef card
}

void wanpipe_card_unlock_irq(void *card_id, unsigned long *flags)
{
#define card  ((sdla_t*)card_id)
	wan_spin_unlock_irq(&card->wandev.lock,(wan_smp_flag_t*)flags);	
#undef card
}



/*===================================================
 * Abstracted Wanpipe Specific Functions 
 *===================================================
 */

/* 
 * ============================================================================
 * Get WAN device state.
 */
char wpabs_get_state (void* card_id)
{
	return wanpipe_get_state(card_id);
}

void wpabs_card_lock_irq(void *card_id,unsigned long *flags)
{
	wanpipe_card_lock_irq(card_id,flags);	
}

void wpabs_card_unlock_irq(void *card_id,unsigned long *flags)
{
	wanpipe_card_unlock_irq(card_id,flags);	
}

int wpabs_trace_queue_len(void *trace_ptr)
{
	wan_trace_t*	trace = (wan_trace_t *)trace_ptr;
	wan_skb_queue_t*	trace_queue = NULL;

	WAN_ASSERT(trace == NULL);
	trace_queue = &trace->trace_queue;
	return WAN_IFQ_LEN(trace_queue);
}

int wpabs_tracing_enabled(void *trace_ptr)
{
	return wan_tracing_enabled((wan_trace_t*)trace_ptr);
}

void* wpabs_trace_info_alloc(void)
{
	return wan_malloc(sizeof(wan_trace_t));
}

void wpabs_trace_info_init(void *trace_ptr, int max_trace_queue)
{
	wan_trace_info_init(trace_ptr, max_trace_queue);
}

int wpabs_trace_purge (void *trace_ptr)
{
	return wan_trace_purge(trace_ptr);
}

int wpabs_trace_enqueue(void *trace_ptr, void *skb_ptr)
{
	return wan_trace_enqueue(trace_ptr, skb_ptr);
}

void wpabs_set_state (void* card_id, int state)
{
	wanpipe_set_state(card_id, state);
}

/* 
 * ============================================================================
 * Get WAN device state.
 */
void wpabs_set_baud (void* card_id, unsigned int baud)
{
	wanpipe_set_baud(card_id, baud);
}

/*void* wpabs_dma_alloc(void* pcard, DMA_DESCRIPTION* dma_descr)
*/
void* wpabs_dma_alloc(void* pcard, unsigned long max_length)
{
	sdla_t*			card = (sdla_t*)pcard;
	wan_dma_descr_org_t	*dma_descr = NULL;

	dma_descr = wan_malloc(sizeof(wan_dma_descr_org_t));
	if (dma_descr == NULL){
		return NULL;
	}
	dma_descr->max_length = max_length;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	card->hw_iface.getcfg(card->hw, SDLA_DMATAG, &dma_descr->dmat);
#elif defined(__WINDOWS__)
	dma_descr->DmaAdapterObject = card->DmaAdapterObject;
#endif
	if (wan_dma_alloc_org(card->hw, dma_descr)){
		wan_free(dma_descr);
		return NULL;
	}
	return dma_descr;
}

int wpabs_dma_free(void* pcard, void* dma_descr)
{
	int err=0;
	sdla_t*	card = (sdla_t*)pcard;
	err=wan_dma_free_org(card->hw, (wan_dma_descr_org_t*)dma_descr);
	wan_free(dma_descr);
	return err;
}

/* LIP ATM definitions */
#define ATM_CELL_SIZE	53
#define ATM_HEADER_SZ 	5
#define ATM_PAYLOAD_SZ	48
#define ATM_PAD_CHAR	0x6A

/*#define DEBUG_ATM	DEBUG_EVENT*/
#define DEBUG_ATM	DEBUG_TEST

int init_atm_idle_buffer(unsigned char *buff, int buff_len, char *if_name, char hardware_flip)
{
	const unsigned char idle_cell_header[ATM_HEADER_SZ] = {0x00, 0x00, 0x00, 0x01, 0x52};
	unsigned char idle_cell[ATM_CELL_SIZE];
	int ind, number_of_cells_fit_idle_buffer;

	/* copy header */
	memcpy(idle_cell, idle_cell_header, ATM_HEADER_SZ);
	/* init pad chars */
	memset(&idle_cell[ATM_HEADER_SZ], ATM_PAD_CHAR, ATM_PAYLOAD_SZ);
	
	number_of_cells_fit_idle_buffer = buff_len / ATM_CELL_SIZE;

	if(number_of_cells_fit_idle_buffer == 0 /*|| buff_len % ATM_CELL_SIZE*/){
	      DEBUG_ERROR("%s: Error: Invalid Idle buffer length=%d, not multiple of ATM_CELL_SIZE (53)!\n",
			if_name, buff_len);
	      return 1;
	}

	/* finally, init the buffer */
	for(ind = 0; ind < number_of_cells_fit_idle_buffer; ind++){
		memcpy(&buff[ind*ATM_CELL_SIZE], idle_cell, ATM_CELL_SIZE);
	}

	return 0;
}

int atm_add_data_to_skb(void* skb, void *data, int data_len, char *if_name)
{
	unsigned char 	*skb_data_ptr;

	if (data_len != ATM_CELL_SIZE) {
                DEBUG_ERROR("%s: %s(): Error, invalid datalen=%i\n",
                        if_name, __FUNCTION__, data_len);
                return 1;
        }

	DEBUG_ATM("%s(): data_len=%d, skb tail room=%d\n", 
			__FUNCTION__, data_len, wan_skb_tailroom(skb));

	if(data_len > wan_skb_tailroom(skb)){
		DEBUG_EVENT("%s: %s(): Warining: 'data_len=%d > skb tail room=%d'!\n", 
			if_name, __FUNCTION__, data_len, wan_skb_tailroom(skb));
		return 1;
	}

	skb_data_ptr = wan_skb_put(skb, data_len);

	/* copy data to the new skb */
	memcpy(skb_data_ptr, data, data_len);

	return 0;
}

int atm_pad_idle_cells_in_tx_skb(void *skb, void *tx_idle_skb, char *if_name)
{
	unsigned char 	*idle_skb_data_ptr;
	int		num_of_cells_to_pad, i, empty_space;
	
	DEBUG_ATM("%s()\n", __FUNCTION__);

	idle_skb_data_ptr = wan_skb_data(tx_idle_skb);

	empty_space = wan_skb_len(tx_idle_skb) - wan_skb_len(skb);
	DEBUG_ATM("empty space length: %d\n", empty_space);

	num_of_cells_to_pad = empty_space / ATM_CELL_SIZE;
	DEBUG_ATM("num_of_cells_to_pad: %d\n", num_of_cells_to_pad);
	
	if(empty_space % ATM_CELL_SIZE){
		DEBUG_ERROR("%s: %s(): Error, empty space length (%d) is not multiple of ATM_CELL_SIZE!\n", 
			if_name, __FUNCTION__, empty_space);
		return 1;
	}

	if(num_of_cells_to_pad){
		for(i = 0 ; i < num_of_cells_to_pad; i++){
			if(atm_add_data_to_skb(skb, idle_skb_data_ptr, ATM_CELL_SIZE, if_name)){
				return 2;
			}
		}
	}

	return 0;
}




void *atm_tx_skb_dequeue(void* wp_tx_pending_list, void *tx_idle_skb, char *if_name)
{
	void	*tx_skb, *single_cell_skb;
	int	max_num_of_cells_fit_in_tx_buffer, i;
	int  	err;

	if(wan_skb_queue_len(wp_tx_pending_list) == 0){
		return NULL;
	}

	DEBUG_ATM("%s(): queue len: %d\n", __FUNCTION__, wan_skb_queue_len(wp_tx_pending_list));

	tx_skb = wan_skb_alloc(wan_skb_len(tx_idle_skb));
	if (!tx_skb){
		DEBUG_EVENT("%s: Failed to allocate ATM TX skb!\n", if_name);
		return NULL;
	}
	
	max_num_of_cells_fit_in_tx_buffer = wan_skb_len(tx_idle_skb) / ATM_CELL_SIZE;
	DEBUG_ATM("max_num_of_cells_fit_in_tx_buffer=%d\n", max_num_of_cells_fit_in_tx_buffer);

	i=0;
	while(i++ < max_num_of_cells_fit_in_tx_buffer){
	
		single_cell_skb = wan_skb_dequeue(wp_tx_pending_list);
		if(single_cell_skb == NULL){
			/* no more data, but may be idle cells should be added */
			DEBUG_ATM("no more data\n");
			break;
		}

		err=atm_add_data_to_skb(tx_skb,
					wan_skb_data(single_cell_skb),
					wan_skb_len (single_cell_skb),
					if_name);

		wan_skb_free(single_cell_skb);
		if (err) {
			wan_skb_free(tx_skb);
                        return NULL;
		}
	}

 	/* try to add idle cells, if no space it won't do anything */
        err=atm_pad_idle_cells_in_tx_skb(tx_skb, tx_idle_skb, if_name);
        if (err) {
                wan_skb_free(tx_skb);
                return NULL;
        }


	return tx_skb;
}

int wanpipe_globals_util_init(void)
{
	wan_generate_bit_rev_table();

	return 0;
}



/*
 *	Verify iovec. The caller must ensure that the iovec is big enough
 *	to hold the message iovec.
 *
 *	Save time not doing verify_area. copy_*_user will make this work
 *	in any case.
 */

#if defined(__LINUX__)

#if 0 
int wan_verify_iovec(struct msghdr *m, struct iovec *iov, char *address, int mode)
{
	int size, err, ct;
	
	m->msg_name = NULL;
	
	size = m->msg_iovlen * sizeof(struct iovec);
	
	if (copy_from_user(iov, m->msg_iov, size))
		return -EFAULT;

	m->msg_iov = iov;
	err = 0;

	for (ct = 0; ct < m->msg_iovlen; ct++) {
		err += iov[ct].iov_len;
		/*
		 * Goal is not to verify user data, but to prevent returning
		 * negative value, which is interpreted as errno.
		 * Overflow is still possible, but it is harmless.
		 */
		if (err < 0)
			return -EMSGSIZE;
	}

	return err;
}
      
 /*
 *	Copy iovec to kernel. Returns -EFAULT on error.
 *
 *	Note: this modifies the original iovec.
 */
 
int wan_memcpy_fromiovec(unsigned char *kdata, struct iovec *iov, int len)
{
	while (len > 0) {
		if (iov->iov_len) {
			int copy = min_t(unsigned int, len, iov->iov_len);
			if (copy_from_user(kdata, iov->iov_base, copy))
				return -EFAULT;
			len -= copy;
			kdata += copy;
			iov->iov_base += copy;
			iov->iov_len -= copy;
		}
		iov++;
	}

	return 0;
}                

/*
 *	Copy kernel to iovec. Returns -EFAULT on error.
 *
 *	Note: this modifies the original iovec.
 */
 
int wan_memcpy_toiovec(struct iovec *iov, unsigned char *kdata, int len)
{
	while (len > 0) {
		if (iov->iov_len) {
			int copy = min_t(unsigned int, iov->iov_len, len);
			if (copy_to_user(iov->iov_base, kdata, copy))
				return -EFAULT;
			kdata += copy;
			len -= copy;
			iov->iov_len -= copy;
			iov->iov_base += copy;
		}
		iov++;
	}

	return 0;
}             
#endif

void debug_print_udp_pkt(unsigned char *data,int len,char trc_enabled, char direction)
{
#if defined(__LINUX__) && defined(__KERNEL__)
	int i,res;
	DEBUG_EVENT("\n");
	DEBUG_EVENT("%s UDP PACKET: ",direction?"RX":"TX");
	for (i=0; i<sizeof(wan_udp_pkt_t); i++){
		if (i==0){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("IP PKT: ");
		}
		if (i==sizeof(struct iphdr)){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("UDP PKT: ");
		}
		if (i==sizeof(struct iphdr)+sizeof(struct udphdr)){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("MGMT PKT: ");
		}
		if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+sizeof(wan_mgmt_t)){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("CMD PKT: ");
		}
		
		if (trc_enabled){
			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)){
				DEBUG_EVENT("\n");
				DEBUG_EVENT("TRACE PKT: ");
			}
			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+
			       sizeof(wan_trace_info_t)){

				DEBUG_EVENT("\n");
				DEBUG_EVENT("DATA PKT: ");
			}

			res=len-(sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+sizeof(wan_trace_info_t));
		
			res=(res>10)?10:res;

			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+sizeof(wan_trace_info_t)+res){
				break;
			}
			
		}else{
	
			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)){
				DEBUG_EVENT("\n");
				DEBUG_EVENT("DATA PKT: ");
			}

			res=len-(sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t));
		
			res=(res>10)?10:res;

			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+res){
				break;
			}
		}

		_DEBUG_EVENT("%02X ",*(data+i));
	}
	DEBUG_EVENT("\n");
#endif
}


#endif
