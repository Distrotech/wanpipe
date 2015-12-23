//////////////////////////////////////////////////////////////////////
// sangoma_port_configurator.cpp: implementation of the sangoma_port_configurator class.
//
// Author	:	David Rokhvarg
//////////////////////////////////////////////////////////////////////

#include "sangoma_port_configurator.h"

#define DBG_CFG		if(1)printf("PORTCFG:");if(1)printf
#define _DBG_CFG	if(1)printf

#define INFO_CFG	if(1)printf("PORTCFG:");if(1)printf
#define _INFO_CFG	if(1)printf

#define ERR_CFG		if(1)printf("%s():line:%d Error: ", __FUNCTION__, __LINE__);if(1)printf
#define _ERR_CFG	if(1)printf

//////////////////////////////////////////////////////////////////////

//these are recommendations only:
#define MIN_RECOMMENDED_BITSTREAM_MTU	256
#define MIN_RECOMMENDED_HDLC_MTU		2048

/*!
  \brief Brief description
 *
 */
static int get_number_of_channels(unsigned int chan_bit_map)
{
	int chan_counter = 0, i;

	for(i = 0; i < (int)(sizeof(unsigned int)*8); i++){
		if(chan_bit_map & (1 << i)){
			chan_counter++;
		}
	}
	return chan_counter;
}

/*!
  \brief Brief description
 *
 */
static int get_recommended_bitstream_mtu(int num_of_channels)
{
	int recommended_mtu = num_of_channels * 32;//<--- will work well in most cases

	//
	//Smaller mtu will cause more interrupts.
	//Recommend at least MIN_RECOMMENDED_BITSTREAM_MTU, user will set to smaller, if timing is important.
	//
	while(recommended_mtu < MIN_RECOMMENDED_BITSTREAM_MTU){
		recommended_mtu = recommended_mtu * 2;
	}

	return recommended_mtu;
}

/*!
  \brief Brief description
 *
 */
static int check_mtu(int mtu, int hdlc_streaming, unsigned int channel_bitmap)
{
	DBG_CFG("%s(): channel_bitmap: 0x%X\n", __FUNCTION__, channel_bitmap);

	if(mtu % 4){
		ERR_CFG("Invalid MTU (%d). Must be divizible by 4.\n", mtu);
		return 1;
	}

	//if BitStream, check MTU is also divisible by the number of channels (timeslots) in the group
	if(hdlc_streaming == WANOPT_NO){

		int num_of_channels = get_number_of_channels(channel_bitmap);

		DBG_CFG("num_of_channels in bitmap: %d\n", num_of_channels);

		if(mtu % num_of_channels){
			ERR_CFG("Invalid MTU. For 'BitStream' Must be divisible by 4 AND by the number of\n\
						\tchannels in the group (%d). Recommended MTU: %d.\n",
						num_of_channels, get_recommended_bitstream_mtu(num_of_channels));
			return 2;
		}
	}//if()

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/*!
  \brief Brief description
 *
 */
sangoma_port_configurator::sangoma_port_configurator()
{
#if defined(__WINDOWS__)
	hPortRegistryKey = (struct HKEY__ *)INVALID_HANDLE_VALUE;
#endif
}

/*!
  \brief Brief description
 *
 */
sangoma_port_configurator::~sangoma_port_configurator()
{
#if defined(__WINDOWS__)
	if(hPortRegistryKey != (struct HKEY__ *)INVALID_HANDLE_VALUE){
		RegCloseKey(hPortRegistryKey);
	}
#endif
}


/*!
  \brief SET new configuration of the API driver.
 *
 */
int sangoma_port_configurator::SetPortVolatileConfigurationCommand(port_cfg_t *port_cfg)
{
	int err = sangoma_driver_port_set_config(wp_handle, port_cfg, wp_number);
	if (err) {
		err = 1;
	}
	return err;
}

/*!
  \brief Brief description
 *
 */
int sangoma_port_configurator::get_configration(port_cfg_t *port_cfg)
{
	int err = sangoma_driver_port_get_config(wp_handle, port_cfg, wp_number);
	if (err) {
		err = 1;
	}
	return err;
}


/*!
  \brief Function to check correctness of 'port_cfg_t' structure.
 *
 */
int sangoma_port_configurator::check_port_cfg_structure(port_cfg_t *port_cfg)
{
	unsigned int i, rc = 0;
	unsigned long tmp_active_ch = 0;
	unsigned long cumulutive_active_ch = 0;

	wandev_conf_t		*wandev_conf = &port_cfg->wandev_conf;
	sdla_fe_cfg_t		*sdla_fe_cfg = &wandev_conf->fe_cfg;
	wanif_conf_t		*wanif_conf;

	DBG_CFG("%s()\n", __FUNCTION__);

	INFO_CFG("Running Driver Configration structure check...\n");
	if(port_cfg->num_of_ifs < 1 || port_cfg->num_of_ifs > NUM_OF_E1_TIMESLOTS){
			_INFO_CFG("Invalid number of Timeslot Groups - %d! Should be between %d and %d (including).\n",
				port_cfg->num_of_ifs, 1, NUM_OF_E1_TIMESLOTS);
			return 1;
	}

	for(i = 0; i < port_cfg->num_of_ifs; i++){

		//check all logic channels were configured
		wanif_conf = &port_cfg->if_cfg[i];
		if(wanif_conf->active_ch == 0){
			_INFO_CFG("Group %d - no T1/E1 channels specified! Expecting a bitmap of channels.\n", i + 1);
			rc = 1; break;
		}

		//check the channels are valid for media type.
		if(sdla_fe_cfg->media == WAN_MEDIA_T1){
			unsigned int t1_invalid_channels = ~T1_ALL_CHANNELS_BITMAP;

			if(wanif_conf->active_ch & (t1_invalid_channels)){
				_INFO_CFG("Group %d - Invalid channels in Channel BitMap (0x%08X)!\n", i + 1,
					wanif_conf->active_ch & (t1_invalid_channels));
				rc = 2; break;
			}
		}

		//check channels are NOT in use already by another Group
		if(wanif_conf->active_ch & cumulutive_active_ch){
			_INFO_CFG("Group %d - one or more \"T1/E1 channels\" already used by another group!\n",	i + 1);
			rc = 3; break;
		}

		//update cumulative channels for the next iteration
		cumulutive_active_ch |= wanif_conf->active_ch;

		tmp_active_ch = wanif_conf->active_ch;
		if(sdla_fe_cfg->media == WAN_MEDIA_E1 && sdla_fe_cfg->frame != WAN_FR_UNFRAMED){
			//do not count the bit for channel 0! (valid only for Unframed E1)
			tmp_active_ch &= (~0x1);
		}

		if(check_mtu(wanif_conf->mtu, wanif_conf->hdlc_streaming, tmp_active_ch)){
			rc = 4; break;
		}
	}//for()

	INFO_CFG("Driver Configration structure check - %s.\n", (rc == 0 ? "OK":"FAILED"));
	return rc;
}

/*!
  \brief Brief description
 *
 */
int sangoma_port_configurator::print_port_cfg_structure(port_cfg_t *port_cfg)
{
	wandev_conf_t		*wandev_conf = &port_cfg->wandev_conf;
	sdla_fe_cfg_t		*sdla_fe_cfg = &wandev_conf->fe_cfg;
	wanif_conf_t		*wanif_conf = &port_cfg->if_cfg[0];
	unsigned int		i;

	DBG_CFG("%s()\n", __FUNCTION__);

	_INFO_CFG("\n================================================\n");

	INFO_CFG("Card Type\t: %s(%d)\n", SDLA_DECODE_CARDTYPE(wandev_conf->card_type),
		wandev_conf->card_type);/* Sangoma Card type - S514, S518 or AFT.*/

	INFO_CFG("Number of TimeSlot Groups: %d\n", port_cfg->num_of_ifs);

	INFO_CFG("MTU\t\t: %d\n",	wandev_conf->mtu);

	print_sdla_fe_cfg_t_structure(sdla_fe_cfg);

	for(i = 0; i < port_cfg->num_of_ifs; i++){
		_INFO_CFG("\n************************************************\n");
		_INFO_CFG("Configration of Group Number %d:\n", i);
		print_wanif_conf_t_structure(&wanif_conf[i]);
	}
	return 0;
}

/*!
  \brief Brief description
 *
 */
int sangoma_port_configurator::print_sdla_fe_cfg_t_structure(sdla_fe_cfg_t *sdla_fe_cfg)
{
	_INFO_CFG("\n################################################\n");
	INFO_CFG("MEDIA\t\t: %s\n", MEDIA_DECODE(sdla_fe_cfg));
	if(FE_MEDIA(sdla_fe_cfg) == WAN_MEDIA_T1 || FE_MEDIA(sdla_fe_cfg) == WAN_MEDIA_E1){
		INFO_CFG("Line CODE\t: %s\n", LCODE_DECODE(sdla_fe_cfg));
		INFO_CFG("Framing\t\t: %s\n", FRAME_DECODE(sdla_fe_cfg));
		INFO_CFG("Clock Mode\t: %s\n", TECLK_DECODE(sdla_fe_cfg));
		INFO_CFG("Clock Reference port: %d (0 - not used)\n", FE_REFCLK(sdla_fe_cfg));
		INFO_CFG("Signalling Insertion Mode: %s\n", TE1SIG_DECODE(sdla_fe_cfg));

		INFO_CFG("High Impedance Mode: %s\n",
			(FE_HIMPEDANCE_MODE(sdla_fe_cfg) == WANOPT_YES ? "Yes":"No"));
		INFO_CFG("FE_RX_SLEVEL: %d\n", FE_RX_SLEVEL(sdla_fe_cfg));
	}//if()
	INFO_CFG("TDMV LAW\t: %s\n", (FE_TDMV_LAW(sdla_fe_cfg) == WAN_TDMV_MULAW ?"MuLaw":"ALaw"));
	INFO_CFG("TDMV SYNC\t: %d\n", FE_NETWORK_SYNC(sdla_fe_cfg));

	switch(FE_MEDIA(sdla_fe_cfg))
	{
	case WAN_MEDIA_T1:
		INFO_CFG("LBO\t\t: %s\n", LBO_DECODE(sdla_fe_cfg));
		break;
	case WAN_MEDIA_E1:
		break;
	default:
		break;
	}
	return 0;
}


/*!
  \brief Brief port description
 *
 */

int sangoma_port_configurator::print_wanif_conf_t_structure(wanif_conf_t *wanif_conf)
{
	INFO_CFG("Operation Mode\t: %s\n", wanif_conf->usedby);
	INFO_CFG("Timeslot BitMap\t: 0x%08X\n", wanif_conf->active_ch);
	INFO_CFG("Line Mode\t: %s\n",
				(wanif_conf->hdlc_streaming == WANOPT_YES ? "HDLC":"BitStream"));
	INFO_CFG("MTU\\MRU\t\t: %d\n", wanif_conf->mtu);

	return 0;
}


/*!
  \brief Brief port description
 *
 */
int sangoma_port_configurator::set_default_configuration(port_cfg_t *port_cfg)
{
	return set_volatile_configration(port_cfg);
}


/*!
  \brief Brief port description
 *
 */

int sangoma_port_configurator::set_volatile_configration(port_cfg_t *port_cfg)
{
	int rc;

	DBG_CFG("%s()\n", __FUNCTION__);

	//////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
	if(check_port_cfg_structure(port_cfg)){
		ERR_CFG("Error(s) found in 'port_cfg_t' structure!\n");
		return 1;
	}
#endif

	rc = SetPortVolatileConfigurationCommand(port_cfg);
	if(rc){
		//configration failed
		INFO_CFG("%s(): Line: %d: return code: %s (%d)\n", __FUNCTION__, __LINE__, SDLA_DECODE_SANG_STATUS(rc), rc);
		return 2;
	}

	INFO_CFG("%s(): return code: %s (%d)\n", __FUNCTION__,
		SDLA_DECODE_SANG_STATUS(port_cfg->operation_status), port_cfg->operation_status);
	switch(port_cfg->operation_status)
	{
	case SANG_STATUS_DEVICE_BUSY:
		INFO_CFG("Error: open handles exist for 'wanpipe%d_if\?\?' interfaces!\n",	wp_number);
		return port_cfg->operation_status;
	case SANG_STATUS_SUCCESS:
		//OK
		break;
	default:
		//error
		return 5;
	}

	return 0;
}

/* optional optimization - single interrupt for both Rx and Tx audio streams */
#define SPAN_TX_ONLY_IRQ 0

/*!
  \brief Example initialization for T1
 *
 */

int sangoma_port_configurator::initialize_t1_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span)
{
    wandev_conf_t	*wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t	*sdla_fe_cfg = &wandev_conf->fe_cfg;
	//sdla_te_cfg_t	*te_cfg = &sdla_fe_cfg->cfg.te_cfg;
    wan_tdmv_conf_t *tdmv_cfg = &wandev_conf->tdmv_conf;
    wanif_conf_t	*wanif_cfg = &port_cfg->if_cfg[0];
	wan_xilinx_conf_t 	*aft_cfg = &wandev_conf->u.aft;

    // T1 parameters
    FE_MEDIA(sdla_fe_cfg) = WAN_MEDIA_T1;
    FE_LCODE(sdla_fe_cfg) = WAN_LCODE_B8ZS;
    FE_FRAME(sdla_fe_cfg) = WAN_FR_ESF;
	FE_LINENO(sdla_fe_cfg) =  hardware_info->port_number;
	FE_TDMV_LAW(sdla_fe_cfg) = WAN_TDMV_MULAW;
	FE_NETWORK_SYNC(sdla_fe_cfg) = 0;

	FE_LBO(sdla_fe_cfg) = WAN_T1_0_110;
	FE_REFCLK(sdla_fe_cfg) = 0;
	FE_HIMPEDANCE_MODE(sdla_fe_cfg) = 0;
	FE_SIG_MODE(sdla_fe_cfg) = WAN_TE1_SIG_CCS;
	FE_RX_SLEVEL(sdla_fe_cfg) = WAN_TE1_RX_SLEVEL_12_DB;

#if 1
 	FE_CLK(sdla_fe_cfg) = WAN_NORMAL_CLK;
#else
	/* Use Master clock with a loopback cable or as Telco simulator. */
	FE_CLK(sdla_fe_cfg) = WAN_MASTER_CLK;
#endif

	// API parameters
    port_cfg->num_of_ifs = 1;

    wandev_conf->config_id = WANCONFIG_AFT_TE1;
    wandev_conf->magic = ROUTER_MAGIC;
	
    wandev_conf->mtu = 2048;
	wandev_conf->PCI_slot_no = hardware_info->pci_slot_number;
	wandev_conf->pci_bus_no = hardware_info->pci_bus_number;
    wandev_conf->card_type = WANOPT_AFT; //m_DeviceInfoData.card_model;

#if SPAN_TX_ONLY_IRQ
	aft_cfg->span_tx_only_irq = 1;
#else
	aft_cfg->span_tx_only_irq = 0;
#endif

	wanif_cfg->magic = ROUTER_MAGIC;
    wanif_cfg->active_ch = 0x00FFFFFF;//channels 1-24 (starting from bit zero)
    sprintf(wanif_cfg->usedby,"TDM_SPAN_VOICE_API");
    wanif_cfg->u.aft.idle_flag=0xFF;
    wanif_cfg->mtu = 160;
    wanif_cfg->u.aft.mtu = 160;
	wanif_cfg->u.aft.mru = 160;
	sprintf(wanif_cfg->name,"w%dg1",span);

    if (hardware_info->max_hw_ec_chans) {
		/* wan_hwec_conf_t - HWEC configuration for Port */
		/*wandev_conf->hwec_conf.dtmf = 1;*/
		/* wan_hwec_if_conf_t - HWEC configuration for Interface */
		wanif_cfg->hwec.enable = 1;
	}

    tdmv_cfg->span_no = (unsigned char)span;

	/* DCHAN Configuration */
    switch(FE_MEDIA(sdla_fe_cfg))
    {
	case WAN_MEDIA_T1:
		tdmv_cfg->dchan = (1<<23);/* Channel 24. This is a bitmap, not a channel number. */
        break;
	default:
		printf("%s(): Error: invalid media type!\n", __FUNCTION__);
		return 1;
    }

	printf("T1: tdmv_cfg->dchan bitmap: 0x%X\n", tdmv_cfg->dchan);

	return 0;
}

#define BUILD_FOR_PRI 1
int sangoma_port_configurator::initialize_e1_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span)
{
    wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
    wan_tdmv_conf_t  *tdmv_cfg = &wandev_conf->tdmv_conf;
    wanif_conf_t     *wanif_cfg = &port_cfg->if_cfg[0];
	wan_xilinx_conf_t 	*aft_cfg = &wandev_conf->u.aft;

	// E1 parameters
    FE_MEDIA(sdla_fe_cfg) = WAN_MEDIA_E1;
    FE_LCODE(sdla_fe_cfg) = WAN_LCODE_HDB3;
    FE_FRAME(sdla_fe_cfg) = WAN_FR_CRC4;
	FE_LINENO(sdla_fe_cfg) = hardware_info->port_number;
	FE_TDMV_LAW(sdla_fe_cfg) = WAN_TDMV_ALAW;
	FE_NETWORK_SYNC(sdla_fe_cfg) = 0;

	FE_LBO(sdla_fe_cfg) = WAN_E1_120;
	FE_REFCLK(sdla_fe_cfg) = 0;
	FE_HIMPEDANCE_MODE(sdla_fe_cfg) = 0;
	FE_SIG_MODE(sdla_fe_cfg) = WAN_TE1_SIG_CCS;
	FE_RX_SLEVEL(sdla_fe_cfg) = WAN_TE1_RX_SLEVEL_12_DB;

#if 1
 	FE_CLK(sdla_fe_cfg) = WAN_NORMAL_CLK;
#else
	/* Use Master clock with a loopback cable or as Telco simulator. */
	FE_CLK(sdla_fe_cfg) = WAN_MASTER_CLK;
#endif

	// API parameters
    port_cfg->num_of_ifs = 1;

    wandev_conf->config_id = WANCONFIG_AFT_TE1;
    wandev_conf->magic = ROUTER_MAGIC;
	
    wandev_conf->mtu = 2048;
	wandev_conf->PCI_slot_no = hardware_info->pci_slot_number;
	wandev_conf->pci_bus_no = hardware_info->pci_bus_number;
    wandev_conf->card_type = WANOPT_AFT; //m_DeviceInfoData.card_model;

#if SPAN_TX_ONLY_IRQ
	aft_cfg->span_tx_only_irq = 1;
#else
	aft_cfg->span_tx_only_irq = 0;
#endif

	wanif_cfg->magic = ROUTER_MAGIC;
#if BUILD_FOR_PRI
	//PRI Signaling on chan 16
    wanif_cfg->active_ch = 0x7FFFFFFF;// channels 1-31 (starting from bit zero)
#else
	//CAS 
    wanif_cfg->active_ch = 0xFFFF7FFF;// channels 1-15.17-31 -> Active Ch Map :0xFFFEFFFE (in message log)
#endif

    sprintf(wanif_cfg->usedby,"TDM_SPAN_VOICE_API");
    wanif_cfg->u.aft.idle_flag=0xFF;
    wanif_cfg->mtu = 160;
    wanif_cfg->u.aft.mtu = 160;
	wanif_cfg->u.aft.mru = 160;
	sprintf(wanif_cfg->name,"w%dg1",span);

    if (hardware_info->max_hw_ec_chans) {
		/* wan_hwec_conf_t - HWEC configuration for Port */
		/*wandev_conf->hwec_conf.dtmf = 1;*/
		/* wan_hwec_if_conf_t - HWEC configuration for Interface */
		wanif_cfg->hwec.enable = 1;
	}

    tdmv_cfg->span_no = (unsigned char)span;

#if BUILD_FOR_PRI
	/* DCHAN Configuration */
    switch(FE_MEDIA(sdla_fe_cfg))
    {
	case WAN_MEDIA_E1:
		tdmv_cfg->dchan = (1<<15);/* Channel 16. This is a bitmap, not a channel number. */
        break;
	default:
		printf("%s(): Error: invalid media type!\n", __FUNCTION__);
		return 1;
    }
#endif

	printf("E1: tdmv_cfg->dchan bitmap: 0x%X\n", tdmv_cfg->dchan);
	return 0;
}

int sangoma_port_configurator::initialize_bri_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span)
{
    wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
    wan_tdmv_conf_t  *tdmv_cfg = &wandev_conf->tdmv_conf;
    wanif_conf_t     *wanif_cfg = &port_cfg->if_cfg[0];

	// BRI parameters
    FE_MEDIA(sdla_fe_cfg) = WAN_MEDIA_BRI;
	FE_LINENO(sdla_fe_cfg) = hardware_info->port_number;
	FE_TDMV_LAW(sdla_fe_cfg) = WAN_TDMV_ALAW;
	FE_NETWORK_SYNC(sdla_fe_cfg) = 0;

	FE_REFCLK(sdla_fe_cfg) = 0;

#if 0
	/* 
	 * TE Module: WAN_NORMAL_CLK is the default. 
	 * Clock recovered from the line will be used by this module and
	 * will be routed to ALL other BRI modules on the card which
	 * become "connected" after this module. That means at any
	 * time there is a single BRI line where the clock is recovered from
	 * and this clock is used for all other BRI lines.
	 *
	 * NT Module: always runs on internal oscillator clock.
	 */
	BRI_FE_CLK((*sdla_fe_cfg)) = WAN_NORMAL_CLK;
#else
	/*
	 * TE Module: if WAN_MASTER_CLK, clock recovered from
	 * the line will be NOT be used by this module and
	 * will NOT be routed to ALL other BRI modules on the card.
	 * Instead, the module will use clock from internal oscillator.
	 *
	 * NT Module: WAN_MASTER_CLK will be ignored because NT should not
	 * recover clock from the line, it always runs on clock from
	 * internal oscillator.
	 */
	BRI_FE_CLK((*sdla_fe_cfg)) = WAN_MASTER_CLK;
#endif

	/*	Notes on BRI FE clock option:
		The clock option is checked by the driver only
		when TE becomes "Connected"/"Disconnected".
		No check is done on Port start-up, it is always Master during start-up.

		If TE becomes "Connected" and it's clock mode is Master,
		in the wanpipelog the following message will appear from TE:
		BRI Module: 3 connected!

		If TE becomes "Connected" and it's clock mode is Normal,
		in the wanpipelog the following messages will appear from TE:
		BRI Module: 3 connected!
		Setting Master Clock!
		TE Clock line recovery Module=3 Port=1: Enabled
		Module=3 Port=1: using 512khz from PLL
	*/

    port_cfg->num_of_ifs = 1;

    wandev_conf->config_id = WANCONFIG_AFT_ISDN_BRI;
    wandev_conf->magic = ROUTER_MAGIC;
	
    wandev_conf->mtu = 2048;
	wandev_conf->PCI_slot_no = hardware_info->pci_slot_number;
	wandev_conf->pci_bus_no = hardware_info->pci_bus_number;
    wandev_conf->card_type = WANOPT_AFT; //m_DeviceInfoData.card_model;

	wanif_cfg->magic = ROUTER_MAGIC;
    wanif_cfg->active_ch = 0xFFFFFFFF;
    sprintf(wanif_cfg->usedby,"TDM_SPAN_VOICE_API");
    wanif_cfg->u.aft.idle_flag=0xFF;
    wanif_cfg->mtu = 160;
    wanif_cfg->u.aft.mtu = 160;
	wanif_cfg->u.aft.mru = 160;
	sprintf(wanif_cfg->name,"w%dg1",span);

    if (hardware_info->max_hw_ec_chans) {
		/* wan_hwec_conf_t - HWEC configuration for Port */
		/*wandev_conf->hwec_conf.dtmf = 1;*/
		/* wan_hwec_if_conf_t - HWEC configuration for Interface */
		wanif_cfg->hwec.enable = 1;
	}

    tdmv_cfg->span_no = (unsigned char)span;

	/* There is no DCHAN Configuration on BRI because it is
	 * configured automatically by the driver. */
	
	return 0;
}

int sangoma_port_configurator::initialize_gsm_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span)
{
  wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
  sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
  wan_tdmv_conf_t  *tdmv_cfg = &wandev_conf->tdmv_conf;
  wanif_conf_t     *wanif_cfg = &port_cfg->if_cfg[0];

  // BRI parameters
  FE_MEDIA(sdla_fe_cfg) = WAN_MEDIA_GSM;
  FE_LINENO(sdla_fe_cfg) = hardware_info->port_number;
  FE_TDMV_LAW(sdla_fe_cfg) = WAN_TDMV_MULAW;
  FE_NETWORK_SYNC(sdla_fe_cfg) = 0;

  FE_REFCLK(sdla_fe_cfg) = 0;

  //BRI_FE_CLK((*sdla_fe_cfg)) = WAN_MASTER_CLK;
  FE_CLK((sdla_fe_cfg)) = WAN_MASTER_CLK;

  port_cfg->num_of_ifs = 1;

  wandev_conf->config_id = WANCONFIG_AFT_GSM;
  wandev_conf->magic = ROUTER_MAGIC;

  wandev_conf->mtu = 1500;
  wandev_conf->PCI_slot_no = hardware_info->pci_slot_number;
  wandev_conf->pci_bus_no = hardware_info->pci_bus_number;
  wandev_conf->card_type = WANOPT_AFT_GSM; //m_DeviceInfoData.card_model;

  wanif_cfg->magic = ROUTER_MAGIC;
  wanif_cfg->active_ch = 0xFFFFFFFF;
  sprintf(wanif_cfg->usedby,"TDM_VOICE_API");
  wanif_cfg->u.aft.idle_flag=0xFF;
  wanif_cfg->mtu = 160;
  wanif_cfg->u.aft.mtu = 160;
  wanif_cfg->u.aft.mru = 160;
  sprintf(wanif_cfg->name,"w%dg1",span);

  if (hardware_info->max_hw_ec_chans) {
    /* wan_hwec_conf_t - HWEC configuration for Port */
    /*wandev_conf->hwec_conf.dtmf = 1;*/
    /* wan_hwec_if_conf_t - HWEC configuration for Interface */
    wanif_cfg->hwec.enable = 1;
  }

  tdmv_cfg->span_no = (unsigned char)span;
	
  tdmv_cfg->dchan = (1<<2);/* Channel 2. This is a bitmap, not a channel number. */

  return 0;
}

int sangoma_port_configurator::initialize_serial_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span)
{
    wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
    wan_tdmv_conf_t  *tdmv_cfg = &wandev_conf->tdmv_conf;
    wanif_conf_t     *wanif_cfg = &port_cfg->if_cfg[0];

	// Serial parameters
    FE_MEDIA(sdla_fe_cfg) = WAN_MEDIA_SERIAL;
	FE_LINENO(sdla_fe_cfg) = hardware_info->port_number;

    wandev_conf->line_coding = WANOPT_NRZ;
	/* WANOPT_V35/WANOPT_X21 are valid for card models:
	 * AFT_ADPTR_2SERIAL_V35X21 and AFT_ADPTR_4SERIAL_V35X21.
	 * WANOPT_RS232 valid for card models:
	 * AFT_ADPTR_2SERIAL_RS232 and AFT_ADPTR_4SERIAL_RS232.
	 */
    wandev_conf->electrical_interface = WANOPT_V35;
#if 1
	wandev_conf->clocking = WANOPT_EXTERNAL;
	wandev_conf->bps = 0;
#else
	wandev_conf->clocking = WANOPT_INTERNAL;
	wandev_conf->bps = 56000;
#endif

	wandev_conf->connection = WANOPT_PERMANENT;//or WANOPT_SWITCHED
	wandev_conf->line_idle = WANOPT_IDLE_FLAG;

	// API parameters
    port_cfg->num_of_ifs = 1;

    wandev_conf->config_id = WANCONFIG_AFT_SERIAL;
    wandev_conf->magic = ROUTER_MAGIC;
	
    wandev_conf->mtu = 2048;
	wandev_conf->PCI_slot_no = hardware_info->pci_slot_number;
	wandev_conf->pci_bus_no = hardware_info->pci_bus_number;
    wandev_conf->card_type = WANOPT_AFT; //m_DeviceInfoData.card_model;

	wanif_cfg->magic = ROUTER_MAGIC;
    wanif_cfg->active_ch = 0xFFFFFFFF;
    sprintf(wanif_cfg->usedby,"API");
    wanif_cfg->u.aft.idle_flag=0xFF;
    wanif_cfg->mtu = 1600;
    wanif_cfg->u.aft.mtu = 1600;
	wanif_cfg->u.aft.mru = 1600;
	sprintf(wanif_cfg->name,"w%dg1",span);

	wanif_cfg->hdlc_streaming = WANOPT_YES;

	return 0;
}

int sangoma_port_configurator::write_configration_on_persistent_storage(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span)
{
	//all the work is done by libsangoma
	return sangoma_write_port_config_on_persistent_storage(hardware_info, port_cfg, (unsigned short)span);
}

int sangoma_port_configurator::control_analog_rm_lcm(port_cfg_t *port_cfg, int control_val)
{
	wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
	if (wandev_conf->card_type == WANOPT_AFT_ANALOG) { //Only valid for Analog cards
		if (control_val == 1) {
			INFO_CFG("%s(): enabling FXO Loop Current Monitoring.\n", __FUNCTION__);
			sdla_fe_cfg->cfg.remora.rm_lcm = 1;
		} else if(control_val == 0) {
			INFO_CFG("%s(): disabling FXO Loop Current Monitoring.\n", __FUNCTION__);
			sdla_fe_cfg->cfg.remora.rm_lcm = 0;
		} else {
			ERR_CFG("invalid parameter!\n");
			return -EINVAL;
		}	
	} else {
		ERR_CFG("invalid card type: %d!\n", wandev_conf->card_type);
		return -EINVAL;
	}
	return 0;
}

	/**************************************************************/
	/*********** Analog Global Gain Setting Functions**************/
	/**************************************************************/

	//Note: following gain settings are global. It requires drivers to
	//restart the card.
	//It is recommended to use dynamic analog txgain/rxgain functions
	//from libsangoma library:sangoma_set_rm_tx_gain and sangoma_set_rm_rx_gain

int sangoma_port_configurator::set_analog_rm_fxo_txgain(port_cfg_t *port_cfg, int txgain)
{
	wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
	
	if(wandev_conf->card_type == WANOPT_AFT_ANALOG){//Only valid for Analog cards
			sdla_fe_cfg->cfg.remora.fxo_txgain = txgain;
	} else{
			return -EINVAL;
	}
	return 0;
}
int sangoma_port_configurator::set_analog_rm_fxo_rxgain(port_cfg_t *port_cfg, int rxgain)
{
	wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
	
	if(wandev_conf->card_type == WANOPT_AFT_ANALOG){//Only valid for Analog cards
			sdla_fe_cfg->cfg.remora.fxo_rxgain = rxgain;
	} else{
			return -EINVAL;
	}
	return 0;
}

int sangoma_port_configurator::set_analog_rm_fxs_txgain(port_cfg_t *port_cfg, int txgain)
{
	wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
	
	if(wandev_conf->card_type == WANOPT_AFT_ANALOG){//Only valid for Analog cards
			sdla_fe_cfg->cfg.remora.fxs_txgain = txgain;
	} else{
			return -EINVAL;
	}
	return 0;
}

int sangoma_port_configurator::set_analog_rm_fxs_rxgain(port_cfg_t *port_cfg, int rxgain)
{
	wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
	
	if(wandev_conf->card_type == WANOPT_AFT_ANALOG){//Only valid for Analog cards
			sdla_fe_cfg->cfg.remora.fxs_rxgain = rxgain;
	} else{
			return -EINVAL;
	}
	return 0;
}

	/**************************************************************/
	/*********** End of Analog Global Gain Setting Functions ******/
	/**************************************************************/


int sangoma_port_configurator::set_analog_opermode(port_cfg_t *port_cfg, char *opermode)
{
	wandev_conf_t    *wandev_conf = &port_cfg->wandev_conf;
    sdla_fe_cfg_t    *sdla_fe_cfg = &wandev_conf->fe_cfg;
	if(wandev_conf->card_type == WANOPT_AFT_ANALOG){ //Only valid for Analog cards
			if(sizeof(sdla_fe_cfg->cfg.remora.opermode_name) < strlen(opermode))
				return -EINVAL;
			strcpy(sdla_fe_cfg->cfg.remora.opermode_name,opermode);                                                                                              
	} else{
			return -EINVAL;
	}
	return 0;
}

void sangoma_port_configurator::initialize_interface_mtu_mru(port_cfg_t *port_cfg, unsigned int mtu, unsigned int mru)
{
    wanif_conf_t	*wanif_cfg = &port_cfg->if_cfg[0];

	INFO_CFG("%s(): new MTU: %d, new MRU: %d.\n", __FUNCTION__, mtu, mru);

    wanif_cfg->mtu = mtu;
    wanif_cfg->u.aft.mtu = mtu;
	wanif_cfg->u.aft.mru = mru;
}
