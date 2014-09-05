/***************************************************************************
 * sdla_bri.c	WANPIPE(tm) 
 *		ISDN-BRI support module for "Cologne XHFC-2SU" chip.
 *
 * Author(s): 	David Rokhvarg   <davidr@sangoma.com>
 *
 * Copyright:	(c) 1984 - 2007 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * ============================================================================
 * March	12, 2007	David Rokhvarg
 *				v1.0 Initial version.
 *
 * February	26, 2008	David Rokhvarg	
 *				v1.1 Imrovements in SU State transition code.
 *				Re-implemented T3 and T4 timers.
 *
 * March	10, 2008	David Rokhvarg	
 *				v1.2 Added 'loopback' commands.
 *				Added commands to switch between line recovery
 *				clock and internal oscillator clock.
 *
 * April	1,  2008	David Rokhvarg	
 *				v1.3 In TE mode, when in F3 state, Do NOT
 *				indicate 'disconnect' right away, do it when 
 *				T4 expires.
 *
 * July		21  2009	David Rokhvarg	
 *				v1.4 Implemented T1 timer for NT.
 *
 * January	24  2011	David Rokhvarg	
 *				v1.5 The T3 timer was broken by changes in AFT Core - fixed it
 *					 by adding T3_TIMER_EXPIRED bit.
 *					 The T3_TIMER_EXPIRED bit resolved "failed TE activation"
 *					 issue on some BRI lines.
 ******************************************************************************
 */

/*******************************************************************************
**			   INCLUDE FILES
*******************************************************************************/

# include "wanpipe_includes.h"
# include "wanpipe_defines.h"
# include "wanpipe_debug.h"
# include "wanpipe_abstr.h"
# include "wanpipe_common.h"
# include "wanpipe_events.h"
# include "wanpipe.h"
# include "wanpipe_events.h"
# include "if_wanpipe_common.h"	/* for 'wanpipe_common_t' used in 'aft_core.h'*/
# include "aft_core.h"		/* for 'private_area_t' */
# include "sdla_bri.h"


/* DEBUG macro definitions */
#define DEBUG_HFC_INIT		if(0)DEBUG_EVENT
#define DEBUG_HFC_MODE		if(0)DEBUG_EVENT
#define DEBUG_HFC_IRQ		if(0)DEBUG_EVENT
#define DEBUG_HFC_SU_IRQ	if(0)DEBUG_EVENT

#define DEBUG_HFC_TX		if(0)DEBUG_EVENT
#define DEBUG_TX_DATA		if(0)DEBUG_EVENT
#define TX_EXTRA_DBG		if(0)DEBUG_EVENT
#define TX_FAST_DBG			if(0)DEBUG_EVENT

#define DEBUG_HFC_RX		if(0)DEBUG_EVENT
#define RX_EXTRA_DBG		if(0)DEBUG_EVENT
#define DEBUG_RX1			if(0)DEBUG_EVENT
#define DBG_RX_DATA			if(0)DEBUG_EVENT

#define DEBUG_HFC_CLOCK		if(0)DEBUG_EVENT
#define DBG_MODULE_TESTER	if(0)DEBUG_EVENT

#define NT_STATE_FUNC()		if(0)DEBUG_EVENT("%s(): line: %d\n", __FUNCTION__, __LINE__)
#define CLOCK_FUNC()		if(0)DEBUG_EVENT("%s(): line: %d\n", __FUNCTION__, __LINE__)

#define DBG_SPI				if(0)DEBUG_EVENT

#define DEBUG_FE_STATUS		if(0)DEBUG_EVENT

#define DEBUG_LOOPB			if(0)DEBUG_EVENT

#define DEBUG_TE_STATES		if(0)DEBUG_EVENT

#define FIFO_THRESHOLD_INDEX	1

typedef enum _nt_states{
	NT_STATE_RESET_G0 = 0,
	NT_STATE_DEACTIVATED_G1,
	NT_STATE_PENDING_ACTIVATION_G2,
	NT_STATE_ACTIVE_G3,
	NT_STATE_PENDING_DEACTIVATION_G4
}nt_states_t;

typedef enum _te_states{
	TE_STATE_RESET_F0 = 0,
	TE_STATE_SENSING_F2 = 2,
	TE_STATE_DEACTIVATED_F3 = 3,
	TE_STATE_AWAITING_SIGNAL_F4 = 4,
	TE_STATE_IDENTIFYING_INPUT_F5 = 5,
	TE_STATE_SYNCHRONIZED_F6 = 6,
	TE_STATE_ACTIVATED_F7 = 7,
	TE_STATE_LOST_FRAMING_F8 = 8
}te_states_t;

#define WP_DECODE_TE_STATE(te_state)	\
(te_state == TE_STATE_RESET_F0) ? "TE_STATE_RESET_F0" :	\
(te_state == TE_STATE_SENSING_F2) ? "TE_STATE_SENSING_F2" :	\
(te_state == TE_STATE_DEACTIVATED_F3) ? "TE_STATE_DEACTIVATED_F3" :	\
(te_state == TE_STATE_AWAITING_SIGNAL_F4) ? "TE_STATE_AWAITING_SIGNAL_F4" :	\
(te_state == TE_STATE_IDENTIFYING_INPUT_F5) ? "TE_STATE_IDENTIFYING_INPUT_F5" :	\
(te_state == TE_STATE_SYNCHRONIZED_F6) ? "TE_STATE_SYNCHRONIZED_F6" :	\
(te_state == TE_STATE_ACTIVATED_F7) ? "TE_STATE_ACTIVATED_F7" :	\
(te_state == TE_STATE_LOST_FRAMING_F8) ? "TE_STATE_LOST_FRAMING_F8" :	\
"Invalid TE State"


#define CHECK_DATA 0
#if CHECK_DATA
static void dump_chip_SRAM(sdla_fe_t *fe, u8 mod_no, u8 port_no);
static void dump_data(u8 *data, int data_len);
static int check_data(u8 *data, int data_len);
#endif

typedef enum _wp_bri_critical_bits {
	WP_BCB_BRI_CONFIG,
	WP_BCB_BRI_POST_INIT
} wp_bri_critical_bits_t;

/*******************************************************************************
**			  DEFINES AND MACROS
*******************************************************************************/
#define REPORT_MOD_NO(mod_no)	(mod_no+1)

static u8 validate_fe_line_no(sdla_fe_t *fe, const char *caller_name)
{
	if ((int32_t)WAN_FE_LINENO(fe) < 0 || WAN_FE_LINENO(fe) > MAX_BRI_LINES){
		DEBUG_EVENT("%s(): %s: ISDN BRI: Invalid FE line number %d (Min=1 Max=%d)\n",
			caller_name, fe->name, WAN_FE_LINENO(fe)+1, MAX_BRI_LINES);
		return 1;
	}
	return 0;
}

static int32_t validate_physical_mod_no(u32 mod_no, const char *caller_name)
{
	if(mod_no % 2){
		DEBUG_ERROR("%s(): Error: mod_no (%d) is not divisible by 2!!\n", 
			caller_name, mod_no);
		return 1;
	}

	if(mod_no >= MAX_BRI_LINES){
		DEBUG_ERROR("%s(): Error: mod_no (%d) is greate than maximum of %d!!\n", 
			caller_name, mod_no, MAX_BRI_LINES - 1);
		return 1;
	}
	return 0;
}

/* Translate FE_LINENO to physical module number divisible by BRI_MAX_PORTS_PER_CHIP. */
static u8 fe_line_no_to_physical_mod_no(sdla_fe_t *fe)
{
	u8 mod_no;

	mod_no = (u8)WAN_FE_LINENO(fe);
	/* get quotient between 0 and 11 (including) */
	mod_no = mod_no / BRI_MAX_PORTS_PER_CHIP; 
	/* here WAN_FE_LINENO(fe) is translated into an EVEN number between 0 and 22 (including). */
	mod_no *= BRI_MAX_PORTS_PER_CHIP;
	
	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return 0;
	}

	return mod_no;
}

/* Translate FE_LINENO to port number on the module. can be only 0 or 1. */
static u8 fe_line_no_to_port_no(sdla_fe_t *fe)
{
	return WAN_FE_LINENO(fe) % BRI_MAX_PORTS_PER_CHIP;
}


/*******************************************************************************/
/* SPI access functions and dbg macros */

static
__u8 _read_xhfc(sdla_fe_t *fe, u32 mod_no, u8 reg, const char *caller_name, int32_t file_lineno);

static 
int32_t _write_xhfc(sdla_fe_t *fe, u32 mod_no, u8 reg, u8 val, 
		    const char *caller_name, int32_t file_lineno);

/* Read/Write to front-end register */
#define WRITE_REG(reg,val)	_write_xhfc(fe, mod_no, reg, val, __FUNCTION__, __LINE__)
#define READ_REG(reg)		_read_xhfc(fe, mod_no, reg, __FUNCTION__, __LINE__)

/*******************************************************************************/


/*******************************************************************************
**			STRUCTURES AND TYPEDEFS
*******************************************************************************/


/*******************************************************************************
**			   GLOBAL VARIABLES
*******************************************************************************/
#if !defined(__WINDOWS__)
extern WAN_LIST_HEAD(, wan_tdmv_) wan_tdmv_head;
#endif


/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
static int32_t	bri_global_config(void* pfe);
static int32_t	bri_global_unconfig(void* pfe);

static int32_t	wp_bri_config(void *pfe);
static int32_t	wp_bri_post_init(void *pfe);

static int32_t	wp_bri_unconfig(void *pfe);
static int32_t	wp_bri_post_unconfig(void* pfe);

static int32_t	wp_bri_if_config(void *pfe, u32 mod_map, u8);
static int32_t	wp_bri_if_unconfig(void *pfe, u32 mod_map, u8);

static int32_t	wp_bri_disable_irq(sdla_fe_t *fe, u32 mod_no, u8 port_no);
static int	wp_bri_disable_fe_irq(void *fe);
static void	bri_enable_interrupts(sdla_fe_t *fe, u32 mod_no, u8 port_no);
static int32_t	wp_bri_intr(sdla_fe_t *); 
static int32_t	wp_bri_check_intr(sdla_fe_t *); 
static int32_t	wp_bri_polling(sdla_fe_t*);
static int32_t	wp_bri_udp(sdla_fe_t*, void*, u8*);
static u32	wp_bri_active_map(sdla_fe_t* fe, u8 line_no);
static u8	wp_bri_fe_media(sdla_fe_t *fe);
static int32_t	wp_bri_set_dtmf(sdla_fe_t*, int32_t, u8);

static int	wp_bri_intr_ctrl(sdla_fe_t *fe, int, u_int8_t, u_int8_t, unsigned int);
static int	wp_bri_event_ctrl(sdla_fe_t*, wan_event_ctrl_t*);

static int32_t	wp_bri_dchan_tx(sdla_fe_t *fe, void *src_data_buffer, u32 buffer_len);

static void	*wp_bri_dchan_rx(sdla_fe_t *fe, u8 mod_no, u8 port_no);

static int	wp_bri_get_fe_status(sdla_fe_t *fe, unsigned char *status, int notused);
static int	wp_bri_set_fe_status(sdla_fe_t *fe, unsigned char status);

static int	wp_bri_control(sdla_fe_t *fe, u32 command);

/*******************************************************************************/
/* TE timer routines */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void l1_timer_expire_t3(void* pfe);
static void l1_timer_expire_t4(void* pfe);
#elif defined(__WINDOWS__)
static void l1_timer_expire_t3(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3);
static void l1_timer_expire_t4(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3);
#else
static void l1_timer_expire_t3(unsigned long pfe);
static void l1_timer_expire_t4(unsigned long pfe);
#endif

static void l1_timer_start_t3(void *pport);
static void l1_timer_stop_t3(void *pport);
static void l1_timer_start_t4(void *pport);
static void l1_timer_stop_t4(void *pport);

static void __l1_timer_expire_t3(sdla_fe_t *fe);

/* NT timer routines */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void l1_timer_expire_t1(void* pfe);
#elif defined(__WINDOWS__)
static void l1_timer_expire_t1(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3);
#else
static void l1_timer_expire_t1(unsigned long pfe);
#endif

static void l1_timer_start_t1(void *pport);
static void l1_timer_stop_t1(void *pport);
static void __l1_timer_expire_t1(sdla_fe_t *fe);

/*******************************************************************************/

static int32_t	wp_bri_spi_bus_reset(sdla_fe_t	*fe);
static int32_t	reset_chip(sdla_fe_t *fe, u32 mod_no);
static int32_t	init_xfhc(sdla_fe_t *fe, u32 mod_no);
static void	xhfc_select_fifo(sdla_fe_t *fe, u32 mod_no, u8 fifo);
static void	xhfc_waitbusy(sdla_fe_t *fe, u32 mod_no);
static void	xhfc_select_pcm_slot(sdla_fe_t *fe, u32 mod_no, u8 slot, u8 direction);
static void	xhfc_increment_fifo(sdla_fe_t *fe, u32 mod_no);

static int32_t	__config_clock_routing(sdla_fe_t *fe, u32 mod_no, u8 master_mode);
static int32_t	config_clock_routing(sdla_fe_t *fe, u8 master_mode);

static void xhfc_ph_command(sdla_fe_t *fe, bri_xhfc_port_t *port, u_char command);

static u8 __su_new_state(sdla_fe_t *fe, u8 mod_no, u8 port_no);
static void sdla_bri_set_status(sdla_fe_t* fe, u8 mod_no, u8 port_no, u8 status);

/* for selecting PCM direction */
#define XHFC_DIRECTION_TX 0
#define XHFC_DIRECTION_RX 1

#if 0
static void xhfc_select_xhfc_channel(sdla_fe_t *fe, u32 mod_no,
				  u8 channel, u8 direction, u8 vrout_bitmap);
#endif

static int32_t check_f0cl_increment(sdla_fe_t *fe, u8 old_f0cl, u8 new_f0cl, int32_t *diff);

/*******************************************************************************
**			  FUNCTION DEFINITIONS
*******************************************************************************/


/*******************************************************************************/

static
u8 _read_xhfc(sdla_fe_t *fe, u32 mod_no, u8 reg, const char *caller_name, int32_t file_lineno)
{				
	u8 val = READ_BRI_REG(mod_no, reg);
	return val;
}

static 
int32_t _write_xhfc(sdla_fe_t *fe, u32 mod_no, u8 reg, u8 val, const char *caller_name, int32_t file_lineno)
{
	return WRITE_BRI_REG(mod_no, reg, val);
}

/*******************************************************************************/

static void xhfc_waitbusy(sdla_fe_t *fe, u32 mod_no)
{	
	u32	wait_counter = 0;
#define MAX_XHFC_WAIT_COUNTER	200

	do{
		if(!(READ_REG(R_STATUS) & M_BUSY)){
			break;
		}
		WP_DELAY(10);
	}while(wait_counter++ < MAX_XHFC_WAIT_COUNTER);

	if(wait_counter >= MAX_XHFC_WAIT_COUNTER){
		DEBUG_EVENT("%s: %s() time out!\n", fe->name, __FUNCTION__);		
	}
}

static inline void xhfc_resetfifo(sdla_fe_t *fe, u32 mod_no)
{
	WRITE_REG(A_INC_RES_FIFO, M_RES_FIFO | M_RES_FIFO_ERR);
	xhfc_waitbusy(fe, mod_no);
}

static void xhfc_select_fifo(sdla_fe_t *fe, u32 mod_no, u8 fifo)
{
	WRITE_REG(R_FIFO, fifo);
	xhfc_waitbusy(fe, mod_no);
}

static void xhfc_select_pcm_slot(sdla_fe_t *fe, u32 mod_no, u8 slot, u8 direction)
{
	reg_r_slot	r_slot;

	memset(&r_slot, 0, sizeof(reg_r_slot));
	
	r_slot.bit.v_sl_dir = direction;
	r_slot.bit.v_sl_num = slot;
	
	WRITE_REG(R_SLOT, r_slot.reg);
	xhfc_waitbusy(fe, mod_no);
}

#if 0
static void xhfc_select_xhfc_channel(sdla_fe_t *fe, u32 mod_no,
				  u8 channel, u8 direction, u8 vrout_bitmap)
{
	reg_a_sl_cfg	a_sl_cfg;
	memset(&a_sl_cfg, 0, sizeof(reg_a_sl_cfg));

	a_sl_cfg.bit.v_ch_sdir	= direction;	/* 1 bit */
	a_sl_cfg.bit.v_ch_snum	= channel;	/* 5 bits */
	a_sl_cfg.bit.v_rout	= vrout_bitmap;

	WRITE_REG(A_SL_CFG, a_sl_cfg.reg);
	xhfc_waitbusy(fe, mod_no);
}
#endif

static void xhfc_increment_fifo(sdla_fe_t *fe, u32 mod_no)
{
	WRITE_REG(A_INC_RES_FIFO, M_INC_F);
	xhfc_waitbusy(fe, mod_no);
}

static u32 calculate_pcm_timeslot(u32 mod_no, u8 port_no, u8 bchan)
{
	u32	pcm_slot;
#if 0
	DEBUG_HFC_INIT("port_no: %d, bchan: %d\n", port_no, bchan);
#endif
	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return 0;
	}

	pcm_slot = 2*port_no + bchan;	/* 0, 1, 2, 3 */
	pcm_slot *= 2;			/* 0, 2, 4, 6 */
	pcm_slot += (4*mod_no);		/* mod_no is 0,2,4.... -->0+0, 0+8, 0+16 */

	return pcm_slot;
}

static int32_t reset_chip(sdla_fe_t *fe, u32 mod_no)
{
	sdla_bri_param_t	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	int32_t				err = 0, maximum_reset_wait_counter;
	reg_r_fifo_thres	r_fifo_thres;

	DEBUG_HFC_INIT("%s(): mod_no: %d\n", __FUNCTION__, mod_no);

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return 1;
	}

	bri_module = &bri->mod[mod_no];

	/* read ChipID from Read Only register R_CHIP_ID */
	fe->fe_chip_id = READ_REG(R_CHIP_ID);
	switch (fe->fe_chip_id)
	{
	case CHIP_ID_2SU:
		DEBUG_EVENT("%s: Detected XHFC-2SU chip.\n", fe->name);
		bri_module->num_ports = 2;
		bri_module->max_fifo = 8;
		bri_module->max_z = 0x7F;
		DBG_MODULE_TESTER("configure FIFOs\n");
		/* 01 = 8 FIFOs with 128 bytes for TX and RX each. page 125 */
		WRITE_REG( R_FIFO_MD, M1_FIFO_MD * 1);
		break;
	default:
		err = 1;
		DEBUG_EVENT("%s: %s(): unknown Chip ID 0x%x!\n",
		       fe->name, __FUNCTION__, fe->fe_chip_id);
		return err;
	}

	/* general soft chip reset */
	WRITE_REG(R_CIRM, M_SRES);
	WP_DELAY(50);
	WRITE_REG(R_CIRM, 0);
	WP_DELAY(2000);

	/* wait for XHFC init seqeuence to be finished. should take ~40 microseconds (p.285). */
	maximum_reset_wait_counter = 1000;
	while ((READ_REG(R_STATUS) & (M_BUSY | M_PCM_INIT)) && (maximum_reset_wait_counter)){
		WP_DELAY(10);/* 10 microsecond delay */
		maximum_reset_wait_counter--;
	}

	if (!(maximum_reset_wait_counter)) {
		DEBUG_ERROR("%s: %s(): Error: chip initialization sequence timeout!\n",
			       fe->name, __FUNCTION__);
		return 1;
	}

	/* amplitude */
	WRITE_REG(R_PWM_MD, 0x80);
	WRITE_REG(R_PWM1, 0x18);

	/* Set FIFO threshold. page 124.*/
	r_fifo_thres.reg = 0;
	r_fifo_thres.bit.v_thres_tx = r_fifo_thres.bit.v_thres_rx = FIFO_THRESHOLD_INDEX;
	WRITE_REG(R_FIFO_THRES, r_fifo_thres.reg);

	WP_DELAY(2000);
	return 0;
}


/******************************************************************************
*			__config_clock_routing()	
*
* Description	: TE mode: if 'master_mode' is set to WANOPT_YES, clock
*		  recovered from the line will be routed to ALL other BRI
*		  modules on the card.
*		  NT mode: the only valid value for 'master_mode' is WANOPT_NO.
*
* Arguments	:  pfe - pointer to Front End structure.
*		   mod_no - module number.
*		   master_mode - Yes or No
*
* Returns	:  0 - configred successfully, otherwise non-zero value.
*******************************************************************************/
static int32_t __config_clock_routing(sdla_fe_t *fe, u32 mod_no, u8 master_mode)
{
	reg_r_pcm_md0		pcm_md0;
	reg_r_pcm_md2		r_pcm_md2;
	reg_r_gpio_sel		r_gpio_sel;
	reg_r_gpio_en0		r_gpio_en0;

	DEBUG_HFC_CLOCK("%s(): mod_no: %d\n", __FUNCTION__, mod_no);

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return 1;
	}

	if (fe->bri_param.mod[mod_no].type != MOD_TYPE_TE && master_mode) {
		DEBUG_ERROR("%s: Module %d: error configuring clock routing on NT\n", 
			fe->name, mod_no);
		return 1;
	} 

	
	DEBUG_EVENT("%s: %s Clock line recovery Module=%d Port=%d: %s\n", 
		fe->name, WP_BRI_DECODE_MOD_TYPE(fe->bri_param.mod[mod_no].type), 
		REPORT_MOD_NO(mod_no), fe_line_no_to_port_no(fe)+1, 
		(master_mode == WANOPT_YES ? "Enabled" : "Disabled"));
	

	/************************************************************************/
#if 0
	{
		reg_r_su_sync		r_su_sync;
		r_su_sync.reg = 0;
		if(master_mode == WANOPT_YES){
			CLOCK_FUNC();
			/*r_su_sync.bit.v_sync_sel = 0;	//000 = source is line interface 0
			r_su_sync.bit.v_man_sync = 1;
			r_su_sync.bit.v_auto_synci = 1;*/
		}else{
			CLOCK_FUNC();

		}
		WRITE_REG(R_SU_SYNC, r_su_sync.reg);
	}
#endif
	/************************************************************************/
	r_gpio_sel.reg = 0;
	r_gpio_en0.reg = 0;
	if(master_mode == WANOPT_YES){
		CLOCK_FUNC();
		/* select 1-st function of pin 16 - SYNC_O. page 315. */
		r_gpio_sel.bit.v_gpio_sel6 = 0;	

		/* enable output on pin 16 - page 314.*/
		r_gpio_en0.bit.v_gpio_en6 = 1;
	}else{
		CLOCK_FUNC();
		/* if in NORMAL mode, do NOT provide output on 16, 
		so there is only one clock source - the master. */

		/* select 2-nd function of pin 16 - GPIO6. page 315.*/
		r_gpio_sel.bit.v_gpio_sel6 = 1;	

		/* DISABLE output on pin 16 - page 314. */
		r_gpio_en0.bit.v_gpio_en6 = 0;
	}
	WRITE_REG(R_GPIO_SEL, r_gpio_sel.reg);
	WRITE_REG(R_GPIO_EN0, r_gpio_en0.reg);

	/************************************************************************/
	pcm_md0.reg = 0;
	pcm_md0.bit.v_pcm_idx = 0xA;	/* get access to R_PCM_MD2 */
#if 0
	/* PCM master mode test */
	pcm_md0.bit.v_pcm_md = 0x1;	/* PCM bus mode.
					0 = slave (pins C4IO and F0IO are inputs)
					1 = master (pins C4IO and F0IO are outputs)
					If no external C4IO and F0IO signal is provided
					this bit must be set for operation. */
#endif

	WRITE_REG(R_PCM_MD0, pcm_md0.reg);

	r_pcm_md2.reg = 0;
	if(master_mode == WANOPT_YES){
		CLOCK_FUNC();
			
		if(fe->bri_param.use_512khz_recovery_clock == 1){
			DEBUG_EVENT("%s: Module=%d Port=%d: using 512khz from PLL\n",
				fe->name, REPORT_MOD_NO(mod_no), fe_line_no_to_port_no(fe)+1);

			r_pcm_md2.bit.v_sync_out1 = 1;/* 1 = SYNC_O is either 512 kHz from the PLL or
							the received multiframe / superframe
							synchronization pulse. page 244. */
		}else{
			DEBUG_EVENT("%s: Module=%d Port=%d: SYNC_O -> SYNC_I / Sync Pulse.\n",
				fe->name, REPORT_MOD_NO(mod_no), fe_line_no_to_port_no(fe)+1);

			r_pcm_md2.bit.v_sync_out1 = 0;/* 0 = SYNC_O is either SYNC_I or the received
							synchronization pulse. page 244. */
		}
	}
	r_pcm_md2.bit.v_sync_out2 = 0;/* SYNC_O output selection
					0 = ST/Up receive from the selected line interface
					in TE mode (see R_SU_SYNC register for synchronization source selection)
					1 = SYNC_I is connected to SYNC_O. page 244. */
#if 0
	if(master_mode == WANOPT_NO){
		r_pcm_md2.bit.v_sync_src =	1;	/*V_SYNC_SRC PCM PLL synchronization source selection
							0 = line interface (see R_SU_SYNC for further
							synchronization configuration)
							1 = SYNC_I input (8 kHz). page 244.
							*/
	}
#endif

	WRITE_REG(R_PCM_MD2, r_pcm_md2.reg);
	/************************************************************************/

	return 0;
}

static int config_clock_routing(sdla_fe_t *fe, u8 master_mode)
{
	u8	mod_no;

	mod_no = fe_line_no_to_physical_mod_no(fe);	

	return __config_clock_routing(fe, mod_no, master_mode);
}


/******************************************************************************
*				init_xfhc()	
*
* Description	: Physical module global configuration. Should be done only one
*		  time per-module.
*		  Assuming chip was already reset.
*
* Arguments	:  pfe - pointer to Front End structure.
*		   mod_no - module number.
*
* Returns	:  0 - configred successfully, otherwise non-zero value.
*******************************************************************************/
static int32_t init_xfhc(sdla_fe_t *fe, u32 mod_no)
{
	sdla_bri_param_t	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	int32_t			err = 0, i;

	u8			port_no, bchan;
	reg_a_su_ctrl0		a_su_ctrl0;
	reg_a_su_rd_sta		a_su_rd_sta;
		
	DEBUG_HFC_INIT("%s(): mod_no: %d\n", __FUNCTION__, mod_no);

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return 1;
	}

	bri_module = &bri->mod[mod_no];

	DBG_MODULE_TESTER("read ChipID from Read Only register R_CHIP_ID: must be 0x61\n");

	if((err = reset_chip(fe, mod_no))){
		return err;
	}

	a_su_ctrl0.reg = 0;
	a_su_rd_sta.reg = 0;

	/* PCM: 1. Slave mode 2. PCM64 (4MBit/s data rate). */
	/* slow PCM adjust speed */
	bri_module->pcm_md1.bit.v_pll_adj = 3;
	bri_module->pcm_md1.bit.v_pcm_dr = 1; /* dr stands for 'data rate' (64) */ 

	WRITE_REG(R_PCM_MD0, bri_module->pcm_md0.reg + 0x90); /* get access to R_PCM_MD1 */
	WRITE_REG(R_PCM_MD1, bri_module->pcm_md1.reg);

	/* After chip reset SYNC_O is set for OUTPUT by default, make sure
	   SYNC_O is set for INPUT! Otherwise may cause clock conflict. */	
	__config_clock_routing(fe, mod_no, WANOPT_NO);

	DEBUG_HFC_INIT("\n%s: configuring B-channels FIFOs...\n", fe->name);
	/* configure B channel fifos for ST<->PCM data flow */
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {/* 2 ports */
		DEBUG_HFC_INIT("=========== port_no: %d ========\n", port_no);
		for (bchan = 0; bchan < 2; bchan++) {	/* 2 B channels on each port_no */
			DEBUG_HFC_INIT("port_no: %d, bchan: %d\n", port_no, bchan);
			/* B chan - Tx of port_no */
			xhfc_select_fifo(fe, mod_no, port_no*8 + bchan*2);
			/* ST-->PCM, channel enabled */
			WRITE_REG(A_CON_HDLC, 0xde);/* page 83: Tx, Transparent, ST/U-->PCM */
			/* 64kbit/s */
			WRITE_REG(A_SUBCH_CFG, 0);
			/* no interrupts */
			WRITE_REG(A_FIFO_CTRL, 0);
	
			/* B chan - Rx of port_no */
			xhfc_select_fifo(fe, mod_no, port_no*8 + bchan*2 + 1);
			/* ST<--PCM, channel enabled */
			WRITE_REG(A_CON_HDLC, 0xde);/* page 83: Rx, Transparent, ST/U<--PCM */
			/* 64kbit/s */
			WRITE_REG(A_SUBCH_CFG, 0);
			/* no interrupts */
			WRITE_REG(A_FIFO_CTRL, 0);
		}
	}
	DEBUG_HFC_INIT("\nDone\n");

	DEBUG_HFC_INIT("\nconfiguring D-channel FIFOs...\n");
	/* configure D channel fifos */
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {
		reg_a_con_hdlc	a_con_hdlc;

		a_con_hdlc.reg = 0;
			
		a_con_hdlc.bit.v_iff = 1;/* InterFrameFill=ones */
		/* Interrupt every 2^n bytes. n = V_FIFO_IRQ+2 in register A_CON_HDLC. p. 137 */
		/*a_con_hdlc.bit.v_fifo_irq = 3;*/	/* 2^(3+2) = 32 bytes an interrupt is generated */
		a_con_hdlc.bit.v_fifo_irq = 4;		/* 2^(4+2) = 64 bytes an interrupt is generated */

		DEBUG_HFC_INIT("=========== port_no: %d ========\n", port_no);
		/* D - Tx of port_no */
		xhfc_select_fifo(fe, mod_no, port_no*8 + 4);
		/* FIFO-->ST, channel enabled, IFF=ones */
		WRITE_REG(A_CON_HDLC, a_con_hdlc.reg);
		/* 16kbit/s */
		WRITE_REG(A_SUBCH_CFG, 2);
#if 0
		/* interrupts at end of frame only */
		WRITE_REG(A_FIFO_CTRL, 1);
#else
		/* Interrupts at end of frame AND at fifo threshold.
		   Will work as 'transmit interrupt'. */
		WRITE_REG(A_FIFO_CTRL, 0x5);
#endif

		/* D - Rx of port_no */
		xhfc_select_fifo(fe, mod_no, port_no*8 + 4 + 1);
		/* ST--> FIFO, channel enabled */
		WRITE_REG(A_CON_HDLC, a_con_hdlc.reg);
		/* 16kbit/s */
		WRITE_REG(A_SUBCH_CFG, 2);
#if 0
		/* interrupts at end of frame only */
		WRITE_REG(A_FIFO_CTRL, 1);
#else
		/* interrupts at end of frame AND at fifo threshold */
		WRITE_REG(A_FIFO_CTRL, 0x5);
#endif
	}
	DEBUG_HFC_INIT("\nDone\n");

	DEBUG_HFC_INIT("Configure PCM time slots\n");
	/*	Configure PCM time slots. 
		B-chan data will use PCM64 bus. D-chan data will use SPI. 
	*/
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {
		for (bchan = 0; bchan < 2; bchan++) {

			u8	pcm_slot;

			DEBUG_HFC_INIT("port_no: %d, bchan: %d\n", port_no, bchan);

			if(mod_no >= MAX_BRI_MODULES){
				/* adjust mod_no to be between 0 and 10 (including)*/
				pcm_slot = (u8)calculate_pcm_timeslot(mod_no - MAX_BRI_MODULES, port_no, bchan);
				/* AFT Line 1 will use odd PCM timeslots */
				pcm_slot += 1;
			}else{
				/* AFT Line 0 will use even PCM timeslots */
				pcm_slot = (u8)calculate_pcm_timeslot(mod_no, port_no, bchan);
			}

			DEBUG_HFC_INIT("selecting TX pcm_slot: %d\n", pcm_slot);

			/*****************************************************************************************/
			/*	transmit slot - select direction TX */
			xhfc_select_pcm_slot(fe, mod_no, pcm_slot, XHFC_DIRECTION_TX);

			/*	Connect time slot with channel and pin.
				PCM data output on pin STIO2 (+0x40 swap pins)
				0x80+0x40==0xC0==11000000 -> v_rout==0x3 -> "output buffer for STIO2 enabled" page 257.
				0*8 + 0*2 = 0 --> HFC channel 0.

				Assign HFC channel (from 0 to 15) to the selected PCM slot.
			*/
			WRITE_REG(A_SL_CFG,0x80+0x40+port_no*8+bchan*2); /* assign HFC channel (from 0 to 15) 
									to the selected PCM slot. */

			/*****************************************************************************************/
			/* receive slot - select direction RX */
			DEBUG_HFC_INIT("selecting RX pcm_slot: %d\n", pcm_slot);

			xhfc_select_pcm_slot(fe, mod_no, pcm_slot, XHFC_DIRECTION_RX);

			/*	Connect time slot with channel and pin.
				PCM data input from pin STIO1  (+0x40 swap pins).
				Assign HFC channel (from 0 to 15) to the selected PCM slot.
			*/

			WRITE_REG(A_SL_CFG,0x80+0x40+port_no*8+bchan*2+1);/* assign HFC channel (from 0 to 15) 
									  to the selected PCM slot. */
		}/* for (bchan = 0; bchan < 2; bchan++) */
	}/* for (port_no = 0; port_no < bri_module->num_ports; port_no++) */

	DBG_MODULE_TESTER("configure ST ports\n");
	/* configure ST ports */
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {
		u8 old_f0cl, new_f0cl;
		int32_t f0cl_diff;

		DEBUG_HFC_INIT("%s(): configurig ST port_no: %d\n", __FUNCTION__, port_no);

		WRITE_REG(R_SU_SEL, port_no);
		
		switch(bri_module->type)
		{
		case MOD_TYPE_TE:
			WRITE_REG(A_SU_CLK_DLY, CLK_DLY_TE);
			
			/* TE, ST B1+B2 tx enabled, end of pulse control enabled */
			WRITE_REG(A_SU_CTRL0, 0x43);
			break;

		case MOD_TYPE_NT:
			WRITE_REG(A_SU_CLK_DLY, CLK_DLY_NT);
			
			/* NT, ST B1+B2 tx enabled, end of pulse control enabled */
			a_su_ctrl0.bit.v_b1_tx_en = 1;		
			a_su_ctrl0.bit.v_b2_tx_en = 1;		
			a_su_ctrl0.bit.v_su_md = 1;
			WRITE_REG(A_SU_CTRL0, a_su_ctrl0.reg);
			break;
		}
		
		/* reset default. TE and NT */
		WRITE_REG(A_SU_CTRL1, 0x0);

		switch(bri_module->type)
		{
		case MOD_TYPE_NT:
			/* enables automatic transition G2->G3 */
			WRITE_REG(A_SU_CTRL1, M_G2_G3_EN);
			break;
		}

		/* ST B1+B2 rx enabled */
		WRITE_REG(A_SU_CTRL2, 0x3);
		
		/* end of pulse control for layer 1 compliance */
		WRITE_REG(A_ST_CTRL3, 0xf8);

		/* WRITE_REG(R_SU_SEL, port_no); */
#if defined(BUILD_MOD_TESTER)
		DBG_MODULE_TESTER("Activate ST port_no state machines (NT only!!)\n");
		/* try to activate port_no */
		switch(bri_module->type)
		{
		case MOD_TYPE_TE:
			/*WRITE_REG(A_SU_WR_STA, STA_ACTIVATE);*/
			break;
		case MOD_TYPE_NT:
			WRITE_REG(A_SU_WR_STA, STA_ACTIVATE | M_SU_SET_G2_G3);
			break;
		}
#endif
		DBG_MODULE_TESTER("poll R_F0_CNTL to make sure PCM is connected\n");

		/* poll R_F0_CNTL to make sure PCM is connected */
		for (i = 0; i < 10; i++) {	
		
			DBG_MODULE_TESTER("get current R_F0_CNTL\n");

			old_f0cl=READ_REG(R_F0_CNTL);
			DBG_MODULE_TESTER("wait for 10ms - f0cl should be incremented by 80 (+- 10 is ok)\n");
			/* wait for 10ms - f0cl should be incremented by 80 (+- 10 is ok) */
			WP_MDELAY(10);

			DBG_MODULE_TESTER("get the R_F0_CNTL after the wait\n");
			new_f0cl=READ_REG(R_F0_CNTL);
		
			if(check_f0cl_increment(fe, old_f0cl, new_f0cl, &f0cl_diff)){
				return 1;
			}
		}
	
		DEBUG_EVENT("%s: Module: %d, PCM 125us pulse ok. (f0cl diff: %d)\n", 
			fe->name, REPORT_MOD_NO(mod_no) + port_no, f0cl_diff);	

	}/* for (port_no = 0; port_no < bri_module->num_ports; port_no++) */

	/* init line interfaces state machines (in software only!) */
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {
	
		bri_module->port[port_no].idx = port_no;
		bri_module->port[port_no].hw = bri_module;
		
		/*wan_set_bit(HFC_L1_ACTIVATING, &bri_module->port[port_no].l1_flags);*/

	}/* for (port_no = 0; port_no < bri_module->num_ports; port_no++) */

#if defined(BUILD_MOD_TESTER)
	/* for debugging only - read line status */
	WP_MDELAY(300);

	DBG_MODULE_TESTER("read line status - Connected/Disconnected\n");
	/* read line status - Connected/Disconnected */
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {

		DBG_MODULE_TESTER("select port_no %d\n", port_no);
		WRITE_REG(R_SU_SEL, port_no);

		/* poll line status register for around 5ms */
		for (i = 0; i < 5; i++) {	
			u8	line_status;

			DBG_MODULE_TESTER("read ST line status for port_no %d\n", port_no);
			/* read ST line status*/
			a_su_rd_sta.reg = line_status = READ_REG(A_SU_RD_STA);
	
			if(bri_module->type == MOD_TYPE_TE){
				DEBUG_HFC_S0_STATES("%d: TE: force F7 - pt:%d line status: 0x%02x, v_su_fr_sync: %d\n",
					i, port_no, line_status, a_su_rd_sta.bit.v_su_fr_sync);
			}else{
				DEBUG_HFC_S0_STATES("%d: NT: force G3 - pt:%d line status: 0x%02x, v_su_fr_sync: %d\n", 
					i, port_no, line_status, a_su_rd_sta.bit.v_su_fr_sync);
			}

			DBG_MODULE_TESTER("a_su_rd_sta.bit.v_su_fr_sync: %d (0-Disconnected, 1-Connected)\n", a_su_rd_sta.bit.v_su_fr_sync);

			WP_MDELAY(200);
		}
	}
#endif
	DBG_MODULE_TESTER("%s(): finished\n", __FUNCTION__);

	return 0;
}

static int32_t check_f0cl_increment(sdla_fe_t *fe, u8 old_f0cl, u8 new_f0cl, int32_t *diff)
{
	*diff = new_f0cl - old_f0cl;
	if(*diff < 0){
		*diff = 255 - old_f0cl;
		*diff += new_f0cl;
	}

	/* should be between 70 and 90 over 10ms time */
	if(*diff == 0){
		DEBUG_ERROR("%s: PCM ERROR: BRI Modlue NO CLOCK found! 125us pulse f0cl diff: %d\n",
			fe->name, *diff);	
		return 1;
	}
	
	if(*diff > 150 || *diff < 70){
		DEBUG_WARNING("%s: PCM Warning 125us pulse count f0cl diff: %d\n",
			fe->name, *diff);	
	}

	DBG_MODULE_TESTER("f0cl diff: %d\n", *diff);
	return 0;
}

typedef enum _DCHAN_RC{
	DCHAN_STATUS_BUSY = 1,
	DCHAN_STATUS_COMPLETE,
	DCHAN_STATUS_INCOMPLETE,
	DCHAN_STATUS_EMPTY
}DCHAN_RC;

#define TX_EMPTY_FIFO	1

static u8 xhfc_write_fifo_dchan(sdla_fe_t *fe,	u8 mod_no,
			wp_bri_module_t *bri_module, bri_xhfc_port_t *port, 
			u8 *free_space)
{
	u8	*buf = NULL;
	int 	*len = NULL,
		*idx = NULL;
	u8	fcnt, tcnt, i;
	u8	free;
	u8	f1, f2;
	reg_a_fifo_sta	fstat;
	u8	*data;	
	u8	rc;
	sdla_t	*card = (sdla_t*)fe->card;
	private_area_t *chan=NULL;
	int	fifo_usage;

	buf = port->dtxbuf;		/* data buffer */
	len = &port->bytes2transmit;	/* hdlc packet len */
	idx = &port->dtx_indx;		/* already transmitted */

	DEBUG_HFC_TX("%s(): *len: %d\n", __FUNCTION__, *len);

	/* select the D-channel TX fifo */
	xhfc_select_fifo(fe, mod_no, (port->idx*8+4));

	fstat.reg = READ_REG(A_FIFO_STA);
	if (fstat.reg) WRITE_REG( A_INC_RES_FIFO, 8);

	free = (bri_module->max_z - (READ_REG( A_USAGE)));
	*free_space = free;
	
	TX_FAST_DBG("%s(): Line:%d: free: %d\n", __FUNCTION__, __LINE__, free);

	tcnt = ((free >= (*len - *idx)) ? (*len - *idx) : free);

	f1 = READ_REG( A_F1);
	f2 = READ_REG( A_F2);
	fcnt = 0x07 - ((f1 - f2) & 0x07);	/* free frame count in tx fifo */

	TX_FAST_DBG("%s(): free: %d, fcnt: %d, tcnt: %d\n", __FUNCTION__, free, fcnt, tcnt);

	TX_EXTRA_DBG("%s(): START: usage: 0x%X, z1: 0x%X z2: 0x%X f1: 0x%X: f2:0x%X f0c:0x%X\n", 
		__FUNCTION__, READ_REG( A_USAGE),READ_REG( A_Z1),READ_REG( A_Z2),f1,f2,READ_REG( R_F0_CNTL));

	if (free && fcnt && tcnt) {
		data = buf + *idx;
		*idx += tcnt;

		TX_EXTRA_DBG("%s(): tcnt: %d, *idx: %d, fstat.reg: 0x%X, fstat.bit.v_fifo_err: %d\n", 
			__FUNCTION__, tcnt, *idx, fstat.reg, fstat.bit.v_fifo_err);

		/* write data to FIFO */
		i=0;
		while (i<tcnt) {
			WRITE_REG(A_FIFO_DATA, *(data+i));
			i++;
		}

		/* terminate frame */
		if (*idx == *len) {
			xhfc_increment_fifo(fe, mod_no);/* incrementing fifo sends the frame out */
#if !TX_EMPTY_FIFO
			*len = 0;
			*idx = 0;
#endif			
			DEBUG_HFC_TX("%s(): finished frame transmission.\n", __FUNCTION__);
			rc = DCHAN_STATUS_COMPLETE;
#if !TX_EMPTY_FIFO
			chan=(private_area_t*)card->u.aft.dev_to_ch_map[BRI_DCHAN_LOGIC_CHAN];
			if (chan) {
				*len = 0;
				*idx = 0;
				TX_FAST_DBG("%s(): calling wanpipe_wake_stack()\n", __FUNCTION__);
				wanpipe_wake_stack(chan);
			}
#endif

		}else{
			xhfc_select_fifo(fe, mod_no, (port->idx*8+4));/* addition ?? */

			DEBUG_HFC_TX("%s(): transmitted a part of frame.\n", __FUNCTION__);
			rc = DCHAN_STATUS_INCOMPLETE;
		}
	}else{
		DEBUG_HFC_TX("%s(): NO free space in tx fifo!!\n", __FUNCTION__);
		rc = DCHAN_STATUS_BUSY; /* there is NO free space in tx fifo */
	}

	TX_EXTRA_DBG("%s(): END: eof usage: 0x%X, z1: 0x%X z2: 0x%X f1: 0x%X: f2:0x%X f0c:0x%X\n",
		__FUNCTION__, 
		READ_REG(A_USAGE), READ_REG( A_Z1), READ_REG(A_Z2),f1,f2,READ_REG( R_F0_CNTL));

#if TX_EMPTY_FIFO
	chan=(private_area_t*)card->u.aft.dev_to_ch_map[BRI_DCHAN_LOGIC_CHAN];

	/* make sure FIFO is empty before notifying the kernel about free TX space */
	fifo_usage = READ_REG(A_USAGE);
	TX_FAST_DBG("%s(): chan: 0x%p, TX FIFO usage: %d\n", __FUNCTION__, chan, fifo_usage);

	if (chan) {
		if(fifo_usage == 0){
			/* FIFO is empty */
			/*WP_DELAY(10000);*/
			*len = 0;
			*idx = 0;

			TX_FAST_DBG("%s(): calling wanpipe_wake_stack()\n", __FUNCTION__);
			wanpipe_wake_stack(chan);
		}
	}
#endif

	DEBUG_HFC_TX("%s(): returning: %d.\n", __FUNCTION__, rc);
	return rc;
}

/*
Transmit D channel frames.
Transmitting fifo data requires running PCM clocks with signal at C4IO and F0IO.
*/
static int32_t wp_bri_dchan_tx(sdla_fe_t *fe, void *src_data_buffer, u32 buffer_len)
{
	u8			mod_no, port_no, rc;
	sdla_bri_param_t	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	bri_xhfc_port_t		*port_ptr;
	u8			free_space;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	DEBUG_TX_DATA("%lu: %s(): Module: %d, port_no: %d. fe->name: %s blen=%i\n",
		jiffies, __FUNCTION__, mod_no, port_no, fe->name, buffer_len);

#if defined(BUILD_MOD_TESTER)
	{
		u32	i;
		u8	*tmp = (u8*)src_data_buffer;

		DEBUG_TX_DATA("TX data:\n");

		for(i = 0; i < buffer_len; i++) {
			_DEBUG_EVENT("%02x ", tmp[i]);
			if(i && ((i % 20) == 0)){
				DEBUG_EVENT("\n");
			}
		}
		DEBUG_EVENT("\n");
	}
#endif

	bri_module = &bri->mod[mod_no];
	port_ptr = &bri_module->port[port_no];

	if(src_data_buffer == NULL){
		/* Caller is interested in tx buffer space, 
		   return how many bytes is still left to transmit */
		return port_ptr->bytes2transmit;
	}

	if(MAX_DFRAME_LEN_L1 <= buffer_len){
		DEBUG_EVENT("%s: %s(): Tx data length %d exceeds maximum of %d bytes!\n",
			fe->name, __FUNCTION__, buffer_len, MAX_DFRAME_LEN_L1);
		return -EINVAL;
	}

	if(port_ptr->bytes2transmit){
		/* still transmitting previous frame */
		return -EBUSY;
	}
/*
	{
		int ind;
		u8 * u8_src_data_buffer = (u8*)src_data_buffer;

		for(ind = 0; ind < buffer_len; ind++){
			if(u8_src_data_buffer[ind] != ind){
				DEBUG_RX1("tx: u8_src_data_buffer[0x%x] is: 0x%x != 0x%x!!\n", 
					ind, u8_src_data_buffer[ind], ind);
			}
		}
	}
*/

	memcpy(port_ptr->dtxbuf, src_data_buffer, buffer_len);
	port_ptr->bytes2transmit = buffer_len;
		
	rc = xhfc_write_fifo_dchan(fe, mod_no, bri_module, port_ptr, &free_space);

	/* The frame was accepted for transmission.
	 * Return 0 - Successful tx but now queue is full.
	 * Note that the frame maybe actually transmitted in parts, depending on the length. 
	 */
	return 0;
}

#if CHECK_DATA
static void dump_chip_SRAM(sdla_fe_t *fe, u8 mod_no, u8 port_no)
{
	u8 db;
	u16 a;
	/*
	XHFC dump of internal RAM
	
	the dump shows content of internal array registers
	Z and F counters and other internal variables
	
	please select address range for D channel that shows the errors

	128 bytes address range for receive buffer of channel  5, D rx port_no 0
	*/
	/* wrong offset
	u16 start_addr = 0x580;
	u16 end_addr   = 0x600;
	*/
	u16 start_addr = 0x280;
	u16 end_addr   = 0x300;

/*
	128 bytes address range for receive buffer of channel 13, D rx port_no 1
	u16 start_addr = 0xd80;
	u16 end_addr   = 0xe00;
*/
	for (a = start_addr; a < end_addr; a++)
	{
		WRITE_REG(R_RAM_ADDR, (a & 0xff));
		WRITE_REG(R_RAM_CTRL, ((a >> 8) & 0xff));
		db=READ_REG(R_RAM_DATA);

		/* dummy read to add some delay */
		/*db=READ_REG(R_INT_DATA);
		db=READ_REG(R_INT_DATA);*/

		if((a % 16) == 0){
			_DEBUG_EVENT("\n %04x",a);
		}
		_DEBUG_EVENT(" %02x",db);
	}
	DBG_RX_DATA("\n");
}

static int check_data(u8 *data, int data_len)
{
	int i, rc = 0;

	/*DBG_RX_DATA("%s(): data_len: %d\n", __FUNCTION__, data_len);*/

	for(i = 0; i < data_len; i++){
		if(data[i] == 0xFF){
			rc = 1;
			break;
		}
	}
	return rc;
}

static void dump_data(u8 *data, int data_len)
{
	int i;

	DBG_RX_DATA("%s(): data_len: %d\n", __FUNCTION__, data_len);
	
	for(i = 0; i < data_len; i++){
		if((i%16) == 0){
			_DEBUG_EVENT("\n %04X", i);
		}

		_DEBUG_EVENT(" %02X", data[i]);

		if (i == (data_len - 4)){
			printk (" -");
		}
	}
	_DEBUG_EVENT("\n");
}
#endif/* CHECK_DATA */

static int xhfc_read_fifo_dchan(sdla_fe_t *fe,	u8 mod_no,
			wp_bri_module_t *bri_module, bri_xhfc_port_t *port, int *rx_data_len)
{
	u8	*buf = port->drxbuf;
	int	*idx = &port->drx_indx;
	u8	*data; /* pointer for new data */
	u8	rcnt, i;
	u8	f1=0, f2=0, z1=0, z2=0;

	/* select D-RX fifo */
	xhfc_select_fifo(fe, mod_no, (port->idx * 8) + 5);

	/* hdlc rcnt */
	f1 = READ_REG( A_F1);
	f2 = READ_REG( A_F2);
	z1 = READ_REG( A_Z1);
	z2 = READ_REG( A_Z2);

	rcnt = (z1 - z2) & bri_module->max_z;
	if (f1 != f2)
		rcnt++;	

	/* debug message of F and Z counters */
	RX_EXTRA_DBG("%s(): START: usage: 0x%X, z1: 0x%X z2: 0x%X f1: 0x%X: f2:0x%X f0c:0x%X\n",
		__FUNCTION__, 
		READ_REG(A_USAGE), READ_REG( A_Z1), READ_REG(A_Z2),f1,f2,READ_REG( R_F0_CNTL));

	if (rcnt > 0) {
		data = buf + *idx;
		*idx += rcnt;
		if(*idx >= MAX_DFRAME_LEN_L1){
			if (0 && WAN_NET_RATELIMIT()) {
				DEBUG_EVENT("%s: %s(): frame in mod_no %d, port_no %d > maximum size of %d bytes!\n",
					fe->name, __FUNCTION__, mod_no, port->idx, MAX_DFRAME_LEN_L1);
			}
			*idx = 0;
			*rx_data_len = 0;
			return DCHAN_STATUS_EMPTY;
		}

		DEBUG_HFC_RX("rcnt: %d\n", rcnt);
		/* read data from FIFO */
		i=0;
		while (i < rcnt) {
			*(data+i) = READ_REG(A_FIFO_DATA);
			i++;
		}

		/* hdlc frame termination */	
		if (f1 != f2) {
			xhfc_increment_fifo(fe, mod_no);
			/* check minimum frame size */
			if (*idx < 4) {
				if (0 && WAN_NET_RATELIMIT()) {
					DEBUG_EVENT("%s: %s(): frame in mod_no %d, port_no %d < minimum size of 4 bytes!\n",
							  fe->name, __FUNCTION__, mod_no, port->idx);
				}
				*idx = 0;
				*rx_data_len = 0;
				return DCHAN_STATUS_EMPTY;
			}

			/* check crc */
			if (buf[(*idx) - 1]) {
				if (0 && WAN_NET_RATELIMIT()) {
					DEBUG_ERROR("%s: %s(): CRC-error in frame in mod_no %d, port_no %d!\n",
							  fe->name, __FUNCTION__, mod_no, port->idx);
				}
				*idx = 0;
				*rx_data_len = 0;
				return DCHAN_STATUS_EMPTY;
			}

			/* D-Channel debug to syslog */
			DBG_RX_DATA("%lu:D-RX len(%02i):\n", jiffies, (*idx));
#if 0
			i = 0;
			while(i < (*idx)){
				printk("%02x ", buf[i++]);
				if (i == (*idx - 3)){
					printk ("- ");
				}
			}
			printk("\n");
#endif
			*rx_data_len = *idx - 3;/* discard CRC and STATUS - 3 bytes */
						/* STATUS is the last byte of a frame in the fifo
						   which is used to check if CRC is correct or not.

				After the ending flag of a frame the XHFC-2SU checks the HDLC CRC checksum.
				If it is correct one byte with all �0�s is inserted behind
				the CRC data in the FIFO named STAT (see Fig. 4.2). This last byte of a frame in the FIFO is different
				from all �0�s if there is no correct CRC field at the end of the frame.
				If the STAT value is 0xFF, the HDLC frame ended with at least 8 bits �1�s. This is similar to an abort
				HDLC frame condition.
				The ending flag of a HDLC frame can also be the starting flag of the next frame.
				page 122.
			*/
#if CHECK_DATA
			if(check_data(port->drxbuf, *rx_data_len)){
				dump_chip_SRAM(fe, mod_no, port->idx);
				dump_chip_SRAM(fe, mod_no, port->idx);
				reset_chip(fe, mod_no);
				dump_data(port->drxbuf, *idx);
			}
#endif
			*idx = 0;

			RX_EXTRA_DBG("%s(): END: eof usage: 0x%X, z1: 0x%X z2: 0x%X f1: 0x%X: f2:0x%X f0c:0x%X\n",
				__FUNCTION__, 
				READ_REG(A_USAGE), READ_REG( A_Z1), READ_REG(A_Z2),f1,f2,READ_REG( R_F0_CNTL));

			DEBUG_HFC_RX("%s(): finished receiving a frame.\n", __FUNCTION__);
			return DCHAN_STATUS_COMPLETE;
		}else{

			RX_EXTRA_DBG("%s(): END: eof usage: 0x%X, z1: 0x%X z2: 0x%X f1: 0x%X: f2:0x%X f0c:0x%X\n",
				__FUNCTION__, 
				READ_REG(A_USAGE), READ_REG( A_Z1), READ_REG(A_Z2),f1,f2,READ_REG( R_F0_CNTL));

			DEBUG_HFC_RX("%s(): received a part of frame.\n", __FUNCTION__);
			return DCHAN_STATUS_INCOMPLETE;
		}
	}else{

		RX_EXTRA_DBG("%s(): END: eof usage: 0x%X, z1: 0x%X z2: 0x%X f1: 0x%X: f2:0x%X f0c:0x%X\n",
				__FUNCTION__, 
				READ_REG(A_USAGE), READ_REG( A_Z1), READ_REG(A_Z2),f1,f2,READ_REG( R_F0_CNTL));

		DEBUG_HFC_RX("%s(): RX FIFO is empty!\n", __FUNCTION__);
		return DCHAN_STATUS_EMPTY;
	}
}


static void *wp_bri_dchan_rx(sdla_fe_t *fe, u8 mod_no, u8 port_no)
{
	sdla_bri_param_t	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	bri_xhfc_port_t		*port_ptr;
	netskb_t		*skb;
	u8			*skb_data_area;
	int			rc, rx_data_len = 0;
	
	/* Note: D channel is slow (less than 2 bytes / ms!!) */
	DEBUG_HFC_RX("%s(): line: %d, mod_no: %d port_no: %d\n", __FUNCTION__, __LINE__, mod_no, port_no);

	bri_module = &bri->mod[mod_no];
	port_ptr = &bri_module->port[port_no];

	rc = xhfc_read_fifo_dchan(fe, mod_no, bri_module, port_ptr, &rx_data_len);

	skb = NULL;
	if((rc == DCHAN_STATUS_COMPLETE) && (rx_data_len < MAX_DFRAME_LEN_L1) && (rx_data_len > 0)){
		/**************************/
		skb = wan_skb_alloc(rx_data_len);
		if(skb == NULL){
			return NULL;
		}
	
		skb_data_area = wan_skb_put(skb, rx_data_len);
		memcpy(skb_data_area, port_ptr->drxbuf, rx_data_len);
#if 0
		{
			int ind;
			for(ind = 0; ind < rx_data_len; ind++){
				if(skb_data_area[ind] != ind){
					DEBUG_RX1("rx: skb_data_area[0x%x] is: 0x%x != 0x%x!!\n", 
						ind, skb_data_area[ind], ind);
				}
			}
		}
#endif
	}

	return skb;
}


/******************************************************************************
** wp_bri_iface_init) - 
**
**	OK
*/
int32_t wp_bri_iface_init(void *pfe_iface)
{
	sdla_fe_iface_t	*fe_iface = (sdla_fe_iface_t*)pfe_iface;

	BRI_FUNC();
	fe_iface->global_config		= &bri_global_config;	/* not used in BRI */
	fe_iface->global_unconfig	= &bri_global_unconfig;	/* not used in BRI */

	fe_iface->config		= &wp_bri_config;
	fe_iface->unconfig		= &wp_bri_unconfig;

	fe_iface->post_unconfig		= &wp_bri_post_unconfig;

	fe_iface->post_init		= &wp_bri_post_init;

	fe_iface->if_config		= &wp_bri_if_config;
	fe_iface->if_unconfig		= &wp_bri_if_unconfig;

	fe_iface->active_map		= &wp_bri_active_map;

	fe_iface->set_fe_status		= &wp_bri_set_fe_status;
	fe_iface->get_fe_status		= &wp_bri_get_fe_status;

	fe_iface->isr			= &wp_bri_intr;

	fe_iface->disable_irq		= &wp_bri_disable_fe_irq; 

	fe_iface->check_isr		= &wp_bri_check_intr;

	fe_iface->polling		= &wp_bri_polling;
	fe_iface->process_udp		= &wp_bri_udp;
	fe_iface->get_fe_media		= &wp_bri_fe_media;

	fe_iface->set_dtmf		= &wp_bri_set_dtmf;
	fe_iface->intr_ctrl		= &wp_bri_intr_ctrl;
	fe_iface->event_ctrl		= &wp_bri_event_ctrl;

	fe_iface->isdn_bri_dchan_tx	= &wp_bri_dchan_tx;

	fe_iface->clock_ctrl		= &config_clock_routing;

	return 0;
}

/* Should be done only ONCE per card. */
static int32_t wp_bri_spi_bus_reset(sdla_fe_t	*fe)
{
	sdla_t	*card = (sdla_t*)fe->card;

	BRI_FUNC();

	DEBUG_EVENT("%s: Executing SPI bus reset....\n", fe->name);

	card->hw_iface.bus_write_4(	card->hw,
					SPI_INTERFACE_REG,
					MOD_SPI_RESET);
	WP_DELAY(1000);
	card->hw_iface.bus_write_4(	card->hw,
					SPI_INTERFACE_REG,
					0x00000000);
	WP_DELAY(1000);
	return 0;
}

/******************************************************************************
*				scan_modules()	
*
* Description	: Scan for installed modules.
*
* Arguments	: pfe - pointer to Front End structure.	
*
* Returns	: number of discovered modules.
*******************************************************************************/
static u_int8_t scan_modules(sdla_fe_t *fe, u_int8_t rm_no)
{
	u_int8_t mod_no = RM_BRI_STATUS_READ/*0x3*/;/* to read remora status register ALWAYS put 0x3 into mod_addr. */
	u_int8_t value, ind, mod_counter = 0;
	u_int8_t mod_no_index;	/* index in the array of ALL modules (NOT lines) on ALL remoras. From 0 to 11 */

	/* format of remora status register: 
		bit 0 == 1	- module 1 active (exist)
		bit 1		- type of module 1 (0 - NT, 1 - TE)

		bit 2 == 1	- module 2 active (exist)
		bit 3		- type of module 1 (0 - NT, 1 - TE)

  		bit 4 == 1	- module 3 active (exist)
		bit 5		- type of module 1 (0 - NT, 1 - TE)

		bit 6,7		- has to be zeros for active remora. if non-zero, remora does not exist.
	*/

	value = fe->read_fe_reg(fe->card, 
				mod_no,
				MOD_TYPE_NONE,	
				rm_no,
				0);

#define MODULE1	0
#define MODULE2	2
#define MODULE3	4

	DEBUG_BRI_INIT("remora number: %d, remora status register: 0x%02X\n", rm_no, value);

	if((value >> 6) == 0x2){
		DEBUG_EVENT("%s: Remora number %d: Found 512khz Recovery clock remora.\n", fe->name, rm_no);
		fe->bri_param.use_512khz_recovery_clock = 1;
	}else{

		if(((value >> 7) & 0x01)){
			DEBUG_EVENT("%s: Remora number %d does not exist.\n", fe->name, rm_no);
			return 0;
		}else{
			DEBUG_EVENT("%s: Remora number %d exist.\n", fe->name, rm_no);
		}
	}

	for(ind = 0; ind < 6; ind++){

		switch(ind){

		case MODULE1:
		case MODULE2:
		case MODULE3:
			DEBUG_BRI_INIT("module Number on REMORA (0-2): %d\n", ind / 2);

			/* 0-11, all (even and odd) numbers */
			mod_no_index = rm_no * MAX_BRI_MODULES_PER_REMORA + ind / 2;
			DEBUG_BRI_INIT("mod_no_index on CARD (should be 0-11): %d\n", mod_no_index);

			/* 0-23, only even numbers */
			mod_no_index = mod_no_index * 2;
			DEBUG_BRI_INIT("mod_no_index (line number) on CARD (should be 0-23): %d\n", mod_no_index);

			if(mod_no_index >= MAX_BRI_LINES){
					DEBUG_ERROR("%s: Error: Module %d/%d exceeds maximum (%d)\n",
						fe->name, mod_no_index, mod_no_index, MAX_BRI_LINES);
				return 0;
			}

			if((value >> ind) & 0x01){

				mod_counter++;

				if((value >> (ind + 1)) & 0x01){
					
					DEBUG_BRI_INIT("%s: Module %d type is: TE\n",
						fe->name, mod_no_index);

					fe->bri_param.mod[mod_no_index].type = MOD_TYPE_TE;

					fe->bri_param.mod[mod_no_index].port[PORT_0].mode |= PORT_MODE_TE;
					fe->bri_param.mod[mod_no_index].port[PORT_1].mode |= PORT_MODE_TE;

				}else{
					DEBUG_BRI_INIT("%s: Module %d type is: NT\n",
						fe->name, mod_no_index);

					fe->bri_param.mod[mod_no_index].type = MOD_TYPE_NT;

					fe->bri_param.mod[mod_no_index].port[PORT_0].mode |= PORT_MODE_NT;
					fe->bri_param.mod[mod_no_index].port[PORT_1].mode |= PORT_MODE_NT;
				}

				/*Copy information from (even numbered) 'mod_no_index' to 
				(odd numbered) 'mod_no_index + 1' because for each module there are two lines.*/
				memcpy(&fe->bri_param.mod[mod_no_index + 1], &fe->bri_param.mod[mod_no_index], 
					sizeof(wp_bri_module_t));
			}else{
				DEBUG_BRI_INIT("%s: Module %d is not installed\n",
						fe->name, mod_no_index);
			}
			DEBUG_BRI_INIT("=================================\n\n");
			break;
		}/* switch() */
	}/* for() */

	return mod_counter;
}

/******************************************************************************
*				scan_remoras_and_modules()	
*
* Description	: Scan for installed Remoras and modules.
*
* Arguments	: pfe - pointer to Front End structure.	
*
* Returns	: 0 - configred successfully, otherwise non-zero value.
*******************************************************************************/
static int32_t scan_remoras_and_modules(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	u8		rm_no, modules_counter = 0;

	BRI_FUNC();

	for(rm_no = 0; rm_no < MAX_BRI_REMORAS; rm_no++){

		modules_counter += scan_modules(fe, rm_no);
	}

	if(modules_counter == 0){
		DEBUG_ERROR("%s: Error: modules counter is zero!\n", fe->name);
		return 1;
	}else{
		DEBUG_EVENT("%s: Total number of modules: %d.\n", fe->name, modules_counter);
		return 0;
	}
}

/******************************************************************************
*				bri_global_config()	
*
* Description	: Global configuration for Sangoma BRI board.
*
* 		Notes	: 1. This routine runs only ONCE for a physical 'base' CARD,
*					 not for 'additional' cards. 
*				  2. reset card's SPI.
*				  3. Scan for installed Remoras and modules.
*
* Arguments	: pfe - pointer to Front End structure.	
*
* Returns	: 0 - configred successfully, otherwise non-zero value.
*******************************************************************************/
static int32_t bri_global_config(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	BRI_FUNC();

	DEBUG_EVENT("%s: %s Global Front End configuration\n", 
			fe->name, FE_MEDIA_DECODE(fe));

	return 0;
}

/*******************************************************************************
*				bri_global_unconfig()	
*
* Description	: Global un-configuration for Sangoma BRI board.
* 			Note: This routne runs only ONCE for a physical card.
*
* Arguments	:	
*
* Returns	: 0
*******************************************************************************/
static int32_t bri_global_unconfig(void* pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	BRI_FUNC();

	DEBUG_EVENT("%s: %s Global Front End unconfiguration!\n",
				fe->name, FE_MEDIA_DECODE(fe));

	return 0;
}


/*
 ******************************************************************************
 *			wp_bri_post_unconfig()	
 *
 * Description: Free resources used by 'fe'. This function is NOT locked and
 *				it must NOT be called more than one time.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int wp_bri_post_unconfig(void* pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;
	wp_bri_module_t		*bri_module;
	sdla_bri_param_t	*bri = &fe->bri_param;
	u8			mod_no, port_no;
	bri_xhfc_port_t		*port_ptr;

	if (!wan_test_bit(WP_BCB_BRI_POST_INIT, &fe->bri_param.critical)) {
		return 1;
	}

	if(validate_fe_line_no(fe, __FUNCTION__)){
		return 1;
	}

	DEBUG_EVENT("%s: Running post-unconfig...\n", fe->name);

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	DEBUG_HFC_INIT("%s(): mod_no: %i, port_no: %i\n", __FUNCTION__, mod_no, port_no);
	
	bri_module = &bri->mod[mod_no];

	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {
		port_ptr = &bri_module->port[port_no];
	
		wan_del_timer(&port_ptr->t3_timer);
		wan_del_timer(&port_ptr->t4_timer);
		wan_del_timer(&port_ptr->t1_timer);		

	}/* for (port_no = 0; port_no < bri_module->num_ports; port_no++) */

	wan_clear_bit(WP_BCB_BRI_POST_INIT, &fe->bri_param.critical);

	return 0;
}

/******************************************************************************
*			wp_bri_config() - initialise the XHFC ISDN Chip.
*
* Description	:  Configure the PHYSICAL module ONE time. 
*		   On each module 2 lines will be configured 
*		   in exactly the same way.
*
* Arguments	:  pfe - pointer to Front End structure.
*
* Returns	:  0 - configred successfully, otherwise non-zero value.
*******************************************************************************/
static int32_t wp_bri_config(void *pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;
	sdla_t 			*card = (sdla_t*)fe->card;
	u8			mod_no, port_no, mod_cnt = 0;
	/*sdlahw_t* 		hw = (sdlahw_t*)card->hw;*/
	int32_t			err = 0, aft_line_no = 0;
	u16			physical_module_config_counter;
	u32			physical_card_config_counter;
	wp_bri_module_t		*bri_module;
	sdla_bri_param_t	*bri = &fe->bri_param;

	BRI_FUNC();

	if (wan_test_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical)) {
		DEBUG_ERROR("%s: %s: Error: Line already configred! Line number %i !\n", 
			fe->name, FE_MEDIA_DECODE(fe), WAN_FE_LINENO(fe)+1);
		return 1;
	}

	fe->fe_status = FE_UNITIALIZED;

	if(validate_fe_line_no(fe, __FUNCTION__)){
		DEBUG_ERROR("%s: %s: Error: Invalid Front End Line number %i !\n", 
			fe->name, FE_MEDIA_DECODE(fe), WAN_FE_LINENO(fe)+1);
		return 1;
	}

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	
	DEBUG_EVENT("%s: %s: Front End configuration Line=%d Mod=%i\n", 
			fe->name, FE_MEDIA_DECODE(fe), WAN_FE_LINENO(fe) + 1, mod_no);

	DEBUG_TEST("%s(): mod_no: %i, port_no: %i\n", __FUNCTION__, mod_no, port_no);
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	fe->bri_param.max_fe_channels 	= MAX_TIMESLOTS;
	fe->bri_param.module_map[0]	= fe->bri_param.module_map[1] = 0;
		
	bri_module = &bri->mod[mod_no];

	card->hw_iface.getcfg(card->hw, SDLA_HWTYPE_USEDCNT, &physical_card_config_counter);

	if(physical_card_config_counter == 1){
		/* Per-card initialization. Important to do only ONCE.*/
		wp_bri_spi_bus_reset(fe);
	}

	fe->bri_param.use_512khz_recovery_clock = 0;

	if(scan_remoras_and_modules(fe)){
		return 1;
	} 

#if defined(BUILD_MOD_TESTER)
	/* for Production Test nothing else should be done */
	return 0;
#endif

	/*Event lock */
	WAN_LIST_INIT(&fe->event);
	/* wan_spin_lock_init(&fe->lock, "wp_bri_lock"); FIXME: 'lock' is not there, but what is there? */

	card->hw_iface.getcfg(card->hw, SDLA_HWLINEREG, &physical_module_config_counter);

	bri_module->fe = fe;

	switch(fe->bri_param.mod[mod_no].type)
	{
	case MOD_TYPE_TE:
	case MOD_TYPE_NT:
		if(physical_module_config_counter == 1){

			if((err = init_xfhc(fe, mod_no))){
				return err;
			}
		}else{
			bri_module->num_ports = BRI_MAX_PORTS_PER_CHIP;
			bri_module->max_fifo = 8;
			bri_module->max_z = 0x7F;

			/* init line interfaces state machines (in software only!) */
			for (port_no = 0; port_no < bri_module->num_ports; port_no++) {
				
				bri_module->port[port_no].idx = port_no;
				bri_module->port[port_no].hw = bri_module;
				
			}/* for (port_no = 0; port_no < bri_module->num_ports; port_no++) */
		}
		break;

	default:
		DEBUG_ERROR("%s(): %s: Error: Module %d (AFT Line: %d): Not Installed!!\n",
			__FUNCTION__, fe->name, REPORT_MOD_NO(mod_no), aft_line_no);
		return 1;
	}
	
	/**************************************************************************/
	/* Set active modules (channels) bitmap for all installed LOGICAL modules */

#define BCHAN_BITMAP	0x3 /* each BRI line has 2 b-channels --> 2 bits */

	if(mod_no < (MAX_BRI_LINES / 2)){
		/* 1-st aft line has timeslots 0-23 */
		aft_line_no = 0;
	}else{
		/* 2-nd aft line has timeslots 24-47 */
		aft_line_no = 1;
	}

	fe->bri_param.mod[mod_no].mod_no = mod_no;

	switch(fe->bri_param.mod[mod_no].type)
	{
	case MOD_TYPE_TE:
	case MOD_TYPE_NT:
		fe->bri_param.module_map[aft_line_no] |= (BCHAN_BITMAP << (mod_no*2));

		DEBUG_EVENT("%s: Module %d (AFT Line: %d): Installed -- %s. Timeslots map: 0x%08X\n",
				fe->name, REPORT_MOD_NO(mod_no), aft_line_no,
				WP_BRI_DECODE_MOD_TYPE(fe->bri_param.mod[mod_no].type),
				(BCHAN_BITMAP << (mod_no*2)));	
		mod_cnt++;
		break;

	default:
		DEBUG_WARNING("%s(): %s: Warning: Module %d (AFT Line: %d): Not Installed.\n",
			__FUNCTION__, fe->name, REPORT_MOD_NO(mod_no), aft_line_no);	
		break;
	}

	/*******************************************************/
	/* Enable interrupts on the installed PHYSICAL module. */
	switch(fe->bri_param.mod[mod_no].type)
	{
	case MOD_TYPE_TE:
	case MOD_TYPE_NT:
		if(physical_module_config_counter == 1){
/* DAVIDR-consider moving call bri_enable_interrupts() to wp_bri_post_init() */
			bri_enable_interrupts(fe, mod_no, port_no);
		}else{
			DEBUG_HFC_INIT("%s(): the other port_no is ALREADY running, interrupts are enabled.\n",
				__FUNCTION__);
		}
		break;
	default:
		DEBUG_WARNING("%s(): %s: Warning: Module %d (AFT Line: %d): Not Installed.\n",
			__FUNCTION__, fe->name, REPORT_MOD_NO(mod_no), aft_line_no);	
		break;
	}

	/******************************************************/
	/*------------------ CLOCK RECOVERY ------------------*/
	if(physical_card_config_counter == 1){
		/* Per-card initialization. Important to do only ONCE.*/
		u32 i;

		/* ALL modules MUST have GPIO6 function switched
		   from the default 'output' to 'input'.
		   Otherwise there will be a clock conflict!*/

		for(i = 0; i < MAX_BRI_LINES; i += BRI_MAX_PORTS_PER_CHIP){

			switch(fe->bri_param.mod[i].type)
			{
			case MOD_TYPE_TE:
			case MOD_TYPE_NT:
				__config_clock_routing(fe, i, WANOPT_NO);
				break;
			default:
				continue;
			}/* switch() */
		}/* for() */
	}/* if() */
	/*----------- END OF CLOCK RECOVERY ------------------*/
	/******************************************************/

	fe->fe_status = FE_DISCONNECTED;

	wan_set_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical);

	return 0;
}

/******************************************************************************
** wp_bri_unconfig() - 
**
**	OK
*/
static int32_t wp_bri_unconfig(void *pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	u8		mod_no, port_no;
	u16		physical_module_config_counter;
	sdla_t		*card = (sdla_t*)fe->card;
	sdla_bri_param_t *bri = &fe->bri_param;
	wp_bri_module_t	*bri_module;
	bri_xhfc_port_t	*port_ptr;

	if(validate_fe_line_no(fe, __FUNCTION__)){
		return 1;
	}

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	DEBUG_HFC_INIT("%s(): mod_no: %d, port_no: %d\n", __FUNCTION__, mod_no, port_no);

	/* Check if port was configured. If not, return. */
	if (!wan_test_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical)) {
		DEBUG_BRI("%s: %s(): Warning: Front End initialization was incomplete OR not a first call to UnConfig!\n", 
			fe->name, __FUNCTION__);
		return 1;
	}

	DEBUG_EVENT("%s: Unconfiguring BRI Front End...\n", fe->name);

	bri_module = &bri->mod[mod_no];
	port_ptr   = &bri_module->port[port_no];

	card->hw_iface.getcfg(card->hw, SDLA_HWLINEREG, &physical_module_config_counter);

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	/******************************************************************/
	switch(fe->bri_param.mod[mod_no].type)
	{
	case MOD_TYPE_TE:
	case MOD_TYPE_NT:
		
		if(physical_module_config_counter == 1){
			wp_bri_disable_irq(fe, mod_no, port_no);
		}else{
			DEBUG_HFC_INIT("%s(): the other port_no is still running, leave interrupts enabled.\n",
				__FUNCTION__);
		}
		break;
	default:
		/* for missing (not installed) modules - do nothing  */
		DEBUG_EVENT("%s(): %s: Warining: unknown module type!\n", 
			__FUNCTION__, fe->name);
		break;
	}

#if 1
	{
	reg_a_su_wr_sta	a_su_wr_sta;
	/******************************************************************/
	/* When we stop a port (because not used anymore), force the 
	deactivation of the line interface by writing the deactivated state
	into the A_SU_WR_STA register.
	For a port that is configured in NT mode, write 0x11 to A_SU_WR_STA (force G1) 
	and for a TE port write 0x13 (force F3) to this register.
	In these states the port will send only INFO0 (no signal) and no 
	line interface state change interrupts will be generated. */
	WRITE_REG(R_SU_SEL, port_no);

	DEBUG_HFC_INIT("%s(): port_no: %i, port_ptr: 0x%p, port_ptr->mode: %i\n", __FUNCTION__, port_no, port_ptr, port_ptr->mode);

	a_su_wr_sta.reg = 0;
	if (port_ptr->mode & PORT_MODE_TE) {
		/* TE to F3 */
		a_su_wr_sta.bit.v_su_set_sta = 0x3;
		a_su_wr_sta.bit.v_su_ld_sta  = 0x1;
	}else{
		/* NT to G1 */
		a_su_wr_sta.bit.v_su_set_sta = 0x1;
		a_su_wr_sta.bit.v_su_ld_sta  = 0x1;
	}

	WRITE_REG(A_SU_WR_STA, a_su_wr_sta.reg);
	/* Immediatly after that we get SU state interrupt - clear it! (for all ports) */
	/*READ_REG(R_SU_IRQ);*/
	}
#endif
	/******************************************************************/
	
	fe->fe_status = FE_UNITIALIZED;
	
	wan_clear_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical);

	return 0;
}

/******************************************************************************
** wp_bri_post_init() - 
**
**	OK
*/
static int32_t wp_bri_post_init(void *pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;
	wp_bri_module_t		*bri_module;
	sdla_bri_param_t	*bri = &fe->bri_param;
	u8			mod_no, port_no;
	bri_xhfc_port_t		*port_ptr;

	if (wan_test_bit(WP_BCB_BRI_POST_INIT, &fe->bri_param.critical)) {
		return 1;
	}

	if(validate_fe_line_no(fe, __FUNCTION__)){
		return 1;
	}

	DEBUG_EVENT("%s: Running post initialization...\n", fe->name);

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	DEBUG_HFC_INIT("%s(): mod_no: %i, port_no: %i\n", __FUNCTION__, mod_no, port_no);
	
	bri_module = &bri->mod[mod_no];

	/* */
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {
		port_ptr = &bri_module->port[port_no];

		wan_init_timer(&port_ptr->t3_timer, l1_timer_expire_t3, (wan_timer_arg_t)port_ptr);
		wan_init_timer(&port_ptr->t4_timer, l1_timer_expire_t4, (wan_timer_arg_t)port_ptr);
		wan_init_timer(&port_ptr->t1_timer, l1_timer_expire_t1, (wan_timer_arg_t)port_ptr);
	}/* for (port_no = 0; port_no < bri_module->num_ports; port_no++) */

#if 1
	{
		wan_smp_flag_t	smp_flags1;
		sdla_t		*card = (sdla_t*)fe->card;

		/* Try to activate the port_no - if cable is in, line will get activated.
		   If no cable, the application will have to call wp_bri_set_fe_status()
		   to get line activated. */
		card->hw_iface.hw_lock(card->hw,&smp_flags1);   
		wp_bri_set_fe_status(fe, WAN_FE_CONNECTED);
		card->hw_iface.hw_unlock(card->hw,&smp_flags1);   
	}
#endif

	wan_set_bit(WP_BCB_BRI_POST_INIT, &fe->bri_param.critical);

	return 0;
}

/**
 * l1_timer_start_t3
 */
static void l1_timer_start_t3(void *pport)
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module;
	sdla_fe_t	*fe;
	u8		mod_no;

	WAN_ASSERT_VOID(port_ptr == NULL);

	WAN_ASSERT_VOID(port_ptr->hw == NULL);
	bri_module = port_ptr->hw;

	WAN_ASSERT_VOID(bri_module->fe == NULL);
	fe = (sdla_fe_t*)bri_module->fe;

	mod_no = (u8)bri_module->mod_no;

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	if (!wan_test_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical)) {
		/* may get here during unload!! */
		return;
	}

	if (!wan_test_bit(T3_TIMER_ACTIVE,  &port_ptr->timer_flags) &&
		!wan_test_bit(T3_TIMER_EXPIRED, &port_ptr->timer_flags)){

		DEBUG_HFC_S0_STATES("Starting T3 timer...\n");

		wan_set_bit(T3_TIMER_ACTIVE, &port_ptr->timer_flags); 

		wan_add_timer(&port_ptr->t3_timer, (XHFC_TIMER_T3 * HZ) / 1000);
	}
}

/**
 * l1_timer_stop_t3
 */
static void l1_timer_stop_t3(void *pport)
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module = port_ptr->hw;
	u8		mod_no = (u8)bri_module->mod_no;

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	if(wan_test_bit(T3_TIMER_ACTIVE, &port_ptr->timer_flags)){	

		wan_clear_bit(T3_TIMER_ACTIVE,  &port_ptr->timer_flags);
		wan_clear_bit(T3_TIMER_EXPIRED, &port_ptr->timer_flags);

		wan_clear_bit(HFC_L1_ACTIVATING, &port_ptr->l1_flags);
		wan_del_timer(&port_ptr->t3_timer);
	}
}

/*
 ******************************************************************************
 *				l1_timer_expire_t3()	
 *
 * Description: called when timer t3 expires.
 *				Activation failed, force clean L1 deactivation.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void l1_timer_expire_t3(void* pport)
#elif defined(__WINDOWS__)
static void l1_timer_expire_t3(IN PKDPC Dpc, void* pport, void* arg2, void* arg3)
#else
static void l1_timer_expire_t3(unsigned long pport)
#endif
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module = port_ptr->hw;
	sdla_fe_t	*fe = (sdla_fe_t*)bri_module->fe;
	sdla_t 		*card = (sdla_t*)fe->card;
	wan_device_t	*wandev = &card->wandev;

	DEBUG_HFC_S0_STATES("%s()\n", __FUNCTION__);

	if (wandev->fe_enable_timer){

		wan_set_bit(T3_TIMER_EXPIRED, &port_ptr->timer_flags);

		wandev->fe_enable_timer(fe->card);
	}
}

static void __l1_timer_expire_t3(sdla_fe_t *fe)
{
	sdla_bri_param_t *bri = &fe->bri_param;
	wp_bri_module_t	*bri_module;
	bri_xhfc_port_t	*port_ptr;
	u8		mod_no, port_no;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	bri_module = &bri->mod[mod_no];
	port_ptr   = &bri_module->port[port_no];

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	wan_clear_bit(T3_TIMER_ACTIVE, &port_ptr->timer_flags);
	wan_clear_bit(T3_TIMER_EXPIRED, &port_ptr->timer_flags);

	wan_clear_bit(HFC_L1_ACTIVATING, &port_ptr->l1_flags);
	xhfc_ph_command(fe, port_ptr, HFC_L1_FORCE_DEACTIVATE_TE);
}


/**
 * l1_timer_start_t4
 */
static void l1_timer_start_t4(void *pport)
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module;
	sdla_fe_t	*fe;
	u8		mod_no;

	WAN_ASSERT_VOID(port_ptr == NULL);

	WAN_ASSERT_VOID(port_ptr->hw == NULL);
	bri_module = port_ptr->hw;

	WAN_ASSERT_VOID(bri_module->fe == NULL);
	fe = (sdla_fe_t*)bri_module->fe;

	mod_no = (u8)bri_module->mod_no;

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	if (!wan_test_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical)) {
		/* may get here during unload!! */
		return;
	}

	if(!wan_test_and_set_bit(T4_TIMER_ACTIVE, &port_ptr->timer_flags)){
		DEBUG_HFC_S0_STATES("Starting T4 timer...\n");
		wan_set_bit(HFC_L1_DEACTTIMER, &port_ptr->l1_flags);
		wan_add_timer(&port_ptr->t4_timer, (XHFC_TIMER_T4 * HZ) / 1000);
	}
}


/**
 * l1_timer_stop_t4
 */
static void l1_timer_stop_t4(void *pport)
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module = port_ptr->hw;
	u8		mod_no = (u8)bri_module->mod_no;

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	if(wan_test_bit(T4_TIMER_ACTIVE, &port_ptr->timer_flags)){	
		wan_clear_bit(T4_TIMER_ACTIVE, &port_ptr->timer_flags);
		wan_clear_bit(HFC_L1_DEACTTIMER, &port_ptr->l1_flags);
		wan_del_timer(&port_ptr->t4_timer);
	}
}


/*
 ******************************************************************************
 *				l1_timer_expire_t4()	
 *
 * Description: l1_timer_expire_t4 - called when timer t4 expires.
 *		Send (PH_DEACTIVATE | INDICATION) to upper layer.
 *
 *		Note that this function does NOT access the hardware so we
 *		don't have to use wandev->fe_enable_timer() as it is done
 *		for T1 and T3.
 *
 * Arguments:
 * Returns:
 ******************************************************************************
 */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void l1_timer_expire_t4(void* pport)
#elif defined(__WINDOWS__)
static void l1_timer_expire_t4(IN PKDPC Dpc, void* pport, void* arg2, void* arg3)
#else
static void l1_timer_expire_t4(unsigned long pport)
#endif
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module = port_ptr->hw;
	sdla_fe_t	*fe = bri_module->fe;
	u8		mod_no, port_no;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	wan_clear_bit(T4_TIMER_ACTIVE, &port_ptr->timer_flags);
	wan_clear_bit(HFC_L1_DEACTTIMER, &port_ptr->l1_flags);
	sdla_bri_set_status(fe, mod_no, port_no, FE_DISCONNECTED);
}


/*
 ******************************************************************************
 *				l1_timer_expire_t1()	
 *
 * Description: called when timer t1 expires.
 *				Activation failed, force clean L1 deactivation.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void l1_timer_expire_t1(void* pport)
#elif defined(__WINDOWS__)
static void l1_timer_expire_t1(IN PKDPC Dpc, void* pport, void* arg2, void* arg3)
#else
static void l1_timer_expire_t1(unsigned long pport)
#endif
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module = port_ptr->hw;
	sdla_fe_t	*fe = (sdla_fe_t*)bri_module->fe;
	sdla_t 		*card = (sdla_t*)fe->card;
	wan_device_t	*wandev = &card->wandev;

	DEBUG_HFC_S0_STATES("%s()\n", __FUNCTION__);

	if (wandev->fe_enable_timer){

		wan_set_bit(T1_TIMER_EXPIRED, &port_ptr->timer_flags);

		wandev->fe_enable_timer(fe->card);
	}
}

static void __l1_timer_expire_t1(sdla_fe_t *fe)
{
	sdla_bri_param_t *bri = &fe->bri_param;
	wp_bri_module_t	*bri_module;
	bri_xhfc_port_t	*port_ptr;
	u8		mod_no, port_no, connected = 0;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	bri_module = &bri->mod[mod_no];
	port_ptr   = &bri_module->port[port_no];

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	wan_clear_bit(T1_TIMER_EXPIRED, &port_ptr->timer_flags);
	wan_clear_bit(T1_TIMER_ACTIVE,  &port_ptr->timer_flags);
	
	if(	(port_ptr->su_state.bit.v_su_info0)	||
		(!port_ptr->su_state.bit.v_su_fr_sync)){
		/* If receiving INFO0 or Lost Framing, after T1 expired, we must deactivate
		 * NT, because otherwise the user will be prevented from NT activation
		 * forever by the HFC_L1_ACTIVATED bit. */
		connected = 0;
	}else if(port_ptr->su_state.bit.v_su_fr_sync){
		/* Got synchronized. No automatic state change expected. */
		connected = 1;
	}

	if(!wan_test_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags)) {

		DEBUG_HFC_S0_STATES("%s(): (1) De-Activating NT...\n", __FUNCTION__);
		xhfc_ph_command(fe, port_ptr, HFC_L1_DEACTIVATE_NT);

	}else if(!connected){

		DEBUG_HFC_S0_STATES("%s(): (2) De-Activating NT...\n", __FUNCTION__);
		xhfc_ph_command(fe, port_ptr, HFC_L1_DEACTIVATE_NT);
		
	}else{
		/* T1 expired AFTER line become active. */
		DEBUG_HFC_S0_STATES("%s(): NT in Activated state. Doing nothing.\n", __FUNCTION__);
	}
}

/**
 * l1_timer_start_t1
 */
static void l1_timer_start_t1(void *pport)
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module;
	sdla_fe_t	*fe;
	u8		mod_no;

	WAN_ASSERT_VOID(port_ptr == NULL);

	WAN_ASSERT_VOID(port_ptr->hw == NULL);
	bri_module = port_ptr->hw;

	WAN_ASSERT_VOID(bri_module->fe == NULL);
	fe = (sdla_fe_t*)bri_module->fe;

	mod_no = (u8)bri_module->mod_no;

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	if (!wan_test_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical)) {
		/* may get here during unload!! */
		return;
	}

	if (!wan_test_bit(T1_TIMER_ACTIVE,  &port_ptr->timer_flags) &&
		!wan_test_bit(T1_TIMER_EXPIRED, &port_ptr->timer_flags)){

		DEBUG_HFC_S0_STATES("%s(): Starting T1 timer...\n", __FUNCTION__);

		wan_set_bit(T1_TIMER_ACTIVE, &port_ptr->timer_flags); 

		wan_add_timer(&port_ptr->t1_timer, (XHFC_TIMER_T1 * HZ) / 1000);

	}else{
		DEBUG_HFC_S0_STATES("%s(): the T1_TIMER_ACTIVE bit already set\n", __FUNCTION__);
	}
}

/**
* l1_timer_stop_t1
*/
static void l1_timer_stop_t1(void *pport)
{
	bri_xhfc_port_t	*port_ptr = (bri_xhfc_port_t*)pport;
	wp_bri_module_t	*bri_module = port_ptr->hw;
	u8		mod_no = (u8)bri_module->mod_no;
	
	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);
	
	if(wan_test_bit(T1_TIMER_ACTIVE, &port_ptr->timer_flags)){	

		wan_clear_bit(T1_TIMER_ACTIVE,  &port_ptr->timer_flags);
		wan_clear_bit(T1_TIMER_EXPIRED, &port_ptr->timer_flags);

		wan_del_timer(&port_ptr->t1_timer);
	}
}

/******************************************************************************
** wp_bri_if_config() - 
**
**	OK
*/
static int32_t wp_bri_if_config(void *pfe, u32 mod_map, u8 usedby)
{
	BRI_FUNC();
	return 0;
}


/******************************************************************************
** wp_bri_if_unconfig() - 
**
**	OK
*/
static int32_t wp_bri_if_unconfig(void *pfe, u32 mod_map, u8 usedby)
{
	BRI_FUNC();
	return 0;
}


/*******************************************************************************
*			bri_enable_interrupts()	
*
* Description: Enable BRI interrupts - start interrupt and set interrupt mask.
*
* Arguments:
*
* Returns:
*******************************************************************************/
static void bri_enable_interrupts(sdla_fe_t *fe, u32 mod_no, u8 port_no)
{
	sdla_bri_param_t 	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	reg_r_ti_wd		r_ti_wd;
	reg_r_misc_irqmsk	r_misc_irqmsk;
	reg_r_irq_ctrl		r_irq_ctrl;
	reg_r_su_irqmsk		r_su_irqmsk;

	BRI_FUNC();

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return;
	}

	DEBUG_EVENT("%s: Module: %d: Enabling %s Interrupts \n",
				fe->name, REPORT_MOD_NO(mod_no),
				FE_MEDIA_DECODE(fe));

	bri_module = &bri->mod[mod_no];

	WRITE_REG(R_SU_IRQMSK, 0);

	r_ti_wd.reg = 0x0;
	/* Configure Timer Interrupt - every 2.048 seconds (page 289, 297).
	   Used for SU port_no state monitoring. */
	r_ti_wd.bit.v_ev_ts = 0xD;
	/* Watch Dog interrupt not used */
	r_ti_wd.bit.v_wd_ts = 0x0;
	WRITE_REG(R_TI_WD, r_ti_wd.reg);

	r_misc_irqmsk.reg = 0;
#if !defined(BUILD_MOD_TESTER)
	r_misc_irqmsk.bit.v_ti_irqmsk = 1;/* Enable (one) / Disable (zero) timer interrupts */
#endif
	WRITE_REG( R_MISC_IRQMSK, r_misc_irqmsk.reg);

	/* clear all pending interrupts bits */
	READ_REG( R_MISC_IRQ);
	READ_REG( R_SU_IRQ);
	READ_REG( R_FIFO_BL0_IRQ);
	READ_REG( R_FIFO_BL1_IRQ);
	READ_REG( R_FIFO_BL2_IRQ);
	READ_REG( R_FIFO_BL3_IRQ);

	/* unmask SU state interrupt for all ports */	
	r_su_irqmsk.reg = 0;
	r_su_irqmsk.bit.v_su0_irqmsk = 1;
	r_su_irqmsk.bit.v_su1_irqmsk = 1;
	WRITE_REG( R_SU_IRQMSK, r_su_irqmsk.reg);

	/* enable global (all) interrupts */
	r_irq_ctrl.reg = 0;
	r_irq_ctrl.bit.v_glob_irq_en = 1;
	r_irq_ctrl.bit.v_fifo_irq_en = 1;
	WRITE_REG( R_IRQ_CTRL, r_irq_ctrl.reg);
}


/******************************************************************************
** wp_bri_disable_fe_irq() - disable all interrupts by disabling M_GLOB_IRQ_EN
**
**	OK
*/

static int wp_bri_disable_fe_irq(void *pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	u32 mod_no;
	u8  port_no;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	wp_bri_disable_irq(fe,mod_no,port_no);

	return 0;
}

/******************************************************************************
** wp_bri_disable_irq() - disable all interrupts by disabling M_GLOB_IRQ_EN
**
**	OK
*/
static int32_t wp_bri_disable_irq(sdla_fe_t *fe, u32 mod_no, u8 port_no)
{
	sdla_bri_param_t 	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	reg_r_su_irqmsk		r_su_irqmsk;
	reg_r_irq_ctrl		r_irq_ctrl;

	BRI_FUNC();

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return 1;
	}

	DEBUG_EVENT("%s: Module: %d: Disabling %s Interrupts \n",
					fe->name, REPORT_MOD_NO(mod_no),
				FE_MEDIA_DECODE(fe));

	bri_module = &bri->mod[mod_no];

	/* disable SU state interrupt for all ports */	
	r_su_irqmsk.reg = 0;
	WRITE_REG( R_SU_IRQMSK, r_su_irqmsk.reg);

/*
	if (!wan_test_bit(WP_RM_CONFIGURED,(void*)&fe->bri_param.critical)){
		return -EINVAL;
	}
*/

	/* disable global (all) other interrupts */
	r_irq_ctrl.reg = 0;
	WRITE_REG( R_IRQ_CTRL, 0);

	return 0;
}

static u32 wp_bri_active_map(sdla_fe_t* fe, u8 line_no)
{
	BRI_FUNC();

	if(line_no >= 2){
		DEBUG_ERROR("%s: %s(): Error: Line number %d is out of range!\n",
					fe->name, __FUNCTION__, line_no);
		return 0;
	}
	
	DEBUG_TEST("%s: ACTIVE MAP Port=%i Returning 0x%08X\n",
			fe->name,  
			WAN_FE_LINENO(fe),
			 0x03 << (WAN_FE_LINENO(fe)%MAX_BRI_MODULES)*2);

	return 0x03 << (WAN_FE_LINENO(fe)%MAX_BRI_MODULES)*2;
}

/******************************************************************************
*				wp_bri_fe_status()	
*
* Description:
* Arguments:	
* Returns:
*******************************************************************************/
static u8 wp_bri_fe_media(sdla_fe_t *fe)
{
	BRI_FUNC();
	return fe->fe_cfg.media;
}

/******************************************************************************
*				wp_bri_set_dtmf()	
*
* Description:
* Arguments:	
* Returns:
*******************************************************************************/
static int32_t wp_bri_set_dtmf(sdla_fe_t *fe, int32_t mod_no, u8 val)
{
	BRI_FUNC();
#if 0

	if (mod_no > MAX_REMORA_MODULES){
		DEBUG_EVENT("%s: Module %d: Module number out of range!\n",
					fe->name, mod_no);
		return -EINVAL;
	}	
	if (!wan_test_bit(mod_no-1, &fe->bri_param.module_map)){
		DEBUG_EVENT("%s: Module %d: Not configures yet!\n",
					fe->name, mod_no);
		return -EINVAL;
	}
	
#endif
	return -EINVAL;
}

#if 0

/*******************************************************************************
*				sdla_bri_timer()	
*
* Description:
* Arguments:
* Returns:
*******************************************************************************/
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void wp_bri_timer(void* pfe)
#elif defined(__WINDOWS__)
static void wp_bri_timer(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3)
#else
static void wp_bri_timer(void *pfe)
#endif
{
	BRI_FUNC();
	return;
}

/*******************************************************************************
*				wp_bri_enable_timer()	
*
* Description: Enable software timer interrupt in delay ms.
* Arguments:
* Returns:
*******************************************************************************/
static void wp_bri_enable_timer(sdla_fe_t* fe, u8 mod_no, u8 cmd, u32 delay)
{
	BRI_FUNC();
	return;	
}

static int32_t wp_bri_regdump(sdla_fe_t* fe, u8 *data)
{
	BRI_FUNC();
	return 0;
}

#endif /* if 0*/

static int32_t wp_bri_polling(sdla_fe_t* fe)
{
	sdla_bri_param_t *bri = &fe->bri_param;
	wp_bri_module_t	*bri_module;
	bri_xhfc_port_t	*port_ptr;
	u8		mod_no, port_no;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	bri_module = &bri->mod[mod_no];
	port_ptr   = &bri_module->port[port_no];

	/* Note that aft_core.c calls card->wandev.fe_iface.polling() 
	 * UNCONDITIONALLY, on each Front End interrupt. 
	 * It is a problem for BRI timers because we need an additional flag,
	 * which will indicate that "T1 timer really expired". 
	 * Only if the flag is set, we act as we should on timer expiration. */

	if (port_ptr->mode & PORT_MODE_TE) {
		/*************/
		/* TE timers */
		if(wan_test_bit(T3_TIMER_ACTIVE, &port_ptr->timer_flags)){
			DEBUG_HFC_S0_STATES("%s(): TE T3_TIMER_ACTIVE\n", __FUNCTION__);
		}

		if(wan_test_bit(T3_TIMER_EXPIRED, &port_ptr->timer_flags)){
			DEBUG_HFC_S0_STATES("%s(): TE T3_TIMER_EXPIRED\n", __FUNCTION__);
		}

		if (wan_test_bit(T3_TIMER_EXPIRED, &port_ptr->timer_flags) &&
			wan_test_bit(T3_TIMER_ACTIVE,  &port_ptr->timer_flags)){

			__l1_timer_expire_t3(fe);
		}
	}else{
		/*************/
		/* NT timers */
		if(wan_test_bit(T1_TIMER_ACTIVE, &port_ptr->timer_flags)){
			DEBUG_HFC_S0_STATES("%s(): NT T1_TIMER_ACTIVE\n", __FUNCTION__);
		}

		if(wan_test_bit(T1_TIMER_EXPIRED, &port_ptr->timer_flags)){
			DEBUG_HFC_S0_STATES("%s(): NT T1_TIMER_EXPIRED\n", __FUNCTION__);
		}
		
		if (wan_test_bit(T1_TIMER_EXPIRED, &port_ptr->timer_flags) &&
			wan_test_bit(T1_TIMER_ACTIVE,  &port_ptr->timer_flags)) {
			
			__l1_timer_expire_t1(fe);
		}
	}
	return 0;
}

/*******************************************************************************
*				wp_bri_udp()	
*
* Description:
* Arguments:
* Returns:
*******************************************************************************/
static int32_t wp_bri_udp(sdla_fe_t *fe, void* p_udp_cmd, u8* data)
{
	wan_cmd_t		*udp_cmd = (wan_cmd_t*)p_udp_cmd;
	wan_femedia_t		*fe_media;
	sdla_fe_timer_event_t	event;

	BRI_FUNC();
	memset(&event, 0, sizeof(sdla_fe_timer_event_t));
	switch(udp_cmd->wan_cmd_command){
		case WAN_GET_MEDIA_TYPE:
                fe_media = (wan_femedia_t*)data;
                memset(fe_media, 0, sizeof(wan_femedia_t));
                fe_media->media         = fe->fe_cfg.media;
                fe_media->sub_media     = fe->fe_cfg.sub_media;
                fe_media->chip_id       = 0x00;
                fe_media->max_ports     = 1;
                udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
                udp_cmd->wan_cmd_data_len = sizeof(wan_femedia_t);
                break;
		default:
			udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
				udp_cmd->wan_cmd_data_len = 0;
			break;
		}
	return 0;
}

/******************************************************************************
*wp_bri_get_fe_status()	
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
static int wp_bri_get_fe_status(sdla_fe_t *fe, unsigned char *status, int notused)
{
	*status = fe->fe_status;
	return 0;
}


static int bchan_loopback_control(sdla_fe_t *fe, u8 bchan_no, u8 loopback_enable)
{
	u8			mod_no, port_no, pcm_slot;
	sdla_bri_param_t	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	bri_xhfc_port_t		*port_ptr;
	reg_a_sl_cfg		a_sl_cfg;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	bri_module = &bri->mod[mod_no];
	port_ptr = &bri_module->port[port_no];


	DEBUG_LOOPB("%s()\n", __FUNCTION__);

    if (!((bchan_no == 0) || (bchan_no == 1))) {
		DEBUG_LOOPB("%s %s(): port_no(%i) ERROR: bchan_no(%i) invalid!\n",
                       fe->name, __FUNCTION__, port_no, bchan_no);
		return 1;
	}

    DEBUG_LOOPB("%s %s(): %s loopback, port_no(%i), bchan_no(%i)\n",
		fe->name, __FUNCTION__,
		(loopback_enable) ? ("enable") : ("disable"), port_no, bchan_no);

	if(mod_no >= MAX_BRI_MODULES){
		/* adjust mod_no to be between 0 and 10 (including)*/
		pcm_slot = (u8)calculate_pcm_timeslot(mod_no - MAX_BRI_MODULES, port_no, bchan_no);
		/* AFT Line 1 will use odd PCM timeslots */
		pcm_slot += 1;
	}else{
		/* AFT Line 0 will use even PCM timeslots */
		pcm_slot = (u8)calculate_pcm_timeslot(mod_no, port_no, bchan_no);
	}

	DEBUG_LOOPB("selecting pcm_slot: %i, HFC channel: %i\n", pcm_slot, port_no*4+bchan_no);

	/*****************************************************************************************/
	/* transmit slot - select direction TX */
	xhfc_select_pcm_slot(fe, mod_no, pcm_slot, XHFC_DIRECTION_TX);

	/* Connect time slot with channel and pin.
	Assign HFC channel (from 0 to 15) to the selected PCM slot.*/

	a_sl_cfg.reg = 0;

	a_sl_cfg.bit.v_ch_sdir	= 0;
	a_sl_cfg.bit.v_ch_snum	= port_no*4+bchan_no;/*page 75 */
	a_sl_cfg.bit.v_rout	= (loopback_enable ? 0x01:0x03);/* page 257 */

	WRITE_REG(A_SL_CFG, a_sl_cfg.reg);


	/*****************************************************************************************/
	/* receive slot - select direction RX */
	xhfc_select_pcm_slot(fe, mod_no, pcm_slot, XHFC_DIRECTION_RX);

	/* Connect time slot with channel and pin.
	Assign HFC channel (from 0 to 15) to the selected PCM slot. */
	a_sl_cfg.reg = 0;

	a_sl_cfg.bit.v_ch_sdir	= 1;
	a_sl_cfg.bit.v_ch_snum	= port_no*4+bchan_no;/*page 75 */
	a_sl_cfg.bit.v_rout	= (loopback_enable ? 0x01:0x03);/* page 257 */

	WRITE_REG(A_SL_CFG,a_sl_cfg.reg);

	return 0;
}


/* Commands from user. */
static int wp_bri_control(sdla_fe_t *fe, u32 command)
{
	u8			mod_no, port_no;
	int			rc;
	sdla_bri_param_t	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	bri_xhfc_port_t		*port_ptr;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	DEBUG_LOOPB("%s(): Module: %d, port_no: %d. fe->name: %s, command: %i\n",
		__FUNCTION__, mod_no, port_no, fe->name, command);

	bri_module = &bri->mod[mod_no];
	port_ptr = &bri_module->port[port_no];

	switch(command)
	{
	case HFC_L1_ENABLE_LOOP_B1:
		DEBUG_LOOPB("HFC_L1_ENABLE_LOOP_B1\n");
		rc = bchan_loopback_control(fe, 0, 1);
		break;

	case HFC_L1_ENABLE_LOOP_B2:
		DEBUG_LOOPB("HFC_L1_ENABLE_LOOP_B2\n");
		rc = bchan_loopback_control(fe, 1, 1);
		break;

	case HFC_L1_DISABLE_LOOP_B1:
		DEBUG_LOOPB("HFC_L1_DISABLE_LOOP_B1\n");
		rc = bchan_loopback_control(fe, 0, 0);
		break;

	case HFC_L1_DISABLE_LOOP_B2:
		DEBUG_LOOPB("HFC_L1_DISABLE_LOOP_B2\n");
		rc = bchan_loopback_control(fe, 1, 0);
		break;

	default:
		DEBUG_ERROR("%s(): %s: Error: invalid command '%i'requested!\n",
			__FUNCTION__, fe->name, command);
		rc = 1;
		break;
	}	

	return rc;
}


/******************************************************************************
*				wp_bri_set_fe_status()	
*
* Description	: Set FE line state to Connected or Disconnected.
*		  In BRI this means Activate or Deactivate the line.
*
* Arguments	: fe - pointer to Front End structure.	
*		  new_status - the new FE line state.
*
* Returns	: 0 - success.
*		  1 - failure.
*******************************************************************************/
static int wp_bri_set_fe_status(sdla_fe_t *fe, unsigned char new_status)
{
	u8			mod_no, port_no;
	int			rc = 0;
	sdla_bri_param_t	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	bri_xhfc_port_t		*port_ptr;

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	DEBUG_FE_STATUS("%s(): Module: %d, port_no: %d. fe->name: %s, new status: %d (%s)\n",
		__FUNCTION__, mod_no, port_no, fe->name, new_status, FE_STATUS_DECODE(new_status));

	bri_module = &bri->mod[mod_no];
	port_ptr = &bri_module->port[port_no];

#if defined (BUILD_MOD_TESTER)
        sdla_bri_set_status(fe, mod_no, port_no, FE_CONNECTED);
        return 0;
#endif

	switch(new_status)
	{
	case WAN_FE_CONNECTED:
		DEBUG_L2_TO_L1_ACTIVATION("%s: L2->L1 -- ACTIVATE REQUEST\n", 
			fe->name);
		if (port_ptr->mode & PORT_MODE_TE) {
			if (wan_test_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags)) {
				/* The line is already in active state. Confirm to L2 that line is connected. */
				DEBUG_L2_TO_L1_ACTIVATION("%s: TE: L1->L2 -- ACTIVATE CONFIRM (line already active)\n",
					fe->name);
				sdla_bri_set_status(fe, mod_no, port_no, FE_CONNECTED);	
			} else {
				wan_test_and_set_bit(HFC_L1_ACTIVATING, &port_ptr->l1_flags);
				xhfc_ph_command(fe, port_ptr, HFC_L1_ACTIVATE_TE);
				l1_timer_start_t3(port_ptr);
			}
		} else {
			if(wan_test_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags)) {
				/* The line is already in active state. Confirm to L2 that line is connected. */
				DEBUG_L2_TO_L1_ACTIVATION("%s: NT: L1->L2 -- ACTIVATE CONFIRM (line already active)\n",
					fe->name);
				sdla_bri_set_status(fe, mod_no, port_no, FE_CONNECTED);	
			} else {
				xhfc_ph_command(fe, port_ptr, HFC_L1_ACTIVATE_NT);
				/* After activation, state will automatically change to G2, where
				 * T1 will be started. Don't start T1 here as recommended by Table 5.4. */
			}
        }
		break;

	case WAN_FE_DISCONNECTED:
		DEBUG_L2_TO_L1_ACTIVATION("%s: L2->L1 -- DEACTIVATE REQUEST\n", 
			fe->name);
		if (port_ptr->mode & PORT_MODE_TE) {
			/* no deact request in TE mode ! */
			DEBUG_ERROR("%s(): %s: Error: 'deactivate' request is invalid for TE!\n",
				__FUNCTION__, fe->name);
			rc = 1;
		} else {
			xhfc_ph_command(fe, port_ptr, HFC_L1_DEACTIVATE_NT);
		}
		break;

	default:
		DEBUG_ERROR("%s(): %s: Error: invalid new status '%d' (%s) requested!\n",
			__FUNCTION__, fe->name, new_status, FE_STATUS_DECODE(new_status));
		rc = 1;
		break;
	}	

	return rc;
}


/******************************************************************************
*				wp_bri_event_ctrl()	
*
* Description: Enable/Disable event types
* Arguments: mod_no -  Module number (1,2,3,... MAX_REMORA_MODULES)
* Returns:
******************************************************************************/
static int wp_bri_event_ctrl(sdla_fe_t *fe, wan_event_ctrl_t *ectrl)
{
	int err = 1;

	BRI_FUNC();

	WAN_ASSERT(ectrl == NULL);

	DEBUG_LOOPB("%s: Event Type: %s, Mode: %s.\n",
		fe->name, WAN_EVENT_TYPE_DECODE(ectrl->type),
		WAN_EVENT_MODE_DECODE(ectrl->mode));

	switch(ectrl->type)
	{
	case WAN_EVENT_BRI_CHAN_LOOPBACK:
		switch(ectrl->channel)
		{
		case WAN_BRI_BCHAN1:
			err = wp_bri_control(fe, (ectrl->mode == WAN_EVENT_ENABLE ? HFC_L1_ENABLE_LOOP_B1:HFC_L1_DISABLE_LOOP_B1));
			break;
		case WAN_BRI_BCHAN2:
			err = wp_bri_control(fe, (ectrl->mode == WAN_EVENT_ENABLE ? HFC_L1_ENABLE_LOOP_B2:HFC_L1_DISABLE_LOOP_B2));
			break;
		}
		break;
	}

	return err;
}

/******************************************************************************
*			wp_bri_watchdog()	
*
* Description:
* Arguments: mod_no -  Module number (1,2,3,... MAX_REMORA_MODULES)
* Returns:
******************************************************************************/
#if 0
static int32_t wp_bri_watchdog(sdla_fe_t *fe)
{
	BRI_FUNC();
	
	return 0;
}
#endif

/******************************************************************************
*				wp_bri_intr_ctrl()	
*
* Description: Enable/Disable extra interrupt types
* Arguments: mod_no -  Module number (1,2,3,... MAX_REMORA_MODULES)
* Returns:
******************************************************************************/
static int wp_bri_intr_ctrl(sdla_fe_t *fe, int mod_no, u_int8_t type, u_int8_t mode, unsigned int ts_map)
{
	int32_t		err = 0;

	BRI_FUNC();

	return err;

}

/****************************************************/
/* Physical S/U commands to control Line Interface  */
/****************************************************/
static void xhfc_ph_command(sdla_fe_t *fe, bri_xhfc_port_t *port, u_char command)
{
	wp_bri_module_t	*bri_module = port->hw;
	u8		mod_no = (u8)bri_module->mod_no;

	DEBUG_HFC_S0_STATES("%s(): command: 0x%X\n", __FUNCTION__, command);

	switch (command) 
	{
	case HFC_L1_ACTIVATE_TE:
		DEBUG_L2_TO_L1_ACTIVATION("HFC_L1_ACTIVATE_TE port(%i)\n", port->idx);

		WRITE_REG(R_SU_SEL, port->idx);
		WRITE_REG(A_SU_WR_STA, STA_ACTIVATE);
		break;

	case HFC_L1_FORCE_DEACTIVATE_TE:
		DEBUG_L2_TO_L1_ACTIVATION("HFC_L1_FORCE_DEACTIVATE_TE port(%i)\n", port->idx);
			        
		WRITE_REG(R_SU_SEL, port->idx);
		WRITE_REG(A_SU_WR_STA, STA_DEACTIVATE);
		break;

	case HFC_L1_ACTIVATE_NT:
		DEBUG_L2_TO_L1_ACTIVATION("HFC_L1_ACTIVATE_NT port(%i)\n", port->idx);

		WRITE_REG(R_SU_SEL, port->idx);
		WRITE_REG(A_SU_WR_STA, STA_ACTIVATE | M_SU_SET_G2_G3);
		break;

	case HFC_L1_DEACTIVATE_NT:
		DEBUG_L2_TO_L1_ACTIVATION("HFC_L1_DEACTIVATE_NT port(%i)\n", port->idx);

		WRITE_REG(R_SU_SEL, port->idx);
		WRITE_REG(A_SU_WR_STA, STA_DEACTIVATE);
		break;

	default:
		DEBUG_L2_TO_L1_ACTIVATION("Invalid command: %i !\n", command);
		break;
	}
}

/******************************************************************************
*				sdla_bri_set_status()	
*
* Description:	set line status to 'connected' or 'disconnected' and indicate
*		line state change to upper layer.
* Arguments:	fe, mod_no, port_no, new line status
* Returns:	nothing
******************************************************************************/
static void sdla_bri_set_status(sdla_fe_t* fe, u8 mod_no, u8 port_no, u8 new_status)
{
	sdla_t	*card = (sdla_t*)fe->card;

	BRI_FUNC();

	DEBUG_HFC_S0_STATES("%s(): new_status: %i, old status: %i\n",
		__FUNCTION__, new_status, fe->fe_status);

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return;
	}

	if (new_status == fe->fe_status){
		return;
	}

	fe->fe_status = new_status;

	if (new_status == FE_CONNECTED){

		DEBUG_EVENT("%s: %s Module: %d connected!\n", 
			fe->name,
			FE_MEDIA_DECODE(fe), REPORT_MOD_NO(mod_no) + port_no);

		if (card->wandev.te_report_alarms){
			card->wandev.te_report_alarms(card, 0);
		}
	}else{

		DEBUG_EVENT("%s: %s Module: %d disconnected!\n", 
			fe->name,
			FE_MEDIA_DECODE(fe), REPORT_MOD_NO(mod_no) + port_no);

		if (card->wandev.te_report_alarms){
			card->wandev.te_report_alarms(card, (1|WAN_TE_BIT_ALARM_RED));
		}
	}

	if (card->wandev.te_link_state){
		card->wandev.te_link_state(card);
	}

	return;
}


/******************************************************************************
*				su_new_state()	
*
* Description: handle SU port state interrupt on a physical module
*
* SU port state interrupt notes:
*	1. Chip automatically goes into inactive state if:
*		1.1 line is disconnected
*		1.2 line is deactivated
*				   
*	2. Because of (1) user application will have to activate the line,
*	   wait for the line to get 'connected' for about 1 second and if
*	   after 1 second line is not getting 'connected', it means line is
*	   actually disconnected and NOT simply deactivated.
*
* Arguments: fe, mod_no
*
* Returns:	nothing
******************************************************************************/
static void su_new_state(sdla_fe_t *fe, u8 mod_no, u8 port_no)
{
	bri_xhfc_port_t		*port;
	sdla_bri_param_t 	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	u8			connected = 0;

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return;
	}

	bri_module	= &bri->mod[mod_no];
	port		= &bri_module->port[port_no];

	connected = __su_new_state(fe, mod_no, port_no);
	if(connected == 1){
		sdla_bri_set_status(fe, mod_no, port_no, FE_CONNECTED);	
	}else{
		sdla_bri_set_status(fe, mod_no, port_no, FE_DISCONNECTED);
	}
}


static u8 __su_new_state(sdla_fe_t *fe, u8 mod_no, u8 port_no)
{
	bri_xhfc_port_t		*port_ptr;
	sdla_bri_param_t 	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	u8		connected = 0;
	u8		current_fe_status = fe->fe_status;
	

	BRI_FUNC();

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return connected;
	}

	bri_module	= &bri->mod[mod_no];
	port_ptr	= &bri_module->port[port_no];

	DEBUG_HFC_S0_STATES("%s(): mod_no: %i, port number: %i\n", __FUNCTION__, mod_no, port_ptr->idx);

	if (port_ptr->mode & PORT_MODE_TE) {
		DEBUG_HFC_S0_STATES("TE F%d\n", port_ptr->l1_state);

		if ((port_ptr->l1_state <= 3) || (port_ptr->l1_state >= 7)){
			l1_timer_stop_t3(port_ptr);
		}

		switch (port_ptr->l1_state) 
		{
		case (3):
			if (wan_test_and_clear_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags)){
			/* Do NOT indicate 'disconnect' right away, do it when 
				T4 expires. */
				if (fe->fe_status == FE_CONNECTED){
					connected = 1; /* keep the old state */
				}
				l1_timer_start_t4(port_ptr);
			}
			return connected;
			
		case (7):
			l1_timer_stop_t4(port_ptr);
			connected = 1;
			
			if (wan_test_and_clear_bit(HFC_L1_ACTIVATING, &port_ptr->l1_flags)) {
				DEBUG_HFC_S0_STATES("l1->l2 -- ACTIVATE CONFIRM\n");
				
				wan_set_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags);
				
			} else {
				if (!(wan_test_and_set_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags))) {
					DEBUG_HFC_S0_STATES("l1->l2 -- ACTIVATE INDICATION\n");
				} else {
					/* L1 was already activated (e.g. F8->F7) */
					return connected;
				}
			}
			break;
			
		case (8):/* framing is lost but not a disconnect yet */
			l1_timer_stop_t4(port_ptr);
			connected = 1;
			return connected;
			
		case (6):/* synchronized */
			connected = 1;
			return connected;
			
		default:
			return connected;
		}

	} else if (port_ptr->mode & PORT_MODE_NT) {

		DEBUG_HFC_S0_STATES("NT G%d\n", port_ptr->l1_state);

		/*	S/T state transition based Cologne recomendations:
		 *	T1 is a counter that can be reset. An other stop function is not needed.
		 *	There are only 3 states of T1:
		 *	1. Reset permanent (Stop)
		 *	2. Running
		 *	3. Expire
		 *
		 *	The easiest way to implement T1 is:
		 *	state		action for T1
		 *	0,1         T1 Reset
		 *	2			T1 Running
		 *	3,4			T1 Reset
		 */

		if(port_ptr->l1_state != NT_STATE_PENDING_ACTIVATION_G2){
			l1_timer_stop_t1(port_ptr);
		}

		/* S/T state transitions based on Table 5.4 */
		switch (port_ptr->l1_state) 
		{
		case NT_STATE_RESET_G0:
		case NT_STATE_DEACTIVATED_G1:
		case NT_STATE_PENDING_DEACTIVATION_G4:
			wan_clear_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags);
			connected = 0;
			break;

		case NT_STATE_PENDING_ACTIVATION_G2:
			/* Start 10 seconds software timer T1. If already started, function has no effect. */
			l1_timer_start_t1(port_ptr);

			if(!port_ptr->su_state.bit.v_su_info0){
				/* We are NOT receiving INFO0 and very likely we are receiving INFO3.
				 * That means link is on the way UP. Next state should be G3.
				 *
				 * Automatic G2->G3 transition is allowed by init_xfhc() -
				 * V_G2_G3_EN was set in the register A_SU_CTRL1.*/
			}else{
				/* Link on the way DOWN, but no automatic state change. 
				 * The state change will be triggered by T1 expiration,
				 * where the line is deactivated. */
				wan_clear_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags);
			}

			/* In any case, do NOT change the line state. */
			connected = (current_fe_status == FE_CONNECTED ? 1 : 0);
			break;

		case NT_STATE_ACTIVE_G3:
			if(	(port_ptr->su_state.bit.v_su_info0)	||
				(!port_ptr->su_state.bit.v_su_fr_sync)){
				/* If receiving INFO0 or Lost Framing, chip will automatically go into G2. */
				/* Wait for G2 to go into disconnected state, so here stay 'connected'. */
				wan_set_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags);
				connected = 1;
			}else if(port_ptr->su_state.bit.v_su_fr_sync){
				/* Got synchronized. No automatic state change expected. */
				wan_set_bit(HFC_L1_ACTIVATED, &port_ptr->l1_flags);
				connected = 1;
			}
			break;

		default:
			DEBUG_ERROR("%s: Error: invalid NT State: %d.\n", fe->name, port_ptr->l1_state);
			break;
		}

		if(connected == 1){
			DEBUG_HFC_S0_STATES("NT: l1->l2 -- ACTIVATE INDICATION\n");
		}else{
			DEBUG_HFC_S0_STATES("NT: l1->l2 (PH_DEACTIVATE | INDICATION)\n");
		}
	}

	return connected;
}

/******************************************************************************
*				get_FE_ptr_for_port()	
*
* Description	: get pointer to FE structure belonging to a 'port_no' on
*		  module 'mod_no'.
*		  Allows to handle FE interrupt of ALL ports on the SAME
*		  physical module.
*		  Note: the returned pointer is NOT the same as the pointer
*			in 'wp_bri_module_t->fe' because this function returns
*			'fe' related to the 'card'.
*
* Arguments	: fe, mod_no, port_no
*
* Returns	: fe pointer - found FE for the 'port_no'.
*		  NULL	     - FE for the 'port_no' not found (it means port_no
*			       is not used).
*
******************************************************************************/
sdla_fe_t *get_FE_ptr_for_port(sdla_fe_t *original_fe, u8 mod_no, u8 port_no)
{
	sdla_t	*card = (sdla_t*)original_fe->card;
	sdla_t	*tmp_card;
	void	**card_list;

	BRI_FUNC();

	if(!card || !card->hw){
		DEBUG_HFC_IRQ("%s(): card: 0x%p!!\n",  __FUNCTION__, card);
		return NULL;
	}
	DEBUG_HFC_IRQ("%s(): mod: %d, port_no: %d (sum: %d)\n", 
		__FUNCTION__, mod_no, port_no, mod_no + port_no);

	card_list=__sdla_get_ptr_isr_array(card->hw);
/*
{
	int i;
	for(i = 0; i < SDLA_MAX_PORTS; i++){
		DEBUG_HFC_IRQ("card_list[%d]: 0x%p\n", i, card_list[i]);
	}
}
*/
	tmp_card=(sdla_t*)card_list[mod_no + port_no];

	DEBUG_HFC_IRQ("%s(): card_list ptr: 0x%p\n", __FUNCTION__, card_list);
	DEBUG_HFC_IRQ("%s(): card ptr: 0x%p, tmp_card ptr: 0x%p\n", __FUNCTION__, card, tmp_card);

	if (!tmp_card){
		return NULL;
	}

	return &tmp_card->fe;
}

/******************************************************************************
*				xhfc_interrupt()	
*
* Description	: handle interrupt on a PHYSICAL module
*
* Arguments	: fe, mod_no
*
* Returns	: 1 - interrupt recognized and handled
*		  0 - interrupt not recognized (not generated by this module)
*
******************************************************************************/
static int32_t xhfc_interrupt(sdla_fe_t *fe, u8 mod_no)
{
	sdla_fe_t		*new_fe;
	int32_t			fifo_irq = 0;
	sdla_bri_param_t 	*bri = &fe->bri_param;
	wp_bri_module_t		*bri_module;
	u8			port_no, i;
	reg_a_su_rd_sta		new_su_state;
	reg_r_su_irq		r_su_irq;
	reg_r_misc_irq		r_misc_irq;

	if(validate_physical_mod_no(mod_no, __FUNCTION__)){
		return 0;
	}

	bri_module = &bri->mod[mod_no];

#if 0
	DEBUG_HFC_SU_IRQ("%s(%lu): %s: mod_no: %d\n", __FUNCTION__, jiffies, fe->name, mod_no);
#endif

	/* clear SU state interrupt (for all ports) */
	r_su_irq.reg = READ_REG(R_SU_IRQ);

	/* clear 'misc' interrupts such as timer interrupt */
	r_misc_irq.reg = READ_REG(R_MISC_IRQ);

	/***************************************************************************/
	fifo_irq = 0;
	for (port_no = 0; port_no < bri_module->num_ports; port_no++) {

		DBG_MODULE_TESTER("get fifo IRQ state for port_no %d\n", port_no);

		/* get fifo IRQ states in bundle */
		fifo_irq |= (READ_REG(R_FIFO_BL0_IRQ + port_no) << (port_no * 8));

		DBG_MODULE_TESTER("fifo_irq: 0x%X\n", fifo_irq);
	}

	/***************************************************************************/
	for (i = 0; i < bri_module->num_ports; i++) {

		/****************************************************************/
		new_fe = get_FE_ptr_for_port(fe, mod_no, i);
		DEBUG_HFC_IRQ("%s(): fe ptr: 0x%p\n", __FUNCTION__, fe);
		if(new_fe == NULL){
			/* 'port_no' is not used by any 'wanpipe' */
			continue;
		}
		fe = new_fe;

		mod_no = fe_line_no_to_physical_mod_no(fe);	
		port_no = fe_line_no_to_port_no(fe);
		/****************************************************************/

		if(r_misc_irq.reg & M_TI_IRQ){
			/*sdla_bri_param_t	*su_bri = &fe->bri_param;
			wp_bri_module_t		*su_bri_module = &su_bri->mod[mod_no];
			bri_xhfc_port_t		*port_ptr = &su_bri_module->port[port_no];*/

			/*DEBUG_HFC_SU_IRQ("Timer IRQ\n");*/
		}

		/****************************************************************/

		/* select the port on the chip */
		WRITE_REG(R_SU_SEL, port_no);
		new_su_state.reg = READ_REG(A_SU_RD_STA);

		DEBUG_HFC_SU_IRQ("%s(): SU State: sta:%d, v_su_fr_sync: %d, v_su_t2_exp: %d, v_su_info0: %d, v_g2_g3: %d\n",
			__FUNCTION__,
			new_su_state.bit.v_su_sta, new_su_state.bit.v_su_fr_sync, new_su_state.bit.v_su_t2_exp,
			new_su_state.bit.v_su_info0,	new_su_state.bit.v_g2_g3);

		if((r_su_irq.reg & (1 << port_no)) || (r_misc_irq.reg & M_TI_IRQ)){
			sdla_bri_param_t 	*su_bri = &fe->bri_param;
			wp_bri_module_t		*su_bri_module = &su_bri->mod[mod_no];
			bri_xhfc_port_t		*port_ptr = &su_bri_module->port[port_no];

			if (new_su_state.bit.v_su_sta != port_ptr->l1_state) {

				DEBUG_HFC_S0_STATES("%s():SU IRQ:%lu: %s: SU State: 0x%X, v_su_fr_sync: %d, v_su_info0: %d, v_g2_g3: %d\n",
					__FUNCTION__, jiffies,
					(port_ptr->mode & PORT_MODE_NT) ? "NT: G" : "TE: F",
					new_su_state.bit.v_su_sta, new_su_state.bit.v_su_fr_sync,
					new_su_state.bit.v_su_info0, new_su_state.bit.v_g2_g3);

#if 0
				if (port_ptr->mode & PORT_MODE_TE) {
					DEBUG_TE_STATES("v_su_sta: 0x%02X (%s), v_su_fr_sync: %d, v_su_info0: %d, v_g2_g3: %d\n",
						new_su_state.bit.v_su_sta, WP_DECODE_TE_STATE(new_su_state.bit.v_su_sta),
						new_su_state.bit.v_su_fr_sync,
						new_su_state.bit.v_su_info0,
						new_su_state.bit.v_g2_g3);
				}
#endif
 				port_ptr->l1_state = new_su_state.bit.v_su_sta;
				port_ptr->su_state = new_su_state;
				/* Handle S/U state change. */
				su_new_state(fe, mod_no, port_no);
			}

		}/* if(r_misc_irq.reg & M_TI_IRQ || (r_su_irq.reg & (1 << port_no))) */

		if(bri_module->port[port_no].bytes2transmit){
			u8 free_space;

			TX_FAST_DBG("%s(): port_no: %d, bytes2transmit: %d, dtx_indx: %d\n", 
				__FUNCTION__, port_no,	bri_module->port[port_no].bytes2transmit,
							bri_module->port[port_no].dtx_indx);

			xhfc_write_fifo_dchan(fe, mod_no, bri_module, &bri_module->port[port_no], &free_space);
		}

		/* receive D-Channel Data */
		if (fifo_irq & (1 << (port_no*8+5)) ) {
			netskb_t *skb = NULL;
			DBG_MODULE_TESTER("There is Rx D-Channel Data for port_no %d\n", port_no);

			skb = wp_bri_dchan_rx(fe, mod_no, port_no);
			if(skb != NULL){
				sdla_t	*card = (sdla_t*)fe->card;
				private_area_t *chan;

				DEBUG_HFC_IRQ("%s(): Module: %d, port_no: %d.\n", __FUNCTION__, mod_no, port_no);
				chan=(private_area_t*)card->u.aft.dev_to_ch_map[BRI_DCHAN_LOGIC_CHAN];

				DEBUG_HFC_IRQ("%s(): chan ptr: 0x%p\n", __FUNCTION__, chan);
				if (!chan){
					DEBUG_ERROR("%s: Error: BRI D-Chan: No Device for Rx data.(logical ch=%d)\n",
							card->devname, BRI_DCHAN_LOGIC_CHAN);
					break;
				}
				wan_skb_queue_tail(&chan->wp_rx_bri_dchan_complete_list, skb);
				WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
			}

		}/* if ( fifo_irq & (1 << (port_no*8+5)) ) */
	}/* for (port_no = 0; port_no < bri_module->num_ports; port_no++) */
	/***************************************************************************/

	return 1;
}

static int32_t wp_bri_check_intr(sdla_fe_t *fe)
{
	/* must return 1! */
	return 1;
} 

static int32_t wp_bri_intr(sdla_fe_t *fe)
{
	u8		mod_no, port_no;
	int32_t		interrupt_serviced = 0;

	BRI_FUNC();

	if (!wan_test_bit(WP_BCB_BRI_CONFIG, &fe->bri_param.critical)) {
		return 0;
	}

	mod_no = fe_line_no_to_physical_mod_no(fe);	
	port_no = fe_line_no_to_port_no(fe);

	switch(fe->bri_param.mod[mod_no].type)
	{
	case MOD_TYPE_TE:
	case MOD_TYPE_NT:
		if(xhfc_interrupt(fe, mod_no)){
			/* at least one module generated an interrupt */
			interrupt_serviced = 1;
		}
		break;
	default:
		/* for missing (not installed) modules - do nothing  */
		break;
	}

	return interrupt_serviced;
}



