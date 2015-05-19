/*****************************************************************************
* sdla_ft1.c	WANPIPE(tm) Multiprotocol WAN Link Driver. Cisco HDLC module.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*		Gideon Hack  
*
* Copyright:	(c) 1995-1999 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Aug 23, 2001  Nenad Corbic    Removed the if_header and set the hard_header
* 			        length to zero. Caused problems with Checkpoint
* 			        firewall.
* Sep 30, 1999  Nenad Corbic    Fixed dynamic IP and route setup.
* Sep 23, 1999  Nenad Corbic    Added SMP support, fixed tracing 
* Sep 13, 1999  Nenad Corbic	Split up Port 0 and 1 into separate devices.
* Jun 02, 1999  Gideon Hack     Added support for the S514 adapter.
* Oct 30, 1998	Jaspreet Singh	Added Support for CHDLC API (HDLC STREAMING).
* Oct 28, 1998	Jaspreet Singh	Added Support for Dual Port CHDLC.
* Aug 07, 1998	David Fong	Initial version.
*****************************************************************************/


#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_debug.h>
#include <linux/wanpipe_common.h>
#include <linux/wanpipe_events.h>
#include <linux/wanpipe_cfg.h>


#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/sdlapci.h>
#include <linux/sdla_chdlc.h>		/* CHDLC firmware API definitions */

/****** Defines & Macros ****************************************************/

/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   	0x0001
#define TMR_INT_ENABLED_UPDATE	0x0002
 
#define	CHDLC_DFLT_DATA_LEN	1500		/* default MTU */
#define CHDLC_HDR_LEN		1

#define IFF_POINTTOPOINT 0x10

#define WANPIPE 0x00
#define API	0x01
#define CHDLC_API 0x01

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )

 
/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with CHDLC specific data
 */

typedef struct chdlc_private_area
{
	netdevice_t 	*slave;
	sdla_t		*card;
	int 		TracingEnabled;		/* For enabling Tracing */
	unsigned long 	curr_trace_addr;	/* Used for Tracing */
	unsigned long 	start_trace_addr;
	unsigned long 	end_trace_addr;
	unsigned long 	base_addr_trace_buffer;
	unsigned long 	end_addr_trace_buffer;
	unsigned short 	number_trace_elements;
	unsigned  	available_buffer_space;
	unsigned long 	router_start_time;
	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	unsigned long 	router_up_time;
        u32             IP_address;		/* IP addressing */
        u32             IP_netmask;
	unsigned char  mc;			/* Mulitcast support on/off */
	unsigned short udp_pkt_lgth;		/* udp packet processing */
	char udp_pkt_src;
	char udp_pkt_data[MAX_LGTH_UDP_MGNT_PKT];
	unsigned short timer_int_enabled;
	char update_comms_stats;		/* updating comms stats */
	//FIXME: add driver stats as per frame relay!

} chdlc_private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03


/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int wpft1_exec (struct sdla *card, void *u_cmd, void *u_data);
static int chdlc_read_version (sdla_t* card, char* str);
static int chdlc_error (sdla_t *card, int err, wan_mbox_t *mb);

/****** Public Functions ****************************************************/

/*============================================================================
 * Cisco HDLC protocol initialization routine.
 *
 * This routine is called by the main WANPIPE module during setup.  At this
 * point adapter is completely initialized and firmware is running.
 *  o read firmware version (to make sure it's alive)
 *  o configure adapter
 *  o initialize protocol-specific fields of the adapter data space.
 *
 * Return:	0	o.k.
 *		< 0	failure.
 */
int wpft1_init (sdla_t* card, wandev_conf_t* conf)
{
	unsigned char port_num;
	int err;

	union
		{
		char str[80];
		} u;
	volatile wan_mbox_t* mb;
	wan_mbox_t* mb1;
	unsigned long timeout;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_CHDLC) {
		printk(KERN_INFO "%s: invalid configuration ID %lu!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Use primary port */
	card->u.c.comm_port = 0;
	

	/* Initialize protocol-specific fields */
	/* Alex Apr 8 2004 Sangoma ISA card */
	card->mbox_off = PRI_BASE_ADDR_MB_STRUCT;

	mb = &card->wan_mbox;
	mb1 = &card->wan_mbox;

	if (!card->configured){
		unsigned char return_code = 0x00;

		/* The board will place an 'I' in the return code to indicate that it is
	   	ready to accept commands.  We expect this to be completed in less
           	than 1 second. */

		timeout = jiffies;
		do {
			return_code = 0x00;
			card->hw_iface.peek(card->hw, 
				card->mbox_off+offsetof(wan_mbox_t, wan_return_code),
				&return_code, 
				sizeof(unsigned char));
			if ((jiffies - timeout) > 1*HZ) break;
		}while(return_code != 'I');
		if (return_code != 'I') {
			printk(KERN_INFO
				"%s: Initialization not completed by adapter\n",
				card->devname);
			printk(KERN_INFO "Please contact Sangoma representative.\n");
			return -EIO;
		}
	}

	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	if (chdlc_read_version(card, u.str))
		return -EIO;

	printk(KERN_INFO "%s: Running FT1 Configuration firmware v%s\n",
		card->devname, u.str); 

	card->isr			= NULL;
	card->poll			= NULL;
	card->exec			= &wpft1_exec;
	card->wandev.update		= NULL;
 	card->wandev.new_if		= NULL;
	card->wandev.del_if		= NULL;
	card->wandev.state		= WAN_DUALPORT;
	card->wandev.udp_port   	= conf->udp_port;

	card->wandev.new_if_cnt = 0;

	/* This is for the ports link state */
	card->u.c.state = WAN_DISCONNECTED;
	
	/* reset the number of times the 'update()' proc has been called */
	card->u.c.update_call_count = 0;
	
	card->wandev.ttl = 0x7F;
	card->wandev.electrical_interface = 0; 

	card->wandev.clocking = 0;

	port_num = card->u.c.comm_port;

	/* Setup Port Bps */

       	card->wandev.bps = 0;

	card->wandev.mtu = MIN_LGTH_CHDLC_DATA_CFG;

	/* Set up the interrupt status area */
	/* Read the CHDLC Configuration and obtain: 
	 *	Ptr to shared memory infor struct
         * Use this pointer to calculate the value of card->u.c.flags !
 	 */
	mb1->wan_data_len = 0;
	mb1->wan_command = READ_CHDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb1);
	if(err != COMMAND_OK) {
		chdlc_error(card, err, mb1);
		return -EIO;
	}

	/* Alex Apr 8 2004 Sangoma ISA card */
	card->flags_off = 
			((CHDLC_CONFIGURATION_STRUCT *)mb1->wan_data)->
			ptr_shared_mem_info_struct;
	
    	card->intr_type_off = 
			card->flags_off +
			offsetof(SHARED_MEMORY_INFO_STRUCT, interrupt_info_struct) +
			offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_type);
	card->intr_perm_off = 
			card->flags_off +
			offsetof(SHARED_MEMORY_INFO_STRUCT, interrupt_info_struct) +
			offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_permission);

	card->wandev.state = WAN_FT1_READY;
	printk(KERN_INFO "%s: FT1 Config Ready !\n",card->devname);

	return 0;
}

static int wpft1_exec(sdla_t *card, void *u_cmd, void *u_data)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int len,err;

	
	if (copy_from_user(mbox, u_cmd, sizeof(wan_cmd_t))){
		DEBUG_EVENT("%s:%d: Failed to copy mbox from user\n",
			   __FUNCTION__,__LINE__);
		return -EFAULT;
	}

	mbox->wan_opp_flag=0;

	len = mbox->wan_data_len;

	if (len) {
		if(copy_from_user((void*)&mbox->wan_data, u_data, len)){
			DEBUG_EVENT("%s:%d: Failed to copy mbox data from user\n",
			   __FUNCTION__,__LINE__);
			return -EFAULT;
		}
	}

	/* execute command */
	if ((err=card->hw_iface.cmd(card->hw, card->mbox_off, mbox)) != 0){
		DEBUG_EVENT("%s:%d: Failed FT1 command err=0x%X\n",
			   __FUNCTION__,__LINE__,err);
		return -EIO;
	}

	/* return result */
	if (copy_to_user(u_cmd, (void*)mbox, sizeof(wan_cmd_t))){
		DEBUG_EVENT("%s:%d: Failed to copy mbox to user\n",
			   __FUNCTION__,__LINE__);
		return -EFAULT;
	}

	len = mbox->wan_data_len;

	if (len && u_data && copy_to_user(u_data, (void*)&mbox->wan_data, len)){
		DEBUG_EVENT("%s:%d: Failed to copy mbox data to user\n",
			   __FUNCTION__,__LINE__);
		return -EFAULT;
	}

	return 0;

}

/*============================================================================
 * Read firmware code version.
 *	Put code version as ASCII string in str. 
 */
static int chdlc_read_version (sdla_t* card, char* str)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int len;
	char err;
	mb->wan_data_len = 0;
	mb->wan_command = READ_CHDLC_CODE_VERSION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		chdlc_error(card,err,mb);
	}
	else if (str) {  /* is not null */
		len = mb->wan_data_len;
		memcpy(str, mb->wan_data, len);
		str[len] = '\0';
	}
	return (err);
}

/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int chdlc_error (sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_command;

	switch (err) {

	case CMD_TIMEOUT:
		printk(KERN_INFO "%s: command 0x%02X timed out!\n",
			card->devname, cmd);
		break;

	case S514_BOTH_PORTS_SAME_CLK_MODE:
		if(cmd == SET_CHDLC_CONFIGURATION) {
			printk(KERN_INFO
			 "%s: Configure both ports for the same clock source\n",
				card->devname);
			break;
		}

	default:
		printk(KERN_INFO "%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}

