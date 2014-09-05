//////////////////////////////////////////////////////////////////////
// sangoma_port.cpp: implementation of the sangoma_port class.
//
// Author	:	David Rokhvarg
//////////////////////////////////////////////////////////////////////

#include "sangoma_port.h"

#define DBG_CFG		if(0)printf("PORT:");if(1)printf
#define _DBG_CFG	if(0)printf

#define INFO_CFG	if(0)printf("PORT:");if(1)printf
#define _INFO_CFG	if(0)printf

#define ERR_CFG		if(1)printf("PORT:");if(1)printf
#define _ERR_CFG	if(1)printf

//////////////////////////////////////////////////////////////////////
//common functions
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

sangoma_port::sangoma_port()
{
	DBG_CFG("%s()\n", __FUNCTION__);
	wp_handle = INVALID_HANDLE_VALUE;
}

sangoma_port::~sangoma_port()
{
	DBG_CFG("%s()\n", __FUNCTION__);
	cleanup();
}

int sangoma_port::init(uint16_t wanpipe_number)
{
	DBG_CFG("%s()\n", __FUNCTION__);

	wp_number = wanpipe_number;
	wp_handle = sangoma_open_driver_ctrl(wanpipe_number);
	if(wp_handle == INVALID_HANDLE_VALUE){
		ERR_CFG("Error: failed to open wanpipe%d!!\n", wp_number);
		return 1;
	}
	return 0;
}

void sangoma_port::cleanup()
{
	DBG_CFG("%s()\n", __FUNCTION__);

	if(wp_handle != INVALID_HANDLE_VALUE){
		sangoma_close(&wp_handle);
		wp_handle = INVALID_HANDLE_VALUE;
	}
}

int sangoma_port::get_hardware_info(hardware_info_t *hardware_info)
{
	port_management_struct_t port_management;

	int err=sangoma_driver_get_hw_info(wp_handle,&port_management, wp_number);
	if (err) {
		ERR_CFG("Error: failed to get hw info for wanpipe%d!\n", wp_number);
		err=1;
		return err;
	}

	memcpy(hardware_info,port_management.data,sizeof(hardware_info_t));

	return 0;
}

int sangoma_port::start_port()
{
	port_management_struct_t port_mgmnt;
	int err;

	err=sangoma_driver_port_start(wp_handle,&port_mgmnt,wp_number);

	INFO_CFG("%s(): return code: (%d)\n", __FUNCTION__, err);

	return err;
}

int sangoma_port::stop_port()
{
	port_management_struct_t port_mgmnt;
	int err;

	err = sangoma_driver_port_stop(wp_handle,&port_mgmnt,wp_number);
	
	INFO_CFG("%s(): return code: (%d)\n", __FUNCTION__, err);


	return err;
}

int sangoma_port::push_a_card_into_wanpipe_info_array(
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
int sangoma_port::scan_for_sangoma_cards(wanpipe_instance_info_t *wanpipe_info_array, int card_model)
{
	uint16_t card_counter = 0, wp_ind;
	hardware_info_t	hardware_info;

	//initialize hardware information array.
	memset(wanpipe_info_array, -1, sizeof(wanpipe_instance_info_t)*MAX_CARDS);

	DBG_CFG("%s(): wanpipe_info_array: 0x%p\n", __FUNCTION__, wanpipe_info_array);

	for(wp_ind = 0; wp_ind < (MAX_CARDS	* MAX_PORTS_PER_CARD); wp_ind++){
		if(init(wp_ind)){
			continue;
		}
		if(get_hardware_info(&hardware_info)){
			card_counter = -1;
			break;
		}

		DBG_CFG("card_model		: %s (0x%08X)\n",
			SDLA_ADPTR_NAME(hardware_info.card_model), hardware_info.card_model);
		DBG_CFG("firmware_version\t: 0x%02X\n", hardware_info.firmware_version);
		DBG_CFG("pci_bus_number\t\t: %d\n", hardware_info.pci_bus_number);
		DBG_CFG("pci_slot_number\t\t: %d\n", hardware_info.pci_slot_number);
		DBG_CFG("port_number\t\t: %d\n", hardware_info.port_number);
		DBG_CFG("max_hw_ec_chans\t\t: %d\n", hardware_info.max_hw_ec_chans);
		
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

		//For each init() there MUST be cleanup(). Otherwise Open Handles will remain to the Port.
		cleanup();

	}//for()

	return card_counter;
}
