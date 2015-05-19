/*
 * Copyright (c) 2006
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
 *	$Id: sdla_8te1.c,v 1.121 2008-04-25 16:23:20 sangoma Exp $
 */

/******************************************************************************
** sdla_8te1.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
**				8 ports T1/E1 board configuration.
**
** Author: 	Alex Feldman <al.feldman@sangoma.com>
**
** ============================================================================
** Date			Name			Label		Description
** ============================================================================
**
** Aug 18, 2011 David Rokhvarg			Enable transmit pulse density for
**										T1 AMI.
** 
** Feb 01, 2010 David Rokhvarg	PATTERN_LEN
**										Fixed division-by-zero error in BERT
**										test, when test pattern was *not*
**										initialized by user-mode application.  
** 
** Feb 06, 2008 Alex Feldman	E1_120	Adjust waveform for E1, 120 ohm.
**
** Nov 23, 2007 Alex Feldman	TXTRI	Add support for TX Tri-state.
**
** Nov 23, 2007	Alex Feldman	UNFRM	Add support E1 Unframe mode for E1
**										interface. 
**
** 07-10-07	Alex Feldman		EBIT	Enable auto E-bit support.
**
** 02-18-06	Alex Feldman		Initial version.

******************************************************************************/

/******************************************************************************
*			   INCLUDE FILES
******************************************************************************/

# include "wanpipe_includes.h"
# include "wanpipe_defines.h"
# include "wanpipe_debug.h"
# include "wanproc.h"

# if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
#  include "wanpipe_snmp.h"
# endif

# include "sdla_te1_ds.h"
# include "wanpipe.h"	/* WANPIPE common user API definitions */


/******************************************************************************
*			  DEFINES AND MACROS
******************************************************************************/
#define WAN_TE1_DEVICE_ID	DEVICE_ID_DS(READ_REG_LINE(0, REG_IDR))

#define CLEAR_REG(sreg,ereg) {				\
	unsigned short reg;				\
	for(reg = sreg; reg < ereg; reg++){		\
		WRITE_REG(reg, 0x00);			\
	}						\
	}

#define IS_GLREG(reg)	((reg) >= 0xF0 && (reg) <= 0xFF)
#define IS_FRREG(reg)	((reg) <= 0x1F0)
#define IS_LIUREG(reg)	((reg) >= 0x1000 && (reg) <= 0x101F)
#define IS_BERTREG(reg)	((reg) >= 0x1100 && (reg) <= 0x110F)

#define DLS_PORT_DELTA(reg)			\
		IS_GLREG(reg)	? 0x000 :	\
		IS_FRREG(reg)	? 0x200 :	\
		IS_LIUREG(reg)	? 0x020 :	\
		IS_BERTREG(reg)	? 0x010 : 0x001

#define IS_DS16P(val) (val==DEVICE_ID_DS26519)

/* Read/Write to front-end register */
#define WRITE_REG(reg,val)						\
	fe->write_fe_reg(						\
		((sdla_t*)fe->card)->hw,			\
		(int)(((sdla_t*)fe->card)->wandev.state==WAN_CONNECTED),		\
		(int)fe->fe_cfg.line_no,					\
		(int)sdla_ds_te1_address(fe,fe->fe_cfg.line_no,(reg)),	\
		(int)(val))

#define WRITE_REG_LINE(fe_line_no, reg,val)				\
	fe->write_fe_reg(						\
		((sdla_t*)fe->card)->hw,			\
		(int)(((sdla_t*)fe->card)->wandev.state==WAN_CONNECTED),		\
		(int)fe_line_no,						\
		(int)sdla_ds_te1_address(fe,fe_line_no,(reg)),		\
		(int)(val))
	
#define READ_REG(reg)							\
	fe->read_fe_reg(						\
		((sdla_t*)fe->card)->hw,			\
		(int)(((sdla_t*)fe->card)->wandev.state==WAN_CONNECTED),		\
		(int)fe->fe_cfg.line_no,					\
		(int)sdla_ds_te1_address(fe,fe->fe_cfg.line_no,(reg)))
	
#define __READ_REG(reg)							\
	fe->__read_fe_reg(						\
		((sdla_t*)fe->card)->hw,			\
		(int)(((sdla_t*)fe->card)->wandev.state==WAN_CONNECTED),		\
		(int)fe->fe_cfg.line_no,					\
		(int)sdla_ds_te1_address(fe,fe->fe_cfg.line_no,(reg)))
	
#define READ_REG_LINE(fe_line_no, reg)					\
	fe->read_fe_reg(						\
		((sdla_t*)fe->card)->hw,			\
		(int)(((sdla_t*)fe->card)->wandev.state==WAN_CONNECTED),		\
		(int)fe_line_no,						\
		(int)sdla_ds_te1_address(fe,fe_line_no,(reg)))

#define WAN_T1_FRAMED_ALARMS		(WAN_TE_BIT_ALARM_RED |	WAN_TE_BIT_ALARM_LOF)
#define WAN_E1_FRAMED_ALARMS		(WAN_TE_BIT_ALARM_RED |	WAN_TE_BIT_ALARM_LOF) // | WAN_TE_BIT_ALARM_LIU_LOS)
/*Nov 23, 2007 UNFRM */ 
#define WAN_TE1_UNFRAMED_ALARMS		(WAN_TE_BIT_ALARM_RED | 		\
					 WAN_TE_BIT_ALARM_LOS)

#define IS_T1_ALARM(alarm)			\
		(alarm & 			\
			(			\
			 WAN_TE_BIT_ALARM_RED |	\
			 WAN_TE_BIT_ALARM_AIS |	\
			 WAN_TE_BIT_ALARM_LOF |	\
			 WAN_TE_BIT_ALARM_LOS |	\
			 WAN_TE_BIT_ALARM_ALOS	\
			 )) 

#define IS_E1_ALARM(alarm)			\
		(alarm &	 		\
			(			\
			 WAN_TE_BIT_ALARM_RED | \
			 WAN_TE_BIT_ALARM_AIS | \
			 WAN_TE_BIT_ALARM_OOF | \
			 WAN_TE_BIT_ALARM_LOS | \
			 WAN_TE_BIT_ALARM_ALOS 	\
			 ))



#define WAN_DS_REGBITMAP(fe)	(((fe)->fe_chip_id==DEVICE_ID_DS26521)?0:WAN_FE_LINENO((fe)))


extern int a104_set_digital_fe_clock(sdla_t * card);

typedef struct {
    unsigned int reg;
    unsigned int value;
} wan_reg_indx_t;


wan_reg_indx_t ncrc4_regs[] = {
        {0x40,0x00},{0x41,0xFF},{0x42,0xFF},{0x43,0xFF},{0x44,0xFF},{0x45,0xFF},
        {0x46,0xFF},{0x47,0xFF},{0x48,0xFF},{0x49,0xFF},{0x4a,0xFF},{0x4b,0xFF},
        {0x4c,0xFF},{0x4d,0xFF},{0x4e,0xFF},{0x4f,0xFF},{0x56,0x03},{0x57,0xE8},
        {0x5e,0x01},{0x5f,0x38},{0x60,0x5F},{0x64,0x1B},{0x65,0x5F},{0x66,0x00},
        {0x67,0x00},{0x77,0x1B},{0x78,0x00},{0x79,0x00},{0x81,0x44},{0x82,0xF8},
        {0x91,0x03},{0x92,0x22},{0x93,0x01},{0x96,0x02},{0xa0,0xFF},{0xa2,0x11},
        {0xa3,0x02},{0xb6,0x6C},{0x114,0xDF},{0x118,0xFF},{0x119,0xFF},{0x11a,0xFF},
        {0x11b,0xFF},{0x140,0x0B},{0x141,0x99},{0x142,0x99},{0x143,0x99},{0x144,0x99},
        {0x145,0x99},{0x146,0x99},{0x147,0x99},{0x148,0x99},{0x149,0x99},{0x14a,0x99},
        {0x14b,0x99},{0x14c,0x99},{0x14d,0x99},{0x14e,0x99},{0x14f,0x99},{0x164,0x9B},
        {0x165,0xC0},{0x166,0xFF},{0x167,0xFF},{0x169,0xFF},{0x16a,0xFF},{0x16b,0xFF},
        {0x16c,0xFF},{0x16d,0xFF},{0x181,0x04},{0x182,0xA0},{0x191,0x00},{0x1bb,0x9B},
        {0x1004,0x00},{0x1005,0x55},{0x1007,0x30},{0x100c,0x00},{0xFFFF,0xFFFF}
};




/******************************************************************************
*			  GLOBAL VERIABLES
******************************************************************************/
char *wan_t1_ds_rxlevel[] = {
	"> -2.5db",	
	"-2.5db to -5db",	
	"-5db to -7.5db",	
	"-7.5db to -10db",	
	"-10db to -12.5db",	
	"-12.5db to -15db",	
	"-15db to -17.5db",	
	"-17.5db to -20db",	
	"-20db to -23db",	
	"-23db to -26db",	
	"-26db to -29db",	
	"-29db to -32db",	
	"-32db to -36db",	
	"< -36db",	
	"",	
	""	
};

char *wan_e1_ds_rxlevel[] = {
	"> -2.5db",	
	"-2.5db to -5db",	
	"-5db to -7.5db",	
	"-7.5db to -10db",	
	"-10db to -12.5db",	
	"-12.5db to -15db",	
	"-15db to -17.5db",	
	"-17.5db to -20db",	
	"-20db to -23db",	
	"-23db to -26db",	
	"-26db to -29db",	
	"-29db to -32db",	
	"-32db to -36db",	
	"-36db to -40db",	
	"-40db to -44db",	
	"< -44db"
};

/******************************************************************************
*			  FUNCTION PROTOTYPES
******************************************************************************/
static int sdla_ds_te1_reset(void* pfe, int port_no, int reset);
static int sdla_ds_te1_global_config(void* pfe);	/* Change to static */
static int sdla_ds_te1_global_unconfig(void* pfe);	/* Change to static */
static int sdla_ds_te1_chip_config(void* pfe);
/*static int sdla_ds_te1_chip_config_verify(sdla_fe_t* pfe);*/
static int sdla_ds_te1_config(void* pfe);	/* Change to static */
static int sdla_ds_te1_force_config(void* pfe);	/* Change to static */
static int sdla_ds_te1_reconfig(sdla_fe_t* fe);
static int sdla_ds_te1_post_init(void *pfe);
static int sdla_ds_te1_unconfig(void* pfe);	/* Change to static */
static int sdla_ds_te1_force_unconfig(void* pfe);	/* Change to static */
static int sdla_ds_te1_post_unconfig(void* pfe);
static int sdla_ds_te1_TxChanCtrl(sdla_fe_t* fe, int channel, int enable);
static int sdla_ds_te1_RxChanCtrl(sdla_fe_t* fe, int channel, int enable);
static int sdla_ds_te1_disable_irq(void* pfe);	/* Change to static */
static int sdla_ds_te1_intr_ctrl(sdla_fe_t*, int, u_int8_t, u_int8_t, unsigned int); 
static int sdla_ds_te1_check_intr(sdla_fe_t *fe); 
static int sdla_ds_te1_intr(sdla_fe_t *fe); 
static int sdla_ds_te1_udp(sdla_fe_t *fe, void* p_udp_cmd, unsigned char* data);
static int sdla_ds_te1_flush_pmon(sdla_fe_t *fe);
static int sdla_ds_te1_pmon(sdla_fe_t *fe, int action);
static int sdla_ds_te1_rxlevel(sdla_fe_t* fe);
static int sdla_ds_te1_polling(sdla_fe_t* fe);
static int sdla_ds_te1_update_alarms(sdla_fe_t*, u_int32_t);
static u_int32_t sdla_ds_te1_read_alarms(sdla_fe_t *fe, int read);
static u_int32_t  sdla_ds_te1_read_tx_alarms(sdla_fe_t *fe, int action);
static int sdla_ds_te1_set_alarms(sdla_fe_t* fe, u_int32_t alarms);
static int sdla_ds_te1_clear_alarms(sdla_fe_t* fe, u_int32_t alarms);
static int sdla_ds_te1_set_status(sdla_fe_t* fe, u_int32_t alarms);
static int sdla_ds_te1_print_alarms(sdla_fe_t*, unsigned int);
static int sdla_ds_te1_set_lb(sdla_fe_t*, u_int8_t, u_int8_t, u_int32_t); 
static int sdla_ds_te1_rbs_init(sdla_fe_t* fe);
static int sdla_ds_te1_rbs_update(sdla_fe_t* fe, int, unsigned char);
static int sdla_ds_te1_set_rbsbits(sdla_fe_t *fe, int, unsigned char);
static int sdla_ds_te1_rbs_report(sdla_fe_t* fe);
static int sdla_ds_te1_check_rbsbits(sdla_fe_t* fe, int, unsigned int, int);
static unsigned char sdla_ds_te1_read_rbsbits(sdla_fe_t* fe, int, int);
static int sdla_ds_te1_add_event(sdla_fe_t*, sdla_fe_timer_event_t*);
static int sdla_ds_te1_add_timer(sdla_fe_t*, unsigned long);

//static void sdla_ds_te1_enable_timer(sdla_fe_t*, unsigned char, unsigned long);
static int sdla_ds_te1_sigctrl(sdla_fe_t *fe, int, unsigned long, int);

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void sdla_ds_te1_timer(void* pfe);
#elif defined(__WINDOWS__)
static void sdla_ds_te1_timer(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3);
#else
static void sdla_ds_te1_timer(unsigned long pfe);
#endif

static int sdla_ds_te1_swirq_trigger(sdla_fe_t* fe, int type, int subtype, int delay);
static int sdla_ds_te1_swirq_link(sdla_fe_t* fe);
static int sdla_ds_te1_swirq_alarm(sdla_fe_t* fe, int type);
static int sdla_ds_te1_swirq(sdla_fe_t* fe);

static int sdla_ds_te1_update_alarm_info(sdla_fe_t*, struct seq_file*, int*);
static int sdla_ds_te1_update_pmon_info(sdla_fe_t*, struct seq_file*, int*);

static int sdla_ds_te1_txlbcode_done(sdla_fe_t *fe);

static int sdla_ds_te1_boc(sdla_fe_t *fe, int mode);
static int sdla_ds_te1_liu_rlb(sdla_fe_t* fe, unsigned char cmd);
static int sdla_ds_te1_fr_plb(sdla_fe_t* fe, unsigned char cmd);
static int sdla_ds_te1_pclb(sdla_fe_t*, u_int8_t, u_int32_t);

static int sdla_ds_te1_bert_read_status(sdla_fe_t *fe);
static int sdla_ds_te1_bert_status(sdla_fe_t *fe, sdla_te_bert_stats_t *);

static int sdla_ds_e1_sa6code(sdla_fe_t* fe);
static int sdla_ds_e1_sabits(sdla_fe_t* fe);

/******************************************************************************
*			  FUNCTION DEFINITIONS
******************************************************************************/

/******************************************************************************
 *				sdla_te3_get_fe_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static char* sdla_ds_te1_get_fe_media_string(void)
{
	return ("AFT-A108 T1/E1");
}

/******************************************************************************
 *				sdla_ds_te1_get_fe_media()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static unsigned char sdla_ds_te1_get_fe_media(sdla_fe_t *fe)
{
	return fe->fe_cfg.media;
}

/******************************************************************************
* sdla_ds_te1_get_fe_status()	
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
static int sdla_ds_te1_get_fe_status(sdla_fe_t *fe, unsigned char *status,int notused)
{
	*status = fe->fe_status;
	return 0;
}

/******************************************************************************
 *				sdla_te1_ds_te1_address()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_address(sdla_fe_t *fe, int port_no, int reg)
{
	/* for T116 */
	int line_no = port_no;
	int port = port_no+1;

	if (fe->fe_chip_id == DEVICE_ID_DS26519){
		DEBUG_TEST("Port Number = %d\n",port_no);

		if (reg >= 0xF0 && reg <=0xFF){
		}
		else if (reg < 0x1F0){
			/* Framer registers */
			if (port > 8 ){
				reg = reg + 0x200 * (line_no - 8);
			}else{
				reg = reg + 0x200 * line_no;
			}
		}else if (reg >= 0x1000 && reg <= 0x101F){
			/* LIU registers */
			if (port > 8 ){
				reg = reg + 0x20 * (line_no - 8);
			}else{
				reg = reg + 0x20 * line_no;
			}
		}else if (reg >= 0x1100 && reg <= 0x110F){
			/* BERT registers */
			if (port > 8 ){
				reg = reg + 0x10 * (line_no - 8);
			}else{
				reg = reg + 0x10 * line_no;
			}
		}

		if (port > 8){
			reg =  reg + 0x2000;
		}

	}
	
	/* for a102, replace port number of second chip to 1 (1->0) */
	if (fe->fe_chip_id == DEVICE_ID_DS26521){
		port_no = 0;
	}

	if (fe->fe_chip_id == DEVICE_ID_DS26519){
		return reg;
	}else{
		return (int)((reg) + ((port_no)*(DLS_PORT_DELTA(reg))));
	}
}

/*
 ******************************************************************************
 *				sdla_ds_te1_TxChanCtrl()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_TxChanCtrl(sdla_fe_t* fe, int channel, int enable)	
{
	int		off = channel / 8;
	int		bit = channel % 8;
	unsigned char	value;

	value = READ_REG(REG_TGCCS1 + off);
	if (enable){
		value &= ~(1 << (bit-1));
	}else{
		value |= (1 << (bit-1));
	}
	WRITE_REG(REG_TGCCS1 + off, value);
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_RxChanCtrl()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_RxChanCtrl(sdla_fe_t* fe, int channel, int enable)
{
	int		off = channel / 8;
	int		bit = channel % 8;
	unsigned char	value;

	value = READ_REG(REG_RGCCS1 + off);
	if (enable){
		value &= ~(1 << (bit-1));
	}else{
		value |= (1 << (bit-1));
	}
	WRITE_REG(REG_RGCCS1 + off, value);
	return 0;
}


int sdla_ds_te1_iface_init(void *p_fe, void *p_fe_iface)
{
	sdla_fe_t	*fe = (sdla_fe_t*)p_fe;
	sdla_fe_iface_t	*fe_iface = (sdla_fe_iface_t*)p_fe_iface;

	fe_iface->reset				= &sdla_ds_te1_reset;
	fe_iface->global_config		= &sdla_ds_te1_global_config;
	fe_iface->global_unconfig	= &sdla_ds_te1_global_unconfig;
	fe_iface->chip_config		= &sdla_ds_te1_chip_config;
	fe_iface->config			= &sdla_ds_te1_config;
	fe_iface->force_config		= &sdla_ds_te1_force_config;
	fe_iface->post_init			= &sdla_ds_te1_post_init;
	fe_iface->reconfig			= &sdla_ds_te1_reconfig;
	fe_iface->unconfig			= &sdla_ds_te1_unconfig;
	fe_iface->force_unconfig	= &sdla_ds_te1_force_unconfig;
	fe_iface->post_unconfig		= &sdla_ds_te1_post_unconfig;
	fe_iface->disable_irq		= &sdla_ds_te1_disable_irq;
	fe_iface->isr				= &sdla_ds_te1_intr;
	fe_iface->check_isr			= &sdla_ds_te1_check_intr;
	fe_iface->intr_ctrl			= &sdla_ds_te1_intr_ctrl;
	fe_iface->polling			= &sdla_ds_te1_polling;
	fe_iface->process_udp		= &sdla_ds_te1_udp;

	fe_iface->print_fe_alarm	= &sdla_ds_te1_print_alarms;
	/*fe_iface->print_fe_act_channels	= &sdla_te_print_channels;*/
	fe_iface->read_alarm		= &sdla_ds_te1_read_alarms;
	fe_iface->read_tx_alarm		= &sdla_ds_te1_read_tx_alarms;
	//fe_iface->set_fe_alarm		= &sdla_ds_te1_set_alarms;
	fe_iface->read_pmon			= &sdla_ds_te1_pmon;
	fe_iface->flush_pmon		= &sdla_ds_te1_flush_pmon;
	fe_iface->get_fe_status		= &sdla_ds_te1_get_fe_status;
	fe_iface->get_fe_media		= &sdla_ds_te1_get_fe_media;
	fe_iface->get_fe_media_string	= &sdla_ds_te1_get_fe_media_string;
	fe_iface->update_alarm_info	= &sdla_ds_te1_update_alarm_info;
	fe_iface->update_pmon_info	= &sdla_ds_te1_update_pmon_info;
	fe_iface->set_fe_lbmode		= &sdla_ds_te1_set_lb;
	fe_iface->read_rbsbits		= &sdla_ds_te1_read_rbsbits;
	fe_iface->check_rbsbits		= &sdla_ds_te1_check_rbsbits;
	fe_iface->report_rbsbits	= &sdla_ds_te1_rbs_report;
	fe_iface->set_rbsbits		= &sdla_ds_te1_set_rbsbits;
	fe_iface->set_fe_sigctrl	= &sdla_ds_te1_sigctrl;

#if 0
	fe_iface->led_ctrl		= &sdla_te_led_ctrl;
#endif
	
	/* Initial FE state */
	fe->fe_status = FE_UNITIALIZED;	//FE_DISCONNECTED;
	if (IS_E1_FEMEDIA(fe)) {
		if (WAN_FE_FRAME(fe) == WAN_FR_UNFRAMED){
			fe->fe_alarm = WAN_TE1_UNFRAMED_ALARMS;
		} else {
			fe->fe_alarm = WAN_E1_FRAMED_ALARMS;	
		}
	} else {
		fe->fe_alarm = WAN_T1_FRAMED_ALARMS;
	}

	WAN_LIST_INIT(&fe->event);
	wan_spin_lock_irq_init(&fe->lockirq, "wan_8te1_lock");
	return 0;
}

/******************************************************************************
*				sdla_ds_te1_device_id()	
*
* Description: Verify device id
* Arguments:	
* Returns:	0 - device is supported, otherwise - device is not supported
******************************************************************************/
static int sdla_ds_te1_device_id(sdla_fe_t* fe)
{
//	u_int8_t	value;
	u_int32_t       value;

	/* Revision/Chip ID (Reg. 0x0D) */
//	value = READ_REG_LINE(0, REG_IDR);
//	fe->fe_chip_id = DEVICE_ID_DS(value);

#ifdef T116_FE_RESET
	if(fe->fe_chip_id == DEVICE_ID_DS26519){
		WRITE_REG_LINE(1, 0x44, 0x4000);
		READ_REG_LINE(1, REG_GLSRR);
		WRITE_REG_LINE(1, 0x44, 0x00054000);
		WP_DELAY(1000);
	}
#endif

	value = READ_REG_LINE(0, REG_IDR);

	/*CHECK FOF THE T116 ID*/
	if (value == DEVICE_ID_DS26519){
		fe->fe_chip_id = value;
	}else{
		fe->fe_chip_id = WAN_TE1_DEVICE_ID;
	}
	
#ifdef T116_FE_RESET
	if(fe->fe_chip_id == DEVICE_ID_DS26519){
		WRITE_REG_LINE(1, 0x44, 0x4000);
		READ_REG_LINE(1, REG_GLSRR);
		WRITE_REG_LINE(1, 0x44, 0x00054000);
		WP_DELAY(1000);
	}
#endif

	switch(fe->fe_chip_id){
	case DEVICE_ID_DS26519:
		fe->fe_max_ports = 16;
		break;
	case DEVICE_ID_DS26528:
		fe->fe_max_ports = 8;
		break;
	case DEVICE_ID_DS26524:
		fe->fe_max_ports = 4;
		break;
	case DEVICE_ID_DS26521:
		fe->fe_max_ports = 1;
		break;
	case DEVICE_ID_DS26522:
		fe->fe_max_ports = 2;
		break;
	default:
		DEBUG_ERROR("%s: ERROR: Unsupported DS %s CHIP (%02X:%02X)\n",
				fe->name, 
				FE_MEDIA_DECODE(fe),
				fe->fe_chip_id,
				READ_REG_LINE(0, REG_IDR));
		return -EINVAL;
	}
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_reset()	
 *
 * Description: Global configuration for Sangoma TE1 DS board.
 * 		Note: 	These register should be program only once for AFT-QUAD
 * 			cards.
 * Arguments:	fe	- front-end structure
 *		port_no	- 0 - global set/clear reset, 1-8 - set/clear reset per port
 *		reset	- 0 - clear reset, 1 - set reset
 * Returns:	WANTRUE - TE1 configred successfully, otherwise WAN_FALSE.
 ******************************************************************************
 */
static int sdla_ds_te1_reset(void* pfe, int port_no, int reset)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	u_int8_t	mask = 0x00, value, liu_status, fr_be_status;

	if (sdla_ds_te1_device_id(fe)) return -EINVAL;
	
	if (port_no){
		DEBUG_EVENT("%s: %s Front End Reset for port %d\n", 
					fe->name,
					(reset) ? "Set" : "Clear",
					port_no);
	}else{
		DEBUG_EVENT("%s: %s Global Front End Reset\n", 
				fe->name, (reset) ? "Set" : "Clear");
	}
	if (port_no){
		mask = (0x01 << WAN_DS_REGBITMAP(fe));
	}else{
		int	i = 0;
		
		mask = 0x00;
		for(i=0;i<fe->fe_max_ports;i++){
			mask |= (1<<i);
		}
	}
	/* Set reset first */
	liu_status = READ_REG(REG_GLSRR);
	liu_status |= mask;
	WRITE_REG(REG_GLSRR, liu_status);
	fr_be_status = READ_REG(REG_GFSRR);
	fr_be_status |= mask;
	WRITE_REG(REG_GFSRR, fr_be_status);
	if (fe->fe_chip_id == DEVICE_ID_DS26521 && !port_no){
		value = READ_REG_LINE(1, REG_GLSRR);
		value |= 0x01;
		WRITE_REG_LINE(1, REG_GLSRR, value);
		value = READ_REG_LINE(1, REG_GFSRR);
		value |= 0x01;
		WRITE_REG_LINE(1, REG_GFSRR, value);	
	}
	
	if (!reset){
		WP_DELAY(1000);
	
		/* Clear reset */
		liu_status &= ~mask;
		WRITE_REG(REG_GLSRR, liu_status);
		fr_be_status &= ~mask;
		WRITE_REG(REG_GFSRR, fr_be_status);	
		if (fe->fe_chip_id == DEVICE_ID_DS26521 && !port_no){
			value = READ_REG_LINE(1, REG_GLSRR);
			value &= ~0x01;
			WRITE_REG_LINE(1, REG_GLSRR, value);
			value = READ_REG_LINE(1, REG_GFSRR);
			value &= ~0x01;
			WRITE_REG_LINE(1, REG_GFSRR, value);	
		}
	}

#ifdef T116_FE_RESET
	if(fe->fe_chip_id == DEVICE_ID_DS26519){
		WRITE_REG_LINE(1, 0x44, 0x4000);
		READ_REG_LINE(1, REG_GLSRR);
		WRITE_REG_LINE(1, 0x44, 0x00054000);
		WP_DELAY(1000);
	}
#endif

	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_e1_set_sig_mode()	
 *
 * Description: Set E1 signalling mode for A101/A102/A104/A108 DallasMaxim board.
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_e1_set_sig_mode(sdla_fe_t *fe, int verbose)
{
	unsigned char	value = 0x00;

	if (WAN_TE1_SIG_MODE(fe) == WAN_TE1_SIG_CAS){
			
		/* CAS signalling mode */
		if (verbose){
			DEBUG_EVENT("%s: Enable E1 CAS Signalling mode!\n",
							fe->name);
		}
		value = READ_REG(REG_RCR1);
		WRITE_REG(REG_RCR1, value & ~BIT_RCR1_E1_RSIGM);
		//value = READ_REG(REG_RSIGC);
		//WRITE_REG(REG_RSIGC, value | BIT_RSIGC_CASMS);			
		value = READ_REG(REG_TCR1);
		WRITE_REG(REG_TCR1, value | BIT_TCR1_E1_T16S);
	
		if (WAN_FE_FRAME(fe) == WAN_FR_NCRC4){
			int i=0;
			while (1) {
				if (ncrc4_regs[i].reg == 0xffff) {
					break;
				}	
				WRITE_REG(ncrc4_regs[i].reg,ncrc4_regs[i].value);
				i++;	
			}
			DEBUG_EVENT("%s:    E1 CAS NCRC4 updated config %d\n",
							fe->name, i);
		}
	}else{
		
		/* CCS signalling mode */
		if (verbose){
			DEBUG_EVENT("%s: Enable E1 CCS Signalling mode!\n",
							fe->name);
		}
		WAN_TE1_SIG_MODE(fe) = WAN_TE1_SIG_CCS;
		value = READ_REG(REG_RCR1);
		WRITE_REG(REG_RCR1, value | BIT_RCR1_E1_RSIGM);

		value = READ_REG(REG_TCR1);
		WRITE_REG(REG_TCR1, value & ~BIT_TCR1_E1_T16S);
			
	}	
	return 0;
}
	
/*
 ******************************************************************************
 *				sdla_8te_global_config()	
 *
 * Description: Global configuration for Sangoma TE1 DS board.
 * 		Note: 	These register should be program only once for AFT-OCTAL
 * 			cards.
 * Arguments:	
 * Returns:	WANTRUE - TE1 configred successfully, otherwise WAN_FALSE.
 ******************************************************************************
 */
static int sdla_ds_te1_global_config(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	if (sdla_ds_te1_device_id(fe)) return -EINVAL;
	
	DEBUG_EVENT("%s: Global %s Front End configuration\n", 
				fe->name, FE_MEDIA_DECODE(fe));

	WRITE_REG_LINE(0, REG_GTCR1, 0x01);  
	WRITE_REG_LINE(0, REG_GTCCR, 0x00);

	WRITE_REG_LINE(0, REG_GLSRR, 0xFF);  
	WRITE_REG_LINE(0, REG_GFSRR, 0xFF);  
	WP_DELAY(1000);
	WRITE_REG_LINE(0, REG_GLSRR, 0x00);  
	WRITE_REG_LINE(0, REG_GFSRR, 0x00);  
	

	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_global_unconfig()	
 *
 * Description: Global configuration for Sangoma TE1 DS board.
 * 		Note: 	These register should be program only once for AFT-QUAD
 * 			cards.
 * Arguments:	
 * Returns:	WANTRUE - TE1 configred successfully, otherwise WAN_FALSE.
 ******************************************************************************
 */
static int sdla_ds_te1_global_unconfig(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	DEBUG_EVENT("%s: Global %s Front End unconfigation!\n",
				fe->name, FE_MEDIA_DECODE(fe));
	
	/* Inhibit the global interrupt */
	WRITE_REG_LINE(0, REG_GTCR1, 0x01);  
	WRITE_REG_LINE(0, REG_GFIMR, 0x00);
	WRITE_REG_LINE(0, REG_GLIMR, 0x00);  
	WRITE_REG_LINE(0, REG_GBIMR, 0x00);  
	WP_DELAY(1000);
	
	WRITE_REG_LINE(0, REG_GLSRR, 0xFF);  
	WRITE_REG_LINE(0, REG_GFSRR, 0xFF);  
		
	WP_DELAY(1000);
	return 0;
}

/******************************************************************************
**                              sdla_ds_t1_cfg_verify()
**
** Description: Verify T1 Front-End configuration
** Arguments:
** Returns:     0 - successfully, otherwise -EINVAL.
*******************************************************************************/
static int sdla_ds_t1_cfg_verify(void* pfe)
{
	sdla_fe_t       *fe = (sdla_fe_t*)pfe;

	/* Verify FE framing type */
	switch(WAN_FE_FRAME(fe)){
	case WAN_FR_D4: case WAN_FR_ESF: case WAN_FR_UNFRAMED:
		break;
	case WAN_FR_NONE:
		DEBUG_EVENT("%s: Defaulting T1 Frame      = ESF\n",
						fe->name);
		WAN_FE_FRAME(fe) = WAN_FR_ESF;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid %s FE Framing type (%X)\n",
						fe->name,
						FE_MEDIA_DECODE(fe),
						WAN_FE_FRAME(fe));
		return -EINVAL;
		break;
	}

	/* Verify FE line code type */
	switch(WAN_FE_LCODE(fe)){
	case WAN_LCODE_B8ZS: case WAN_LCODE_AMI:
		break;
	case WAN_LCODE_NONE:
		DEBUG_EVENT("%s: Defaulting T1 Line Code  = B8ZS\n",
						fe->name);
		WAN_FE_LCODE(fe) = WAN_LCODE_B8ZS;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid %s FE Line code type (%X)\n",
						fe->name,
						FE_MEDIA_DECODE(fe),
						WAN_FE_LCODE(fe));
		return -EINVAL;
		break;
	}

	/* Verify LBO */
	switch(WAN_TE1_LBO(fe)) {
	case WAN_T1_LBO_0_DB: case WAN_T1_LBO_75_DB:
	case WAN_T1_LBO_15_DB: case WAN_T1_LBO_225_DB:
	case WAN_T1_0_133: case WAN_T1_0_110: 
	case WAN_T1_133_266: case WAN_T1_110_220:
	case WAN_T1_266_399: case WAN_T1_220_330:
	case WAN_T1_399_533: case WAN_T1_330_440: case WAN_T1_440_550:
	case WAN_T1_533_655: case WAN_T1_550_660:
		break;
	case WAN_T1_LBO_NONE:
		DEBUG_EVENT("%s: Defaulting T1 LBO        = 0 db\n",
						fe->name);
		WAN_TE1_LBO(fe) = WAN_T1_LBO_0_DB;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid %s LBO value (%X)\n",
						fe->name,
						FE_MEDIA_DECODE(fe),
						WAN_TE1_LBO(fe));
		return -EINVAL;
		break;
	}

	if (WAN_TE1_HI_MODE(fe)){
		switch(fe->fe_cfg.cfg.te_cfg.rx_slevel){
		case WAN_TE1_RX_SLEVEL_30_DB: case WAN_TE1_RX_SLEVEL_225_DB:
		case WAN_TE1_RX_SLEVEL_175_DB: case WAN_TE1_RX_SLEVEL_12_DB:
			break;
		case  WAN_TE1_RX_SLEVEL_NONE:
			DEBUG_EVENT("%s: Defaulting T1 Rx Sens. Gain= 30 db\n",
						fe->name);
			fe->fe_cfg.cfg.te_cfg.rx_slevel = WAN_TE1_RX_SLEVEL_30_DB;
			break;
		default:
			DEBUG_EVENT(
			"%s: Error: Invalid High-Mode T1 Rx Sensitivity Gain (%d).\n", 
					fe->name,
					fe->fe_cfg.cfg.te_cfg.rx_slevel);
			return -EINVAL;
		}
	}else{
		switch(fe->fe_cfg.cfg.te_cfg.rx_slevel){
		case WAN_TE1_RX_SLEVEL_36_DB: case WAN_TE1_RX_SLEVEL_30_DB:
		case WAN_TE1_RX_SLEVEL_18_DB: case WAN_TE1_RX_SLEVEL_12_DB:
			break;
		case  WAN_TE1_RX_SLEVEL_NONE:
			DEBUG_EVENT("%s: Defaulting T1 Rx Sens. Gain= 36 db\n",
							fe->name);
			fe->fe_cfg.cfg.te_cfg.rx_slevel = WAN_TE1_RX_SLEVEL_36_DB;
			break;
		default:
			DEBUG_EVENT(
			"%s: Error: Invalid T1 Rx Sensitivity Gain (%d).\n", 
					fe->name,
					fe->fe_cfg.cfg.te_cfg.rx_slevel);
			return -EINVAL;
		}
	}

	return 0;
}

/******************************************************************************
**                              sdla_ds_e1_cfg_verify()
**
** Description: Verify E1 Front-End configuration
** Arguments:
** Returns:     0 - successfully, otherwise -EINVAL.
*******************************************************************************/
static int sdla_ds_e1_cfg_verify(void* pfe)
{
        sdla_fe_t       *fe = (sdla_fe_t*)pfe;

	/* Verify FE framing type */
	switch(WAN_FE_FRAME(fe)){
	case WAN_FR_NCRC4: case WAN_FR_CRC4: case WAN_FR_UNFRAMED:
		break;
	case WAN_FR_NONE:
		DEBUG_EVENT("%s: Defaulting E1 Frame      = CRC4\n",
					fe->name);
		WAN_FE_FRAME(fe) = WAN_FR_CRC4;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid %s FE Framing type (%X)\n",
					fe->name,
					FE_MEDIA_DECODE(fe),
					WAN_FE_FRAME(fe));
		return -EINVAL;
		break;
	}
	/* Verify FE line code type */
	switch(WAN_FE_LCODE(fe)){
	case WAN_LCODE_HDB3: case WAN_LCODE_AMI:
		break;
	case WAN_LCODE_NONE:
		DEBUG_EVENT("%s: Defaulting E1 Line Code  = HDB3\n",
					fe->name);
		WAN_FE_LCODE(fe) = WAN_LCODE_HDB3;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid %s FE Line code type (%X)\n",
					fe->name,
					FE_MEDIA_DECODE(fe),
					WAN_FE_LCODE(fe));
		return -EINVAL;
		break;
	}

	/* Verify LBO */
	switch(WAN_TE1_LBO(fe)) {
	case WAN_E1_120: case WAN_E1_75:
		break;
	case WAN_T1_LBO_NONE:
		DEBUG_EVENT("%s: Defaulting E1 LBO        = 120 OH\n",
					fe->name);
		WAN_TE1_LBO(fe) = WAN_E1_120;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid %s LBO value (%X)\n",
					fe->name,
					FE_MEDIA_DECODE(fe),
					WAN_TE1_LBO(fe));
		return -EINVAL;
		break;
	}

	switch(WAN_TE1_SIG_MODE(fe)){
	case WAN_TE1_SIG_CAS: case WAN_TE1_SIG_CCS:
		break;
	case WAN_TE1_SIG_NONE:
		DEBUG_EVENT("%s: Defaulting E1 Signalling = CCS\n",
					fe->name);
		WAN_TE1_SIG_MODE(fe) = WAN_TE1_SIG_CCS;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid E1 Signalling type (%X)\n",
					fe->name,
					WAN_TE1_SIG_MODE(fe));
		return -EINVAL;
		break;
	}

	if (WAN_TE1_HI_MODE(fe)){
		switch(fe->fe_cfg.cfg.te_cfg.rx_slevel){
		case WAN_TE1_RX_SLEVEL_30_DB: case WAN_TE1_RX_SLEVEL_225_DB:
		case WAN_TE1_RX_SLEVEL_175_DB: case WAN_TE1_RX_SLEVEL_12_DB:
			break;
		case  WAN_TE1_RX_SLEVEL_NONE:
			DEBUG_EVENT("%s: Defaulting E1 Rx Sens. Gain= 30 db\n",
							fe->name);
			fe->fe_cfg.cfg.te_cfg.rx_slevel = WAN_TE1_RX_SLEVEL_30_DB;
			break;
		default:
			DEBUG_EVENT(
			"%s: Error: Invalid T1 Rx Sensitivity Gain (%d).\n", 
					fe->name,
					fe->fe_cfg.cfg.te_cfg.rx_slevel);
			return -EINVAL;
		}
	}else{
		switch(fe->fe_cfg.cfg.te_cfg.rx_slevel){
		case WAN_TE1_RX_SLEVEL_43_DB: case WAN_TE1_RX_SLEVEL_30_DB:
		case WAN_TE1_RX_SLEVEL_18_DB: case WAN_TE1_RX_SLEVEL_12_DB:
			break;
		case  WAN_TE1_RX_SLEVEL_NONE:
			DEBUG_EVENT("%s: Defaulting E1 Rx Sens. Gain= 43 db\n",
							fe->name);
			fe->fe_cfg.cfg.te_cfg.rx_slevel = WAN_TE1_RX_SLEVEL_43_DB;
			break;
		default:
			DEBUG_EVENT(
			"%s: Error: Invalid T1 Rx Sensitivity Gain (%d).\n", 
					fe->name,
					fe->fe_cfg.cfg.te_cfg.rx_slevel);
			return -EINVAL;
		}

	}

	return 0;
}

void t116_errata_reset(sdla_fe_t *fe)
{
	int i;
	/* Note: Due to errata with the soft reset we have to manually clear
	 * all registers in the port framer and liu before reconfiguring */

	for(i = 0; i < 0xF0; i++) {
		WRITE_REG(0x00+i, 0);
	}

	for(i = 0; i < 0xF0; i++) {
		WRITE_REG(0x100+i, 0);
	}
	
	for(i = 0; i < 0x20; i++) {
		WRITE_REG(0x1000+i, 0);
	}

	/* Errata: now we need to write FF to all latched status registers */
	WRITE_REG(REG_RLS1, 0xFF);
	WRITE_REG(REG_RLS2, 0xFF);
	WRITE_REG(REG_RLS3, 0xFF);
	WRITE_REG(REG_RLS4, 0xFF);
	WRITE_REG(REG_RLS5, 0xFF);
	WRITE_REG(REG_RSS1, 0xFF);
	WRITE_REG(REG_RSS2, 0xFF);
	WRITE_REG(REG_RSS3, 0xFF);
	WRITE_REG(REG_RSS4, 0xFF);
	WRITE_REG(REG_TLS1, 0xFF);
	WRITE_REG(REG_TLS2, 0xFF);
	WRITE_REG(REG_TLS3, 0xFF);

}

/******************************************************************************
**				sdla_ds_te1_chip_config()	
**
** Description: Configure Dallas Front-End chip
** Arguments:	
** Returns:	0 - successfully, otherwise -EINVAL.
*******************************************************************************/
static int sdla_ds_te1_chip_config(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	unsigned char	value = 0x00;
	unsigned char	value_lrcr = 0x00;
	unsigned char	value_lrismr = 0x00;
	unsigned char	tmp_rsms0 = 0x00;
	unsigned char	tmp_rsms1 = 0x00;
	unsigned char	tmp_rmonen = 0x00;
	unsigned char	tmp_rtr = 0x00;
	unsigned char	tmp_rimpm0 = 0x00;
	unsigned char	tmp_rimpm1 = 0x00;
	unsigned char	tmp_rg703 = 0x00;
	u_int32_t       value_id;

	value_id = READ_REG_LINE(0, REG_IDR); 

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	/* Init Rx Framer registers */
	CLEAR_REG(0x0000, 0x00F0);
	/* Init Tx Framer registers */
	CLEAR_REG(0x0100, 0x01F0);
	/* Init LIU registers */
	CLEAR_REG(0x1000, 0x1020);
	/* Init BERT registers */
	CLEAR_REG(0x1100, 0x1110);

	/* T116 Errata - SOFT RST does not work */
	if (IS_DS16P(value_id)){
		/* Clear Rx Framer soft reset */
		WRITE_REG(REG_RMMR, 0x00);
		/* Clear Tx Framer soft reset */
		WRITE_REG(REG_TMMR, 0x00);

		t116_errata_reset(fe);

	} else {
		/* Set Rx Framer soft reset */
		WRITE_REG(REG_RMMR, BIT_RMMR_SFTRST);
		/* Set Tx Framer soft reset */
		WRITE_REG(REG_TMMR, BIT_RMMR_SFTRST);
	}

	if (IS_T1_FEMEDIA(fe)){
		/* Clear Rx Framer soft reset */
		WRITE_REG(REG_RMMR, 0x00);
		/* Clear Tx Framer soft reset */
		WRITE_REG(REG_TMMR, 0x00);
		/* Enable Rx Framer */
		WRITE_REG(REG_RMMR, BIT_RMMR_FRM_EN);
		/* Enable Tx Framer */
		WRITE_REG(REG_TMMR, BIT_TMMR_FRM_EN);
	}else{
		/* Clear Rx Framer soft reset */
		WRITE_REG(REG_RMMR, BIT_RMMR_T1E1);
		/* Clear Tx Framer soft reset */
		WRITE_REG(REG_TMMR, BIT_TMMR_T1E1);
		/* Enable Rx Framer */
		WRITE_REG(REG_RMMR, (BIT_RMMR_FRM_EN | BIT_RMMR_T1E1));
		/* Enable Tx Framer */
		WRITE_REG(REG_TMMR, (BIT_TMMR_FRM_EN | BIT_TMMR_T1E1));
	}


	if (IS_T1_FEMEDIA(fe)){
		/* Toggle from 0 to 1: resynchronization of rx framer
		   is initiated */
		WRITE_REG(REG_RCR1, BIT_RCR1_T1_SYNCT); 
	}
	switch(WAN_FE_FRAME(fe)){
	case WAN_FR_D4:

		/* Enable D4 on rx framer */
		value = READ_REG(REG_RCR1);
		WRITE_REG(REG_RCR1, value | BIT_RCR1_T1_RFM/* | BIT_RCR1_T1_SYNCC*/);

		/* Source FDL or Fs bits from internal TFDL register 	
		   or the SLC-96 data formatter */
		value = READ_REG(REG_TCR2);
		WRITE_REG(REG_TCR2, value & ~BIT_TCR2_T1_TFDLS);
	
		/* Enable D4 on tx framer */	
		value = READ_REG(REG_TCR3);
		WRITE_REG(REG_TCR3, value | BIT_TCR3_TFM);
	
		/* Transmit 0x1C in Tx FDL Register */	
		WRITE_REG(REG_T1TFDL, 0x1c);
		break;

	case WAN_FR_ESF:

		/* Enable ESF on rx framer */
		value = READ_REG(REG_RCR1);
		value |= BIT_RCR1_T1_SYNCC; /* Auto sync disabled */
		value &= ~BIT_RCR1_T1_RFM;  /* ESF mode */
		WRITE_REG(REG_RCR1, value);
	
		/* Enable ESF on tx framer */
		value = READ_REG(REG_TCR3);
		value &= ~BIT_TCR3_TFM;
		WRITE_REG(REG_TCR3, value);

		/* NC as per ncomm stack */
		value = READ_REG(REG_TCR2);
		WRITE_REG(REG_TCR2, (value | BIT_TCR2_T1_TFDLS));
		
		/* Transmit 0x7e in Tx FDL Register */	
		WRITE_REG(REG_T1TFDL, 0x7e);

		break;
		
	case WAN_FR_SLC96:
		value = READ_REG(REG_RCR1);
		value |= (BIT_RCR1_T1_RFM|BIT_RCR1_T1_SYNCC);
		value &= ~BIT_RCR1_T1_SYNCT;
		WRITE_REG(REG_RCR1, value);
		
		value = READ_REG(REG_T1RCR2);
		WRITE_REG(REG_T1RCR2, value | BIT_T1RCR2_RSLC96);
		
		value = READ_REG(REG_TCR1);
		value &= ~BIT_TCR1_T1_TFPT;
		WRITE_REG(REG_TCR1, value);
		
		value = READ_REG(REG_TCR2);
		value |= BIT_TCR2_T1_TSLC96;
		value &= ~BIT_TCR2_T1_TFDLS;
		WRITE_REG(REG_TCR2, value);
		
		value = READ_REG(REG_TCR3);
		WRITE_REG(REG_TCR3, value | BIT_TCR3_TFM);
		break;
	case WAN_FR_NCRC4:
		value = READ_REG(REG_RCR1);
		value &= ~BIT_RCR1_E1_RCRC4;
		value |= BIT_RCR1_E1_RSIGM;
		WRITE_REG(REG_RCR1, value);

		value = READ_REG(REG_TCR1);
		value &= ~BIT_TCR1_E1_TCRC4;
		WRITE_REG(REG_TCR1, value);

		value = READ_REG(REG_TCR2);
		value &= ~BIT_TCR2_E1_AEBE;
		WRITE_REG(REG_TCR2, value);

		WRITE_REG(REG_SSIE1+0, 0);	
		WRITE_REG(REG_SSIE1+1, 0);	
		WRITE_REG(REG_SSIE1+2, 0);	
		WRITE_REG(REG_SSIE1+3, 0);	
		break;
	case WAN_FR_CRC4:
		value = READ_REG(REG_RCR1);
		value |= BIT_RCR1_E1_RSIGM; /* CCS */
		WRITE_REG(REG_RCR1, value | BIT_RCR1_E1_RCRC4);

		value = READ_REG(REG_TCR1);
		WRITE_REG(REG_TCR1, value | BIT_TCR1_E1_TCRC4);
		/* EBIT: Enable auto E-bit support */
		value = READ_REG(REG_TCR2);
		WRITE_REG(REG_TCR2, value | BIT_TCR2_E1_AEBE);

		WRITE_REG(REG_SSIE1+0, 0);	
		WRITE_REG(REG_SSIE1+1, 0);	
		WRITE_REG(REG_SSIE1+2, 0);	
		WRITE_REG(REG_SSIE1+3, 0);	
		break;
	case WAN_FR_UNFRAMED:
		/* Nov 23, 2007 UNFRM */
		value = READ_REG(REG_TCR1);
		WRITE_REG(REG_TCR1, value | BIT_TCR1_E1_TTPT);
		value = READ_REG(REG_RCR1);
		WRITE_REG(REG_RCR1, value | BIT_RCR1_E1_SYNCE);
		break;
	default:
		DEBUG_EVENT("%s: Unsupported DS Frame mode (%X)\n",
				fe->name, WAN_FE_FRAME(fe));
		return -EINVAL;
	}
	
	if (IS_E1_FEMEDIA(fe)){
		sdla_ds_e1_set_sig_mode(fe, 0);
	}

	switch(WAN_FE_LCODE(fe)){
	case WAN_LCODE_B8ZS:
		value = READ_REG(REG_RCR1);
		WRITE_REG(REG_RCR1, value | BIT_RCR1_T1_RB8ZS);
		value = READ_REG(REG_TCR1);
		WRITE_REG(REG_TCR1, value | BIT_TCR1_T1_TB8ZS);

		/* NC as per ncomm */
		value = READ_REG(REG_TCR2);
		value &= ~BIT_TCR2_T1_TCR2_PDE; 
		WRITE_REG(REG_TCR2, value);
		
		break;

	case WAN_LCODE_HDB3:
		value = READ_REG(REG_RCR1);
		WRITE_REG(REG_RCR1, value | BIT_RCR1_E1_RHDB3);
		value = READ_REG(REG_TCR1);
		WRITE_REG(REG_TCR1, value | BIT_TCR1_E1_THDB3);
		break;

	case WAN_LCODE_AMI:
		if (IS_T1_FEMEDIA(fe)){
			value = READ_REG(REG_RCR1);
			WRITE_REG(REG_RCR1, value & ~BIT_RCR1_T1_RB8ZS);
			value = READ_REG(REG_TCR1);
			WRITE_REG(REG_TCR1, value & ~BIT_TCR1_T1_TB8ZS);

			/* (August 18, 2011) enable transmit pulse density 
			enforcer for AMI. See page 198 of DS26528.pdf. */
			value = READ_REG(REG_TCR2);
			WRITE_REG(REG_TCR2, value | BIT_TCR2_T1_TCR2_PDE);
		}else{
			value = READ_REG(REG_RCR1);
			WRITE_REG(REG_RCR1, value & ~BIT_RCR1_E1_RHDB3);
			value = READ_REG(REG_TCR1);
			WRITE_REG(REG_TCR1, value & ~BIT_TCR1_E1_THDB3);

			/* (August 18, 2011) When running B8ZS, 
			pulse density enforcer should be set to zero since
			B8ZS-encoded data streams cannot violate the pulse density
			requirements. See page 198 of DS26528.pdf. */
			value = READ_REG(REG_TCR2);
			WRITE_REG(REG_TCR2, value & ~BIT_TCR2_T1_TCR2_PDE);
		}
		break;

	default:
		DEBUG_EVENT("%s: Unsupported DS Line code mode (%X)\n",
				fe->name, WAN_FE_LCODE(fe));
		return -EINVAL;
	}

	/* RSYSCLK output */
	WRITE_REG(REG_RIOCR, BIT_RIOCR_RSCLKM);
	/* TSYSCLK input */
	if (IS_T1_FEMEDIA(fe)){
		WRITE_REG(REG_TIOCR, 0x00);
	}else{
		WRITE_REG(REG_TIOCR, BIT_TIOCR_TSCLKM);
	}
#if 0	
	if (WAN_TE1_CLK(fe) == WAN_MASTER_CLK){
		/* RSYNC as input */
		value = READ_REG(REG_RIOCR);
		value |= BIT_RIOCR_RSIO;
		WRITE_REG(REG_RIOCR, value);
		/* RESE enable */
		value = READ_REG(REG_RESCR);
		value |= BIT_RESCR_RESE;
		WRITE_REG(REG_RESCR, value);

		/* TSYNC as output */
		value = READ_REG(REG_TIOCR);
		value |= BIT_TIOCR_TSIO;
		WRITE_REG(REG_TIOCR, value);
	}
#endif

	if (IS_E1_FEMEDIA(fe)){
		//WRITE_REG(REG_E1TAF, 0x1B);
		//WRITE_REG(REG_E1TNAF, 0x40);
		
		WRITE_REG(REG_E1TAF, 0x1B);
		WRITE_REG(REG_E1TNAF, 0x5F);
		WRITE_REG(REG_E1TSa4, 0x00);
		WRITE_REG(REG_E1TSa5, 0x00);
		WRITE_REG(REG_E1TSa6, 0x00);
		WRITE_REG(REG_E1TSa7, 0x00);
		WRITE_REG(REG_E1TSa8, 0x00);
		WRITE_REG(REG_E1TSACR, 0x00);
		if (WAN_FE_FRAME(fe) == WAN_FR_CRC4){
			WRITE_REG(REG_E1TSa4, 0xFF);
			WRITE_REG(REG_E1TSa5, 0xFF);
			WRITE_REG(REG_E1TSa6, 0xFF);
			WRITE_REG(REG_E1TSa7, 0xFF);
			WRITE_REG(REG_E1TSa8, 0xFF);
			WRITE_REG(REG_E1TSACR, 0x1F);
		}
		
		/* Enable Rx Sabit detection */
		WRITE_REG(REG_E1RSAIMR, 0x00);
	}

// FIXME: BOC
	if (IS_T1_FEMEDIA(fe)){
		WRITE_REG(REG_T1RBOCC, BIT_T1RBOCC_RBD0 | BIT_T1RBOCC_RBF1 | BIT_T1RBOCC_RBF0);
    	WRITE_REG(REG_T1RIBCC, BIT_T1RIBCC_RUP2 | BIT_T1RIBCC_RDN1);
		/* Errata: read any reg before CDN1 */
		READ_REG(REG_RIIR);
    	WRITE_REG(REG_T1RUPCD1, 0x80);
    	WRITE_REG(REG_T1RDNCD1, 0x80);
	}
	
	/* T1/J1 or E1 */
	if (IS_T1_FEMEDIA(fe)){
		WRITE_REG(REG_LTRCR, BIT_LTRCR_T1J1E1S);
	}else{
		/* E1 | G.775 LOS */
		WRITE_REG(REG_LTRCR, 0x00);
	}

	value = 0x00;
	switch(WAN_TE1_LBO(fe)) {
	case WAN_T1_LBO_0_DB:
		value = 0x00;
		break;
	case WAN_T1_LBO_75_DB:
		value = BIT_LTITSR_L2 | BIT_LTITSR_L0;
		break;
	case WAN_T1_LBO_15_DB:
		value = BIT_LTITSR_L2 | BIT_LTITSR_L1;
		break;
	case WAN_T1_LBO_225_DB:
		value = BIT_LTITSR_L2 | BIT_LTITSR_L1 | BIT_LTITSR_L0;
		break;
	case WAN_T1_0_133: case WAN_T1_0_110:
		value = 0x00;
		break;
	case WAN_T1_133_266: case WAN_T1_110_220:
		value = BIT_LTITSR_L0;
		break;
	case WAN_T1_266_399: case WAN_T1_220_330:
		value = BIT_LTITSR_L1;
		break;
	case WAN_T1_399_533: case WAN_T1_330_440:
		value = BIT_LTITSR_L1 | BIT_LTITSR_L0;
		break;
	case WAN_T1_533_655: case WAN_T1_440_550: case WAN_T1_550_660: 
		value = BIT_LTITSR_L2;
		break;
	case WAN_E1_120:
		value = BIT_LTITSR_L0;
		break;
	case WAN_E1_75:
		value = 0x00;
		break;
	default:
		if (IS_E1_FEMEDIA(fe)){
			value = BIT_LTITSR_L0;
		}
		break;
	}
	if (IS_T1_FEMEDIA(fe)){
		WRITE_REG(REG_LTITSR, value | BIT_LTITSR_TIMPL0);
	}else if (IS_E1_FEMEDIA(fe)){
		if (WAN_TE1_LBO(fe) == WAN_E1_120){
			value |= (BIT_LTITSR_TIMPL1 | BIT_LTITSR_TIMPL0);
		}
		WRITE_REG(REG_LTITSR, value);
	}else if (IS_J1_FEMEDIA(fe)){
		WRITE_REG(REG_LTITSR,
			value | BIT_LTITSR_TIMPL0);
	}

	if (IS_DS16P(value_id)){
		tmp_rmonen = BIT_LRCR_TAP_RMONEN;
		tmp_rsms0 = BIT_LRCR_TAP_RSMS0;
		tmp_rsms1 = BIT_LRCR_TAP_RSMS1;
		tmp_rtr = BIT_LRCR_TAP_RTR;
		tmp_rimpm0 = BIT_LRISMR_TAP_RIMPM0;
		tmp_rimpm1 = BIT_LRISMR_TAP_RIMPM1;
		tmp_rg703 = BIT_LRCR_TAP_RG703;
	}else{
		tmp_rmonen = BIT_LRISMR_RMONEN;
		tmp_rsms0 = BIT_LRISMR_RSMS0;
		tmp_rsms1 = BIT_LRISMR_RSMS1;
		tmp_rtr = BIT_LRISMR_RTR;
		tmp_rimpm0 = BIT_LRISMR_RIMPM0;
		tmp_rimpm1 = BIT_LRISMR_RIMPM1;
		tmp_rg703 = BIT_LRISMR_RG703;
	}
	value = 0x00;
	value_lrcr=0;
	value_lrismr=0;

	if (WAN_TE1_HI_MODE(fe)){
		if (IS_DS16P(value_id)) {
			value_lrcr |= tmp_rmonen;	
		} else {
			value_lrismr |= tmp_rmonen;
		}
		switch(fe->fe_cfg.cfg.te_cfg.rx_slevel){
		case WAN_TE1_RX_SLEVEL_30_DB:
			break;
		case WAN_TE1_RX_SLEVEL_225_DB:
			if (IS_DS16P(value_id)) {
				value_lrcr |=tmp_rsms0;
			} else {
				value_lrismr |= tmp_rsms0;
			}
			break;
		case WAN_TE1_RX_SLEVEL_175_DB:
			if (IS_DS16P(value_id)) {
				value_lrcr |=tmp_rsms1;
			} else {
				value_lrismr |= tmp_rsms1;
			}
			break;
		case WAN_TE1_RX_SLEVEL_12_DB:
			if (IS_DS16P(value_id)) {
				value_lrcr |=(tmp_rsms1|tmp_rsms0);
			} else {
				value_lrismr |= (tmp_rsms1 | tmp_rsms0);
			}
			break;
		default:	/* set default value */ 
			fe->fe_cfg.cfg.te_cfg.rx_slevel = WAN_TE1_RX_SLEVEL_30_DB;
			break;
		}
		DEBUG_EVENT(
		"%s:    Rx Sensitivity Gain %s%s (High Impedence mode).\n", 
			fe->name, 
			WAN_TE1_RX_SLEVEL_DECODE(fe->fe_cfg.cfg.te_cfg.rx_slevel),
			(fe->fe_cfg.cfg.te_cfg.rx_slevel==WAN_TE1_RX_SLEVEL_30_DB)?
							" (default)":"");
	}else{
		switch(fe->fe_cfg.cfg.te_cfg.rx_slevel){
		case WAN_TE1_RX_SLEVEL_12_DB:
			break;
		case WAN_TE1_RX_SLEVEL_18_DB:
			if (IS_DS16P(value_id)) {
				value_lrcr |=tmp_rsms0;
			} else {
				value_lrismr |= tmp_rsms0;
			}
			break;
		case WAN_TE1_RX_SLEVEL_30_DB:
			if (IS_DS16P(value_id)) {
				value_lrcr |=tmp_rsms1;
			} else {
				value_lrismr |= tmp_rsms1;
			}
			break;
		case WAN_TE1_RX_SLEVEL_36_DB:
		case WAN_TE1_RX_SLEVEL_43_DB:
		default:	/* set default value */ 
			if (IS_DS16P(value_id)) {
				value_lrcr |=(tmp_rsms1|tmp_rsms0);
			} else {
				value_lrismr |= (tmp_rsms1 | tmp_rsms0);
			}
			break;
		}
		DEBUG_EVENT("%s:    Rx Sensitivity Gain %s%s.\n", 
			fe->name, 
			WAN_TE1_RX_SLEVEL_DECODE(fe->fe_cfg.cfg.te_cfg.rx_slevel),
			((IS_T1_FEMEDIA(fe) && (fe->fe_cfg.cfg.te_cfg.rx_slevel==WAN_TE1_RX_SLEVEL_36_DB)) ||
			 (IS_E1_FEMEDIA(fe) && (fe->fe_cfg.cfg.te_cfg.rx_slevel==WAN_TE1_RX_SLEVEL_43_DB))) ?
					 " (default)": "");
	}

	if (IS_T1_FEMEDIA(fe)){
		if (IS_DS16P(value_id)){
			/* Setting Impedance to 120H with External Resistor */
			value_lrismr |= BIT_LRISMR_TAP_RIMPM0;
			value_lrismr |= BIT_LRISMR_TAP_RIMPON;
		}else{
			value_lrismr |= BIT_LRISMR_RIMPM0;
		}
	}else{
		//value |= BIT_LRISMR_RIMPOFF;		
		if (IS_DS16P(value_id)){
			/* Setting Impedance to 120H with External Resistor */
			value_lrismr |= BIT_LRISMR_TAP_RIMPON;
			if (WAN_TE1_LBO(fe) == WAN_E1_120){
				value_lrismr |= BIT_LRISMR_TAP_RIMPM0;
				value_lrismr |= BIT_LRISMR_TAP_RIMPM1;
			}
		} else {
			if (WAN_TE1_LBO(fe) == WAN_E1_120){
				value_lrismr |= tmp_rimpm1 | tmp_rimpm0;
			}
		}
	}

	if (IS_DS16P(value_id)){
		WRITE_REG(REG_LRISMR_TAP, value_lrismr);
		WRITE_REG(REG_LRCR_TAP, value_lrcr);
		DEBUG_EVENT("%s: Configurig DS16 for %s LRISMR=0x%X LRCR=0x%X\n",fe->name,IS_T1_FEMEDIA(fe)?"T1":"E1",value_lrismr,value_lrcr);
	}else{
		WRITE_REG(REG_LRISMR, value_lrismr);
		DEBUG_EVENT("%s: Configurig DS8/4/2/1 for %s LRISMR=0x%X \n",fe->name,IS_T1_FEMEDIA(fe)?"T1":"E1",value_lrismr);
	}

	if (IS_E1_FEMEDIA(fe) && WAN_TE1_LBO(fe) == WAN_E1_120){
		/* Feb 7, 2008
		** Adjust DAC gain (-4.88%) */
		WRITE_REG(REG_LTXLAE, 0x09);
	}
		
	/* Additional front-end settings */
	value = READ_REG(REG_ERCNT);
	if (WAN_FE_LCODE(fe) == WAN_LCODE_AMI){
		value &= ~BIT_ERCNT_LCVCRF;
	}else{
		value |= BIT_ERCNT_LCVCRF;
	}
	value |= BIT_ERCNT_EAMS;		/* manual mode select */
	WRITE_REG(REG_ERCNT, value);

#if 1
	if (WAN_TE1_ACTIVE_CH(fe) != ENABLE_ALL_CHANNELS){
		unsigned long	active_ch = WAN_TE1_ACTIVE_CH(fe);
		int channel_range = (IS_T1_FEMEDIA(fe)) ? 
				NUM_OF_T1_CHANNELS : NUM_OF_E1_TIMESLOTS;
		unsigned char	rescr, tescr, gfcr;
		int i = 0;

		DEBUG_EVENT("%s: %s:%d: Disable channels: ", 
					fe->name,
					FE_MEDIA_DECODE(fe), WAN_FE_LINENO(fe)+1);
 		for(i = 1; i <= channel_range; i++){
			if (!(active_ch & (1 << (i-1)))){
				_DEBUG_EVENT("%d ", i);
				sdla_ds_te1_TxChanCtrl(fe, i, 0);
				sdla_ds_te1_RxChanCtrl(fe, i, 0);
			}
		}
		_DEBUG_EVENT("\n");
		gfcr = READ_REG(REG_GFCR);
		WRITE_REG(REG_GFCR, gfcr | BIT_GFCR_TCBCS | BIT_GFCR_RCBCS);
		rescr = READ_REG(REG_RESCR);
		WRITE_REG(REG_RESCR, rescr | BIT_RESCR_RGCLKEN);
		tescr = READ_REG(REG_TESCR);
		WRITE_REG(REG_TESCR, tescr | BIT_TESCR_TGPCKEN);
	}
#endif

	if (WAN_FE_FRAME(fe) != WAN_FR_UNFRAMED){
		/* Set INIT_DONE (for not unframed mode) */
		value = READ_REG(REG_RMMR);
		WRITE_REG(REG_RMMR, value | BIT_RMMR_INIT_DONE);
		value = READ_REG(REG_TMMR);
		WRITE_REG(REG_TMMR, value | BIT_TMMR_INIT_DONE);
	}

	/* Turn on LIU output */
	if (IS_FE_TXTRISTATE(fe)){
		DEBUG_EVENT("%s:    Disable TX (tri-state mode)\n",
						fe->name);
	}else{	
		if (IS_E1_FEMEDIA(fe)){
			WRITE_REG(REG_LMCR, BIT_LMCR_TE);
		}else{
			/* Sep 17, 2009 - Auto AIS transmition (experimental) */
			
			/* This feature forces AIS when the link goes down.
			   This breaks the specifications. When link goes down
			   we should send Yellow alarm */
			if (fe->fe_cfg.cfg.te_cfg.ais_auto_on_los) {
				WRITE_REG(REG_LMCR, BIT_LMCR_ATAIS | BIT_LMCR_TE);
			} else {
				WRITE_REG(REG_LMCR, BIT_LMCR_TE);
			}
		}


		/* If port started in maintenance mode then do not
		   remove AIS alarm until done manually */
		if (fe->fe_cfg.cfg.te_cfg.ais_maintenance == WANOPT_YES){
			fe->te_param.tx_ais_startup_timeout=0;
			fe->fe_stats.tx_maint_alarms |= WAN_TE_BIT_ALARM_AIS;
		
			/* Send out AIS alarm if port is started in Maintenance Mode */
			sdla_ds_te1_set_alarms(fe,WAN_TE_BIT_ALARM_AIS);
			
		} 
#if 0
		/* NC: Do not send out AIS on startup. Causes stuck RAI alarm
		   on some equipment */
		else {
			fe->te_param.tx_ais_startup_timeout=SYSTEM_TICKS;
		}
#endif

	}


	/* INIT RBS bits to 1 */
	sdla_ds_te1_rbs_init(fe);


	return 0;
}

#if 0
/*
 ******************************************************************************
 *				sdla_ds_te1_chip_config_verify()	
 *
 * Description: Configure Sangoma 8 ports T1/E1 board
 * Arguments:	
 * Returns:	WANTRUE - TE1 configred successfully, otherwise WAN_FALSE.
 ******************************************************************************
 */
static int sdla_ds_te1_chip_config_verify(sdla_fe_t *fe)
{
	int		e1_mode = 0;
	u_int8_t	rmmr, tmmr, value = 0x00;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	rmmr = READ_REG(REG_RMMR);
	DEBUG_EVENT("%s: RX: %s mode\n", 
			fe->name, (rmmr & BIT_RMMR_T1E1) ? "E1" : "T1");
	e1_mode = (value & BIT_RMMR_T1E1) ? 1 : 0;
	
	tmmr = READ_REG(REG_TMMR);
	DEBUG_EVENT("%s: TX: %s mode\n", 
			fe->name, (tmmr & BIT_TMMR_T1E1) ? "E1" : "T1");
	if ((rmmr & BIT_RMMR_T1E1) && (tmmr & BIT_RMMR_T1E1)){
		e1_mode = 1;
	} else  if ((rmmr & BIT_RMMR_T1E1) || (tmmr & BIT_RMMR_T1E1)){
		DEBUG_EVENT(
		"%s: ERROR: RX/TX has different mode configuration (%02X:%02X)!\n",
					fe->name, rmmr, tmmr);
		return -EINVAL;	
	}
	
	if (e1_mode){
		value = READ_REG(REG_RCR1);
		DEBUG_EVENT("%s: RX Ctrl Reg: %s %s %s\n", 
				fe->name,
				(value & BIT_RCR1_E1_RCRC4) ? "CRC4" : "NCRC4",
				(value & BIT_RCR1_E1_RHDB3) ? "HDB3": "AMI",
				(value & BIT_RCR1_E1_RSIGM) ? "CCS" : "CAS");
		value = READ_REG(REG_TCR1);
		DEBUG_EVENT("%s: TX Ctrl Reg: %s %s %s\n", 
				fe->name,
				(value & BIT_TCR1_E1_TCRC4) ? "CRC4" : "NCRC4",
				(value & BIT_TCR1_E1_THDB3) ? "HDB3": "AMI",
				(value & BIT_TCR1_E1_T16S) ? "CAS" : "CCS");	
	}else{
		value = READ_REG(REG_RCR1);
		DEBUG_EVENT("%s: RX Ctrl Reg: %s %s\n", 
				fe->name,
				(value & BIT_RCR1_T1_RFM) ? "D4" : "ESF",
				(value & BIT_RCR1_T1_RB8ZS) ? "B8ZS": "AMI");
		value = READ_REG(REG_TCR1);
		DEBUG_EVENT("%s: TX Ctrl Reg: %s\n", 
				fe->name,
				(value & BIT_TCR1_T1_TB8ZS) ? "B8ZS": "AMI");
		value = READ_REG(REG_TCR3);
		DEBUG_EVENT("%s: TX Ctrl Reg: %s\n", 
				fe->name,
				(value & BIT_TCR3_TFM) ? "D4" : "ESF");
	}
	
	return 0;
}
#endif

/*
 ******************************************************************************
 *				sdla_ds_te1_config()	
 *
 * Description: Configure Sangoma 8 ports T1/E1 board
 * Arguments:	
 * Returns:	WANTRUE - TE1 configred successfully, otherwise WAN_FALSE.
 ******************************************************************************
 */
static int sdla_ds_te1_config(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	int		err = 0;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if (wan_test_bit(TE_CONFIG_PAUSED,&fe->te_param.critical)) {
		return 0;
	}

	/* Revision/Chip ID (Reg. 0x0D) */
	if (sdla_ds_te1_device_id(fe)) return -EINVAL;
	switch(fe->fe_chip_id){
	case DEVICE_ID_DS26519:
		if ((int)WAN_FE_LINENO(fe) < 0 || WAN_FE_LINENO(fe) > 16){
			DEBUG_EVENT(
			"%s: TE Config: Invalid Port selected %d (Min=1 Max=16)\n",
					fe->name,
					WAN_FE_LINENO(fe)+1);
			return -EINVAL;
		}
		fe->fe_cfg.poll_mode=WANOPT_YES;
		break;
	case DEVICE_ID_DS26528:
		if ((int)WAN_FE_LINENO(fe) < 0 || WAN_FE_LINENO(fe) > 8){
			DEBUG_EVENT(
			"%s: TE Config: Invalid Port selected %d (Min=1 Max=8)\n",
					fe->name,
					WAN_FE_LINENO(fe)+1);
			return -EINVAL;
		}
		break;
	case DEVICE_ID_DS26524:
		if ((int)WAN_FE_LINENO(fe) < 0 || WAN_FE_LINENO(fe) > 4){
			DEBUG_EVENT(
			"%s: TE Config: Invalid Port selected %d (Min=1 Max=4)\n",
					fe->name,
					WAN_FE_LINENO(fe)+1);
			return -EINVAL;
		}
		break;
	case DEVICE_ID_DS26521:
	case DEVICE_ID_DS26522:
		if ((int)WAN_FE_LINENO(fe) < 0 || WAN_FE_LINENO(fe) > 1){
			DEBUG_EVENT(
			"%s: TE Config: Invalid Port selected %d (Min=1 Max=2)\n",
					fe->name,
					WAN_FE_LINENO(fe)+1);
			return -EINVAL;
		}
		break;		
	}

        if (IS_T1_FEMEDIA(fe) || IS_J1_FEMEDIA(fe)){
		err = sdla_ds_t1_cfg_verify(fe);
	}else if (IS_E1_FEMEDIA(fe)){
		err = sdla_ds_e1_cfg_verify(fe);
	}else{
		DEBUG_ERROR("%s: Error: Invalid FE Media type (%X)\n",
					fe->name,
					WAN_FE_MEDIA(fe));
		err =-EINVAL;
	}
	if (err) return -EINVAL;

	DEBUG_EVENT("%s: Configuring DS %s %s FE\n", 
				fe->name, 
				DECODE_CHIPID(fe->fe_chip_id),
				FE_MEDIA_DECODE(fe));
	DEBUG_EVENT("%s:    Port %d,%s,%s,%s\n", 
				fe->name, 
				WAN_FE_LINENO(fe)+1,
				FE_LCODE_DECODE(fe),
				FE_FRAME_DECODE(fe),
				TE_LBO_DECODE(fe));
	DEBUG_EVENT("%s:    Clk %s:%d, Channels: %X\n",
				fe->name, 
				TE_CLK_DECODE(fe),
				WAN_TE1_REFCLK(fe),
				WAN_TE1_ACTIVE_CH(fe));
	DEBUG_EVENT("%s:    AIS on LOS: %s\n",
			    fe->name,
				fe->fe_cfg.cfg.te_cfg.ais_auto_on_los?"On":"Off (default)");
						
	if (IS_E1_FEMEDIA(fe)){				
		DEBUG_EVENT("%s:    Sig Mode %s\n",
					fe->name, 
					WAN_TE1_SIG_DECODE(fe));
	}
	if (fe->fe_cfg.poll_mode == WANOPT_YES){
		sdla_t	*card = (sdla_t*)fe->card;
		DEBUG_EVENT("%s:    FE Poll driven\n",
					fe->name); 
		card->fe_no_intr = 1;	/* disable global front interrupt */
	}
	if (fe->fe_cfg.cfg.te_cfg.ignore_yel_alarm == WANOPT_YES){
		DEBUG_EVENT("%s:    YEL alarm ignored\n",
					fe->name); 
	}
	if (sdla_ds_te1_chip_config(fe)){
		return -EINVAL;
	}
	
	fe->te_param.max_channels = 
		(IS_E1_FEMEDIA(fe)) ? NUM_OF_E1_TIMESLOTS: NUM_OF_T1_CHANNELS;

	sdla_ds_te1_flush_pmon(fe);

	fe->swirq = wan_malloc(WAN_TE1_SWIRQ_MAX*sizeof(sdla_fe_swirq_t));
	if (fe->swirq == NULL){
		DEBUG_EVENT("%s: Failed to allocate memory (%s:%d)!\n",
				fe->name, __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	memset(fe->swirq, 0, WAN_TE1_SWIRQ_MAX*sizeof(sdla_fe_swirq_t));

	wan_set_bit(TE_CONFIGURED,(void*)&fe->te_param.critical);
	
	/* Start the global interrupt only when a first port been configured */
	{
		u_int8_t gintr_reg=READ_REG_LINE(0,REG_GTCR1);
		if (gintr_reg & 0x01) {
			gintr_reg &= ~(0x01);
			WRITE_REG_LINE(0, REG_GTCR1, gintr_reg);  
		}
	}
	
			
#if 0
/* FIXME: Enable all interrupt only when link is connected (event global) */
	/* Enable interrupts */
	sdla_ds_te1_intr_ctrl(fe, 0, WAN_TE_INTR_GLOBAL, WAN_FE_INTR_ENABLE, 0x00);
#endif 
	return 0;
}

static int sdla_ds_te1_force_config(void* pfe)
{
	sdla_fe_t       *fe = (sdla_fe_t*)pfe;

	wan_clear_bit(TE_CONFIG_PAUSED,&fe->te_param.critical);

	return sdla_ds_te1_config(pfe);
}

/*
 ******************************************************************************
 *			sdla_ds_te1_post_init()	
 *
 * Description: T1/E1 post init.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_post_init(void* pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;
	
	if (wan_test_bit(TE_CONFIG_PAUSED,&fe->te_param.critical)) {
		return 0;
	}

	/* Initialize and start T1/E1 timer */
	wan_set_bit(TE_TIMER_KILL,(void*)&fe->te_param.critical);
	
	wan_init_timer(&fe->timer, sdla_ds_te1_timer, (wan_timer_arg_t)fe);

	/* Initialize T1/E1 timer */
	wan_clear_bit(TE_TIMER_KILL,(void*)&fe->te_param.critical);

	/* Start T1/E1 timer */
	sdla_ds_te1_swirq_trigger(
			fe,
			WAN_TE1_SWIRQ_TYPE_LINK,
			WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN,
			POLLING_TE1_TIMER);

	/* Give the system time to start the rest of the card before
       we start polling */
	sdla_ds_te1_add_timer(fe, POLLING_TE1_TIMER*2);
	return 0;
}

/*
 ******************************************************************************
 *			sdla_ds_te1_post_unconfig()	
 *
 * Description: T1/E1 post_unconfig function (not locked routines)
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_post_unconfig(void* pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;
	sdla_fe_timer_event_t	*fe_event = NULL;
	wan_smp_flag_t		smp_flags;
	int			empty = 0;
	
	/* Kill TE timer poll command */
	wan_set_bit(TE_TIMER_KILL,(void*)&fe->te_param.critical);
	if (wan_test_bit(TE_TIMER_RUNNING,(void*)&fe->te_param.critical)){
		wan_del_timer(&fe->timer);
	}
	wan_clear_bit(TE_TIMER_RUNNING,(void*)&fe->te_param.critical);
	do{
		wan_spin_lock_irq(&fe->lockirq,&smp_flags);
		if (!WAN_LIST_EMPTY(&fe->event)){
			fe_event = WAN_LIST_FIRST(&fe->event);
			WAN_LIST_REMOVE(fe_event, next);
		}else{
			empty = 1;
		}
		wan_spin_unlock_irq(&fe->lockirq,&smp_flags);
		/* Free should be called not from spin_lock_irq (windows) !!!! */
		if (fe_event) wan_free(fe_event);
		fe_event = NULL;
	}while(!empty);

	if (fe->swirq){
		wan_free(fe->swirq);
		fe->swirq = NULL;
	}
	return 0;
}

/*
 ******************************************************************************
 *			sdla_ds_te1_unconfig()	
 *
 * Description: T1/E1 unconfig.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_unconfig(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	if (!wan_test_bit(TE_CONFIGURED,(void*)&fe->te_param.critical)) {
     	return 0;
	}

	DEBUG_EVENT("%s: %s Front End unconfigation!\n",
				fe->name, FE_MEDIA_DECODE(fe));	

	/* FIXME: Alex to disable interrupts here */
	sdla_ds_te1_disable_irq(fe);
	
	/* Set Rx Framer soft reset */
	WRITE_REG(REG_RMMR, BIT_RMMR_SFTRST);
	/* Set Tx Framer soft reset */
	WRITE_REG(REG_TMMR, BIT_RMMR_SFTRST);

	/* Clear configuration flag */
	wan_set_bit(TE_TIMER_KILL,(void*)&fe->te_param.critical);
	wan_clear_bit(TE_CONFIGURED,(void*)&fe->te_param.critical);

	//sdla_ds_te1_reset(fe, WAN_FE_LINENO(fe)+1, 1);
	
	//if (fe->fe_chip_id == DEVICE_ID_DS26521/* && fe->fe_cfg.line_no == 1*/){
	//	sdla_ds_te1_global_unconfig(fe);
	//}

	return 0;
}

static int sdla_ds_te1_force_unconfig(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	wan_set_bit(TE_CONFIG_PAUSED,&fe->te_param.critical);
	return sdla_ds_te1_unconfig(pfe);
}

/*
 ******************************************************************************
 *			sdla_ds_te1_disable_irq()	
 *
 * Description: T1/E1 unconfig.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_disable_irq(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	if (fe->fe_cfg.poll_mode == WANOPT_NO){
		/* Disable all interrupts */
		sdla_ds_te1_intr_ctrl(
			fe, 0, 
			(WAN_TE_INTR_GLOBAL|WAN_TE_INTR_BASIC|WAN_TE_INTR_PMON), 
			WAN_FE_INTR_MASK, 0x00);
	}
	return 0;
}

/*
 ******************************************************************************
 *			sdla_ds_te1_reconfig()	
 *
 * Description: T1/E1 post configuration.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_reconfig(sdla_fe_t* fe)
{
	if (wan_test_bit(TE_CONFIG_PAUSED,&fe->te_param.critical)) {
		return 0;
	}

	if (IS_E1_FEMEDIA(fe)){
		sdla_ds_e1_set_sig_mode(fe, 1);
	}
	return 0;
}

static int 
sdla_ds_te1_sigctrl(sdla_fe_t *fe, int sig_mode, unsigned long ch_map, int mode)
{	
	sdla_fe_timer_event_t	event;
	int			err;
	
	event.type		= (mode == WAN_ENABLE) ? 
					TE_RBS_ENABLE : TE_RBS_DISABLE;
	event.delay		= POLLING_TE1_TIMER;
	event.te_event.ch_map	= ch_map;
	err = sdla_ds_te1_add_event(fe, &event);
	if (err){
		DEBUG_EVENT("%s: Failed to add new fe event %02X ch_map=%08lX!\n",
					fe->name,
					event.type, event.te_event.ch_map);
		return -EINVAL;
	}
	return 0;

}


/******************************************************************************
**			sdla_ds_t1_is_alarm()	
**
** Description: Verify T1 status.
** Arguments:
** Returns:	1 - the port is connected
**		0 - the port is disconnected
******************************************************************************/
static u_int32_t sdla_ds_t1_is_alarm(sdla_fe_t *fe, u_int32_t alarms)
{
	u_int32_t alarm_mask = WAN_T1_FRAMED_ALARMS;

	/* Alex Feb 27, 2008
	** Special case for customer that uses 
	** YEL alarm for protocol control */
	if (fe->fe_cfg.cfg.te_cfg.ignore_yel_alarm == WANOPT_NO){
		alarm_mask |= WAN_TE_BIT_RAI_ALARM;
	}

	return (alarms & alarm_mask);
}

/******************************************************************************
**			sdla_ds_e1_is_alarm()	
**
** Description: Verify E1 status.
** Arguments:
** Returns:	1 - the port is connected
**		0 - the port is disconnected
******************************************************************************/
static u_int32_t sdla_ds_e1_is_alarm(sdla_fe_t *fe, u_int32_t alarms)
{
	u_int32_t	alarm_mask = 0x00;

	if (WAN_FE_FRAME(fe) == WAN_FR_UNFRAMED){
		alarm_mask = WAN_TE1_UNFRAMED_ALARMS;
		if (!fe->te_param.lb_mode_map){
			alarm_mask |= ( WAN_TE_BIT_ALARM_LIU_OC |
					WAN_TE_BIT_ALARM_LIU_SC |
					WAN_TE_BIT_ALARM_LIU_LOS);
		}
	}else{
		alarm_mask = WAN_E1_FRAMED_ALARMS;
	}

	/* Alex Feb 27, 2008
	** Special case for customer that uses 
	** YEL alarm for protocol control */
	if (fe->fe_cfg.cfg.te_cfg.ignore_yel_alarm == WANOPT_NO){
		alarm_mask |= WAN_TE_BIT_RAI_ALARM;
	}

	return (alarms & alarm_mask);
}

/******************************************************************************
**			sdla_ds_te1_set_status()	
**
** Description: Set T1/E1 status. Enable OOF and LCV interrupt (if status 
** 		changed to disconnected.
** Arguments:
** Returns:
******************************************************************************/
static int sdla_ds_te1_set_status(sdla_fe_t* fe, u_int32_t alarms)
{
	sdla_t		*card = (sdla_t*)fe->card;
	unsigned char	new_fe_status = fe->fe_status;
	u_int32_t	valid_rx_alarms = 0x00;

	if (IS_T1_FEMEDIA(fe)){
		valid_rx_alarms = sdla_ds_t1_is_alarm(fe, alarms);
	}else if (IS_E1_FEMEDIA(fe)){
		valid_rx_alarms = sdla_ds_e1_is_alarm(fe, alarms);
	}

	if (valid_rx_alarms){
		if (fe->fe_status != FE_DISCONNECTED){
			new_fe_status = FE_DISCONNECTED;
		}
	}else{
		if (fe->fe_status != FE_CONNECTED){
			new_fe_status = FE_CONNECTED;
		}
	}

	if (fe->fe_status == new_fe_status){

		DEBUG_TE1("%s: No Change TxYel=%i  Valid_rx=0x%08X RED=%i RAI Only=%i\n",
				fe->name,fe->te_param.tx_yel_alarm,valid_rx_alarms,valid_rx_alarms&WAN_TE_BIT_RED_ALARM,valid_rx_alarms==WAN_TE_BIT_RAI_ALARM);

		/* We must keep checking for RAI transmission here as well as on state change
  		   otherwise we could fail to tx RAI if the first reason for state change is RAI
  		   and then other other alarms get activated */
		if (!fe->te_param.tx_yel_alarm && (valid_rx_alarms & WAN_TE_BIT_RED_ALARM)){
			sdla_ds_te1_set_alarms(fe, WAN_TE_BIT_YEL_ALARM);
		}

		/* Special case where we tx RAI alarm and only incoming alarm is RAI
		   in this case we should disable our RAI. Usual case is loopback */
        if (fe->te_param.tx_yel_alarm && valid_rx_alarms == WAN_TE_BIT_RAI_ALARM){
			sdla_ds_te1_clear_alarms(fe, WAN_TE_BIT_YEL_ALARM);
		}     

		fe->te_param.status_cnt = 0;	
		DEBUG_TE1("%s: (1) %s %s...%d\n", 
					fe->name,
					FE_MEDIA_DECODE(fe),
					WAN_FE_STATUS_DECODE(fe),
					fe->te_param.status_cnt);
		return 0;
	}

	if (new_fe_status == FE_CONNECTED){
		if (fe->te_param.status_cnt > WAN_TE1_STATUS_THRESHOLD){

			if (fe->te_param.tx_yel_alarm){
				sdla_ds_te1_clear_alarms(fe, WAN_TE_BIT_YEL_ALARM);
			}

			DEBUG_EVENT("%s: %s connected!\n", 
					fe->name,
					FE_MEDIA_DECODE(fe));
			fe->fe_status = FE_CONNECTED;
			if (card->wandev.te_report_alarms){
				card->wandev.te_report_alarms(
						card,
						fe->fe_alarm);
			}
		}else{

			DEBUG_TE1("%s: (2) %s %s...%d\n", 
					fe->name,
					FE_MEDIA_DECODE(fe),
					WAN_FE_STATUS_DECODE(fe),
					fe->te_param.status_cnt);

			fe->te_param.status_cnt ++;
			fe->fe_status = FE_DISCONNECTED;
		}
	}else{
		DEBUG_EVENT("%s: %s disconnected!\n", 
				fe->name,
				FE_MEDIA_DECODE(fe));
		fe->fe_status = FE_DISCONNECTED;

		DEBUG_TE1("%s: TxYel=%i  Valid_rx=0x%08X\n",
				fe->name,fe->te_param.tx_yel_alarm,valid_rx_alarms);

		/* Line went down due to alarm (not just RAI) if yellow alarm was not
		   transmited then do so now */
		if (!fe->te_param.tx_yel_alarm && (valid_rx_alarms & WAN_TE_BIT_RED_ALARM)){
			sdla_ds_te1_set_alarms(fe, WAN_TE_BIT_YEL_ALARM);
		}

        /* Special case, loopback if only valid alarm is RAI and we already transmitted yellow,
		   then we must clear yellow */
		if (fe->te_param.tx_yel_alarm && valid_rx_alarms == WAN_TE_BIT_RAI_ALARM){
			sdla_ds_te1_clear_alarms(fe, WAN_TE_BIT_YEL_ALARM);
		}

		fe->te_param.status_cnt = 0;
		if (card->wandev.te_report_alarms){
			card->wandev.te_report_alarms(card, fe->fe_alarm);
		}
	}
	return 1;
}


/*
*******************************************************************************
**				sdla_te_alarm_print()	
**
** Description: 
** Arguments:
** Returns:
*/
static int sdla_ds_te1_print_alarms(sdla_fe_t* fe, unsigned int alarms)
{
	uint32_t tx_alarms=0;


	DEBUG_EVENT("%s: %s Framer Alarms status: RX (%08X):\n",
			fe->name,
			FE_MEDIA_DECODE(fe),
			alarms & WAN_TE_ALARM_FRAMER_MASK);

	if (alarms & WAN_TE_BIT_ALARM_RAI){
		DEBUG_EVENT("%s:    RAI : ON\n", fe->name);
	}
	if (alarms & WAN_TE_BIT_ALARM_LOS){
		DEBUG_EVENT("%s:    LOS : ON\n", fe->name);
	}
	if (alarms & WAN_TE_BIT_ALARM_OOF){
		DEBUG_EVENT("%s:    OOF : ON\n", fe->name);
	}
	if (alarms & WAN_TE_BIT_ALARM_RED){
		DEBUG_EVENT("%s:    RED : ON\n", fe->name);
	}
	DEBUG_EVENT("%s: %s LIU Alarms status (%08X):\n",
			fe->name,
			FE_MEDIA_DECODE(fe),
			alarms & WAN_TE_ALARM_LIU_MASK);
	if (alarms & WAN_TE_BIT_ALARM_LIU_OC){
		DEBUG_EVENT("%s:    Open Circuit is detected!\n",
				fe->name);
	}
	if (alarms & WAN_TE_BIT_ALARM_LIU_SC){
		DEBUG_EVENT("%s:    Short Circuit is detected!\n",
				fe->name);
	}
	if (alarms & WAN_TE_BIT_ALARM_LIU_LOS){
		DEBUG_EVENT("%s:    Lost of Signal is detected!\n",
				fe->name);
	}

	if (fe->te_param.tx_yel_alarm) {
		tx_alarms |= WAN_TE_BIT_YEL_ALARM;
	}

	if (fe->te_param.tx_ais_alarm) {
		tx_alarms |= WAN_TE_BIT_ALARM_AIS;
	}

	DEBUG_EVENT("%s: %s Framer Alarms status: TX (%08X):\n",
			fe->name,
			FE_MEDIA_DECODE(fe),
			tx_alarms);

	if (fe->te_param.tx_yel_alarm) {
		DEBUG_EVENT("%s:    YEL : TX\n", fe->name);
	}

	if (fe->te_param.tx_ais_alarm) {
		DEBUG_EVENT("%s:    AIS : TX\n", fe->name);
	}

	return 0;
}


/******************************************************************************
**				sdla_te_read_alarms()	
**
** Description: 
** Arguments:
** Returns:
******************************************************************************/
static u_int32_t sdla_ds_te1_read_frame_alarms(sdla_fe_t *fe)
{
	u_int32_t		alarm = fe->fe_alarm;
	unsigned char		rrts1 = READ_REG(REG_RRTS1);

	alarm &= WAN_TE_ALARM_FRAMER_MASK;
	DEBUG_TE1("%s: Reading %s Framer status (Old:%08X,%02X)\n", 
				fe->name, FE_MEDIA_DECODE(fe),
				rrts1, alarm);
			
	/* Framer alarms */
	//if (WAN_FE_FRAME(fe) != WAN_FR_UNFRAMED){
		if (rrts1 & BIT_RRTS1_RRAI){
			if (!(alarm & WAN_TE_BIT_ALARM_RAI)){
				DEBUG_EVENT("%s:    RAI : ON\n", 
							fe->name);
			}
			alarm |= WAN_TE_BIT_ALARM_RAI;		
		}else{
			if (alarm & WAN_TE_BIT_ALARM_RAI){
				DEBUG_EVENT("%s:    RAI : OFF\n", 
							fe->name);
			}
			alarm &= ~WAN_TE_BIT_ALARM_RAI;		
		}
	//}

	if (rrts1 & BIT_RRTS1_RAIS){
		if (!IS_TE_ALARM_AIS(alarm)){
			sdla_ds_te1_swirq_trigger(
					fe,
					WAN_TE1_SWIRQ_TYPE_ALARM_AIS,
					WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON,
					WAN_T1_ALARM_THRESHOLD_AIS_ON);
		}
	}else{
		if (IS_TE_ALARM_AIS(alarm)){
			sdla_ds_te1_swirq_trigger(
					fe,
					WAN_TE1_SWIRQ_TYPE_ALARM_AIS,
					WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF,
					WAN_T1_ALARM_THRESHOLD_AIS_OFF);
		}
	}

	if (rrts1 & BIT_RRTS1_RLOS){
		if (!IS_TE_ALARM_LOS(alarm)){
			sdla_ds_te1_swirq_trigger(
					fe,
					WAN_TE1_SWIRQ_TYPE_ALARM_LOS,
					WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON,
					WAN_T1_ALARM_THRESHOLD_LOS_ON);
		}
	}else{
		if (IS_TE_ALARM_LOS(alarm)){
			sdla_ds_te1_swirq_trigger(
					fe,
					WAN_TE1_SWIRQ_TYPE_ALARM_LOS,
					WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF,
					WAN_T1_ALARM_THRESHOLD_LOS_OFF);
		}
	}

	if (WAN_FE_FRAME(fe) != WAN_FR_UNFRAMED){
		if (rrts1 & BIT_RRTS1_RLOF){
			if (!IS_TE_ALARM_LOF(alarm)){
				sdla_ds_te1_swirq_trigger(
					fe,
					WAN_TE1_SWIRQ_TYPE_ALARM_LOF,
					WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON,
					WAN_T1_ALARM_THRESHOLD_LOF_ON);
			}
		}else{
			if (IS_TE_ALARM_LOF(alarm)){
				sdla_ds_te1_swirq_trigger(
					fe,
					WAN_TE1_SWIRQ_TYPE_ALARM_LOF,
					WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF,
					WAN_T1_ALARM_THRESHOLD_LOF_OFF);
			}
		}
	}
	/* Aug 30, 2006
	** Red alarm is either LOS or OOF alarms */
	if (IS_TE_ALARM_LOF(alarm) ||
	    IS_TE_ALARM_LOS(alarm)){
		if (!IS_TE_ALARM_RED(alarm)){
			DEBUG_EVENT("%s:    RED : ON\n", 
						fe->name);
		}
		alarm |= WAN_TE_BIT_ALARM_RED;
	}else{
		if (IS_TE_ALARM_RED(alarm)){
			DEBUG_EVENT("%s:    RED : OFF\n", 
						fe->name);
		}
		alarm &= ~WAN_TE_BIT_ALARM_RED;
	}
	return alarm;
}

static unsigned int sdla_ds_te1_read_liu_alarms(sdla_fe_t *fe)
{
	unsigned int	alarm = fe->fe_alarm;
	unsigned char	lrsr = READ_REG(REG_LRSR);

	alarm &= WAN_TE_ALARM_LIU_MASK;
	DEBUG_TE1("%s: Reading %s LIU status (Old:%08X,%02X)\n", 
			    	fe->name, FE_MEDIA_DECODE(fe),
                    		lrsr, alarm);

	/* LIU alarms */
	if (lrsr & BIT_LRSR_OCS){
		if (!(alarm & WAN_TE_BIT_ALARM_LIU_OC)){
			DEBUG_EVENT("%s: Open Circuit is detected!\n",
					fe->name);
		}
		alarm |= WAN_TE_BIT_ALARM_LIU_OC;
	}else{
		if (alarm & WAN_TE_BIT_ALARM_LIU_OC){
			DEBUG_EVENT("%s: Open Circuit is cleared!\n",
					fe->name);
		}
		alarm &= ~WAN_TE_BIT_ALARM_LIU_OC;
	}
	if (lrsr & BIT_LRSR_SCS){
		if (!(alarm & WAN_TE_BIT_ALARM_LIU_SC)){
			DEBUG_EVENT("%s: Short Circuit is detected!(%i)\n",
					fe->name, __LINE__);
		}
		alarm |= WAN_TE_BIT_ALARM_LIU_SC;
	}else{
		if (alarm & WAN_TE_BIT_ALARM_LIU_SC){
			DEBUG_EVENT("%s: Short Circuit is cleared!(%i)\n",
					fe->name, __LINE__);
		}
		alarm &= ~WAN_TE_BIT_ALARM_LIU_SC;	
	}
	if (lrsr & BIT_LRSR_LOSS){
		if (!(alarm & WAN_TE_BIT_ALARM_LIU_LOS)){
			DEBUG_EVENT("%s: Lost of Signal is detected!\n",
					fe->name);
		}
		alarm |= WAN_TE_BIT_ALARM_LIU_LOS;
	}else{
		if (alarm & WAN_TE_BIT_ALARM_LIU_LOS){
			DEBUG_EVENT("%s: Lost of Signal is cleared!\n",
					fe->name);
		}
		alarm &= ~WAN_TE_BIT_ALARM_LIU_LOS;	
	}

	return alarm;
}


static u_int32_t  sdla_ds_te1_read_tx_alarms(sdla_fe_t *fe, int action)
{
	u_int32_t alarm=0;

	if (IS_FE_ALARM_READ(action)){
		if (fe->te_param.tx_yel_alarm) {
			alarm |= WAN_TE_BIT_YEL_ALARM;
		}
	
		if (fe->te_param.tx_ais_alarm) {
			alarm |= WAN_TE_BIT_ALARM_AIS;
		}
	}

	if (IS_FE_ALARM_PRINT(action)){
		sdla_ds_te1_print_alarms(fe, fe->fe_alarm);
	}

	return alarm;
}

static u_int32_t  sdla_ds_te1_read_alarms(sdla_fe_t *fe, int action)
{
	u_int32_t alarm = fe->fe_alarm;

	if (IS_FE_ALARM_READ(action)){

		alarm = sdla_ds_te1_read_frame_alarms(fe);
		alarm |= sdla_ds_te1_read_liu_alarms(fe);
	}
	if (IS_FE_ALARM_PRINT(action)){
		sdla_ds_te1_print_alarms(fe, alarm);
	}
	if (IS_FE_ALARM_UPDATE(action)){
		fe->fe_alarm = alarm;
	}

	return fe->fe_alarm;
}

/******************************************************************************
**				sdla_ds_te1_update_alarm()	
**
** Description: 
** Arguments:
** Returns:
******************************************************************************/
static int
sdla_ds_te1_update_alarms(sdla_fe_t *fe, u_int32_t alarms)
{
	
	if (!IS_TE_ALARM_AIS(fe->fe_alarm)){
		if (IS_TE_ALARM_AIS(alarms)){
			DEBUG_EVENT("%s:    AIS : ON\n", 
						fe->name);
			fe->fe_alarm |= WAN_TE_BIT_ALARM_AIS;
		}
	}else{
		if (!IS_TE_ALARM_AIS(alarms)){
			DEBUG_EVENT("%s:    AIS : OFF\n", 
						fe->name);
			fe->fe_alarm &= ~WAN_TE_BIT_ALARM_AIS;
		}
	}

	if (!IS_TE_ALARM_LOF(fe->fe_alarm)){
		if (IS_TE_ALARM_LOF(alarms)){
			DEBUG_EVENT("%s:    LOF : ON\n", 
						fe->name);
			fe->fe_alarm |= WAN_TE_BIT_ALARM_LOF;
		}
	}else{
		if (!IS_TE_ALARM_LOF(alarms)){
			DEBUG_EVENT("%s:    LOF : OFF\n", 
						fe->name);
			fe->fe_alarm &= ~WAN_TE_BIT_ALARM_LOF;
		}
	}

	if (!IS_TE_ALARM_LOS(fe->fe_alarm)){
		if (IS_TE_ALARM_LOS(alarms)){
			DEBUG_EVENT("%s:    LOS : ON\n", 
						fe->name);
			fe->fe_alarm |= WAN_TE_BIT_ALARM_LOS;
		}
	}else{
		if (!IS_TE_ALARM_LOS(alarms)){
			DEBUG_EVENT("%s:    LOS : OFF\n", 
						fe->name);
			fe->fe_alarm &= ~WAN_TE_BIT_ALARM_LOS;
		}
	}

	if (IS_T1_FEMEDIA(fe)){
		if (IS_TE_ALARM_LOF(fe->fe_alarm) ||
		    IS_TE_ALARM_LOS(fe->fe_alarm)){
			if (!IS_TE_ALARM_RED(fe->fe_alarm)){
				DEBUG_EVENT("%s:    RED : ON\n", 
							fe->name);
			}
			fe->fe_alarm |= WAN_TE_BIT_ALARM_RED;
		}else{
			if (IS_TE_ALARM_RED(fe->fe_alarm)){
				DEBUG_EVENT("%s:    RED : OFF\n", 
							fe->name);
			}
			fe->fe_alarm &= ~WAN_TE_BIT_ALARM_RED;
		}
	}

	DEBUG_TE1("%s: Alarm update called  prev=0x%08X  new=0x%08X...\n",fe->name,fe->fe_prev_alarm,fe->fe_alarm);
	if (fe->fe_prev_alarm != fe->fe_alarm) {
		sdla_t* card = (sdla_t*)fe->card;
		fe->fe_prev_alarm = fe->fe_alarm;
		if (card->wandev.te_report_alarms){
			card->wandev.te_report_alarms(
				card,
				fe->fe_alarm);
		}
	}

	return 0;
}

#define WAN_TE_CRIT_ALARM_TIMEOUT	30	/* 30 sec */
static int sdla_ds_te1_read_crit_alarms(sdla_fe_t *fe)
{
	u_int32_t	liu_alarms = 0x00;

	liu_alarms = sdla_ds_te1_read_liu_alarms(fe);
	if (liu_alarms & WAN_TE_BIT_ALARM_LIU_SC){
		fe->te_param.crit_alarm_start = SYSTEM_TICKS;
	}else{
		if (WAN_STIMEOUT(fe->te_param.crit_alarm_start, WAN_TE_CRIT_ALARM_TIMEOUT)){
			/* The link was stable for 30 sec, let try to go back */
			return 0;
		}
	}
	/* we are still in critical alarm state */
	return 1;
}

/******************************************************************************
**				sdla_ds_te1_set_alarms()	
**
** Description:
** Arguments:
** Returns:
*/
static int sdla_ds_te1_set_alarms(sdla_fe_t* fe, u_int32_t alarms)
{
	u8	value;

	

	/* NC: Always set yellow alarm no need to check whether
	 * yellow alarm is ignored */	
	if (alarms & WAN_TE_BIT_YEL_ALARM && !fe->te_param.tx_ais_alarm){
		if (IS_T1_FEMEDIA(fe)){
			value = READ_REG(REG_TCR1);
			if (!(value & BIT_TCR1_T1_TRAI)){
				DEBUG_EVENT("%s: Enable transmit YEL alarm\n",
								fe->name);
				WRITE_REG(REG_TCR1, value | BIT_TCR1_T1_TRAI);
			}
			fe->te_param.tx_yel_alarm = 1;
			fe->fe_stats.tx_alarms |= WAN_TE_BIT_YEL_ALARM;
		}else{
			value = READ_REG(REG_E1TNAF);
			if (!(value & BIT_E1TNAF_A)){
				DEBUG_EVENT("%s: Enable transmit YEL alarm\n",
								fe->name);
				WRITE_REG(REG_E1TNAF, value | BIT_E1TNAF_A);
			}
			fe->te_param.tx_yel_alarm = 1;
			fe->fe_stats.tx_alarms |= WAN_TE_BIT_YEL_ALARM;
		}
	}

	if (alarms & WAN_TE_BIT_ALARM_AIS) {
		value = READ_REG(REG_TCR1);
		if (!(value & BIT_TCR1_T1_TAIS)){
			DEBUG_EVENT("%s: Enable transmit AIS alarm\n",
							fe->name);
			WRITE_REG(REG_TCR1, value | BIT_TCR1_T1_TAIS);
		}
		fe->te_param.tx_ais_alarm = 1;
		fe->fe_stats.tx_alarms |= WAN_TE_BIT_ALARM_AIS;
	}
	
	return 0;
}

/******************************************************************************
**				sdla_ds_te1_clear_alarms()	
**
** Description:
** Arguments:
** Returns:
*/
static int sdla_ds_te1_clear_alarms(sdla_fe_t* fe, u_int32_t alarms)
{
	u8	value;

	
	
	/* NC: Always set yellow alarm no need to check whether
	 * yellow alarm is ignored */	
	if (alarms & WAN_TE_BIT_YEL_ALARM) {
		if (IS_T1_FEMEDIA(fe)){
			value = READ_REG(REG_TCR1);
			if (value & BIT_TCR1_T1_TRAI) {
				DEBUG_EVENT("%s: Disable transmit YEL alarm\n",
							fe->name);
				WRITE_REG(REG_TCR1, value & ~BIT_TCR1_T1_TRAI);
			}
			fe->te_param.tx_yel_alarm = 0;
			fe->fe_stats.tx_alarms &= ~WAN_TE_BIT_YEL_ALARM;
		}else{
			value = READ_REG(REG_E1TNAF);
			if (value & BIT_E1TNAF_A) {
				DEBUG_EVENT("%s: Disable transmit YEL alarm\n",
							fe->name);
				WRITE_REG(REG_E1TNAF, value & ~BIT_E1TNAF_A);
			}
			fe->te_param.tx_yel_alarm = 0;
			fe->fe_stats.tx_alarms &= ~WAN_TE_BIT_YEL_ALARM;
		}
	}

	if (alarms & WAN_TE_BIT_ALARM_AIS) {
		value = READ_REG(REG_TCR1);
		if (value & BIT_TCR1_T1_TAIS) {
			DEBUG_EVENT("%s: Disable transmit AIS alarm\n",
							fe->name);
			WRITE_REG(REG_TCR1, value & ~BIT_TCR1_T1_TAIS);
		}
		fe->te_param.tx_ais_alarm = 0;
		fe->fe_stats.tx_alarms &= ~WAN_TE_BIT_ALARM_AIS;
	}

	return 0;
}

/******************************************************************************
**				sdla_ds_te1_clear_alarms()	
**
** Description:
** Arguments:
** Returns:
*/
static int sdla_ds_te1_rbs_init(sdla_fe_t* fe)
{
	int	i;

	if (IS_E1_FEMEDIA(fe)){
		for(i = 1; i < 16; i++){
			WRITE_REG(REG_TS1+i, 0xFF);
		}		
	}else{
		for(i = 0; i < 12; i++){
			WRITE_REG(REG_TS1+i, 0xFF);
		}		
	}
	return 0;
}

/******************************************************************************
**				sdla_ds_te1_clear_alarms()	
**
** Description:
** Arguments:
** Returns:
*/
static int sdla_ds_te1_rbs_ctrl(sdla_fe_t* fe, u_int32_t ch_map, int enable)
{
	u_int8_t	value;
	unsigned int	ch, bit, off;

	if (IS_T1_FEMEDIA(fe)){
		value = READ_REG(REG_TCR1);
		if (enable == WAN_TRUE){		
			value |= BIT_TCR1_T1_TSSE;
		}else{
			value &= ~BIT_TCR1_T1_TSSE;
		}
		WRITE_REG(REG_TCR1, value);
	}else{
		WAN_TE1_SIG_MODE(fe) = WAN_TE1_SIG_CAS;
		sdla_ds_e1_set_sig_mode(fe,1);
	}
			
	for(ch = 1; ch <= fe->te_param.max_channels; ch++){
		if (!wan_test_bit(ch, &ch_map)){
			continue;
		}
		if (IS_T1_FEMEDIA(fe)){
			bit = (ch-1) % 8;
			off = (ch-1) / 8;
		}else{
			if (ch == 16) continue;
			bit = ch % 8;
			off = ch / 8;
		}
		value = READ_REG(REG_SSIE1+off);
		if (enable == WAN_TRUE){		
			value |= (1<<bit);
		}else{
			value &= ~(1<<bit);
		}
		DEBUG_TE1("%s: Channel %d: SSIE=%02X Val=%02X\n",
					fe->name, ch, REG_SSIE1+off, value);
		WRITE_REG(REG_SSIE1+off, value);	
	}

	return 0;
}

/******************************************************************************
**				sdla_ds_te1_rbs_report()	
**
** Description:
** Arguments:
** Returns:
******************************************************************************/
static int sdla_ds_te1_rbs_report(sdla_fe_t* fe)
{
	sdla_t*		card = (sdla_t*)fe->card;
	int		ch = 1, max_channels;
	
	max_channels = fe->te_param.max_channels;
	ch = (IS_E1_FEMEDIA(fe)) ? 0 : 1;
	for(; ch <= max_channels; ch++) {
		if (wan_test_bit(ch, &fe->te_param.rx_rbs_status)){
			if (card->wandev.te_report_rbsbits){
				card->wandev.te_report_rbsbits(
					card, 
					ch, 
					fe->te_param.rx_rbs[ch]);
			}
			wan_clear_bit(ch, &fe->te_param.rx_rbs_status);
		}
	}
	return 0;
}


/*
 ******************************************************************************
 *				sdla_ds_te1_read_rbsbits()	
 *
 * Description:
 * Arguments:	channo: 1-24 for T1
 *			1-32 for E1
 * Returns:
 ******************************************************************************
 */
static unsigned char sdla_ds_te1_read_rbsbits(sdla_fe_t* fe, int channo, int mode)
{
	sdla_t*		card = (sdla_t*)fe->card;
	int		rs_offset = 0, range = 0;
	unsigned char	rbsbits = 0x00, status = 0x00;

	if (wan_test_bit(WAN_TE1_SWIRQ_TYPE_ALARM_LOS, (void*)&fe->swirq_map) || 
	    wan_test_bit(WAN_TE1_SWIRQ_TYPE_ALARM_LOF, (void*)&fe->swirq_map)){
		/* Keep the original values */
		return 0;
	}
	
	if (IS_E1_FEMEDIA(fe)){
		rs_offset = channo % 16;
		range = 16;
	}else{
		rs_offset = (channo - 1) % 12;
		range = 12;
	}
	rbsbits = READ_REG(REG_RS1 + rs_offset);
	if (channo <= range){
		rbsbits = (rbsbits >> 4) & 0x0F;
	}else{
		rbsbits &= 0xF;
	}
	
	if (rbsbits & BIT_RS_A) status |= WAN_RBS_SIG_A;
	if (rbsbits & BIT_RS_B) status |= WAN_RBS_SIG_B;
	if (rbsbits & BIT_RS_C) status |= WAN_RBS_SIG_C;
	if (rbsbits & BIT_RS_D) status |= WAN_RBS_SIG_D;

	if (mode & WAN_TE_RBS_UPDATE){
		sdla_ds_te1_rbs_update(fe, channo, status);
	}

	if ((mode & WAN_TE_RBS_REPORT) && card->wandev.te_report_rbsbits){
		card->wandev.te_report_rbsbits(
					card, 
					channo, 
					status);
	}
	return status;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_check_rbsbits()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int 
sdla_ds_te1_check_rbsbits(sdla_fe_t* fe, int ch_base, unsigned int ts_map, int report)
{
	sdla_t*		card = (sdla_t*)fe->card;
	unsigned char	rs_status = 0x0, rbsbits = 0x00, bit_mask = 0x00;
	int		rs_reg = 0, rs_offset = 0;
	int		i = 0, channel, range = 12;

	if (wan_test_bit(WAN_TE1_SWIRQ_TYPE_ALARM_LOS, (void*)&fe->swirq_map) || 
	    wan_test_bit(WAN_TE1_SWIRQ_TYPE_ALARM_LOF, (void*)&fe->swirq_map)){
		/* Keep the original values */
		return 0;
	}

	switch(ch_base){
	case 1:
		rs_reg = REG_RSS1;
		break;
	case 9:
		rs_reg = REG_RSS2;
		break;
	case 17:
		rs_reg = REG_RSS3;
		break;
	case 25:
		rs_reg = REG_RSS4;
		break;
	}
	rs_status = READ_REG(rs_reg);
	
	if (rs_status == 0x00){
		return 0;
	}

	if (IS_E1_FEMEDIA(fe)){
		range = 16;
	}
	for(i = 0; i < 8; i ++) {
		channel = ch_base + i;
		if (IS_E1_FEMEDIA(fe)){
			if (channel == 1 || channel == 17){
				continue;
			}
			rs_offset = (channel - 1) % 16;
		}else{
			rs_offset = (channel - 1) % 12;
		}
		/* If this channel/timeslot is not selected, move to 
		 * another channel/timeslot */
		if (!wan_test_bit(channel-1, &ts_map)){
			continue;
		}
		bit_mask = (1 << i);
		if(rs_status & bit_mask) {
			unsigned char	abcd_status = 0x00;

			rbsbits = READ_REG(REG_RS1 + rs_offset);
			DEBUG_TE1("%s: Channel %d: RS=%X+%d Val=%02X\n",
					fe->name, channel, REG_RS1, rs_offset, rbsbits);
			if (channel > range){
				rbsbits &= 0x0F;
			}else{
				rbsbits = (rbsbits >> 4) & 0x0F;
			}
		
			if (rbsbits & BIT_RS_A) abcd_status |= WAN_RBS_SIG_A;
			if (rbsbits & BIT_RS_B) abcd_status |= WAN_RBS_SIG_B;
			if (rbsbits & BIT_RS_C) abcd_status |= WAN_RBS_SIG_C;
			if (rbsbits & BIT_RS_D) abcd_status |= WAN_RBS_SIG_D;
			if (IS_E1_FEMEDIA(fe)){
				channel--;
			}

			sdla_ds_te1_rbs_update(fe, channel, abcd_status);
			if (report && card->wandev.te_report_rbsbits){
				card->wandev.te_report_rbsbits(
							card, 
							channel, 
							abcd_status);
			}
			WRITE_REG(rs_reg, bit_mask);
		}
	}	
	return 0; 
}

/*
 ******************************************************************************
 *				sdla_ds_te1_set_RBS()	
 *
 * Description:
 * Arguments:	T1: 1-24 E1: 0-31
 * Returns:
 ******************************************************************************
 */
static int 
sdla_ds_te1_set_rbsbits(sdla_fe_t *fe, int channel, unsigned char status)
{
	int		ts_off = 0, range = 0;
	unsigned char	rbsbits = 0x00, ts_org;

	if ((unsigned int)channel > fe->te_param.max_channels){
		DEBUG_EVENT("%s: Invalid channel number %d (%d)\n",
				fe->name, channel, fe->te_param.max_channels);
		return -EINVAL;
	}
#if 0
	if (IS_E1_FEMEDIA(fe) && (channel == 0 || channel == 16)){
		DEBUG_EVENT("%s: Invalid channel number %d for E1 Rx SIG\n",
				fe->name, channel);
		return 0;
	}
#endif
	if (status & WAN_RBS_SIG_A) rbsbits |= BIT_TS_A;
	if (status & WAN_RBS_SIG_B) rbsbits |= BIT_TS_B;
	if (!(IS_T1_FEMEDIA(fe) && WAN_FE_FRAME(fe) == WAN_FR_D4)){
		if (status & WAN_RBS_SIG_C) rbsbits |= BIT_TS_C;
		if (status & WAN_RBS_SIG_D) rbsbits |= BIT_TS_D;
	}
	if (fe->fe_debug & WAN_FE_DEBUG_RBS_TX_ENABLE){
		DEBUG_EVENT("%s: %s:%-3d TX RBS A:%1d B:%1d C:%1d D:%1d\n",
				fe->name,
				FE_MEDIA_DECODE(fe),
				channel,	
				(rbsbits & BIT_TS_A) ? 1 : 0,
				(rbsbits & BIT_TS_B) ? 1 : 0,
				(rbsbits & BIT_TS_C) ? 1 : 0,
				(rbsbits & BIT_TS_D) ? 1 : 0);
	}
	if (rbsbits & BIT_TS_A){
		wan_set_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_A);
	}else{
		wan_clear_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_A);
	}
	if (rbsbits & BIT_TS_B){
		wan_set_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_B);
	}else{
		wan_clear_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_B);
	}
	if (rbsbits & BIT_TS_C){
		wan_set_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_C);
	}else{
		wan_clear_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_C);
	}
	if (rbsbits & BIT_TS_D){
		wan_set_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_D);
	}else{
		wan_clear_bit(channel,(unsigned long*)&fe->te_param.tx_rbs_D);
	}
	if (IS_E1_FEMEDIA(fe)){
		ts_off	= channel % 16;
		range	= 15;
	}else{
		ts_off	= (channel - 1) % 12;
		range	= 12;
	}
	ts_org = READ_REG(REG_TS1 + ts_off);
	if (channel <= range){
		rbsbits = rbsbits << 4;
		ts_org &= 0xF;
	}else{
		ts_org &= 0xF0;
	}

	DEBUG_TE1("%s: TS=%02X Val=%02X\n",
				fe->name, 
				REG_TS1+ts_off,
				rbsbits);

	WRITE_REG(REG_TS1 + ts_off, ts_org | rbsbits);
	return 0;
}

/*
 ******************************************************************************
 *				sdla_te_rbs_update()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int 
sdla_ds_te1_rbs_update(sdla_fe_t* fe, int channo, unsigned char status)
{

	if (fe->fe_debug & WAN_FE_DEBUG_RBS_RX_ENABLE && 
	    fe->te_param.rx_rbs[channo] != status){
		DEBUG_EVENT(
		"%s: %s:%-3d RX RBS A:%1d B:%1d C:%1d D:%1d\n",
					fe->name,
					FE_MEDIA_DECODE(fe),
					channo, 
					(status & WAN_RBS_SIG_A) ? 1 : 0,
					(status & WAN_RBS_SIG_B) ? 1 : 0,
					(status & WAN_RBS_SIG_C) ? 1 : 0,
					(status & WAN_RBS_SIG_D) ? 1 : 0);
	}
	
	/* Update rbs value in private structures */
	wan_set_bit(channo, &fe->te_param.rx_rbs_status);
	fe->te_param.rx_rbs[channo] = status;

	if (status & WAN_RBS_SIG_A){
		wan_set_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_A);
	}else{
		wan_clear_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_A);
	}	
	if (status & WAN_RBS_SIG_B){
		wan_set_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_B);
	}else{
		wan_clear_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_B);
	}
	if (status & WAN_RBS_SIG_C){
		wan_set_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_C);
	}else{
		wan_clear_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_C);
	}
	if (status & WAN_RBS_SIG_D){
		wan_set_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_D);
	}else{
		wan_clear_bit(channo,
			(unsigned long*)&fe->te_param.rx_rbs_D);
	}

	return 0;
}

/*
 ******************************************************************************
 *				sdla_te_rbs_print_banner()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int 
sdla_ds_te1_rbs_print_banner(sdla_fe_t* fe)
{
	if (IS_T1_FEMEDIA(fe)){
		DEBUG_EVENT("%s:                111111111122222\n",
					fe->name);
		DEBUG_EVENT("%s:       123456789012345678901234\n",
					fe->name);
		DEBUG_EVENT("%s:       ------------------------\n",
					fe->name);
	}else{
		DEBUG_EVENT("%s:                11111111112222222222333\n",
					fe->name);
		DEBUG_EVENT("%s:       12345678901234567890123456789012\n",
					fe->name);
		DEBUG_EVENT("%s:       --------------------------------\n",
					fe->name);
	}	
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_rbs_print_bits()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int 
sdla_ds_te1_rbs_print_bits(sdla_fe_t* fe, unsigned long bits, char *msg)
{
	int 	i, max_channels = fe->te_param.max_channels;
	int	start_chan = 1;
	char bits_str[256];

	if (IS_E1_FEMEDIA(fe)){
		start_chan = 0;
	}

	wp_snprintf(bits_str, sizeof(bits_str), "%s: %s ", fe->name, msg);

	for(i=start_chan; i <= max_channels; i++) {

		wp_snprintf(&bits_str[strlen(bits_str)], 
			sizeof(bits_str) - strlen(bits_str), 
			"%01d", 
			wan_test_bit(i, &bits) ? 1 : 0);

	}

	DEBUG_EVENT("%s\n", bits_str);

	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_rbs_print()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int 
sdla_ds_te1_rbs_print(sdla_fe_t* fe, int last_status)
{
	unsigned long	rx_a = 0x00;
	unsigned long	rx_b = 0x00;
	unsigned long	rx_c = 0x00;
	unsigned long	rx_d = 0x00;
	
	if (last_status){
		rx_a = fe->te_param.rx_rbs_A;	
		rx_b = fe->te_param.rx_rbs_B;
		rx_c = fe->te_param.rx_rbs_C;
		rx_d = fe->te_param.rx_rbs_D;
		
		DEBUG_EVENT("%s: Last Status:\n",
					fe->name);
		sdla_ds_te1_rbs_print_banner(fe);

		/* The TX bits come from user-mode in 1-based form, for 
		 * consistency with RX bits, which printed in 0-based
		 * form, shift the TX bits right by one. */
		sdla_ds_te1_rbs_print_bits(fe, fe->te_param.tx_rbs_A >> 1, "TX A:");
		sdla_ds_te1_rbs_print_bits(fe, fe->te_param.tx_rbs_B >> 1, "TX B:");
		sdla_ds_te1_rbs_print_bits(fe, fe->te_param.tx_rbs_C >> 1, "TX C:");
		sdla_ds_te1_rbs_print_bits(fe, fe->te_param.tx_rbs_D >> 1, "TX D:");
		DEBUG_EVENT("%s:\n", fe->name);
	}else{
		unsigned int	i, chan = 0;
		unsigned char	abcd = 0x00;
		for(i = 1; i <= fe->te_param.max_channels; i++) {

			abcd = sdla_ds_te1_read_rbsbits(fe, i, WAN_TE_RBS_NONE);

			chan = (IS_E1_FEMEDIA(fe))? i - 1 : i;
			if (abcd & WAN_RBS_SIG_A){
				wan_set_bit(chan, &rx_a);
			}
			if (abcd & WAN_RBS_SIG_B){
				wan_set_bit(chan, &rx_b);
			}
			if (abcd & WAN_RBS_SIG_C){
				wan_set_bit(chan, &rx_c);
			}
			if (abcd & WAN_RBS_SIG_D){
				wan_set_bit(chan, &rx_d);
			}
		}
		DEBUG_EVENT("%s: Current Status:\n",
					fe->name);
		sdla_ds_te1_rbs_print_banner(fe);
	}

	sdla_ds_te1_rbs_print_bits(fe, rx_a, "RX A:");
	sdla_ds_te1_rbs_print_bits(fe, rx_b, "RX B:");
	sdla_ds_te1_rbs_print_bits(fe, rx_c, "RX C:");
	sdla_ds_te1_rbs_print_bits(fe, rx_d, "RX D:");
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_intr_ctrl()	
 *
 * Description: Check interrupt type.
 * Arguments: 	card 		- pointer to device structure.
 * 		write_register 	- write register function.
 * 		read_register	- read register function.
 * Returns:	None.
 ******************************************************************************
 */
static int
sdla_ds_te1_intr_ctrl(sdla_fe_t *fe, int dummy, u_int8_t type, u_int8_t mode, unsigned int ts_map) 
{
	unsigned char	mask, value;
	unsigned char	rscse;
	unsigned int	ch, bit, off;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	WAN_ASSERT(fe->fe_cfg.poll_mode == WANOPT_YES);

	if (!wan_test_bit(TE_CONFIGURED,(void*)&fe->te_param.critical)){
		return 0;
	}

	if (type & WAN_TE_INTR_GLOBAL){
		mask = READ_REG(REG_GFIMR);
		if (mode == WAN_FE_INTR_ENABLE){
			mask |= (1<<WAN_DS_REGBITMAP(fe));
		}else{
			mask &= ~(1<<WAN_DS_REGBITMAP(fe));
		}
		WRITE_REG(REG_GFIMR, mask);

		mask = READ_REG(REG_GLIMR);
		if (mode == WAN_FE_INTR_ENABLE){
			mask |= (1<<WAN_DS_REGBITMAP(fe));
		}else{
			mask &= ~(1<<WAN_DS_REGBITMAP(fe));
		}
		WRITE_REG(REG_GLIMR, mask);
		
		mask = READ_REG(REG_GBIMR);
		if (mode == WAN_FE_INTR_ENABLE){
			mask |= (1<<WAN_DS_REGBITMAP(fe));
		}else{
			mask &= ~(1<<WAN_DS_REGBITMAP(fe));
		}
		WRITE_REG(REG_GBIMR, mask);
	}

	if (type & WAN_TE_INTR_BASIC){
		if (mode == WAN_FE_INTR_ENABLE){
			unsigned char	mask = 0x00;

			mask = 	BIT_RIM1_RAISC | BIT_RIM1_RAISD|
				BIT_RIM1_RRAIC | BIT_RIM1_RRAID;
			if (WAN_FE_FRAME(fe) != WAN_FR_UNFRAMED){
				mask |= (BIT_RIM1_RLOFC | BIT_RIM1_RLOFD);
			}
#if defined(FE_LOS_ENABLE)
			mask |= (BIT_RIM1_RLOSC | BIT_RIM1_RLOSD);
#endif
			WRITE_REG(REG_RIM1, mask);
			WRITE_REG(REG_RLS1, 0xFF);

			/* In-band loop codes */
			if (IS_T1_FEMEDIA(fe)){
    				mask = BIT_RIM3_T1_LDNC|BIT_RIM3_T1_LDND |
				BIT_RIM3_T1_LUPC|BIT_RIM3_T1_LUPD;
				WRITE_REG(REG_RIM3, mask);
                		WRITE_REG(REG_RLS3, 0xFF);
            		}

#ifdef WAN_RIM4_TIMER
#warning "WAN_RIM4_TIMER Enabled"
			WRITE_REG(REG_RIM4, BIT_RIM4_TIMER);
#endif
			WRITE_REG(REG_RLS4, 0xFF);
			
			if (IS_T1_FEMEDIA(fe)){
                		mask = BIT_RIM7_T1_BC | BIT_RIM7_T1_BD;
                		/* mask |= BIT_RIM7_T1_RSLC96; */ 			 
    				WRITE_REG(REG_RIM7, mask);
    			}
			WRITE_REG(REG_RLS7, 0xFF);

			WRITE_REG(REG_LSIMR,
				BIT_LSIMR_OCCIM | BIT_LSIMR_OCDIM |
				BIT_LSIMR_SCCIM | BIT_LSIMR_SCCIM |
				BIT_LSIMR_LOSCIM | BIT_LSIMR_LOSDIM |
				BIT_LSIMR_JALTCIM | BIT_LSIMR_JALTSIM);
			WRITE_REG(REG_LLSR, 0xFF);
		}else{
			WRITE_REG(REG_RIM1, 0x00);
			WRITE_REG(REG_RIM2, 0x00);
			WRITE_REG(REG_RIM3, 0x00);
			WRITE_REG(REG_RIM4, 0x00);
			WRITE_REG(REG_RIM5, 0x00);
			WRITE_REG(REG_RIM7, 0x00);

			WRITE_REG(REG_TIM1, 0x00);
			WRITE_REG(REG_TIM2, 0x00);
			WRITE_REG(REG_TIM3, 0x00);

			WRITE_REG(REG_LSIMR, 0x00);
		}
	}

	if (type & WAN_TE_INTR_SIGNALLING){
		for(ch = 1; ch <= fe->te_param.max_channels; ch++){
			if (!wan_test_bit(ch, &ts_map)){
				continue;
			}
			if (IS_T1_FEMEDIA(fe)){
				bit = (ch-1) % 8;
				off = (ch-1) / 8;
			}else{
				if (ch == 16) continue;
				bit = ch % 8;
				off = ch / 8;
			}
			rscse = READ_REG(REG_RSCSE1+off);
			if (mode == WAN_FE_INTR_ENABLE){
				rscse |= (1<<bit);
			}else{
				rscse &= ~(1<<bit);
			}
			DEBUG_TE1("%s: Ch %d RSCSE=%02X Val=%02X\n",
					fe->name, ch, REG_RSCSE1+off, rscse);
			WRITE_REG(REG_RSCSE1+off, rscse);
		}
		value = READ_REG(REG_RIM4);
		if (mode == WAN_FE_INTR_ENABLE){
			value |= BIT_RIM4_RSCOS;
		}else{
			value &= ~BIT_RIM4_RSCOS;
		}
		WRITE_REG(REG_RIM4, value);
	}

	if (type & WAN_TE_INTR_PMON){
		value = READ_REG(REG_ERCNT);
		if (mode == WAN_FE_INTR_ENABLE){
			value &= ~BIT_ERCNT_EAMS;
		}else{
			value |= BIT_ERCNT_EAMS;
		}
		WRITE_REG(REG_ERCNT, value);
	}
 
	return 0;
}


static int sdla_ds_te1_fr_rxintr_rls1(sdla_fe_t *fe, int silent) 
{
	unsigned char	rim1 = READ_REG(REG_RIM1);
	unsigned char	rls1 = READ_REG(REG_RLS1);
	unsigned char	rrts1 = READ_REG(REG_RRTS1);

	//if (WAN_FE_FRAME(fe) != WAN_FR_UNFRAMED){
		if (rls1 & (BIT_RLS1_RRAIC|BIT_RLS1_RRAID)){
			if (rrts1 & BIT_RRTS1_RRAI){
				fe->fe_alarm |= WAN_TE_BIT_ALARM_RAI;		
				if (!silent) DEBUG_EVENT("%s: RAI alarm is ON\n",
							fe->name);
			}else{
				fe->fe_alarm &= ~WAN_TE_BIT_ALARM_RAI;		
				if (!silent) DEBUG_EVENT("%s: RAI alarm is OFF\n",
							fe->name);
			}
		}
	//}
        if (rim1 & (BIT_RIM1_RAISC|BIT_RIM1_RAISD)){
		if (rls1 & (BIT_RLS1_RAISC|BIT_RLS1_RAISD)){
			if (rrts1 & BIT_RRTS1_RAIS){

				sdla_ds_te1_swirq_trigger(
						fe,
						WAN_TE1_SWIRQ_TYPE_ALARM_AIS,
						WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON,
						WAN_T1_ALARM_THRESHOLD_AIS_ON);
			}else{
				sdla_ds_te1_swirq_trigger(
						fe,
						WAN_TE1_SWIRQ_TYPE_ALARM_AIS,
						WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF,
						WAN_T1_ALARM_THRESHOLD_AIS_OFF);
			}
		}
	}

        if (rim1 & (BIT_RIM1_RLOSC|BIT_RIM1_RLOSD)){
    		if (rls1 & (BIT_RLS1_RLOSC|BIT_RLS1_RLOSD)){
			if (rrts1 & BIT_RRTS1_RLOS){
				sdla_ds_te1_swirq_trigger(
						fe,
						WAN_TE1_SWIRQ_TYPE_ALARM_LOS,
						WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON,
						WAN_T1_ALARM_THRESHOLD_LOS_ON);
			}else{
				sdla_ds_te1_swirq_trigger(
						fe,
						WAN_TE1_SWIRQ_TYPE_ALARM_LOS,
						WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF,
						WAN_T1_ALARM_THRESHOLD_LOS_OFF);
			}
    		}
    	}
	if (WAN_FE_FRAME(fe) != WAN_FR_UNFRAMED){
		if (rim1 & (BIT_RIM1_RLOFC | BIT_RIM1_RLOFD)){
			if (rls1 & (BIT_RLS1_RLOFC|BIT_RLS1_RLOFD)){
				if (rrts1 & BIT_RRTS1_RLOF){

					sdla_ds_te1_swirq_trigger(
							fe,
							WAN_TE1_SWIRQ_TYPE_ALARM_LOF,
							WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON,
							WAN_T1_ALARM_THRESHOLD_LOF_ON);
							
				}else{

					if (fe->fe_status == FE_CONNECTED){
						sdla_t 	*card = (sdla_t*)fe->card;
						if (card && card->wandev.te_link_reset){
							card->wandev.te_link_reset(card);
						}
					}
					
					sdla_ds_te1_swirq_trigger(
							fe,
							WAN_TE1_SWIRQ_TYPE_ALARM_LOF,
							WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF,
							WAN_T1_ALARM_THRESHOLD_LOF_OFF);
				}
			}
		}    			
	}
	WRITE_REG(REG_RLS1, rls1);
	return 0;
}

static int sdla_ds_te1_fr_rx_intr(sdla_fe_t *fe, int silent) 
{
	unsigned char	istatus;

	istatus = READ_REG(REG_RIIR);

	if (istatus & BIT_RIIR_RLS1){
		sdla_ds_te1_fr_rxintr_rls1(fe, silent); 
	}
	if (istatus & BIT_RIIR_RLS2){
		unsigned char	rls2 = READ_REG(REG_RLS2);
		if (!silent) DEBUG_TE1("%s: E1 RX Latched Status Register 2 %02X\n",
						fe->name, rls2);
		if (IS_T1_FEMEDIA(fe)){
			if (rls2 & BIT_RLS2_T1_SEFE){
				if (!silent) DEBUG_EVENT(
				"%s: Receive Errored Framing Bits (2/6)!\n",
						fe->name);
			}
			if (rls2 & BIT_RLS2_T1_FBE){
				if (!silent) DEBUG_EVENT(
				"%s: Receive Framing Bit error!\n",
						fe->name);
			}
		}else{
			if (rls2 & BIT_RLS2_E1_RSA1){
				if (!silent) DEBUG_EVENT(
				"%s: Receive Signalling All Ones Event!\n",
						fe->name);
			}
			if (rls2 & BIT_RLS2_E1_RSA0){
				if (!silent) DEBUG_EVENT(
				"%s: Receive Signalling All Ones Event!\n",
						fe->name);
			}
			if (rls2 & BIT_RLS2_E1_RCMF){
				if (!silent) DEBUG_EVENT(
				"%s: Receive CRC4 Multiframe Event!\n",
						fe->name);
			}
			if (rls2 & BIT_RLS2_E1_RAF){
				u8	rnaf = READ_REG(REG_E1RNAF);
 
				if (!silent) DEBUG_EVENT(
				"%s: Receive Align Frame Event!\n",
						fe->name);
				if (rnaf & BIT_E1RNAF_A){
					fe->fe_alarm |= WAN_TE_BIT_ALARM_RAI;		
					if (!silent) DEBUG_EVENT("%s: RAI alarm is ON\n",
								fe->name);
				}else{
					fe->fe_alarm &= ~WAN_TE_BIT_ALARM_RAI;		
					if (!silent) DEBUG_EVENT("%s: RAI alarm is OFF\n",
								fe->name);
				}
			}
		}
		WRITE_REG(REG_RLS2, rls2);
	}
	if (istatus & BIT_RIIR_RLS3){
		unsigned char	rls3 = READ_REG(REG_RLS3);
		unsigned char	rrts3 = READ_REG(REG_RRTS3);
		if (!silent) DEBUG_TE1("%s: RX Latched Status Register 3 %02X\n",
					fe->name, rls3);
		if (IS_T1_FEMEDIA(fe)){

			if (rls3 & BIT_RLS3_T1_LORCC){
				if (!silent) DEBUG_EVENT(
				"%s: Loss of Receive Clock Condition Clear!\n",
						fe->name);
			}
			if (rls3 & BIT_RLS3_T1_LORCD){
				if (!silent) DEBUG_EVENT(
				"%s: Loss of Receive Clock Condition Detect!\n",
						fe->name);
			}
			if (rls3 & (BIT_RLS3_T1_LUPD|BIT_RLS3_T1_LUPC)){
				if (rrts3 & BIT_RRTS3_T1_LUP){
					if (!silent) DEBUG_EVENT(
					"%s: Loop-Up Code Detected Condition Detect!\n",
							fe->name);
					sdla_ds_te1_set_lb(
						fe, 
						WAN_TE1_LINELB_MODE,WAN_TE1_LB_ENABLE, 
						ENABLE_ALL_CHANNELS); 

				}else{
					if (!silent) DEBUG_TE1(
					"%s: Loop-Up Code Detected Condition Clear!\n",
							fe->name);
				}
			}
			if (rls3 & (BIT_RLS3_T1_LDND|BIT_RLS3_T1_LDNC)){
				if (rrts3 & BIT_RRTS3_T1_LDN){
					if (!silent) DEBUG_EVENT(
					"%s: Loop-Down Code Detected Condition Detect!\n",
							fe->name);
					sdla_ds_te1_set_lb(
						fe, 
						WAN_TE1_LINELB_MODE,WAN_TE1_LB_DISABLE, 
						ENABLE_ALL_CHANNELS); 
				}else{
					if (!silent) DEBUG_TE1(
					"%s: Loop-Down Code Detected Condition Clear!\n",
							fe->name);
				}
			}
		}else{
			if (rls3 & BIT_RLS3_E1_LORCC){
				if (!silent) DEBUG_EVENT(
				"%s: Loss of Receive Clock Condition Clear!\n",
						fe->name);
			}
			if (rls3 & BIT_RLS3_E1_LORCD){
				if (!silent) DEBUG_EVENT(
				"%s: Loss of Receive Clock Condition Detect!\n",
						fe->name);
			}
		}
		WRITE_REG(REG_RLS3, rls3);
	}
	if (istatus & BIT_RIIR_RLS4){
		unsigned char	rls4 = READ_REG(REG_RLS4);
		if (!silent) DEBUG_TE1("%s: RX Latched Status Register 4 %02X\n",
					fe->name, rls4);
		if (rls4 & BIT_RLS4_RSCOS){
			if (!silent) DEBUG_EVENT(
			"%s: Receive Signalling status changed!\n",
					fe->name);
			sdla_ds_te1_check_rbsbits(fe, 1, ENABLE_ALL_CHANNELS, 1); 
			sdla_ds_te1_check_rbsbits(fe, 9, ENABLE_ALL_CHANNELS, 1);
			sdla_ds_te1_check_rbsbits(fe, 17, ENABLE_ALL_CHANNELS, 1);
			if (IS_E1_FEMEDIA(fe)){
				sdla_ds_te1_check_rbsbits(fe, 25, ENABLE_ALL_CHANNELS, 1);
			}
		}
		if (rls4 & BIT_RLS4_TIMER){
			if (!silent) DEBUG_ISR(
			"%s: Performance monitor counters have been updated!\n",
					fe->name);
			//sdla_ds_te1_pmon(fe, WAN_FE_PMON_READ);
		}
		WRITE_REG(REG_RLS4, rls4);
	}
	if (istatus & BIT_RIIR_RLS5){
		unsigned char	rls5 = READ_REG(REG_RLS5);
		if (!silent) DEBUG_TE1("%s: RX Latched Status Register 5 %02X\n",
					fe->name, rls5);
		if (rls5 & BIT_RLS5_ROVR){
			if (!silent) DEBUG_EVENT("%s: Receive FIFO overrun (HDLC)!\n",
					fe->name);
		}					
		if (rls5 & BIT_RLS5_RHOBT){
			if (!silent) DEBUG_EVENT("%s: Receive HDLC Opening Byte Event (HDLC)!\n",
					fe->name);
		}					
		if (rls5 & BIT_RLS5_RPE){
			if (!silent) DEBUG_EVENT("%s: Receive Packet End Event (HDLC)!\n",
					fe->name);
		}					
		if (rls5 & BIT_RLS5_RPS){
			if (!silent) DEBUG_EVENT("%s: Receive Packet Start Event (HDLC)!\n",
					fe->name);
		}					
		if (rls5 & BIT_RLS5_RHWMS){
			if (!silent) DEBUG_EVENT("%s: Receive FIFO Above High Watermark Set Event (HDLC)!\n",
					fe->name);
		}					
		if (rls5 & BIT_RLS5_RNES){
			if (!silent) DEBUG_EVENT("%s: Receive FIFO Not Empty Set Event (HDLC)!\n",
					fe->name);
		}					
		WRITE_REG(REG_RLS5, rls5);
	}
#if 0	
	if (istatus & BIT_RIIR_RLS6){
	}
#endif	
	if (istatus & BIT_RIIR_RLS7){
		unsigned char	rls7 = READ_REG(REG_RLS7);
		
		if (IS_T1_FEMEDIA(fe)){
			if (!silent) DEBUG_TE1("%s: RX Latched Status Register 7 %02X\n",
						fe->name, rls7);
			if (rls7 & BIT_RLS7_T1_RRAI_CI){
				if (!silent) DEBUG_EVENT("%s: Receive RAI-CI Detect!\n",
						fe->name);
			}					
			if (rls7 & BIT_RLS7_T1_RAIS_CI){
				if (!silent) DEBUG_EVENT("%s: Receive RAI-CI Detect!\n",
						fe->name);
			}					
			if (rls7 & BIT_RLS7_T1_RSLC96){
				if (!silent) DEBUG_EVENT("%s: Receive SLC-96 Alignment Event!\n",
						fe->name);
			}					
			if (rls7 & BIT_RLS7_T1_RFDLF){
				if (!silent) DEBUG_TE1("%s: Receive FDL Register Full Event!\n",
						fe->name);
			}					
			if (rls7 & BIT_RLS7_T1_BC){
				if (!silent) DEBUG_TE1("%s: BOC Clear Event!\n",
						fe->name);
				sdla_ds_te1_boc(fe, 0);
			}					
			if (rls7 & BIT_RLS7_T1_BD){
				if (!silent) DEBUG_TE1("%s: BOC Detect Event!\n",
						fe->name);
				sdla_ds_te1_boc(fe, 1);
			}
		}else{
			if (rls7 & BIT_RLS7_E1_Sa6CD){
				if (!silent) DEBUG_EVENT("%s: Sa6 Codeword detect!\n",
						fe->name);
				sdla_ds_e1_sa6code(fe);
			}					
			if (rls7 & BIT_RLS7_E1_SaXCD){
				if (!silent) DEBUG_EVENT("%s: SaX bit change detect!\n",
						fe->name);
				sdla_ds_e1_sabits(fe);
			}
		}					
		WRITE_REG(REG_RLS7, rls7);
	}
	
	return 0;
}

static int sdla_ds_te1_fr_tx_intr(sdla_fe_t *fe, int silent) 
{
	unsigned char	istatus;
	unsigned char	status;

	istatus = READ_REG(REG_TIIR);
	if (istatus & BIT_TIIR_TLS1){
		status = READ_REG(REG_TLS1);
		if (status & BIT_TLS1_TPDV){
			if (!silent) DEBUG_EVENT(
			"%s: Transmit Pulse Density Violation Event!\n",
					fe->name);
		}
		if (status & BIT_TLS1_LOTCC){
			if (!silent) DEBUG_EVENT(
			"%s: Loss of Transmit Clock condition Clear!\n",
					fe->name);
		}
		WRITE_REG(REG_TLS1, status);
	}
	if (istatus & BIT_TIIR_TLS2){
		status = READ_REG(REG_TLS2);
		WRITE_REG(REG_TLS2, status);
	}
	if (istatus & BIT_TIIR_TLS3){
		status = READ_REG(REG_TLS3);
		WRITE_REG(REG_TLS3, status);
	}
	return 0;
}

static int sdla_ds_te1_liu_intr(sdla_fe_t *fe, int silent) 
{
	unsigned char	lsimr = READ_REG(REG_LSIMR);
	unsigned char	llsr = READ_REG(REG_LLSR);
	unsigned char	lrsr = READ_REG(REG_LRSR);

	if (llsr & BIT_LLSR_JALTC && lsimr & BIT_LSIMR_JALTCIM){
		if (!silent) DEBUG_TE1("%s: Jitter Attenuator Limit Trip Clear!\n",
					fe->name);
	}
	if (llsr & BIT_LLSR_JALTS && lsimr & BIT_LSIMR_JALTSIM){
		if (!silent) DEBUG_TE1("%s: Jitter Attenuator Limit Trip Set!\n",
					fe->name);
	}
	if (llsr & (BIT_LLSR_OCC | BIT_LLSR_OCD)){
#if 0	
		if (IS_T1_FEMEDIA(fe) && WAN_TE1_LBO(fe) == LBO 5/6/7){
			/* Use LRSR register to get real value. LLSR is not
			** valid for these modes */
		}
#endif
		if (lrsr & BIT_LRSR_OCS){
			if (!(fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_OC)){
				if (!silent) DEBUG_TE1("%s: Open Circuit is detected!\n",
					fe->name);
				fe->fe_alarm |= WAN_TE_BIT_ALARM_LIU_OC;
			}
		}else{
			if (fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_OC){
				if (!silent) DEBUG_TE1("%s: Open Circuit is cleared!\n",
					fe->name);
				fe->fe_alarm &= ~WAN_TE_BIT_ALARM_LIU_OC;
			}
		}
	}
	if (llsr & (BIT_LLSR_SCC | BIT_LLSR_SCD)){
		if (lrsr & BIT_LRSR_SCS){
			if (!(fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_SC)){
				if (!silent) DEBUG_EVENT("%s: Short Circuit is detected!(%i)\n",
					fe->name, __LINE__);
				fe->fe_alarm |= WAN_TE_BIT_ALARM_LIU_SC;
			}
		}else{
			if (fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_SC){
				if (!silent) DEBUG_EVENT("%s: Short Circuit is cleared!(%i)\n",
					fe->name, __LINE__);
				fe->fe_alarm &= ~WAN_TE_BIT_ALARM_LIU_SC;	
			}
		}
	}
    if (lsimr & (BIT_LSIMR_LOSDIM|BIT_LSIMR_LOSCIM)){
    	if (llsr & (BIT_LLSR_LOSC | BIT_LLSR_LOSD)){
    		if (lrsr & BIT_LRSR_LOSS){
    			if (!(fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_LOS)){
    				if (!silent) DEBUG_EVENT("%s: Lost of Signal is detected!\n",
    					fe->name);
    				fe->fe_alarm |= WAN_TE_BIT_ALARM_LIU_LOS;
    			}
    		}else{
    			if (fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_LOS){
    				if (!silent) DEBUG_EVENT("%s: Lost of Signal is cleared!\n",
    					fe->name);
    				fe->fe_alarm &= ~WAN_TE_BIT_ALARM_LIU_LOS;	
    			}
    		}
    	}
    }
	WRITE_REG(REG_LLSR, llsr);
	return 0;
}

static int sdla_ds_te1_bert_intr(sdla_fe_t *fe, int silent) 
{
	unsigned char	blsr = READ_REG(REG_BLSR);
	unsigned char	bsim = READ_REG(REG_BSIM);
	
	if (blsr & BIT_BLSR_BBED && bsim & BIT_BSIM_BBED){
		if (!silent && WAN_NET_RATELIMIT())
			DEBUG_ERROR("%s: BERT bit error detected!\n",
					fe->name);
	}
	if (blsr & BIT_BLSR_BBCO && bsim & BIT_BSIM_BBCO){
		if (!silent) DEBUG_EVENT("%s: BERT Bit Counter overflows!\n",
					fe->name);
	}
	if (blsr & BIT_BLSR_BECO && bsim & BIT_BSIM_BECO){
		if (!silent) DEBUG_ERROR("%s: BERT Error Counter overflows!\n",
					fe->name);
	}
	if (blsr & BIT_BLSR_BRA1 && bsim & BIT_BSIM_BRA1){
		if (!silent) DEBUG_EVENT("%s: BERT Receive All-Ones Condition!\n",
					fe->name);
	}
	if (blsr & BIT_BLSR_BRA0 && bsim & BIT_BSIM_BRA0){
		if (!silent) DEBUG_EVENT("%s: BERT Receive All-Zeros Condition!\n",
					fe->name);	
	}
	if (blsr & BIT_BLSR_BRLOS && bsim & BIT_BSIM_BRLOS){
		if (!silent && WAN_NET_RATELIMIT())
			DEBUG_EVENT("%s: BERT Receive Loss of Synchronization Condition!\n",
					fe->name);
		wan_clear_bit(WAN_TE_BERT_FLAG_INLOCK, &fe->te_param.bert_flag);
	}
	if (blsr & BIT_BLSR_BSYNC && bsim & BIT_BSIM_BSYNC){
		if (!silent && WAN_NET_RATELIMIT())
			DEBUG_EVENT("%s: BERT in synchronization Condition!\n",
					fe->name);	
		wan_set_bit(WAN_TE_BERT_FLAG_INLOCK, &fe->te_param.bert_flag);
	}

	WRITE_REG(REG_BLSR, blsr);
	return 0;
}

static int sdla_ds_te1_check_intr(sdla_fe_t *fe) 
{
	unsigned char	framer_istatus, framer_imask;
	unsigned char	liu_istatus, liu_imask;
	unsigned char	bert_istatus, bert_imask;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	WAN_ASSERT(fe->fe_cfg.poll_mode == WANOPT_YES);

	framer_istatus	= __READ_REG(REG_GFISR);
	framer_imask	= __READ_REG(REG_GFIMR);

	//if (framer_istatus & (1 << WAN_FE_LINENO(fe))){
	if ((framer_istatus & (1 << WAN_DS_REGBITMAP(fe))) && 
	    (framer_imask & (1 << WAN_DS_REGBITMAP(fe)))) {
		DEBUG_ISR("%s: Interrupt for line %d (FRAMER)\n",
					fe->name, WAN_FE_LINENO(fe));
		return 1;
	}
	liu_istatus	= __READ_REG(REG_GLISR);
	liu_imask	= __READ_REG(REG_GLIMR);
	//if (liu_istatus & (1 << WAN_FE_LINENO(fe))){
	if ((liu_istatus & (1 << WAN_DS_REGBITMAP(fe))) &&
	    (liu_imask & (1 << WAN_DS_REGBITMAP(fe)))) { 
		DEBUG_ISR("%s: Interrupt for line %d (LIU)\n",
					fe->name, WAN_FE_LINENO(fe));
		return 1;
	}
	bert_istatus	= __READ_REG(REG_GBISR);
	bert_imask	= __READ_REG(REG_GBIMR);	
	//if (bert_istatus & (1 << WAN_FE_LINENO(fe))){
	if ((bert_istatus & (1 << WAN_DS_REGBITMAP(fe))) &&
	    (bert_imask & (1 << WAN_DS_REGBITMAP(fe)))) {
		DEBUG_ISR("%s: Interrupt for line %d (BERT)\n",
					fe->name, WAN_FE_LINENO(fe));
		return 1;
	}
	DEBUG_ISR("%s: This interrupt not for this port %d\n",
				fe->name,
				WAN_FE_LINENO(fe)+1);
	return 0;
}

static int sdla_ds_te1_intr(sdla_fe_t *fe) 
{
	u_int8_t	framer_istatus, liu_istatus, bert_istatus; 
	u_int8_t	device_id;
	int		silent = 0;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	device_id = WAN_TE1_DEVICE_ID;
	if (device_id == DEVICE_ID_BAD){
		DEBUG_EVENT(
		"%s: ERROR: Failed to verify Device id (silent mode)!\n",
				fe->name);
		silent = 1;
	}
	framer_istatus = READ_REG(REG_GFISR);
	liu_istatus = READ_REG(REG_GLISR);
	bert_istatus = READ_REG(REG_GBISR);

	//if (framer_istatus & (1 << WAN_FE_LINENO(fe))){
	if (framer_istatus & (1 << WAN_DS_REGBITMAP(fe))){
		sdla_ds_te1_fr_rx_intr(fe, silent);
		sdla_ds_te1_fr_tx_intr(fe, silent);
		//WRITE_REG(REG_GFISR, (1<<WAN_FE_LINENO(fe)));
		WRITE_REG(REG_GFISR, (1<<WAN_DS_REGBITMAP(fe)));
	}
	//if (liu_istatus & (1 << WAN_FE_LINENO(fe))){
	if (liu_istatus & (1 << WAN_DS_REGBITMAP(fe))){
		sdla_ds_te1_liu_intr(fe, silent);
		//WRITE_REG(REG_GLISR, (1<<WAN_FE_LINENO(fe)));
		WRITE_REG(REG_GLISR, (1<<WAN_DS_REGBITMAP(fe)));
	}
	//if (bert_istatus & (1 << WAN_FE_LINENO(fe))){
	if (bert_istatus & (1 << WAN_DS_REGBITMAP(fe))){
		sdla_ds_te1_bert_intr(fe, silent);
		//WRITE_REG(REG_GBISR, (1<<WAN_FE_LINENO(fe)));
		WRITE_REG(REG_GBISR, (1<<WAN_DS_REGBITMAP(fe)));
	}

	DEBUG_TE1("%s: FE Interrupt Alarms=0x%X\n",
			fe->name,fe->fe_alarm);

#if 1
	if (fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_SC){
		/* AL: March 1, 2006
		** 1. Mask global FE intr
		** 2. Disable automatic update */
		sdla_ds_te1_intr_ctrl(
			fe, 0,
			(WAN_TE_INTR_GLOBAL|WAN_TE_INTR_BASIC|WAN_TE_INTR_PMON),
			WAN_FE_INTR_MASK,
			0x00);
		/* Start LINKDOWN poll */
		sdla_ds_te1_swirq_trigger(
				fe,
				WAN_TE1_SWIRQ_TYPE_LINK,
				WAN_TE1_SWIRQ_SUBTYPE_LINKCRIT,
				POLLING_TE1_TIMER*5);
		return 0;
	}
#endif

	if (sdla_ds_te1_set_status(fe, fe->fe_alarm)){
		if (fe->fe_status != FE_CONNECTED){
			/* AL: March 1, 2006
			** 1. Mask global FE intr
			** 2. Disable automatic update */
			sdla_ds_te1_intr_ctrl(
				fe, 0,
				(WAN_TE_INTR_GLOBAL|WAN_TE_INTR_BASIC|WAN_TE_INTR_PMON),
				WAN_FE_INTR_MASK,
				0x00);
			/* Start LINKDOWN poll */
			sdla_ds_te1_swirq_trigger(
					fe,
					WAN_TE1_SWIRQ_TYPE_LINK,
					WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN,
					POLLING_TE1_TIMER*5);
		}
	}
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_timer()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void sdla_ds_te1_timer(void* pfe)
#elif defined(__WINDOWS__)
static void sdla_ds_te1_timer(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3)
#else
static void sdla_ds_te1_timer(unsigned long pfe)
#endif
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	sdla_t 		*card = (sdla_t*)fe->card;
	wan_device_t	*wandev = &card->wandev;
	wan_smp_flag_t	smp_flags;
	int		empty = 1;

	if (wan_test_bit(TE_TIMER_KILL,(void*)&fe->te_param.critical)){
		wan_clear_bit(TE_TIMER_RUNNING,(void*)&fe->te_param.critical);
		return;
	}
	if (!wan_test_bit(TE_TIMER_RUNNING,(void*)&fe->te_param.critical)){
		/* Somebody clear this bit */
		DEBUG_EVENT("WARNING: %s: Timer bit is cleared (should never happened)!\n", 
					fe->name);
		return;
	}
	wan_clear_bit(TE_TIMER_RUNNING,(void*)&fe->te_param.critical);
			
	/* Enable hardware interrupt for TE1 */
	wan_spin_lock_irq(&fe->lockirq,&smp_flags);
	empty = WAN_LIST_EMPTY(&fe->event);
	wan_spin_unlock_irq(&fe->lockirq,&smp_flags);

	if (!empty || fe->swirq_map){
		DEBUG_TEST("%s: TE1 timer!\n", fe->name);
		if (wan_test_and_set_bit(TE_TIMER_EVENT_PENDING,(void*)&fe->te_param.critical)){
			DEBUG_EVENT("%s: TE1 timer event is pending!\n", fe->name);
			return;
		}	
		if (wandev->fe_enable_timer){
			wandev->fe_enable_timer(fe->card);
		}else{
			sdla_ds_te1_polling(fe);
		}
	}else{
		sdla_ds_te1_add_timer(fe, POLLING_TE1_TIMER);
	}
	return;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_add_timer()	
 *
 * Description: Enable software timer interrupt.
 * Arguments: delay - (in Seconds!!)
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_add_timer(sdla_fe_t* fe, unsigned long delay)
{
	int	err=0;

	if (wan_test_bit(TE_TIMER_KILL,(void*)&fe->te_param.critical) ||
	    wan_test_bit(TE_TIMER_RUNNING,(void*)&fe->te_param.critical)) {
		return 0;
	}

	//err = wan_add_timer(&fe->timer, delay * HZ / 1000); //this is if 'delay' is in Ms
	err = wan_add_timer(&fe->timer, delay * HZ );

	if (err){
		/* Failed to add timer */
		return -EINVAL;
	}
	wan_set_bit(TE_TIMER_RUNNING,(void*)&fe->te_param.critical);
	return 0;	
}

/*
 ******************************************************************************
 *				sdla_ds_te1_add_event()	
 *
 * Description: Enable software timer interrupt in delay ms.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int
sdla_ds_te1_add_event(sdla_fe_t *fe, sdla_fe_timer_event_t *event)
{
	sdla_t			*card = (sdla_t*)fe->card;
	sdla_fe_timer_event_t	*tevent = NULL;
	wan_smp_flag_t		smp_flags;

	WAN_ASSERT_RC(card == NULL, -EINVAL);

	if (wan_test_bit(TE_TIMER_EVENT_INPROGRESS,(void*)&fe->te_param.critical)){
		DEBUG_EVENT("ADBG> %s: Event in progress...\n", fe->name);
		return -EINVAL;
	}
	
	/* Creating event timer */	
	tevent = wan_malloc(sizeof(sdla_fe_timer_event_t));
	if (tevent == NULL){
		DEBUG_EVENT(
       		"%s: Failed to allocate memory for timer event!\n",
					fe->name);
		return -EINVAL;
	}
	memcpy(tevent, event, sizeof(sdla_fe_timer_event_t));
	
#if 0	
	DEBUG_EVENT("%s: %s:%d: ---------------START ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
	WARN_ON(1);
	DEBUG_EVENT("%s: %s:%d: ---------------STOP ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
#endif	
	wan_spin_lock_irq(&fe->lockirq,&smp_flags);
	/* Set event from pending event map */
	if (wan_test_and_set_bit(event->type,(void*)&fe->event_map)){
		DEBUG_TE1("%s: WARNING: Event type %d is already pending!\n",
							fe->name, event->type);
		wan_spin_unlock_irq(&fe->lockirq, &smp_flags);
		wan_free(tevent);
		return -EINVAL;
	}
	tevent->start = SYSTEM_TICKS;
	if (WAN_LIST_EMPTY(&fe->event)){
		WAN_LIST_INSERT_HEAD(&fe->event, tevent, next);
	}else{
		sdla_fe_timer_event_t	*tmp;
		int			cnt = 0;
		WAN_LIST_FOREACH(tmp, &fe->event, next){
			if (!WAN_LIST_NEXT(tmp, next)) break;
			cnt ++;
		}
		if (tmp == NULL){
			DEBUG_ERROR("%s: ERROR: Internal Error!!!\n", fe->name);
			wan_clear_bit(event->type,(void*)&fe->event_map);
			wan_spin_unlock_irq(&fe->lockirq, &smp_flags);
			return -EINVAL;
		}
		if (cnt > WAN_FE_MAX_QEVENT_LEN){
			DEBUG_ERROR("%s: ERROR: Too many events in event queue!\n",
							fe->name);
			DEBUG_ERROR("%s: ERROR: Dropping new event type %d!\n",
							fe->name, event->type);
			wan_clear_bit(event->type,(void*)&fe->event_map);
			wan_spin_unlock_irq(&fe->lockirq, &smp_flags);
			wan_free(tevent);
			return -EINVAL;
		}
		WAN_LIST_INSERT_AFTER(tmp, tevent, next);
	}
	DEBUG_TEST("%s: Add new DS Event=0x%X (%d sec)\n",
			fe->name, event->type,event->delay);
	wan_spin_unlock_irq(&fe->lockirq, &smp_flags);
	return 0;
}

/******************************************************************************
**				sdla_ds_te1_swirq_link()	
**
** Description:
** Arguments:
** Returns:     0 - There are no more event. Do not need to schedule sw timer
**              number - delay to schedule next event.  
******************************************************************************/
static int sdla_ds_te1_swirq_link(sdla_fe_t* fe)
{
	sdla_t		*card = (sdla_t*)fe->card;
	sdla_fe_swirq_t	*swirq;
	int		subtype = 0, delay = 0;

	WAN_ASSERT(fe->swirq == NULL);
	swirq = &fe->swirq[WAN_TE1_SWIRQ_TYPE_LINK];

	if (!WAN_STIMEOUT(swirq->start, swirq->delay)){
		/* Timeout is not expired yet */
		DEBUG_TEST("%s: T1/E1 link swirq (%s) is not ready (%ld:%ld:%d)...\n",
				fe->name, WAN_TE1_SWIRQ_SUBTYPE_DECODE(swirq->subtype),
				swirq->start,SYSTEM_TICKS,swirq->delay*HZ);
		wan_set_bit(WAN_TE1_SWIRQ_TYPE_LINK, (void*)&fe->swirq_map);
		return 1;
	}
	DEBUG_TE1("%s: T1/E1 link swirq (%s)\n",
			fe->name, WAN_TE1_SWIRQ_SUBTYPE_DECODE(swirq->subtype));
	subtype = swirq->subtype;
	switch(subtype){
	case WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN:
		sdla_ds_te1_read_alarms(fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
		if (fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_SC){
			/* Short circuit detected, go to LINKCRIT state */
			subtype	= WAN_TE1_SWIRQ_SUBTYPE_LINKCRIT;
			delay	= POLLING_TE1_TIMER;
			break;
		}
		sdla_ds_te1_pmon(fe, WAN_FE_PMON_UPDATE|WAN_FE_PMON_READ);
		sdla_ds_te1_set_status(fe, fe->fe_alarm);
		if (fe->fe_status == FE_CONNECTED){
			subtype	= WAN_TE1_SWIRQ_SUBTYPE_LINKUP;
		}
		delay	= POLLING_TE1_TIMER;
		break;

	case WAN_TE1_SWIRQ_SUBTYPE_LINKCRIT:
		if (!sdla_ds_te1_read_crit_alarms(fe)){
			subtype = WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN;
		}
		delay	= POLLING_TE1_TIMER;
		break;

	case WAN_TE1_SWIRQ_SUBTYPE_LINKUP:
		/* ALEX: 
		** Do not update protocol front end state from TE_LINKDOWN_TIMER
		** because it cause to stay longer in interrupt handler
	        ** (critical for XILINX code) */
		if (fe->fe_status == FE_CONNECTED){
			if (card->wandev.te_link_state){
				card->wandev.te_link_state(card);
			}
			if (fe->fe_cfg.poll_mode == WANOPT_YES){
				subtype	= WAN_TE1_SWIRQ_SUBTYPE_LINKREADY;
				delay	= POLLING_TE1_TIMER;
			}else{
				/* Read alarm status before enabling fe interrupt */
				sdla_ds_te1_read_alarms(fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
				/* Enable Basic Interrupt
				** Enable automatic update pmon counters */
				sdla_ds_te1_intr_ctrl(	
					fe, 0, 
					(WAN_TE_INTR_GLOBAL|WAN_TE_INTR_BASIC|WAN_TE_INTR_PMON), 
					WAN_FE_INTR_ENABLE, 
					0x00);
				return 0;
			}
		}else{
			subtype	= WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN;
			delay	= POLLING_TE1_TIMER;
		}
		break;

	case WAN_TE1_SWIRQ_SUBTYPE_LINKREADY:
		/* Only used in no interrupt driven front-end mode */
		sdla_ds_te1_read_alarms(fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
		if (fe->fe_alarm & WAN_TE_BIT_ALARM_LIU_SC){
			/* Short circuit detected, go to LINKCRIT state */
			subtype	= WAN_TE1_SWIRQ_SUBTYPE_LINKCRIT;
			delay	= POLLING_TE1_TIMER;
			break;
		}
		sdla_ds_te1_set_status(fe, fe->fe_alarm);
		if (fe->fe_status != FE_CONNECTED){
			subtype	= WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN;
		}
		delay	= POLLING_TE1_TIMER;
		break;
	
	default:
		return -EINVAL;
	}
	/* SW irq is pending */
	wan_clear_bit(1, (void*)&swirq->pending);
	sdla_ds_te1_swirq_trigger(fe, WAN_TE1_SWIRQ_TYPE_LINK, subtype, delay); 
	return 1;
}

/******************************************************************************
**				sdla_ds_te1_swirq_alarm()	
**
** Description:
** Arguments:
** Returns:     0 - There are no more event. Do not need to schedule sw timer
**              number - delay to schedule next event.  
******************************************************************************/
static int sdla_ds_te1_swirq_alarm(sdla_fe_t* fe, int type)
{
	sdla_fe_swirq_t	*swirq;
	u_int32_t 	alarms = fe->fe_alarm;
	int		update = WAN_FALSE;

	WAN_ASSERT(fe->swirq == NULL);
	swirq = &fe->swirq[type];

	DEBUG_TEST("%s: %s swirq (%s)\n",
			fe->name,
			WAN_TE1_SWIRQ_TYPE_DECODE(type),
			WAN_TE1_SWIRQ_SUBTYPE_DECODE(swirq->subtype));

	if (swirq->subtype == WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON){

		if (WAN_STIMEOUT(swirq->start, swirq->delay)){
			/* Got real LOS alarm */
			if (type == WAN_TE1_SWIRQ_TYPE_ALARM_AIS){
				alarms |= WAN_TE_BIT_ALARM_AIS;
			}else if (type == WAN_TE1_SWIRQ_TYPE_ALARM_LOS){
				alarms |= WAN_TE_BIT_ALARM_LOS;
			}else if (type == WAN_TE1_SWIRQ_TYPE_ALARM_LOF){
				alarms |= WAN_TE_BIT_ALARM_LOF;
			}			
			wan_clear_bit(1, (void*)&swirq->pending);
			update = WAN_TRUE; 
		}else{
			DEBUG_TEST("%s: %s swirq (%s) is not ready (%ld:%ld:%d)...\n",
					fe->name,
					WAN_TE1_SWIRQ_TYPE_DECODE(type),
					WAN_TE1_SWIRQ_SUBTYPE_DECODE(swirq->subtype),
					swirq->start,SYSTEM_TICKS,swirq->delay*HZ);
			wan_set_bit(WAN_TE1_SWIRQ_TYPE_LINK, (void*)&fe->swirq_map);
			return 1;
		}
	
	}else if (swirq->subtype == WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF){

		if (WAN_STIMEOUT(swirq->start, swirq->delay)){
			/* Clearing LOS alarm */
			if (type == WAN_TE1_SWIRQ_TYPE_ALARM_AIS){
				alarms &= ~WAN_TE_BIT_ALARM_AIS;
			}else if (type == WAN_TE1_SWIRQ_TYPE_ALARM_LOS){
				alarms &= ~WAN_TE_BIT_ALARM_LOS;
			}else if (type == WAN_TE1_SWIRQ_TYPE_ALARM_LOF){
				alarms &= ~WAN_TE_BIT_ALARM_LOF;
			}			
			wan_clear_bit(1, (void*)&swirq->pending);
			update = WAN_TRUE; 
		}else{
			DEBUG_TEST("%s: %s swirq (%s) is not ready (%ld:%ld:%d)...\n",
					fe->name,
					WAN_TE1_SWIRQ_TYPE_DECODE(type),
					WAN_TE1_SWIRQ_SUBTYPE_DECODE(swirq->subtype),
					swirq->start,SYSTEM_TICKS,swirq->delay*HZ);
			wan_set_bit(WAN_TE1_SWIRQ_TYPE_LINK, (void*)&fe->swirq_map);
			return 1;
		}
	}

	if (update == WAN_TRUE){
		DEBUG_TEST("%s: %s swirq (%s) updating state ...\n",
					fe->name,
					WAN_TE1_SWIRQ_TYPE_DECODE(type),
					WAN_TE1_SWIRQ_SUBTYPE_DECODE(swirq->subtype));
		sdla_ds_te1_update_alarms(fe, alarms);
		if (sdla_ds_te1_set_status(fe, fe->fe_alarm)) {
			if (fe->fe_status == FE_CONNECTED) {
				sdla_ds_te1_swirq_trigger(
						fe, 
						WAN_TE1_SWIRQ_TYPE_LINK, 
						WAN_TE1_SWIRQ_SUBTYPE_LINKUP, 
						POLLING_TE1_TIMER);
			} else {
				sdla_ds_te1_swirq_trigger(
						fe, 
						WAN_TE1_SWIRQ_TYPE_LINK, 
						WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN, 
						POLLING_TE1_TIMER);
			}
			return 1;
		}
	}
	return 0;
}

/******************************************************************************
**				sdla_ds_te1_swirq_trigger()	
**
** Description:
** Arguments:
** Returns:     0 - There are no more event. Do not need to schedule sw timer
**              number - delay to schedule next event.  
******************************************************************************/
static int sdla_ds_te1_swirq_trigger(sdla_fe_t* fe, int type, int subtype, int delay)
{

	WAN_ASSERT(fe == NULL);
	WAN_ASSERT(fe->swirq == NULL);

	DEBUG_TE1("%s: Trigger %s swirq (%s) (%d:%d) ...\n",
				fe->name,
				WAN_TE1_SWIRQ_TYPE_DECODE(type),
				WAN_TE1_SWIRQ_SUBTYPE_DECODE(subtype),
				wan_test_bit(1, (void*)&fe->swirq[type].pending),
				fe->swirq[type].subtype);
	if (!wan_test_and_set_bit(1, (void*)&fe->swirq[type].pending)){
		/* new swirq */
		fe->swirq[type].subtype = (u8)subtype;
		fe->swirq[type].start = SYSTEM_TICKS;
	}else{
		/* already in a process */
		if (fe->swirq[type].subtype != subtype){
			fe->swirq[type].subtype = (u8)subtype;
			fe->swirq[type].start = SYSTEM_TICKS;
		}
	} 

	fe->swirq[type].delay = delay;	
	wan_set_bit(type, (void*)&fe->swirq_map);
	return 0;
}

/******************************************************************************
**				sdla_ds_te1_swirq()	
**
** Description:
** Arguments:
** Returns:     0 - There are no more event. Do not need to schedule sw timer
**              number - delay to schedule next event.  
******************************************************************************/
static int sdla_ds_te1_swirq(sdla_fe_t* fe)
{

	WAN_ASSERT(fe->swirq == NULL);
	fe->swirq_map = 0;
	if (wan_test_bit(1, (void*)&fe->swirq[WAN_TE1_SWIRQ_TYPE_LINK].pending)){
		sdla_ds_te1_swirq_link(fe);
	}

	if (wan_test_bit(1, (void*)&fe->swirq[WAN_TE1_SWIRQ_TYPE_ALARM_AIS].pending)){
		sdla_ds_te1_swirq_alarm(fe, WAN_TE1_SWIRQ_TYPE_ALARM_AIS);
	}

	if (wan_test_bit(1, (void*)&fe->swirq[WAN_TE1_SWIRQ_TYPE_ALARM_LOS].pending)){
		sdla_ds_te1_swirq_alarm(fe, WAN_TE1_SWIRQ_TYPE_ALARM_LOS);
	}

	if (wan_test_bit(1, (void*)&fe->swirq[WAN_TE1_SWIRQ_TYPE_ALARM_LOF].pending)){
		sdla_ds_te1_swirq_alarm(fe, WAN_TE1_SWIRQ_TYPE_ALARM_LOF);
	}
	return 0;
}

/******************************************************************************
**				sdla_ds_te1_polling()	
**
** Description:
** Arguments:
** Returns:     0 - There are no more event. Do not need to schedule sw timer
**              number - delay to schedule next event.  
******************************************************************************/
static int sdla_ds_te1_exec_event(sdla_fe_t* fe, sdla_fe_timer_event_t *event, uint8_t *pending)
{
	int err=0;
	
	*pending = 0;
	
	
	switch(event->type){
		case TE_RBS_READ:
			/* Physically read RBS status and print */
			err=sdla_ds_te1_rbs_print(fe, 0);
			break;

		case TE_SET_RBS:
			/* Set RBS bits */
			DEBUG_TE1("%s: Set ABCD bits (%X) for channel %d!\n",
					  fe->name,
					  event->te_event.rbs_abcd,
					  event->te_event.rbs_channel);
			err=sdla_ds_te1_set_rbsbits(fe, 
									event->te_event.rbs_channel,
									event->te_event.rbs_abcd);
			break;
	
		case TE_RBS_ENABLE:
		case TE_RBS_DISABLE:
			if (event->type == TE_RBS_ENABLE){
				err=sdla_ds_te1_rbs_ctrl(fe, event->te_event.ch_map, WAN_TRUE);
			}else{ 
				err=sdla_ds_te1_rbs_ctrl(fe, event->te_event.ch_map, WAN_FALSE);
			} 
			break;

		case TE_SET_LB_MODE:
			err=sdla_ds_te1_set_lb(fe, event->te_event.lb_type, event->mode, ENABLE_ALL_CHANNELS); 
			break;
				
		case TE_POLL_CONFIG:			
			DEBUG_EVENT("%s: Re-configuring %s Front-End chip...\n",
						fe->name, FE_MEDIA_DECODE(fe));
			if ((err=sdla_ds_te1_chip_config(fe))){
				DEBUG_EVENT("%s: Failed to re-configuring Front-End chip!\n",
							fe->name);
				break;
			}
			/* Do not enable interrupts here */
			event->type	= TE_LINKDOWN_TIMER;
			event->delay	= POLLING_TE1_TIMER;
			*pending = 1;
			
			break;
#if 0		
		case TE_POLL_CONFIG_VERIFY:
		DEBUG_EVENT("%s: Verifing %s Front-End chip configuration...\n",
						fe->name, FE_MEDIA_DECODE(fe));
		if (sdla_ds_te1_chip_config_verify(fe)){
			DEBUG_EVENT("%s: Failed to verify Front-End chip configuration!\n",
					fe->name);
	}
		break;
#endif		
		case TE_POLL_READ:
			fe->te_param.reg_dbg_value = READ_REG(event->te_event.reg);
			DEBUG_TE1("%s: Read %s Front-End Reg:%04X=%02X\n",
					  fe->name, FE_MEDIA_DECODE(fe),
					  event->te_event.reg,
					  fe->te_param.reg_dbg_value);
			fe->te_param.reg_dbg_ready = 1;		
#if 0
		DEBUG_EVENT("%s: Reading %s Front-End Registers:\n",
					fe->name, FE_MEDIA_DECODE(fe));
		for(reg=0x0;reg<=0x1ff;reg++){
			DEBUG_EVENT("%s: REG[%04X]=%02X\n",
						fe->name, reg, READ_REG(reg));
	}
		DEBUG_EVENT("%s: Reading %s Front-End LIU  Registers:\n",
					fe->name, FE_MEDIA_DECODE(fe));
		for(reg=0x1000;reg<=0x1007;reg++){
			DEBUG_EVENT("%s: REG[%04X]=%02X\n",
						fe->name, reg, READ_REG(reg));
	}
		DEBUG_EVENT("%s: Reading %s Front-End BERT Registers:\n",
					fe->name, FE_MEDIA_DECODE(fe));
		for(reg=0x1100;reg<=0x110F;reg++){
			DEBUG_EVENT("%s: REG[%04X]=%02X\n",
						fe->name, reg, READ_REG(reg));
	}
#endif		
		break;
		
		case TE_POLL_WRITE:
			DEBUG_EVENT("%s: Write %s Front-End Reg:%04X=%02X\n",
						fe->name, FE_MEDIA_DECODE(fe),
						event->te_event.reg,
						event->te_event.value);
			WRITE_REG(event->te_event.reg, event->te_event.value);
			break;

		case TE_LINELB_TIMER:		
			if (IS_T1_FEMEDIA(fe)){
				err=sdla_ds_te1_txlbcode_done(fe);
			}
			break;

		case WAN_TE_POLL_BERT:
			err=sdla_ds_te1_bert_read_status(fe);
			if (!err){
				*pending = 1;
			}
			break;

		default:
			DEBUG_EVENT("%s: Unknown DS TE1 Polling type %02X\n",
						fe->name, event->type);
			err=-1;
			break;
	}
	
	return err;
}

static int sdla_ds_te1_poll_events(sdla_fe_t* fe)
{
	sdla_fe_timer_event_t	*event;
	wan_smp_flag_t		smp_flags;
	u_int8_t	pending = 0;
	int err;
	
	wan_spin_lock_irq(&fe->lockirq,&smp_flags);
	if (WAN_LIST_EMPTY(&fe->event)){
		wan_spin_unlock_irq(&fe->lockirq,&smp_flags);
		DEBUG_EVENT("%s: WARNING: No FE events in a queue!\n",
					fe->name);
		sdla_ds_te1_add_timer(fe, POLLING_TE1_TIMER);
		return 0;
	}

	event = WAN_LIST_FIRST(&fe->event);
	WAN_LIST_REMOVE(event, next);
	/* Clear event from pending event map */
	wan_clear_bit(event->type,(void*)&fe->event_map);
	wan_spin_unlock_irq(&fe->lockirq,&smp_flags);
		
	DEBUG_TE1("%s: TE1 Polling event:0x%02X State=%s!\n", 
			fe->name, event->type, WAN_FE_STATUS_DECODE(fe));	

	err=sdla_ds_te1_exec_event(fe,event,&pending);
	
	/* Add new event */
	if (pending){
		sdla_ds_te1_add_event(fe, event);
	}
	wan_clear_bit(TE_TIMER_EVENT_PENDING,(void*)&fe->te_param.critical);
	if (event) wan_free(event);

	return 0;
}

/******************************************************************************
**				sdla_ds_te1_polling()	
**
** Description:
** Arguments:
** Returns:     0 - There are no more event. Do not need to schedule sw timer
**              number - delay to schedule next event.  
******************************************************************************/
static int sdla_ds_te1_polling(sdla_fe_t* fe)
{
	WAN_ASSERT(fe == NULL);

	if (wan_test_bit(TE_TIMER_KILL,(void*)&fe->te_param.critical)) {
		DEBUG_EVENT("%s:%s Device is shutting down ignoring poll!\n",
			fe->name, __FUNCTION__);
		return 0;
	}

	WAN_ASSERT_RC(fe->write_fe_reg == NULL, 0);
	WAN_ASSERT_RC(fe->read_fe_reg == NULL, 0);
	WAN_ASSERT(fe->swirq == NULL);


#if 0	
	DEBUG_EVENT("%s: %s:%d: ---------------START ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
	WARN_ON(1);
	DEBUG_EVENT("%s: %s:%d: ---------------STOP ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
#endif

	if (fe->swirq_map){
		sdla_ds_te1_swirq(fe);
	}

	if (!WAN_LIST_EMPTY(&fe->event)){
		sdla_ds_te1_poll_events(fe);
	}

#if 0
   	/* NC: This logic was used to turn off the AIS alarm after system
	       started up. We do not use this any more as we have found that
		   sending AIS on start causes stuck RAI alarm on some equipment 
    */
	if (fe->te_param.tx_ais_alarm && fe->te_param.tx_ais_startup_timeout) {
		if ((SYSTEM_TICKS - fe->te_param.tx_ais_startup_timeout) > HZ*2) {
			if (!(fe->fe_alarm & WAN_TE_BIT_ALARM_LOS)) {
				fe->te_param.tx_ais_startup_timeout=0;
				sdla_ds_te1_clear_alarms(fe,WAN_TE_BIT_ALARM_AIS);

				sdla_ds_te1_swirq_trigger(
						fe,
						WAN_TE1_SWIRQ_TYPE_ALARM_AIS,
						WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF,
						WAN_T1_ALARM_THRESHOLD_AIS_OFF);

			} 
		}
	}
#endif

		
	wan_clear_bit(TE_TIMER_EVENT_PENDING,(void*)&fe->te_param.critical);
	sdla_ds_te1_add_timer(fe, POLLING_TE1_TIMER);

	return 0;
}

/******************************************************************************
*				sdla_ds_te1_txlbcode_done()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_txlbcode_done(sdla_fe_t *fe)
{

	if (WAN_FE_FRAME(fe) == WAN_FR_D4){

		WRITE_REG(REG_TCR3, READ_REG(REG_TCR3) & ~BIT_TCR3_TLOOP);
		WRITE_REG(REG_RIM3, BIT_RIM3_T1_LDNC|BIT_RIM3_T1_LUPC|BIT_RIM3_T1_LDND|BIT_RIM3_T1_LUPD);

	}else if (WAN_FE_FRAME(fe) == WAN_FR_ESF){

		WRITE_REG(REG_THC2, READ_REG(REG_THC2) & ~BIT_THC2_SBOC);

	}else{
		return -EINVAL;
	}
	
	wan_clear_bit(fe->te_param.lb_tx_mode, &fe->te_param.lb_mode_map);

	DEBUG_TE1("%s: T1 %s loopback code sent.\n",
					fe->name,	
					WAN_TE1_BOC_LB_CODE_DECODE(fe->te_param.lb_tx_code));
	fe->te_param.lb_tx_cmd	= 0x00;
	fe->te_param.lb_tx_code = 0x00;
	wan_clear_bit(LINELB_WAITING,(void*)&fe->te_param.critical);
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_flush_pmon()	
 *
 * Description: Flush Dallas performance monitoring counters
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_flush_pmon(sdla_fe_t *fe)
{
	sdla_te_pmon_t	*pmon = &fe->fe_stats.te_pmon;
	sdla_ds_te1_pmon(fe, WAN_FE_PMON_UPDATE | WAN_FE_PMON_READ);
	pmon->lcv_errors	= 0;	
	pmon->oof_errors	= 0;
	pmon->bee_errors	= 0;
	pmon->crc4_errors	= 0;
	pmon->feb_errors	= 0;
	pmon->fas_errors	= 0;
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_pmon()	
 *
 * Description: Read DLS performance monitoring counters
 * Arguments:
 * Returns:
 ******************************************************************************
 */

#define VUC_FIELD(a, b, c) (((a) & (0xff >> (8 - (c)))) << (b)

static int sdla_ds_te1_pmon(sdla_fe_t *fe, int action)
{
	sdla_te_pmon_t	*pmon = &fe->fe_stats.te_pmon;
	u16		pmon1 = 0, pmon2 = 0, pmon3 = 0, pmon4 = 0;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if (IS_FE_PMON_UPDATE(action)){
		unsigned char	ercnt = READ_REG(REG_ERCNT);
		WRITE_REG(REG_ERCNT, ercnt & ~BIT_ERCNT_MECU);
		WP_DELAY(10);
		WRITE_REG(REG_ERCNT, ercnt | BIT_ERCNT_MECU);
		WP_DELAY(250);
		WRITE_REG(REG_ERCNT, ercnt);
	}

	if (IS_FE_PMON_READ(action)){
		pmon->mask = 0x00;
		/* Line code violation count register */	
		pmon1 = (READ_REG(REG_LCVCR1)&0xFF) << 8 | (READ_REG(REG_LCVCR2)&0xFF);
		/* Path code violation count for E1/T1 */
		pmon2 = (READ_REG(REG_PCVCR1)&0xFF) << 8 | (READ_REG(REG_PCVCR2)&0xFF);
		/* OOF Error for T1/E1 */
		pmon3 = READ_REG(REG_FOSCR1) << 8 | READ_REG(REG_FOSCR2);
		if (IS_E1_FEMEDIA(fe) && WAN_FE_FRAME(fe) == WAN_FR_CRC4){
			/* E-bit counter (Far End Block Errors) for CRC4 */
			pmon4 = READ_REG(REG_E1EBCR1) << 8 | READ_REG(REG_E1EBCR2);
		}

		pmon->lcv_diff = pmon1;
		pmon->lcv_errors = pmon->lcv_errors + pmon1;	
		if (IS_T1_FEMEDIA(fe)){
			pmon->mask =	WAN_TE_BIT_PMON_LCV |
					WAN_TE_BIT_PMON_BEE |
					WAN_TE_BIT_PMON_OOF;
			pmon->bee_diff = pmon2;
			pmon->bee_errors = pmon->bee_errors + pmon2;
			pmon->oof_diff = pmon3;
			pmon->oof_errors = pmon->oof_errors + pmon3;
		}else{
			pmon->mask =	WAN_TE_BIT_PMON_LCV |
					WAN_TE_BIT_PMON_CRC4 |
					WAN_TE_BIT_PMON_FAS |
					WAN_TE_BIT_PMON_FEB;
			if (fe->fe_chip_id == DEVICE_ID_DS26519) {
				if (WAN_FE_FRAME(fe) == WAN_FR_CRC4){
					pmon->crc4_diff = pmon2;
					pmon->crc4_errors = pmon->crc4_errors + pmon2;
				} else {
					pmon->crc4_diff = 0;
					pmon->crc4_errors = 0;
				}
			} else {
					pmon->crc4_diff = pmon2;
					pmon->crc4_errors = pmon->crc4_errors + pmon2;
			}
			pmon->fas_diff = pmon3;
			pmon->fas_errors = pmon->fas_errors + pmon3;
			pmon->feb_diff = pmon4;
			pmon->feb_errors = pmon->feb_errors + pmon4;
		}
	}
	if (IS_FE_PMON_PRINT(action)){
		DEBUG_EVENT("%s: Line Code Viilation:\t\t%d\n",
				fe->name, pmon->lcv_errors);
		if (IS_T1_FEMEDIA(fe)){
			DEBUG_ERROR("%s: Bit error events:\t\t%d\n",
					fe->name, pmon->bee_errors);
			DEBUG_EVENT("%s: Frames out of sync:\t\t%d\n",
					fe->name, pmon->oof_errors);
		}else{
			DEBUG_ERROR("%s: CRC4 errors:\t\t\t%d\n",
					fe->name, pmon->crc4_errors);
			DEBUG_ERROR("%s: Frame Alignment signal errors:\t%d\n",
					fe->name, pmon->fas_errors);
			DEBUG_ERROR("%s: Far End Block errors:\t\t%d\n",
					fe->name, pmon->feb_errors);
		}
	}
	return 0;
}

/******************************************************************************
*				sdla_ds_te1_boc()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_boc(sdla_fe_t *fe, int mode)
{
	unsigned char	boc, lb_type, lb_mode;

	boc = READ_REG(REG_T1RBOC);
	if (!mode){
		DEBUG_TE1("%s: BOC Clear Event (BOC=%02X)!\n",
					fe->name, boc);
		return 0;
	}

	DEBUG_TE1("%s: BOC Detect Event (BOC=%02X)!\n",
					fe->name, boc);
	switch(boc){
	case LINELB_ACTIVATE_CODE:
	case LINELB_DEACTIVATE_CODE:
	case PAYLB_ACTIVATE_CODE:
	case PAYLB_DEACTIVATE_CODE:
		if (boc == LINELB_ACTIVATE_CODE || boc == LINELB_DEACTIVATE_CODE){
			lb_type = WAN_TE1_LINELB_MODE;
			if (wan_test_bit(WAN_TE1_TX_LINELB_MODE, &fe->te_param.lb_mode_map)){
				return 0;	/* skip ack */
			}
		}else{
			lb_type = WAN_TE1_PAYLB_MODE;
			if (wan_test_bit(WAN_TE1_TX_PAYLB_MODE, &fe->te_param.lb_mode_map)){
				return 0;	/* skip ack */
			}
		}
		if (boc == LINELB_ACTIVATE_CODE || boc == PAYLB_ACTIVATE_CODE){
			lb_mode	= WAN_TE1_LB_ENABLE;
		}else{
			lb_mode = WAN_TE1_LB_DISABLE;
		}


		sdla_ds_te1_set_lb(fe, lb_type, lb_mode, ENABLE_ALL_CHANNELS);
		DEBUG_TE1("%s: Received T1 %s %s code from far end.\n", 
				fe->name, 
				WAN_TE1_LB_ACTION_DECODE(lb_mode),
				WAN_TE1_LB_MODE_DECODE(lb_type));
		break;

	case UNIVLB_DEACTIVATE_CODE:
		DEBUG_EVENT("%s: Received T1 %s code from far end.(LB Mode Map: %08X)\n", 
				fe->name, WAN_TE1_BOC_LB_CODE_DECODE(boc),fe->te_param.lb_mode_map);
		if (wan_test_bit(WAN_TE1_LINELB_MODE, &fe->te_param.lb_mode_map)){
			sdla_ds_te1_set_lb(fe, WAN_TE1_LINELB_MODE, WAN_TE1_LB_DISABLE, ENABLE_ALL_CHANNELS);
		}
		if (wan_test_bit(WAN_TE1_PAYLB_MODE, &fe->te_param.lb_mode_map)){
			sdla_ds_te1_set_lb(fe, WAN_TE1_PAYLB_MODE, WAN_TE1_LB_DISABLE, ENABLE_ALL_CHANNELS);
		}
		break;

	default:
		DEBUG_TE1("%s: Received Unsupport Bit-Oriented code %02X!\n", 
							fe->name, boc);
		break;
	}
	return 0;
}

/******************************************************************************
*				sdla_ds_e1_sabits()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_e1_sabits(sdla_fe_t* fe)
{
	u_int8_t	sabits = READ_REG(REG_SaBITS);

	DEBUG_EVENT("%s: Sa Bits: %d:%d:%d:%d:%d\n",
				fe->name,
				(sabits & BIT_SaBITS_Sa4) ? 1 : 0,
				(sabits & BIT_SaBITS_Sa5) ? 1 : 0,
				(sabits & BIT_SaBITS_Sa6) ? 1 : 0,
				(sabits & BIT_SaBITS_Sa7) ? 1 : 0,
				(sabits & BIT_SaBITS_Sa8) ? 1 : 0);
	return 0;
}
 
/******************************************************************************
*				sdla_ds_e1_sa6code()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_e1_sa6code(sdla_fe_t* fe)
{
	u_int8_t	sa6code = READ_REG(REG_Sa6CODE);

	switch(sa6code){
	case 0x08: case 0x04: case 0x02: case 0x01:
		DEBUG_EVENT("%s: Received Sa6_8 Sa6 codeword\n", fe->name);
		break;
	case 0x0A: case 0x05:
		DEBUG_EVENT("%s: Received Sa6_C Sa6 codeword\n", fe->name);
		break;
	case 0x06: case 0x03: case 0x09:
		DEBUG_EVENT("%s: Received Sa6_C Sa6 codeword\n", fe->name);
		break;
	case 0x0E: case 0x07: case 0x0B: case 0x0D:
		DEBUG_EVENT("%s: Received Sa6_E Sa6 codeword\n", fe->name);
		break;
	case 0x0F:
		DEBUG_EVENT("%s: Received Sa6_F Sa6 codeword\n", fe->name);
		break;
	default:
		DEBUG_EVENT("%s: Received unknown Sa6 codeword\n", fe->name);
		break;
	}
	DEBUG_EVENT("%s: Sa Bits: %d:%d:%d:%d:%d\n",
				fe->name,
				(sa6code & BIT_SaBITS_Sa4) ? 1 : 0,
				(sa6code & BIT_SaBITS_Sa5) ? 1 : 0,
				(sa6code & BIT_SaBITS_Sa6) ? 1 : 0,
				(sa6code & BIT_SaBITS_Sa7) ? 1 : 0,
				(sa6code & BIT_SaBITS_Sa8) ? 1 : 0);
	return 0;
}

/******************************************************************************
*				sdla_ds_te1_rxlevel()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_rxlevel(sdla_fe_t* fe) 
{
	int		index = 0;
	unsigned char	rxlevel;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	rxlevel = READ_REG(REG_LRSL);
	index = (rxlevel >> REG_LRSL_SHIFT) & REG_LRSL_MASK;
	
	memset(fe->fe_stats.u.te1_stats.rxlevel, 0, WAN_TE_RXLEVEL_LEN);
	if (IS_T1_FEMEDIA(fe)){
		memcpy(	fe->fe_stats.u.te1_stats.rxlevel,
			wan_t1_ds_rxlevel[index],
			strlen(wan_t1_ds_rxlevel[index]));
	}else{
		memcpy(	fe->fe_stats.u.te1_stats.rxlevel,
			wan_e1_ds_rxlevel[index],
			strlen(wan_e1_ds_rxlevel[index]));	
	}
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_liu_alb()	
 *
 * Description: check if port configuration allows to enable/disable Loop Back
 * Arguments:
 * Returns:	1 - yes it is ok to enable LB; 0 - it is not possible to enable LB
 ******************************************************************************
 */
static int sdla_is_loop_back_valid_for_clock_mode(sdla_fe_t* fe, unsigned char cmd)
{
	if (cmd == WAN_TE1_LB_ENABLE) {
		
		if (WAN_TE1_CLK(fe) == WAN_MASTER_CLK || WAN_TE1_REFCLK(fe)) {
			/* If Master clock OR referencing another port, which
			 * must be Master clock, it is ok to enable LB. */
			return 1;
		}else{
			/* can not enable LB */
			return 0;
		}
	} else {
		/* It is always ok to disable LB */
		return 1;
	}
}

/*
 ******************************************************************************
 *				sdla_ds_te1_liu_alb()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_liu_alb(sdla_fe_t* fe, unsigned char cmd) 
{
	unsigned char	value;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if (!sdla_is_loop_back_valid_for_clock_mode(fe, cmd)) {
		DEBUG_ERROR("%s: %s(): ERROR: You must be configured to Master Clock to execute this Loopback type!\n",
				fe->name, __FUNCTION__);
		return -EINVAL;
	}

	value = READ_REG(REG_LMCR);
	if (cmd == WAN_TE1_LB_ENABLE){
		value |= BIT_LMCR_ALB;
	}else{
		value &= ~BIT_LMCR_ALB;
	}
	WRITE_REG(REG_LMCR, value);
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_liu_llb()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_liu_llb(sdla_fe_t* fe, unsigned char cmd) 
{
	unsigned char	value;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if (!sdla_is_loop_back_valid_for_clock_mode(fe, cmd)) {
		DEBUG_ERROR("%s: %s(): ERROR: You must be configured to Master Clock to execute this Loopback type!\n",
				fe->name, __FUNCTION__);
		return -EINVAL;
	}

	value = READ_REG(REG_LMCR);
	if (cmd == WAN_TE1_LB_ENABLE){
		value |= BIT_LMCR_LLB;
	}else{
		value &= ~BIT_LMCR_LLB;
	}
	WRITE_REG(REG_LMCR, value);
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_liu_rlb()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_liu_rlb(sdla_fe_t* fe, unsigned char cmd) 
{
	unsigned char	value;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	value = READ_REG(REG_LMCR);
	if (cmd == WAN_TE1_LB_ENABLE){
		value |= BIT_LMCR_RLB;
	}else{
		value &= ~BIT_LMCR_RLB;
	}
	WRITE_REG(REG_LMCR, value);
	return 0;
}


#if 0

/*NC: March 31, 2010: replaced by sdla_ds_te1_liu_alb() 
  Function is still here for historical reasons */

/*
 ******************************************************************************
 *				sdla_ds_te1_fr_flb()	
 *
 * Description:
	Bit 0: Framer Loopback (FLB).	is it the "Local" LB??
		0 = loopback disabled
		1 = loopback enabled

		This loopback is useful in testing and debugging applications.
		In FLB, the DS26528 loops data from the transmit side back to the receive side.
		When FLB is enabled, the following will occur:
			1) (T1 mode) An unframed all-ones code will be transmitted at TTIP and TRING.
				(E1 mode) Normal data will be transmitted at TTIP and TRING.
			2) Data at RTIP and RRING will be ignored.
			3) All receive-side signals will take on timing synchronous with TCLK instead of RCLK.
			4) Note that it is not acceptable to have RCLK tied to TCLK during this loopback because this will cause an
				unstable condition.

 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_fr_flb(sdla_fe_t* fe, unsigned char cmd) 
{
	unsigned char	value;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if (!sdla_is_loop_back_valid_for_clock_mode(fe, cmd)) {
		DEBUG_ERROR("%s: %s(): ERROR: You must be configured to Master Clock to execute this Loopback type!\n",
				fe->name, __FUNCTION__);
		return -EINVAL;
	}

	value = READ_REG(REG_RCR3);
	if (cmd == WAN_TE1_LB_ENABLE){
		value |= BIT_RCR3_FLB;
	}else{
		value &= ~BIT_RCR3_FLB;
	}
	WRITE_REG(REG_RCR3, value);
	return 0;
}
#endif

/*
 ******************************************************************************
 *				sdla_ds_te1_fr_plb()
 *
 * Description:
	Bit 1: Payload Loopback (PLB).
		0 = loopback disabled
		1 = loopback enabled

	When PLB is enabled, the following will occur:
		1) Data will be transmitted from the TTIP and TRING pins synchronous with RCLK instead of TCLK.
		2) All the receive-side signals will continue to operate normally.
		3) The TCHCLK and TCHBLK signals are forced low.
		4) Data at the TSER, TDATA, and TSIG pins is ignored.
		5) The TLCLK signal will become synchronous with RCLK instead of TCLK.
			In a PLB situation, the DS26528 loops the 192 bits (248 for E1) of payload data (with BPVs corrected) from the
			receive section back to the transmit section. The transmitter follows the frame alignment provided by the receiver.
			The receive frame boundary is automatically fed into the transmit section, such that the transmit frame position is
			locked to the receiver (i.e., TSYNC is sourced from RSYNC). The FPS framing pattern, CRC-6 calculation, and the
			FDL bits (FAS word, Si, Sa, E-bits, and CRC-4 for E1) are not looped back. Rather, they are reinserted by the
		DS26528 (i.e., the transmit section will modify the payload as if it was input at TSER).

 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_fr_plb(sdla_fe_t* fe, unsigned char cmd) 
{
	unsigned char	value;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	value = READ_REG(REG_RCR3);
	if (cmd == WAN_TE1_LB_ENABLE){
		value |= BIT_RCR3_PLB;
	}else{
		value &= ~BIT_RCR3_PLB;
	}
	WRITE_REG(REG_RCR3, value);
	return 0;
}

/******************************************************************************
*				sdla_ds_te1_tx_lb()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int
sdla_ds_te1_tx_lb(sdla_fe_t* fe, u_int8_t type, u_int8_t mode) 
{
	sdla_fe_timer_event_t	fe_event;
	int			delay = 0;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if (!IS_T1_FEMEDIA(fe)){
		DEBUG_TE1("%s: ERROR: Unsupported Loopback type %s!\n",
				fe->name, WAN_TE1_LB_MODE_DECODE(type));
		return WAN_FE_LBMODE_RC_FAILED;
	}

	if (wan_test_bit(LINELB_WAITING,(void*)&fe->te_param.critical)){
		DEBUG_TE1("%s: Still waiting for far end to send loopback signal back!\n",
				fe->name);
		return WAN_FE_LBMODE_RC_TXBUSY;			
	}

	if (!sdla_is_loop_back_valid_for_clock_mode(fe, WAN_TE1_LB_ENABLE)) {
		DEBUG_ERROR("%s: %s(): ERROR: You must be configured to Master Clock to execute this Loopback type!\n",
				fe->name, __FUNCTION__);
		return WAN_FE_LBMODE_RC_FAILED;
	}

	fe->te_param.lb_tx_mode	= type;
	fe->te_param.lb_tx_cmd	= mode;
	if (type == WAN_TE1_TX_LINELB_MODE){
		if (mode == WAN_TE1_LB_ENABLE){
			fe->te_param.lb_tx_code = LINELB_ACTIVATE_CODE;
		}else{
			fe->te_param.lb_tx_code = LINELB_DEACTIVATE_CODE;
		}
	}else if (type == WAN_TE1_TX_PAYLB_MODE){
		if (mode == WAN_TE1_LB_ENABLE){
			fe->te_param.lb_tx_code = PAYLB_ACTIVATE_CODE;
		}else{
			fe->te_param.lb_tx_code = PAYLB_DEACTIVATE_CODE;
		}
	}
	DEBUG_TE1("%s: Sending T1 %s loopback code...\n",
		fe->name, WAN_TE1_BOC_LB_CODE_DECODE(fe->te_param.lb_tx_code));
	wan_set_bit(type, &fe->te_param.lb_mode_map);

	if (WAN_FE_FRAME(fe) == WAN_FR_ESF){

		delay = (WAN_T1_FDL_MSG_TIME * (WAN_T1_ESF_LINELB_TX_CNT + 1)) / 1000;

		/* Start BOC transmition */
		WRITE_REG(REG_T1TBOC, fe->te_param.lb_tx_code); 
		WRITE_REG(REG_THC2, READ_REG(REG_THC2) | BIT_THC2_SBOC);

	}else if (WAN_FE_FRAME(fe) == WAN_FR_D4){

		delay = (WAN_T1_FDL_MSG_TIME * (WAN_T1_D4_LINELB_TX_CNT + 1)) / 1000;

		WRITE_REG(REG_RIM3, READ_REG(REG_RIM3) & ~(BIT_RIM3_T1_LDNC|BIT_RIM3_T1_LUPC|BIT_RIM3_T1_LDND|BIT_RIM3_T1_LUPD));

		if (fe->te_param.lb_tx_cmd == WAN_TE1_LB_ENABLE){
			WRITE_REG(REG_T1TCD1, 0x80);
			WRITE_REG(REG_TCR4, 0x00);
		}else{
			WRITE_REG(REG_T1TCD1, 0x80);
			WRITE_REG(REG_TCR4, BIT_TCR4_TC0);
		}
		WRITE_REG(REG_TCR3, READ_REG(REG_TCR3) | BIT_TCR3_TLOOP);
	}

	wan_set_bit(LINELB_WAITING,(void*)&fe->te_param.critical);
	wan_set_bit(LINELB_CODE_BIT,(void*)&fe->te_param.critical);
	wan_set_bit(LINELB_CHANNEL_BIT,(void*)&fe->te_param.critical);
	fe_event.type	= TE_LINELB_TIMER;
	fe_event.delay	= delay + 1;
	sdla_ds_te1_add_event(fe, &fe_event);
	return WAN_FE_LBMODE_RC_PENDING;
}

/******************************************************************************
*				sdla_ds_te1_pclb()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_pclb(sdla_fe_t *fe, u_int8_t cmd, u_int32_t chan_map)
{
	int	off, shift, chan;
	int	channel_range = (IS_T1_FEMEDIA(fe)) ? 
					NUM_OF_T1_CHANNELS : 
					NUM_OF_E1_TIMESLOTS;
	unsigned char	val;

	for(chan = 1; chan <= channel_range; chan++){

		if (wan_test_bit(chan, &chan_map)){

			off = (chan-1) / 8;
			shift = (chan-1) % 8;
			val = READ_REG(REG_PCL1+off);
			WRITE_REG(REG_PCL1+off, val | (0x01 << shift));
		}
	}
	return 0;
}

/******************************************************************************
*				sdla_ds_te1_set_lb()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int 
sdla_ds_te1_set_lb(sdla_fe_t *fe, u_int8_t type, u_int8_t mode, u_int32_t chan_map) 
{
	int	err = 0;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	if (!chan_map) chan_map = WAN_TE1_ACTIVE_CH(fe);
	switch(type){
	case WAN_TE1_LIU_ALB_MODE: /* Analog (Local) LB () */
		err = sdla_ds_te1_liu_alb(fe, mode);
		break;
	case WAN_TE1_LIU_LLB_MODE: /* Local LB */
		err = sdla_ds_te1_liu_llb(fe, mode);
		break;
	case WAN_TE1_LIU_RLB_MODE:/* Remote LB - issued LOCALLY */
		err = sdla_ds_te1_liu_rlb(fe, mode);
		break;
	case WAN_TE1_LIU_DLB_MODE:/* Dual LB */
		if (!(err = sdla_ds_te1_liu_llb(fe, mode))){
			err = sdla_ds_te1_liu_rlb(fe, mode);
		}
		break;
	case WAN_TE1_LINELB_MODE:/* Remote LB (allb/dllb commands).
							  * If issued REMOTELY (T1 only) by sending LB command 
							  *	from master clock device the right thing to do is
							  * activate LIU Line Loopback. 
							  * Can be issued LOCALLY for both T1 and E1.
							  * */
		err = sdla_ds_te1_liu_rlb(fe, mode);
		break;
	case WAN_TE1_PAYLB_MODE:
		err = sdla_ds_te1_fr_plb(fe, mode);
		break;
	case WAN_TE1_DDLB_MODE:	/* Diagnostic (Local) LB (adlb/ddlb commands) */
		/*err = sdla_ds_te1_fr_flb(fe, mode); March 31, 2010: replaced by sdla_ds_te1_liu_alb() */
		err = sdla_ds_te1_liu_alb(fe, mode);
		break;
	case WAN_TE1_PCLB_MODE:/* Per-channel LB */
		err = sdla_ds_te1_pclb(fe, mode, chan_map);
		break;
	default:
		DEBUG_EVENT("%s: Unsupported loopback type (%s) type=%d mode=%d!\n",
				fe->name,
				WAN_TE1_LB_MODE_DECODE(type),type,mode);
		return -EINVAL;
	}
	if (err == WAN_FE_LBMODE_RC_SUCCESS){
		if (mode == WAN_TE1_LB_ENABLE){
			wan_set_bit(type, &fe->te_param.lb_mode_map);
		}else{
			wan_clear_bit(type, &fe->te_param.lb_mode_map);
		}
	}

	DEBUG_EVENT("%s: %s %s mode ... %s\n",
				fe->name,
				WAN_TE1_LB_ACTION_DECODE(mode),
				WAN_TE1_LB_MODE_DECODE(type),
				(!err) ? "Done" : "Failed");
	return err;
}

/******************************************************************************
 *				sdla_ds_te1_get_lbmode()	
 *
 * Description:
 * Arguments:
 * Returns:
 *****************************************************************************/
static u32 sdla_ds_te1_get_lbmode(sdla_fe_t *fe) 
{
	u32	type_map = 0;
	u8	lmcr, rcr3;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	lmcr = READ_REG(REG_LMCR);
	if (lmcr & BIT_LMCR_ALB && lmcr & BIT_LMCR_LLB){
		wan_set_bit(WAN_TE1_LIU_DLB_MODE, &type_map);
	}else if (lmcr & BIT_LMCR_ALB){
		wan_set_bit(WAN_TE1_LIU_ALB_MODE, &type_map);
	}else if (lmcr & BIT_LMCR_LLB){
		wan_set_bit(WAN_TE1_LIU_LLB_MODE, &type_map);
	}
 	
	//DavidR: this is misleading!!
	//if (lmcr & BIT_LMCR_RLB) wan_set_bit(WAN_TE1_LINELB_MODE, &type_map);

	//this is what it should be:
	if (lmcr & BIT_LMCR_RLB) wan_set_bit(WAN_TE1_LIU_RLB_MODE, &type_map);


	rcr3 = READ_REG(REG_RCR3);
	if (rcr3 & BIT_RCR3_FLB) wan_set_bit(WAN_TE1_DDLB_MODE, &type_map); 
	if (rcr3 & BIT_RCR3_PLB) wan_set_bit(WAN_TE1_PAYLB_MODE, &type_map);

	/* Check remote loopback modes */
	if (wan_test_bit(WAN_TE1_TX_PAYLB_MODE, &fe->te_param.lb_mode_map)){
		wan_set_bit(WAN_TE1_TX_PAYLB_MODE, &type_map);
	}
	if (wan_test_bit(WAN_TE1_TX_LINELB_MODE, &fe->te_param.lb_mode_map)){
		wan_set_bit(WAN_TE1_TX_LINELB_MODE, &type_map);
	}

	return type_map;
}

/******************************************************************************
 *				sdla_ds_te1_udp_lb()	
 *
 * Description:
 * Arguments:
 * Returns:
 *****************************************************************************/
static int sdla_ds_te1_udp_lb(sdla_fe_t *fe, char *data) 
{
	sdla_fe_lbmode_t	*lb = (sdla_fe_lbmode_t*)data;
	int			err = 0;

	lb->rc = WAN_FE_LBMODE_RC_SUCCESS;
	if (lb->cmd == WAN_FE_LBMODE_CMD_SET){
		switch(lb->type){
		case WAN_TE1_TX_LINELB_MODE:
		case WAN_TE1_TX_PAYLB_MODE:
			lb->rc = (u8)sdla_ds_te1_tx_lb(fe, lb->type, lb->mode);
			return 0; 
			break;

		/*case WAN_TE1_LINELB_MODE:
			DEBUG_WARNING("%s: Warning: %s %s should not be used Locally. This loopback should be controlled by Remote side.\n",
				fe->name, WAN_TE1_LB_ACTION_DECODE(lb->mode), WAN_TE1_LB_MODE_DECODE(lb->type));*/

		default:
			err = sdla_ds_te1_set_lb(fe, lb->type, lb->mode, lb->chan_map);
			break;
		}
	}else if (lb->cmd == WAN_FE_LBMODE_CMD_GET){
		lb->type_map = sdla_ds_te1_get_lbmode(fe);
	}
	if (err) lb->rc = WAN_FE_LBMODE_RC_FAILED;
	return 0;
}

/******************************************************************************
*			sdla_ds_te1_udp_get_stats()	
*
* Description: 
* Arguments:
* Returns:
******************************************************************************/
static int 
sdla_ds_te1_udp_get_stats(sdla_fe_t *fe, char *data,int force) 
{
	sdla_fe_stats_t	*stats = (sdla_fe_stats_t*)&data[0];

	/* TE1 Update T1/E1 perfomance counters */
	sdla_ds_te1_pmon(fe, WAN_FE_PMON_UPDATE|WAN_FE_PMON_READ);
	sdla_ds_te1_rxlevel(fe);
	memcpy(stats, &fe->fe_stats, sizeof(sdla_fe_stats_t));
	

	if (force){
		/* force to read FE alarms */
		DEBUG_EVENT("%s: Force to read Front-End alarms\n",
					fe->name);
		stats->alarms = 
			sdla_ds_te1_read_alarms(fe, WAN_FE_ALARM_READ);
	}
	/* Display Liu alarms */
	stats->alarms |= WAN_TE_ALARM_LIU;
	return 0;
}

/******************************************************************************
*			sdla_ds_te1_bert_count()	
*
* Description: should be called each second
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert_count(sdla_fe_t *fe)
{
	u32	count;
	u8	value;

	DEBUG_TE1("%s: Reading BERT status ...\n", fe->name);
	value = READ_REG(REG_BC1);
	WRITE_REG(REG_BC1, value & ~BIT_BC1_LC);
	WP_DELAY(1000);
	WRITE_REG(REG_BC1, value | BIT_BC1_LC);
	WP_DELAY(100);
	
	/* Bit count */
	count = READ_REG(REG_BBC1);
	count |= (READ_REG(REG_BBC2) << 8);
	count |= (READ_REG(REG_BBC3) << 16);
	count |= (READ_REG(REG_BBC4) << 24);
	DEBUG_TE1("%s: BERT Bit Count (diff)  : %08X\n", fe->name, count);
	fe->te_param.bert_stats.bit_cnt += count;
				
	/* Error count */
	count = READ_REG(REG_BEC1);
	count |= (READ_REG(REG_BEC2) << 8);
	count |= (READ_REG(REG_BEC3) << 16);
	DEBUG_TE1("%s: BERT Error Count (diff): %08X\n", fe->name, count);
	fe->te_param.bert_stats.err_cnt += count;
	if (count){
		fe->te_param.bert_stats.err_sec++;
	}else{
		fe->te_param.bert_stats.err_free_sec++;
	}
	fe->te_param.bert_stats.avail_sec++;
	return 0;
}

/******************************************************************************
*			sdla_ds_te1_bert_reset()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert_reset(sdla_fe_t *fe)
{

	if (!wan_test_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag)){
		return -EINVAL;
	}
	memset(&fe->te_param.bert_stats, 0, sizeof(sdla_te_bert_stats_t));
	return 0; 
}

/******************************************************************************
*			sdla_ds_te1_bert_read_status()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert_read_status(sdla_fe_t *fe)
{

	if (!wan_test_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag)){
		return -EINVAL;
	}
	if (!wan_test_bit(WAN_TE_BERT_FLAG_INLOCK, &fe->te_param.bert_flag)){
		return 0;
	}
	if (fe->fe_status != FE_CONNECTED){
		return 0;
	}
	sdla_ds_te1_bert_count(fe);
	return 0;
}

/******************************************************************************
*			sdla_ds_te1_bert_status()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert_status(sdla_fe_t *fe, sdla_te_bert_stats_t *bert_stats)
{

	if (wan_test_bit(WAN_TE_BERT_FLAG_INLOCK, &fe->te_param.bert_flag)){
		fe->te_param.bert_stats.inlock = 1;
	}else{
		fe->te_param.bert_stats.inlock = 0;
	}
	if (fe->fe_status != FE_CONNECTED){
		fe->te_param.bert_stats.inlock = 0;	/* Force */
	}
	memcpy(bert_stats, &fe->te_param.bert_stats, sizeof(sdla_te_bert_stats_t));
	return 0;
}

/******************************************************************************
*			sdla_ds_te1_bert_stop()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert_stop(sdla_fe_t *fe)
{

	DEBUG_ERROR("%s: Stopping Bit-Error-Test ...\n", fe->name);
	WRITE_REG(REG_BSIM, 0x00);

	WRITE_REG(REG_RXPC, READ_REG(REG_RXPC) & ~BIT_RXPC_RBPEN);
	WRITE_REG(REG_TXPC, READ_REG(REG_TXPC) & ~BIT_TXPC_TBPEN);

	WRITE_REG(REG_BC1, 0x00); 
	WRITE_REG(REG_BC2, 0x00); 

	WRITE_REG(REG_TBPCS1, 0x00);
	WRITE_REG(REG_TBPCS2, 0x00);
	WRITE_REG(REG_TBPCS3, 0x00);
	WRITE_REG(REG_TBPCS4, 0x00);
	WRITE_REG(REG_RBPCS1, 0x00);
	WRITE_REG(REG_RBPCS2, 0x00);
	WRITE_REG(REG_RBPCS3, 0x00);
	WRITE_REG(REG_RBPCS4, 0x00);

	wan_clear_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag);
	memset(&fe->te_param.bert_cfg, 0, sizeof(sdla_te_bert_cfg_t));
	return 0;
}

/******************************************************************************
*			sdla_ds_te1_bert_eib()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert_eib(sdla_fe_t *fe, sdla_te_bert_t *bert)
{
	unsigned char	value;

	if (bert->un.cfg.eib == WAN_TE_BERT_EIB_NONE) return 0;

	if (bert->cmd == WAN_TE_BERT_CMD_EIB){
		if (!wan_test_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag)){
			return -EINVAL;
		}
	}else if (bert->cmd != WAN_TE_BERT_CMD_START){
		/* Invalid BERT command */
		return -EINVAL;
	}

	value = READ_REG(REG_BC2);
	value &= ~(BIT_BC2_EIB2|BIT_BC2_EIB1|BIT_BC2_EIB0|BIT_BC2_SBE);
	WRITE_REG(REG_BC2, value);
	switch(bert->un.cfg.eib){
	case WAN_TE_BERT_EIB_SINGLE:
		value |= BIT_BC2_SBE;
		break;
	case WAN_TE_BERT_EIB1:
		value |= BIT_BC2_EIB0;
		break;
	case WAN_TE_BERT_EIB2:
		value |= BIT_BC2_EIB1;
		break;
	case WAN_TE_BERT_EIB3:
		value |= (BIT_BC2_EIB1|BIT_BC2_EIB0);
		break;
	case WAN_TE_BERT_EIB4:
		value |= BIT_BC2_EIB2;
		break;
	case WAN_TE_BERT_EIB5:
		value |= (BIT_BC2_EIB2|BIT_BC2_EIB0);
		break;
	case WAN_TE_BERT_EIB6:
		value |= (BIT_BC2_EIB2|BIT_BC2_EIB1);
		break;
	case WAN_TE_BERT_EIB7:
		value |= (BIT_BC2_EIB2|BIT_BC2_EIB1|BIT_BC2_EIB0);
		break;
	}
	WP_DELAY(100);
	if (bert->cmd == WAN_TE_BERT_CMD_EIB){
		DEBUG_ERROR("%s: Insert Bit Errors: %s\n", 
				fe->name, WAN_TE_BERT_EIB_DECODE(bert->un.cfg.eib));
	}
	WRITE_REG(REG_BC2, value); 
	return 0;
}

/******************************************************************************
*			sdla_ds_te1_bert_config()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert_config(sdla_fe_t *fe, sdla_te_bert_t *bert)
{
	sdla_fe_timer_event_t	fevent;
	unsigned long		active_ch;
	u8			rxpc, txpc, value;	
	int			i = 0, channel_range = (IS_T1_FEMEDIA(fe)) ? 
							NUM_OF_T1_CHANNELS : 
							NUM_OF_E1_TIMESLOTS;

	DEBUG_EVENT("%s: Starting Bit-Error-Test ...\n", fe->name);

	if (bert->verbose){
		DEBUG_EVENT("%s: BERT command        : %s\n", fe->name, WAN_TE_BERT_CMD_DECODE(bert->cmd));
		DEBUG_EVENT("%s: BERT channel list   : %08X (FE chans: %08X)\n", fe->name, bert->un.cfg.chan_map, WAN_TE1_ACTIVE_CH(fe)); 
		DEBUG_EVENT("%s: BERT pattern type   : %s\n", fe->name, WAN_TE_BERT_PATTERN_DECODE(bert->un.cfg.pattern_type)); 
		DEBUG_EVENT("%s: BERT pattern length : %d\n", fe->name, bert->un.cfg.pattern_len); 
		DEBUG_EVENT("%s: BERT pattern        : %08X\n",fe->name, bert->un.cfg.pattern);
		DEBUG_EVENT("%s: BERT word count     : %d\n", fe->name, bert->un.cfg.count); 
		DEBUG_EVENT("%s: BERT loopback mode  : %s\n", fe->name, WAN_TE_BERT_LOOPBACK_DECODE(bert->un.cfg.lb_type)); 
		DEBUG_EVENT("%s: BERT EIB            : %s\n", fe->name, WAN_TE_BERT_EIB_DECODE(bert->un.cfg.eib)); 
	}

	memset(&fe->te_param.bert_stats, 0, sizeof(sdla_te_bert_stats_t));

	/* Enable BERT */
	rxpc = READ_REG(REG_RXPC);
	txpc = READ_REG(REG_TXPC);
	if (WAN_FE_FRAME(fe) == WAN_FR_UNFRAMED){
		rxpc |= BIT_RXPC_RBPFUS;
		txpc |= BIT_TXPC_TBPFUS;
	}
	WRITE_REG(REG_RXPC, rxpc | BIT_RXPC_RBPEN);
	WRITE_REG(REG_TXPC, txpc | BIT_TXPC_TBPEN);

	/* Channel assignment (def. all channels) */
	WRITE_REG(REG_TBPCS1, 0x00);
	WRITE_REG(REG_TBPCS2, 0x00);
	WRITE_REG(REG_TBPCS3, 0x00);
	WRITE_REG(REG_TBPCS4, 0x00);
	WRITE_REG(REG_RBPCS1, 0x00);
	WRITE_REG(REG_RBPCS2, 0x00);
	WRITE_REG(REG_RBPCS3, 0x00);
	WRITE_REG(REG_RBPCS4, 0x00);

	active_ch  = WAN_TE1_ACTIVE_CH(fe);
	if (bert->un.cfg.chan_map == ENABLE_ALL_CHANNELS){
		bert->un.cfg.chan_map  = WAN_TE1_ACTIVE_CH(fe);
	}

	for(i = 1 /* DavidR: starting from bit one!? */; i <= channel_range; i++){
		int 		off, shift;
		u_int8_t	val;
		if (wan_test_bit(i, (void*)&bert->un.cfg.chan_map) && wan_test_bit(i, (void*)&active_ch)){
			off = (i-1) / 8;
			shift = (i-1) % 8;
			val = READ_REG(REG_TBPCS1+off);
			WRITE_REG(REG_TBPCS1+off, val | (0x01 << shift));
			val = READ_REG(REG_RBPCS1+off);
			WRITE_REG(REG_RBPCS1+off, val | (0x01 << shift));
		}
	}

	/* BERT pattern (def. preusorandom 2e7-1) */
	value = 0;
	switch(bert->un.cfg.pattern_type){
	case WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E7:
		value = 0x00;
		break;
	case WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E11:
		value = BIT_BC1_PS0;
		break;
	case WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E15:
		value = BIT_BC1_PS1;
		break;
	case WAN_TE_BERT_PATTERN_PSEUDORANDOM_QRSS:
		value = BIT_BC1_PS1 | BIT_BC1_PS0;
		break;
	case WAN_TE_BERT_PATTERN_REPETITIVE:
		value = BIT_BC1_PS2;
		if (bert->un.cfg.pattern_len > 32 || 
			bert->un.cfg.pattern_len < 1 /* prevent zero-length pattern length */){
			DEBUG_ERROR("%s: ERROR: Invalid BERT pattern length of %d bits. (Must be between 1 and 32)!\n",
					fe->name, bert->un.cfg.pattern_len);
			return -EINVAL;
		}
		break;
	case WAN_TE_BERT_PATTERN_WORD:
		value = BIT_BC1_PS2 | BIT_BC1_PS0;
		if (bert->un.cfg.count >= 256){
			DEBUG_ERROR("%s: ERROR: Invalid BERT Alternating Word Count %d (1-256)!\n",
							fe->name, bert->un.cfg.count);
			return -EINVAL;
		}
		break;
	case WAN_TE_BERT_PATTERN_DALY:
		value = BIT_BC1_PS2 | BIT_BC1_PS1;
		break;
	case WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E9:
		value = BIT_BC1_PS2 | BIT_BC1_PS1 | BIT_BC1_PS0;
		break;
	default:
		DEBUG_ERROR("%s: Error: Invalid BERT pattern type %s\n", 
				fe->name,
				WAN_TE_BERT_PATTERN_DECODE(bert->un.cfg.pattern_type));
		return -EINVAL;
	}
	WRITE_REG(REG_BC1, value);

	/* BERT pattern string */
	if (bert->un.cfg.pattern_type == WAN_TE_BERT_PATTERN_REPETITIVE){
		u_int32_t	pattern = bert->un.cfg.pattern;
		int		i = 0;
		u16		repeat = 0;
		
		if (bert->un.cfg.pattern_len && bert->un.cfg.pattern_len < 17){
			repeat = 32 / bert->un.cfg.pattern_len;
			for(i=0; i < repeat; i++){
				bert->un.cfg.pattern |= (pattern << bert->un.cfg.pattern_len);
			} 
			bert->un.cfg.pattern_len *= repeat; 
		}
		WRITE_REG(REG_BRP1, bert->un.cfg.pattern & 0xFF);
		WRITE_REG(REG_BRP2, (bert->un.cfg.pattern >> 8) & 0xFF);
		WRITE_REG(REG_BRP3, (bert->un.cfg.pattern >> 16) & 0xFF);
		WRITE_REG(REG_BRP4, (bert->un.cfg.pattern >> 24) & 0xFF);

		/* pattern length */	
		value = READ_REG(REG_BC2);
		WRITE_REG(REG_BC2, value | (bert->un.cfg.pattern_len-17)); 

	} else if (bert->un.cfg.pattern_type == WAN_TE_BERT_PATTERN_WORD){

		WRITE_REG(REG_BRP1, bert->un.cfg.pattern & 0xFF);
		WRITE_REG(REG_BRP2, (bert->un.cfg.pattern >> 8) & 0xFF);
		WRITE_REG(REG_BRP3, bert->un.cfg.pattern & 0xFF);
		WRITE_REG(REG_BRP4, (bert->un.cfg.pattern >> 8) & 0xFF);

		WRITE_REG(REG_BAWC, bert->un.cfg.count/2); 

	}else{
		WRITE_REG(REG_BRP1, 0xFF);
		WRITE_REG(REG_BRP2, 0xFF);
		WRITE_REG(REG_BRP3, 0xFF);
		WRITE_REG(REG_BRP4, 0xFF);
	}

	/* EIB */
	sdla_ds_te1_bert_eib(fe, bert);

	/* Load pattern */
	value = READ_REG(REG_BC1);
	WRITE_REG(REG_BC1, value & ~BIT_BC1_TC);
	WP_DELAY(1000);
	WRITE_REG(REG_BC1, value | BIT_BC1_TC);

	/* Force resynchronization */
	value = READ_REG(REG_BC1);
	WRITE_REG(REG_BC1, value & ~BIT_BC1_RESYNC);
	WP_DELAY(1000);
	WRITE_REG(REG_BC1, value | BIT_BC1_RESYNC);

	/* Enable BERT events */
	WRITE_REG(REG_BSIM, BIT_BSIM_BRLOS|BIT_BSIM_BSYNC);

	/* Clear counters */
	value = READ_REG(REG_BC1);
	WRITE_REG(REG_BC1, value & ~BIT_BC1_LC);
	WP_DELAY(1000);
	WRITE_REG(REG_BC1, value | BIT_BC1_LC);
	
	wan_set_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag);
	sdla_ds_te1_bert_reset(fe);
	memcpy(&fe->te_param.bert_cfg, &bert->un.cfg, sizeof(sdla_te_bert_cfg_t));
	fe->te_param.bert_start = SYSTEM_TICKS;

	fevent.type		= WAN_TE_POLL_BERT;	
	fevent.delay		= POLLING_TE1_TIMER;
	sdla_ds_te1_add_event(fe, &fevent);
	return 0;
}


/******************************************************************************
*			sdla_ds_te1_bert()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_ds_te1_bert(sdla_fe_t *fe, sdla_te_bert_t *bert)
{

	switch(bert->cmd){
	case WAN_TE_BERT_CMD_START:
		if (wan_test_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag)){
			DEBUG_ERROR("%s: ERROR: BERT test is still running!\n", 
						fe->name);
			bert->rc = WAN_TE_BERT_RC_RUNNING;
			return 0;
		}
		break;
	case WAN_TE_BERT_CMD_STOP:
	case WAN_TE_BERT_CMD_EIB:
	case WAN_TE_BERT_CMD_RESET:
		if (!wan_test_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag)){
			DEBUG_ERROR("%s: ERROR: BERT test is not running!\n",
						fe->name);
			bert->rc = WAN_TE_BERT_RC_STOPPED;
			return 0;
		}
		break;
	case WAN_TE_BERT_CMD_STATUS:
	case WAN_TE_BERT_CMD_RUNNING:
		break;
	default:
		DEBUG_ERROR("%s: ERROR: Invalid BERT command (%02X)\n", 
						fe->name, bert->cmd);
		bert->rc = WAN_TE_BERT_RC_EINVAL;
		return 0;
	}
	if (bert->cmd == WAN_TE_BERT_CMD_STATUS){
		if (wan_test_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag)){
			bert->status = WAN_TE_BERT_STATUS_RUNNING;
		}else{
			bert->status = WAN_TE_BERT_STATUS_STOPPED;
		}
		return sdla_ds_te1_bert_status(fe, &bert->un.stats);
	}
	if (bert->cmd == WAN_TE_BERT_CMD_RESET){
		return sdla_ds_te1_bert_reset(fe);
	}
	if (bert->cmd == WAN_TE_BERT_CMD_RUNNING){
		if (wan_test_bit(WAN_TE_BERT_FLAG_READY, &fe->te_param.bert_flag)){
			bert->status = WAN_TE_BERT_STATUS_RUNNING;
		}else{
			bert->status = WAN_TE_BERT_STATUS_STOPPED;
		}
		return 0;	
	}
	if (bert->cmd == WAN_TE_BERT_CMD_EIB){
		return sdla_ds_te1_bert_eib(fe, bert);
	}
	if (bert->cmd == WAN_TE_BERT_CMD_STOP){
		if (fe->te_param.bert_cfg.lb_type != WAN_TE_BERT_LOOPBACK_NONE){
			bert->un.stop.chan_map = fe->te_param.bert_cfg.chan_map;
			bert->un.stop.lb_type  = fe->te_param.bert_cfg.lb_type;
		}
		sdla_ds_te1_bert_stop(fe);
		return 0;
	}
	if (sdla_ds_te1_bert_config(fe, bert)){
		bert->rc = WAN_TE_BERT_RC_EINVAL;
	}
	return 0;
}

/*
 ******************************************************************************
 *				sdla_ds_te1_udp()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int sdla_ds_te1_udp(sdla_fe_t *fe, void* p_udp_cmd, unsigned char* data)
{
	wan_cmd_t		*udp_cmd = (wan_cmd_t*)p_udp_cmd;
	wan_femedia_t		*fe_media;
	sdla_fe_debug_t		*fe_debug;
	sdla_fe_timer_event_t	event;
	uint8_t pending;
	int err;
	
	switch(udp_cmd->wan_cmd_command){
	case WAN_GET_MEDIA_TYPE:
		fe_media = (wan_femedia_t*)data;
		memset(fe_media, 0, sizeof(wan_femedia_t));
		fe_media->media		= fe->fe_cfg.media;
		fe_media->sub_media	= fe->fe_cfg.sub_media;
		fe_media->chip_id	= WAN_TE_CHIP_DM;
		fe_media->max_ports	= fe->fe_max_ports;
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		udp_cmd->wan_cmd_data_len = sizeof(wan_femedia_t); 
		break;

	case WAN_FE_LB_MODE:
		/* Activate/Deactivate Line Loopback modes */
		sdla_ds_te1_udp_lb(fe, data);
		udp_cmd->wan_cmd_data_len = sizeof(sdla_fe_lbmode_t);
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		break;

	case WAN_FE_GET_STAT:
		sdla_ds_te1_udp_get_stats(fe, data, udp_cmd->wan_cmd_fe_force);
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		udp_cmd->wan_cmd_data_len = sizeof(sdla_fe_stats_t);
		break;

 	case WAN_FE_FLUSH_PMON:
		/* TE1 Flush T1/E1 pmon counters */
		sdla_ds_te1_flush_pmon(fe);
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		break;
 
	case WAN_FE_GET_CFG:
		/* Read T1/E1 configuration */
		memcpy(&data[0],
			&fe->fe_cfg,
			sizeof(sdla_fe_cfg_t));
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = sizeof(sdla_fe_cfg_t);
		break;

	case WAN_FE_SET_CFG:
		/* Read T1/E1 configuration */
		memcpy(&fe->fe_cfg,
			   &data[0],
			   sizeof(sdla_fe_cfg_t));
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		DEBUG_EVENT("%s: Set FE Config!\n",
					fe->name );

		DEBUG_EVENT("%s:    Port %d,%s,%s,%s,%s\n",
                fe->name,
                WAN_FE_LINENO(fe)+1,
                FE_LCODE_DECODE(fe),
                FE_FRAME_DECODE(fe),
                TE_CLK_DECODE(fe),
                TE_LBO_DECODE(fe));


		sdla_ds_te1_unconfig(fe);
		sdla_ds_te1_post_unconfig(fe);

		a104_set_digital_fe_clock(fe->card);

		sdla_ds_te1_config(fe);
		sdla_ds_te1_reconfig(fe);
		sdla_ds_te1_post_init(fe);

		break;

	case WAN_FE_SET_DEBUG_MODE:
		fe_debug = (sdla_fe_debug_t*)&data[0];
		switch(fe_debug->type){
		case WAN_FE_DEBUG_RBS:
			if (fe_debug->mode == WAN_FE_DEBUG_RBS_READ){
				DEBUG_EVENT("%s: Reading RBS status!\n",
					fe->name);
				event.type	= TE_RBS_READ;
				event.delay	= POLLING_TE1_TIMER;
				err=sdla_ds_te1_exec_event(fe, &event, &pending);
				udp_cmd->wan_cmd_return_code = err;
				
			}else if (fe_debug->mode == WAN_FE_DEBUG_RBS_PRINT){
				/* Enable extra debugging */
				sdla_ds_te1_rbs_print(fe, 1);
	    			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			}else if (fe_debug->mode == WAN_FE_DEBUG_RBS_RX_ENABLE){
				/* Enable extra debugging */
				DEBUG_EVENT("%s: Enable RBS RX DEBUG mode!\n",
					fe->name);
				fe->fe_debug |= WAN_FE_DEBUG_RBS_RX_ENABLE;
				sdla_ds_te1_rbs_print(fe, 1);
	    			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			}else if (fe_debug->mode == WAN_FE_DEBUG_RBS_TX_ENABLE){
				/* Enable extra debugging */
				DEBUG_EVENT("%s: Enable RBS TX DEBUG mode!\n",
					fe->name);
				fe->fe_debug |= WAN_FE_DEBUG_RBS_TX_ENABLE;
				sdla_ds_te1_rbs_print(fe, 1);
	    			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			}else if (fe_debug->mode == WAN_FE_DEBUG_RBS_RX_DISABLE){
				/* Disable extra debugging */
				DEBUG_EVENT("%s: Disable RBS RX DEBUG mode!\n",
					fe->name);
				fe->fe_debug &= ~WAN_FE_DEBUG_RBS_RX_ENABLE;
				sdla_ds_te1_rbs_print(fe, 1);
	    			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			}else if (fe_debug->mode == WAN_FE_DEBUG_RBS_TX_DISABLE){
				/* Disable extra debugging */
				DEBUG_EVENT("%s: Disable RBS TX DEBUG mode!\n",
					fe->name);
				fe->fe_debug &= ~WAN_FE_DEBUG_RBS_TX_ENABLE;
				sdla_ds_te1_rbs_print(fe, 1);
	    			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			}else if (fe_debug->mode == WAN_FE_DEBUG_RBS_SET){
				
				
				if (IS_T1_FEMEDIA(fe)){
					if (fe_debug->fe_debug_rbs.channel < 1 || 
					    fe_debug->fe_debug_rbs.channel > 24){
						DEBUG_EVENT(
						"%s: Invalid channel number %d\n",
							fe->name,
							fe_debug->fe_debug_rbs.channel);
						break;
					}
				}else{
					if (fe_debug->fe_debug_rbs.channel < 0 || 
					    fe_debug->fe_debug_rbs.channel > 31){
						DEBUG_EVENT(
						"%s: Invalid channel number %d\n",
							fe->name,
							fe_debug->fe_debug_rbs.channel);
						break;
					}
				}

				event.type		= TE_SET_RBS;
				event.delay		= POLLING_TE1_TIMER;
				event.te_event.rbs_channel = fe_debug->fe_debug_rbs.channel;
				event.te_event.rbs_abcd	= fe_debug->fe_debug_rbs.abcd;
				err=sdla_ds_te1_exec_event(fe, &event, &pending);
				udp_cmd->wan_cmd_return_code = err;
				
			}
			break;
		case WAN_FE_DEBUG_RECONFIG:
			event.type	= TE_POLL_CONFIG;
			event.delay	= POLLING_TE1_TIMER;
			sdla_ds_te1_add_event(fe, &event);
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			break;

#if 0						
		case WAN_FE_DEBUG_CONFIG_VERIFY:
			event.type	= TE_POLL_CONFIG_VERIFY;
			event.delay	= POLLING_TE1_TIMER;
			sdla_ds_te1_add_event(fe, &event);
    			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			break;
#endif
			
		case WAN_FE_DEBUG_REG:

#if 0
			if (fe->te_param.reg_dbg_busy){
				if (fe_debug->fe_debug_reg.read == 2 && fe->te_param.reg_dbg_ready){
					/* Poll the register value */
					fe_debug->fe_debug_reg.value = fe->te_param.reg_dbg_value;
					udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
					fe->te_param.reg_dbg_busy = 0;
				}
				break;
			}
#endif

			event.type		= (fe_debug->fe_debug_reg.read) ? 
							TE_POLL_READ : TE_POLL_WRITE;
			event.te_event.reg	= (u_int16_t)fe_debug->fe_debug_reg.reg;
			event.te_event.value	= fe_debug->fe_debug_reg.value;
			event.delay		= POLLING_TE1_TIMER;
#if 0
			if (fe_debug->fe_debug_reg.read){
				fe->te_param.reg_dbg_busy = 1;
				fe->te_param.reg_dbg_ready = 0;
			}
#endif
			err=sdla_ds_te1_exec_event(fe, &event, &pending);

			if (err == 0 ) {
				if (fe_debug->fe_debug_reg.read) {
					fe_debug->fe_debug_reg.value = fe->te_param.reg_dbg_value;
				}
				udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
				fe->te_param.reg_dbg_busy = 0;
			}
			
			udp_cmd->wan_cmd_return_code = err;
			break;
		
		case WAN_FE_DEBUG_ALARM:
			
			if (fe_debug->mode == WAN_FE_DEBUG_ALARM_AIS_ENABLE) {
				DEBUG_EVENT("%s: Alarm Maintenance AIS On\n",fe->name);
				sdla_ds_te1_set_alarms(fe,WAN_TE_BIT_ALARM_AIS);
				fe->fe_stats.tx_maint_alarms |= WAN_TE_BIT_ALARM_AIS;
				/* If user set the AIS the make sure to turn off
				   the startup timer that will turn off AIS
				   after timeout */
				fe->te_param.tx_ais_startup_timeout=0;
				udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			} else if (fe_debug->mode == WAN_FE_DEBUG_ALARM_AIS_DISABLE) {
				DEBUG_EVENT("%s: Alarm Maintenance AIS Off\n",fe->name);
				sdla_ds_te1_clear_alarms(fe,WAN_TE_BIT_ALARM_AIS);
				fe->fe_stats.tx_maint_alarms &= ~WAN_TE_BIT_ALARM_AIS;
				udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			} else {
				DEBUG_ERROR("%s: Error Invalid Alarm command 0x%02X\n",fe->name,fe_debug->mode);
				udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
			}
			break;

		default:
			DEBUG_ERROR("%s: Error Invalid WAN_FE_SET_DEBUG_MODE cmd 0x%02X\n",
				fe->name,fe_debug->type);
			udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
			break;
		}
    	    	udp_cmd->wan_cmd_data_len = 0;
		break;
	case WAN_FE_TX_MODE:
		fe_debug = (sdla_fe_debug_t*)&data[0];
		switch(fe_debug->mode){
		case WAN_FE_TXMODE_ENABLE:
			DEBUG_EVENT("%s: Enable Transmitter!\n",
					fe->name);
			WRITE_REG(REG_LMCR, READ_REG(REG_LMCR) | BIT_LMCR_TE);
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			break;
		case WAN_FE_TXMODE_DISABLE:
			DEBUG_EVENT("%s: Disable Transmitter (tx tri-state mode)!\n",
					fe->name);
			WRITE_REG(REG_LMCR, READ_REG(REG_LMCR) & ~BIT_LMCR_TE);
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			break;
		default:
			udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
		    	udp_cmd->wan_cmd_data_len = 0;
			break;
		}
		break;

	case WAN_FE_BERT_MODE:

		if (fe->fe_status != FE_CONNECTED){
			DEBUG_WARNING("%s: Warning: Bit-Error-Test can not run when line is in \"Disconnected\" state.\n", fe->name);
			udp_cmd->wan_cmd_return_code = SANG_STATUS_LINE_DISCONNECTED;
			break;
		}

		if (IS_E1_FEMEDIA(fe)){
			DEBUG_WARNING("%s: Warning: Transmission of LB Activation Code not supported in E1 mode.\n", fe->name);
			udp_cmd->wan_cmd_return_code = SANG_STATUS_OPTION_NOT_SUPPORTED;
			break;
		}

		sdla_ds_te1_bert(fe, (sdla_te_bert_t*)&data[0]);
    	udp_cmd->wan_cmd_data_len = sizeof(sdla_te_bert_t);
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		break;

	default:
		udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
    	udp_cmd->wan_cmd_data_len = 0;
		break;
	}
	return 0;
}

static int 
sdla_ds_te1_update_alarm_info(sdla_fe_t* fe, struct seq_file* m, int* stop_cnt)
{

#if !defined(__WINDOWS__)
	PROC_ADD_LINE(m,
		"=============================== %s Alarms ===============================\n",
		FE_MEDIA_DECODE(fe));
	PROC_ADD_LINE(m,
		PROC_STATS_ALARM_FORMAT,
		"ALOS", WAN_TE_PRN_ALARM_ALOS(fe->fe_alarm), 
		"LOS", WAN_TE_PRN_ALARM_LOS(fe->fe_alarm));
	PROC_ADD_LINE(m,
		PROC_STATS_ALARM_FORMAT,
		"RED", WAN_TE_PRN_ALARM_RED(fe->fe_alarm), 
		"AIS", WAN_TE_PRN_ALARM_AIS(fe->fe_alarm));
	if (IS_T1_FEMEDIA(fe)){
		PROC_ADD_LINE(m,
			PROC_STATS_ALARM_FORMAT,
			"RAI", WAN_TE_PRN_ALARM_RAI(fe->fe_alarm),
			"LOF", WAN_TE_PRN_ALARM_LOF(fe->fe_alarm));
	}else{ 
		PROC_ADD_LINE(m,
			PROC_STATS_ALARM_FORMAT,
			"LOF", WAN_TE_PRN_ALARM_LOF(fe->fe_alarm), 
			"RAI", WAN_TE_PRN_ALARM_RAI(fe->fe_alarm));
	}
	return m->count;	
#endif
	return 0;
}

static int 
sdla_ds_te1_update_pmon_info(sdla_fe_t* fe, struct seq_file* m, int* stop_cnt)
{
		
#if !defined(__WINDOWS__)
	PROC_ADD_LINE(m,
		 "=========================== %s PMON counters ============================\n",
		 FE_MEDIA_DECODE(fe));
	if (IS_T1_FEMEDIA(fe)){
		PROC_ADD_LINE(m,
			PROC_STATS_PMON_FORMAT,
			"Line Code Violation", fe->fe_stats.te_pmon.lcv_errors,
			"Bit Errors", fe->fe_stats.te_pmon.bee_errors);
		PROC_ADD_LINE(m,
			PROC_STATS_PMON_FORMAT,
			"Out of Frame Errors", fe->fe_stats.te_pmon.oof_errors,
			"", (u_int32_t)0);
	}else{
		PROC_ADD_LINE(m,
			PROC_STATS_PMON_FORMAT,
			"Line Code Violation", fe->fe_stats.te_pmon.lcv_errors,
			"CRC4 Errors", fe->fe_stats.te_pmon.crc4_errors);
		PROC_ADD_LINE(m,
			PROC_STATS_PMON_FORMAT,
			"FAS Errors", fe->fe_stats.te_pmon.fas_errors,
			"Far End Block Errors", fe->fe_stats.te_pmon.feb_errors);
	}
	return m->count;
#endif
	return 0;
}


