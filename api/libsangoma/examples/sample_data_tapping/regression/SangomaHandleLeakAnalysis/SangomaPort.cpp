///////////////////////////////////////////////////////////////////////////////////////////////
///	\file	SangomaPort.cpp
///	\brief	This file contains the implementation of the SangomaPort class
///////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__WINDOWS__)
# ifndef _DEBUG				
#  pragma optimize("2gt",on)		// Windows specific (MS_SPECIFIC), enables optimization in Release mode
# endif
#endif

#include <sstream>
#include <time.h>
#include "SangomaPort.h"
#include "driver_configurator.h"

#define DBG_PORT	if(0)printf
#define INFO_PORT	if(0)printf
//This macro is for Error reporting and should NOT be disabled.
#define ERR_PORT	printf("Error:%s():line:%d: ", __FUNCTION__, __LINE__);printf

#define DBG_OPEN	if(0)printf
#define PORT_FUNC()	if(0)printf("%s(): Line: %u\n", __FUNCTION__, __LINE__)

extern void print_data_buffer(unsigned char *buffer, int buffer_length);

using namespace Sangoma;

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		Configuration::Configuration()
///	\brief	Constructor (for the Configuration structure)
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
Configuration::Configuration()
: 	e1Ort1(E1),
	Framing(E1_CAS_CRC4_ON),
	lineCoding(OFF),
	HighImpedanceMode(true),
	TxTristateMode(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		Statistics::Statistics()
///	\brief	Constructor (for the Statistics structure)
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
Statistics::Statistics()
:	TimeLastOpened(0),
	TimeLastClosed(0),
	TimeLastFoundSync(0),
	TimeLastLostSync(0),
	OpenCount(0),
	CloseCount(0),
	FoundSyncCount(0),
	LostSyncCount(0),
	BytesReceived(0),
	BytesSent(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		Alarms::Alarms()
///	\brief	Constructor (for the Alarms structure)
///	\author	J. Markwordt
///	\date	10/19/2007
///////////////////////////////////////////////////////////////////////////////////////////////
Alarms::Alarms()
:	LossOfSignal(false),
	ReceiveLossOfSignal(false),
	AlternateLossOfSignalStatus(false),
	OutOfFrame(false),
	Red(false),
	AlarmIndicationSignal(false),
	LossOfSignalingMultiframe(false),
	LossOfCrcMultiframe(false),
	OutOfOfflineFrame(false),
	ReceiveLossOfSignalV(false),
	Yellow(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		DriverVersion::DriverVersion()
///	\brief	Constructor (for the DriverVersion structure)
///	\author	J. Markwordt
///	\date	10/19/2007
///////////////////////////////////////////////////////////////////////////////////////////////
DriverVersion::DriverVersion()
:	Major(0),
	Minor(0),
	Build(0),
	Revision(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		PortEvent::PortEvent()
///	\brief	Constructor (for the PortEvent structure)
///	\author	J. Markwordt
///	\date	10/18/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
PortEvent::PortEvent()
:	EventType(Unsignalled),
	EventHandle(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::SangomaPort(const unsigned int card_number, const unsigned int port_number, 
///		const PortEvent & event, const BufferSettings & ReceiveBufferSettings,
///		const BufferSettings & TransmitBufferSettings)
///	\brief	Constructor
///
///			Sets the size and number of the port's buffers for transmit and recieve (see the
///			BufferSettings structure for more details).  Receive or transmit can be disabled,
///			allowing more resources to be devoted to the other direction by setting the 
///			NumberOfBuffersPerPort for their respective BufferSettings structures.
///	\param	card_number						unsigned int number of the SangomaCard that this port is part of
///											(zero indexed)
///	\param	port_number						unsigned int number of the port that this object represents
///											(this is relative to the card and zero indexed, so the first
///											port of both card 1 and 2 will be 0)
///	\param	event							PortEvent structure to signal port events with (contains HANDLE
///											to signal the event with and EventType to set indicating the type of event)
///	\param	ReceiveBufferSettings			BufferSettings to configure the host memory buffers 
///											for receive operations on this port (set NumberOfBuffersPerPort
///											to 0 to disable receiving data on this port)
///	\param	TransmitBufferSettings		BufferSettings to configure the host memory buffers 
///											for transmit operations on this port (set NumberOfBuffersPerPort
///											to 0 to disable transmitting data on this port)
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
SangomaPort::SangomaPort(const unsigned int card_number, const unsigned int port_number, 
		const BufferSettings & ReceiveBufferSettings,
		const BufferSettings & TransmitBufferSettings)
:	CardNumber(card_number),
	PortNumber(port_number),
	IsOpen(false),
	CurrentConfiguration(),
	PortStatistics()
{
	PORT_FUNC();

	LastError = NO_ERROR_OCCURRED;

	DBG_PORT("%s(): card_number: %u, port_number: %u\n", __FUNCTION__, card_number, port_number);

	InitializeCriticalSection(&cs_CriticalSection);

	SangomaInterface = new sangoma_interface(CardNumber, PortNumber);
	DBG_PORT("SangomaInterface: 0x%p\n", SangomaInterface);

	this->ReceiveBufferSettings = ReceiveBufferSettings;
	this->TransmitBufferSettings = TransmitBufferSettings;

	// Calculate the size of a host memory buffer by multiplying the maximum buffer size as defined
	// by the Sangoma API by a multiplier (buffer_multiplier_factor) for both the receive and transmit buffers
	ReceiveBuffer = new PortBuffer(sizeof(RX_DATA_STRUCT) * ReceiveBufferSettings.BufferMultiplierFactor);
	TransmitBuffer = new PortBuffer(sizeof(TX_DATA_STRUCT) * TransmitBufferSettings.BufferMultiplierFactor);

	pTxFile = NULL;
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::~SangomaPort()
///	\brief	Destructor
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
SangomaPort::~SangomaPort()
{
	PORT_FUNC();
	//EnterPortCriticalSection();
	//LeavePortCriticalSection();
	//PORT_FUNC();

	// If the has not been properly closed, close it
	if (IsOpen) {
		Close();
	}

	// Deallocate Rx/Tx Port buffers
	delete ReceiveBuffer;
	delete TransmitBuffer;

	if(SangomaInterface != NULL){
		delete SangomaInterface;
		SangomaInterface = NULL;
	}

	if (pTxFile) {
		fclose( pTxFile );
	}

	PORT_FUNC();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::~SangomaPort(const Configuration & config)
///	\brief	Implementation of Sangoma's low level API call to set port configuration - T1/E1.
///	\author	David Rokhvarg
///	\date	11/08/2007
///////////////////////////////////////////////////////////////////////////////////////////////	
int SangomaPort::SetPortConfiguration(const Configuration & config)
{
	int				return_code;
	sdla_fe_cfg_t	sdla_fe_cfg;
	buffer_settings_t buffer_settings;
	std::ostringstream error_msg;

	driver_configurator *p_drv_cfg_obj = new driver_configurator();
	if(p_drv_cfg_obj->init(CardNumber, SangomaInterface->get_mapped_port_number())){
		error_msg << "ERROR: " << __FUNCTION__ << "(): failed to initialize Sangoma Driver Configuration object";
		LastError = error_msg.str();
		delete p_drv_cfg_obj;
		return 1;
	}

	memset(&sdla_fe_cfg, 0x00, sizeof(sdla_fe_cfg_t));
	switch(config.e1Ort1)
	{
	case T1:
		FE_LBO(&sdla_fe_cfg) = WAN_T1_LBO_0_DB;//important to set to a valid value for both T1 and E1!!
		FE_MEDIA(&sdla_fe_cfg) = WAN_MEDIA_T1;
		switch(config.lineCoding)
		{
		case OFF:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_AMI;
			break;
		case ON:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_B8ZS;
			break;
		default:
			error_msg << "ERROR: " << __FUNCTION__ << "(): T1 invalid lineCoding " << config.lineCoding;
			LastError = error_msg.str();
			delete p_drv_cfg_obj;
			return 1;
		}

		switch(config.Framing)
		{
		case T1_EXTENDED_SUPERFRAME:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_ESF;
			break;
		case T1_SUPERFRAME:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_D4;
			break;
		default:
			error_msg << "ERROR: " << __FUNCTION__ << "(): T1: invalid Framing " << config.Framing;
			LastError = error_msg.str();
			delete p_drv_cfg_obj;
			return 1;
		}

		FE_SIG_MODE(&sdla_fe_cfg)	= WAN_TE1_SIG_CCS;
		FE_LBO(&sdla_fe_cfg)		= WAN_T1_LBO_0_DB;
		//320*24=7680
		TxMtu = 7680;
		break;

	case E1:
		FE_LBO(&sdla_fe_cfg) = WAN_E1_120;//important to set to a valid value for both T1 and E1!!
		FE_MEDIA(&sdla_fe_cfg) = WAN_MEDIA_E1;
		switch(config.lineCoding)
		{
		case OFF:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_AMI;
			break;
		case ON:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_HDB3;
			break;
		default:
			error_msg << "ERROR: " << __FUNCTION__ << "(): E1 invalid lineCoding " << config.lineCoding;
			LastError = error_msg.str();
			delete p_drv_cfg_obj;
			return 1;
		}

		//FE_FRAME(&sdla_fe_cfg) = WAN_FR_CRC4;
		switch(config.Framing)
		{
		case E1_CAS_CRC4_ON:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_CRC4;
			FE_SIG_MODE(&sdla_fe_cfg) = WAN_TE1_SIG_CAS;
			//240*31=7440
			TxMtu = 7440;
			break;
		case E1_CAS_CRC4_OFF:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_NCRC4;
			FE_SIG_MODE(&sdla_fe_cfg) = WAN_TE1_SIG_CAS;
			//240*31=7440
			TxMtu = 7440;
			break;
		case E1_CCS_CRC4_ON:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_CRC4;
			FE_SIG_MODE(&sdla_fe_cfg) = WAN_TE1_SIG_CCS;
			//240*31=7440
			TxMtu = 7440;
			break;
		case E1_CCS_CRC4_OFF:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_NCRC4;
			FE_SIG_MODE(&sdla_fe_cfg) = WAN_TE1_SIG_CCS;
			//240*31=7440
			TxMtu = 7440;
			break;

		case E1_CAS_UNFRAMED:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_UNFRAMED;
			FE_SIG_MODE(&sdla_fe_cfg) = WAN_TE1_SIG_CAS;
			//240*32=7680
			TxMtu = 7680;
			break;
		case E1_CCS_UNFRAMED:
			FE_FRAME(&sdla_fe_cfg) = WAN_FR_UNFRAMED;
			FE_SIG_MODE(&sdla_fe_cfg) = WAN_TE1_SIG_CCS;
			//240*32=7680
			TxMtu = 7680;
			break;

		default:
			error_msg << "ERROR: " << __FUNCTION__ << "(): E1: invalid Framing " << config.Framing;
			LastError = error_msg.str();
			delete p_drv_cfg_obj;
			return 1;
		}
		break;

	default:
		error_msg << "ERROR: " << __FUNCTION__ << "(): Invalid configuration! Not T1 or E1.";
		LastError = error_msg.str();
		delete p_drv_cfg_obj;
		return 1;
	}//switch(config.e1Ort1)

	//FE_REFCLK(&sdla_fe_cfg)		= 0;	//optional

	///////////////////////////////////////////////////////////////////////////////
	//DAVIDR: for "TAPPING" set clock to WAN_NORMAL_CLK
	//FE_CLK(&sdla_fe_cfg)			= WAN_NORMAL_CLK;
	//DAVIDR: for "PLAYBACK/RECORD" set clock to WAN_NORMAL_CLK or WAN_MASTER_CLK,
	//			depending on how the other side	is configured!

	if (config.Master) {
		FE_CLK(&sdla_fe_cfg)			= WAN_MASTER_CLK;
	} else {
		FE_CLK(&sdla_fe_cfg)			= WAN_NORMAL_CLK;
	}
	///////////////////////////////////////////////////////////////////////////////

	//DAVIDR: High Impedance can be used only with external filters installed!!
	FE_HIMPEDANCE_MODE(&sdla_fe_cfg) = (config.HighImpedanceMode == true ? WANOPT_YES:WANOPT_NO);
	FE_RX_SLEVEL(&sdla_fe_cfg) = config.MaxCableLoss;
	FE_TXTRISTATE(&sdla_fe_cfg) = (config.TxTristateMode == true ? WANOPT_YES:WANOPT_NO);

	///////////////////////////////////////////////////////////////////////////////
	//The same buffer settings will be used in both directions - Tx and Rx.
	buffer_settings.buffer_multiplier_factor = ReceiveBufferSettings.BufferMultiplierFactor;
	buffer_settings.number_of_buffers_per_api_interface = ReceiveBufferSettings.NumberOfBuffersPerPort;

	return_code = p_drv_cfg_obj->set_t1_e1_configuration(&sdla_fe_cfg, &buffer_settings);

	delete p_drv_cfg_obj;
	return return_code;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::Open(const Configuration & config)
///	\brief	Opens this port, setting its current configuration to the one provided
///	\author	J. Markwordt
///	\date	10/05/2007
///////////////////////////////////////////////////////////////////////////////////////////////
bool SangomaPort::Open(const Configuration & config)
{
	LastError = NO_ERROR_OCCURRED;	 // Reset so if the function is successful GetLastError() will return NO_ERROR_OCCURRED 

	EnterPortCriticalSection();

	PORT_FUNC();
	DBG_OPEN("%s(): CardNumber: %u, PortNumber: %u (mapped port: %u)\n", __FUNCTION__,
		CardNumber, PortNumber, SangomaInterface->get_mapped_port_number());

	DBG_PORT("%s, Framing: %u, lineCoding: %u, HighImpedanceMode: %u\n", 
		(config.e1Ort1 == T1 ? "T1" :
		(config.e1Ort1 == E1 ? "E1" : "Error: Not T1 or E1")),
		config.Framing, 
		config.lineCoding,
		config.HighImpedanceMode);

	// If the port is already opened, return false, it must be Closed first before it can be reopened
	if (IsOpen) {
		PORT_FUNC();
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Open() failed to open, port " << GetPortNumber() << " is already opened.";
		error_msg << "SangomaPort::Close() must be called before attempting to reopen.";
		LastError = error_msg.str();

		// Return false to indicate the open failed, but do not change 
		// IsOpen since port is currently open in a valid configuration
		LeavePortCriticalSection();
		return false;
	}

	// Apply configuration to the port after checking that the configuration is valid.
	if(SetPortConfiguration(config)){
		PORT_FUNC();

		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::SetPortConfiguration() failed for port " << GetPortNumber() << ".";
		LastError = error_msg.str();

		Event.EventHandle = NULL;
		ERR_PORT("SetPortConfiguration() failed! (CardNumber: %u, PortNumber: %u (mapped port: %u))\n",
			CardNumber, PortNumber, SangomaInterface->get_mapped_port_number());
		//axit(1);
		LeavePortCriticalSection();
		return false;
	}

	// Perform any operations necessary to open the port with the new configuration and prepare to receive data
	if(SangomaInterface->init()){
		PORT_FUNC();

		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort: SangomaInterface->init() failed for port " << GetPortNumber() << ".";
		LastError = error_msg.str();

		Event.EventHandle = NULL;
		ERR_PORT("SangomaInterface->init() failed! (CardNumber: %u, PortNumber: %u (mapped port: %u))\n",
			CardNumber, PortNumber, SangomaInterface->get_mapped_port_number());
		//exit(1);
		LeavePortCriticalSection();
		return false;
	}

	if(SangomaInterface->set_buffer_multiplier(&wp_api, ReceiveBufferSettings.BufferMultiplierFactor)){
		PORT_FUNC();

		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort: SangomaInterface->set_buffer_multiplier() failed for port " << GetPortNumber() << ".";
		LastError = error_msg.str();

		Event.EventHandle = NULL;
		ERR_PORT("SangomaInterface->set_buffer_multiplier() failed! (CardNumber: %u, PortNumber: %u (mapped port: %u))\n",
			CardNumber, PortNumber, SangomaInterface->get_mapped_port_number());
		//exit(1);
		LeavePortCriticalSection();
		return false;
	}

	Event.EventHandle = SangomaInterface->get_wait_object_reference();

	// Store the time that this successful Open() occurred, both for statistics
	// purposes and so that we can calculate time elapsed since opening for 
	// determining whether we have acheived syncronization
	time(&PortStatistics.TimeLastOpened);

	DBG_PORT("%s(): PortStatistics.TimeLastOpened:%u (ptr: 0x%p)\n",
		__FUNCTION__, (uint32_t)PortStatistics.TimeLastOpened, &PortStatistics.TimeLastOpened);

	IsOpen = true;
	
	PORT_FUNC();
	// Save the current configuration since it was successfully applied to the port
	CurrentConfiguration = config;		

	LeavePortCriticalSection();
	// Unless the port was successfully opened, IsOpen will still be false
	return IsOpen;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::Close()
///	\brief	Closes this port
///	\return	true if the port was successfully closed, false if
///	\author	J. Markwordt
///	\date	10/05/2007
///////////////////////////////////////////////////////////////////////////////////////////////
bool SangomaPort::Close()
{
	EnterPortCriticalSection();
	PORT_FUNC();

	LastError = NO_ERROR_OCCURRED;	 // Reset so if the function is successful GetLastError() will return NO_ERROR_OCCURRED 

	DBG_OPEN("%s(): CardNumber: %u, PortNumber: %u (mapped port: %u)\n", __FUNCTION__,
		CardNumber, PortNumber, SangomaInterface->get_mapped_port_number());

	// If the port is not currently opened just return true, there is nothing to do
	if (!IsOpen) {
		LeavePortCriticalSection();
		return true;		// this is not an error, there is just nothing to do, already closed
	}

	IsOpen = false;

	///////////////////////////////////////////////////////////////////////////////////////
	// Signal the wait object BEFORE deleting it so anyone who is waiting will not use it anymore
	//sangoma_wait_obj_signal(SangomaInterface->get_wait_object_reference());
	///////////////////////////////////////////////////////////////////////////////////////

	// Perform actions to close and cleanup the port
	bool close_successful = false;
	if(SangomaInterface->cleanup() == 0){
		close_successful = true;
	}else{
		ERR_PORT("CardNumber: %u, PortNumber: %u (mapped port: %u): failed the cleanup()!!\n",
			CardNumber, PortNumber, SangomaInterface->get_mapped_port_number());
	}

	if (close_successful) {
		// Store the time that this successful Close() occurred for statistical purposes
		time(&PortStatistics.TimeLastClosed);
		LeavePortCriticalSection();
		return true;
	}

	LeavePortCriticalSection();
	return false;
}

bool SangomaPort::GetIsOpen()
{
	bool boolTmpIsOpen;

	if(TryEnterPortCriticalSection()){
		//Do NOT wait! We only want to check, not actually wait for another thread to complete its work.
		return false;
	}

	boolTmpIsOpen = IsOpen;
	LeavePortCriticalSection();

	return boolTmpIsOpen;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///		\fn			efficient_32bit_byte_copy(unsigned char **p_destination_buf, unsigned char **p_source_buf, unsigned int num_bytes_to_copy){
///		\brief		Copies data bytes from one byte buffer to another. This routine is optimized
///					for 32 bit processors. This routine does not handle detection of overrunning
///					the buffer boundaries. That responsibility is left to the calling routine.
///
///		\param		**p_destination_buf	: Address of a pointer to the destination byte buffer
///		\param		**p_source_buf		: Address of a pointer to the source byte buffer.
///		\param		num_bytes_to_copy	: Number of bytes to copy from the source buffer to the 
///							destination buffer.
///		\author		Andrew Park
///		\date       07/18/2005
///////////////////////////////////////////////////////////////////////////////////////////////
const unsigned int BITMASK_FOR_LEAST_SIGNIFICANT_TWO_BITS = 0x3;
void efficient_32bit_byte_copy(unsigned char **p_destination_buf, unsigned char **p_source_buf, unsigned int num_bytes_to_copy){
		// We do not need to do a NULL check here, because the function that calls this checks.  NOTE: since this is a public
		// function, there does still exist the possibility that NULL arrays may be passed in, but I didn't want to mess with 
		// the efficiency here if at all possible

		//Transfer bytes in chunks of 32 bit words
		int num_words = static_cast<int>(num_bytes_to_copy >> 2);	//divide by 4 to get the integral # words
		unsigned int *p_destination_buf_32bit = reinterpret_cast<unsigned int*>(*p_destination_buf);
		unsigned int *p_source_buf_32bit = reinterpret_cast<unsigned int*>(*p_source_buf);
		unsigned int i=0;
		for(i=0;i<(unsigned int)num_words;i++){
			*p_destination_buf_32bit++ = *p_source_buf_32bit++;
		}
		//Convert pointers back from 32 bit to byte pointers
		*p_destination_buf = reinterpret_cast<unsigned char*>(p_destination_buf_32bit);
		*p_source_buf = reinterpret_cast<unsigned char*>(p_source_buf_32bit);

		//Transfer remaining bytes individually
		unsigned int remaining_bytes = static_cast<unsigned int>(num_bytes_to_copy & BITMASK_FOR_LEAST_SIGNIFICANT_TWO_BITS);
		for(i=0;i<remaining_bytes;i++) {
			*(*p_destination_buf)++ = *(*p_source_buf)++;
		}
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::Read()
///	\brief	Reads data and places the data in Port's ReceiveBuffer.
///			If ReceiveBuffer is not large enough to contain all of the data in the
///			Sangoma API receive buffer, an error ERROR_INVALID_DESTINATION_BUFFER_TOO_SMALL 
//			will be returned as this is a configuration error.
///	\return	One of the following Status or Error codes:
///			LineDisconnected - Sangoma API inidcated Line Disconnect.
///			LineConnected - Sangoma API inidcated Line Connect.
///			ReadDataAvailable - Sangoma API inidcated there is receive buffer ready in API queue.
///			ERROR_NO_DATA_AVILABLE - lower level code reported no data available in host memory
///			ERROR_INVALID_DESTINATION_BUFFER - a NULL destination buffer was provided so nothing was read
///			ERROR_INVALID_DESTINATION_BUFFER_TOO_SMALL - a destination buffer was provided that
///					is too small to contain a full buffer's worth of data
///			ERROR_PORT_NOT_OPEN - Read attempted before the port had been opened
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	11/15/2007
///////////////////////////////////////////////////////////////////////////////////////////////
int SangomaPort::Read(unsigned int out_flags)
{
	int return_code = ERROR_NO_DATA_AVILABLE;
	
	EnterPortCriticalSection();

	// The port must have been successfully opened with a valid configuration before a read can be performed
	if (!IsOpen) {
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Read(unsigned char*, int) attempted when port " << GetPortNumber() << " had not been opened yet.";
		ERR_PORT("Port %u: SangomaPort::Read() attempted when port had not been opened yet!\n", GetPortNumber());
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_PORT_NOT_OPEN;
	}

	switch(out_flags)//note that 'out_flags' is NOT treated as BitMap anymore - it is expected only a SINGLE bit to be set
	{
	case POLLIN:
		if(SangomaInterface->read_data(&rxhdr, ReceiveBuffer->GetPortBuffer(), ReceiveBuffer->GetPortBufferSize()))
		{
			std::ostringstream error_msg;
			error_msg << "ERROR: read_data() failed! Port: " << GetPortNumber() << " error description: DeviceIoControl() failed\n";
			LastError = error_msg.str();
			LeavePortCriticalSection();
			return ERROR_DEVICE_IOCTL;
		}
		break;
	case POLLPRI:
		if(SangomaInterface->read_event(&wp_api))
		{
			std::ostringstream error_msg;
			error_msg << "ERROR: read_event() failed! Port: " << GetPortNumber() << " error description: DeviceIoControl() failed\n";
			LastError = error_msg.str();
			LeavePortCriticalSection();
			return ERROR_DEVICE_IOCTL;
		}
		break;
	case POLLOUT:
		return_code = TransmitBufferAvailable;
		break;
	default:
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Read(unsigned int out_flags) for port " << GetPortNumber() << " invalid 'out_flags'=" << out_flags << "\n";
		ERR_PORT("Port %u: SangomaPort::Read() invalid out_flags=0x%X!\n", GetPortNumber(), out_flags);
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_INVALID_READ_REQUEST;
	}//switch()

	if (POLLIN == out_flags) 
	{
		switch (rxhdr.operation_status) 
		{
		case SANG_STATUS_RX_DATA_AVAILABLE:
			return_code = ReadDataAvailable;
			ReceiveBuffer->SetUserDataLength(rxhdr.data_length);
			break;
		default:
			ERR_PORT("Port %u: Rx Error: Operation Status: %s (%d)\n", GetPortNumber(),
				SDLA_DECODE_SANG_STATUS(rxhdr.operation_status), rxhdr.operation_status);
			return_code = ERROR_NO_DATA_AVILABLE;
			break;
		}
	}

	if (POLLPRI == out_flags) 
	{
		wp_api_event_t	*wp_tdm_api_event = &wp_api.wp_cmd.event;

		switch (wp_api.wp_cmd.result)
		{
		case SANG_STATUS_SUCCESS:
				
			switch(wp_tdm_api_event->wp_tdm_api_event_type)
			{
			case WP_TDMAPI_EVENT_LINK_STATUS:
				if(wp_tdm_api_event->wp_tdm_api_event_link_status == WAN_EVENT_LINK_STATUS_CONNECTED){
					return_code = LineConnected;
				}else{
					return_code = LineDisconnected;
				}
				break;
			case WP_TDMAPI_EVENT_ALARM:
				if(wp_tdm_api_event->wp_tdm_api_event_alarm == 0){
					return_code = LineConnected;
				}else{
					return_code = LineDisconnected;
				}
				break;
			default:
				//FIXME: Remove this
				ERR_PORT("Port %u INVALID EVENT Type 0x%X\n",GetPortNumber(),wp_tdm_api_event->wp_tdm_api_event_type);
				exit(1);
			}
			break;

		default:

			if (rxhdr.operation_status == SANG_STATUS_RX_DATA_AVAILABLE) {
				return_code = ReadDataAvailable;
			} else {
				ERR_PORT("Port %u: Event Error: Operation Status: %s (%d)\n", GetPortNumber(),
					SDLA_DECODE_SANG_STATUS(rxhdr.operation_status), rxhdr.operation_status);
				return_code = ERROR_NO_DATA_AVILABLE;
			}
			//FIXME: Remove this
			ERR_PORT("Port %u INVALID EVENT\n",GetPortNumber());
			exit(1);
			break;
		}
	}

	if(return_code == ReadDataAvailable){
		PortStatistics.BytesReceived += ReceiveBuffer->GetUserDataLength();
	}

	LeavePortCriticalSection();
	return return_code;
}


///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::Write(unsigned char* p_source_buffer, const int bytes_to_write)
///	\brief	Writes the specified number of bytes from the source buffer to the port
///	\param	p_source_buffer		Pointer to an array of unsigned chars used as the source of the
///								data to write.  Assumes that it contains at least bytes_to_write
///								number of bytes.
///	\param	bytes_to_write		the number of bytes to write to the port
///	\return	Zero if successful (all bytes are written) or one of the following error codes:
///			ERROR_NO_DATA_WRITTEN - no data was able to be written on the port
///			ERROR_INVALID_SOURCE_BUFFER - a NULL source buffer was provided so nothing was written
///			ERROR_PORT_NOT_OPEN - Write attempted before the port had been opened
///	\author	J. Markwordt
///	\date	10/15/2007
///////////////////////////////////////////////////////////////////////////////////////////////
int SangomaPort::Write(unsigned char* p_source_buffer, const unsigned int bytes_to_write)
{
	EnterPortCriticalSection();
	//PORT_FUNC();
	
	// The port must have been successfully opened with a valid configuration before a write can be performed
	if (!IsOpen) {
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Write(unsigned char*, int) attempted when port " << GetPortNumber() << " had not been opened yet.";
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_PORT_NOT_OPEN;
	}

	// Check to make sure the source is valid before attempting to access it
	bool invalid_source_buffer = (NULL == p_source_buffer);
	if (invalid_source_buffer) {
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Write(unsigned char*, int) attempted with invalid source buffer on port " << GetPortNumber() << ".";
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_INVALID_SOURCE_BUFFER;
	}

	// Perform Sangoma API functions to write data to the port.
	if(bytes_to_write > TransmitBuffer->GetPortBufferSize()){
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Write(unsigned char*, int) attempted with Tx data length exceeding maximum " << GetPortNumber() << ".";
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_INVALID_TX_DATA_LENGTH;
	}
	
	TransmitBuffer->SetUserDataLength(bytes_to_write);
	//
	//If src and dst buffers are the same, there is no need to copy the data because
	//it is already there. This is the case for TxFile (see code in WriteChunkOfTxFile()). 
	//
	if (TransmitBuffer->GetUserDataBuffer() != p_source_buffer) {
		memcpy(TransmitBuffer->GetUserDataBuffer(), p_source_buffer, bytes_to_write);
	}

	int number_of_bytes_written = 0;

	memset(&txhdr, 0x00, sizeof(txhdr));

#if 0
	if (GetPortNumber() == 0) {
		print_data_buffer((unsigned char*)TransmitBuffer->GetUserDataBuffer(), TransmitBuffer->GetUserDataLength());
	}
#endif

	if(SangomaInterface->transmit(&txhdr, TransmitBuffer->GetUserDataBuffer(), TransmitBuffer->GetUserDataLength())){
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::" << __FUNCTION__ << "() failed! Port: " << GetPortNumber() << " error description: DeviceIoControl() failed";
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_DEVICE_IOCTL;
	}

	std::ostringstream error_msg;

	switch(txhdr.operation_status)
	{
	case SANG_STATUS_SUCCESS:
		PortStatistics.BytesSent += bytes_to_write;
		number_of_bytes_written = bytes_to_write;
		break;
	case SANG_STATUS_TX_TIMEOUT:
		error_msg << "ERROR: SangomaPort::" << __FUNCTION__ << "() failed! Port: " << GetPortNumber() << " error description: TX_TIMEOUT";
		break;
	case SANG_STATUS_TX_DATA_TOO_LONG:
		error_msg << "ERROR: SangomaPort::" << __FUNCTION__ << "() failed! Port: " << GetPortNumber() << " error description: TX_DATA_TOO_LONG";
		break;
	case SANG_STATUS_TX_DATA_TOO_SHORT:
		error_msg << "ERROR: SangomaPort::" << __FUNCTION__ << "() failed! Port: " << GetPortNumber() << " error description: TX_DATA_TOO_SHORT";
		break;
	case SANG_STATUS_LINE_DISCONNECTED:
		error_msg << "ERROR: SangomaPort::" << __FUNCTION__ << "() failed! Port: " << GetPortNumber() << " error description: PORT_OUT_OF_SYNC";
		break;
	default:
		break;
	}//switch()

	bool no_data_written = (0 == number_of_bytes_written);
	if (no_data_written) {
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_NO_DATA_WRITTEN;
	}
		
	LeavePortCriticalSection();
	return 0;// ALL bytes were written
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetSynchronizationStatus()
///	\brief	Closes this port
///	\return	true if the port was successfully closed, false if
///	\author	J. Markwordt
///	\date	10/05/2007
///////////////////////////////////////////////////////////////////////////////////////////////
SynchronizationStatus SangomaPort::GetSynchronizationStatus()
{
	EnterPortCriticalSection();
	PORT_FUNC();

	// If the port is not currently open with a valid configuration
	// the port cannot be in sync, simply return not in sync.
	if (!IsOpen) {
		PORT_FUNC();
		LeavePortCriticalSection();
		return PORT_NOT_IN_SYNC;
	}

	// Else we need to know if enough time has elapsed to be able to reliably
	// check for alarms.  If ALARM_WAIT_TIME_MS has not passed, the Sangoma card
	// will always return no alarms, giving a false positive that we have found
	// synchronization, when in fact we have not.
	time_t CurrentTime;
	time(&CurrentTime);

	DBG_PORT("%s(): CurrentTime:%u, PortStatistics.TimeLastOpened:%u (ptr: 0x%p)\n",
		__FUNCTION__, (uint32_t)CurrentTime, (uint32_t)PortStatistics.TimeLastOpened, &PortStatistics.TimeLastOpened);

#if 0
	time_t t_time_diff = CurrentTime - PortStatistics.TimeLastOpened;
	DBG_PORT("%s(): t_time_diff: %d\n", __FUNCTION__, t_time_diff);
	//translate time differemce into milliseconds
	t_time_diff *= 1000;
	bool ready_to_check_for_alarms = (t_time_diff >= ALARM_WAIT_TIME_MS);
#else
	double t_time_diff = difftime(CurrentTime, PortStatistics.TimeLastOpened);
	DBG_PORT("%s(): t_time_diff: %f\n", __FUNCTION__, t_time_diff);
	bool ready_to_check_for_alarms = (t_time_diff >= ALARM_WAIT_TIME_MS / 1000 / 2 /* allow to poll after 1/2 of maximum wait time (approximately 16 seconds) */);
#endif

	DBG_PORT("%s(): ready_to_check_for_alarms: %d\n", __FUNCTION__, ready_to_check_for_alarms);

	if (ready_to_check_for_alarms) {
		// Check to see if there are alarms present.  We aren't interested in
		// what individual ports are present or saving them, just see if any 
		// are present.
		char alarms_present = SangomaInterface->alarms_present();
		if (alarms_present) {
			sync_cnt=0;
			if (t_time_diff >= ALARM_WAIT_TIME_MS / 1000) {
				// Set the LastError string to indicate that 
				std::ostringstream error_msg;
				error_msg << "ERROR: SangomaPort::GetSynchronizationStatus() detected alarms present on port " << GetPortNumber() << " with the current configuration.  Closing the port.";
				LastError = error_msg.str();

				LeavePortCriticalSection();

				// DAVIDR: reached maximum wait time.
				// The *caller* will decide what to do with the port: to close it or not.
				return PORT_NOT_IN_SYNC;
			} else {
				// DAVIDR: the maximum wait time was not reached yet, keep waiting for sync.
				LeavePortCriticalSection();
				return PORT_WAITING_FOR_SYNC;
			}
		} else {
			PortStatistics.TimeLastFoundSync = CurrentTime;
			sync_cnt++;
			LeavePortCriticalSection();
			return PORT_IN_SYNC;
		}

	}else{
		PORT_FUNC();
	}

	LeavePortCriticalSection();
	return PORT_WAITING_FOR_SYNC;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetPortNumber() const
///	\brief	Returns this port's number (zero indexed, relative to the card)
///	\return	This port's number (zero indexed, relative to the card)
///	\author	J. Markwordt
///	\date	10/15/2007
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int SangomaPort::GetPortNumber() const
{
	return PortNumber;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetUnmappedPortNumber()
///	\brief	Returns this port's number UNMAPPED (Without G3 port mapping). Use for debugging only.
///	\return	This port's UNMAPPED number  (zero indexed, relative to the card)
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	1/10/2008
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int SangomaPort::GetUnmappedPortNumber()
{
	unsigned int unmapped_port = 0;
	EnterPortCriticalSection();
	if(SangomaInterface != NULL){
		unmapped_port = SangomaInterface->get_mapped_port_number();
	}
	LeavePortCriticalSection();
	return 	unmapped_port;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetConfiguration() const
///	\brief	Returns the current configuration of the port
///	\return	Current Configuration structure for the port
///	\author	J. Markwordt
///	\date	10/15/2007
///////////////////////////////////////////////////////////////////////////////////////////////
Configuration SangomaPort::GetConfiguration() const
{
	PORT_FUNC();
	return CurrentConfiguration;
}
	
///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetEvent() const
///	\brief	Return the PortEvent associated with this port
///	\return	PortEvent structure for events associated with this port
///	\author	J. Markwordt
///	\date	10/18/2007
///////////////////////////////////////////////////////////////////////////////////////////////
PortEvent SangomaPort::GetEvent() const
{
	return Event;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetDriverVersion() const
///	\brief	Returns this port's hardware abstraction driver version number.
///
///			Sangoma has given the card a driver and the port what Sangoma refers to as a "Hardware 
///			Abstraction Driver", and although these two should have the same version number if 
///			installed properly, they are in fact two separate drivers.
///
///			This function is to retrieve the driver associated with the port's hardware abstraction
///			driver and can be used to verify that it matches the version number of the card as a whole.
///	\return	DriverVersion containing the version of the software abstraction driver associated
///			with this port.
///	\author	J. Markwordt
///	\date	10/17/2007
///////////////////////////////////////////////////////////////////////////////////////////////
DriverVersion SangomaPort::GetDriverVersion() /* DAVIDR: removed 'const' to get rid of compiler errors */
{
	DriverVersion software_abstraction_driver_version;
	wan_driver_version_t drv_version;

	if(SangomaInterface->get_api_driver_version(&drv_version)){
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::%s()" << __FUNCTION__ << " failed! Port: " << GetPortNumber();
		LastError = error_msg.str();
	}

	//translate low level version structure into high level version structure
	software_abstraction_driver_version.Major = drv_version.major;
	software_abstraction_driver_version.Minor = drv_version.minor;
	software_abstraction_driver_version.Build = drv_version.minor1;
	software_abstraction_driver_version.Revision = drv_version.minor2;

	return software_abstraction_driver_version;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetStatistics()
///	\brief	Get the statistics for this port.
///
///			The reason this function is not declared constant is that the Statistics strucure
///			that is returned contains both statisics that are maintained in software by this 
///			class, and statistics that are maintained in hardware on the port.  We do not want
///			to continuously poll to keep these statistics up to date, so when requested, we will
///			issue low level commands to fill in these statistics.
///	\return	Statistics structure containing information related to this port
///	\author	J. Markwordt
///	\date	10/17/2007
///////////////////////////////////////////////////////////////////////////////////////////////
Statistics SangomaPort::GetStatistics()
{
	///		  Perform low level calls to retrieve the net_device_stats_t structure and update 
	///		  PortStatistics with the statistics we are interested in.  For example,
	///		  net_device_stats_t.rx_bytes maps to Statistics.BytesReceived.  Statistics maintained
	///		  by the SangomaPort class (such as TimeLastOpened, TimeLastFoundSync, etc.) will be
	///		  returned as is.
	wanpipe_chan_stats_t stats;

	EnterPortCriticalSection();
	if (IsOpen) 
	{
		SangomaInterface->operational_stats(&stats);
		//translate low level statistics structure into high level statistics structure
		PortStatistics.BytesReceived = stats.rx_bytes;
		PortStatistics.BytesSent = stats.tx_bytes;
		PortStatistics.Errors = stats.errors;
	}
	LeavePortCriticalSection();
	return PortStatistics;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::ClearStatistics()
///	\brief	Clear the statistics for this port
///	\author	J. Markwordt
///	\date	10/17/2007
///////////////////////////////////////////////////////////////////////////////////////////////
void SangomaPort::ClearStatistics()
{
	/// \todo Reset the PortStatistics member to default values

	/// Issue low level command to clear the statistics maintained on the port
	EnterPortCriticalSection();
	if (IsOpen) 
	{
		SangomaInterface->flush_operational_stats();
	}
	LeavePortCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::FlushBuffers()
///	\brief	Clear the statistics for this port
///	\author	J. Markwordt
///	\date	10/17/2007
///////////////////////////////////////////////////////////////////////////////////////////////
void SangomaPort::FlushBuffers()
{
	/// \todo Reset the PortStatistics member to default values

	/// Issue low level command to clear the statistics maintained on the port
	EnterPortCriticalSection();
	if (IsOpen) 
	{
		SangomaInterface->flush_buffers();
	}
	LeavePortCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetAlarms() const
///	\brief	Retrieves the alarms from the hardware for this port
///	\return	Alarms structure containing indications of the current alarms
///	\author	J. Markwordt
///	\date	10/17/2007
///////////////////////////////////////////////////////////////////////////////////////////////
Alarms SangomaPort::GetAlarms()
{
	Alarms alarms(FALSE);
	unsigned long u_alarms = 1;
	unsigned char adapter_type;

	EnterPortCriticalSection();
	if (IsOpen) 
	{
		adapter_type = SangomaInterface->get_adapter_type();
		SangomaInterface->read_te1_56k_stat(&u_alarms);
		/// translate low level alarms structure into high level alarms structure
		if (adapter_type == WAN_MEDIA_T1 || adapter_type == WAN_MEDIA_E1){
			if(!strcmp(WAN_TE_LOS_ALARM(u_alarms), ON_STR)){
				alarms.LossOfSignal = TRUE;
			}
			if(!strcmp(WAN_TE_ALOS_ALARM(u_alarms), ON_STR)){
				alarms.AlternateLossOfSignalStatus = TRUE;
			}
			if(!strcmp(WAN_TE_RED_ALARM(u_alarms), ON_STR)){
				alarms.Red = TRUE;
			}
			if(!strcmp(WAN_TE_AIS_ALARM(u_alarms), ON_STR)){
				alarms.AlarmIndicationSignal = TRUE;
			}
			if(!strcmp(WAN_TE_OOF_ALARM(u_alarms), ON_STR)){
				alarms.OutOfFrame = TRUE;
			}
			if (adapter_type == WAN_MEDIA_T1){ 
				if(!strcmp(WAN_TE_YEL_ALARM(u_alarms), ON_STR)){
					alarms.Yellow = TRUE;
				}
			}

			if(!strcmp(WAN_TE_OOSMF_ALARM(u_alarms), ON_STR)){
				alarms.LossOfSignalingMultiframe = TRUE;
			}
			if(!strcmp(WAN_TE_OOCMF_ALARM(u_alarms), ON_STR)){
				alarms.LossOfCrcMultiframe = TRUE;
			}
			if(!strcmp(WAN_TE_OOOF_ALARM(u_alarms), ON_STR)){
				alarms.OutOfOfflineFrame = TRUE;
			}
			if(!strcmp(WAN_TE_RAI_ALARM(u_alarms), ON_STR)){
				alarms.ReceiveLossOfSignalV = TRUE;
			}
		}
	}
	LeavePortCriticalSection();
	return alarms;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetLastError() const
///	\brief	Returns a description of the last error that occurred 
///	\return Returns a description of the last error that occurred (NO_ERROR_OCCURRED if the last command was successful)
///	\author	J. Markwordt
///	\date	10/17/2007
///////////////////////////////////////////////////////////////////////////////////////////////
std::string SangomaPort::GetLastError() const
{
	return LastError;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetRxDataBuffer()
///	\brief	Returns a pointer to received data.
///	\return Returns a pointer to received data after a successful Read()
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	1/10/2008
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned char *SangomaPort::GetRxDataBuffer()
{
	return ReceiveBuffer->GetUserDataBuffer();
}

////////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetRxDataLength()
///	\brief	Returns length of received data.
///	\return Returns length of received data after a successful Read().
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	1/10/2008
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int SangomaPort::GetRxDataLength()
{
	return ReceiveBuffer->GetUserDataLength();
}

////////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::EnterPortCriticalSection()
///	\brief	Enter a SangomaPort operation in thread-safe manner.
///	\return No return value.
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	1/10/2008
///////////////////////////////////////////////////////////////////////////////////////////////
void SangomaPort::EnterPortCriticalSection()
{
	EnterCriticalSection(&cs_CriticalSection);
}

////////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::LeavePortCriticalSection()
///	\brief	Leave a SangomaPort operation in thread-safe manner.
///	\return No return value.
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	1/10/2008
///////////////////////////////////////////////////////////////////////////////////////////////
void SangomaPort::LeavePortCriticalSection()
{
	LeaveCriticalSection(&cs_CriticalSection);
}


////////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::TryEnterPortCriticalSection()
///	\brief	Try to enter a SangomaPort operation in thread-safe manner.
///	\return 1 - the mutex already taken, return without waiting 0 - the mutex was free
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	6/2/2010
///////////////////////////////////////////////////////////////////////////////////////////////
int SangomaPort::TryEnterPortCriticalSection()
{
	if (! TryEnterCriticalSection(&cs_CriticalSection) )
	{
		//If another thread already owns the critical section, the return value of TryEnterCriticalSection() is zero.
		return 1;
	}
	return 0;
}

int SangomaPort::SetAndOpenTxFile(char *szTxFileName)
{
	pTxFile = fopen(szTxFileName, "rb" );
	if( pTxFile == NULL){
		ERR_PORT("Can't open file: [%s]\n", szTxFileName);
		return 1;
	}

	return 0;
}
	
int SangomaPort::WriteChunkOfTxFile()
{
	unsigned char *user_tx_buffer = NULL;

	if (!pTxFile) {
		ERR_PORT("TxFile is NOT open!\n");
		return 1;
	}

	user_tx_buffer = this->TransmitBuffer->GetUserDataBuffer();

	//Initialize the buffer to 0xFF, so when End-of-File is reached, 
	//the padding up-to-end of tx buffer is automatic.
	memset( user_tx_buffer, 0xFF, this->TransmitBuffer->GetUserDataLength() );

	if (feof( pTxFile )) {
		//End-of-File reached. Re-wind the file to zero.
		fseek( pTxFile, 0, SEEK_SET );
	}

	//Read tx data from the file.
	fread( user_tx_buffer, 1, TxMtu, pTxFile );

	return this->Write(user_tx_buffer, TxMtu);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::LineLoopbackEnable()
///	\brief	Enable line loopback for testing. All transmitted data will be also received
//			locally. Requires the Port to be in Master clock mode.
///	\return	0 - success, non-zero - error
///	\author	David Rokhvarg
///	\date	01/27/2011
///////////////////////////////////////////////////////////////////////////////////////////////
int SangomaPort::LineLoopbackEnable()
{
	int rc = SangomaInterface->loopback_command(WAN_TE1_DDLB_MODE, WAN_TE1_LB_ENABLE, ENABLE_ALL_CHANNELS);

	if (rc) {
		ERR_PORT("Port %u: %s() failed! rc: %d\n", GetPortNumber(), __FUNCTION__, rc);
	}

	return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::LineLoopbackDisable()
///	\brief	Disable line loopback which was enabled by SangomaPort::LineLoopbackEnable()
///	\return	0 - success, non-zero - error
///	\author	David Rokhvarg
///	\date	01/27/2011
///////////////////////////////////////////////////////////////////////////////////////////////
int SangomaPort::LineLoopbackDisable()
{
	int rc = SangomaInterface->loopback_command(WAN_TE1_DDLB_MODE, WAN_TE1_LB_DISABLE, ENABLE_ALL_CHANNELS);

	if (rc) {
		ERR_PORT("Port %u: %s() failed! rc: %d\n", GetPortNumber(), __FUNCTION__, rc);
	}

	return rc;

}

#if defined(__WINDOWS__)
# ifndef _DEBUG				
#  pragma optimize("2gt",off)// Windows specific (MS_SPECIFIC), disables optimization in Release mode
# endif
#endif

