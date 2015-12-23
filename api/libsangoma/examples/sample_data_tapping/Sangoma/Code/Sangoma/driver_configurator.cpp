//////////////////////////////////////////////////////////////////////
// driver_configurator.cpp: implementation of the driver_configurator class.
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//////////////////////////////////////////////////////////////////////

#if defined WIN32 && defined NDEBUG
#pragma optimize("gt",on)
#endif

#include "driver_configurator.h"

#define DBG_CFG		if(0)printf("DRVCFG:");if(0)printf
#define _DBG_CFG	if(0)printf

#define INFO_CFG	if(0)printf("DRVCFG:");if(0)printf
#define _INFO_CFG	if(0)printf

#define ERR_CFG		if(0)printf("Error:%s():line:%d: ", __FUNCTION__, __LINE__);if(0)printf
#define _ERR_CFG	if(0)printf


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

driver_configurator::driver_configurator()
{
	DBG_CFG("%s()\n", __FUNCTION__);

	wp_handle = INVALID_HANDLE_VALUE;
}

driver_configurator::~driver_configurator()
{
	DBG_CFG("%s()\n", __FUNCTION__);

	cleanup();
}

void driver_configurator::cleanup()
{
	DBG_CFG("%s()\n", __FUNCTION__);

	if(wp_handle != INVALID_HANDLE_VALUE){
		sangoma_close(&wp_handle);
		wp_handle = INVALID_HANDLE_VALUE;
	}
}

//GET current configuraion of the API driver
int driver_configurator::get_configuration(port_cfg_t *out_port_cfg)
{
	DBG_CFG("%s()\n", __FUNCTION__);

	int err = sangoma_driver_port_get_config(wp_handle, out_port_cfg, wp_number);
	if (err) {
		return 1;
	}

	return 0;
}

//SET new configuration of the API driver
int driver_configurator::set_configuration(port_cfg_t *port_cfg)
{
	port_management_struct_t port_mgmnt;
	int err;

	DBG_CFG("%s()\n", __FUNCTION__);

	////////////////////////////////////////////////////////////////////////////////////
	memset(&port_mgmnt, 0x00, sizeof(port_mgmnt));
	port_mgmnt.operation_status = SANG_STATUS_GENERAL_ERROR;

	err = sangoma_driver_port_stop(wp_handle, &port_mgmnt, wp_number);
	if (err) {
		ERR_CFG("%s: failed to stop Port for re-configuration!\n", wanpipe_name_str);
		return 1;
	}

	switch(port_mgmnt.operation_status)
	{
	case SANG_STATUS_SUCCESS:
	case SANG_STATUS_CAN_NOT_STOP_DEVICE_WHEN_ALREADY_STOPPED:
		//OK
		break;
	default:
		ERR_CFG("%s: %s(): return code: %s\n", wanpipe_name_str,
			__FUNCTION__, SDLA_DECODE_SANG_STATUS(port_mgmnt.operation_status));
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////////////
	err = sangoma_driver_port_set_config(wp_handle, port_cfg, wp_number);
	if (err) {
		ERR_CFG("%s: failed to set Port configuration!\n", wanpipe_name_str);
		return 1;
	}

	DBG_CFG("%s(): return code: %s (%d)\n", __FUNCTION__,
		SDLA_DECODE_SANG_STATUS(port_cfg->operation_status), port_cfg->operation_status);
	switch(port_cfg->operation_status)
	{
	case SANG_STATUS_DEVICE_BUSY:
		ERR_CFG("Error: open handles exist for '%s_IF\?\?' interfaces!\n",	wanpipe_name_str);
		return port_cfg->operation_status;
	case SANG_STATUS_SUCCESS:
		//OK
		break;
	default:
		//error
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////////////
	memset(&port_mgmnt, 0x00, sizeof(port_mgmnt));
	port_mgmnt.operation_status = SANG_STATUS_GENERAL_ERROR;

	err=sangoma_driver_port_start(wp_handle, &port_mgmnt, wp_number);
	if(err || port_mgmnt.operation_status){
		ERR_CFG("%s: failed to start Port after re-configuration!\n", wanpipe_name_str);
		return 1;
	}

	return 0;
}


int driver_configurator::init(unsigned int wanpipe_number, unsigned int port_number)
{
	DBG_CFG("%s():\n", __FUNCTION__);
	
	memset(&port_cfg, 0x00, sizeof(port_cfg_t));
	port_cfg.num_of_ifs = 1;//1 Group always

	//this the MAPPED port number
	wp_number = wanpipe_number + port_number + 1;

	wp_snprintf(wanpipe_name_str, sizeof(wanpipe_name_str), "wanpipe%d", wp_number);
	DBG_CFG("%s\n", wanpipe_name_str);

	wp_handle = sangoma_open_driver_ctrl(wp_number);

	if(wp_handle == INVALID_HANDLE_VALUE){
//		ERR_CFG("Error: failed to open %s!!\n", wanpipe_name_str);
		return 1;
	}

	return 0;
}

int driver_configurator::print_port_cfg_structure(port_cfg_t *port_cfg)
{
	wandev_conf_t		*wandev_conf = &port_cfg->wandev_conf;
	sdla_fe_cfg_t		*sdla_fe_cfg = &wandev_conf->fe_cfg;
	wanif_conf_t		*wanif_conf = &port_cfg->if_cfg[0];

	DBG_CFG("%s()\n", __FUNCTION__);

	_INFO_CFG("\n================================================\n");

	INFO_CFG("Card Type\t: %s(%d)\n", SDLA_DECODE_CARDTYPE(wandev_conf->card_type),
		wandev_conf->card_type);/* Sangoma Card type - S514, S518 or AFT.*/

	INFO_CFG("Number of TimeSlot Groups: %d\n", port_cfg->num_of_ifs);

	INFO_CFG("MTU\t\t: %d\n",	wandev_conf->mtu);

	print_sdla_fe_cfg_t_structure(sdla_fe_cfg);

	for(unsigned int i = 0; i < port_cfg->num_of_ifs; i++){
		_INFO_CFG("\n************************************************\n");
		_INFO_CFG("Configration of Group Number %d:\n", i);
		print_wanif_conf_t_structure(&wanif_conf[i]);
	}
	return 0;
}

int driver_configurator::print_sdla_fe_cfg_t_structure(sdla_fe_cfg_t *sdla_fe_cfg)
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

int driver_configurator::print_wanif_conf_t_structure(wanif_conf_t *wanif_conf)
{
	INFO_CFG("Operation Mode\t: %s\n", wanif_conf->usedby);
	INFO_CFG("Timeslot BitMap\t: 0x%08X\n", wanif_conf->active_ch);
	INFO_CFG("Line Mode\t: %s\n",
				(wanif_conf->hdlc_streaming == WANOPT_YES ? "HDLC":"BitStream"));
	INFO_CFG("MTU\\MRU\t\t: %d\n", wanif_conf->mtu);

	return 0;
}

int driver_configurator::get_hardware_info(hardware_info_t *hardware_info)
{
	DBG_CFG("%s()\n", __FUNCTION__);

	port_management_struct_t port_management;

	int err=sangoma_driver_get_hw_info(wp_handle,&port_management, wp_number);

	if (err) {
		ERR_CFG("Error: failed to get hw info for wanpipe%d!\n", wp_number);
		return 1;
	}

	memcpy(hardware_info,port_management.data,sizeof(hardware_info_t));

	return 0;
}


int driver_configurator::get_driver_version(wan_driver_version_t *version)
{
	DBG_CFG("%s()\n", __FUNCTION__);

        port_management_struct_t port_management;

	int err = sangoma_driver_get_version(wp_handle, &port_management, wp_number);

	if (err) {
        	ERR_CFG("Error: failed to get Driver Version from wanpipe%d!\n", wp_number);
                return 1;
        }

	memcpy(version, port_management.data, sizeof(*version));

	return 0;
}


int driver_configurator::push_a_card_into_wanpipe_info_array(
	wanpipe_instance_info_t *wanpipe_info_array,
	hardware_info_t *new_hw_info)
{
	unsigned int card_ind;
	hardware_info_t *tmp_hw_info;

	//check this card is not in the array already
	for(card_ind = 0; card_ind < MAX_CARDS; card_ind++){
		tmp_hw_info = &wanpipe_info_array[card_ind].hardware_info;

		if(	(new_hw_info->pci_bus_number == tmp_hw_info->pci_bus_number) &&
			(new_hw_info->pci_slot_number == tmp_hw_info->pci_slot_number)){
			//already in array. not an error.
			DBG_CFG("already in array.\n");
			return 1;
		}
	}

	//find an empty slot in the array and place the new card there
	for(card_ind = 0; card_ind < MAX_CARDS; card_ind++){
		tmp_hw_info = &wanpipe_info_array[card_ind].hardware_info;
		if(tmp_hw_info->card_model == -1){
			//found an empty slot in array
			memcpy(tmp_hw_info, new_hw_info, sizeof(hardware_info_t));
			return 0;
		}
	}

	//too many cards in the computer.
	INFO_CFG("Warning: number of cards in the computer is greater than the maximum of %d.\n",
		MAX_CARDS);
	return 2;
}

//
//Scan all running wanpipes, check which ones are on the same card,
//figure out how many physical cards installed.
//
//Returns:	-1 - if error.
//			number of cards, starting from zero.
int driver_configurator::scan_for_sangoma_cards(wanpipe_instance_info_t *wanpipe_info_array, int card_model)
{
	int card_counter = 0, wp_ind;
	hardware_info_t	hardware_info;

	//initialize hardware information array.
	memset(wanpipe_info_array, -1, sizeof(wanpipe_instance_info_t)*MAX_CARDS);

	DBG_CFG("%s(): wanpipe_info_array: 0x%p\n", __FUNCTION__, wanpipe_info_array);

	for(wp_ind = 0; wp_ind < (MAX_CARDS	* MAX_PORTS_PER_CARD); wp_ind++){
		if(init(wp_ind, 0)){
			continue;
		}
		if(get_hardware_info(&hardware_info)){
			//Port was opened but could not read hw info - dont add the port to the list
			//and also consider there are no more ports, so exit the loop.
			break;
		}

		DBG_CFG("card_model		: %s (0x%08X)\n",
			SDLA_ADPTR_NAME(hardware_info.card_model), hardware_info.card_model);
		DBG_CFG("firmware_version\t: 0x%02X\n", hardware_info.firmware_version);
		DBG_CFG("pci_bus_number\t\t: %d\n", hardware_info.pci_bus_number);
		DBG_CFG("pci_slot_number\t\t: %d\n", hardware_info.pci_slot_number);

		if(card_model == hardware_info.card_model){
			if(push_a_card_into_wanpipe_info_array(wanpipe_info_array, &hardware_info) == 0){
				//At this point we know: 1. WANPIPE number, for example WANPIPE1.
				//						 2. Card type, for example A104.
				//It allows to figure out Device Name of each port on the card.
				//For example: for A104 it will be WANPIPE1, WANPIPE2, WANPIPE3, WANPIPE4
				wanpipe_info_array[card_counter].wanpipe_number = wp_ind;
				DBG_CFG("%s(): wp_ind: %u\n", __FUNCTION__, wp_ind);
				card_counter++;
			}
		}else{
			DBG_CFG("Found a Card but Card Model is different from what was searched for.\n");
		}

		//it is important to Close each port which was opened during the scan.
		cleanup();
	}//for()

	return card_counter;
}

int driver_configurator::set_dchan(int dchan, int seven_bit, int mtp1_filter)
{
	wandev_conf_t	*wandev_conf	= &port_cfg.wandev_conf;
    wanif_conf_t	*wanif_cfg		= &port_cfg.if_cfg[0];
	
	if(!dchan) {
		return -1;
	}
	
	if (seven_bit) {	
		wanif_cfg->u.aft.sw_hdlc=1;
		wanif_cfg->u.aft.seven_bit_hdlc=1;
	}
	if (mtp1_filter) {
		wanif_cfg->u.aft.sw_hdlc=1;
		wanif_cfg->u.aft.mtp1_filter=1;
		wanif_cfg->u.aft.hdlc_repeat=1;
	}

	dchan--;
	wandev_conf->tdmv_conf.dchan=(1<<dchan);

	return 0; 
}

int driver_configurator::set_chunk_ms(int chunk_ms)
{
    wanif_conf_t	*wanif_cfg		= &port_cfg.if_cfg[0];

	wanif_cfg->u.aft.mru = chunk_ms*8;
	wanif_cfg->u.aft.mtu = chunk_ms*8;

	return 0; 
}

int driver_configurator::set_data_api_mode(void)
{
	wandev_conf_t	*wandev_conf	= &port_cfg.wandev_conf;
    wanif_conf_t	*wanif_cfg		= &port_cfg.if_cfg[0];
	sprintf(wanif_cfg->usedby, "DATA_API");

	if (wandev_conf->tdmv_conf.dchan) {
		wanif_cfg->hdlc_streaming=1;
		wanif_cfg->active_ch=wandev_conf->tdmv_conf.dchan;
	}
	return 0; 
}
int driver_configurator::set_chan_api_mode(void)
{
    wanif_conf_t	*wanif_cfg		= &port_cfg.if_cfg[0];
	sprintf(wanif_cfg->usedby, SDLA_DECODE_USEDBY_FIELD(TDM_CHAN_VOICE_API));
	return 0; 
}
int driver_configurator::set_span_api_mode(void)
{
    wanif_conf_t	*wanif_cfg		= &port_cfg.if_cfg[0];
	sprintf(wanif_cfg->usedby, SDLA_DECODE_USEDBY_FIELD(TDM_SPAN_VOICE_API));
	return 0; 
}

//function to switch between T1 and E1
int driver_configurator::set_t1_e1_configuration(sdla_fe_cfg_t *in_sdla_fe_cfg, buffer_settings_t *buffer_settings)
{
	wandev_conf_t	*wandev_conf	= &port_cfg.wandev_conf;
	sdla_fe_cfg_t 	*sdla_fe_cfg	= &wandev_conf->fe_cfg;
    wan_tdmv_conf_t	*tdmv_cfg		= &wandev_conf->tdmv_conf;
    wanif_conf_t	*wanif_cfg		= &port_cfg.if_cfg[0];
	hardware_info_t tmp_hardware_info;

	DBG_CFG("%s()\n", __FUNCTION__);

	get_hardware_info(&tmp_hardware_info);

	//copy T1/E1 configuration into the Driver configuration structure.
	memcpy(sdla_fe_cfg, in_sdla_fe_cfg, sizeof(sdla_fe_cfg_t));

	port_cfg.buffer_settings.buffer_multiplier_factor = buffer_settings->buffer_multiplier_factor;
	port_cfg.buffer_settings.number_of_buffers_per_api_interface = buffer_settings->number_of_buffers_per_api_interface;

	wandev_conf->fe_cfg.cfg.te_cfg.active_ch = 0xFFFFFFFF;//a constant

	///////////////////////////////////////////////////////////////////////////////
    wandev_conf->config_id = WANCONFIG_AFT_TE1;
    wandev_conf->magic = ROUTER_MAGIC;

    wandev_conf->mtu = 2048;

	DBG_CFG("%s(): slot: %d, bus: %d, port_number:%d\n", __FUNCTION__,
		tmp_hardware_info.pci_slot_number, tmp_hardware_info.pci_bus_number,
		tmp_hardware_info.port_number);

	wandev_conf->PCI_slot_no = tmp_hardware_info.pci_slot_number;
	wandev_conf->pci_bus_no = tmp_hardware_info.pci_bus_number;
    wandev_conf->card_type = WANOPT_AFT; //m_DeviceInfoData.card_model;

	wanif_cfg->magic = ROUTER_MAGIC;

    if (wandev_conf->tdmv_conf.dchan && FE_FRAME(sdla_fe_cfg) == WAN_FR_D4) {
		wanif_cfg->u.aft.sw_hdlc=1;
		wanif_cfg->u.aft.seven_bit_hdlc=1;
	} 

    wanif_cfg->u.aft.idle_flag=0xFF;

	sprintf(wanif_cfg->name, "w%dg1", wp_number);
	///////////////////////////////////////////////////////////////////////////////

	port_cfg.port_no = FE_LINENO(sdla_fe_cfg) = tmp_hardware_info.port_number;

	//
	//Set MTU/MRU per-TIMESLOT - the API driver will multiply this value by the
	//number of timeslots this interface is using - t1:24, e1:31 or 32.
	//The 'buffer_multiplier_factor' value will control how many of these
	//buffers will be received on each RX indication.
	//
    if (!wanif_cfg->u.aft.mru) {
		if(FE_MEDIA(sdla_fe_cfg) == WAN_MEDIA_T1){
			//320*24=7680
			wanif_cfg->mtu = wanif_cfg->u.aft.mtu = wanif_cfg->u.aft.mru = 320;
		} else {
			//240*31=7440, 240*32=7680
			wanif_cfg->mtu = wanif_cfg->u.aft.mtu = wanif_cfg->u.aft.mru = 240;
		}
	}

	if(FE_MEDIA(sdla_fe_cfg) == WAN_MEDIA_T1){
		//T1, 1 Group of 24 Timeslots.
		if (!wanif_cfg->active_ch) {
			wanif_cfg->active_ch = 0xFFFFFF;
		}

		FE_TDMV_LAW(sdla_fe_cfg) = WAN_TDMV_MULAW;

	}else if(FE_MEDIA(sdla_fe_cfg) == WAN_MEDIA_E1){
		//E1, 1 Group of 31 or 32 Timeslots.
		//API driver will automatically adjust timeslot bitmap for "framed" e1
		if (!wanif_cfg->active_ch) {
			wanif_cfg->active_ch = 0xFFFFFFFF;
		}

		FE_TDMV_LAW(sdla_fe_cfg) = WAN_TDMV_ALAW;
		FE_LBO(sdla_fe_cfg) = WAN_T1_LBO_NONE;//important to set to a valid value!!
	}else{
		ERR_CFG("%s(): Error: invalid Media Type %d!\n", __FUNCTION__, sdla_fe_cfg->media);
		return 1;
	}

    tdmv_cfg->span_no = (unsigned char)wp_number;

	print_port_cfg_structure(&port_cfg);

	return set_configuration(&port_cfg);
}
