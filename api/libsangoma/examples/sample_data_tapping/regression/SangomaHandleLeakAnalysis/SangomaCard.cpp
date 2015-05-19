///////////////////////////////////////////////////////////////////////////////////////////////
///	\file	SangomaCard.cpp
///	\brief	This file contains the implementation of the SangomaCard class
///////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__WINDOWS__)
# ifndef _DEBUG
#  pragma optimize("2gt",on)		// Windows specific (MS_SPECIFIC), enables optimization in Release mode
# endif
#endif

#include "SangomaCard.h"
#include <sstream>

using namespace Sangoma;

#include "driver_configurator.h"


#define DBG_CARD	if(0)printf
#define INFO_CARD	if(0)printf
#define FUNC_TRACE()	DBG_CARD("%s(): Line: %u\n", __FUNCTION__, __LINE__)
//This macro is for Error reporting and should NOT be disabled.
#define ERR_CARD	printf("Error:%s():line:%d: ", __FUNCTION__, __LINE__);printf

//these arrays is initialized by GetNumberOfCardsInstalled()
static wanpipe_instance_info_t a104_wanpipe_info_array[MAX_CARDS];
static wanpipe_instance_info_t a108_wanpipe_info_array[MAX_CARDS];
static wanpipe_instance_info_t a116_wanpipe_info_array[MAX_CARDS];
static wanpipe_instance_info_t t116_wanpipe_info_array[MAX_CARDS];


///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		unsigned int GetNumberOfCardsInstalled(const CardModel model)
///	\brief	Returns the number of Sangoma cards of the model provided for this system.  It is
///			possible for there to be a mixture of A104 and A108, and so the number of each can
///			be requested by changing the CardModel provided.
///	\param	model			CardModel for which we want to retrieve the number of cards installed
///	\return	unsigned int number of cards of the CardModel provided
///	\author	J. Markwordt
///	\date	10/08/2007
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int GetNumberOfCardsInstalled(const CardModel model)
{
	unsigned int number_of_card_model = 0;
	driver_configurator	*drv_cfg_obj;

	/// Perform operations to get the number of Sangoma cards of the model provided in the system
	drv_cfg_obj = new driver_configurator();
	switch(model)
	{
	case A104:
		number_of_card_model = drv_cfg_obj->scan_for_sangoma_cards(a104_wanpipe_info_array, A104_ADPTR_4TE1);
		break;

	case A108:
		number_of_card_model = drv_cfg_obj->scan_for_sangoma_cards(a108_wanpipe_info_array, A108_ADPTR_8TE1);
		break;

	case A116:
		number_of_card_model = drv_cfg_obj->scan_for_sangoma_cards(a116_wanpipe_info_array, A116_ADPTR_16TE1);
		break;
	
	case T116:
		number_of_card_model = drv_cfg_obj->scan_for_sangoma_cards(t116_wanpipe_info_array, AFT_ADPTR_T116);
		break;

	default:
		number_of_card_model = 0;
		break;
	}

	delete drv_cfg_obj;
	return number_of_card_model;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::SangomaCard(const unsigned int card_number, const CardModel model,
///			const BufferSettings & ReceiveBufferSettings, const BufferSettings & TransmitBufferSettings)
///	\brief	Constructor
///	\param	card_number					const unsigned int of the number of this card (zero indexed)
///	\param	model						const CardModel of the type of Sangoma card this object is to
///										represent (A104 or A108)
///	\param	ReceiveBufferSettings		BufferSettings to configure the host memory buffers 
///										for receive operations on this port (set NumberOfBuffersPerPort
///										to 0 to disable receiving data on this port)
///	\param	TransmitBufferSettings	BufferSettings to configure the host memory buffers 
///										for transmit operations on this port (set NumberOfBuffersPerPort
///										to 0 to disable transmitting data on this port)
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////
SangomaCard::SangomaCard(const unsigned int card_number, const CardModel model,
		const BufferSettings & ReceiveBufferSettings, const BufferSettings & TransmitBufferSettings)
: CardNumber(card_number),
Model(model),							// Set the card model to that provided
NumberOfPorts(NUMBER_OF_A104_PORTS),	// Initialize to A104, but will be reset in the body of the constructor
Ports(),
LastError(NO_ERROR_OCCURRED)
{
	FUNC_TRACE();
	wanpipe_instance_info_t *wanpipe_instance_info;

	switch (model) {
	case A104:
		wanpipe_instance_info = a104_wanpipe_info_array;
		break;
	case A108:
		wanpipe_instance_info = a108_wanpipe_info_array;
		break;
	case A116:
		wanpipe_instance_info = a116_wanpipe_info_array;
		break;
	case T116:
		wanpipe_instance_info = t116_wanpipe_info_array;
		break;
	}

	if(MAX_CARDS <= card_number){
		ERR_CARD("%s(): invalid 'card_number' %u!!\n", __FUNCTION__, card_number);
		return;
	}

	// The number of ports was initialized above for the A104, 
	// so only change it if A108 was the model that was actually provided.
	if (A108 == model) {
		NumberOfPorts = NUMBER_OF_A108_PORTS;
	}
	if (A116 == model) {
		NumberOfPorts = NUMBER_OF_A116_PORTS;
	}
	if (T116 == model) {
		NumberOfPorts = NUMBER_OF_T116_PORTS;
	}

	DBG_CARD("%s(): wanpipe_instance_info: 0x%p\n", __FUNCTION__, wanpipe_instance_info);

	// Create the number of ports required for this card's
	// model type and add them to the Ports vector.
	for (unsigned int port = 0; port < NumberOfPorts; port++) {

		Ports.push_back(new SangomaPort(wanpipe_instance_info[CardNumber].wanpipe_number, port, 
			ReceiveBufferSettings, TransmitBufferSettings));
	}
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::~SangomaCard()
///	\brief	Destructor
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////
SangomaCard::~SangomaCard()
{
	// Delete the all of the ports created for this card's
	// model type and then clear the  Ports vector
	for (unsigned int port = 0; port < NumberOfPorts; ++port) {
		delete Ports.at(port);
	}	
	Ports.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		unsigned int SangomaCard::GetCardNumber() const
///	\brief	Returns the card number of this Sangoma card (zero indexed)
///	\return	unsigned int card number of this card
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int SangomaCard::GetCardNumber() const
{
	return CardNumber;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		CardModel SangomaCard::GetModel() const
///	\brief	Returns the model of Sangoma card that this object represents (A104 or A108)
///	\return	CardModel of this SangomaCard
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
CardModel SangomaCard::GetModel() const
{
	return Model;
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		unsigned int SangomaCard::GetNumberOfPorts() const
///	\brief	Returns the number of ports in this card
///	\return	unsigned int number of ports in this card
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
unsigned int SangomaCard::GetNumberOfPorts() const
{
	return NumberOfPorts;
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::GetPort(const unsigned int port_number)
///	\brief	Returns a pointer to the port requested
///	\return	SangomaPort* to the port requested
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
SangomaPort* SangomaCard::GetPort(const unsigned int port_number)
{
	// Check to make make sure the port_number requested is valid
	bool valid_port_number = (NumberOfPorts > port_number);
	if (!valid_port_number) {
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaCard::GetPort() called on invalid port number " << port_number << ".";
		LastError = error_msg.str();
		return NULL;
	}

	// Port number is valid, so indicate that no error occured on this operation
	LastError = NO_ERROR_OCCURRED;

	// If valid, return the pointer to the SangomaPort at that position
	return Ports.at(port_number);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::GetCardDriverVersion()
///	\brief	Returns the card's driver version number.  Sangoma has given the card a driver and 
///			the port what Sangoma refers to as a "Hardware Abstraction Driver", and although
///			these two should have the same version number IF INSTALLED PROPERLY, they are in
///			fact two separate drivers.
///
///			This function is to retrieve the driver associated with the card as a whole and
///			can be used to verify that it matches the version number of the ports' hardware
///			abstraction drivers.
///	\return	DriverVersion structure containing the driver version information for this card
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
DriverVersion SangomaCard::GetCardDriverVersion()
{
	DriverVersion		card_driver_version;

	/// Perform low level calls to retrieve the driver version from the card
	SangomaPort	*p_port = GetPort(0);
	if(p_port != NULL){
		card_driver_version.Major = p_port->GetDriverVersion().Major;
		card_driver_version.Build = p_port->GetDriverVersion().Build;
		card_driver_version.Minor = p_port->GetDriverVersion().Minor;
		card_driver_version.Revision = p_port->GetDriverVersion().Revision;
		DBG_CARD("%s(): card_driver_version: %i.%i.%i.%i\n", __FUNCTION__, card_driver_version.Major,
			card_driver_version.Minor, card_driver_version.Build, card_driver_version.Revision);
	}
	return card_driver_version;
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::GetSerialNumber() const
///	\brief	Returns the card's serial number
///	\return	std::string containing the card's serial number
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
std::string SangomaCard::GetSerialNumber() const
{
	std::string serial_number;
	/// \todo Perform low level calls to retrieve the serial number for the card
	//DAVIDR: there is no way to know card serial number.
	return serial_number;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::GetFirmwareVersion() const
///	\brief	Returns the card's firmware version
///	\return	std::string containing  the card's firmware version
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////
std::string SangomaCard::GetFirmwareVersion() const
{
	std::string firmware_version;
	wanpipe_instance_info_t *WanpipeInstanceInfo = NULL;

	/// Perform low level calls to retrieve the firmware version number for the card
	switch(GetModel())
	{
	case A104:
		WanpipeInstanceInfo = a104_wanpipe_info_array;
		break;
	case A108:
		WanpipeInstanceInfo = a108_wanpipe_info_array;
		break;
	case A116:
		WanpipeInstanceInfo = a116_wanpipe_info_array;
		break;
	case T116:
		WanpipeInstanceInfo = t116_wanpipe_info_array;
		break;
	}

	firmware_version = "0000";
	if(WanpipeInstanceInfo){
		sprintf((char*)firmware_version.c_str(), "%X", 
			WanpipeInstanceInfo[GetCardNumber()].hardware_info.firmware_version);
		DBG_CARD("%s(): card firmware version: %s\n", __FUNCTION__, firmware_version.c_str());
	}
	return firmware_version;
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::GetCardPciBus() const
///	\brief	Returns the number of the PCI bus that the card is located in
///	\return	unsigned int number of the PCI bus that the card is located in
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int SangomaCard::GetCardPciBus() const
{
	unsigned int pci_bus_number = 0;
	wanpipe_instance_info_t *WanpipeInstanceInfo = NULL;

	/// Perform low level calls to retrieve the number of the PCI bus that the card is located in
	switch(GetModel())
	{
	case A104:
		WanpipeInstanceInfo = a104_wanpipe_info_array;
		break;
	case A108:
		WanpipeInstanceInfo = a108_wanpipe_info_array;
		break;
	case A116:
		WanpipeInstanceInfo = a116_wanpipe_info_array;
		break;
	case T116:
		WanpipeInstanceInfo = t116_wanpipe_info_array;
		break;
	}

	if(WanpipeInstanceInfo){
		pci_bus_number = WanpipeInstanceInfo[GetCardNumber()].hardware_info.pci_bus_number;
		DBG_CARD("%s(): card pci_bus_number: %i\n", __FUNCTION__, pci_bus_number);
	}
	return pci_bus_number;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::GetCardPciSlot() const
///	\brief	Returns the number of the PCI slot that the card is located in
///	\return	unsigned int number of the PCI slot that the card is located in
///	\author	J. Markwordt
///	\date	10/10/2007
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int SangomaCard::GetCardPciSlot() const
{
	unsigned int pci_bus_slot = 0;
	wanpipe_instance_info_t *WanpipeInstanceInfo = NULL;

	/// Perform low level calls to retrieve the number of the PCI slot that the card is located in
	switch(GetModel())
	{
	case A104:
		WanpipeInstanceInfo = a104_wanpipe_info_array;
		break;
	case A108:
		WanpipeInstanceInfo = a108_wanpipe_info_array;
		break;
	case A116:
		WanpipeInstanceInfo = a116_wanpipe_info_array;
		break;
	case T116:
		WanpipeInstanceInfo = t116_wanpipe_info_array;
		break;
	}

	if(WanpipeInstanceInfo){
		pci_bus_slot = WanpipeInstanceInfo[GetCardNumber()].hardware_info.pci_slot_number;
		DBG_CARD("%s(): card pci_bus_slot: %i\n", __FUNCTION__, pci_bus_slot);
	}
	return pci_bus_slot;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaCard::GetLastError() const
///	\brief	Returns a description of the last error that occurred 
///	\return Returns a description of the last error that occurred (NO_ERROR_OCCURRED if the last command was successful)
///	\author	J. Markwordt
///	\date	10/17/2007
///////////////////////////////////////////////////////////////////////////////////////////////
std::string SangomaCard::GetLastError() const
{
	return LastError;
}


#if defined(__WINDOWS__)
# ifndef _DEBUG				
#  pragma optimize("2gt",off)// Windows specific (MS_SPECIFIC), disables optimization in Release mode
# endif
#endif

