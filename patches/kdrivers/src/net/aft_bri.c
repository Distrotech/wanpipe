/*****************************************************************************
* aft_bri.c 
* 		
* 		WANPIPE(tm) ISDN BRI Hardware Support
*
* Authors: 	David Rokhvarg <davidr@sangoma.com>
*
* Copyright:	(c) 2003-2005 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Nov 9, 2006  David Rokhvarg	Initial Version
*****************************************************************************/

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)

# include <wanpipe_includes.h>
# include <wanpipe.h>
# include <wanpipe_abstr.h>
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>
# include <wanpipe_iface.h>

#elif defined(__WINDOWS__)

# include <wanpipe_includes.h>
# include <wanpipe_defines.h>
# include <wanpipe.h>

# include <wanpipe_abstr.h>
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>

#else
 
/* L I N U X */

# include <linux/wanpipe_includes.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe.h>
# include <linux/wanproc.h>
# include <linux/wanpipe_abstr.h>
# include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
# include <linux/if_wanpipe.h>
# include <linux/sdlapci.h>
# include <linux/aft_core.h>
# include <linux/wanpipe_iface.h>
# include <linux/wanpipe_tdm_api.h>

#endif

#undef BRI_DBG

#if 0
#warning "BRI_DBG  - Debugging Enabled"
#define BRI_DBG		if(1)DEBUG_EVENT
#else
#define BRI_DBG		if(0)DEBUG_EVENT
#endif

#if 0
#warning "DEBUG_REG  - Debugging Enabled"
#undef	DEBUG_REG
#define DEBUG_REG 	if(0)DEBUG_EVENT
#endif

#if 0
#warning "DEBUG_BRI  - Debugging Enabled"
#undef DEBUG_BRI
#define DEBUG_BRI	if(0)DEBUG_EVENT
#endif

#if 0
#warning "BRI_FUNC  - Debugging Enabled"
#undef BRI_FUNC()
#define BRI_FUNC()	DEBUG_EVENT("%s(): Line: %d\n", __FUNCTION__, __LINE__)
#endif



static inline void aft_set_bri_fifo_map(sdla_t *card, int port, unsigned int map)
{
	if (port >= MAX_BRI_MODULES) {
		card->u.aft.fifo_addr_map_l2 |= map;
	} else {
		card->u.aft.fifo_addr_map |= map;
	}
}

static inline void aft_clear_bri_fifo_map(sdla_t *card, unsigned int map)
{
	if (WAN_FE_LINENO(&card->fe) >= MAX_BRI_MODULES) {
		card->u.aft.fifo_addr_map_l2 &= map;
	} else {
		card->u.aft.fifo_addr_map &= map;
	}
}

static inline unsigned long aft_get_bri_fifo_map(sdla_t *card)
{	
	if (WAN_FE_LINENO(&card->fe) >= MAX_BRI_MODULES) {
		return card->u.aft.fifo_addr_map_l2;
	} 
	return card->u.aft.fifo_addr_map;
}


/*==============================================
 * PRIVATE FUNCITONS
 *
 */
#if defined(CONFIG_WANPIPE_HWEC)
static int aft_bri_hwec_reset(void *pcard, int reset);
static int aft_bri_hwec_enable(void *pcard, int enable, int fe_chan);
#endif

static int
read_bri_fe_byte(sdla_t *card, unsigned char mod_no, unsigned char reg, unsigned char *value,
				 unsigned char type, unsigned char optional_arg);
static int
write_bri_fe_byte(sdla_t *card, unsigned char mod_no, unsigned char addr, unsigned char value);


int bri_write_cpld(sdla_t *card, unsigned short off,unsigned char data);

static int aft_map_fifo_baddr_and_size(sdla_t *card, unsigned char fifo_size, unsigned char *addr);

static char fifo_size_vector[] =  {1, 2, 4, 8, 16, 32};
static char fifo_code_vector[] =  {0, 1, 3, 7,0xF,0x1F};

static int request_fifo_baddr_and_size(sdla_t *card, private_area_t *chan)
{
	unsigned char req_fifo_size,fifo_size;
	u32 i;

	BRI_FUNC();

	/* Calculate the optimal fifo size based
         * on the number of time slots requested */

	if (chan->num_of_time_slots == 1) {
		req_fifo_size=1;
	} else if (chan->num_of_time_slots == 2 || chan->num_of_time_slots == 3) {
		req_fifo_size=2;
	} else {
		DEBUG_EVENT("%s:%s: Invalid number of timeslots %d\n",
				card->devname,chan->if_name,chan->num_of_time_slots);
		return -EINVAL;		
	}
	

	for (i=0;i<WAN_FE_LINENO(&card->fe);i++){

		/* This loop will not run when FE_LINE==0 */

		/* Each bri line is separate, thus
	   	   we must account for the line number
	   	   and recognize that other cards 
	   	   have requested fifo */
		DEBUG_TEST("%s: Presetting Fifo: Line=%i, Value=0x%08X\n",
			card->devname,i,(0x03 << i%MAX_BRI_MODULES)*2);
		aft_set_bri_fifo_map(card,i,(0x03 << (i%MAX_BRI_MODULES)*2));
	}

	DEBUG_BRI("%s:%s: Optimal Fifo Size =%d  Timeslots=%d FifoMap=0x%08lX\n",
		card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots,
			aft_get_bri_fifo_map(card));

	fifo_size=(u8)aft_map_fifo_baddr_and_size(card,req_fifo_size,&chan->fifo_base_addr);
	if (fifo_size == 0 || chan->fifo_base_addr == 31){
		DEBUG_ERROR("%s:%s: Error: Failed to obtain fifo size %d or addr %d \n",
				card->devname,chan->if_name,fifo_size,chan->fifo_base_addr);
                return -EINVAL;
        }

	DEBUG_TEST("%s:%s: Optimal Fifo Size =%d  Timeslots=%d New Fifo Size=%d \n",
                card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots,fifo_size);


	for (i=0;i<sizeof(fifo_size_vector);i++){
		if (fifo_size_vector[i] == fifo_size){
			chan->fifo_size_code=fifo_code_vector[i];
			break;
		}
	}

	if (fifo_size != req_fifo_size){
		DEBUG_WARNING("%s:%s: Warning: Failed to obtain the req fifo %d got %d\n",
			card->devname,chan->if_name,req_fifo_size,fifo_size);
	}	

	DEBUG_TEST("%s: %s:Fifo Size=%d  Timeslots=%d Fifo Code=%d Addr=%d\n",
                card->devname,chan->if_name,fifo_size,
		chan->num_of_time_slots,chan->fifo_size_code,
		chan->fifo_base_addr);

	chan->fifo_size = fifo_size;

	return 0;
}


static int aft_map_fifo_baddr_and_size(sdla_t *card, unsigned char fifo_size, unsigned char *addr)
{
	u32 reg=0;
	u8 i;
	
	BRI_FUNC();

	for (i=0;i<fifo_size;i++){
		wan_set_bit(i,&reg);
	} 

	DEBUG_TEST("%s: Trying to MAP 0x%X  to 0x%lX\n",
                        card->devname,reg,aft_get_bri_fifo_map(card));

	for (i=0;i<32;i+=fifo_size){
		if (aft_get_bri_fifo_map(card) & (reg<<i)){
			continue;
		}
		aft_set_bri_fifo_map(card, WAN_FE_LINENO(&card->fe), (reg<<i));
		*addr=i;

		DEBUG_TEST("%s: Card fifo Map 0x%lX Addr =%d\n",
	                card->devname,aft_get_bri_fifo_map(card),i);

		return fifo_size;
	}

	if (fifo_size == 1){
		return 0; 
	}

	fifo_size = fifo_size >> 1;
	
	return aft_map_fifo_baddr_and_size(card,fifo_size,addr);
}


static int aft_free_fifo_baddr_and_size (sdla_t *card, private_area_t *chan)
{
	u32 reg=0;
	int i;
	
	BRI_FUNC();

	for (i=0;i<chan->fifo_size;i++){
                wan_set_bit(i,&reg);
        }
	
	DEBUG_TEST("%s: Unmapping 0x%X from 0x%lX\n",
		card->devname,reg<<chan->fifo_base_addr, aft_get_bri_fifo_map(card));

	aft_clear_bri_fifo_map(card, ~(reg<<chan->fifo_base_addr));

	DEBUG_TEST("%s: New Map is 0x%lX\n",
                card->devname, aft_get_bri_fifo_map(card));


	chan->fifo_size=0;
	chan->fifo_base_addr=0;

	return 0;
}


static int aft_request_logical_channel_num (sdla_t *card, private_area_t *chan)
{
	signed char logic_ch=-1;
	int err;
	int if_cnt=wan_atomic_read(&card->wandev.if_cnt);
	int if_offset=2;
	long i;

	BRI_FUNC();

	DEBUG_TEST("-- Request_Xilinx_logic_channel_num:-- (if_offset=%i)\n",if_offset);

	DEBUG_TEST("%s:%d Global Num Timeslots=%d  Global Logic ch Map 0x%lX \n",
		__FUNCTION__,__LINE__,
                card->u.aft.num_of_time_slots,
                card->u.aft.logic_ch_map);


	/* Check that the time slot is not being used. If it is
	 * stop the interface setup.  Notice, though we proceed
	 * to check for all timeslots before we start binding
	 * the channels in.  This way, we don't have to go back
	 * and clean the time_slot_map */ 
	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&chan->time_slot_map)){

			if (chan->first_time_slot == -1){
				DEBUG_EVENT("%s:    First TSlot   :%ld\n",
						card->devname,i);
				chan->first_time_slot=i;
			}

			chan->last_time_slot=i;

			DEBUG_CFG("%s: Configuring %s for timeslot %ld\n",
					card->devname, chan->if_name,i+1);

			if (wan_test_bit(i,&card->u.aft.time_slot_map)){
				DEBUG_EVENT("%s: Channel/Time Slot resource conflict!\n",
						card->devname);
				DEBUG_EVENT("%s: %s: Channel/Time Slot %ld, aready in use!\n",
						card->devname,chan->if_name,(i+1));

				return -EEXIST;
			}
		}
	}
		

	err=request_fifo_baddr_and_size(card,chan);
	if (err){
		return -1;
	}

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&chan->time_slot_map)){
			if (card->u.aft.security_id == 0){
				/* Unchannelized card must config
				* its hdlc logic ch on FIRST logic
				* ch number */
				
				if (chan->channelized_cfg) {
					if (card->u.aft.tdmv_dchan){
						/* In this case we KNOW that there is
						* only a single hdlc channel */ 
						if (i==0 && !chan->hdlc_eng){
							continue;
						}
					}
				}else{
					if (i==0 || i==1){
						if (!chan->hdlc_eng && 
						if_cnt < (card->u.aft.num_of_time_slots-if_offset)){ 
							continue;
						}
					}
				}
			}
			
			if (!wan_test_and_set_bit(i,&card->u.aft.logic_ch_map)){
				logic_ch=(char)i;
				break;
			}
		}
	}

	if (logic_ch == -1){
		return logic_ch;
	}

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (!wan_test_bit(i,&card->u.aft.logic_ch_map)){
			break;
		}
	}

	if (card->u.aft.dev_to_ch_map[(unsigned char)logic_ch]){
		DEBUG_ERROR("%s: Error, request logical ch=%d map busy\n",
				card->devname,logic_ch);
		return -1;
	}

	card->u.aft.dev_to_ch_map[(unsigned char)logic_ch]=(void*)chan;

	if (logic_ch >= card->u.aft.top_logic_ch){
		card->u.aft.top_logic_ch=logic_ch;
		aft_dma_max_logic_ch(card);
	}

	DEBUG_CFG("!!! %s: Binding logic ch %d  Ptr=%p\n",chan->if_name,logic_ch,chan);
	return logic_ch;
}



static int aft_test_hdlc(sdla_t *card)
{
	int i;
	int err;
	u32 reg;
	
	BRI_FUNC();

	for (i=0;i<10;i++){
		card->hw_iface.bus_read_4(card->hw,AFT_CHIP_CFG_REG, &reg);

		if (!wan_test_bit(AFT_CHIPCFG_HDLC_CTRL_RDY_BIT,&reg) ||
		    !wan_test_bit(AFT_CHIPCFG_RAM_READY_BIT,&reg)){
			/* The HDLC Core is not ready! we have
			 * an error. */
			err = -EINVAL;
			WP_DELAY(200);
		}else{
			err=0;
			break;
		}
	}

	return err;
}


/*==============================================
 * PUBLIC FUNCITONS
 *
 */

int aft_bri_test_sync(sdla_t *card, int tx_only)
{
	volatile int i,err=1;
	u32 reg;
	
	BRI_FUNC();

	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
		
	if (wan_test_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg)){
		DEBUG_WARNING("%s: Warning: BRI Reset Enabled %d! \n",
				card->devname, card->wandev.comm_port+1);
	}

	for (i=0;i<500;i++){
	
		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
		if (tx_only){
			if (wan_test_bit(AFT_LCFG_TX_FE_SYNC_STAT_BIT,&reg)){
				err=-1;
				WP_DELAY(200);
			}else{
				err=0;
				break;
			}
		}else{	
			if (wan_test_bit(AFT_LCFG_TX_FE_SYNC_STAT_BIT,&reg) ||
			    wan_test_bit(AFT_LCFG_RX_FE_SYNC_STAT_BIT,&reg)){
				err=-1;
				WP_DELAY(200);
			}else{
				err=0;
				break;
			}
		}
	}

	DEBUG_BRI("%s: DELAY INDEX = %i, AFT_LINE_CFG_REG: 0x%X\n",
			card->devname,i, reg);

	return err;
}

int aft_bri_led_ctrl(sdla_t *card, int color, int led_pos, int on)
{
	u8 reg;
	u16 red_bit=0,green_bit=0;
	u32 port = WAN_FE_LINENO(&card->fe);
	AFT_FUNC_DEBUG();
			
	if (card->adptr_type != AFT_ADPTR_B500) {
		return 0;
	}
	
	red_bit=port*2;
	green_bit=(port*2)+1;

	reg=aft_bri_read_cpld(card,0x01);

	DEBUG_TEST("%s: READ CPLD 0x%X\n",card->devname,reg);

	/* INSERT LED CODE */
	switch (color){
		
	case WAN_AFT_RED:
		if (on){
			wan_set_bit(red_bit,&reg);
		}else{
			wan_clear_bit(red_bit,&reg);
		}	
		break;

	case WAN_AFT_GREEN:
		if (on){
			wan_set_bit(green_bit,&reg);
		}else{
			wan_clear_bit(green_bit,&reg);
		}	
		break;			
	default:
		return 0;
	}

	DEBUG_TEST("%s: WRITE CPLD 0x%X red_bit %i green_bit %i port %i\n",
					card->devname, reg,red_bit, green_bit, port);
	aft_bri_write_cpld(card,0x01,reg);

	return 0;
}


int aft_bri_cpld0_set(sdla_t *card, int hwec_reset) 
{
	wan_bitmap_t cpld_reg=0;
	u32 reg=0;

	card->hw_iface.bus_read_4(card->hw,AFT_CHIP_CFG_REG,&reg);

	if (hwec_reset) {
		/* Reset ECHO Canceler */
		wan_clear_bit(BRI_CPLD0_ECHO_RESET_BIT,&cpld_reg);
	} else {
		/* Clear Reset Echo Canceler */
		wan_set_bit(BRI_CPLD0_ECHO_RESET_BIT,&cpld_reg);
	}

	/* If the jumper is used as a network sync we must 
         * configure the CPLD for INPUT, Otherwise send the clock
         * out as OUTPUT */
	if (wan_test_bit(AFT_CHIPCFG_A500_NET_SYNC_CLOCK_SELECT_BIT,&reg)) {
		wan_clear_bit(BRI_CPLD0_NETWORK_SYNC_OUT_BIT,&cpld_reg);
	} else {
		wan_set_bit(BRI_CPLD0_NETWORK_SYNC_OUT_BIT,&cpld_reg);
	}

	DEBUG_TEST("%s: Writing to CPLD0 0x%02X (REG=0x%08X)\n",
		card->devname,cpld_reg,reg);

	aft_bri_write_cpld(card,0x00,cpld_reg);

	return 0;
}


int aft_bri_global_chip_config(sdla_t *card)
{
	u32 reg;
	int err=0;
	int used_cnt;
	wan_smp_flag_t smp_flags,flags;

	BRI_FUNC();

	//FIME: hardcoded
	card->u.aft.firm_id = AFT_DS_FE_CORE_ID;

	/*============ GLOBAL CHIP CONFIGURATION ===============*/

	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);

	if (used_cnt == 1) {
		/* Enable the chip/hdlc reset condition */
		reg=0;
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
	
		DEBUG_CFG("--- AFT Chip Reset. -- \n");
	
		card->hw_iface.bus_write_4(card->hw,AFT_CHIP_CFG_REG,reg);
		
		WP_DELAY(10);
	
		/* Disable the chip/hdlc reset condition */
		wan_clear_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_clear_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
		
		wan_clear_bit(AFT_CHIPCFG_FE_INTR_CFG_BIT,&reg);
	
		if (WAN_FE_NETWORK_SYNC(&card->fe)){	/*card->fe.fe_cfg.cfg.remora.network_sync*/
			DEBUG_EVENT("%s: ISDN BRI Clock set to (External) Network Sync!\n",
					card->devname);
			wan_set_bit(AFT_CHIPCFG_A500_NET_SYNC_CLOCK_SELECT_BIT,&reg);	
		} else {
			wan_clear_bit(AFT_CHIPCFG_A500_NET_SYNC_CLOCK_SELECT_BIT,&reg);	
			
		}
		DEBUG_CFG("--- Chip enable/config. -- \n");

		card->hw_iface.bus_write_4(card->hw,AFT_CHIP_CFG_REG,reg);

	} else {
		if (WAN_FE_NETWORK_SYNC(&card->fe)){
			DEBUG_EVENT("%s: Ignoring Network Sync\n", card->devname);
		}
	}

	if (!IS_BRI_CARD(card)) {
			DEBUG_ERROR("%s: Error: Xilinx doesn't support non BRI interface!\n",
					card->devname);
			return -EINVAL;
	}

	if (used_cnt == 1) {
		card->hw_iface.hw_lock(card->hw,&smp_flags);
		wan_spin_lock_irq(&card->wandev.lock,&flags);
		
		/* Reset HWEC and Set CPLD based on network sync */
		aft_bri_cpld0_set(card,1);

		/* Turn on all LED lights by default */
		if (card->adptr_type == AFT_ADPTR_B500) {
			aft_bri_write_cpld(card,0x01,0xFF);
		}
		
		wan_spin_unlock_irq(&card->wandev.lock,&flags);
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
	}
     	
	err=aft_test_hdlc(card);
	if (err != 0){
		DEBUG_ERROR("%s: Error: HDLC Core Not Ready (0x%X)!\n",
					card->devname,reg);
		return -EINVAL;
    	} else{
		DEBUG_CFG("%s: HDLC Core Ready\n",
                                        card->devname);
    	}

	err = -EINVAL;
	if (card->wandev.fe_iface.global_config){
		err=card->wandev.fe_iface.global_config(&card->fe);
	}
	if (err){
		return err;
	}

	aft_fe_intr_ctrl(card, 1);

	return 0;

}

int aft_bri_global_chip_unconfig(sdla_t *card)
{
	u32 reg=0;
	int used_cnt;
	
	BRI_FUNC();

	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);
	/* Global BRI unconfig */
	if (card->wandev.fe_iface.global_unconfig){
		card->wandev.fe_iface.global_unconfig(&card->fe);
	}

	if (used_cnt == 1) {
		/* Set Octasic to reset */
		aft_bri_cpld0_set(card,1);

		/* Turn on all LED by default */
		if (card->adptr_type == AFT_ADPTR_B500) {
			aft_bri_write_cpld(card,0x01,0xFF);
		}
	
		/* Disable the chip/hdlc reset condition */
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
		wan_clear_bit(AFT_CHIPCFG_FE_INTR_CFG_BIT,&reg);
	
		card->hw_iface.bus_write_4(card->hw,AFT_CHIP_CFG_REG,reg);
	}

	return 0;
}


int aft_bri_hwec_config(sdla_t *card, wandev_conf_t *conf)
{
	/* Enable Octasic Chip */
	u16	max_ec_chans, max_chans_no;
	u32 	cfg_reg, fe_chans_map;

	
	card->wandev.ec_dev = NULL;
	card->wandev.hwec_reset = NULL;
	card->wandev.hwec_enable = NULL;

	card->hw_iface.getcfg(card->hw, SDLA_HWEC_NO, &max_ec_chans);
	card->hw_iface.getcfg(card->hw, SDLA_CHANS_NO, &max_chans_no);
	card->hw_iface.getcfg(card->hw, SDLA_CHANS_MAP, &fe_chans_map);

	card->hw_iface.bus_read_4(card->hw,AFT_CHIP_CFG_REG, &cfg_reg); 

	if (max_ec_chans > A500_MAX_EC_CHANS){
		DEBUG_EVENT(
		"%s: Critical Error: Exceeded Maximum Available Echo Channels!\n",
				card->devname);
		DEBUG_EVENT(
		"%s: Critical Error: Max Allowed=%d Configured=%d (%X)\n",
				card->devname,
				A500_MAX_EC_CHANS,
				max_ec_chans,
				cfg_reg);  
		return -EINVAL;
	}

	if (max_ec_chans){
#if defined(CONFIG_WANPIPE_HWEC)
		card->wandev.ec_dev = wanpipe_ec_register(
						card,
						fe_chans_map,
						max_chans_no,	/* b-chans number is 2 */
						max_ec_chans,
						(void*)&conf->oct_conf);
		if (!card->wandev.ec_dev) {
			DEBUG_EVENT(
			"%s: Failed to register device in HW Echo Canceller module!\n",
							card->devname);
			return -EINVAL;
		}

		card->wandev.hwec_reset  = aft_bri_hwec_reset;
		card->wandev.hwec_enable = aft_bri_hwec_enable;

		/* Only suppress H100 errors for old CPLD */
		if (!aft_is_bri_512khz_card(card)){
			wan_event_ctrl_t *event = wan_malloc(sizeof(wan_event_ctrl_t));
			if (event) {
				memset(event,0,sizeof(wan_event_ctrl_t));
				event->type=WAN_EVENT_EC_H100_REPORT;
				event->mode=WAN_EVENT_DISABLE;
				wanpipe_ec_event_ctrl(card->wandev.ec_dev,card,event);
				DEBUG_EVENT("%s: Wanpipe HW Echo Canceller H100 Ignore!\n",
						card->devname);
			}
		}
#else
		DEBUG_EVENT("%s: Wanpipe HW Echo Canceller modele is not compiled!\n",
					card->devname);
#endif
	}else{
		DEBUG_EVENT(
			"%s: WARNING: No Echo Canceller channels are available!\n",
					card->devname);
	}

	return 0;
}

int aft_bri_chip_config(sdla_t *card, wandev_conf_t *conf)
{
	u32 reg=0, ctrl_ram_reg=0;
	int i,err=0;
	wan_smp_flag_t smp_flags;
	int used_cnt;
	int used_type_cnt;

	BRI_FUNC();

	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);
	card->hw_iface.getcfg(card->hw, SDLA_HWTYPE_USEDCNT, &used_type_cnt);


	/* Check for NETWORK SYNC IN Clocking, if network sync in
         * is enabled ignore all clock_modes */
	card->hw_iface.bus_read_4(card->hw,AFT_CHIP_CFG_REG,&reg);
	if (wan_test_bit(AFT_CHIPCFG_A500_NET_SYNC_CLOCK_SELECT_BIT,&reg)) {
		card->fe.fe_cfg.cfg.bri.clock_mode=0;
	}

	reg=0;

	if (used_type_cnt > 1) {

		card->hw_iface.hw_lock(card->hw,&smp_flags);

		err = -EINVAL;
		if (card->wandev.fe_iface.config){
			err=card->wandev.fe_iface.config(&card->fe);
		}
		
		card->hw_iface.hw_unlock(card->hw,&smp_flags);

		if (err) {
			DEBUG_EVENT("%s: Failed BRI configuration!\n",
                                	card->devname);
			return err;
		}	

		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	

		err=aft_bri_hwec_config(card,conf);

		return err;
	}


	/*==========================================================
         * This section of the code will run only ONCE
         * On VERY FIRST BRI Module config 
         *=========================================================*/

	card->hw_iface.hw_lock(card->hw,&smp_flags);

	err = -EINVAL;
	if (card->wandev.fe_iface.config){
		err=card->wandev.fe_iface.config(&card->fe);
	}
		
	card->hw_iface.hw_unlock(card->hw,&smp_flags);

	if (err){
		DEBUG_EVENT("%s: Failed BRI configuration!\n",
                                	card->devname);
		return -EINVAL;
       	}
	/* Run rest of initialization not from lock */
	if (card->wandev.fe_iface.post_init){
		err=card->wandev.fe_iface.post_init(&card->fe);
	}
	
	DEBUG_EVENT("%s: Front end successful\n",
			card->devname);


	/*============ LINE/PORT CONFIG REGISTER ===============*/

	/* FE synch. For BRI reset done only ONCE for both lines */
	card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
	wan_set_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);


	WP_DELAY(10);

	wan_clear_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);

	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);

	WP_DELAY(10);

	err=aft_bri_test_sync(card,1);

	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
	if (err != 0){
		DEBUG_ERROR("%s: Error: Front End Interface Not Ready (0x%08X)!\n",
					card->devname,reg);
		return err;
	} else{
		DEBUG_EVENT("%s: Front End Interface Ready 0x%08X\n",
                                        card->devname,reg);
	}


	err=aft_bri_hwec_config(card,conf);
	if (err) {
		return err;
	}	

	/* Enable only Front End Interrupt
	 * Wait for front end to come up before enabling DMA */
	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
	wan_clear_bit(AFT_LCFG_DMA_INTR_BIT,&reg);
	wan_clear_bit(AFT_LCFG_FIFO_INTR_BIT,&reg);
	wan_clear_bit(AFT_LCFG_TDMV_INTR_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), reg);

	card->u.aft.lcfg_reg=reg;
	

	/*============ DMA CONTROL REGISTER ===============*/
	
	/* Disable Global DMA because we will be 
	 * waiting for the front end to come up */
	reg=0;
	aft_dmactrl_set_max_logic_ch(&reg,0);
	wan_clear_bit(AFT_DMACTRL_GLOBAL_INTR_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_DMA_CTRL_REG),reg);


	/* Initialize all BRI timeslots ONLY on VERY FIRST BRI module */
	reg=0;
	for (i=0;i<32;i++){
		ctrl_ram_reg=AFT_PORT_REG(card,AFT_CONTROL_RAM_ACCESS_BASE_REG);
		ctrl_ram_reg+=(i*4);
		
		aft_ctrlram_set_logic_ch(&reg,0x1F);
		aft_ctrlram_set_fifo_size(&reg,0);
		aft_ctrlram_set_fifo_base(&reg,0x1F);
	
		wan_set_bit(AFT_CTRLRAM_HDLC_MODE_BIT,&reg);
		wan_set_bit(AFT_CTRLRAM_HDLC_TXCH_RESET_BIT,&reg);
		wan_set_bit(AFT_CTRLRAM_HDLC_RXCH_RESET_BIT,&reg);
				
		card->hw_iface.bus_write_4(card->hw, ctrl_ram_reg, reg);
	}
	
	aft_wdt_reset(card);

	return 0;

}

int aft_bri_chip_unconfig(sdla_t *card)
{
	wan_smp_flag_t smp_flags,smp_flags1;
		
	BRI_FUNC();

	/* chip unconfig is done in disable_comms() */

	/* Disable Octasic for this BRI module */
	if (card->wandev.ec_dev){
#if defined(CONFIG_WANPIPE_HWEC)
		DEBUG_EVENT("%s: Unregisterd HWEC\n",
						card->devname);
		wanpipe_ec_unregister(card->wandev.ec_dev, card);
#else
		DEBUG_EVENT("%s: Wanpipe HW Echo Canceller modele is not compiled!\n",
					card->devname);
#endif
	}

	card->wandev.hwec_enable = NULL;
	card->wandev.ec_dev = NULL;

	card->hw_iface.hw_lock(card->hw,&smp_flags1);
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	if (card->wandev.fe_iface.unconfig){
		card->wandev.fe_iface.unconfig(&card->fe);
	}
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags1);

	return 0;
}

int aft_bri_chan_dev_config(sdla_t *card, void *chan_ptr)
{
	u32 reg;
	long i;
	int chan_num=-EBUSY;
	private_area_t *chan = (private_area_t*)chan_ptr;
	u32 ctrl_ram_reg,dma_ram_reg;

	BRI_FUNC();

	if(chan->dchan_time_slot >= 0){
		BRI_FUNC();
 		chan->logic_ch_num = 2;
		card->u.aft.dev_to_ch_map[BRI_DCHAN_LOGIC_CHAN]=(void*)chan;
		/* configure bri dchan here */
		return 0;
	}

	chan_num=aft_request_logical_channel_num(card, chan);
	if (chan_num < 0){
		return -EBUSY;
	}
	chan->logic_ch_num = chan_num;

	dma_ram_reg=AFT_PORT_REG(card,AFT_DMA_CHAIN_RAM_BASE_REG);
	dma_ram_reg+=(chan->logic_ch_num*4);

	reg=0;
	card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);

	card->hw_iface.bus_read_4(card->hw, dma_ram_reg, &reg);

	aft_dmachain_set_fifo_size(&reg, chan->fifo_size_code);
	aft_dmachain_set_fifo_base(&reg, chan->fifo_base_addr);

	/* Initially always disable rx synchronization */
	wan_clear_bit(AFT_DMACHAIN_RX_SYNC_BIT,&reg);

	/* Enable SS7 if configured by user */
	if (chan->cfg.ss7_enable){
		wan_set_bit(AFT_DMACHAIN_SS7_ENABLE_BIT,&reg);
	}else{
		wan_clear_bit(AFT_DMACHAIN_SS7_ENABLE_BIT,&reg);
	}

	if (CHAN_GLOBAL_IRQ_CFG(chan)){
		aft_dmachain_enable_tdmv_and_mtu_size(&reg,chan->mru);
	}
	
	card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);

	reg=0;	


	for (i=0;i<card->u.aft.num_of_time_slots;i++){


		ctrl_ram_reg=AFT_PORT_REG(card,AFT_CONTROL_RAM_ACCESS_BASE_REG);
		ctrl_ram_reg+=(i*4);

		if (wan_test_bit(i,&chan->time_slot_map)){

			BRI_FUNC();
			
			wan_set_bit(i,&card->u.aft.time_slot_map);
			
			card->hw_iface.bus_read_4(card->hw, ctrl_ram_reg, &reg);

			aft_ctrlram_set_logic_ch(&reg,chan->logic_ch_num);

			if (i == chan->first_time_slot){
				wan_set_bit(AFT_CTRLRAM_SYNC_FST_TSLOT_BIT,&reg);
			}
				
			aft_ctrlram_set_fifo_size(&reg,chan->fifo_size_code);

			aft_ctrlram_set_fifo_base(&reg,chan->fifo_base_addr);
			
			
			if (chan->hdlc_eng){
				wan_set_bit(AFT_CTRLRAM_HDLC_MODE_BIT,&reg);
			}else{
				wan_clear_bit(AFT_CTRLRAM_HDLC_MODE_BIT,&reg);
			}

			if (chan->cfg.data_mux){
				wan_set_bit(AFT_CTRLRAM_DATA_MUX_ENABLE_BIT,&reg);
			}else{
				wan_clear_bit(AFT_CTRLRAM_DATA_MUX_ENABLE_BIT,&reg);
			}
			
			if (0){ /* FIXME card->fe.fe_cfg.cfg.te1cfg.fcs == 32){ */
				wan_set_bit(AFT_CTRLRAM_HDLC_CRC_SIZE_BIT,&reg);
			}else{
				wan_clear_bit(AFT_CTRLRAM_HDLC_CRC_SIZE_BIT,&reg);
			}

			/* Enable SS7 if configured by user */
			if (chan->cfg.ss7_enable){
				wan_set_bit(AFT_CTRLRAM_SS7_ENABLE_BIT,&reg);
			}else{
				wan_clear_bit(AFT_CTRLRAM_SS7_ENABLE_BIT,&reg);
			}

			wan_clear_bit(AFT_CTRLRAM_HDLC_TXCH_RESET_BIT,&reg);
			wan_clear_bit(AFT_CTRLRAM_HDLC_RXCH_RESET_BIT,&reg);

			DEBUG_CFG("%s: Configuring %s LC=%i for timeslot %ld : Offset 0x%X Reg 0x%X\n",
					card->devname, chan->if_name, chan->logic_ch_num, i,
					ctrl_ram_reg,reg);

			card->hw_iface.bus_write_4(card->hw, ctrl_ram_reg, reg);

		}
	}


	if (CHAN_GLOBAL_IRQ_CFG(chan)){
	
		BRI_FUNC();

		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
		aft_lcfg_tdmv_cnt_inc(&reg);

		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);
		card->u.aft.lcfg_reg=reg;

		wan_set_bit(chan->logic_ch_num,&card->u.aft.tdm_logic_ch_map);

		DEBUG_TEST("%s: TDMV CNT = %i\n",
				card->devname,
				(reg>>AFT_LCFG_TDMV_CH_NUM_SHIFT)&AFT_LCFG_TDMV_CH_NUM_MASK);
	}

	return 0;
}

int aft_bri_chan_dev_unconfig(sdla_t *card, void *chan_ptr)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	volatile int i;
	u32 dma_ram_reg,ctrl_ram_reg,reg;

	DEBUG_BRI("%s: 1 BRI DEV UNCONFIG %i\n",
				card->devname, chan->logic_ch_num);

	if( chan->dchan_time_slot >= 0) {
		
		DEBUG_BRI("%s: 2 BRI DEV DCHAN UNCONFIG %i\n",
				card->devname, chan->logic_ch_num);

 		chan->logic_ch_num = 2;
		card->u.aft.dev_to_ch_map[BRI_DCHAN_LOGIC_CHAN]=NULL;
		/* configure bri dchan here */
		return 0;
	}

	/* Select an HDLC logic channel for configuration */
	if (chan->logic_ch_num != -1){
	
		DEBUG_BRI("%s: 2 BRI DEV UNCONFIG %i\n",
				card->devname, chan->logic_ch_num);

		dma_ram_reg=AFT_PORT_REG(card,AFT_DMA_CHAIN_RAM_BASE_REG);
		dma_ram_reg+=(chan->logic_ch_num*4);

		card->hw_iface.bus_read_4(card->hw, dma_ram_reg, &reg);

		aft_dmachain_set_fifo_base(&reg,0x1F);
		aft_dmachain_set_fifo_size(&reg,0);
		card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);


	        for (i=0;i<card->u.aft.num_of_time_slots;i++){
        	        if (wan_test_bit(i,&chan->time_slot_map)){
				BRI_FUNC();
				ctrl_ram_reg=AFT_PORT_REG(card,AFT_CONTROL_RAM_ACCESS_BASE_REG);
				ctrl_ram_reg+=(i*4);

				reg=0;
				aft_ctrlram_set_logic_ch(&reg,0x1F);

				aft_ctrlram_set_fifo_base(&reg,0x1F);
				aft_ctrlram_set_fifo_size(&reg,0);
			
				wan_set_bit(AFT_CTRLRAM_HDLC_MODE_BIT,&reg);
				wan_set_bit(AFT_CTRLRAM_HDLC_TXCH_RESET_BIT,&reg);
				wan_set_bit(AFT_CTRLRAM_HDLC_RXCH_RESET_BIT,&reg);
					
				card->hw_iface.bus_write_4(card->hw, ctrl_ram_reg, reg);
			}
		}

		aft_free_logical_channel_num(card,chan->logic_ch_num);
		aft_free_fifo_baddr_and_size(card,chan);

		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			if (wan_test_bit(i,&chan->time_slot_map)){
				wan_clear_bit(i,&card->u.aft.time_slot_map);
			}
		}

		if (CHAN_GLOBAL_IRQ_CFG(chan)){
			BRI_FUNC();
			card->hw_iface.bus_read_4(card->hw,
					AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
			aft_lcfg_tdmv_cnt_dec(&reg);
			card->hw_iface.bus_write_4(card->hw,
					AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);
			wan_clear_bit(chan->logic_ch_num,&card->u.aft.tdm_logic_ch_map);
			DEBUG_TEST("%s: TDMV CNT = %i\n",
				card->devname,
				(reg>>AFT_LCFG_TDMV_CH_NUM_SHIFT)&AFT_LCFG_TDMV_CH_NUM_MASK);
		}

		/* Do not clear the logi_ch_num here. 
		   We will do it at the end of del_if_private() funciton */
	}

	return 0;
}
 
int bri_check_ec_security(sdla_t *card)     
{
	u32 cfg_reg;	
	u32 security_bit=AFT_CHIPCFG_A500_EC_SECURITY_BIT;

    	card->hw_iface.bus_read_4(card->hw,AFT_CHIP_CFG_REG, &cfg_reg);
	if (wan_test_bit(security_bit,&cfg_reg)){ 
    		return 1; 	
	}
	return 0;
}


unsigned char aft_bri_read_cpld(sdla_t *card, unsigned short cpld_off)
{
    u8      tmp=0;
	int	err = -EINVAL;

	BRI_FUNC();

	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
		return 0x00;
	}

	if (card->hw_iface.read_cpld){
		err = card->hw_iface.read_cpld(card->hw, (u16)cpld_off, &tmp);
	}

	card->hw_iface.fe_clear_bit(card->hw,0);

        return tmp;
}

int aft_bri_write_cpld(sdla_t *card, unsigned short off,unsigned short data)
{
	int	err = -EINVAL;

	BRI_FUNC();


	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
		return 0x00;
	}

	if (card->hw_iface.write_cpld){
		err = card->hw_iface.write_cpld(card->hw, (u16)off, (u8)data);
	}

	card->hw_iface.fe_clear_bit(card->hw,0);

        return 0;
}


int bri_write_cpld(sdla_t *card, unsigned short off,unsigned char data)
{
	u16             org_off;

	BRI_FUNC();

	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
		return 0x00;
	}

        off &= ~AFT8_BIT_DEV_ADDR_CLEAR;
        off |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;

        /*ALEX: Save the current original address */
        card->hw_iface.bus_read_2(card->hw,
                                AFT_MCPU_INTERFACE_ADDR,
                                &org_off);

	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	WP_DELAY(5);

        card->hw_iface.bus_write_2(card->hw,
                                AFT_MCPU_INTERFACE_ADDR,
                                off);
        
	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	WP_DELAY(5);

	card->hw_iface.bus_write_1(card->hw,
                                AFT_MCPU_INTERFACE,
                                data);
        /*ALEX: Restore the original address */
        card->hw_iface.bus_write_2(card->hw,
                                AFT_MCPU_INTERFACE_ADDR,
                                org_off);
	card->hw_iface.fe_clear_bit(card->hw,0);

        return 0;
}


void aft_bri_fifo_adjust(sdla_t *card, u32 level)
{
	u32 fifo_size,reg;
	card->hw_iface.bus_read_4(card->hw, AFT_FIFO_MARK_REG, &fifo_size);
	
	BRI_FUNC();

	aft_fifo_mark_gset(&reg,(u8)level);

	if (level == 1) {
		/* FIXME: This is a kluge. Have fifo adjust for each
		          fifo size 
		   For 32 bit fifo if level is 1 set it to zero */
        	reg&=~(0x1);
	}

	if (fifo_size == reg){
		return;
	}
	
	card->hw_iface.bus_write_4(card->hw, AFT_FIFO_MARK_REG, reg);		
	DEBUG_EVENT("%s:    Fifo Level Map:0x%08X\n",card->devname,reg);
}


#if defined(CONFIG_WANPIPE_HWEC)
static int aft_bri_hwec_reset(void *pcard, int reset)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_smp_flag_t	smp_flags;
	wan_smp_flag_t  flags;
	int		err = -EINVAL;

	card->hw_iface.hw_lock(card->hw,&smp_flags);
	wan_spin_lock_irq(&card->wandev.lock,&flags);
	if (!reset){
		DEBUG_EVENT("%s: Clear Echo Canceller chip reset.\n",
					card->devname);

		/* Clear RESET on HWEC */
		aft_bri_cpld0_set(card,0);

		WP_DELAY(1000);
		err = 0;

	}else{
		DEBUG_EVENT("%s: Set Echo Canceller chip reset.\n",
					card->devname);

		/* RESET HWEC */
		aft_bri_cpld0_set(card,1);

		err = 0;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);
	return err;
}
#endif

#if defined(CONFIG_WANPIPE_HWEC)
/******************************************************************************
**              aft_bri_hwec_enable()
**
** Return:      0   - success
**              1   - channel out of channel map
**              < 0 - failed
******************************************************************************/
static int aft_bri_hwec_enable(void *pcard, int enable, int fe_chan)
{
	sdla_t		*card = (sdla_t*)pcard;
	unsigned int	value, new_chan, bri_chan;

	DEBUG_HWEC("%s(): pcard: 0x%p\n", __FUNCTION__, pcard);

	WAN_ASSERT(card == NULL);

	DEBUG_HWEC("[HWEC BRI]: %s: %s bypass mode for channel %d, LineNo: %d!\n",
			card->devname,
			(enable) ? "Enable" : "Disable",
			fe_chan, WAN_FE_LINENO(&card->fe));

	/* make sure channel is 0 or 1, nothing else!! */
	if(fe_chan != 1 && fe_chan != 2){
		DEBUG_HWEC("[HWEC BRI]: %s: invalid channel %d. Must be 1 or 2!\n",
			card->devname, fe_chan);
		return -EINVAL;
	}

	new_chan = 2 * WAN_FE_LINENO(&card->fe); /* 0, 2, 4...*/
	bri_chan = new_chan + (fe_chan-1); /* {0,1}, {2,3}, {4,5}... */

	DEBUG_HWEC("bri_chan: %d\n", bri_chan);

	DEBUG_BRI("Offset: 0x%X!\n", AFT_PORT_REG(card,0x1000) + bri_chan * 4);

	card->hw_iface.bus_read_4(
			card->hw,
			AFT_PORT_REG(card,0x1000) + bri_chan * 4,
			&value);
	if (enable){
		value |= 0x20;
	}else{
		value &= ~0x20;
	}

	DEBUG_HWEC("[HWEC BRI]: %s: writing: 0x%08X!\n",card->devname, value);

	card->hw_iface.bus_write_4(
			card->hw,
			AFT_PORT_REG(card,0x1000) + bri_chan * 4,
			value);

	return 0;
}
#endif


/***************************************************************************
	BRI card
***************************************************************************/

#if defined(CONFIG_PRODUCT_WANPIPE_AFT_BRI)

#if 1
/*============================================================================
 * Write BRI register
 */

int aft_bri_write_fe(void* pcard, ...)
{
	va_list		args;
	sdla_t		*card = (sdla_t*)pcard;
	u8		mod_no, type, reg, value;
#if defined(WAN_DEBUG_FE)
	char		*fname;	
	int		fline;
#endif

	WAN_ASSERT(card == NULL);
	WAN_ASSERT(card->hw_iface.bus_write_4 == NULL);
	WAN_ASSERT(card->hw_iface.bus_read_4 == NULL);

	va_start(args, pcard);
	mod_no		= (u8)va_arg(args, int);
	type		= (u8)va_arg(args, int);
	reg		= (u8)va_arg(args, int);
	value		= (u8)va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			card->devname, __FUNCTION__,__LINE__, fname, fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
#endif			
		return -EINVAL;
	}

	write_bri_fe_byte(card, mod_no, reg, value);

	card->hw_iface.fe_clear_bit(card->hw,0);
	return 0;
}


/*============================================================================
 * Read bri register
 */

unsigned char aft_bri_read_fe (void* pcard, ...)
{
	va_list		args;
	sdla_t		*card = (sdla_t*)pcard;
	u8		mod_no, type, optional_arg, reg;
	unsigned char	data = 0;
#if defined(WAN_DEBUG_FE)
	char		*fname;
	int		fline;
#endif

	WAN_ASSERT(card == NULL);
	WAN_ASSERT(card->hw_iface.bus_write_4 == NULL);
	WAN_ASSERT(card->hw_iface.bus_read_4 == NULL);

	va_start(args, pcard);
	mod_no			= (u8)va_arg(args, int);
	type			= (u8)va_arg(args, int);
	optional_arg		= (u8)va_arg(args, int);
	reg			= (u8)va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			card->devname, __FUNCTION__,__LINE__,fname,fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
#endif		
		return 0x00;
	}

	read_bri_fe_byte(card, mod_no, reg, &data, type, optional_arg);

	card->hw_iface.fe_clear_bit(card->hw,0);
	return data;
}

/*============================================================================
 * Read bri register - debugging version
 */

unsigned char __aft_bri_read_fe (void* pcard, ...)
{
	va_list		args;
	sdla_t		*card = (sdla_t*)pcard;
	u8		mod_no, type, optional_arg, reg;
	unsigned char	data = 0;
#if defined(WAN_DEBUG_FE)
	char		*fname;
	int		fline;
#endif

	WAN_ASSERT(card == NULL);
	WAN_ASSERT(card->hw_iface.bus_write_4 == NULL);
	WAN_ASSERT(card->hw_iface.bus_read_4 == NULL);

	va_start(args, pcard);
	mod_no		= (u8)va_arg(args, int);
	type		= (u8)va_arg(args, int);
	optional_arg	= (u8)va_arg(args, int);
	reg		= (u8)va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	read_bri_fe_byte(card, mod_no, reg, &data, type, optional_arg);

	return data;
}


#define SPI_DELAY if(0)WP_DELAY(10)

#undef SPI_MAX_RETRY_COUNT
#define SPI_MAX_RETRY_COUNT 1000

#define MYBREAK if(1)break

#define FAST_SPI	0

static int
read_bri_fe_byte(sdla_t *card, unsigned char mod_no, unsigned char reg, unsigned char *value,
				 unsigned char type, unsigned char optional_arg)
{
	bri_reg_t	data, dummy;
	u_int32_t	*data_ptr = (u_int32_t*)&data;
	u_int32_t	*dummy_ptr = (u_int32_t*)&dummy;
	u_int32_t	retry_counter;
	u_int8_t	rm_no=0xFF;
	
	WAN_ASSERT(card == NULL);

#if defined(WAN_DEBUG_REG)
	BRI_DBG("%s():%s\n", __FUNCTION__, card->devname);
#endif

	if(type != MOD_TYPE_NT && type != MOD_TYPE_TE){
#if 0
		BRI_DBG("%s(): Warning: unknown module type! (%d)\n", __FUNCTION__, type);
#endif
	}

	/* setup address offset for fe */
	data.reset=0;
	data.start=1;
	data.reserv1=0;

	if(type == MOD_TYPE_NONE){
		/* the only case we get here is if running module detection code. */
		rm_no = optional_arg;
	}else{
		/*	Input mod_no is an even number between 0 and 22 (including).
			Calculate rm_no - should be between 0 and 3 (including). 
		*/
		if(mod_no % 2){
			DEBUG_BRI("%s(): Warning: module number (%d) is not even!!\n",
				__FUNCTION__, mod_no);
		}
		rm_no = mod_no / (MAX_BRI_MODULES_PER_REMORA*2);
#if 0
		BRI_DBG("%s(input): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
		/* Translate mod_no to be between 0 and 2 (including) */
		mod_no = (mod_no / 2) % MAX_BRI_MODULES_PER_REMORA;
	}
#if 0
	BRI_DBG("%s(updated): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
	if(rm_no > MAX_BRI_REMORAS - 1){
		DEBUG_EVENT("%s:%s(): Line:%d: invalid rm_no: %d!!(mod_no: %d)\n",
			card->devname, __FUNCTION__, __LINE__,	rm_no, mod_no);	
		return 0;
	}

	data.remora_addr = rm_no;
	data.mod_addr = mod_no;

	data.data = reg;
	data.contrl = 0;
	data.contrl |= ADDR_BIT;
	if(type == MOD_TYPE_NONE){
		/* DavidR (April 10, 2008): for module detection set 512 khz bit */
		data.contrl |= CPLD_USE_512KHZ_RECOVERY_CLOCK_BIT;
	}

	/* check spi not busy */
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		card->hw_iface.bus_read_4(card->hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{MYBREAK;}
	}
	if(0)DEBUG_EVENT("%s(): Line:%d: retry_counter: %d, dummy.start: %d\n",__FUNCTION__,  __LINE__, retry_counter, dummy.start);
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", card->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	

#if 0
	BRI_DBG("%s():Line:%d: bus_write_4(): data: 0x%08X\n", __FUNCTION__, __LINE__, data);
#endif
	/* setup the address */
	card->hw_iface.bus_write_4(card->hw, SPI_INTERFACE_REG,	*data_ptr);	

	/* printf("2. data: 0x%08X\n", *data_ptr); */
	/* wait for end of spi operation */
#if !FAST_SPI
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		card->hw_iface.bus_read_4(card->hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{MYBREAK;}
	}
	SPI_DELAY;
	if(0)DEBUG_EVENT("%s(): Line:%d: retry_counter: %d, dummy.start: %d\n",__FUNCTION__,  __LINE__, retry_counter, dummy.start);
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", card->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	
#endif

	/* setup data for read spi operation  */
	data.reset=0;
	data.start=1;
	data.reserv1=0;

	data.remora_addr = rm_no;
	data.mod_addr = mod_no;

	data.data = 0;
	data.contrl = 0;
	data.contrl |= READ_BIT;
	if(type == MOD_TYPE_NONE){
		/* DavidR (April 10, 2008): for module detection set 512 khz bit */
		data.contrl |= CPLD_USE_512KHZ_RECOVERY_CLOCK_BIT;
	}

	BRI_DBG("%s(Line: %i): (data: 0x%08X) reset: 0x%X, start: 0x%X, reserv1: 0x%X, remora_addr: 0x%X, mod_addr: 0x%X, data: 0x%X, contrl: 0x%X\n",
		__FUNCTION__, __LINE__, *((u32*)&data), data.reset, data.start, data.reserv1, data.remora_addr, data.mod_addr, data.data, data.contrl);

#if FAST_SPI
	SPI_DELAY;
#else
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		card->hw_iface.bus_read_4(card->hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{MYBREAK;}
	}
	SPI_DELAY;
	if(0)DEBUG_EVENT("%s(): Line:%d: retry_counter: %d, dummy.start: %d\n",__FUNCTION__,  __LINE__, retry_counter, dummy.start);
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", card->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	
#endif

#if 0
	BRI_DBG("%s():Line:%d: bus_write_4(): data: 0x%08X\n", __FUNCTION__, __LINE__, data);
#endif
	/*  start read spi operation */
	card->hw_iface.bus_write_4(card->hw, SPI_INTERFACE_REG, *data_ptr);	

#if FAST_SPI
	SPI_DELAY;
#else
	/* wait for end of spi operation */	
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		card->hw_iface.bus_read_4(card->hw, SPI_INTERFACE_REG, data_ptr);
		if(data.start == 1){SPI_DELAY;}else{MYBREAK;}
	}
#if 0
	BRI_DBG("%s():Line:%d: bus_read_4(): data: 0x%08X\n", __FUNCTION__, __LINE__, data);
#endif
	SPI_DELAY;
	if(0)DEBUG_EVENT("%s(): Line:%d: retry_counter: %d, dummy.start: %d\n",__FUNCTION__,  __LINE__, retry_counter, dummy.start);
	if(data.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", card->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	
#endif
	//printf("3. data: 0x%08X\n", *data_ptr);

	*value = data.data;

#if defined(WAN_DEBUG_REG)
	DEBUG_EVENT("%s():%s: mod_no:%d reg=0x%X (%d) value=0x%02X\n",
		__FUNCTION__,
		card->devname,
		mod_no,
		reg,
		reg,
		*value);
#endif
	return 0;
}

static int
write_bri_fe_byte(sdla_t *card, unsigned char mod_no, unsigned char addr, unsigned char value)
{
	bri_reg_t	data, dummy;
	u_int32_t	*data_ptr = (u_int32_t*)&data;
	u_int32_t	*dummy_ptr = (u_int32_t*)&dummy;
	u_int8_t	rm_no=0xFF;
	u_int32_t	retry_counter;

	WAN_ASSERT(card == NULL);

#if defined(WAN_DEBUG_REG)
	DEBUG_EVENT("%s():%s: mod_no:%d addr=0x%X (%d) value=0x%02X\n",
		__FUNCTION__,
		card->devname,
		mod_no,
		addr,
		addr,
		value);
#endif

	/*	Input mod_no is an even number between 0 and 22 (including).
		Calculate rm_no - should be between 0 and 3 (including). 
	*/
	rm_no = mod_no / (MAX_BRI_MODULES_PER_REMORA*2);
#if 0
	BRI_DBG("%s(input): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
	/* Translate mod_no to be between 0 and 2 (including) */
	mod_no = (mod_no / 2) % MAX_BRI_MODULES_PER_REMORA;
#if 0
	BRI_DBG("%s(updated): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
	if(rm_no > MAX_BRI_REMORAS - 1){
		DEBUG_EVENT("%s:%s(): Line:%d: invalid rm_no: %d!!(mod_no: %d)\n", 
			card->devname, __FUNCTION__, __LINE__, rm_no, mod_no);	
		return 0;
	}

	/* setup address offset for fe */
	data.reset = 0;
	data.start = 1;
	data.reserv1 = 0;

	data.remora_addr = rm_no;
	//data.mod_addr = 0;
	data.mod_addr = mod_no;

	data.data = addr;
	data.contrl = 0;
	data.contrl |= ADDR_BIT;
	
	/* printf("1. data: 0x%08X\n", *data_ptr); */

	/* check spi not busy */
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		card->hw_iface.bus_read_4(card->hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{MYBREAK;}
	}
	SPI_DELAY;
	if(0)DEBUG_EVENT("%s(): Line:%d: retry_counter: %d, dummy.start: %d\n",__FUNCTION__,  __LINE__, retry_counter, dummy.start);	
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", card->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	

	card->hw_iface.bus_write_4(	card->hw,	SPI_INTERFACE_REG,	*data_ptr);	

	/* start write spi operation */
	data.reset=0;
	data.start=1;
	data.reserv1=0;

	//data.remora_addr =0;
	//data.mod_addr =0;
	data.remora_addr = rm_no;
	data.mod_addr = mod_no;

	data.data = value;
	data.contrl = 0;
	
	/* printf("2. data: 0x%08X\n", *data_ptr); */
#if FAST_SPI
	SPI_DELAY;
#else
	/* wait for end of spi operation */
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		card->hw_iface.bus_read_4(card->hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{MYBREAK;}
	}
	SPI_DELAY;
	if(0)DEBUG_EVENT("%s(): Line:%d: retry_counter: %d, dummy.start: %d\n",__FUNCTION__,  __LINE__, retry_counter, dummy.start);
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", card->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	
#endif
	/* write the actual data */
	card->hw_iface.bus_write_4(	card->hw,	SPI_INTERFACE_REG,	*data_ptr);	
/*
	card->hw_iface.bus_read_4(card->hw, 0x40, dummy_ptr);
	card->hw_iface.bus_read_4(card->hw, SPI_INTERFACE_REG, dummy_ptr);
	if((*data_ptr & 0xFFFF) != (*dummy_ptr & 0xFFFF)){
		DEBUG_ERROR("%s:%s(): Line:%d: Error: *data_ptr: 0x%02X, *dummy_ptr: 0x%02X\n",
			card->devname, __FUNCTION__, __LINE__, *data_ptr, *dummy_ptr);			
	}
*/
	return 0;
}

#endif

/********************************************************************************/
/* D channel Transmit */
int aft_bri_dchan_transmit(sdla_t *card, void *chan_ptr, void *src_data_buffer, unsigned int buffer_len)
{
	int		err = 0;
	/*private_area_t	*chan = (private_area_t*)chan_ptr;*/

	BRI_FUNC();

	if (card->wandev.fe_iface.isdn_bri_dchan_tx){
		err = card->wandev.fe_iface.isdn_bri_dchan_tx(
					&card->fe,
					src_data_buffer, 
					buffer_len);
	}else{
		DEBUG_WARNING("%s():%s: Warning: uninitialized isdn_bri_dchan_tx() pointer.\n",
			__FUNCTION__, card->devname);
	}

	return err;
}

/********************************************************************************/
/* D channel Receive */
/*
int aft_bri_dchan_receive( sdla_t *card, void *chan_ptr, void *dst_data_buffer, unsigned int buffer_len)
{
	u32				reg;
	int				err = 0;
	private_area_t	*chan = (private_area_t*)chan_ptr;

	BRI_FUNC();

	if (card->wandev.fe_iface.isdn_bri_dchan_rx){
		err = card->wandev.fe_iface.isdn_bri_dchan_rx(
					&card->fe, 
					dst_data_buffer, 
					buffer_len);
	}else{
		DEBUG_WARNING("%s():%s: Warning: uninitialized isdn_bri_dchan_rx() pointer.\n",
			__FUNCTION__, card->devname);
	}

	return err;
}
*/
#endif/* #if defined(CONFIG_PRODUCT_WANPIPE_AFT_BRI) */
