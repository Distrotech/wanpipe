//////////////////////////////////////////////////////////////////////
// sangoma_interface.cpp: interface for Sangoma API driver.
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//////////////////////////////////////////////////////////////////////

#include "sangoma_interface.h"
#include "driver_configurator.h"

#define DBG_IFACE	if(0)printf
#define INFO_IFACE	if(0)printf
#define ERR_IFACE	printf("Error: %s(): line:%d :", __FUNCTION__, __LINE__);printf
#define WRN_IFACE	printf("Warning: %s(): line:%d :", __FUNCTION__, __LINE__);printf

#define DBG_PORT_NUM if(0)printf
#define IFACE_FUNC() if(0)printf("%s():line:%d\n", __FUNCTION__, __LINE__)


#define DO_COMMAND(wan_udp)	DoManagementCommand(sangoma_dev, &wan_udp);

sangoma_interface::sangoma_interface(int wanpipe_num, int interface_num)
{
	DBG_IFACE("%s()\n", __FUNCTION__);

	wanpipe_number = wanpipe_num;
	interface_number = interface_num;

	DBG_PORT_NUM("%s(): (1) wanpipe_number: %i, interface_number: %i\n", 
		__FUNCTION__, wanpipe_number, interface_number);

	memset(interface_name, 0x00, DEV_NAME_LEN);
	memset(&drv_version, 0x00, sizeof(drv_version));

	//////////////////////////////////////////////////////////////////
	driver_configurator drv_cfg;
	hardware_info_t hardware_info;
	card_model = A104_ADPTR_4TE1;//default
	if(drv_cfg.init(wanpipe_number, 0) == 0){
		if(drv_cfg.get_hardware_info(&hardware_info) == 0){
			DBG_IFACE("card_model		: %s (0x%08X)\n",
				SDLA_ADPTR_NAME(hardware_info.card_model), hardware_info.card_model);
			DBG_IFACE("firmware_version\t: 0x%02X\n", hardware_info.firmware_version);
			DBG_IFACE("pci_bus_number\t\t: %d\n", hardware_info.pci_bus_number);
			DBG_IFACE("pci_slot_number\t\t: %d\n", hardware_info.pci_slot_number);

			card_model = hardware_info.card_model;
		}
	} else {
		ERR_IFACE("Failed to get Card Model!\n");
		exit(1);
	}

	//G3 requred port numbers on A108 to be different from what Sangoma API provides.
	//Physical Port 0: Ports 0 and 1 (not 0 and 4)
	//Physical Port 1: Ports 2 and 3 (not 1 and 5)
	//Physical Port 2: Ports 4 and 5 (not 2 and 6)
	//Physical Port 3: Ports 6 and 7 (not 3 and 7)
	if(card_model == A108_ADPTR_8TE1){
		DBG_PORT_NUM("%s(): card_model == A108_ADPTR_8TE1\n", __FUNCTION__);
		//correct the interface_number for A108 accoringly
		switch(interface_num)
		{
		case 0:
			interface_number = 0;
			break;
		case 1:
			interface_number = 4;
			break;
		case 2:
			interface_number = 1;
			break;
		case 3:
			interface_number = 5;
			break;
		case 4:
			interface_number = 2;
			break;
		case 5:
			interface_number = 6;
			break;
		case 6:
			interface_number = 3;
			break;
		case 7:
			interface_number = 7;
			break;
		}
	}

	DBG_PORT_NUM("%s(): (2) wanpipe_number: %i, MAPPED interface_number: %i\n", 
		__FUNCTION__, wanpipe_number, interface_number);

	//Form the MAPPED Interface Name from Wanpipe Number and Interface Index (i.e. wanpipe1_if1).
	wp_snprintf(interface_name, DEV_NAME_LEN, "\\\\.\\WANPIPE%d_IF%d", 
		wanpipe_number + 1 + interface_number, 1); 
	DBG_PORT_NUM( "Using Device Name: %s\n", interface_name);

	//////////////////////////////////////////////////////////////////
	if(drv_cfg.get_driver_version(&drv_version)){
		ERR_IFACE("Failed to get Driver Version!\n");
		exit(1);
	}

	check_api_driver_version();

	//////////////////////////////////////////////////////////////////
	sangoma_dev = INVALID_HANDLE_VALUE;

	sng_wait_obj = NULL;

	//////////////////////////////////////////////////////////////////
	//receive stuff
	rx_frames_count = 0;
	rx_bytes_count = 0;
	//for counting frames with CRC/Abort errors
	bad_rx_frames_count = 0;

	//////////////////////////////////////////////////////////////////
	//transmit stuff
	tx_bytes_count = 0;
	tx_frames_count = 0;
	tx_test_byte = 0;

	//////////////////////////////////////////////////////////////////
	//IOCTL management structures and variables
	protocol_cb_size = sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+1;
	wan_protocol = 0;
	adapter_type = 0;
}

sangoma_interface::~sangoma_interface()
{
	DBG_IFACE("%s()\n", __FUNCTION__);
	cleanup();
}

HANDLE sangoma_interface::open_first_interface_of_a_mapped_port()
{
	sng_fd_t hTmp;
	int span, chan;

	span = get_mapped_span_number();//wanpipe_number + 1 + interface_number;
	chan = 1;/* wanpipe1_if1... */

	DBG_IFACE("%s(): opening Span: %d, Chan: %d\n", __FUNCTION__, span, chan);

	hTmp = sangoma_open_api_span_chan(span, chan);

	if(hTmp == INVALID_HANDLE_VALUE){
		DBG_IFACE("%s(): failed to open Span: %d, Chan: %d!\n", __FUNCTION__, span, chan);
	}

	return hTmp;
}

int sangoma_interface::init()
{
	DBG_IFACE("%s()\n", __FUNCTION__);
	DBG_IFACE("interface_name: %s, wanpipe_number:%d, interface_number:%d\n", interface_name, wanpipe_number, interface_number);

#if defined(__LINUX__)
	//under Linux must wait for interfaces to be available for opening
	sangoma_interface_wait_up(get_mapped_span_number(), 1/*chan*/, 2/* 2 second timeout */);
#endif

	////////////////////////////////////////////////////////////////////////////
	//open handle for reading and writing data, for events reception and other commands
	sangoma_dev = open_first_interface_of_a_mapped_port();
    if (sangoma_dev == INVALID_HANDLE_VALUE){
		ERR_IFACE( "Unable to open %s for Rx/Tx!\n", interface_name);
		return 1;
	}

	//FIXME: NENAD
	ERR_IFACE("Span %i - creatig an object\n",get_mapped_span_number()); 

	if(SANG_ERROR(sangoma_wait_obj_create(&sng_wait_obj, sangoma_dev, SANGOMA_DEVICE_WAIT_OBJ_SIG))){
		ERR_IFACE("Failed to create 'Tx/Rx/Event WAIT_OBJECT' for %s\n", interface_name);
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////
	//get current protocol
	if(GetWANConfig() == WAN_FALSE){
		ERR_IFACE( "Failed to get current protocol!\n");
		goto init_error;
	}

	////////////////////////////////////////////////////////////////////////////
	//get Front End Type (T1/E1/Analog...)
	if (get_fe_type(&adapter_type) == WAN_FALSE){
		ERR_IFACE( "Failed to get Front End Type!\n");
		goto init_error;
	}

	////////////////////////////////////////////////////////////////////////////
#if 0
	//For Debugging only print the interface configuration.
	if_cfg_t wanif_conf_struct;

	if(get_interface_configuration(&wanif_conf_struct)){
		ERR_IFACE( "Failed to get Interface Configuration!\n");
		return 1;
	}
#endif
	return 0;

init_error:

	if(sng_wait_obj){
		sangoma_wait_obj_delete(&sng_wait_obj);
		sng_wait_obj = NULL;
	}

	return 1;

}

//
//cleanup() - reverse everything what was done in init().
//
//It is very important to close ALL open
//handles, because the API will continue
//receive data until the LAST handel is closed.
int sangoma_interface::cleanup()
{
	DBG_IFACE("%s()\n", __FUNCTION__);

	if(sng_wait_obj){
		sangoma_wait_obj_delete(&sng_wait_obj);
		sng_wait_obj = NULL;
	}

	if(sangoma_dev != INVALID_HANDLE_VALUE){
		DBG_IFACE( "Closing Rx/Tx fd.\n");
		sangoma_close(&sangoma_dev);
		sangoma_dev = INVALID_HANDLE_VALUE;
	}

	return 0;
}

int sangoma_interface::GetWANConfig( void )
{
	int err = WAN_TRUE;

	/* Get Protocol type */ 
	wan_udp.wan_udphdr_command	= WAN_GET_PROTOCOL;
	wan_udp.wan_udphdr_data_len = 0;

	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code){
		ERR_IFACE( "Error: Command WAN_GET_PROTOCOL failed! return code: 0x%X",
			wan_udp.wan_udphdr_return_code);
		return WAN_FALSE;
	}

	wan_protocol = wan_udp.wan_udphdr_data[0];

	DBG_IFACE( "Device %s running protocol: %s\n",
		interface_name, DECODE_PROT(wan_protocol));
	 
	return err;
} 

int sangoma_interface::get_fe_type(unsigned char* adapter_type)
{
	int err = WAN_TRUE;

	/* Read Adapter Type */
	wan_udp.wan_udphdr_command = WAN_GET_MEDIA_TYPE;
	wan_udp.wan_udphdr_data[0] = WAN_MEDIA_NONE;
	wan_udp.wan_udphdr_data_len = 0;

	DO_COMMAND(wan_udp);
	if(wan_udp.wan_udphdr_return_code){
		ERR_IFACE( "Error: Command WAN_GET_MEDIA_TYPE failed! return code: 0x%X",
			wan_udp.wan_udphdr_return_code);
		return WAN_FALSE;
	}

	*adapter_type =	get_wan_udphdr_data_byte(0);

	DBG_IFACE( "Front End Type: ");
	switch(*adapter_type)
	{
	case WAN_MEDIA_NONE:
		DBG_IFACE( "Serial");
		break;

	case WAN_MEDIA_T1:
		DBG_IFACE( "T1");
		break;

	case WAN_MEDIA_E1:
		DBG_IFACE( "E1");
		break;

	case WAN_MEDIA_56K:
		DBG_IFACE( "56K");
		break;

	case WAN_MEDIA_FXOFXS:
		DBG_IFACE( "Aanalog");
		break;

	default:
		DBG_IFACE( "Unknown");
		err = WAN_FALSE;
	}
	DBG_IFACE( "\n");

	return err;
}

//return POINTER to data at offset 'off'
unsigned char* sangoma_interface::get_wan_udphdr_data_ptr(unsigned char off) 
{
	unsigned char *p_data = (unsigned char*)&wan_udp.wan_udphdr_data[0];
	p_data += off;
	return p_data;
}

unsigned char sangoma_interface::set_wan_udphdr_data_byte(unsigned char off, unsigned char data) 
{
	unsigned char *p_data = (unsigned char*)&wan_udp.wan_udphdr_data[0];
	p_data[off] = data;
	return 0;
}

//return DATA at offset 'off'
unsigned char sangoma_interface::get_wan_udphdr_data_byte(unsigned char off) 
{
	unsigned char *p_data = (unsigned char*)&wan_udp.wan_udphdr_data[0];
	return p_data[off];
}

int sangoma_interface::read_te1_56k_stat(unsigned long *p_alarms)
{
	unsigned char* data = NULL;
	unsigned long 	alarms = 0x00;
	sdla_te_pmon_t*	pmon = NULL;

	DBG_IFACE("%s()\n", __FUNCTION__);

	memset(&wan_udp, 0x00, sizeof(wan_udp));

	switch(adapter_type)
	{
	case WAN_MEDIA_T1:
	case WAN_MEDIA_E1:
	case WAN_MEDIA_56K:
		;//do nothing
		break;

	default:
		ERR_IFACE( "Command invalid for Adapter Type %d.\n", adapter_type);
		return 1;
	}

	/* Read T1/E1/56K alarms and T1/E1 performance monitoring counters */
	wan_udp.wan_udphdr_command = WAN_FE_GET_STAT;
	wan_udp.wan_udphdr_data_len = 0;

	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		ERR_IFACE( "Failed to read T1/E1/56K statistics.\n");
		return 1;
	}

	data = get_wan_udphdr_data_ptr(0);

	*p_alarms = alarms = *(unsigned long*)data;
	if (adapter_type == WAN_MEDIA_T1 || adapter_type == WAN_MEDIA_E1){
		DBG_IFACE( "***** %s: %s Alarms *****\n\n",
			interface_name, (adapter_type == WAN_MEDIA_T1) ? "T1" : "E1");
		DBG_IFACE( "ALOS:\t%s\t| LOS:\t%s\n", 
				WAN_TE_ALOS_ALARM(alarms), 
				WAN_TE_LOS_ALARM(alarms));
		DBG_IFACE( "RED:\t%s\t| AIS:\t%s\n", 
				WAN_TE_RED_ALARM(alarms), 
				WAN_TE_AIS_ALARM(alarms));
		if (adapter_type == WAN_MEDIA_T1){ 
			DBG_IFACE( "YEL:\t%s\t| OOF:\t%s\n", 
					WAN_TE_YEL_ALARM(alarms), 
					WAN_TE_OOF_ALARM(alarms));
		}else{
			DBG_IFACE( "OOF:\t%s\n", 
					WAN_TE_OOF_ALARM(alarms));
		}
	}else{
		DBG_IFACE( "***** %s: 56K CSU/DSU Alarms *****\n\n\n", interface_name);
	 	DBG_IFACE( "In Service:\t\t%s\tData mode idle:\t\t%s\n",
			 	INS_ALARM_56K(alarms), 
			 	DMI_ALARM_56K(alarms));
	
	 	DBG_IFACE( "Zero supp. code:\t%s\tCtrl mode idle:\t\t%s\n",
			 	ZCS_ALARM_56K(alarms), 
			 	CMI_ALARM_56K(alarms));

	 	DBG_IFACE( "Out of service code:\t%s\tOut of frame code:\t%s\n",
			 	OOS_ALARM_56K(alarms), 
			 	OOF_ALARM_56K(alarms));
		
	 	DBG_IFACE( "Valid DSU NL loopback:\t%s\tUnsigned mux code:\t%s\n",
			 	DLP_ALARM_56K(alarms), 
			 	UMC_ALARM_56K(alarms));

	 	DBG_IFACE( "Rx loss of signal:\t%s\t\n",
			 	RLOS_ALARM_56K(alarms)); 
	}

	pmon = (sdla_te_pmon_t*)&data[sizeof(unsigned long)];
/*
//FIXME:
	DBG_IFACE( "\n\n***** %s: %s Performance Monitoring Counters *****\n\n",
			interface_name, (adapter_type == WAN_MEDIA_T1) ? "T1" : "E1");
	DBG_IFACE( "Framing Bit Error:\t%ld\tLine Code Violation:\t%ld\n", 
			pmon->frm_bit_error,
			pmon->lcv);
	if (adapter_type == WAN_MEDIA_T1){
		DBG_IFACE( "Out of Frame Errors:\t%ld\tBit Errors:\t\t%ld\n", 
				pmon->oof_errors,
				pmon->bit_errors);
	}else{
		DBG_IFACE( "Far End Block Errors:\t%ld\tCRC Errors:\t%ld\n", 
				pmon->far_end_blk_errors,
				pmon->crc_errors);
	}
*/
	return 0;
}

char sangoma_interface::alarms_present()
{
	unsigned long alarms;
	unsigned char is_alarm_set = 0;
	wanpipe_api_t wp_api;

	SANGOMA_INIT_TDM_API_CMD(wp_api);

	DBG_IFACE("%s()\n", __FUNCTION__);

#define DBG_ALARMS if(0)printf

	if(read_te1_56k_stat(&alarms)){
		return 1;
	}

	if (adapter_type == WAN_MEDIA_T1 || adapter_type == WAN_MEDIA_E1){
		DBG_IFACE( "***** %s: %s Alarms *****\n\n",
			interface_name, (adapter_type == WAN_MEDIA_T1) ? "T1" : "E1");

		if(!strcmp(WAN_TE_ALOS_ALARM(alarms), ON_STR)){
			DBG_ALARMS("ALOS\n");
			is_alarm_set = 1;
		}

		if(!strcmp(WAN_TE_LOS_ALARM(alarms), ON_STR)){
			DBG_ALARMS("LOS\n");
			is_alarm_set = 1;
		}

		if(!strcmp(WAN_TE_RED_ALARM(alarms), ON_STR)){
			DBG_ALARMS("RED\n");
			is_alarm_set = 1;
		}
		
		if(!strcmp(WAN_TE_AIS_ALARM(alarms), ON_STR)){
			DBG_ALARMS("AIS\n");
			is_alarm_set = 1;
		}

		if (adapter_type == WAN_MEDIA_T1){ 
			if(!strcmp(WAN_TE_YEL_ALARM(alarms), ON_STR)){
				DBG_ALARMS("\n");
				is_alarm_set = 1;
			}

			if(!strcmp(WAN_TE_OOF_ALARM(alarms), ON_STR)){
				DBG_ALARMS("\n");
				is_alarm_set = 1;
			}
		}else{
			if(!strcmp(WAN_TE_OOF_ALARM(alarms), ON_STR)){
				DBG_ALARMS("\n");
				is_alarm_set = 1;
			}
		}
	}

	unsigned char fe_status = FE_DISCONNECTED;

	if (is_alarm_set == 1) {
		//An alarm may be set, but the Front End code in the driver has
		//better idea if it should be ignored. Check FE status, and if,
		//it is "Connected", ignore the alarm.
		sangoma_get_fe_status(sangoma_dev, &wp_api, &fe_status);

		if (fe_status == FE_CONNECTED) {
			is_alarm_set = 0;
		}
	}
				
	DBG_ALARMS("is_alarm_set: %d\n", is_alarm_set);

	return is_alarm_set;
}

int sangoma_interface::loopback_command(u_int8_t type, u_int8_t mode, u_int32_t chan_map)
{
	sdla_fe_lbmode_t	*lb;
	int			err = 0, cnt = 0;

	lb = (sdla_fe_lbmode_t*)get_wan_udphdr_data_ptr(0);
	memset(lb, 0, sizeof(sdla_fe_lbmode_t));
	lb->cmd		= WAN_FE_LBMODE_CMD_SET;
	lb->type	= type;
	lb->mode	= mode;
	lb->chan_map	= chan_map;

lb_poll_again:
	wan_udp.wan_udphdr_command	= WAN_FE_LB_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_lbmode_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;
	
	DO_COMMAND(wan_udp);
	
	if (wan_udp.wan_udphdr_return_code){
		err = 1;
		return err;
	}
	if (lb->rc == WAN_FE_LBMODE_RC_PENDING){

		if (!cnt) printf("Please wait ..");fflush(stdout);
		if (cnt++ < 10){
			printf(".");fflush(stdout);
			Sleep(100);
			lb->cmd	= WAN_FE_LBMODE_CMD_POLL;
			lb->rc	= 0x00;
			goto lb_poll_again;
		}
		err = 2;
		goto loopback_command_exit;
	}else if (lb->rc != WAN_FE_LBMODE_RC_SUCCESS){
		err = 3;
	}
	if (cnt) printf("\n");

loopback_command_exit:
	return err;
}

void sangoma_interface::set_lb_modes(unsigned char type, unsigned char mode)
{
	DBG_IFACE("%s()\n", __FUNCTION__);
	switch(adapter_type)
	{
	case WAN_MEDIA_T1:
	case WAN_MEDIA_E1:
	case WAN_MEDIA_56K:
		;//do nothing
		break;
	default:
		ERR_IFACE( "Command invalid for Adapter Type %d.\n", adapter_type);
		return;
	}

	if(loopback_command(type, mode, ENABLE_ALL_CHANNELS)){
		ERR_IFACE("Error: Loop Back command failed!\n");
		return;
	}

	if (adapter_type == WAN_MEDIA_T1 || adapter_type == WAN_MEDIA_E1){
		DBG_IFACE("%s %s mode ... %s!\n",
				WAN_TE1_LB_ACTION_DECODE(mode),
				WAN_TE1_LB_MODE_DECODE(type),
				(!wan_udp.wan_udphdr_return_code)?"Done":"Failed");
	}else if (adapter_type == WAN_MEDIA_DS3 || adapter_type == WAN_MEDIA_E3){
		DBG_IFACE("%s %s mode ... %s!\n",
				WAN_TE3_LB_ACTION_DECODE(mode),
				WAN_TE3_LB_TYPE_DECODE(type),
				(!wan_udp.wan_udphdr_return_code)?"Done":"Failed");
	}else{
		DBG_IFACE("%s %s mode ... %s (default)!\n",
				WAN_TE1_LB_ACTION_DECODE(mode),
				WAN_TE1_LB_MODE_DECODE(type),
				(!wan_udp.wan_udphdr_return_code)?"Done":"Failed");
	}
	return;
}

void sangoma_interface::operational_stats(wanpipe_chan_stats_t *stats)
{
	wanpipe_api_t wp_api;

	SANGOMA_INIT_TDM_API_CMD(wp_api);

	int err = sangoma_get_stats(sangoma_dev, &wp_api, stats);
	if (err || wp_api.wp_cmd.result != SANG_STATUS_SUCCESS) {
		ERR_IFACE("sangoma_get_stats() failed! err: %d (0x%X), result:%d!\n",
			err, err, wp_api.wp_cmd.result);
		return;
	}

	DBG_IFACE( "**** OPERATIONAL_STATS ****\n");

	DBG_IFACE( "\ttx_dropped\t: %u\n", stats->tx_dropped);
	DBG_IFACE( "\ttx_packets\t: %u\n", stats->tx_packets);
	DBG_IFACE( "\ttx_bytes\t: %u\n", stats->tx_bytes);
	DBG_IFACE( "\n");

	//receive stats
	DBG_IFACE( "\trx_packets\t: %u\n", stats->rx_packets);
	DBG_IFACE( "\trx_bytes\t: %u\n", stats->rx_bytes);

	DBG_IFACE( "\trx_errors\t: %u\n", stats->rx_errors);
	DBG_IFACE( "\trx_frame_errors\t: %u\n", stats->rx_frame_errors);

	DBG_IFACE( "\trx_packets_discarded_rx_q_full\t: %u\n",
		stats->rx_dropped);
	DBG_IFACE( "\n");

	//hardware level stats
	//AFT card
	DBG_IFACE( "\trx_fifo_errors\t: %u\n", stats->rx_fifo_errors);
	DBG_IFACE( "\ttx_fifo_errors\t: %u\n", stats->tx_fifo_errors);

	//BitStream mode only - counter of data blocks transmitted automatically,
	//because no User Data was available. Transmit underrun counter from user
	//point of view.
	DBG_IFACE( "\ttx_idle_data\t: %u\n", stats->tx_idle_packets);
}

void sangoma_interface::flush_operational_stats (void)
{
	wanpipe_api_t wp_api;

	SANGOMA_INIT_TDM_API_CMD(wp_api);
    int err = sangoma_flush_stats(sangoma_dev, &wp_api);
	if (err || wp_api.wp_cmd.result != SANG_STATUS_SUCCESS) {
		ERR_IFACE("sangoma_flush_stats() failed! err: %d (0x%X), result:%d!\n",
			err, err, wp_api.wp_cmd.result);
		return;
	}

	DBG_IFACE( "Command FLUSH_OPERATIONAL_STATS was successful.\n");
}

void sangoma_interface::flush_buffers (void)
{
	wanpipe_api_t wp_api;

	SANGOMA_INIT_TDM_API_CMD(wp_api);
    int err = sangoma_flush_bufs(sangoma_dev, &wp_api);
	if (err || wp_api.wp_cmd.result != SANG_STATUS_SUCCESS) {
		ERR_IFACE("sangoma_flush_buffers() failed! err: %d (0x%X), result:%d!\n",
			err, err, wp_api.wp_cmd.result);
		return;
	}

	DBG_IFACE( "Command sangoma_flush_bufs  was successful.\n");
}

int sangoma_interface::get_interface_configuration(if_cfg_t *wanif_conf_ptr)
{
	wan_udp.wan_udphdr_command = WANPIPEMON_READ_CONFIGURATION;
	wan_udp.wan_udphdr_data_len = sizeof(if_cfg_t);

	memset(wanif_conf_ptr, 0x00, sizeof(if_cfg_t));

	DO_COMMAND(wan_udp);
	if(wan_udp.wan_udphdr_return_code){
		ERR_IFACE( "Error: command READ_CONFIGURATION failed!\n");
		return 1;
	}
	memcpy(wanif_conf_ptr, get_wan_udphdr_data_ptr(0), sizeof(if_cfg_t));

	DBG_IFACE( "**** READ_CONFIGURATION ****\n");
	DBG_IFACE( "Operational Mode\t: %s (%d)\n", SDLA_DECODE_USEDBY_FIELD(wanif_conf_ptr->usedby), wanif_conf_ptr->usedby);
	DBG_IFACE( "Configued Active Channels \t: 0x%08X\n", wanif_conf_ptr->active_ch);
	DBG_IFACE( "Echo Canceller Channels\t\t: 0x%08X\n", wanif_conf_ptr->ec_active_ch);
	DBG_IFACE( "User Specified Channels during Port Config\t: 0x%08X\n", wanif_conf_ptr->cfg_active_ch);
	DBG_IFACE( "Interface Number\t: %u\n", wanif_conf_ptr->interface_number);
	DBG_IFACE( "Media type\t\t: %u\n", wanif_conf_ptr->media);
	DBG_IFACE( "Line Mode\t\t: %s\n", wanif_conf_ptr->line_mode);
	DBG_IFACE( "****************************\n");

#if 1
	// Audio Codec can be read for Voice interface.
	// For Data interface this setting is ignored by the API.
	wanpipe_api_t wp_api;
	SANGOMA_INIT_TDM_API_CMD(wp_api);
	sangoma_get_hw_coding(sangoma_dev, &wp_api);
	if (wp_api.wp_cmd.result == SANG_STATUS_SUCCESS) {
		DBG_IFACE("Audio Codec: %s\n", 
			WP_CODEC_FORMAT_DECODE(wp_api.wp_cmd.hw_tdm_coding));
	}
#endif

	return 0;
}

int sangoma_interface::get_api_driver_version (PDRIVER_VERSION p_drv_version)
{
	memcpy(p_drv_version, &drv_version, sizeof(DRIVER_VERSION));

	DBG_IFACE("\nAPI version\t: %d,%d,%d,%d\n",
		p_drv_version->major, p_drv_version->minor, p_drv_version->minor1, p_drv_version->minor2);
	DBG_IFACE("\n");
	return 0;
}

int sangoma_interface::check_api_driver_version( )
{
	DBG_IFACE("\nAPI version\t: %d,%d,%d,%d\n",
		drv_version.major, drv_version.minor, drv_version.minor1, drv_version.minor2);

	// verify that Driver Version is the same as Header files used to compile this application
	if(drv_version.major != WANPIPE_VERSION_MAJOR){
		ERR_IFACE("Error: drv_version.major (%d) != WANPIPE_VERSION_MAJOR (%d)\n",
			 drv_version.major, WANPIPE_VERSION_MAJOR);
		exit(1);
	}

	if(drv_version.minor != WANPIPE_VERSION_MINOR){
		ERR_IFACE("Error: drv_version.minor (%d) != WANPIPE_VERSION_MINOR (%d)\n", 
			drv_version.minor, WANPIPE_VERSION_MINOR);
		exit(1);
	}

	if(drv_version.minor1 != WANPIPE_VERSION_MINOR1){
		ERR_IFACE("Error: drv_version.minor1 (%d) != WANPIPE_VERSION_MINOR1 (%d)\n",
			 drv_version.minor1, WANPIPE_VERSION_MINOR1);
		exit(1);
	}

	unsigned int compiled_minor_version = WANPIPE_VERSION_MINOR2;
	// This is a PATCH number - allow it to be greater or equal because binary 
	// compatibility is guaranteed
	if(drv_version.minor2 < compiled_minor_version){
		ERR_IFACE("Error: drv_version.minor2 (%d) != WANPIPE_VERSION_MINOR2 (%d)\n",
			 drv_version.minor2, WANPIPE_VERSION_MINOR2);
		exit(1);
	}

	return 0;
}

int sangoma_interface::DoManagementCommand(sng_fd_t fd, wan_udp_hdr_t* wan_udp)
{
	return sangoma_mgmt_cmd(fd, wan_udp);
}

sangoma_wait_obj_t* sangoma_interface::get_wait_object_reference()
{
	return sng_wait_obj;
}

unsigned char sangoma_interface::get_adapter_type()
{
	return adapter_type;
}

//Returns Port Number mapped specifically for G3 use  for a CARD.
unsigned int sangoma_interface::get_mapped_port_number()
{
	return interface_number;
}

//Returns Span Number - global for the APPLICATION
unsigned int sangoma_interface::get_mapped_span_number()
{
	return wanpipe_number + 1 + get_mapped_port_number();
}

int sangoma_interface::transmit(wp_api_hdr_t* txhdr, void *pTx, UINT buffer_length)
{
	txhdr->operation_status = SANG_STATUS_GENERAL_ERROR;
	txhdr->data_length = buffer_length;

	if(sangoma_writemsg(sangoma_dev, txhdr, sizeof(*txhdr), pTx, buffer_length, 0) < 0){
		//error
		ERR_IFACE("sangoma_writemsg() failed!\n");
		return SANG_STATUS_IO_ERROR;
	}

	switch(txhdr->operation_status)
	{
	case SANG_STATUS_SUCCESS:
		//OK
		break;
	default:
		//Error! Non-zero return code indicates Error to G3TI application.
		ERR_IFACE("Return code: %s (%d) on transmission!\n",
			SDLA_DECODE_SANG_STATUS(txhdr->operation_status), txhdr->operation_status);
		break;
	}//switch()

	return txhdr->operation_status;
}

int sangoma_interface::read_data(wp_api_hdr_t* rxhdr, void *pRx, UINT buffer_length)
{	
	int return_code;

	memset(rxhdr, 0x00, sizeof(*rxhdr));

	rxhdr->operation_status = SANG_STATUS_GENERAL_ERROR;

	//Non-blocking read data.
	return_code = sangoma_readmsg(sangoma_dev, rxhdr, sizeof(*rxhdr), pRx, buffer_length, 0);

	if(return_code <= 0){

		if (SANG_STATUS_NO_DATA_AVAILABLE == rxhdr->operation_status) {
			// no data is NOT an error at THIS level
			return 0;
		} else {
			//Error! Non-zero return code indicates Error to G3TI application.
			return 1;
		}
	}

	return 0;
}

int sangoma_interface::read_event(wanpipe_api_t* wp_api)
{
	SANGOMA_INIT_TDM_API_CMD(*wp_api);

	//Non-blocking read Event.
	return sangoma_read_event(sangoma_dev, wp_api);
}

int sangoma_interface::set_buffer_multiplier(wanpipe_api_t* wp_api, int buffer_multiplier)
{
	SANGOMA_INIT_TDM_API_CMD(*wp_api);

	return sangoma_tdm_set_buffer_multiplier(sangoma_dev, wp_api, buffer_multiplier);
}
