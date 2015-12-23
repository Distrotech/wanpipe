/******************************************************************************
 * sdla_56k.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				56K board configuration.
 *
 * Author: 	Nenad Corbic  <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 1995-2007 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Jul 22, 2001	Nenad Corbic	
 *				Initial version.
 *
 * Oct 01, 2001 Gideon Hack
 *				Modifications for interrupt usage.
 *
 * Mar 29, 2007 David Rokhvarg	<davidr@sangoma.com>
 *				1. Added support for AFT-56k card, at the same time,
 *				made sure S514-56k card is working too.
 *				2. When line is in 'disconnected' state, there is FE
 *				interrupt every 4 seconds. Do NOT try to disable
 *				this interrupt (in the 56k chip), and do NOT try to use
 *				a timer instead of this interrupt because it will NOT work.
 ******************************************************************************
 */

/*
 ******************************************************************************
			   INCLUDE FILES
 ******************************************************************************
*/
 

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe.h"	/* WANPIPE common user API definitions */
#include "wanproc.h"

/******************************************************************************
			  DEFINES AND MACROS
******************************************************************************/
# if !defined(AFT_FUNC_DEBUG)
#define AFT_FUNC_DEBUG()
#endif

#define WRITE_REG(reg,val) 						\
	fe->write_fe_reg(						\
		((sdla_t*)fe->card)->hw, 				\
		(int)(((sdla_t*)fe->card)->wandev.state==WAN_CONNECTED),\
		0, 							\
		(int)(reg),(int)(val))
#define READ_REG(reg)	   						\
	fe->read_fe_reg(						\
		((sdla_t*)fe->card)->hw, 				\
		(int)(((sdla_t*)fe->card)->wandev.state==WAN_CONNECTED),\
		0,							\
		(int)(reg))


/******************************************************************************
			STRUCTURES AND TYPEDEFS
******************************************************************************/


/******************************************************************************
			   GLOBAL VARIABLES
******************************************************************************/

/******************************************************************************
			  FUNCTION PROTOTYPES
******************************************************************************/

static int sdla_56k_global_config(void* pfe);
static int sdla_56k_global_unconfig(void* pfe);

static int sdla_56k_config(void* pfe);
static u_int32_t sdla_56k_alarm(sdla_fe_t *fe, int manual_read);
static int sdla_56k_udp(sdla_fe_t*, void*, unsigned char*);
static void display_Rx_code_condition(sdla_fe_t* fe);
static int sdla_56k_print_alarm(sdla_fe_t* fe, unsigned int);
static int sdla_56k_update_alarm_info(sdla_fe_t *fe, struct seq_file* m, int* stop_cnt);

static int sdla_56k_polling(sdla_fe_t* fe);
static int sdla_56k_unconfig(void* pfe);
static int sdla_56k_intr(sdla_fe_t *fe);
static int sdla_56k_check_intr(sdla_fe_t *fe);

/******************************************************************************
			  FUNCTION DEFINITIONS
******************************************************************************/

/******************************************************************************
 *				sdla_56k_get_fe_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static char* sdla_56k_get_fe_media_string(void)
{
	return ("S514_56K");
	/*return ("S514_AFT_56K"); ??*/
}

/******************************************************************************
 *				sdla_56k_get_fe_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static unsigned char sdla_56k_get_fe_media(sdla_fe_t *fe)
{
	AFT_FUNC_DEBUG();
	return fe->fe_cfg.media;
}

/******************************************************************************
* sdla_56k_get_fe_status()	
*
* Description	: Get current FE line state - is it Connected or Disconnected
*
* Arguments	: fe - pointer to Front End structure.	
*		  status - pointer to location where the FE line state will
*			be stored. 
*		  notused - ignored 
*
* Returns	: always zero.
*******************************************************************************/

static int sdla_56k_get_fe_status(sdla_fe_t *fe, unsigned char *status, int notused)
{
	AFT_FUNC_DEBUG();
	*status = fe->fe_status;
	return 0;
}

u_int32_t sdla_56k_alarm(sdla_fe_t *fe, int manual_read)
{
	unsigned short 	status = 0x00;
	sdla_t		*card = (sdla_t *)fe->card;
	
	AFT_FUNC_DEBUG();

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	/* set to manually read the front-end registers */
	if(manual_read) {
		fe->fe_param.k56_param.RR8_reg_56k = 0;
		fe->fe_param.k56_param.RR8_reg_56k = READ_REG(REG_INT_EN_STAT);
		fe->fe_param.k56_param.RRA_reg_56k = READ_REG(REG_DEV_STAT);
		fe->fe_param.k56_param.RRC_reg_56k = READ_REG(REG_RX_CODES);
		status |= fe->fe_param.k56_param.RRA_reg_56k;
		status = (status << 8) & 0xFF00;
		status |= fe->fe_param.k56_param.RRC_reg_56k;
	}
	
	DEBUG_56K("56K registers: 8-%02X,C-%02X,A-%02X\n", 
			fe->fe_param.k56_param.RR8_reg_56k,
			fe->fe_param.k56_param.RRC_reg_56k, 
			fe->fe_param.k56_param.RRA_reg_56k);

	/* discard an invalid Rx code interrupt */
	if((fe->fe_param.k56_param.RR8_reg_56k & BIT_INT_EN_STAT_RX_CODE) && 
		(!fe->fe_param.k56_param.RRC_reg_56k)) {
		return(status);
	}

	/* record an RLOS (Receive Loss of Signal) condition */
	if(fe->fe_param.k56_param.RRA_reg_56k & BIT_DEV_STAT_RLOS) {
		fe->fe_param.k56_param.RLOS_56k = 1;
	}

	/* display Rx code conditions if necessary */
	if(!((fe->fe_param.k56_param.RRC_reg_56k ^ fe->fe_param.k56_param.prev_RRC_reg_56k) & 
		fe->fe_param.k56_param.delta_RRC_reg_56k) ||
		(fe->fe_status == FE_CONNECTED)) {
			display_Rx_code_condition(fe);
	}
	
	/* record the changes that occurred in the Rx code conditions */
	fe->fe_param.k56_param.delta_RRC_reg_56k |= 
		(fe->fe_param.k56_param.RRC_reg_56k ^ fe->fe_param.k56_param.prev_RRC_reg_56k);

	
	if(manual_read || (fe->fe_param.k56_param.RR8_reg_56k & BIT_INT_EN_STAT_ACTIVE)) {

		/* if Insertion Loss is less than 44.4 dB, then we are connected */
		if ((fe->fe_param.k56_param.RRA_reg_56k & 0x0F) > BIT_DEV_STAT_IL_44_dB) {
			if((fe->fe_status == FE_DISCONNECTED) ||
			 (fe->fe_status == FE_UNITIALIZED)) {
				
				fe->fe_status = FE_CONNECTED;
				/* reset the Rx code condition changes */
				fe->fe_param.k56_param.delta_RRC_reg_56k = 0;
				
				if(fe->fe_param.k56_param.RLOS_56k == 1) {
					fe->fe_param.k56_param.RLOS_56k = 0;
					DEBUG_EVENT("%s: 56k Receive Signal Recovered\n", 
							fe->name);
				}
				DEBUG_EVENT("%s: 56k Connected\n", fe->name);

				if (card->wandev.te_link_state){
					card->wandev.te_link_state(card);
				}	
			}
		}else{
			if((fe->fe_status == FE_CONNECTED) || 
			 (fe->fe_status == FE_UNITIALIZED)) {
				
				fe->fe_status = FE_DISCONNECTED;
				/* reset the Rx code condition changes */
				fe->fe_param.k56_param.delta_RRC_reg_56k = 0;

				if(fe->fe_param.k56_param.RLOS_56k == 1) {
					DEBUG_EVENT("%s: 56k Receive Loss of Signal\n", 
							fe->name);
				}
				DEBUG_EVENT("%s: 56k Disconnected (loopback)\n", fe->name);
	
				if (card->wandev.te_link_state){
					card->wandev.te_link_state(card);
				}
			}
		}
	}
	
	fe->fe_param.k56_param.prev_RRC_reg_56k = 
				fe->fe_param.k56_param.RRC_reg_56k;
	fe->fe_alarm = status;
	return(status);
}


int sdla_56k_default_cfg(void* pcard, void* p56k_cfg)
{
	AFT_FUNC_DEBUG();
	return 0;
}

int sdla_56k_iface_init(void *p_fe, void* pfe_iface)
{
	sdla_fe_t	*fe = (sdla_fe_t*)p_fe;
	sdla_fe_iface_t	*fe_iface = (sdla_fe_iface_t*)pfe_iface;

	AFT_FUNC_DEBUG();

	fe_iface->global_config		= &sdla_56k_global_config;
	fe_iface->global_unconfig	= &sdla_56k_global_unconfig;

	fe_iface->config		= &sdla_56k_config;
	fe_iface->unconfig		= &sdla_56k_unconfig;
	fe_iface->polling		= &sdla_56k_polling;

	fe_iface->get_fe_status		= &sdla_56k_get_fe_status;
	fe_iface->get_fe_media		= &sdla_56k_get_fe_media;
	fe_iface->get_fe_media_string	= &sdla_56k_get_fe_media_string;
	fe_iface->print_fe_alarm	= &sdla_56k_print_alarm;
	fe_iface->read_alarm		= &sdla_56k_alarm;
	fe_iface->update_alarm_info	= &sdla_56k_update_alarm_info;
	fe_iface->process_udp		= &sdla_56k_udp;

	fe_iface->isr			= &sdla_56k_intr;
	fe_iface->check_isr		= &sdla_56k_check_intr;

	/* The 56k CSU/DSU front end status has not been initialized  */
	fe->fe_status = FE_UNITIALIZED;
	
	return 0;
}

/* called from TASK */
static int sdla_56k_intr(sdla_fe_t *fe) 
{
	AFT_FUNC_DEBUG();

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	sdla_56k_alarm(fe, 1);

	return 0;
}



/*	Called from ISR. On AFT card there is only on 56 port, it 
	means the interrupt is always ours.
	Returns: 1
 */
static int sdla_56k_check_intr(sdla_fe_t *fe) 
{
	AFT_FUNC_DEBUG();

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	return 1;
}

static int sdla_56k_global_config(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	DEBUG_EVENT("%s: %s Global Front End configuration\n", 
			fe->name, FE_MEDIA_DECODE(fe));
	return 0;
}

static int sdla_56k_global_unconfig(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	DEBUG_EVENT("%s: %s Global unconfiguration!\n",
				fe->name,
				FE_MEDIA_DECODE(fe));
	return 0;
}


static int sdla_56k_config(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	sdla_t		*card = (sdla_t *)fe->card;

	AFT_FUNC_DEBUG();

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	/* The 56k CSU/DSU front end status has not been initialized  */
	fe->fe_status = FE_UNITIALIZED;

	/* Zero the previously read RRC register value */
	fe->fe_param.k56_param.prev_RRC_reg_56k = 0;
	
	/* Zero the RRC register changes */ 
	fe->fe_param.k56_param.delta_RRC_reg_56k = 0;
	
	if(WRITE_REG(REG_INT_EN_STAT, (BIT_INT_EN_STAT_IDEL | 
		BIT_INT_EN_STAT_RX_CODE | BIT_INT_EN_STAT_ACTIVE))) {
		return 1;
	} 

	if(WRITE_REG(REG_RX_CODES, 0xFF & ~(BIT_RX_CODES_ZSC))) {
		return 1;
	}

	if (card->wandev.clocking == WANOPT_EXTERNAL){
		DEBUG_EVENT("%s: Configuring 56K CSU/DSU for External Clocking\n",
				card->devname);
		if(WRITE_REG(REG_DEV_CTRL, 
			(BIT_DEV_CTRL_SCT_E_OUT | BIT_DEV_CTRL_DDS_PRI))) {
			return 1;
		}
	}else{
		DEBUG_EVENT("%s: Configuring 56K CSU/DSU for Internal Clocking\n",
				fe->name);
		if(WRITE_REG(REG_DEV_CTRL, (BIT_DEV_CTRL_XTALI_INT | 
			BIT_DEV_CTRL_SCT_E_OUT | BIT_DEV_CTRL_DDS_PRI))) {
			return 1;
		}
 	}

	if(WRITE_REG(REG_TX_CTRL, 0x00)) {
		return 1;
	}

	if(WRITE_REG(REG_RX_CTRL, 0x00)) {
		return 1;
	}

	if(WRITE_REG(REG_EIA_SEL, 0x00)) {
		return 1;
	}

	if(WRITE_REG(REG_EIA_TX_DATA, 0x00)) {
		return 1;
	}

	if(WRITE_REG(REG_EIA_CTRL, (BIT_EIA_CTRL_RTS_ACTIVE | 
		BIT_EIA_CTRL_DTR_ACTIVE | BIT_EIA_CTRL_DTE_ENABLE))) {
		return 1; 
	}

	fe->fe_status = FE_CONNECTED;
	return 0;
}

static int sdla_56k_unconfig(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	AFT_FUNC_DEBUG();

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	fe->fe_status = FE_UNITIALIZED;
	return 0;
}

static int sdla_56k_polling(sdla_fe_t* fe)
{
	
	sdla_56k_alarm(fe, 1);

	return 0;
}



static void display_Rx_code_condition(sdla_fe_t* fe)
{
	sdla_56k_param_t	*k56_param = &fe->fe_param.k56_param;

	AFT_FUNC_DEBUG();

	/* check for a DLP (DSU Non-latching loopback) code condition */
	if((k56_param->RRC_reg_56k ^ k56_param->prev_RRC_reg_56k) &
		BIT_RX_CODES_DLP) {
		if(k56_param->RRC_reg_56k & BIT_RX_CODES_DLP) {
			DEBUG_EVENT("%s: 56k receiving DSU Loopback code\n", 
					fe->name);
		} else {
			DEBUG_EVENT("%s: 56k DSU Loopback code condition ceased\n", 
					fe->name);
		}
	}

	/* check for an OOF (Out of Frame) code condition */
	if((k56_param->RRC_reg_56k ^ k56_param->prev_RRC_reg_56k) &
		BIT_RX_CODES_OOF) {
		if(k56_param->RRC_reg_56k & BIT_RX_CODES_OOF) {
			DEBUG_EVENT("%s: 56k receiving Out of Frame code\n", 
					fe->name);
		} else {
			DEBUG_EVENT("%s: 56k Out of Frame code condition ceased\n", 
					fe->name);
		}
	}

	/* check for an OOS (Out of Service) code condition */
	if((k56_param->RRC_reg_56k ^ k56_param->prev_RRC_reg_56k) &
		BIT_RX_CODES_OOS) {
		if(k56_param->RRC_reg_56k & BIT_RX_CODES_OOS) {
			DEBUG_EVENT("%s: 56k receiving Out of Service code\n", 
					fe->name);
		} else {
			DEBUG_EVENT("%s: 56k Out of Service code condition ceased\n", 
					fe->name);
		}
	}

	/* check for a CMI (Control Mode Idle) condition */
	if((k56_param->RRC_reg_56k ^ k56_param->prev_RRC_reg_56k) &
		BIT_RX_CODES_CMI) {
		if(k56_param->RRC_reg_56k & BIT_RX_CODES_CMI) {
			DEBUG_EVENT("%s: 56k receiving Control Mode Idle\n",
					fe->name);
		} else {
			DEBUG_EVENT("%s: 56k Control Mode Idle condition ceased\n", 
					fe->name);
		}
	}

	/* check for a ZSC (Zero Suppression Code) condition */
	if((k56_param->RRC_reg_56k ^ k56_param->prev_RRC_reg_56k) &
		BIT_RX_CODES_ZSC) {
		if(k56_param->RRC_reg_56k & BIT_RX_CODES_ZSC) {
			DEBUG_EVENT("%s: 56k receiving Zero Suppression code\n", 
					fe->name);
		} else {
			DEBUG_EVENT("%s: 56k Zero Suppression code condition ceased\n", 
					fe->name);
		}
	}

	/* check for a DMI (Data Mode Idle) condition */
	if((k56_param->RRC_reg_56k ^ k56_param->prev_RRC_reg_56k) &
		BIT_RX_CODES_DMI) {
		if(k56_param->RRC_reg_56k & BIT_RX_CODES_DMI) {
			DEBUG_EVENT("%s: 56k receiving Data Mode Idle\n",
				fe->name);
		} else {
			DEBUG_EVENT("%s: 56k Data Mode Idle condition ceased\n", 
				fe->name);
		}
	}
}

/*
 ******************************************************************************
 *				sdla_te_set_lbmode()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int 
sdla_56k_set_lbmode(sdla_fe_t *fe, unsigned char mode, unsigned char enable) 
{
	
	//unsigned char loop=BIT_RX_CTRL_DSU_LOOP|BIT_RX_CTRL_CSU_LOOP;
	//unsigned char loop=BIT_RX_CTRL_DSU_LOOP;
	//unsigned char loop=BIT_RX_CTRL_CSU_LOOP;
	unsigned char loop=0x00;

	if(mode==WAN_TE1_PAYLB_MODE){
                loop=BIT_RX_CTRL_DSU_LOOP;
        }else{
                loop=BIT_RX_CTRL_CSU_LOOP;
        }

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	
	if (enable == WAN_TE1_LB_ENABLE){
		WRITE_REG(REG_RX_CTRL, READ_REG(REG_RX_CTRL) | loop);
			
		DEBUG_EVENT("%s: %s Diagnostic Digital Loopback mode activated (0x%X).\n",
			fe->name,
			FE_MEDIA_DECODE(fe),
			READ_REG(REG_RX_CTRL));
	}else{
		
		WRITE_REG(REG_RX_CTRL, READ_REG(REG_RX_CTRL) & ~(loop));
		DEBUG_EVENT("%s: %s Diagnostic Digital Loopback mode deactivated (0x%X).\n",
			fe->name,
			FE_MEDIA_DECODE(fe),
			READ_REG(REG_RX_CTRL));
	}

	return 0;

}

/******************************************************************************
*				sdla_56k_udp_lb()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_56k_udp_lb(sdla_fe_t *fe, unsigned char* data)
{
	sdla_fe_lbmode_t	*lb = (sdla_fe_lbmode_t*)data;

	if (lb->cmd != WAN_FE_LBMODE_CMD_SET){
		return -EINVAL;
	}
	return sdla_56k_set_lbmode(fe, lb->type, lb->mode);
}


/*
 ******************************************************************************
 *				sdla_56k_udp()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_56k_udp(sdla_fe_t *fe, void* pudp_cmd, unsigned char* data)
{
	wan_cmd_t	*udp_cmd = (wan_cmd_t*)pudp_cmd;
	wan_femedia_t	*fe_media;
	int err;

	AFT_FUNC_DEBUG();

	switch(udp_cmd->wan_cmd_command){
	case WAN_GET_MEDIA_TYPE:
		fe_media = (wan_femedia_t*)data;
		memset(fe_media, 0, sizeof(wan_femedia_t));
		fe_media->media		= WAN_MEDIA_56K;
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		udp_cmd->wan_cmd_data_len = sizeof(wan_femedia_t); 
		break;

	case WAN_FE_GET_STAT:
		/* 56K Update CSU/DSU alarms */
		fe->fe_alarm = sdla_56k_alarm(fe, 1); 
		memcpy(&data[0], &fe->fe_stats, sizeof(sdla_fe_stats_t));
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
    	udp_cmd->wan_cmd_data_len = sizeof(sdla_fe_stats_t);
		break;

	case WAN_FE_LB_MODE:
		/* Activate/Deactivate Line Loopback modes */
		err = sdla_56k_udp_lb(fe, data);
	    	udp_cmd->wan_cmd_return_code = 
				(!err) ? WAN_CMD_OK : WAN_UDP_FAILED_CMD;
	    	udp_cmd->wan_cmd_data_len = 0x00;
		break;
		
 	case WAN_FE_FLUSH_PMON:
	case WAN_FE_GET_CFG:
	default:
		udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
    	udp_cmd->wan_cmd_data_len = 0;
		break;
	}
	return 0;
}

/*
*******************************************************************************
**				sdla_56k_alaram_print()	
**
** Description: 
** Arguments:
** Returns:
*/
static int sdla_56k_print_alarm(sdla_fe_t* fe, u_int32_t status)
{
	u_int32_t alarms = status;

	AFT_FUNC_DEBUG();

	if (!alarms){
		alarms = sdla_56k_alarm(fe, 0);
	}
	
	DEBUG_EVENT("%s: 56K Alarms:\n", fe->name);
 	DEBUG_EVENT("%s: In Service:            %s Data mode idle:    %s\n",
				fe->name,
			 	INS_ALARM_56K(alarms), 
			 	DMI_ALARM_56K(alarms));
	
 	DEBUG_EVENT("%s: Zero supp. code:       %s Ctrl mode idle:    %s\n",
				fe->name,
			 	ZCS_ALARM_56K(alarms), 
			 	CMI_ALARM_56K(alarms));

 	DEBUG_EVENT("%s: Out of service code:   %s Out of frame code: %s\n",
				fe->name,
			 	OOS_ALARM_56K(alarms), 
			 	OOF_ALARM_56K(alarms));
		
 	DEBUG_EVENT("%s: Valid DSU NL loopback: %s Unsigned mux code: %s\n",
				fe->name,
			 	DLP_ALARM_56K(alarms), 
			 	UMC_ALARM_56K(alarms));

 	DEBUG_EVENT("%s: Rx loss of signal:     %s \n",
				fe->name,
			 	RLOS_ALARM_56K(alarms)); 
	return 0;
}

static int sdla_56k_update_alarm_info(sdla_fe_t* fe, struct seq_file* m, int* stop_cnt)
{
	AFT_FUNC_DEBUG();

#if !defined(__WINDOWS__)
	PROC_ADD_LINE(m,
		"\n====================== Rx 56K CSU/DSU Alarms ==============================\n");
	PROC_ADD_LINE(m,
		PROC_STATS_ALARM_FORMAT,
		"In service", INS_ALARM_56K(fe->fe_alarm), 
		"Data mode idle", DMI_ALARM_56K(fe->fe_alarm));
		
	PROC_ADD_LINE(m,
		PROC_STATS_ALARM_FORMAT,
		"Zero supp. code", ZCS_ALARM_56K(fe->fe_alarm), 
		"Ctrl mode idle", CMI_ALARM_56K(fe->fe_alarm));

	PROC_ADD_LINE(m,
		PROC_STATS_ALARM_FORMAT,
		"Out of service code", OOS_ALARM_56K(fe->fe_alarm), 
		"Out of frame code", OOF_ALARM_56K(fe->fe_alarm));
		
	PROC_ADD_LINE(m,
		PROC_STATS_ALARM_FORMAT,
		"Valid DSU NL loopback", DLP_ALARM_56K(fe->fe_alarm), 
		"Unsigned mux code", UMC_ALARM_56K(fe->fe_alarm));

	PROC_ADD_LINE(m,
		PROC_STATS_ALARM_FORMAT,
		"Rx loss of signal", RLOS_ALARM_56K(fe->fe_alarm), 
		"N/A", "N/A");
#endif
	return m->count;	
}


