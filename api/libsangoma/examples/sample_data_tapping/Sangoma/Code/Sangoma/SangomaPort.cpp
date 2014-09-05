#if defined WIN32 && defined NDEBUG
#pragma optimize("gt",on)
#endif

#include <algorithm>
#include <limits>
#include <sstream>
#include <time.h>
#include "SangomaPort.h"
#include "sangoma_interface.h"
#include "driver_configurator.h"

#ifdef linux
#include <math.h>
#include <float.h>
#endif

#ifndef INT_MAX
#define INT_MAX         ((int)(~0U>>1))
#endif

#define DBG_PORT	if(0)printf
#define INFO_PORT	if(0)printf
//This macro is for Error reporting and should NOT be disabled.
#define ERR_PORT	if(0)printf("Error:%s():line:%d: ", __FUNCTION__, __LINE__);if(0)printf

#define DBG_OPEN	if(0)printf
#define PORT_FUNC()	if(0)printf("%s(): Line: %u\n", __FUNCTION__, __LINE__)

extern void print_data_buffer(unsigned char *buffer, int buffer_length);

using namespace Sangoma;

///////////////////////////////////////////////////////////////////////
/// \fn         NotWhitespace(const char character)
/// \brief      Determines whether the specified character is a
///             non-whitespace character.
/// \param[in]  character - The character to be examined.
/// \return     True if the character is a non-whitespace character
///             or false if the character is a whitespace character.
/// \author     Kyle Hickernell
/// \date       06/07/2011
///////////////////////////////////////////////////////////////////////
bool NotWhitespace(const char character)
{
    const int NON_WHITE_SPACE_RETURN_CODE = 0;
    bool character_is_whitespace = (NON_WHITE_SPACE_RETURN_CODE != isspace(character));
    return !character_is_whitespace;
}

///////////////////////////////////////////////////////////////////////
/// \fn			ToFloat(const std::string& string_to_convert, float& float_value)
/// \brief		Converts a string to a float.
/// \param[in]	string_to_convert - The string to convert.
/// \param[out] float_value - The converted float. If this function returns false,
///             then this data should be considered invalid.
/// \return		True if the conversion succeeded. False otherwise.
/// \author		Luke Roling
/// \date		09/16/2011
///////////////////////////////////////////////////////////////////////
bool ToFloat(const std::string& string_to_convert, float& float_value)
{
    // RESET THE OUTPUT PARAMETER TO A DEFAULT VALUE.
    float_value = 0;

    // CONVERT THE STRING TO A DOUBLE.
    // Set errno to 0 for error checking.
#if defined WIN32
    const int DEFAULT_ERRNO_VALUE = 0;
    errno_t set_errno_result = _set_errno(DEFAULT_ERRNO_VALUE);
    const int SET_ERRNO_SUCCESS = 0;
    bool set_errno_successfully = (SET_ERRNO_SUCCESS == set_errno_result);
    if(!set_errno_successfully)
    {
        // Return since the errno value could not be set.
        return false;
    }
#elif defined LINUX
    errno = 0;
#endif

    char* remaining_characters;
    double double_value = strtod(string_to_convert.c_str(), &remaining_characters);

    // VERIFY THAT THE CONVERSION WAS SUCCESSFUL.
    // Check if valid conversion occurred.
    const double POSSIBLE_CONVERSION_ERROR_OCCURRED = 0.0;
    bool possible_conversion_error_occurred = (POSSIBLE_CONVERSION_ERROR_OCCURRED == double_value);
    if (possible_conversion_error_occurred)
    {
        // Check if 0 was parsed.
        bool string_to_convert_valid = (string_to_convert.c_str() != remaining_characters);
        if (!string_to_convert_valid)
        {
            // The conversion failed due to the input string not containing valid digits, so exit here.
            return false;
        }
    }

    // Check if number parsed is out of range.
    bool double_value_in_range = (ERANGE != errno);
    if (!double_value_in_range)
    {
        // Conversion failed because the value was out of a Double's range, so exit here.
        return false;
    }

    // CHECK IF THERE ARE TRAILING INVALID CHARACTERS.
    // Since trailing whitespace characters are acceptable, the only
    // characters we are looking for are non-whitespace characters.
    std::string remaining_string = remaining_characters;
    std::string::iterator non_whitespace_character_index = std::find_if(
        remaining_string.begin(),
        remaining_string.end(),
        NotWhitespace);
    bool trailing_characters_valid = (remaining_string.end() == non_whitespace_character_index);
    if (!trailing_characters_valid)
    {
        // Conversion failed because invalid trailing characters were found, so exit here.
        return false;
    }

    // CHECK IF DOUBLE CAN BE CONVERTED TO FLOAT.
    // A float contains three different ranges. The first two ranges contain the possible positive
    // and negative values a float can represent. The third range is the value zero which does not
    // fall into the other two ranges. The diagram below shows the ranges on a number line.
    //
    //        Float -Max            Float -Min                     Float Min             Float Max
    //           |---------------------|               |              |---------------------|
    //      |-------------------------------|          |         |-------------------------------|
    //  Double -Max                     Double -Min    0      Double Min                      Double Max
    //  <--------------------------------------Real Number line--------------------------------------->

    // Check if double is in the positive float range.
    const double MAX_POSITIVE_FLOAT = FLT_MAX;
    const double MIN_POSITIVE_FLOAT = FLT_MIN;
    bool double_value_is_in_valid_positive_range = ((double_value >= MIN_POSITIVE_FLOAT) && (double_value <= MAX_POSITIVE_FLOAT));

    // Check if the double is zero which a float can represent.
    const double FLOAT_ZERO = 0;
    bool double_value_is_equal_to_zero = (double_value == FLOAT_ZERO);

    // Check if double is in the negative float range.
    const double MAX_NEGATIVE_FLOAT = -MAX_POSITIVE_FLOAT;
    const double MIN_NEGATIVE_FLOAT = -MIN_POSITIVE_FLOAT;
    bool double_value_is_in_valid_negative_range = ((double_value <= MIN_NEGATIVE_FLOAT) && (double_value >= MAX_NEGATIVE_FLOAT));

    // Check if double falls into one of the float ranges.
    bool double_to_float_conversion_possible = (
        double_value_is_in_valid_positive_range ||
        double_value_is_equal_to_zero ||
        double_value_is_in_valid_negative_range);
    if(!double_to_float_conversion_possible)
    {
        // Conversion failed because the double could not be converted to a float, so exit here.
        return false;
    }

    // Conversion was successful.
    float_value = static_cast<float>(double_value);
    return true;
}

const unsigned int BufferSettings::MAXIMUM_BUFFER_MULTIPLIER_FACTOR = MAX_BUFFER_MULTIPLIER_FACTOR;
const unsigned int BufferSettings::MINIMUM_BUFFER_MULTIPLIER_FACTOR = MIN_BUFFER_MULTIPLIER_FACTOR;
const unsigned int BufferSettings::MINIMUM_NUMBER_OF_BUFFERS_PER_API_INTERFACE = MIN_NUMBER_OF_BUFFERS_PER_API_INTERFACE;

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
///	\fn		DriverVersion::ToString()
///	\brief	Converts the DriverVersion to a string.
///	\return	The string representation of the driver version (e.g. "6.0.6.3")
///	\author	Kyle Hickernell
///	\date	03/21/2011
///////////////////////////////////////////////////////////////////////////////////////////////
std::string DriverVersion::ToString()
{
	std::ostringstream driver_version;
	driver_version << Major << "." << Minor << "." << Build << "." << Revision;
	return driver_version.str();
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
///		        const BufferSettings & receive_buffer_settings,
///		        const BufferSettings & transmit_buffer_settings)
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
///	\param	receive_buffer_settings			BufferSettings to configure the host memory buffers
///											for receive operations on this port (set NumberOfBuffersPerPort
///											to 0 to disable receiving data on this port)
///	\param	transmit_buffer_settings		BufferSettings to configure the host memory buffers
///											for transmit operations on this port (set NumberOfBuffersPerPort
///											to 0 to disable transmitting data on this port)
///	\author	J. Markwordt
///	\date	10/04/2007
///////////////////////////////////////////////////////////////////////////////////////////////
SangomaPort::SangomaPort(const unsigned int card_number, const unsigned int port_number,
		const BufferSettings & receive_buffer_settings,
		const BufferSettings & transmit_buffer_settings)
:	pReceiveBuffer(NULL),
    pTransmitBuffer(NULL),
    RxDiscardCounter(0),
	RxCount(0),
	SyncCount(0),
	RxLength(0),
    CardNumber(card_number),
	PortNumber(port_number),
    Event(),
	IsOpen(false),
	CurrentConfiguration(),
	PortStatistics(),
    LastError(NO_ERROR_OCCURRED),
    ReceiveBufferSettings(receive_buffer_settings),
    TransmitBufferSettings(transmit_buffer_settings),
    pSangomaInterface(NULL),
    cs_CriticalSection(),
    rxhdr(),
    txhdr(),
    wp_api(),
    pTxFile(NULL),
    TxMtu(INT_MAX)
{
	PORT_FUNC();

	DBG_PORT("%s(): card_number: %u, port_number: %u\n", __FUNCTION__, card_number, port_number);

	InitializeCriticalSection(&cs_CriticalSection);

	pSangomaInterface = new sangoma_interface(CardNumber, PortNumber);
	DBG_PORT("pSangomaInterface: 0x%p\n", pSangomaInterface);

	rxhdr = new wp_api_hdr_t();
	txhdr = new wp_api_hdr_t();
	wp_api = new wanpipe_api_t();

	// Calculate the size of a host memory buffer by multiplying the maximum buffer size as defined
	// by the Sangoma API by a multiplier (buffer_multiplier_factor) for both the receive and transmit buffers
	pReceiveBuffer = new PortBuffer(sizeof(RX_DATA_STRUCT) * ReceiveBufferSettings.BufferMultiplierFactor);
	pTransmitBuffer = new PortBuffer(sizeof(TX_DATA_STRUCT) * TransmitBufferSettings.BufferMultiplierFactor);
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

	// If the has not been properly closed, close it
	if (IsOpen) {
		Close();
	}

	delete rxhdr;
	delete txhdr;
	delete wp_api;

	// Deallocate Rx/Tx Port buffers
	delete pReceiveBuffer;
	delete pTransmitBuffer;

	if(pSangomaInterface != NULL){
		delete pSangomaInterface;
		pSangomaInterface = NULL;
	}

	if (pTxFile) {
		fclose( pTxFile );
	}

	PORT_FUNC();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::SetPortConfiguration(const Configuration & config)
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
	if(p_drv_cfg_obj->init(CardNumber, pSangomaInterface->get_mapped_port_number())){
		error_msg << "ERROR: " << __FUNCTION__ << "(): failed to initialize Sangoma Driver Configuration object";
		LastError = error_msg.str();
		delete p_drv_cfg_obj;
		return 1;
	}

	memset(&sdla_fe_cfg, 0x00, sizeof(sdla_fe_cfg_t));
	switch(config.E1OrT1)
	{
	case T1:
		FE_LBO(&sdla_fe_cfg) = WAN_T1_LBO_0_DB;//important to set to a valid value for both T1 and E1!!
		FE_MEDIA(&sdla_fe_cfg) = WAN_MEDIA_T1;
		switch(config.LineCoding)
		{
		case OFF:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_AMI;
			break;
		case ON:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_B8ZS;
			break;
		default:
			error_msg << "ERROR: " << __FUNCTION__ << "(): T1 invalid lineCoding " << config.LineCoding;
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
		switch(config.LineCoding)
		{
		case OFF:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_AMI;
			break;
		case ON:
			FE_LCODE(&sdla_fe_cfg) = WAN_LCODE_HDB3;
			break;
		default:
			error_msg << "ERROR: " << __FUNCTION__ << "(): E1 invalid lineCoding " << config.LineCoding;
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

	p_drv_cfg_obj->set_dchan(config.dchan,config.dchan_seven_bit,config.dchan_mtp1_filter);
	p_drv_cfg_obj->set_chunk_ms(config.chunk_ms);
	p_drv_cfg_obj->set_span_api_mode();

	switch (config.api_mode) {
	case SNG_SPAN_MODE:
		p_drv_cfg_obj->set_span_api_mode();
		break;
	case SNG_CHAN_MODE:
		p_drv_cfg_obj->set_chan_api_mode();
		buffer_settings.buffer_multiplier_factor=1;
		ReceiveBufferSettings.BufferMultiplierFactor=0;
		break;
	case SNG_DATA_MODE:
		p_drv_cfg_obj->set_data_api_mode();
		buffer_settings.buffer_multiplier_factor=1;
		ReceiveBufferSettings.BufferMultiplierFactor=0;
		break;
	}

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
		CardNumber, PortNumber, pSangomaInterface->get_mapped_port_number());

	DBG_PORT("%s, Framing: %u, lineCoding: %u, HighImpedanceMode: %u\n",
		(config.E1OrT1 == T1 ? "T1" :
		(config.E1OrT1 == E1 ? "E1" : "Error: Not T1 or E1")),
		config.Framing,
		config.LineCoding,
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
			CardNumber, PortNumber, pSangomaInterface->get_mapped_port_number());
		//axit(1);
		LeavePortCriticalSection();
		return false;
	}

	// Perform any operations necessary to open the port with the new configuration and prepare to receive data
	if(pSangomaInterface->init()){
		PORT_FUNC();

		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort: pSangomaInterface->init() failed for port " << GetPortNumber() << ".";
		LastError = error_msg.str();

		Event.EventHandle = NULL;
		ERR_PORT("pSangomaInterface->init() failed! (CardNumber: %u, PortNumber: %u (mapped port: %u))\n",
			CardNumber, PortNumber, pSangomaInterface->get_mapped_port_number());
		//exit(1);
		LeavePortCriticalSection();
		return false;
	}

	if(ReceiveBufferSettings.BufferMultiplierFactor &&
       pSangomaInterface->set_buffer_multiplier(wp_api, ReceiveBufferSettings.BufferMultiplierFactor)){
		PORT_FUNC();

		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort: pSangomaInterface->set_buffer_multiplier() failed for port " << GetPortNumber() << ".";
		LastError = error_msg.str();

		Event.EventHandle = NULL;
		ERR_PORT("pSangomaInterface->set_buffer_multiplier() failed! (CardNumber: %u, PortNumber: %u (mapped port: %u))\n",
			CardNumber, PortNumber, pSangomaInterface->get_mapped_port_number());
		//exit(1);
		LeavePortCriticalSection();
		return false;
	}

	Event.EventHandle = pSangomaInterface->get_wait_object_reference();

	// Store the time that this successful Open() occurred, both for statistics
	// purposes and so that we can calculate time elapsed since opening for
    // determining whether we have achieved synchronization.
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
		CardNumber, PortNumber, pSangomaInterface->get_mapped_port_number());

	// If the port is not currently opened just return true, there is nothing to do
	if (!IsOpen) {
		LeavePortCriticalSection();
		return true;		// this is not an error, there is just nothing to do, already closed
	}

	IsOpen = false;

	///////////////////////////////////////////////////////////////////////////////////////
	// Signal the wait object BEFORE deleting it so anyone who is waiting will not use it anymore
	//sangoma_wait_obj_signal(pSangomaInterface->get_wait_object_reference());
	///////////////////////////////////////////////////////////////////////////////////////

	// Perform actions to close and cleanup the port
	bool close_successful = false;
	if(pSangomaInterface->cleanup() == 0){
		close_successful = true;
	}else{
		ERR_PORT("CardNumber: %u, PortNumber: %u (mapped port: %u): failed the cleanup()!!\n",
			CardNumber, PortNumber, pSangomaInterface->get_mapped_port_number());
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
///	\fn		    SangomaPort::Read(unsigned int event_flag)
///	\brief	    When the wait object for this port is signaled, this method is used to process the
///             result. If the flag indicates a read event, then the data will be placed in the port's
///             receive buffer. If the flag indicates a write event, then the method returns that a
///             transmit buffer is available. If the flag indicates a status event, then the status
///             event type will be returned. On error, the return value describes the error that occurred.
///
/// \param[in]  event_flag - An integer value representing the type of event to process. The value may
///             be either POLLIN, POLLOUT, or POLLPRI.
/// \return     For each available event, the method's return value is defined below.
///
///             Read Event (POLLIN):
///                 ReadDataAvailable - The data for the read event is available in the port's receive buffer.
///                 ERROR_NO_DATA_AVAILABLE - No data is available from the receiver.
///                 ERROR_PORT_NOT_OPEN - A read event cannot be processed on a closed port.
///                 ERROR_DEVICE_IOCTL - Failed to retrieve information from the driver.
///
///             Write Event (POLLOUT):
///                 TransmitBufferAvailable - A buffer is available to transmit user data.
///                 ERROR_PORT_NOT_OPEN - A write event cannot be processed on a closed port.
///
///             Line Status Event (POLLPRI):
///                 LineConnected - An line has been connected to the port.
///                 LineDisconnected - The line has been disconnected from the port.
///                 ERROR_PORT_NOT_OPEN - A status event cannot be processed on a closed port.
///                 ERROR_DEVICE_IOCTL - Failed to retrieve information from the driver.
///
///             Invalid Event Flag:
///                 ERROR_INVALID_READ_REQUEST - The specified event flag is invalid.
///
///	\author	David Rokhvarg
///	\date	11/15/2007
///////////////////////////////////////////////////////////////////////////////////////////////
int SangomaPort::Read(unsigned int event_flag)
{
	int return_code = ERROR_NO_DATA_AVAILABLE;

	EnterPortCriticalSection();

	// The port must have been successfully opened with a valid configuration before a read can be performed
	if (!IsOpen) {
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Read(unsigned char*, int) attempted when port " << GetPortNumber() << " had not been opened yet.";
        LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_PORT_NOT_OPEN;
	}

	switch(event_flag)
	{
	case POLLIN:
		if(pSangomaInterface->read_data(rxhdr, pReceiveBuffer->GetPortBuffer(), pReceiveBuffer->GetPortBufferSize()))
		{
			std::ostringstream error_msg;
			error_msg << "ERROR: read_data() failed! Port: " << GetPortNumber() << " error description: DeviceIoControl() failed\n";
			LastError = error_msg.str();
			LeavePortCriticalSection();
			return ERROR_DEVICE_IOCTL;
		}
		break;
	case POLLPRI:
		if(pSangomaInterface->read_event(wp_api))
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
		error_msg << "ERROR: SangomaPort::Read(unsigned int event_flag) for port " << GetPortNumber() << " invalid 'event_flag'=" << event_flag << "\n";
		ERR_PORT("Port %u: SangomaPort::Read() invalid event_flag=0x%X!\n", GetPortNumber(), event_flag);
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_INVALID_READ_REQUEST;
	}//switch()

	if (POLLIN == event_flag)
	{
		switch (rxhdr->operation_status)
		{
		case SANG_STATUS_RX_DATA_AVAILABLE:
			return_code = ReadDataAvailable;
			pReceiveBuffer->SetUserDataLength(rxhdr->data_length);
			break;
		default:
			ERR_PORT("Port %u: Rx Error: Operation Status: %s (%d)\n", GetPortNumber(),
				SDLA_DECODE_SANG_STATUS(rxhdr->operation_status), rxhdr->operation_status);
			return_code = ERROR_NO_DATA_AVAILABLE;
			break;
		}
	}

	if (POLLPRI == event_flag)
	{
		wp_api_event_t	*wp_tdm_api_event = &wp_api->wp_cmd.event;

		switch (wp_api->wp_cmd.result)
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
			}
			break;
		default:
			if (rxhdr->operation_status == SANG_STATUS_RX_DATA_AVAILABLE) {
				return_code = ReadDataAvailable;
			} else {
				ERR_PORT("Port %u: Event Error: Operation Status: %s (%d)\n", GetPortNumber(),
					SDLA_DECODE_SANG_STATUS(rxhdr->operation_status), rxhdr->operation_status);
				return_code = ERROR_NO_DATA_AVAILABLE;
			}
			break;
		}
	}

	if(return_code == ReadDataAvailable){
		PortStatistics.BytesReceived += pReceiveBuffer->GetUserDataLength();
	}

	LeavePortCriticalSection();
	return return_code;
}


///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::Write(unsigned char* p_source_buffer, const unsigned int bytes_to_write)
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
	if(bytes_to_write > pTransmitBuffer->GetPortBufferSize()){
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::Write(unsigned char*, int) attempted with Tx data length exceeding maximum " << GetPortNumber() << ".";
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_INVALID_TX_DATA_LENGTH;
	}

	pTransmitBuffer->SetUserDataLength(bytes_to_write);
	//
	//If src and dst buffers are the same, there is no need to copy the data because
	//it is already there. This is the case for TxFile (see code in WriteChunkOfTxFile()).
	//
	if (pTransmitBuffer->GetUserDataBuffer() != p_source_buffer) {
		memcpy(pTransmitBuffer->GetUserDataBuffer(), p_source_buffer, bytes_to_write);
	}

	int number_of_bytes_written = 0;

	memset(txhdr, 0x00, sizeof(*txhdr));

#if 0
	if (GetPortNumber() == 0) {
		print_data_buffer((unsigned char*)pTransmitBuffer->GetUserDataBuffer(), pTransmitBuffer->GetUserDataLength());
	}
#endif

	if(pSangomaInterface->transmit(txhdr, pTransmitBuffer->GetUserDataBuffer(), pTransmitBuffer->GetUserDataLength())){
		std::ostringstream error_msg;
		error_msg << "ERROR: SangomaPort::" << __FUNCTION__ << "() failed! Port: " << GetPortNumber() << " error description: DeviceIoControl() failed";
		LastError = error_msg.str();
		LeavePortCriticalSection();
		return ERROR_DEVICE_IOCTL;
	}

	std::ostringstream error_msg;

	switch(txhdr->operation_status)
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
/// \fn     SangomaPort::GetSynchronizationStatus()
/// \brief  Returns whether the port has achieved frame synchronization using the current
///         configuration. If the port has not yet been opened with a configuration, then this
///         method will fail and return false.
///
///         Frame synchronization is determined by checking the presence of Sangoma hardware
///         alarms on the port. When the port is first opened, it is initialized with an alarm
///         present. If frame synchronization is achieved, all hardware alarms are cleared. If
///         frame synchronization is not achieved, alarms will remain present.
///
///         Failing to achieve synchronization can be caused by the configuration being
///         incompatible with the current data or by attenuation on the port.
/// \return True if the port has achieved synchronization, false otherwise.
/// \author Jeff Barbieri
/// \date   12/29/2011
///////////////////////////////////////////////////////////////////////////////////////////////
bool SangomaPort::GetSynchronizationStatus()
{
    // ENTER A CRITICAL SECTION TO PROTECT HARDWARE RESOURCES FROM SIMULTANEOUS ACCESS.
    EnterPortCriticalSection();

    // CHECK IF THE PORT IS OPEN.
    if (!IsOpen)
    {
        // Synchronization can only be achieved on an open port.
        LeavePortCriticalSection();
        return false;
    }

    // CHECK IF HARDWARE ALARMS ARE PRESENT ON THE PORT.
    char alarms_present_result = pSangomaInterface->alarms_present();
    const char ALARMS_PRESENT_RESULT_TRUE = 1;
    bool hardware_alarms_present_on_port = (ALARMS_PRESENT_RESULT_TRUE == alarms_present_result);
    if (hardware_alarms_present_on_port)
    {
        // Synchronization has not been achieved.
        LeavePortCriticalSection();
        return false;
    }

    // SYNCHRONIZATION HAS BEEN ACHIEVED.
    // Update the port statistics to indicate that the port has found synchronization.
    time_t CurrentTime;
    time(&CurrentTime);
    PortStatistics.TimeLastFoundSync = CurrentTime;

    // Return true to indicate that synchronization has been achieved.
    LeavePortCriticalSection();
    return true;
}



///////////////////////////////////////////////////////////////////////////////////////////////
/// \fn         SangomaPort::GetReceiverLevelInDb(float& receiver_level_in_db)
/// \brief      Returns the current receiver level reported by the front end E1/T1 framer on the
///             Sangoma card. This is a relative value reported in decibels. The value is dependent
///             on the current port configuration and is therefore only defined for an open port.
/// \note       The front end E1/T1 framer reports a range of values with a resolution of -2.5dB.
///             This method returns the minimum receiver level reported. For all possible receiver
///             level ranges see Table 9-17 in the front end E1/T1 framer specification.
/// \param[out] receiver_level_in_db - The current receiver level reported by the front end E1/T1
///             framer on the Sangoma card. This value will range between -2.5dB and -44dB.
/// \author     Jeff Barbieri
/// \date       12/28/2011
///////////////////////////////////////////////////////////////////////////////////////////////
bool SangomaPort::GetReceiverLevelInDb(float& receiver_level_in_db)
{
    // ENTER A CRITICAL SECTION TO PROTECT HARDWARE RESOURCES FROM SIMULTANEOUS ACCESS.
    EnterPortCriticalSection();

    // RESET THE OUT PARAMETER.
    receiver_level_in_db = 0;

    // CHECK IF THE PORT IS OPEN.
    if (!IsOpen)
    {
        // The Rx Level is dependent on the current port configuration and therefore only defined
        // for an open port. Return false to indicate that an error has occurred.
        LeavePortCriticalSection();
        return false;
    }

    // ATTEMPT TO RETRIEVE THE E1/T1 FRONT END FRAMER STATISTICS.
    std::string receiver_level_range;
    bool receiver_level_range_retrieved = pSangomaInterface->GetReceiverLevelRange(receiver_level_range);
    if (!receiver_level_range_retrieved)
    {
        // The receiver level range could not be retrieved.
        LeavePortCriticalSection();
        return false;
    }

    // PARSE THE NUMERICAL RECEIVER LEVEL IN DECIBELS FROM ITS STRING REPRESENTATION.
    // The text representing the receiver level range has the following format. The values range from -2.5
    // decibels to -22 decibels.
    //
    //   >-2.5db
    //   -2.5db to -5db
    //   ...
    const std::string NEGATIVE_SIGN = "-";
    std::string::size_type negative_sign_location = receiver_level_range.find_first_of(NEGATIVE_SIGN);
    const std::string DECIBEL_UNIT = "db";
    std::string::size_type db_unit_location = receiver_level_range.find(DECIBEL_UNIT);

    // Check whether the format of the receiver level range is valid.
    bool receiver_level_range_valid =
        (negative_sign_location != std::string::npos) &&
        (db_unit_location != std::string::npos) &&
        (negative_sign_location < db_unit_location);
    if(!receiver_level_range_valid)
    {
        // The receiver level range did not match the expected format. Return false 
        // to indicate that an error has occurred.
        LeavePortCriticalSection();
        return false;
    }

    // Convert the minimum receiver level string to its associated floating point value.
    std::string::size_type numerical_db_string_length = (db_unit_location - negative_sign_location);
    std::string receiver_level_string = receiver_level_range.substr(negative_sign_location, numerical_db_string_length);
    float receiver_level_float = 0;
    bool conversion_to_float_successful = ToFloat(
        receiver_level_string,
        receiver_level_float);
    if (!conversion_to_float_successful)
    {
        // The minimum receiver level could not be converted. Return false to indicate
        // that an error has occurred.
        LeavePortCriticalSection();
        return false;
    }

    // CHECK IF THE RECEIVER LEVEL IS WITHIN THE EXPECTED RANGE.
    const float MAXIMUM_EXPECTED_RECEIVE_LEVEL = -2.5f;
    const float MINIMUM_EXPECTED_RECEIVE_LEVEL = -44.0f;
    bool receiver_level_float_within_expected_range = 
        (receiver_level_float <= MAXIMUM_EXPECTED_RECEIVE_LEVEL) &&
        (receiver_level_float >= MINIMUM_EXPECTED_RECEIVE_LEVEL);
    if (!receiver_level_float_within_expected_range)
    {
        // The receiver level is outside the expected range.
        LeavePortCriticalSection();
        return false;
    }

    // STORE THE RECEIVER LEVEL TO RETURN TO THE USER.
    receiver_level_in_db = receiver_level_float;
    LeavePortCriticalSection();
    return true;
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
	if(pSangomaInterface != NULL){
		unmapped_port = pSangomaInterface->get_mapped_port_number();
	}
	LeavePortCriticalSection();
	return 	unmapped_port;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// \fn         SangomaPort::GetConfiguration(Configuration& current_configuration)
/// \brief      Returns the current port configuration.
/// \param[out] current_configuration - The configuration currently in use on the port.
/// \return     The current port configuration.
/// \author     Jeff Barbieri
/// \date       12/30/2011
///////////////////////////////////////////////////////////////////////////////////////////////
bool SangomaPort::GetConfiguration(Configuration& current_configuration)
{
    // ENTER A CRITICAL SECTION TO PROTECT HARDWARE RESOURCES FROM SIMULTANEOUS ACCESS.
    EnterPortCriticalSection();

    // CLEAR THE OUT PARAMETER.
    current_configuration = Configuration();

    // CHECK IF THE PORT IS OPEN.
    if(!IsOpen)
    {
        // No configuration is currently defined.
        LeavePortCriticalSection();
        return false;
    }

    // STORE THE CURRENT CONFIGURATION TO RETURN TO THE USER.
    current_configuration = CurrentConfiguration;
    LeavePortCriticalSection();
    return true;
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
///	\fn		SangomaPort::GetDriverVersion() 
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
DriverVersion SangomaPort::GetDriverVersion() 
{
	DriverVersion software_abstraction_driver_version;
	wan_driver_version_t drv_version;

	if(pSangomaInterface->get_api_driver_version(&drv_version)){
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
		pSangomaInterface->operational_stats(&stats);
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
		pSangomaInterface->flush_operational_stats();
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
		pSangomaInterface->flush_buffers();
	}
	LeavePortCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		SangomaPort::GetAlarms() 
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
		adapter_type = pSangomaInterface->get_adapter_type();
		pSangomaInterface->read_te1_56k_stat(&u_alarms);
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
	return pReceiveBuffer->GetUserDataBuffer();
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
	return pReceiveBuffer->GetUserDataLength();
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

int SangomaPort::SetAndOpenTxFile(const char *szTxFileName)
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

	bool file_valid = (pTxFile != NULL);
    if (!file_valid)
    {
		//ERR_PORT("TxFile is NOT open!\n");
		return 1;
	}

	user_tx_buffer = this->pTransmitBuffer->GetUserDataBuffer();

	//Initialize the buffer to 0xFF, so when End-of-File is reached,
	//the padding up-to-end of tx buffer is automatic.
	memset( user_tx_buffer, 0xFF, this->pTransmitBuffer->GetUserDataLength() );

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
	int rc = pSangomaInterface->loopback_command(WAN_TE1_DDLB_MODE, WAN_TE1_LB_ENABLE, ENABLE_ALL_CHANNELS);

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
	int rc = pSangomaInterface->loopback_command(WAN_TE1_DDLB_MODE, WAN_TE1_LB_DISABLE, ENABLE_ALL_CHANNELS);

	if (rc) {
		ERR_PORT("Port %u: %s() failed! rc: %d\n", GetPortNumber(), __FUNCTION__, rc);
	}

	return rc;

}

