/*****************************************************************************
* aft_gsm.c 
* 		
* 		WANPIPE(tm) AFT W400 Hardware Support
*
* Authors: 	Moises Silva <moy@sangoma.com>
*
* Copyright:	(c) 2011 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Oct 06, 2011  Moises Silva Initial Version
*****************************************************************************/
#if defined(__WINDOWS__)

# include <wanpipe_includes.h>
# include <wanpipe_defines.h>
# include <wanpipe.h>

# include <wanpipe_abstr.h>
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>

#else
# include <linux/wanpipe_includes.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe.h>
# include <linux/wanproc.h>
# include <linux/wanpipe_abstr.h>
# include <linux/if_wanpipe_common.h>
# include <linux/if_wanpipe.h>
# include <linux/sdlapci.h>
# include <linux/aft_core.h>
# include <linux/wanpipe_iface.h>
# include <linux/wanpipe_tdm_api.h>
# include <linux/sdla_gsm.h>
#endif


int aft_gsm_test_sync(sdla_t *card, int tx_only)
{
	return 0;
}

int aft_gsm_led_ctrl(sdla_t *card, int color, int led_pos, int on)
{
	return 0;
}

#include "sdla_gsm_inline.h"
int aft_gsm_global_chip_config(sdla_t *card)
{
	u32 reg = 0;
	int used_cnt = 0;
	u8 core_rev = 0;
	int pll_reset_counter = 1;

	DEBUG_EVENT("%s: Performing GSM global chip configuration\n", card->devname);

	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);

	card->fe_no_intr = 1;
	if (used_cnt == 1) {
		DEBUG_EVENT("%s: Performing GSM FPGA reset ...\n", card->devname);
		
		reg = 0;
		/*
		 * You would think to set/clear bits you need to first read the contents of
		 * the registry to not modify other stuff. In fact, looking at analog (aft_analog.c)
		 * it seems that is the way is done there. But in GSM, testing shows that if we 
		 * read the original register contents and only set the SFR EX/IN bits, the first time
		 * the PC boots and the port is started, the TDMV interrupt will not fire. Therefore
		 * is needed to clear the contents of all other bits reg=0 and just set the EX/IN bits
		 *
		 * card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card, AFT_CHIP_CFG_REG), &reg);	
		 *
		 */
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT, &reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card, AFT_CHIP_CFG_REG), reg);
		WP_DELAY(10);

		wan_clear_bit(AFT_CHIPCFG_SFR_EX_BIT, &reg);
		wan_clear_bit(AFT_CHIPCFG_SFR_IN_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card, AFT_CHIP_CFG_REG), reg);
		WP_DELAY(10);

		card->hw_iface.getcfg(card->hw, SDLA_COREREV, &core_rev);
		DEBUG_EVENT("%s: GSM FPGA reset done! (core rev = %X)\n", card->devname, core_rev);
		
		DEBUG_EVENT("%s: Resetting GSM front end\n", card->devname);
		/* FE reset */
		reg = 0;
		card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card, AFT_LINE_CFG_REG), &reg);	
		wan_set_bit(AFT_LCFG_FE_IFACE_RESET_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card, AFT_LINE_CFG_REG), reg);
		WP_DELAY(10);

		wan_clear_bit(AFT_LCFG_FE_IFACE_RESET_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card, AFT_LINE_CFG_REG),reg);
		WP_DELAY(10);
		DEBUG_EVENT("%s: GSM front end reset done!\n", card->devname);

		do {
			/* Due to some sort of hardware bug, it seems we have to do the PLL reset procedure twice 
			 * the first time after the PC boots up, otherwise often the UART will not work.
			 * Sometimes due to the PLL output clock stopped bit being set, but other times
			 * even when the status register says the PLL is locked (enabled) and there is no
			 * clock lost, the UART just won't work. I had to resort to do it twice every time :-(
			 */
			wp_gsm_pll_reset(card);
		} while (pll_reset_counter--);

		reg = 0;
		card->hw_iface.bus_read_4(card->hw, AFT_GSM_GLOBAL_REG, &reg);	
		if (!wan_test_bit(AFT_GSM_PLL_LOCKED_BIT, &reg)) {
			DEBUG_ERROR("%s: Error: Failed to enable PLL!\n", card->devname);
		} else if (wan_test_bit(AFT_GSM_PLL_OUTPUT_CLOCK_STOPPED_BIT, &reg)) {
			DEBUG_ERROR("%s: Error: PLL enabled but no output clock!\n", card->devname);
		} else if (wan_test_bit(AFT_GSM_PLL_INPUT_CLOCK_LOST_BIT, &reg)) {
			DEBUG_ERROR("%s: Error: PLL enabled but no input clock!\n", card->devname);
		} else {
			DEBUG_EVENT("%s: GSM PLL enabled!\n", card->devname);
		}

		/* Make sure the loopback bit is cleared 
		 * wanpipemon cmd can enable it and if the user stops the ports, the bit stays enabled even
		 * after restart, it makes more sense to start without the debug PCM loopback */
		wan_clear_bit(AFT_GSM_GLOBAL_PCM_LOOPBACK_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_GSM_GLOBAL_REG, reg);
#if 0
		DEBUG_EVENT("%s: Dumping Global Configuration (reg=%X)\n", card->devname, reg);
		DEBUG_EVENT("%s: PLL Reset: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_RESET_BIT, &reg));
		DEBUG_EVENT("%s: PLL Phase Shift Overflow: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_PHASE_SHIFT_OVERFLOW_BIT, &reg));
		DEBUG_EVENT("%s: PLL Input Clock Lost: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_INPUT_CLOCK_LOST_BIT, &reg));
		DEBUG_EVENT("%s: PLL Output Clock Stopped: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_OUTPUT_CLOCK_STOPPED_BIT, &reg));
		DEBUG_EVENT("%s: PLL Locked Bit: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_LOCKED_BIT, &reg));
		DEBUG_EVENT("%s: SIM Muxing Error: %d\n", card->devname, wan_test_bit(AFT_GSM_SIM_MUXING_ERROR_BIT, &reg));
		DEBUG_EVENT("%s: Global Shutdown Bit: %d\n", card->devname, wan_test_bit(AFT_GSM_GLOBAL_SHUTDOWN_BIT, &reg));
#endif

	}
	
	DEBUG_EVENT("%s: GSM global chip configuration done!\n", card->devname);
	return 0;
}

int aft_gsm_global_chip_unconfig(sdla_t *card)
{
	u32 reg = 0;
	int used_cnt = 0;

	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &used_cnt);

	DEBUG_EVENT("%s: GSM global chip unconfig (use count = %d)\n", card->devname, used_cnt);

	if (used_cnt == 1) {
		DEBUG_EVENT("%s: Unconfiguring unique GSM line (port = %d)\n", card->devname, card->wandev.comm_port + 1);
		/* GSM PLL */
		reg = 0;
		card->hw_iface.bus_read_4(card->hw, AFT_GSM_GLOBAL_REG, &reg);	
		wan_set_bit(AFT_GSM_PLL_RESET_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_GSM_GLOBAL_REG, reg);

		/* Front end */
		reg = 0;
		card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card, AFT_LINE_CFG_REG), &reg);	
		wan_set_bit(AFT_LCFG_FE_IFACE_RESET_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card, AFT_LINE_CFG_REG), reg);

		/* FPGA */
		reg = 0;
		card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card, AFT_CHIP_CFG_REG), &reg);	
		wan_set_bit(AFT_CHIPCFG_SFR_EX_BIT, &reg);
		wan_set_bit(AFT_CHIPCFG_SFR_IN_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card,AFT_CHIP_CFG_REG), reg);
	}
	return 0;
}

static void aft_gsm_read(sdla_t *card, u32 offset, u32 *reg)
{
	int mod_no = card->wandev.comm_port + 1;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, offset), reg);
	//DEBUG_EVENT("%s: regaccess: read from address 0x%X -> 0x%08X\n", card->devname, AFT_GSM_MOD_REG(mod_no, offset), *reg);
}

static void aft_gsm_write(sdla_t *card, u32 offset, u32 reg)
{
	int mod_no = card->wandev.comm_port + 1;
	card->hw_iface.bus_write_4(card->hw, AFT_GSM_MOD_REG(mod_no, offset), reg);
	//DEBUG_EVENT("%s: regaccess: wrote to address 0x%X -> 0x%08X\n", card->devname, AFT_GSM_MOD_REG(mod_no, offset), reg);
}

/* This is called for each port/line via user ioctl 
 * wanrouter_ioctl -> wan_device_setup -> setup -> wp_aft_w400_init -> aft_gsm_chip_config() */
int aft_gsm_chip_config(sdla_t *card, wandev_conf_t *conf)
{
	u32 reg = 0;
	int mod_no = card->wandev.comm_port + 1;

	DEBUG_EVENT("%s: Configuring GSM module %d\n", card->devname, mod_no);

	reg = 0;
	aft_gsm_read(card, AFT_GSM_CONFIG_REG, &reg);

	if (wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg)) {
		DEBUG_EVENT("%s: GSM module %d is already turned on ...\n", card->devname, mod_no);
		goto select_sim;
	}

	wp_gsm_toggle_power(card->hw, (1 << (mod_no - 1)), WAN_TRUE);

	reg = 0;
	aft_gsm_read(card, AFT_GSM_CONFIG_REG, &reg);
	if (!wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg)) {
		DEBUG_ERROR("%s: Failed to turn on GSM module %d\n", card->devname, mod_no);
		return -ETIMEDOUT;
	}

select_sim:
	aft_gsm_set_sim_no(&reg, mod_no);	
	aft_gsm_write(card, AFT_GSM_CONFIG_REG, reg);

	WP_DELAY(10);

	if (card->wandev.fe_iface.post_init) {
		card->wandev.fe_iface.post_init(&card->fe);
	}

	return 0;
}

int aft_gsm_chip_unconfig(sdla_t *card)
{
	u32 reg = 0;

	int mod_no = card->wandev.comm_port + 1;

	DEBUG_EVENT("%s: Unconfiguring GSM module %d\n", card->devname, mod_no);

	reg = 0;
	aft_gsm_read(card, AFT_GSM_CONFIG_REG, &reg);

	if (!wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg)) {
		DEBUG_EVENT("%s: GSM module %d is already turned off ...\n", card->devname, mod_no);
		goto done;
	}

	wp_gsm_toggle_power(card->hw, (1 << (mod_no - 1)), WAN_FALSE);

	reg = 0;
	aft_gsm_read(card, AFT_GSM_CONFIG_REG, &reg);
	if (wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg)) {
		DEBUG_ERROR("%s: Failed to turn off GSM module %d\n", card->devname, mod_no);
	}

done:
	/* clear the selected SIM */
	aft_gsm_set_sim_no(&reg, 0);
	aft_gsm_write(card, AFT_GSM_CONFIG_REG, reg);
	return 0;
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
	long i;
	int timeslots=0;

	DEBUG_CFG("%s: GSM Timeslots=%d, Logic ChMap 0x%lX \n", card->devname, card->u.aft.num_of_time_slots, card->u.aft.logic_ch_map);

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
				DEBUG_ERROR("%s: Channel/Time Slot resource conflict!\n",
						card->devname);
				DEBUG_ERROR("%s: %s: Channel/Time Slot %ld, aready in use!\n",
						card->devname,chan->if_name, (i+1));

				return -EEXIST;
			}
			timeslots++;
		}
	}

	if (timeslots > 1){
		DEBUG_ERROR("%s: GSM Interface can only support a single timeslot\n", card->devname);
		chan->first_time_slot = -1;
		return -1;
	}

	logic_ch = (signed char)chan->first_time_slot;
	if (logic_ch == 31) {
		/* The UART fake D-channel */
		chan->fifo_base_addr = 0x1;
	} else {
		/* Must be a voice channel */
		chan->fifo_base_addr = 0x0;
	}
	chan->fifo_size = 1;
	chan->fifo_size_code = 0;

	/* Add this timeslot to the map */
	card->u.aft.fifo_addr_map |= (1 << chan->fifo_base_addr);

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
		DEBUG_ERROR("%s: Error, request logical ch=%d map busy\n", card->devname,logic_ch);
		return -1;
	}

	card->u.aft.dev_to_ch_map[(unsigned char)logic_ch]=(void*)chan;

	if (logic_ch >= card->u.aft.top_logic_ch){
		card->u.aft.top_logic_ch=logic_ch;
		aft_dma_max_logic_ch(card);
	}


	DEBUG_EVENT("%s: Binding logic ch %d, fifo_base_addr=%X, fifo_size=%X, fifo addr map=%X, Ptr=%p\n",
			card->devname, logic_ch, chan->fifo_base_addr, chan->fifo_size, card->u.aft.fifo_addr_map, chan);
	return logic_ch;
}


int aft_gsm_chan_dev_config(sdla_t *card, void *chan_ptr)
{
	u32 reg = 0;
	private_area_t *chan = (private_area_t *)chan_ptr;
	u32 dma_ram_reg = 0;

	DEBUG_EVENT("%s: Configuring GSM channel device\n", card->devname);

	chan->logic_ch_num = aft_request_logical_channel_num(card, chan);
	if (chan->logic_ch_num == -1){
		return -EBUSY;
	}

	DEBUG_EVENT("%s: GSM received logic_ch_num = %d\n", card->devname, chan->logic_ch_num);

	dma_ram_reg = AFT_PORT_REG(card, AFT_DMA_CHAIN_RAM_BASE_REG);
	dma_ram_reg += (chan->logic_ch_num * 4);

	DEBUG_EVENT("%s: GSM logic_ch_num %d is at address 0x%08X\n", card->devname, chan->logic_ch_num, dma_ram_reg);

	reg = 0;
	card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);

	card->hw_iface.bus_read_4(card->hw, dma_ram_reg, &reg);

	aft_dmachain_set_fifo_size(&reg, chan->fifo_size_code);
	aft_dmachain_set_fifo_base(&reg, chan->fifo_base_addr);

	/* Initially always disable rx synchronization */
	wan_clear_bit(AFT_DMACHAIN_RX_SYNC_BIT, &reg);

	if (CHAN_GLOBAL_IRQ_CFG(chan)){
		aft_dmachain_enable_tdmv_and_mtu_size(&reg, chan->mru);
	}

	card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);


	reg = 0;	

	if (CHAN_GLOBAL_IRQ_CFG(chan)){
		card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
		aft_lcfg_tdmv_cnt_inc(&reg);

		card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card,AFT_LINE_CFG_REG), reg);
		card->u.aft.lcfg_reg = reg;
		wan_set_bit(chan->logic_ch_num, &card->u.aft.tdm_logic_ch_map);
	}

	if (card->wandev.fe_iface.if_config){
		card->wandev.fe_iface.if_config(
					&card->fe,
					chan->time_slot_map,
					chan->common.usedby);
	}
	
	return 0;
}

int aft_gsm_chan_dev_unconfig(sdla_t *card, void *chan_ptr)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	u32 dma_ram_reg, reg;
	volatile int i;

	DEBUG_EVENT("%s: Unconfiguring GSM channel device\n", card->devname);

	if (card->wandev.fe_iface.if_unconfig){
		card->wandev.fe_iface.if_unconfig(
					&card->fe,
					chan->time_slot_map,
					chan->common.usedby);
	}
	
	/* Select logic channel for configuration */
	if (chan->logic_ch_num != -1){

		dma_ram_reg = AFT_PORT_REG(card, AFT_DMA_CHAIN_RAM_BASE_REG);
		dma_ram_reg += (chan->logic_ch_num * 4);

		card->hw_iface.bus_read_4(card->hw, dma_ram_reg, &reg);

		aft_dmachain_set_fifo_base(&reg, 0x1F);
		aft_dmachain_set_fifo_size(&reg, 0);
		card->hw_iface.bus_write_4(card->hw, dma_ram_reg, reg);


		aft_free_logical_channel_num(card, chan->logic_ch_num);
		aft_free_fifo_baddr_and_size(card, chan);

		for (i = 0; i < card->u.aft.num_of_time_slots; i++){
			if (wan_test_bit(i, &chan->time_slot_map)){
				wan_clear_bit(i, &card->u.aft.time_slot_map);
			}
		}

		if (CHAN_GLOBAL_IRQ_CFG(chan)){
			card->hw_iface.bus_read_4(card->hw, AFT_PORT_REG(card,AFT_LINE_CFG_REG), &reg);
			aft_lcfg_tdmv_cnt_dec(&reg);
			card->hw_iface.bus_write_4(card->hw, AFT_PORT_REG(card,AFT_LINE_CFG_REG), reg);
			wan_clear_bit(chan->logic_ch_num,&card->u.aft.tdm_logic_ch_map);
		}
                 
	       	/* Do not clear the logic_ch_num here the core will do it at the end of del_if_private() function */ 
	}

	return 0;
}

int w400_check_ec_security(sdla_t *card)     
{  
    	return 0; 
}     

unsigned char aft_gsm_read_cpld(sdla_t *card, unsigned short cpld_off)
{
        return 0;
}

int aft_gsm_write_cpld(sdla_t *card, unsigned short off, u_int16_t data_in)
{
        return 0;
}

void aft_gsm_fifo_adjust(sdla_t *card, u32 level)
{
	return;
}

