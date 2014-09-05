/*****************************************************************************
* sdla_gsm.c 
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

/* 
 * The GSM card does not really have front end access, all register access should be done through card->hw_iface instead of fe-> pointer 
 */
# include "wanpipe_includes.h"
# include "wanpipe_defines.h"
# include "wanpipe_debug.h"
# include "wanpipe_abstr.h"
# include "wanpipe_common.h"
# include "wanpipe_events.h"
# include "wanpipe.h"
# include "wanpipe_events.h"
# include "if_wanpipe_common.h"
# include "aft_core.h" /* for private_area_t */
# include "sdla_gsm.h"

extern WAN_LIST_HEAD(, wan_tdmv_) wan_tdmv_head;

static int wp_gsm_config(void *pfe);
static int wp_gsm_unconfig(void *pfe);
static int wp_gsm_post_init(void *pfe);
static int wp_gsm_post_unconfig(void* pfe);
static int wp_gsm_if_config(void *pfe, u32 mod_map, u8);
static int wp_gsm_if_unconfig(void *pfe, u32 mod_map, u8);
static int wp_gsm_disable_irq(void *pfe); 
static int wp_gsm_intr(sdla_fe_t *); 
static int wp_gsm_dchan_tx(sdla_fe_t *fe, void *src_data_buffer, u32 buffer_len);
static int wp_gsm_check_intr(sdla_fe_t *); 
static int wp_gsm_polling(sdla_fe_t*);
static int wp_gsm_udp(sdla_fe_t*, void*, unsigned char*);
static unsigned int wp_gsm_active_map(sdla_fe_t* fe, unsigned char);
static unsigned char wp_gsm_fe_media(sdla_fe_t *fe);
static int wp_gsm_set_dtmf(sdla_fe_t*, int, unsigned char);
static int wp_gsm_intr_ctrl(sdla_fe_t*, int, u_int8_t, u_int8_t, unsigned int);
static int wp_gsm_event_ctrl(sdla_fe_t*, wan_event_ctrl_t*);
static int wp_gsm_get_link_status(sdla_fe_t *fe, unsigned char *status,int mod_no);

#if defined(AFT_TDM_API_SUPPORT)
static int wp_gsm_watchdog(void *card_ptr);
#endif

static void wp_gsm_toggle_pcm_loopback(sdla_t *card)
{
	u32 reg = 0;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_GLOBAL_REG, &reg);
	if (wan_test_bit(AFT_GSM_GLOBAL_PCM_LOOPBACK_BIT, &reg)) {
		wan_clear_bit(AFT_GSM_GLOBAL_PCM_LOOPBACK_BIT, &reg);
		DEBUG_EVENT("%s: PCM Loopback is now disabled\n", card->devname);
	} else {
		wan_set_bit(AFT_GSM_GLOBAL_PCM_LOOPBACK_BIT, &reg);
		DEBUG_EVENT("%s: PCM Loopback is now enabled\n", card->devname);
	}
	card->hw_iface.bus_write_4(card->hw, AFT_GSM_GLOBAL_REG, reg);
}

static void wp_gsm_dump_configuration(sdla_t *card)
{
	u32 reg = 0;
	int mod_no = card->wandev.comm_port + 1;

	reg = 0;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_GLOBAL_REG, &reg);
	DEBUG_EVENT("%s: Dumping Global Configuration (reg=%X)\n", card->devname, reg);
	DEBUG_EVENT("%s: PLL Reset: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_RESET_BIT, &reg));
	DEBUG_EVENT("%s: PLL Phase Shift Overflow: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_PHASE_SHIFT_OVERFLOW_BIT, &reg));
	DEBUG_EVENT("%s: PLL Input Clock Lost: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_INPUT_CLOCK_LOST_BIT, &reg));
	DEBUG_EVENT("%s: PLL Output Clock Stopped: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_OUTPUT_CLOCK_STOPPED_BIT, &reg));
	DEBUG_EVENT("%s: PLL Locked Bit: %d\n", card->devname, wan_test_bit(AFT_GSM_PLL_LOCKED_BIT, &reg));
	DEBUG_EVENT("%s: SIM Muxing Error: %d\n", card->devname, wan_test_bit(AFT_GSM_SIM_MUXING_ERROR_BIT, &reg));
	DEBUG_EVENT("%s: PCM Loopback Bit: %d\n", card->devname, wan_test_bit(AFT_GSM_GLOBAL_PCM_LOOPBACK_BIT, &reg));
	DEBUG_EVENT("%s: Global Shutdown Bit: %d\n", card->devname, wan_test_bit(AFT_GSM_GLOBAL_SHUTDOWN_BIT, &reg));

	reg = 0;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), &reg);
	DEBUG_EVENT("%s: Dumping Module %d Configuration (reg=%X)\n", card->devname, mod_no, reg);
	DEBUG_EVENT("%s: Module %d Power: %d\n", card->devname, mod_no, wan_test_bit(AFT_GSM_MOD_POWER_BIT, &reg));
	DEBUG_EVENT("%s: Module %d Reset: %d\n", card->devname, mod_no, wan_test_bit(AFT_GSM_MOD_RESET_BIT, &reg));
	DEBUG_EVENT("%s: Module %d Power Monitor: %d\n", card->devname, mod_no, wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg));
	DEBUG_EVENT("%s: Module %d Tx Monitor: %d\n", card->devname, mod_no, wan_test_bit(AFT_GSM_MOD_TX_MONITOR_BIT, &reg));
	DEBUG_EVENT("%s: Module %d SIM Inserted: %d\n", card->devname, mod_no, wan_test_bit(AFT_GSM_MOD_SIM_INSERTED_BIT, &reg));
	DEBUG_EVENT("%s: Module %d SIM Select: 0x%X\n", card->devname, mod_no, aft_gsm_get_sim_no(reg));
	DEBUG_EVENT("%s: Module %d UART Baud Rate: 0x%X\n", card->devname, mod_no, aft_gsm_get_uart_baud_rate(reg));
	DEBUG_EVENT("%s: Module %d Service: %d\n", card->devname, mod_no, wan_test_bit(AFT_GSM_MOD_SERVICE_BIT, &reg));
}


int wp_gsm_iface_init(void *p_fe, void *pfe_iface)
{
	sdla_fe_t *fe = p_fe;
	sdla_fe_iface_t	*fe_iface = pfe_iface;

	fe_iface->config	= &wp_gsm_config;
	fe_iface->unconfig	= &wp_gsm_unconfig;
	
	fe_iface->post_init	= &wp_gsm_post_init;
	
	fe_iface->if_config	= &wp_gsm_if_config;
	fe_iface->if_unconfig	= &wp_gsm_if_unconfig;
	fe_iface->post_unconfig	= &wp_gsm_post_unconfig;
	fe_iface->active_map	= &wp_gsm_active_map;

	fe_iface->isr		= &wp_gsm_intr;
	fe_iface->disable_irq	= &wp_gsm_disable_irq;
	fe_iface->check_isr	= &wp_gsm_check_intr;

	fe_iface->polling	= &wp_gsm_polling;
	fe_iface->process_udp	= &wp_gsm_udp;
	fe_iface->get_fe_media	= &wp_gsm_fe_media;

	fe_iface->set_dtmf	= &wp_gsm_set_dtmf;
	fe_iface->intr_ctrl	= &wp_gsm_intr_ctrl;

	fe_iface->event_ctrl	= &wp_gsm_event_ctrl;
#if defined(AFT_TDM_API_SUPPORT) || defined(AFT_API_SUPPORT)
	fe_iface->watchdog	= &wp_gsm_watchdog;
#endif
	fe_iface->get_fe_status = &wp_gsm_get_link_status;

	/* XXX this should be renamed to dchan_tx since is not BRI-specific XXX */
	fe_iface->isdn_bri_dchan_tx = &wp_gsm_dchan_tx;

	/*
	 * XXX Do we need this event initalization?? XXX
	 * WAN_LIST_INIT(&fe->event);
	 */
	wan_spin_lock_irq_init(&fe->lockirq, "wan_gsm_lock");
	return 0;
}

static int wp_gsm_config(void *pfe)
{
	return 0;
}

static int wp_gsm_unconfig(void *pfe)
{
	return 0;
}

static int wp_gsm_post_unconfig(void *pfe)
{
	return 0;
}

#define UPDATE_SIM_STATUS(card, fe) \
	if (card->wandev.te_link_state) { \
		card->wandev.te_link_state(card); \
	} else { \
		DEBUG_ERROR("%s: ERROR: GSM module %d has no te_link_state pointer!\n", card->devname, mod_no); \
	} \
	if (card->wandev.te_report_alarms) { \
		card->wandev.te_report_alarms(card, (fe->fe_status == FE_CONNECTED ? 0 : 1)); \
	}

int wp_gsm_update_sim_status(void *pcard)
{
	sdla_t *card = pcard;
	u32 reg = 0;
	sdla_fe_t *fe = &card->fe;
	int sim_inserted = 0;
	int mod_no = card->wandev.comm_port + 1;
	mod_no = card->wandev.comm_port + 1;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), &reg);
	sim_inserted = wan_test_bit(AFT_GSM_MOD_SIM_INSERTED_BIT, &reg) ? WAN_FALSE : WAN_TRUE;
	if (sim_inserted && fe->fe_status != FE_CONNECTED) {
		DEBUG_EVENT("%s: GSM module %d has a SIM inserted! (%s -> %s)\n", card->devname, mod_no, 
				FE_STATUS_DECODE(fe->fe_status), FE_STATUS_DECODE(FE_CONNECTED));
		fe->fe_status = FE_CONNECTED;
		UPDATE_SIM_STATUS(card, fe);
	} else if (!sim_inserted && fe->fe_status != FE_DISCONNECTED){
		DEBUG_EVENT("%s: GSM module %d has no SIM inserted! (%s -> %s)\n", card->devname, mod_no,
				FE_STATUS_DECODE(fe->fe_status), FE_STATUS_DECODE(FE_DISCONNECTED));
		fe->fe_status = FE_DISCONNECTED;
		UPDATE_SIM_STATUS(card, fe);
	}
	return 0;
}

static int wp_gsm_post_init(void *pfe)
{
	sdla_fe_t *fe = pfe;
	sdla_t *card = fe->card;
	wp_gsm_update_sim_status(card);
	return 0;
}

static int wp_gsm_if_config(void *pfe, u32 mod_map, u8 usedby)
{
	return 0;
}

static int wp_gsm_if_unconfig(void *pfe, u32 mod_map, u8 usedby)
{
	return 0;	
}

static int wp_gsm_disable_irq(void *pfe)
{
	return 0;
}

/* voice active map (not dchan) */
static unsigned int wp_gsm_active_map(sdla_fe_t* fe, unsigned char line)
{
	return (0x01 << (WAN_FE_LINENO(fe)));
}

static unsigned char wp_gsm_fe_media(sdla_fe_t *fe)
{
	return fe->fe_cfg.media;
}

static int wp_gsm_set_dtmf(sdla_fe_t *fe, int mod_no, unsigned char val)
{

	return -EINVAL;
}

static int wp_gsm_polling(sdla_fe_t* fe)
{
	return 0;
}

static int wp_gsm_event_ctrl(sdla_fe_t *fe, wan_event_ctrl_t *ectrl)
{
	return 0;	
}

#include "sdla_gsm_inline.h"
static int wp_gsm_udp(sdla_fe_t *fe, void *p_udp_cmd, unsigned char *data)
{
	wan_cmd_t *udp_cmd = (wan_cmd_t *)p_udp_cmd;
	wan_femedia_t *fe_media = NULL;
	sdla_t *card = fe->card;
	wan_gsm_udp_t *gsm_udp = (wan_gsm_udp_t *)data;
	int mod_no = 0;
	u32 reg = 0;

	switch (udp_cmd->wan_cmd_command) {

	case WAN_GET_MEDIA_TYPE:
		{
			fe_media = (wan_femedia_t*)data;
			memset(fe_media, 0, sizeof(wan_femedia_t));
			fe_media->media         = fe->fe_cfg.media;
			fe_media->sub_media     = fe->fe_cfg.sub_media;
			fe_media->chip_id       = 0x00;
			fe_media->max_ports     = MAX_GSM_MODULES;
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = sizeof(wan_femedia_t);
		}
                break;

	case WAN_GSM_REGDUMP:
		{
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = 0; 
			wp_gsm_dump_configuration(card);
		}
		break;

	case WAN_GSM_UART_DEBUG:
		{
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = 0; 
			if (gsm_udp && gsm_udp->u.uart_debug == WAN_TRUE) {
				wan_set_bit(AFT_GSM_UART_DEBUG_ENABLED_BIT, &card->TracingEnabled);
				DEBUG_EVENT("%s: UART debugging enabled\n", card->devname);
			} else {
				wan_clear_bit(AFT_GSM_UART_DEBUG_ENABLED_BIT, &card->TracingEnabled);
				DEBUG_EVENT("%s: UART debugging disabled\n", card->devname);
			}
		}
		break;

	case WAN_GSM_AUDIO_DEBUG:
		{
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = 0; 
			DEBUG_EVENT("%s: Audio debugging toggled\n", card->devname);
			wan_set_bit(AFT_GSM_AUDIO_DEBUG_TOGGLE_BIT, &card->TracingEnabled);
		}
		break;

	case WAN_GSM_PLL_RESET:
		{
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = 0; 
			wp_gsm_pll_reset(card);
		}
		break;

	case WAN_GSM_POWER_TOGGLE:
		{
			int map = 0;
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = 0; 
			reg = 0;
			mod_no = card->wandev.comm_port + 1;
			card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), &reg);
			DEBUG_EVENT("%s: Performing power toggle on module %d\n", card->devname, mod_no);
			if (wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg)) {
				map = wp_gsm_toggle_power(card->hw, (1 << (mod_no - 1)), WAN_FALSE);
			} else {
				map = wp_gsm_toggle_power(card->hw, (1 << (mod_no - 1)), WAN_TRUE);
			}
			if (map) {
				DEBUG_ERROR("%s: Error: Timed out performing power toggle for module %d (mod_map=0x%X)\n", card->devname, mod_no, map);
				udp_cmd->wan_cmd_return_code = WAN_CMD_TIMEOUT;
			}
		}
		break;

	case WAN_GSM_UPDATE_SIM_STATUS:
		{
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = 0; 
			wp_gsm_update_sim_status(card);
		}
		break;

	case WAN_GSM_PCM_LOOPBACK:
		{
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = 0; 
			wp_gsm_toggle_pcm_loopback(card);
		}
		break;

	default:
		{
			udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
			udp_cmd->wan_cmd_data_len = 0;
		}
		break;

	}
	return 0;
}


static int wp_gsm_watchdog(void *card_ptr)
{
	wan_smp_flag_t flags;
	sdla_t *card = card_ptr;
	sdla_fe_t *fe = &card->fe;
	sdla_gsm_param_t *gsm_data = &fe->fe_param.gsm;
	private_area_t *chan = NULL;
	int txlen = 0;
	int avail = 0;
	int kick_user = 0;

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	if (card->wan_tdmv.sc) {
		/* Do nothing if configured in TDM VOICE mode (DAHDI), the UART is serviced in the DAHDI tx/rx loop code  */
		return 0;
	}
#endif

	wp_gsm_update_sim_status(card);

	chan = card->u.aft.dev_to_ch_map[GSM_DCHAN_LOGIC_CHAN];
	if (!chan || wan_test_bit(0,&chan->interface_down)) {
		return 0;
	}

	/* tx until done and call wanpipe_wake_stack() when finished */
	wan_spin_lock_irq(&fe->lockirq, &flags);

	/* check if we have something to transmit and space to transmit it */
	if (gsm_data->uart_txbuf_len) {
		avail = wp_gsm_uart_check_tx_fifo(card);
		if (avail) {
			txlen = avail > (gsm_data->uart_txbuf_len - gsm_data->uart_tx_cnt) 
				? (gsm_data->uart_txbuf_len - gsm_data->uart_tx_cnt) 
				: avail;
			wp_gsm_uart_tx_fifo(card, (gsm_data->uart_txbuf + gsm_data->uart_tx_cnt), txlen);
			gsm_data->uart_tx_cnt += txlen;
			if (gsm_data->uart_tx_cnt == gsm_data->uart_txbuf_len) {
				/* everything was transmitted */
				gsm_data->uart_txbuf_len = 0;
				gsm_data->uart_tx_cnt = 0;
				chan = card->u.aft.dev_to_ch_map[GSM_DCHAN_LOGIC_CHAN];
				kick_user++;
			}
		}
	}

	if (!wan_test_bit(0,&chan->uart_rx_status)){
		/* service the rx fifo */
		avail = wp_gsm_uart_rx_fifo(card, chan->uart_rx_buffer, sizeof(chan->uart_rx_buffer));
		if (avail) {
			chan->uart_rx_sz=avail;
			wan_set_bit(0,&chan->uart_rx_status);
			WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
		}
	}

	wan_spin_unlock_irq(&fe->lockirq, &flags);

	if (kick_user) {
		wanpipe_wake_stack(chan);
	}

	return 0;
}

static int wp_gsm_intr_ctrl(sdla_fe_t *fe, int mod_no, u_int8_t type, u_int8_t mode, unsigned int ts_map)
{
	return 0;
}

static int wp_gsm_check_intr(sdla_fe_t *fe)
{
	return 0;
} 

static int wp_gsm_intr(sdla_fe_t *fe)
{
	return 0;
}

static int wp_gsm_dchan_tx(sdla_fe_t *fe, void *src_data_buffer, u32 buffer_len)
{
	wan_smp_flag_t flags;
	sdla_t *card = fe->card;
	sdla_gsm_param_t *gsm_data = &fe->fe_param.gsm;
	int ret = 0;
	if (!card) {
		DEBUG_ERROR("No card attached to GSM front end!\n");
		return 0;
	}

	wan_spin_lock_irq(&fe->lockirq, &flags);

	if (!src_data_buffer) {
		/* they want us to perform a space check */
		ret = gsm_data->uart_txbuf_len;
		goto done;
	}
	if (gsm_data->uart_txbuf_len) {
		ret = -EBUSY;
		goto done;
	}
	if (buffer_len > GSM_MAX_UART_FRAME_LEN) {
		/* too big of a frame to transmit for us */
		ret = -EFBIG;
		goto done;
	}
	memcpy(gsm_data->uart_txbuf, src_data_buffer, buffer_len);
	gsm_data->uart_txbuf_len = buffer_len;

done:
	wan_spin_unlock_irq(&fe->lockirq, &flags);

	return ret;
}

static int wp_gsm_get_link_status(sdla_fe_t *fe, unsigned char *status, int mod_no)
{
	u32 reg = 0;
	sdla_t *card = fe->card;
	if (!card) {
		DEBUG_ERROR("No card attached to GSM front end!\n");
		return 0;
	}
	/* ignore the provided module (that one is used for multiple modules in the same line/port */
	mod_no = card->wandev.comm_port + 1;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), &reg);
	if (!wan_test_bit(AFT_GSM_MOD_SIM_INSERTED_BIT, &reg)) {
		*status = FE_CONNECTED;
	} else {
		*status = FE_DISCONNECTED;
	}
	return 0;
}

static char *format_uart_data(char *dest, void *indata, int len) 
{
        int i;
        uint8_t *data = indata;
        char *p = dest;

        for (i = 0; i < len; i++) {
                switch(data[i]) {
                        case '\r':
                                sprintf(p, "\\r");
                                p+=2;
                                break;
                        case '\n':
                                sprintf(p, "\\n");
                                p+=2;
                                break;
                        case 0x1a:
                                sprintf(p, "<sub>");
                                p+=5;
                                break;
                        default:
                                *p = data[i];
                                p++; 
                }    
        }    
        *p = '\0';
        return dest;
}

int wp_gsm_uart_rx_fifo(void *pcard, unsigned char *rx_buff, int reqlen)
{
	u32 reg = 0;
	int i = 0;
	int mod_no = 0;
	int num_bytes = 0;
	char rx_debug[AFT_GSM_UART_RX_FIFO_SIZE * 2];
	sdla_t *card = pcard;

	mod_no = card->wandev.comm_port + 1;

	/* Check UART status */
	reg = 0;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_UART_STAT_REG), &reg);

	if (wan_test_bit(AFT_GSM_UART_RX_FIFO_OVERFLOW_BIT, &reg)) {
		DEBUG_ERROR("%s: RX UART FIFO overflow!\n", card->devname, mod_no);
		wan_clear_bit(AFT_GSM_UART_RX_FIFO_OVERFLOW_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_UART_STAT_REG), reg);
	}

	if (reqlen > AFT_GSM_UART_RX_FIFO_SIZE) {
		DEBUG_ERROR("%s: Can't read more than UART FIFO size %d bytes, requested = %d!\n", card->devname, AFT_GSM_UART_RX_FIFO_SIZE, reqlen);
		return 0;
	}

	/* Try to read from the UART if needed */
	num_bytes = (reg >> AFT_GSM_UART_RX_FIFO_DATA_COUNT_OFFSET) & AFT_GSM_UART_RX_FIFO_DATA_COUNT_MASK;
	if (num_bytes > 0 && AFT_GSM_UART_RX_FIFO_SIZE >= num_bytes) {
		for (i = 0; i < num_bytes; i++) {
			card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_UART_RX_REG), &reg);
			rx_buff[i] = (reg & 0xFF);
		}
		if (wan_test_bit(AFT_GSM_UART_DEBUG_ENABLED_BIT, &card->TracingEnabled)) {
			DEBUG_EVENT("%s: UART RX %d bytes: %s\n", card->devname, num_bytes, format_uart_data(rx_debug, rx_buff, num_bytes));
		}
		return num_bytes;
	} else if (num_bytes > 0) {
		DEBUG_ERROR("%s: wtf? there is %d bytes to be received in the UART of GSM module %d "
			    "(max expected size = %d, requested = %d)\n", card->devname, num_bytes, mod_no, AFT_GSM_UART_RX_FIFO_SIZE, reqlen);
	}
	return 0;
}

int wp_gsm_uart_check_tx_fifo(void *pcard)
{
	u32 reg = 0;
	int mod_no = 0;
	int num_bytes = 0;
	sdla_t *card = pcard;

	mod_no = card->wandev.comm_port + 1;

	/* Check UART status */
	reg = 0;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_UART_STAT_REG), &reg);
	num_bytes = AFT_GSM_UART_TX_FIFO_SIZE - (reg & AFT_GSM_UART_TX_FIFO_DATA_COUNT_MASK);
	if (num_bytes < 0) {
		DEBUG_ERROR("%s: wtf? available tx bytes in UART = %d\n", card->devname, (reg & AFT_GSM_UART_TX_FIFO_DATA_COUNT_MASK));
		return 0;
	}
	return num_bytes;
}

int wp_gsm_uart_tx_fifo(void *pcard, unsigned char *tx_buff, int len)
{
	u32 reg = 0;
	int i = 0;
	int mod_no = 0;
	int num_bytes = 0;
	char tx_debug[AFT_GSM_UART_TX_FIFO_SIZE * 2];
	sdla_t *card = pcard;

	mod_no = card->wandev.comm_port + 1;

	/* Check UART status */
	reg = 0;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_UART_STAT_REG), &reg);

	/* Verify overflow (should we do something else?) */
	if (wan_test_bit(AFT_GSM_UART_TX_FIFO_OVERFLOW_BIT, &reg)) {
		DEBUG_ERROR("%s: TX UART FIFO overflow in GSM module %d!\n", card->devname, mod_no);
		wan_clear_bit(AFT_GSM_UART_TX_FIFO_OVERFLOW_BIT, &reg);
		card->hw_iface.bus_write_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_UART_STAT_REG), reg);
	}

	reg = 0;
	num_bytes = AFT_GSM_UART_TX_FIFO_SIZE - (reg & AFT_GSM_UART_TX_FIFO_DATA_COUNT_MASK);

       	if (num_bytes < 0) {
		DEBUG_ERROR("%s: wtf? available tx bytes in UART = %d\n", card->devname, (reg & AFT_GSM_UART_TX_FIFO_DATA_COUNT_MASK));
		return 0;
	}

	if (!num_bytes) {
		DEBUG_ERROR("%s: Can't transmit anything to GSM UART module %d at the moment (requested = %d)\n", card->devname, len);
		return 0;
	}

	if (num_bytes < len) {
		DEBUG_ERROR("%s: Can't transmit more than %d bytes to GSM UART module %d at the moment (requested = %d)\n", card->devname, num_bytes, len);
		len = num_bytes;
	}

	/* write to the UART */
	for (i = 0; i < len; i++) {
		reg = (tx_buff[i] & 0xFF);
		card->hw_iface.bus_write_4(card->hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_UART_TX_REG), reg);
	}
	if (wan_test_bit(AFT_GSM_UART_DEBUG_ENABLED_BIT, &card->TracingEnabled)) {
		DEBUG_EVENT("%s: UART TX %d bytes: %s\n", card->devname, len, format_uart_data(tx_debug, tx_buff, len));
	}
	return len;
}

int wp_gsm_pll_reset(void *pcard)
{
	u32 reg = 0;
	int timeout_loops = 0;
	sdla_t *card = pcard;

	/* Reset GSM PLL (This allows UART communication) */
	DEBUG_EVENT("%s: Resetting GSM PLL (UART)\n", card->devname);
	reg = 0;
	card->hw_iface.bus_read_4(card->hw, AFT_GSM_GLOBAL_REG, &reg);	

	wan_set_bit(AFT_GSM_PLL_RESET_BIT, &reg);
	card->hw_iface.bus_write_4(card->hw, AFT_GSM_GLOBAL_REG, reg);
	WP_DELAY(10);

	wan_clear_bit(AFT_GSM_PLL_RESET_BIT, &reg);
	card->hw_iface.bus_write_4(card->hw, AFT_GSM_GLOBAL_REG, reg);
	WP_DELAY(10);

	for (timeout_loops = (AFT_GSM_PLL_ENABLE_TIMEOUT_MS / AFT_GSM_PLL_ENABLE_CHECK_INTERVAL_MS); 
	     timeout_loops; 
	     timeout_loops--) {
		reg = 0;
		card->hw_iface.bus_read_4(card->hw, AFT_GSM_GLOBAL_REG, &reg);	
		if (wan_test_bit(AFT_GSM_PLL_LOCKED_BIT, &reg)) {
			break;
		}
		timeout_loops--;
		WP_DELAY(AFT_GSM_MODULE_POWER_TOGGLE_CHECK_INTERVAL_MS * 1000);
	}
	return 0;
}

