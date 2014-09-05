/*****************************************************************************
* aft_a104.c 
* 		
* 		WANPIPE(tm) AFT A104 Hardware Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2007 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Sep 25, 2005  Nenad Corbic	Initial Version
* Mar 28, 2007  David Rokhvarg	<davidr@sangoma.com>
*				Added support for 56k AFT card.
*
*****************************************************************************/

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# include <wanpipe_includes.h>
# include <wanpipe.h>
# include <wanpipe_abstr.h>
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>
# include <wanpipe_iface.h>
# include <sdla_serial.h>
#elif defined(__WINDOWS__)

# include <wanpipe_includes.h>
# include <wanpipe.h>
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>
# include <wanpipe_iface.h>
# include <sdla_serial.h>

extern
void 
sdla_te_timer(
	IN PKDPC,
	void*,
	void*,
	void*
	);

#else
# include <linux/wanpipe_includes.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe.h>
# include <linux/wanproc.h>
# include <linux/wanpipe_abstr.h>
# include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
# include <linux/if_wanpipe.h>
# include <linux/sdlapci.h>
# include <sdla_serial.h>
//# include <linux/sdla_ec.h>
# include <linux/aft_core.h>
# include <linux/wanpipe_iface.h>
# include <linux/wanpipe_tdm_api.h>
# include <linux/sdla_serial.h>

#endif

#define INIT_FE_ONLY 0

#if 1
#define AFT_FUNC_DEBUG()
#else
#define AFT_FUNC_DEBUG()  DEBUG_EVENT("%s:%d\n",__FUNCTION__,__LINE__)
#endif

/*==============================================
 * PRIVATE FUNCITONS
 *
 */
#if defined(CONFIG_WANPIPE_HWEC)
static int aft_hwec_reset(void *pcard, int reset);
static int aft_hwec_enable(void *pcard, int enable, int fe_chan);

static int aft_hwec_reset_b601(void *pcard, int reset);
static int aft_hwec_enable_b601(void *pcard, int enable, int fe_chan);
#endif

int __a104_write_fe (void *pcard, ...);
int __a56k_write_fe (void *pcard, ...);


static int aft_map_fifo_baddr_and_size(sdla_t *card, unsigned char fifo_size, unsigned char *addr);

static char fifo_size_vector[] =  {1, 2, 4, 8, 16, 32};
static char fifo_code_vector[] =  {0, 1, 3, 7,0xF,0x1F};

static int request_fifo_baddr_and_size(sdla_t *card, private_area_t *chan)
{
	unsigned char req_fifo_size,fifo_size;
	int i;
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
#else

	/* Calculate the optimal fifo size based
         * on the number of time slots requested */

	if (IS_SERIAL_CARD(card)) {

		req_fifo_size=32;

	} else if (IS_T1_CARD(card)){	

		if (chan->num_of_time_slots == NUM_OF_T1_CHANNELS){
			req_fifo_size=32;
		}else if (chan->num_of_time_slots == 1){
			req_fifo_size=1;
		}else if (chan->num_of_time_slots == 2 || chan->num_of_time_slots == 3){
			req_fifo_size=2;
		}else if (chan->num_of_time_slots >= 4 && chan->num_of_time_slots<= 7){
			req_fifo_size=4;
		}else if (chan->num_of_time_slots >= 8 && chan->num_of_time_slots<= 15){
			req_fifo_size=8;
		}else if (chan->num_of_time_slots >= 16 && chan->num_of_time_slots<= 23){
			req_fifo_size=16;
		}else{
			DEBUG_EVENT("%s:%s: Invalid number of timeslots %d\n",
					card->devname,chan->if_name,chan->num_of_time_slots);
			return -EINVAL;		
		}
	}else{
		if (chan->num_of_time_slots == (NUM_OF_E1_CHANNELS-1)){
			req_fifo_size=32;
                }else if (chan->num_of_time_slots == 1){
			req_fifo_size=1;
                }else if (chan->num_of_time_slots == 2 || chan->num_of_time_slots == 3){
			req_fifo_size=2;
                }else if (chan->num_of_time_slots >= 4 && chan->num_of_time_slots <= 7){
			req_fifo_size=4;
                }else if (chan->num_of_time_slots >= 8 && chan->num_of_time_slots <= 15){
			req_fifo_size=8;
                }else if (chan->num_of_time_slots >= 16 && chan->num_of_time_slots <= 31){
			req_fifo_size=16;
		}else if (WAN_FE_FRAME(&card->fe) == WAN_FR_UNFRAMED){
			req_fifo_size=16;
                }else{
                        DEBUG_EVENT("%s:%s: Invalid number of timeslots %d\n",
                                        card->devname,chan->if_name,chan->num_of_time_slots);
                        return -EINVAL;
                }
	}

	DEBUG_TEST("%s:%s: Optimal Fifo Size =%d  Timeslots=%d \n",
		card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots);

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
#endif
	return 0;
}


static int aft_map_fifo_baddr_and_size(sdla_t *card, unsigned char fifo_size, unsigned char *addr)
{
	u32 reg=0;
	u8 i;
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
	return 0;
#else

	for (i=0;i<fifo_size;i++){
		wan_set_bit(i,&reg);
	} 

	DEBUG_TEST("%s: Trying to MAP 0x%X  to 0x%lX\n",
                        card->devname,reg,card->u.aft.fifo_addr_map);

	for (i=0;i<32;i+=fifo_size){
		if (card->u.aft.fifo_addr_map & (reg<<i)){
			continue;
		}
		card->u.aft.fifo_addr_map |= reg<<i;
		*addr=i;

		DEBUG_TEST("%s: Card fifo Map 0x%lX Addr =%d\n",
	                card->devname,card->u.aft.fifo_addr_map,i);

		return fifo_size;
	}

	if (fifo_size == 1){
		return 0; 
	}

	fifo_size = fifo_size >> 1;
	
	return aft_map_fifo_baddr_and_size(card,fifo_size,addr);
#endif
}


static int aft_free_fifo_baddr_and_size (sdla_t *card, private_area_t *chan)
{
	u32 reg=0;
	int i;

	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
#else

	for (i=0;i<chan->fifo_size;i++){
                wan_set_bit(i,&reg);
        }
	
	DEBUG_TEST("%s: Unmapping 0x%X from 0x%lX\n",
		card->devname,reg<<chan->fifo_base_addr, card->u.aft.fifo_addr_map);

	card->u.aft.fifo_addr_map &= ~(reg<<chan->fifo_base_addr);

	DEBUG_TEST("%s: New Map is 0x%lX\n",
                card->devname, card->u.aft.fifo_addr_map);


	chan->fifo_size=0;
	chan->fifo_base_addr=0;
#endif
	return 0;
}


static int aft_request_logical_channel_num (sdla_t *card, private_area_t *chan, int force_lch)
{
	signed char logic_ch=-1;
	int err;
	int if_cnt=wan_atomic_read(&card->wandev.if_cnt);
	int if_offset=2;
	long i;

	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
#else
	if (IS_E1_CARD(card) && !(WAN_FE_FRAME(&card->fe) == WAN_FR_UNFRAMED)){
		if_offset=3;
	}

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
					card->devname, chan->if_name, 
				        IS_E1_CARD(card)?i:i+1);

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

	if (force_lch) {
		if (!wan_test_and_set_bit(chan->first_time_slot,&card->u.aft.logic_ch_map)){
			logic_ch=(char)chan->first_time_slot;
		} else {
			DEBUG_ERROR("%s: Error, request logical ch=%d map busy (map 0x%08lX)\n",
				card->devname,chan->first_time_slot,card->u.aft.logic_ch_map);
	  	    return -1;
	    }
	} else {

		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			
			if (card->u.aft.security_id == 0){
				/* Unchannelized card must config
				 * its hdlc logic ch on FIRST logic
				 * ch number */
				 
				if (chan->channelized_cfg) {
					if (card->tdmv_conf.dchan){
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


	DEBUG_TEST("Binding logic ch %d  Ptr=%p\n",logic_ch,chan);
#endif
	return logic_ch;
}



static int aft_test_hdlc(sdla_t *card)
{
	int i;
	int err=-EINVAL;
	u32 reg;
	
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
#else
	for (i=0;i<10;i++){
		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG), &reg);

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
#endif
	return err;
}


/*==============================================
 * PUBLIC FUNCITONS
 *
 */

int a104_test_sync(sdla_t *card, int tx_only)
{
	volatile int i,err=1;
	u32 reg;
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
#else
	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
		
	if (wan_test_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg)){
		DEBUG_WARNING("%s: Warning: T1/E1 Reset Enabled %d! reg=0x%08X \n",
				card->devname, card->wandev.comm_port+1,reg);
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

	DEBUG_TEST("%s: DELAY INDEX = %i\n",
			card->devname,i);
#endif
	return err;
}

int a104_led_ctrl(sdla_t *card, int color, int led_pos, int on)
{
	u32 reg;
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
#else
	if (IS_B601_CARD(card)) {
		card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
		aft_set_led_b601(color, led_pos, on, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);
		return 0;
	}

	if (IS_SERIAL_CARD(card)) {
		return 0;
	}	

	if (card->adptr_subtype == AFT_SUBTYPE_SHARK){

		/* INSERT LED CODE */
		switch (color){
		
		case WAN_AFT_RED:
			if (on){
				wan_clear_bit(0,&card->u.aft.led_ctrl);
			}else{
				wan_set_bit(0,&card->u.aft.led_ctrl);
			}	
			break;
		
		case WAN_AFT_GREEN:
			if (on){
				wan_clear_bit(1,&card->u.aft.led_ctrl);
			}else{
				wan_set_bit(1,&card->u.aft.led_ctrl);
			}	
			break;			
		}

		if(IS_56K_CARD(card)){
			aft_56k_write_cpld(card,card->wandev.comm_port + 0x08,card->u.aft.led_ctrl);
		}else{
			if (card->adptr_type == A108_ADPTR_8TE1) {
				/* On A108 Card the leds are flipped. So the top offset is for leds 1 to 4 
				   and vice versa */
				if (card->wandev.comm_port < 4) {
					aft_te1_write_cpld(card,(4+card->wandev.comm_port) + 0x08,card->u.aft.led_ctrl);
				} else {
					aft_te1_write_cpld(card,(card->wandev.comm_port-4) + 0x08,card->u.aft.led_ctrl);
				}
			} else {
				aft_te1_write_cpld(card,card->wandev.comm_port + 0x08,card->u.aft.led_ctrl);
			}
		}

	}else{
		card->hw_iface.bus_read_4(card->hw,
			AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
		aft_set_led(color, led_pos, on, &reg);
		card->hw_iface.bus_write_4(card->hw,
			AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);	
	}
#endif
	return 0;
}

void a104_set_octasic_clk_src(void *pcard)
{
	u32     reg;
	u16     max_ec_chans;
	sdla_t  *card=(sdla_t*)pcard;
	card->hw_iface.getcfg(card->hw, SDLA_HWEC_NO, &max_ec_chans);
	
	if (max_ec_chans) {
		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),&reg);
		aft_chipcfg_set_oct_clk_src(&reg,card->wandev.comm_port);
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);
	}
	return;
}

int a104_set_octasic_clk_src_b601(sdla_t *card)
{
	wan_smp_flag_t smp_flags, flags;
	u32 reg_1090;
	u8 ext_clk_mux_bits = 0;
	u8 pll_clk_mux_bits = 0;
	u16 max_ec_chans;
	
	max_ec_chans = 0;
	card->hw_iface.getcfg(card->hw, SDLA_HWEC_NO, &max_ec_chans);
	
	card->hw_iface.hw_lock(card->hw,&smp_flags);
	wan_spin_lock_irq(&card->wandev.lock,&flags);
	card->hw_iface.bus_read_4(card->hw, AFT_CLKCFG_A600_REG, &reg_1090);
	
	/* Configure EC Clock source Mux */
	reg_1090 &= ~(AFT_CLKCFG_B601_EC_SRC_MUX_MASK<<AFT_CLKCFG_B601_EC_SRC_MUX_SHIFT);
	if (max_ec_chans) {
		reg_1090 |= AFT_CLKCFG_B601_EC_SRC_MUX_TRISTATE << AFT_CLKCFG_B601_EC_SRC_MUX_SHIFT;
	}
	
	/* Define Ext Clock Val */
	reg_1090 &= ~(AFT_CLKCFG_B601_EXT_CLK_VAL_MASK<<AFT_CLKCFG_B601_EXT_CLK_VAL_SHIFT);
	if (WAN_FE_NETWORK_SYNC(&card->fe) == WAN_CLK_IN_2000HZ) {
		reg_1090 |= 1 << AFT_CLKCFG_B601_EXT_CLK_VAL_SHIFT;
	}
	/* Configure External clock out Mux */
	reg_1090 &= ~(AFT_CLKCFG_B601_EXT_CLK_MUX_MASK<<AFT_CLKCFG_B601_EXT_CLK_MUX_SHIFT);
	reg_1090 &= ~(AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_MASK<<AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_SHIFT);

	if (WAN_TE1_CLK(&card->fe) == WAN_MASTER_CLK) {
		if (IS_T1_CARD(card)) {
			switch (WAN_FE_NETWORK_SYNC(&card->fe)) {
				case WANOPT_NO:
					ext_clk_mux_bits = AFT_CLKCFG_B601_EXT_CLK_MUX_OSC_1500HZ;
					pll_clk_mux_bits = AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_OSC_1500HZ;
					break;
				case WAN_CLK_IN_1500HZ:
					ext_clk_mux_bits = AFT_CLKCFG_B601_EXT_CLK_MUX_H100_CLK;
					pll_clk_mux_bits = AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_EXT_1500HZ;
					break;
				case WANOPT_YES: /* Invalid */
				case WAN_CLK_IN_8000HZ: /* Invalid */
				case WAN_CLK_IN_2000HZ: /* Invalid */
				default:
					DEBUG_EVENT("%s: Error:Invalid T1 Network sync option\n", card->devname);
					return -EINVAL;
			}
		} else {
			switch (WAN_FE_NETWORK_SYNC(&card->fe)) {
				case WANOPT_NO:
					ext_clk_mux_bits = AFT_CLKCFG_B601_EXT_CLK_MUX_H100_CLK;
					pll_clk_mux_bits = AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_OSC_2000HZ;
					break;
				case WANOPT_YES:
				case WAN_CLK_IN_8000HZ:
				case WAN_CLK_IN_2000HZ: /* bit 4 specifies 2Mhz or 8 Mhz source */
					ext_clk_mux_bits = AFT_CLKCFG_B601_EXT_CLK_MUX_H100_CLK;
					pll_clk_mux_bits = AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_EXT_8000HZ;
					break;
				case WAN_CLK_IN_1500HZ: /* Invalid */
				default:
					DEBUG_EVENT("%s: Error:Invalid E1 Network sync option\n", card->devname);
					return -EINVAL;
			}
		}
	}
	reg_1090 |= pll_clk_mux_bits << AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_SHIFT;
	reg_1090 |= ext_clk_mux_bits << AFT_CLKCFG_B601_EXT_CLK_MUX_SHIFT;
	
	card->hw_iface.bus_write_4(card->hw, AFT_CLKCFG_A600_REG, reg_1090);
	wan_spin_unlock_irq(&card->wandev.lock,&flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);
	return 0;
}


int a104_global_chip_config(sdla_t *card)
{
	u32 reg, reg_line_cfg;
	int err=0;
	int used_cnt;

	reg=0;
	/*============ GLOBAL CHIP CONFIGURATION ===============*/
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
	err = -EINVAL;
	if (card->wandev.fe_iface.global_config){
		err=card->wandev.fe_iface.global_config(&card->fe);
	}
	if (err){
		DEBUG_ERROR("%s(): %d: Error: global_config() ptr is NULL\n",
			__FUNCTION__, __LINE__);
		return err;
	}

#else
	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);
	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),&reg);

	if (IS_B601_CARD(card)) {
		/* This B600 card should not have a daughter card */
		if(!aft_chipcfg_get_b601_security(reg)) {
			DEBUG_EVENT("%s: Error: Chip security conflict, cannot load T1/E1\n", card->devname);
			return -EINVAL;
		}
		if (used_cnt > 1) {
	     	DEBUG_EVENT("%s: Error: T1/E1 port has to be started first on B601\n", card->devname);
			return -EINVAL;
		}
	}

	/*============ GLOBAL CHIP CONFIGURATION ===============*/
	if (used_cnt == 1) {
		/* Enable the chip/hdlc reset condition */
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
	
		DEBUG_CFG("--- AFT Chip Reset. -- \n");
	
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);
		
		WP_DELAY(900);
	
		/* Disable the chip/hdlc reset condition */
		wan_clear_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_clear_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
	
		wan_clear_bit(AFT_CHIPCFG_FE_INTR_CFG_BIT,&reg);
	
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);
		
		WP_DELAY(900);
	
		if (!IS_B601_CARD(card)) {
			/* On B601, these bits are specified in AFT_LINE_CFG_REG */
			if (IS_T1_CARD(card)){
				wan_clear_bit(AFT_CHIPCFG_TE1_CFG_BIT,&reg);
			}else if (IS_E1_CARD(card)){
				wan_set_bit(AFT_CHIPCFG_TE1_CFG_BIT,&reg);
			}else if (IS_56K_CARD(card)){
				wan_set_bit(AFT_CHIPCFG_56K_CFG_BIT,&reg);
			}else if (IS_SERIAL_CARD(card)) {
				/* No bit to set nere */
			}else{
				DEBUG_ERROR("%s: Error: Xilinx doesn't support non T1/E1 interface!\n",
						card->devname);
				return -EINVAL;
			}
		}
	}
	
	/* Enable FRONT END Interrupt */
#ifdef WAN_NO_INTR
	wan_clear_bit(AFT_CHIPCFG_FE_INTR_CFG_BIT,&reg);
	card->fe_no_intr=1;
#else
	wan_set_bit(AFT_CHIPCFG_FE_INTR_CFG_BIT,&reg);
#endif
	DEBUG_CFG("--- Chip enable/config. -- \n");

	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);
		
	WP_DELAY(500);

	if (IS_B601_CARD(card)) {
		wan_smp_flag_t smp_flags;
	
		/* Maxim external reset off */
		reg_line_cfg = 0;
		card->hw_iface.hw_lock(card->hw,&smp_flags);
	
		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg_line_cfg);
	
		wan_clear_bit(AFT_LCFG_B601_FE_CHIP_RESET_BIT, &reg_line_cfg);
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), reg_line_cfg);
		WP_DELAY(1000);
		wan_set_bit(AFT_LCFG_B601_FE_CHIP_RESET_BIT, &reg_line_cfg);
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), reg_line_cfg);
		WP_DELAY(1000);
		WP_DELAY(1000);
		WP_DELAY(1000);
	
		card->hw_iface.hw_unlock(card->hw,&smp_flags);

	} else  if (card->u.aft.firm_id == AFT_DS_FE_CORE_ID){

		/* A104/A108 with Dallas FE */
		wan_smp_flag_t smp_flags,flags;


		card->hw_iface.hw_lock(card->hw,&smp_flags);
		wan_spin_lock_irq(&card->wandev.lock,&flags);
		aft_te1_write_cpld(card,0x00,0x00);
		wan_spin_unlock_irq(&card->wandev.lock,&flags);
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
	
		WP_DELAY(500);

		card->hw_iface.hw_lock(card->hw,&smp_flags);
		wan_spin_lock_irq(&card->wandev.lock,&flags);
		aft_te1_write_cpld(card,0x00,0x06);
		wan_spin_unlock_irq(&card->wandev.lock,&flags);
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
		
		WP_DELAY(500);
		
	}else if (IS_SERIAL_CARD(card)) {

		/* There is no CPLD reset for Serial card */

	}else if (card->adptr_subtype == AFT_SUBTYPE_SHARK){

		wan_smp_flag_t smp_flags,flags;
		card->hw_iface.hw_lock(card->hw,&smp_flags);
		wan_spin_lock_irq(&card->wandev.lock,&flags);
		if (IS_T1_CARD(card)){
			aft_te1_write_cpld(card,0x00,0x00);
		}else if (IS_E1_CARD(card)){
			aft_te1_write_cpld(card,0x00,0x02);
		}else if (IS_56K_CARD(card)){
			aft_56k_write_cpld(card,0x00,0x02);/* DR: Framer 'reset off' */
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
		DEBUG_ERROR("%s(): %d: Error: global_config() ptr is NULL\n",
			__FUNCTION__, __LINE__);
		return err;
	}

	if (IS_B601_CARD(card)) {
		if (used_cnt == 1) {
			if(a104_set_octasic_clk_src_b601(card)) {
				DEBUG_EVENT("%s: Failed to configure B601 Octasic clock source\n",
												card->devname);
				return -EINVAL;
			}
		}
	} else {
		if (card->adptr_subtype == AFT_SUBTYPE_SHARK) {
			a104_set_octasic_clk_src(card);
		}
	}

	/* Always disable in the beginning to set timer in correct state */
	aft_free_running_timer_disable(card);

#endif/*INIT_FE_ONLY*/
	return 0;
}

int a104_global_chip_unconfig(sdla_t *card)
{
	u32 reg=0;
	int used_cnt;

	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
	/* Global T1/E1 unconfig */
	if (card->wandev.fe_iface.global_unconfig){
		card->wandev.fe_iface.global_unconfig(&card->fe);
	}
#else
	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);

	/* Global T1/E1 unconfig */
	if (card->wandev.fe_iface.global_unconfig){
		card->wandev.fe_iface.global_unconfig(&card->fe);
	}
	/* Disable front end on purpose since this is last card */

	/* Set Octasic/TE1 clocking to reset (A104)
	** Set Octasic/Framer to reset (A108) */
	if (!IS_B601_CARD(card)) {
		aft_te1_write_cpld(card,0x00,0x00);
	}

	if (used_cnt == 1) {
		/* Disable the chip/hdlc reset condition */
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
	}
	wan_clear_bit(AFT_CHIPCFG_FE_INTR_CFG_BIT,&reg);

	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);
#endif
	return 0;
}

static int aft_ds_set_clock_ref_b601(sdla_t *card, u32 *reg, u32 unused)
{
	u8 clk_route_bits;
	AFT_FUNC_DEBUG();
	
	clk_route_bits = 0;
	
	*reg &= ~(AFT_LCFG_B601_CLK_ROUTE_MASK << AFT_LCFG_B601_CLK_ROUTE_SHIFT);
	if (WAN_TE1_CLK(&card->fe) == WAN_NORMAL_CLK) {
		return 0;
	}
	
	if (IS_T1_CARD(card)) {
		switch (WAN_FE_NETWORK_SYNC(&card->fe)) {
			case WANOPT_NO:
				clk_route_bits = AFT_LCFG_B601_CLK_SRC_OSC_1500HZ;
				break;
			case WAN_CLK_IN_1500HZ:
				clk_route_bits = AFT_LCFG_B601_CLK_SRC_EXT_1500HZ;
				break;
			case WANOPT_YES: /* Invalid */
			case WAN_CLK_IN_8000HZ: /* Invalid */
			case WAN_CLK_IN_2000HZ: /* Invalid */
			default:
				DEBUG_EVENT("%s: Error:Invalid T1 Network sync option\n", card->devname);
				return -EINVAL;
		}
	} else {
		switch (WAN_FE_NETWORK_SYNC(&card->fe)) {
			case WANOPT_NO:
				clk_route_bits = AFT_LCFG_B601_CLK_SRC_OSC_2000HZ;
				break;
			case WANOPT_YES:
			case WAN_CLK_IN_8000HZ:
			case WAN_CLK_IN_2000HZ:
				clk_route_bits = AFT_LCFG_B601_CLK_SRC_EXT_2000HZ;
				break;
			case WAN_CLK_IN_1500HZ: /* Invalid */
			default:
				DEBUG_EVENT("%s: Error:Invalid E1 Network sync option\n", card->devname);
				return -EINVAL;
		}
	}
	
	*reg |= clk_route_bits << AFT_LCFG_B601_CLK_ROUTE_SHIFT;
	return 0;
}


static int aft_ds_set_clock_ref(sdla_t *card, u32 *reg, u32 master_port)
{
	u32 master_cfg;
	
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY
#else

	if (IS_T1_CARD(card)) {
		master_cfg=0x09;
	} else {
		master_cfg=0x08;
	}  

	if (WAN_TE1_CLK(&card->fe) == WAN_MASTER_CLK) {  
	        wan_set_bit(AFT_LCFG_A108_FE_CLOCK_MODE_BIT,reg);

		if (WAN_FE_LINENO(&card->fe) >= 4) {
        	 	a108m_write_cpld(card,
					WAN_FE_LINENO(&card->fe)-4,
					(u8)master_cfg);      	
		}  
	       	/* July 5, 2006 
	      	 ** Modification for Master mode 
	       	 ** (next line execute for all channels) 
	       	 */
		/* We must configure the xilinx space for
		 * each port, only for ports greater than 3
		 * we must also configure the CPLD */
               	aft_lcfg_a108_fe_clk_source(reg,master_cfg);       

	} else {
	        wan_clear_bit(AFT_LCFG_A108_FE_CLOCK_MODE_BIT,reg);

		if (WAN_FE_LINENO(&card->fe) >= 4) {
        	 	a108m_write_cpld(card,
					WAN_FE_LINENO(&card->fe)-4,
					(u8)master_port);      	
		} 

		/* We must configure the xilinx space for
		 * each port, only for ports greater than 3
		 * we must also configure the CPLD */
           		
		aft_lcfg_a108_fe_clk_source(reg,master_port);
	}
#endif
	return 0;
}


static void aft_set_hwec_clock_src(sdla_t *card)
{
	wan_smp_flag_t smp_flags, flags;

	if (card->adptr_subtype == AFT_SUBTYPE_SHARK && card->hwec_conf.clk_src){
		
		u32 cfg_reg;
		card->hwec_conf.clk_src = WAN_FE_LINENO(&card->fe);
	
	       	DEBUG_EVENT("%s: Global EC Clock Port = %d\n",
	                card->devname,
	                 card->hwec_conf.clk_src+1);


		card->hw_iface.hw_lock(card->hw,&smp_flags);
		wan_spin_lock_irq(&card->wandev.lock,&flags);

		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG), &cfg_reg);
	     	aft_chipcfg_set_oct_clk_src(&cfg_reg,card->hwec_conf.clk_src);
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG), cfg_reg);

		wan_spin_unlock_irq(&card->wandev.lock,&flags);
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
	}
}

int a104_chip_config(sdla_t *card, wandev_conf_t *conf)
{
	u32 reg=0, ctrl_ram_reg=0;
	int i,err=0, max_channels;

	wan_smp_flag_t smp_flags, flags;
	AFT_FUNC_DEBUG();
	
	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
	if (!wan_test_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg)){
	
		DEBUG_ERROR("%s: Error: Physical Port %d is busy!\n",
				card->devname, card->wandev.comm_port+1);
		return -EBUSY;
	}

	
	/* On A108 Cards the T1/E1 will be configured per PORT  
	 * not per CARD */
	if (card->u.aft.firm_id == AFT_DS_FE_CORE_ID) {

		if (IS_T1_CARD(card)) {
			wan_clear_bit(AFT_LCFG_A108_FE_TE1_MODE_BIT,&reg);
		} else {
			wan_set_bit(AFT_LCFG_A108_FE_TE1_MODE_BIT,&reg);
		}

		card->hw_iface.hw_lock(card->hw,&smp_flags);
		wan_spin_lock_irq(&card->wandev.lock,&flags);

		if (IS_B601_CARD(card)) {
			aft_ds_set_clock_ref_b601(card,&reg,WAN_FE_LINENO(&card->fe));
		} else {
			aft_ds_set_clock_ref(card,&reg,WAN_FE_LINENO(&card->fe));
		}

		wan_spin_unlock_irq(&card->wandev.lock,&flags);
		card->hw_iface.hw_unlock(card->hw,&smp_flags);

		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), reg);
	}

	card->hw_iface.hw_lock(card->hw,&smp_flags);

	if(card->adptr_type == AFT_ADPTR_56K){
		
		aft_56k_write_cpld(card, 0x00,0x00);//reset_on_LXT441PE(card);
		WP_DELAY(1000);

		aft_56k_write_cpld(card, 0x00, 0x03);//reset_off_LXT441PE(card);
		WP_DELAY(1000);
	}

	err = -EINVAL;
	if (card->wandev.fe_iface.config){
		err=card->wandev.fe_iface.config(&card->fe);
	}
		
	a104_led_ctrl(card, WAN_AFT_RED, 0,WAN_AFT_ON);
	a104_led_ctrl(card, WAN_AFT_GREEN, 0, WAN_AFT_OFF);

	card->hw_iface.hw_unlock(card->hw,&smp_flags);

	if (err){
		DEBUG_EVENT("%s: Failed %s configuration!\n",
                                	card->devname,
                                	(IS_T1_CARD(card))?"T1":"E1");
		return -EINVAL;
   	}
	/* Run rest of initialization not from lock */
	if (card->wandev.fe_iface.post_init){
		err=card->wandev.fe_iface.post_init(&card->fe);
	}
	
	DEBUG_EVENT("%s: Front end successful\n",
			card->devname);

	if (card->adptr_type == A104_ADPTR_4TE1 ||
	    card->u.aft.firm_id == AFT_DS_FE_CORE_ID) {

		if (WAN_TE1_CLK(&card->fe) == WAN_MASTER_CLK &&
		    WAN_TE1_REFCLK(&card->fe) > 0){

			int mclk_ver=AFT_TDMV_FRM_CLK_SYNC_VER;
			int max_port=4;
			
			if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
				mclk_ver=AFT_TDMV_SHARK_FRM_CLK_SYNC_VER;
			}

			if (card->u.aft.firm_id == AFT_DS_FE_CORE_ID){
				mclk_ver=AFT_TDMV_SHARK_A108_FRM_CLK_SYNC_VER;
				switch(card->adptr_type){
				case A108_ADPTR_8TE1:
					max_port=8;
					break;
				case A104_ADPTR_4TE1:
					max_port=4;
					break;
				case A101_ADPTR_2TE1:
					max_port=2;
					break;
				case AFT_ADPTR_B601:
				case A101_ADPTR_1TE1:
					max_port=1;
					break;
				}
			}

			if (card->u.aft.firm_ver < mclk_ver){
				DEBUG_ERROR("%s: Error: AFT FE Clock Sync Depends on Firmware Ver: %X (Cur=%X)\n",
						card->devname,mclk_ver,card->u.aft.firm_ver);
				DEBUG_EVENT("%s:        Please upgrade your AFT Firmware to Ver=%X or greater!\n",
						card->devname,mclk_ver);
				return -EINVAL;
			}
			
			if (WAN_TE1_REFCLK(&card->fe) == card->wandev.comm_port+1){
				DEBUG_ERROR("%s: Error: Invalid FE Clock Source Line=%i (same as configured line=%i)\n",
						card->devname,WAN_TE1_REFCLK(&card->fe),
						card->wandev.comm_port+1);
				return -EINVAL;
			}
			
			if (WAN_TE1_REFCLK(&card->fe) > max_port){
				DEBUG_ERROR("%s: Error: Invalid FE Clock Source Line=%i\n",
						card->devname,WAN_TE1_REFCLK(&card->fe));
				return -EINVAL;

			}

			/* FIXME: Check that REFCLOCK Port is configured for T1 or E1
			 *        as the current port is!!! */

			card->hw_iface.bus_read_4(card->hw,
						  AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
			
			if (card->u.aft.firm_id == AFT_DS_FE_CORE_ID){
				card->hw_iface.hw_lock(card->hw,&smp_flags);
				wan_spin_lock_irq(&card->wandev.lock,&flags);
				
				if (IS_B601_CARD(card)) {
					aft_ds_set_clock_ref(card, &reg , 0);
				} else {
					/* For A108 the refclock indicates NORMAL mode.
					* For backward compatilbity we make the user
					* indicate a MASTER mode */
	
					WAN_TE1_CLK(&card->fe) = WAN_NORMAL_CLK;
					aft_ds_set_clock_ref(card,&reg,WAN_TE1_REFCLK(&card->fe)-1);
				}

				wan_spin_unlock_irq(&card->wandev.lock,&flags);
				card->hw_iface.hw_unlock(card->hw,&smp_flags); 
				
			} else {

				aft_lcfg_fe_clk_source(&reg,WAN_TE1_REFCLK(&card->fe)-1);	
				wan_set_bit(AFT_LCFG_FE_CLK_ROUTE_BIT,&reg);
				
			}
			
			card->hw_iface.bus_write_4(card->hw,
						   AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);

			DEBUG_EVENT("%s: Configuring FE Line=%d Clock Source: Line=%d\n",
					card->devname,
					card->wandev.comm_port+1,
					WAN_TE1_REFCLK(&card->fe));
		}
		
	} 

	/*============ LINE/PORT CONFIG REGISTER ===============*/

	card->hw_iface.bus_read_4(card->hw,
				  AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
	wan_set_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);

	WP_DELAY(10);

	wan_clear_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);

	if (IS_E1_CARD(card) && (WAN_FE_FRAME(&card->fe) == WAN_FR_UNFRAMED)){
		wan_set_bit(AFT_LCFG_CLR_CHNL_EN,&reg);
	}
	
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);

	WP_DELAY(10);

	err=a104_test_sync(card,1);

	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
	if (err != 0){
		DEBUG_ERROR("%s: Error: Front End Interface Not Ready (0x%08X)!\n",
					card->devname,reg);
		return err;
	} else{
		DEBUG_EVENT("%s: Front End Interface Ready 0x%08X\n",
                                        card->devname,reg);
	}


	card->wandev.ec_dev = NULL;
	card->wandev.hwec_reset = NULL;
	card->wandev.hwec_enable = NULL;

	/* Enable Octasic Chip */
	if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
		u16		max_ec_chans;
		u32		cfg_reg;

		card->hw_iface.getcfg(card->hw, SDLA_HWEC_NO, &max_ec_chans);

		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG), &cfg_reg);
		if (max_ec_chans > aft_chipcfg_get_ec_channels(cfg_reg)){
					DEBUG_ERROR("%s: Critical Error: Exceeded Maximum Available Echo Channels!\n",
					card->devname);
					DEBUG_ERROR("%s: Critical Error: Max Allowed=%d Configured=%d\n",
					card->devname,
					aft_chipcfg_get_ec_channels(cfg_reg),
					max_ec_chans);  
       		return -EINVAL;
		}
	
		if (max_ec_chans){	
#if defined(CONFIG_WANPIPE_HWEC)
			if (IS_B601_CARD(card)) {
				card->wandev.hwec_reset = aft_hwec_reset_b601;
				card->wandev.hwec_enable = aft_hwec_enable_b601;
			} else {
				card->wandev.hwec_reset = aft_hwec_reset;
				card->wandev.hwec_enable = aft_hwec_enable;
			}

			card->wandev.ec_dev = wanpipe_ec_register(
						card,
						0,
						WAN_FE_MAX_CHANNELS(&card->fe),
						max_ec_chans,
						(void*)&conf->oct_conf);
			if (!card->wandev.ec_dev) {
				DEBUG_EVENT(
				"%s: Failed to register device in HW Echo Canceller module!\n",
							card->devname);
				return -EINVAL;
			}

			if (!IS_B601_CARD(card)) {
				aft_set_hwec_clock_src(card);
				card->wandev.ec_clock_ctrl=a104_set_octasic_clk_src;
			}

#else
			
			DEBUG_EVENT("%s: Wanpipe HW Echo Canceller module is not compiled!\n",
						card->devname);
#endif
		}else{
			DEBUG_EVENT(
			"%s: WARNING: No Echo Canceller channels are available!\n",
						card->devname);
		}
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

	AFT_FUNC_DEBUG();

	reg=0;
	if(IS_56K_CARD(card)){
		max_channels = 1;
	}else{
		max_channels = 32;
	}

	for (i=0;i<max_channels;i++){
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

	/* Only start the watchdog timer 
	 * when link comes up */
	aft_wdt_reset(card);

	AFT_FUNC_DEBUG();

	return 0;
}

int a104_chip_unconfig(sdla_t *card)
{
	u32 reg=0;

	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY

	if (card->wandev.fe_iface.unconfig){
		card->wandev.fe_iface.unconfig(&card->fe);
	}

#else

	aft_wdt_reset(card);

	/* Disable Octasic Chip */
	if (card->adptr_subtype == AFT_SUBTYPE_SHARK && card->wandev.ec_dev){
		/* ALEX CALL DISABLE */
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
	}

	/* Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		wan_smp_flag_t smp_flags1;

		card->hw_iface.hw_lock(card->hw,&smp_flags1);
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
		card->hw_iface.hw_unlock(card->hw,&smp_flags1);

	}else if (IS_56K_CARD(card)) {
		//reset_on_LXT441PE(card);
		aft_56k_write_cpld(card, 0x00,0x00);
		WP_DELAY(1000);
	}

	wan_set_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);
#endif
	return 0;

}

void a104_chan_config_mux_enable_b601(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	u32 i;
	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card, AFT_B601_DATA_MUX_EN_CTRL_REG),&reg);
	if (chan->cfg.data_mux) {
		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			if (wan_test_bit(i,&chan->time_slot_map)){
				wan_set_bit(i, &reg);
			}
		}
	} else {
		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			if (wan_test_bit(i,&chan->time_slot_map)){
				wan_clear_bit(i, &reg);
			}
		}
	}
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card, AFT_B601_DATA_MUX_EN_CTRL_REG),reg);
}

void a104_chan_dev_config_tdmv(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	if (CHAN_GLOBAL_IRQ_CFG(chan)){
	
		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
		aft_lcfg_tdmv_cnt_inc(&reg);
	
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);
		card->u.aft.lcfg_reg=reg;
	
		wan_set_bit(chan->logic_ch_num,&card->u.aft.tdm_logic_ch_map);
	}
}

int a104_chan_dev_config(sdla_t *card, void *chan_ptr)
{
	u32 reg;
	long i;
	int chan_num=-EBUSY;
	private_area_t *chan = (private_area_t*)chan_ptr;
	u32 ctrl_ram_reg,dma_ram_reg;
	int force_lch=0;

	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY

#else

#if 0
#warning "FORCE_LCH Should not be used and should be removed after B601 code"
	if (card->u.aft.global_tdm_irq) {
		force_lch=0;
	}
#endif

	chan_num=aft_request_logical_channel_num(card, chan, force_lch);
	if (chan_num < 0){
		return -EBUSY;
	}
	chan->logic_ch_num = chan_num;

	DEBUG_TEST("%s: LOGIC CH NUM %i\n", card->devname, chan_num);

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

	DEBUG_TEST("%s: CHAN = %i DMA RAM 0x%X  Reg=0x%08X  MRU=%i\n",
			card->devname, chan->logic_ch_num, dma_ram_reg, reg, chan->mru);

	reg=0;	

	for (i=0;i<card->u.aft.num_of_time_slots;i++){

		ctrl_ram_reg=AFT_PORT_REG(card,AFT_CONTROL_RAM_ACCESS_BASE_REG);
		ctrl_ram_reg+=(i*4);

		if (wan_test_bit(i,&chan->time_slot_map)){
			
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

			DEBUG_TEST("%s: Configuring %s for timeslot %ld : Offset 0x%X Reg 0x%X\n",
					card->devname, chan->if_name, i,
					ctrl_ram_reg,reg);

			card->hw_iface.bus_write_4(card->hw, ctrl_ram_reg, reg);

		}
	}

	a104_chan_dev_config_tdmv(card, chan);
#endif
	return 0;
}

int a104_chan_dev_unconfig(sdla_t *card, void *chan_ptr)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	volatile int i;
	u32 dma_ram_reg,ctrl_ram_reg,reg;

	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY

#else
	/* Select an HDLC logic channel for configuration */
	if (chan->logic_ch_num != -1){

		dma_ram_reg=AFT_PORT_REG(card,AFT_DMA_CHAIN_RAM_BASE_REG);
		dma_ram_reg+=(chan->logic_ch_num*4);

		card->hw_iface.bus_read_4(card->hw, dma_ram_reg, &reg);

		aft_dmachain_set_fifo_base(&reg,0x1F);
		aft_dmachain_set_fifo_size(&reg,0);
		card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);


	        for (i=0;i<card->u.aft.num_of_time_slots;i++){
        	        if (wan_test_bit(i,&chan->time_slot_map)){

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
			card->hw_iface.bus_read_4(card->hw,
					AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
			aft_lcfg_tdmv_cnt_dec(&reg);
			card->hw_iface.bus_write_4(card->hw,
					AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);
			wan_clear_bit(chan->logic_ch_num,&card->u.aft.tdm_logic_ch_map);
		}

		/* Do not clear the logi_ch_num here. 
		   We will do it at the end of del_if_private() funciton */
	}
#endif
	return 0;
}
 
int a104_check_ec_security(sdla_t *card)     
{
	u32 cfg_reg;	
	u32 security_bit=AFT_CHIPCFG_A104D_EC_SECURITY_BIT;
	
	AFT_FUNC_DEBUG();
#if INIT_FE_ONLY

#else
	if (card->u.aft.firm_id == AFT_DS_FE_CORE_ID/*card->adptr_type == A108_ADPTR_8TE1*/) {
		security_bit=AFT_CHIPCFG_A108_EC_SECURITY_BIT;
	}
	
	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG), &cfg_reg);
	if (wan_test_bit(security_bit,&cfg_reg)){ 
    		return 1; 	
	}
#endif
	return 0;
}


/*============================================================================
 * Read/Write 56k CPLD. Different from TE1!!
 */

int aft_56k_write_cpld(sdla_t *card, unsigned short cpld_off, u_int16_t cpld_data)
{
	cpld_off |= AFT56K_BIT_DEV_ADDR_CPLD; 
#if 0
	DEBUG_56K("%s(): cpld_off: 0x%X, cpld_data: 0x%X\n", 
		__FUNCTION__, cpld_off, cpld_data);
#endif

   	card->hw_iface.bus_write_2(card->hw, AFT56K_MCPU_INTERFACE_ADDR, cpld_off);
   	card->hw_iface.bus_write_2(card->hw, AFT56K_MCPU_INTERFACE, cpld_data);
	return 0;
}

unsigned char aft_56k_read_cpld(sdla_t *card, unsigned short cpld_off)
{
	unsigned int cpld_data;

	cpld_off |= AFT56K_BIT_DEV_ADDR_CPLD; 

   	card->hw_iface.bus_write_2(card->hw, AFT56K_MCPU_INTERFACE_ADDR, cpld_off);
   	card->hw_iface.bus_read_4(card->hw, AFT56K_MCPU_INTERFACE, &cpld_data);

#if 0
	DEBUG_56K("%s(): cpld_off: 0x%X, cpld_data: 0x%X\n", 
		__FUNCTION__, cpld_off, cpld_data);
#endif

	return (unsigned char)cpld_data;
}


/*============================================================================
 * Read TE1 CPLD.
 */

unsigned char aft_te1_read_cpld(sdla_t *card, unsigned short cpld_off)
{
        u8      tmp;
	int	err = -EINVAL;

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

int aft_te1_write_cpld(sdla_t *card, unsigned short off,u_int16_t data)
{
	int	err = -EINVAL;

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

unsigned char a108m_read_cpld(sdla_t *card, unsigned short cpld_off)
{
        u16     org_off;
        u8      tmp;

	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
		return 0x00;
	}
        cpld_off &= ~AFT8_BIT_DEV_ADDR_CLEAR;
        cpld_off |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;

        /*ALEX: Save the current address. */
        card->hw_iface.bus_read_2(card->hw,
                                AFT_MCPU_INTERFACE_ADDR,
                                &org_off);

        card->hw_iface.bus_write_2(card->hw,
                                AFT_MCPU_INTERFACE_ADDR,
                                cpld_off);

        card->hw_iface.bus_read_1(card->hw,AFT_MCPU_INTERFACE, &tmp);

        /*ALEX: Restore original address */
        card->hw_iface.bus_write_2(card->hw,
                                AFT_MCPU_INTERFACE_ADDR,
                                org_off);
	card->hw_iface.fe_clear_bit(card->hw,0);
        return tmp;
}

int a108m_write_cpld(sdla_t *card, unsigned short off,u_int16_t data)
{
	u16             org_off;

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
                                (u8)data);
        /*ALEX: Restore the original address */
        card->hw_iface.bus_write_2(card->hw,
                                AFT_MCPU_INTERFACE_ADDR,
                                org_off);
	card->hw_iface.fe_clear_bit(card->hw,0);
        return 0;
}


void a104_fifo_adjust(sdla_t *card, u32 level)
{
	u32 fifo_size,reg;
	card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card,AFT_FIFO_MARK_REG), &fifo_size);

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
	
	card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card,AFT_FIFO_MARK_REG), reg);		
	DEBUG_EVENT("%s:    Fifo Level Map:0x%08X\n",card->devname,reg);
}

#if defined(CONFIG_WANPIPE_HWEC)
static int aft_hwec_reset_b601(void *pcard, int reset)
{
	sdla_t      *card = (sdla_t*)pcard;
	wan_smp_flag_t  smp_flags,flags;
	int     err = -EINVAL;
	u32     reg;
	
	card->hw_iface.hw_lock(card->hw,&smp_flags);
	wan_spin_lock_irq(&card->wandev.lock,&flags);
	card->hw_iface.bus_read_4(card->hw,
					AFT_PORT_REG(card,AFT_CHIP_CFG_REG),
							&reg);
	if (!reset){
		DEBUG_EVENT("%s: B601 Clear Echo Canceller chip reset.\n",
				card->devname);
	
		reg &= ~0x1000000;
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card,AFT_CHIP_CFG_REG), reg);
	
		WP_DELAY(1000);
		err = 0;
	} else {
		DEBUG_EVENT("%s: B601 Set Echo Canceller chip reset.\n",
				card->devname);
	
		reg |= 0x1000000;
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card,AFT_CHIP_CFG_REG), reg);
	
		err = 0;
	}

	wan_spin_unlock_irq(&card->wandev.lock,&flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);

	return err;
}

static int aft_hwec_reset(void *pcard, int reset)
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

		if (card->u.aft.firm_id == AFT_DS_FE_CORE_ID) {
			/* Changed for all Dallas chips
			   changed from 0x07 to 0x0F to support
			   external clocking */
			aft_te1_write_cpld(card,0x00,0x0F);
		}else{

			if (IS_T1_CARD(card)){
	                //	aft_te1_write_cpld(card, 0x00, 0x00);
			//	WP_DELAY(1000);
                		aft_te1_write_cpld(card, 0x00, 0x01);
			}else{
        	        //	aft_te1_write_cpld(card,0x00, 0x02);
			//	WP_DELAY(1000);
	                	aft_te1_write_cpld(card,0x00, 0x03);
			}
		}
		WP_DELAY(1000);
		err = 0;

	}else{
		DEBUG_EVENT("%s: Set Echo Canceller chip reset.\n",
					card->devname);
		if (card->u.aft.firm_id == AFT_DS_FE_CORE_ID/*card->adptr_type == A108_ADPTR_8TE1*/) {
			aft_te1_write_cpld(card,0x00,0x06);
		}else{

			if (IS_T1_CARD(card)){
                		aft_te1_write_cpld(card, 0x00, 0x00);
			}else{
        	        	aft_te1_write_cpld(card,0x00, 0x02);
			}
		}
		err = 0;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);
	return err;
}
#endif

#if defined(CONFIG_WANPIPE_HWEC)	
static int aft_hwec_enable_b601(void *pcard, int enable, int fe_chan)
{
	sdla_t      *card = (sdla_t*)pcard;
	int     hw_chan = fe_chan;
	unsigned int    value;
	
	WAN_ASSERT(card == NULL);
	
	if (IS_T1_CARD(card)){
		hw_chan = fe_chan-1;
	}
	
	card->hw_iface.bus_read_4(
			card->hw,
			AFT_PORT_REG(card, AFT_B601_ECHO_EN_CTRL_REG),
			&value);
	
	if (enable) {
		wan_set_bit(fe_chan-1, &value);
	} else {
		wan_clear_bit(fe_chan-1, &value);
	}
	
	DEBUG_HWEC("[HWEC]: %s: %s bypass mode for fe_chan:%d (value=%X)...!\n",
			card->devname,
			(enable) ? "Enable" : "Disable",
			fe_chan, value);
	
	card->hw_iface.bus_write_4(
			card->hw,
			AFT_PORT_REG(card, AFT_B601_ECHO_EN_CTRL_REG),
			value);
	
	return 0;
}

/******************************************************************************
**		aft_hwec_enable()
**
** Return:	0   - success
**		1   - channel out of channel map
**		< 0 - failed
******************************************************************************/
static int aft_hwec_enable(void *pcard, int enable, int fe_chan)
{
	sdla_t		*card = (sdla_t*)pcard;
	int		hw_chan = fe_chan;
	unsigned int	value;

	WAN_ASSERT(card == NULL);

	if (IS_T1_CARD(card)){
		hw_chan = fe_chan-1;
	}

	card->hw_iface.bus_read_4(
			card->hw,
			AFT_PORT_REG(card,0x1000) + hw_chan * 4,
			&value);
   	if (enable){
		value |= 0x20;
        } else {
		value &= ~0x20;
        }
	DEBUG_HWEC("[HWEC]: %s: %s bypass mode for fe_chan:%d (value=%X)...!\n",
			card->devname,
			(enable) ? "Enable" : "Disable",
			fe_chan, value);
	card->hw_iface.bus_write_4(
			card->hw,
			AFT_PORT_REG(card,0x1000) + hw_chan * 4,
			value);
	return 0;
}
#endif
