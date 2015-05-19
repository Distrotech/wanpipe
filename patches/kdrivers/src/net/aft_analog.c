/*****************************************************************************
* aft_aft_analog.c 
* 		
* 		WANPIPE(tm) AFT A104 Hardware Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2005 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Sep 25, 2005  Nenad Corbic	Initial Version
*****************************************************************************/

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# include <wanpipe_includes.h>
# include <wanpipe.h>
# include <wanpipe_abstr.h>
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>
# include <wanpipe_iface.h>
# include <sdla_remora.h>
#elif defined(__WINDOWS__)
# include <wanpipe_includes.h>
# include <wanpipe.h>
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>
# include <wanpipe_iface.h>
# include <wanpipe_tdm_api.h>
# include <sdla_remora.h>

#else
# include <linux/wanpipe_includes.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe.h>
# include <linux/wanproc.h>
# include <linux/wanpipe_abstr.h>
# include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
# include <linux/if_wanpipe.h>
# include <linux/sdlapci.h>
//# include <linux/sdla_ec.h>
# include <linux/aft_core.h>
# include <linux/wanpipe_iface.h>
# include <linux/wanpipe_tdm_api.h>
# include <linux/sdla_remora.h>
#endif


/*==============================================
 * PRIVATE FUNCITONS
 *
 */
#if defined(CONFIG_WANPIPE_HWEC)
static int aft_analog_hwec_reset(void *pcard, int reset);
static int aft_analog_hwec_enable(void *pcard, int enable, int fe_chan);

static int aft_analog_hwec_reset_a600(void *pcard, int reset);
#endif

static int aft_map_fifo_baddr_and_size(sdla_t *card, unsigned char fifo_size, unsigned char *addr);

static char fifo_size_vector[] =  {1, 2, 4, 8, 16, 32};
static char fifo_code_vector[] =  {0, 1, 3, 7,0xF,0x1F};

static int request_fifo_baddr_and_size(sdla_t *card, private_area_t *chan)
{
	unsigned char req_fifo_size,fifo_size;
	int i;

	/* Calculate the optimal fifo size based
         * on the number of time slots requested */

	req_fifo_size = 1;

	DEBUG_INIT("%s:%s: Optimal Fifo Size =%d  Timeslots=%d \n",
		card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots);

	fifo_size=(unsigned char)aft_map_fifo_baddr_and_size(card,req_fifo_size,&chan->fifo_base_addr);
	if (fifo_size == 0 || chan->fifo_base_addr == 31){
		DEBUG_ERROR("%s:%s: Error: Failed to obtain fifo size %d or addr %d \n",
				card->devname,chan->if_name,fifo_size,chan->fifo_base_addr);
                return -EINVAL;
        }

	DEBUG_INIT("%s:%s: Optimal Fifo Size =%d  Timeslots=%d New Fifo Size=%d \n",
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

	DEBUG_INIT("%s: %s:Fifo Size=%d  Timeslots=%d Fifo Code=%d Addr=%d\n",
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
}


static int aft_free_fifo_baddr_and_size (sdla_t *card, private_area_t *chan)
{
	u32 reg=0;
	int i;

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

	return 0;
}


static char aft_request_logical_channel_num (sdla_t *card, private_area_t *chan)
{
	signed char logic_ch=-1;
	int err;
	long i;
	int timeslots=0;

	DEBUG_TEST("-- Request_Xilinx_logic_channel_num:--\n");

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
			timeslots++;
		}
	}

	if (timeslots > 1){
		DEBUG_ERROR("%s: Error: Analog Interface can only support a single timeslot\n",
				card->devname);
		chan->first_time_slot = -1;
		return -1;
	}


	err=request_fifo_baddr_and_size(card,chan);
	if (err){
		return -1;
	}

	logic_ch = (signed char)chan->first_time_slot;

	if (wan_test_and_set_bit(logic_ch,&card->u.aft.logic_ch_map)){
		return -1;
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
	return logic_ch;
}



/*==============================================
 * PUBLIC FUNCITONS
 *
 */

int aft_analog_test_sync(sdla_t *card, int tx_only)
{
	volatile int i,err=1;
	u32 reg;

	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
		
	if (wan_test_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg)){
		DEBUG_WARNING("%s: Warning: Analog Reset Enabled %d! \n",
				card->devname, card->wandev.comm_port+1);
	}

	
	for (i=0;i<500;i++){
	
		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
		if (tx_only){
			if (wan_test_bit(AFT_LCFG_TX_FE_SYNC_STAT_BIT,&reg)){
				err=-EINVAL;
				WP_DELAY(200);
			}else{
				err=0;
				break;
			}
		}else{	
			if (wan_test_bit(AFT_LCFG_TX_FE_SYNC_STAT_BIT,&reg) ||
			    wan_test_bit(AFT_LCFG_RX_FE_SYNC_STAT_BIT,&reg)){
				err=-EINVAL;
				WP_DELAY(200);
			}else{
				err=0;
				break;
			}
		}
	}

	DEBUG_TEST("%s: DELAY INDEX = %i\n",
			card->devname,i);

	return err;
}

int aft_analog_led_ctrl(sdla_t *card, int color, int led_pos, int on)
{
	
	/* NO LED ON ANALOG CARD */
	return 0;
}

int aft_analog_set_network_sync_a600(sdla_t *card)
{
	u32 reg_1090;

	reg_1090 = 0x00;
	card->hw_iface.bus_read_4(card->hw, AFT_CLKCFG_A600_REG, &reg_1090);	

	switch (WAN_FE_NETWORK_SYNC(&card->fe)) {
		default:
			DEBUG_WARNING("%s: Warning:Invalid FE_NETWORK_SYNC value\n",
					card->devname);
		case WANOPT_NO:
			DEBUG_EVENT("%s: Configuring for internal oscillator\n",
						card->devname);
		
			reg_1090 &= ~AFT_CLKCFG_A600_CLK_SRC_BIT_MASK;
			break;
		case WANOPT_YES:
			reg_1090 = 0x00;
			DEBUG_EVENT("%s: Analog Clock set to Network Sync!\n",
					card->devname);
		
			/* Set the board clock source to external clocking */
			reg_1090 |= (AFT_CLKCFG_A600_CLK_SRC_EXT_NO_PLL << 1);

			/* Set external clock source to 8.192 Mhz */
			wan_clear_bit(AFT_CLKCFG_A600_CLK_EXT_CLK_SRC_BIT, &reg_1090);
			break;
		case WAN_CLK_OUT_OSC:
			DEBUG_EVENT("%s: Configuring for clock output oscillator\n",
					card->devname);

			reg_1090 = 0x00;
			/* Enable clock output on jumper */
			wan_set_bit(AFT_CLKCFG_A600_CLK_OUTPUT_BIT, &reg_1090);
			reg_1090 &= ~AFT_CLKCFG_A600_CLK_SRC_BIT_MASK;
			break;
		case WAN_CLK_OUT_LINE:
/* daughter card is required for line clock output - not supported yet*/
#if 1
			/* Configure for oscillator output for now */
			DEBUG_WARNING("%s: Warning: Clock output line not supported on A600\n",
					card->devname);
			DEBUG_EVENT("%s: Configuring for oscillator output\n",
					card->devname);
		
			reg_1090 = 0x00;
			/* Enable clock output on jumper */
			wan_set_bit(AFT_CLKCFG_A600_CLK_OUTPUT_BIT, &reg_1090);
			reg_1090 &= ~AFT_CLKCFG_A600_CLK_SRC_BIT_MASK;	
			break;
#else
/* Use this section of code once daughter card is implemented */
			DEBUG_EVENT("%s: Configuring for clock output line\n",
					card->devname);

			/* Configure board to use clock from daughter card */
			reg_1090 |= (AFT_CLKCFG_A600_CLK_SRC_EXT_PLL << 1);
			break;
#endif
	}
	/* Set the external clock output mux to use board clock */
	reg_1090 |= (AFT_CLKCFG_A600_CLK_OUT_BOARD << 5);

	card->hw_iface.bus_write_4(card->hw, AFT_CLKCFG_A600_REG, reg_1090);	

	return 0;
}

int aft_analog_set_network_sync_b601(sdla_t *card)
{
   wan_smp_flag_t smp_flags, flags;
   u32 reg_1090;
   u16 max_ec_chans;

   reg_1090 = 0x00;
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

   card->hw_iface.bus_write_4(card->hw, AFT_CLKCFG_A600_REG, reg_1090);
   wan_spin_unlock_irq(&card->wandev.lock,&flags);
   card->hw_iface.hw_unlock(card->hw,&smp_flags);
   return 0;
}

int aft_analog_set_octasic_clk_src_b601(sdla_t *card)
{
	wan_smp_flag_t smp_flags, flags;
	u32 reg_1090;
	u8 ext_clk_mux_bits = 0;
	u8 pll_clk_mux_bits = 0;
	
	reg_1090 = 0x00;
	
	card->hw_iface.hw_lock(card->hw,&smp_flags);
	wan_spin_lock_irq(&card->wandev.lock,&flags);
	card->hw_iface.bus_read_4(card->hw, AFT_CLKCFG_A600_REG, &reg_1090);
	
	/* Define Ext Clock Val */
	reg_1090 &= ~(AFT_CLKCFG_B601_EXT_CLK_VAL_MASK<<AFT_CLKCFG_B601_EXT_CLK_VAL_SHIFT);
	if (WAN_FE_NETWORK_SYNC(&card->fe) == WAN_CLK_IN_2000HZ) {
		reg_1090 |= 1 << AFT_CLKCFG_B601_EXT_CLK_VAL_SHIFT;
	}

	/* Configure External clock out Mux */
	reg_1090 &= ~(AFT_CLKCFG_B601_EXT_CLK_MUX_MASK<<AFT_CLKCFG_B601_EXT_CLK_MUX_SHIFT);
	reg_1090 &= ~(AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_MASK<<AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_SHIFT);
	
	switch (WAN_FE_NETWORK_SYNC(&card->fe)) {
		case WANOPT_NO:
			ext_clk_mux_bits = AFT_CLKCFG_B601_EXT_CLK_MUX_OSC_2000HZ;
			pll_clk_mux_bits = AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_OSC_2000HZ;
			break;
		case WANOPT_YES:
		case WAN_CLK_IN_8000HZ:
		case WAN_CLK_IN_2000HZ:
			ext_clk_mux_bits = AFT_CLKCFG_B601_EXT_CLK_MUX_H100_CLK;
			pll_clk_mux_bits = AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_EXT_8000HZ;
			break;
		default:
		case WAN_CLK_IN_1500HZ:
			DEBUG_EVENT("%s: Error:Invalid Analog Network sync option\n", card->devname);
			return -EINVAL;
	}
	reg_1090 |= pll_clk_mux_bits << AFT_CLKCFG_B601_PLL_CLK_SRC_MUX_SHIFT;
	reg_1090 |= ext_clk_mux_bits << AFT_CLKCFG_B601_EXT_CLK_MUX_SHIFT;
	
	card->hw_iface.bus_write_4(card->hw, AFT_CLKCFG_A600_REG, reg_1090);
	wan_spin_unlock_irq(&card->wandev.lock,&flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);
	return 0;
}

int aft_analog_global_chip_config(sdla_t *card)
{
	u32	reg;
	int	err;
	int used_cnt;
	

	if (IS_A700_CARD(card)) {
	   	wan_set_bit(0,&card->fe_ignore_intr);
	}
	
	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);
			
	card->fe_no_intr=1;	
	
	/*============ GLOBAL CHIP CONFIGURATION ===============*/
	if (used_cnt == 1) {
		card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card,AFT_CHIP_CFG_REG), &reg);
		
		DEBUG_CFG("--- AFT Chip Reset. -- \n");
			/* Enable the chip/hdlc reset condition */
		reg=0;
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
	
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);

		WP_DELAY(10);

		/* Disable the chip/hdlc reset condition */
		wan_clear_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_clear_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
		
		/* Do not allow front end interrupt to start */
		if (!IS_A700_CARD(card)) {
			wan_clear_bit(AFT_CHIPCFG_FE_INTR_CFG_BIT,&reg);
		}
		if (IS_A200_CARD(card) || IS_A700_CARD(card) || IS_B800_CARD(card)) {
			if (WAN_FE_NETWORK_SYNC(&card->fe)){	/*card->fe.fe_cfg.cfg.remora.network_sync*/
				DEBUG_EVENT("%s: Analog Clock set to Network Sync!\n",
						card->devname);
				wan_set_bit(AFT_CHIPCFG_ANALOG_CLOCK_SELECT_BIT,&reg);	
			}
		}
		
		DEBUG_CFG("--- Chip enable/config. -- \n");
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);

		if (IS_B601_CARD(card)) {
			err = aft_analog_set_octasic_clk_src_b601(card);
			if (err) {
				DEBUG_EVENT("%s: Failed to configure B601 Octasic clock source\n",
								card->devname);
				return -EINVAL;
			}
			err = aft_analog_set_network_sync_b601(card);
			if (err) {
				DEBUG_EVENT("%s: Failed to configure B601 Network Sync\n",
								card->devname);
				return -EINVAL;
			}
		} else if (IS_A600_CARD(card)) {
			u8 core_rev;
			card->hw_iface.getcfg(card->hw, SDLA_COREREV, &core_rev);

			if (core_rev >= 3 && aft_chipcfg_get_b601_security(reg)) {
				/* It looks like the customer bought a B601 but removed the T1/E1 daughter card */
				DEBUG_EVENT("%s: AFT-B600 card was built as AFT-B601\n", card->devname);

				err = aft_analog_set_octasic_clk_src_b601(card);
				if (err) {
					DEBUG_EVENT("%s: Failed to configure B601 Octasic clock source\n",
									card->devname);
					return -EINVAL;
				}
				err = aft_analog_set_network_sync_b601(card);
				if (err) {
					DEBUG_EVENT("%s: Failed to configure B601 Network Sync\n",
									card->devname);
					return -EINVAL;
				}
			} else {
				err = aft_analog_set_network_sync_a600(card);
				if (err) {
					DEBUG_EVENT("%s: Failed to configure clock source\n",
									card->devname);
					return -EINVAL;
				}
			}
		} else {
			/* Set Octasic reset */
			aft_analog_write_cpld(card, 0x00, 0x00);
		}
	} else {
		if (WAN_FE_NETWORK_SYNC(&card->fe)){
			DEBUG_EVENT("%s: Ignoring Network Sync\n", card->devname);
		}
	}

	
	DEBUG_EVENT("%s: Global Front End Configuration!\n",card->devname);
	err = -EINVAL;
	
	if (card->wandev.fe_iface.config){
		err = card->wandev.fe_iface.config(&card->fe);
	}
	if (err){
		DEBUG_EVENT("%s: Failed Front End configuration!\n",
					card->devname);
		return -EINVAL;
	}

	/* Run rest of initialization not from lock */
	if (card->wandev.fe_iface.post_init){
		err=card->wandev.fe_iface.post_init(&card->fe);
	}

	DEBUG_EVENT("%s: Remora config done!\n",card->devname);
	return 0;
}

int aft_analog_global_chip_unconfig(sdla_t *card)
{
	u32	reg=0;
	int used_cnt;

	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);
	if (card->wandev.fe_iface.unconfig){
		card->wandev.fe_iface.unconfig(&card->fe);
	}

	if (used_cnt == 1) {
		/* Set Octasic to reset */
		aft_analog_write_cpld(card, 0x00, 0x00);
		/* Disable the chip/hdlc reset condition */
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT,&reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT,&reg);
	
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG),reg);
	}
	return 0;
}

int aft_analog_chip_config(sdla_t *card, wandev_conf_t *conf)
{
	u32 reg=0;
	int err=0;
	
	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
	if (!wan_test_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg)){
		DEBUG_ERROR("%s: Error: Physical Port %i is busy! \n",
				card->devname, card->wandev.comm_port+1);
		return -EBUSY;
	}
	
	/*============ LINE/PORT CONFIG REGISTER ===============*/

	card->hw_iface.bus_read_4(card->hw,
				AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
	
	wan_set_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);

	WP_DELAY(10);

	wan_clear_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);

	WP_DELAY(10);


	if (card->adptr_type == AFT_ADPTR_FLEXBRI) {
		/* A700 works like A700, therefore need to check for RX Sync */
		err=aft_analog_test_sync(card,0);
	} else {
		err=aft_analog_test_sync(card,1);
	}

	card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
	if (err != 0){
		DEBUG_ERROR("%s: Error: Front End Interface Not Ready (0x%08X)!\n",
					card->devname,reg);
		return err;
    	} else{
		DEBUG_EVENT("%s: Front End Interface Ready 0x%08X\n",
                                        card->devname,reg);
    	}

	/* Enable Octasic Chip */
	if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
		u16	max_ec_chans, max_chans_no;
		u16	max_chip_cfg_ec_chans;
		u32 	cfg_reg, fe_chans_map;
		u8 core_rev;

		card->hw_iface.getcfg(card->hw, SDLA_HWEC_NO, &max_ec_chans);
		card->hw_iface.getcfg(card->hw, SDLA_CHANS_NO, &max_chans_no);
		card->hw_iface.getcfg(card->hw, SDLA_CHANS_MAP, &fe_chans_map);
		card->hw_iface.getcfg(card->hw, SDLA_COREREV, &core_rev);

		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG), &cfg_reg);
		
       if (IS_A600_CARD(card)) {
			max_chip_cfg_ec_chans = (u16)aft_chipcfg_get_a600_ec_channels(cfg_reg, core_rev);
		} else if (IS_B601_CARD(card)) {
			max_chip_cfg_ec_chans = (u16)aft_chipcfg_get_b601_ec_channels(cfg_reg);
		} else if (IS_A700_CARD(card)) {
			max_chip_cfg_ec_chans = (u16)aft_chipcfg_get_a700_ec_channels(cfg_reg);	
		} else {
			max_chip_cfg_ec_chans = (u16)aft_chipcfg_get_a200_ec_channels(cfg_reg);	
		}
		
		if (max_ec_chans > max_chip_cfg_ec_chans){
	        	DEBUG_ERROR("%s: Critical Error: Exceeded Maximum Available Echo Channels!\n",
					card->devname);
			DEBUG_ERROR("%s: Critical Error: Max Allowed=%d Configured=%d (%X)\n",
				card->devname,
				max_chip_cfg_ec_chans,
				max_ec_chans,
				cfg_reg);  
			return -EINVAL;
		}

		if (max_ec_chans){
#if defined(CONFIG_WANPIPE_HWEC)
			card->wandev.ec_dev = wanpipe_ec_register(
							card,
							fe_chans_map,
							max_chans_no,
							max_ec_chans,
							(void*)&conf->oct_conf);
						
			if (IS_A600_CARD(card) || IS_B601_CARD(card)) {
				card->wandev.hwec_reset = aft_analog_hwec_reset_a600;
			} else {
				card->wandev.hwec_reset = aft_analog_hwec_reset;
			}

			card->wandev.hwec_enable = aft_analog_hwec_enable;
#else
			DEBUG_EVENT("%s: Wanpipe HW Echo Canceller modele is not compiled!\n",
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
	
	aft_analog_led_ctrl(card, WAN_AFT_RED, 0,WAN_AFT_ON);
	aft_analog_led_ctrl(card, WAN_AFT_GREEN, 0, WAN_AFT_OFF);

	
	/*============ DMA CONTROL REGISTER ===============*/
	
	/* Disable Global DMA because we will be 
		* waiting for the front end to come up */
	reg=0;
	aft_dmactrl_set_max_logic_ch(&reg,0);
	wan_clear_bit(AFT_DMACTRL_GLOBAL_INTR_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_DMA_CTRL_REG),reg);

	/*============ ENABLE MUX ==================*/

	/* A700 does not use Data Mux, uses similar regs as A500 */
	if (!IS_A700_CARD(card)) {
		reg=0;	/* 0xFFFFFFFF; */
		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_ANALOG_DATA_MUX_CTRL_REG),reg);
	}

	aft_wdt_reset(card);
#if 0
/*FIXME: NC Dont need watchdog */
	aft_wdt_set(card,AFT_WDTCTRL_TIMEOUT);
#endif

	return 0;
}

int aft_analog_chip_unconfig(sdla_t *card)
{
	u32 reg=0;
	int used_cnt;

	aft_wdt_reset(card);
	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);	

	/* Disable Octasic Chip */
	if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
		if (card->wandev.ec_dev){
#if defined(CONFIG_WANPIPE_HWEC)
			wanpipe_ec_unregister(card->wandev.ec_dev, card);
#else
			DEBUG_EVENT(
			"%s: Wanpipe HW Echo Canceller modele is not compiled!\n",
						card->devname);
#endif
		}
		card->wandev.hwec_enable = NULL;
		card->wandev.ec_dev = NULL;
	}
	
	wan_set_bit(AFT_LCFG_FE_IFACE_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);

	return 0;
}

static int aft_analog_ctrl_ram_config_a700(sdla_t *card, private_area_t* chan)
{
	u32 ctrl_ram_reg;
	u32 reg;

	ctrl_ram_reg=AFT_PORT_REG(card,AFT_CONTROL_RAM_ACCESS_BASE_REG);
	ctrl_ram_reg+=(chan->first_time_slot*4);

	

		BRI_FUNC();
		
		wan_set_bit(chan->first_time_slot,&card->u.aft.time_slot_map);
		
		card->hw_iface.bus_read_4(card->hw, ctrl_ram_reg, &reg);

		aft_ctrlram_set_logic_ch(&reg,chan->logic_ch_num);

		wan_set_bit(AFT_CTRLRAM_SYNC_FST_TSLOT_BIT,&reg);	
			
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

		wan_clear_bit(AFT_CTRLRAM_SS7_ENABLE_BIT,&reg);		

		wan_clear_bit(AFT_CTRLRAM_HDLC_TXCH_RESET_BIT,&reg);
		wan_clear_bit(AFT_CTRLRAM_HDLC_RXCH_RESET_BIT,&reg);

		DEBUG_CFG("%s: Configuring %s LC=%i for timeslot %ld : Offset 0x%X Reg 0x%X\n",
				card->devname, chan->if_name, chan->logic_ch_num, chan->first_time_slot,
				ctrl_ram_reg,reg);

		card->hw_iface.bus_write_4(card->hw, ctrl_ram_reg, reg);

	return 0;
}

int aft_analog_chan_dev_config(sdla_t *card, void *chan_ptr)
{
	u32 reg;
	private_area_t *chan = (private_area_t*)chan_ptr;
	u32 dma_ram_reg;
	int err;


	chan->logic_ch_num=aft_request_logical_channel_num(card, chan);
	if (chan->logic_ch_num == -1){
		return -EBUSY;
	}

	dma_ram_reg=AFT_PORT_REG(card,AFT_DMA_CHAIN_RAM_BASE_REG);
	dma_ram_reg+=(chan->logic_ch_num*4);

	reg=0;
	card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);

	card->hw_iface.bus_read_4(card->hw, dma_ram_reg, &reg);

	aft_dmachain_set_fifo_size(&reg, chan->fifo_size_code);
	aft_dmachain_set_fifo_base(&reg, chan->fifo_base_addr);

	/* Initially always disable rx synchronization */
	wan_clear_bit(AFT_DMACHAIN_RX_SYNC_BIT,&reg);

	if (CHAN_GLOBAL_IRQ_CFG(chan)){
		aft_dmachain_enable_tdmv_and_mtu_size(&reg,chan->mru);
	}

	card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);


	reg=0;	

	if (IS_A700_CARD(card)) {
		err = aft_analog_ctrl_ram_config_a700(card, chan);
	}

	if (CHAN_GLOBAL_IRQ_CFG(chan)){

		card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),&reg);
		aft_lcfg_tdmv_cnt_inc(&reg);

		card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_LINE_CFG_REG),reg);
		card->u.aft.lcfg_reg=reg;
		wan_set_bit(chan->logic_ch_num,&card->u.aft.tdm_logic_ch_map);
	}

	/* FE chan config */
	if (card->wandev.fe_iface.if_config){
		card->wandev.fe_iface.if_config(
					&card->fe,
					chan->time_slot_map,
					chan->common.usedby);
	}
	
	return 0;
}

int aft_analog_chan_dev_unconfig(sdla_t *card, void *chan_ptr)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	u32 dma_ram_reg,reg;
	volatile int i;

	/* FE chan unconfig */
	if (card->wandev.fe_iface.if_unconfig){
		card->wandev.fe_iface.if_unconfig(
					&card->fe,
					chan->time_slot_map,
					chan->common.usedby);
	}
	
	/* Select an HDLC logic channel for configuration */
	if (chan->logic_ch_num != -1){

		dma_ram_reg=AFT_PORT_REG(card,AFT_DMA_CHAIN_RAM_BASE_REG);
		dma_ram_reg+=(chan->logic_ch_num*4);

		card->hw_iface.bus_read_4(card->hw, dma_ram_reg, &reg);

		aft_dmachain_set_fifo_base(&reg,0x1F);
		aft_dmachain_set_fifo_size(&reg,0);
		card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);


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

	return 0;
}

int a200_check_ec_security(sdla_t *card)     
{  
	u32 cfg_reg;	
	
    card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_CHIP_CFG_REG), &cfg_reg);
	if (wan_test_bit(AFT_CHIPCFG_A200_EC_SECURITY_BIT,&cfg_reg)){ 
    	return 1; 	
	}
	return 0;
}     

unsigned char aft_analog_read_cpld(sdla_t *card, unsigned short cpld_off)
{
        u16     org_off;
        u8      tmp;

	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
		return 0x00;
	}
 
        cpld_off |= AFT4_BIT_DEV_ADDR_CPLD;

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

int aft_analog_write_cpld(sdla_t *card, unsigned short off, u_int16_t data_in)
{
	u16             org_off;
	u8		data=(u8)data_in;

	if (card->hw_iface.fe_test_and_set_bit(card->hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			card->devname, __FUNCTION__,__LINE__);
		return 0x00;
	}

        off |= AFT4_BIT_DEV_ADDR_CPLD;

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

void aft_analog_fifo_adjust(sdla_t *card,u32 level)
{
	return;
}

#if defined(CONFIG_WANPIPE_HWEC)
static int aft_analog_hwec_reset_a600(void *pcard, int reset)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_smp_flag_t	smp_flags,flags;
	int		err = -EINVAL;
	u32		reg;

	card->hw_iface.hw_lock(card->hw,&smp_flags);
	wan_spin_lock_irq(&card->wandev.lock,&flags);
	card->hw_iface.bus_read_4(card->hw, 
				  AFT_PORT_REG(card,AFT_CHIP_CFG_REG),
					       &reg);
	if (!reset){
		DEBUG_EVENT("%s: A600 Clear Echo Canceller chip reset.\n",
			    card->devname);

		reg &= ~0x1000000;
		card->hw_iface.bus_write_4(card->hw,
						AFT_PORT_REG(card,AFT_CHIP_CFG_REG),
						reg);
						
		WP_DELAY(1000);
		err = 0;
	}else{
		DEBUG_EVENT("%s: A600 Set Echo Canceller chip reset.\n",
			    card->devname);
		
		
		reg |= 0x1000000;
		card->hw_iface.bus_write_4(card->hw, 
					   AFT_PORT_REG(card,AFT_CHIP_CFG_REG),
							   reg);
		
		err = 0;
	}
	
	wan_spin_unlock_irq(&card->wandev.lock,&flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);

	return err;
}

static int aft_analog_hwec_reset(void *pcard, int reset)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_smp_flag_t	smp_flags,flags;
	int		err = -EINVAL;

	card->hw_iface.hw_lock(card->hw,&smp_flags);
	wan_spin_lock_irq(&card->wandev.lock,&flags);
	if (!reset){
		DEBUG_EVENT("%s: Clear Echo Canceller chip reset.\n",
					card->devname);

	        aft_analog_write_cpld(card, 0x00, 0x01);
		WP_DELAY(1000);
		err = 0;
	}else{
		DEBUG_EVENT("%s: Set Echo Canceller chip reset.\n",
					card->devname);
		aft_analog_write_cpld(card, 0x00, 0x00);
		err = 0;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);

	return err;
}
#endif

#if defined(CONFIG_WANPIPE_HWEC)
static int aft_a700_analog_hwec_enable(sdla_t* card, int enable, int fe_chan)
{
	unsigned int    value, analog_chan;
	analog_chan = (fe_chan-1); /* {0,1}, {2,3}, {4,5}... */
	card->hw_iface.bus_read_4(
					card->hw,
					AFT_PORT_REG(card,0x1000) + analog_chan * 4,
					&value);
	if (enable){
		value |= 0x20;
	}else{
		value &= ~0x20;
	}

	DEBUG_HWEC("[HWEC A700]: %s: writing: off:0x%X val:0x%08X!\n",card->devname,
				AFT_PORT_REG(card,0x1000) + analog_chan * 4,
				value);

	card->hw_iface.bus_write_4(
				card->hw,
				AFT_PORT_REG(card,0x1000) + analog_chan * 4,
				value);
	return 0;
}

/******************************************************************************
**		aft_analog_hwec_enable()
**
** Return:	0   - success
**		1   - channel out of channel map
**		< 0 - failed
******************************************************************************/
#define	AFT_REMORA_MUX_TS_EC_ENABLE	0x210
static int aft_analog_hwec_enable(void *pcard, int enable, int fe_chan)
{
	sdla_t		*card = (sdla_t*)pcard;
	unsigned int	value = 0x00;
	int		hw_chan = fe_chan-1;

	WAN_ASSERT(card == NULL);

	if (card->adptr_type == AFT_ADPTR_FLEXBRI) {
		return aft_a700_analog_hwec_enable(card, enable, fe_chan);
	}
	card->hw_iface.bus_read_4(
			card->hw,
   			AFT_PORT_REG(card,AFT_REMORA_MUX_TS_EC_ENABLE),
			&value);
	if (enable){
		value |= (1 << hw_chan);
        } else {
		value &= ~(1 << hw_chan);
        }
	
	DEBUG_HWEC("[HWEC]: %s: %s bypass mode for fe_chan:%d (value=%X)...!\n",
			card->devname,
			(enable) ? "Enable" : "Disable",
			fe_chan, value);
	card->hw_iface.bus_write_4(
			card->hw,
			AFT_PORT_REG(card,AFT_REMORA_MUX_TS_EC_ENABLE),
			value);
	return 0;
}
#endif



