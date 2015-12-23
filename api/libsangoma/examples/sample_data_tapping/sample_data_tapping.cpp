

// sample_data_tapping.cpp : Defines the entry point for the console application.
//

#include <ctime>
#include <iostream>
#include "Sangoma/SangomaCard.h"

#ifndef __WINDOWS__
# include <stdio.h>
# include <termios.h>
# include <unistd.h>
# include <fcntl.h>

int _kbhit(void);
#endif

#include "libsangoma.h"

using namespace Sangoma;

/* 64 is maximum number of handles in windows 
   This code will start another thread for 
   maximum capacity of 128 handles */
#define MAX_HANDLES	64

const int VALID_NUM_OF_ARGS = 2;			///< The valid number of arguements expected by the program
const std::string A104_MODEL = "A104";		///< Command line string used to indicate to check the system for Sangoma A104 cards
const std::string A108_MODEL = "A108";		///< Command line string used to indicate to check the system for Sangoma A108 cards
const std::string A116_MODEL = "A116";		///< Command line string used to indicate to check the system for Sangoma A116 cards
const std::string T116_MODEL = "T116";		///< Command line string used to indicate to check the system for Sangoma A116 cards
bool gRunning = false;	///< Flag which is true if the sample program is currently monitoring the cards, false otherwise
	
std::vector<SangomaCard*> sangoma_cards;			//stores all the cards in the system
std::vector<SangomaPort*> sangoma_ports;			//stores ALL the ports in the system
std::vector<SangomaPort*> sangoma_ports_bank2;		//stores ALL the ports in the system

#define DBG_MAIN	if(0)printf
#define INFO_MAIN	if(1)printf
#define WRN_MAIN	if(1)printf
#define ERR_MAIN	printf("Error: %s():line:%d: ", __FUNCTION__, __LINE__);printf

#define FUNC_MAIN()	if(1)printf("%s():line:%d\n", __FUNCTION__, __LINE__)

//SETUP FOR DETERMING PORT CONFIGURATION
const unsigned int NUMBER_OF_E1_FRAME_TYPES = 2;
const unsigned int NUMBER_OF_T1_FRAME_TYPES = 2;
const FramingType E1_TYPES[NUMBER_OF_E1_FRAME_TYPES] = {E1_CCS_CRC4_ON, E1_CCS_CRC4_OFF};
const FramingType T1_TYPES[NUMBER_OF_T1_FRAME_TYPES] = {T1_EXTENDED_SUPERFRAME, T1_SUPERFRAME};	

static int iForcedLineType = -1;//< When greater than zero, will disable "Line Discovery" and configure for Line Type specified on command line.
static char szTxFileName[1024];	//< Full path to a file for transmission.
static int iLocalLoopback = 0;	//< When non-zero, will enable Local Loopback at T1/E1 level
static int iSilent = 0;		
static int iTimeout = 0;		
static char rx2tx = 0;		//< When non-zero, all Rx data will be transmitted at the time of reception.
static unsigned int total_number_of_ports=0;


#define sng_getch() do { if (!iSilent) _getch(); } while(0) 

char* wp_get_time_stamp()
{
    static char formatted_time_stamp[400];

#if defined(__WINDOWS__)
    SYSTEMTIME timestamp;
    GetLocalTime(&timestamp);
	wp_snprintf(formatted_time_stamp, sizeof(formatted_time_stamp), "Y:%d/M:%d/D:%d. H:%d/M:%d/S:%d/MS:%u",
		timestamp.wYear, 
		timestamp.wMonth, 
		timestamp.wDay,
		timestamp.wHour, 
		timestamp.wMinute,
		timestamp.wSecond,
		timestamp.wMilliseconds);
#else
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);

	wp_snprintf(formatted_time_stamp, sizeof(formatted_time_stamp), "Sec:%u Ms:%u",
		(uint32_t)timestamp.tv_sec, (uint32_t)(timestamp.tv_usec / 1000));
#endif
	return formatted_time_stamp;
}   

void print_data_buffer(unsigned char *buffer, int buffer_length)
{
	printf("Data length: %u\n", buffer_length);

	for(int ln = 0; ln < buffer_length; ln++){
		if((ln % 20 == 0)){
			if(ln){
				printf("\n");
			}
			printf("%04d ", ln/20);//print offset
		}
		printf("%02X ", buffer[ln]);
	}
	printf("\n");
}

void print_to_debugger(void *pszFormat, ...)
{
	char str[512];
	va_list	vaArg;

	va_start (vaArg, pszFormat);
	_vsnprintf((char*)str, sizeof(str), (char *)pszFormat, vaArg);
	va_end (vaArg);

#if defined(__WINDOWS__)
    OutputDebugString(str);
#else
    printf("%s\n",str);
#endif

}

static int parse_command_line_args(
	int argc, char* argv[],
	CardModel *model,
	unsigned int *number_of_ports_per_card,
	Configuration & configuration,
	BufferSettings & buffer_settings)
{
	int i;
#define USAGE_STR \
"\n\
Usage: sample_data_tapping CardModel [LineType] [h MaxCableLossOption]\n\
\n\
Options:\n\
 CardModel             [a104 | a108 | a116]\n\
 LineType              T1 or E1\n\
 MaxCableLossOption    430, 360, 300, 225, 180, 175, 120\n\
 tx_tristate           this option will disable transmitter\n\
 bm <Number>           Buffer Multiplier - between 1 and 7 (including)\n\
 -tx_file <TxFileName> Will continuesly transmit <TxFileName> when Line becomes synchronized.\n\
                       Must be used in conjuction with one of \"Line Type\" options because\n\
                       Line Tapping Device will discover the line type.\n\
 -local_loopback       Enable \"Local Line Looopback\" at T1/E1 level. Must be used only for Master clock.\n\
 -e1_cas_crc4          Disable \"Line Discovery\" and configure for E1_CAS_CRC4_ON\n\
 -e1_cas_no_crc        Disable \"Line Discovery\" and configure for E1_CAS_CRC4_OFF\n\
 -e1_ccs_crc4          Disable \"Line Discovery\" and configure for E1_CCS_CRC4_ON\n\
 -e1_ccs_no_crc        Disable \"Line Discovery\" and configure for E1_CCS_CRC4_OFF\n\
 -e1_cas_unframed      Disable \"Line Discovery\" and configure for E1_CAS_UNFRAMED\n\
 -e1_ccs_unframed      Disable \"Line Discovery\" and configure for E1_CCS_UNFRAMED\n\
 -t1_esf               Disable \"Line Discovery\" and configure for T1_EXTENDED_SUPERFRAME\n\
 -t1_sf                Disable \"Line Discovery\" and configure for T1_SUPERFRAME\n\
 -rx2tx                If used, all Rx data will be transmitted back to the transmitter.\n\
 -data_api_mode        Enable DATA_API mode,       Default SPAN Voice API Mode\n\
 -chan_api_mode        Enable CHAN_VOICE_API mode, Default SPAN Voice API Mode\n\
 -dchan                [ 1-31 ] #Default None\n\
 -dchan_seven_bit      Enable 56K HDLC on D-Chan\n\
 -dchan_mtp1_filter    Enable SS7 MTP1 Filter on D-Chan\n\
 -chunk_ms             [ 10 | 20 | 30 | 40 | 50 | 60 ] #Default 20ms (160 bytes per timeslot)\n\
 -silent               No user interaction.\n\
 -timeout               No user interaction.\n\
\n\
Example 1: sample_data_tapping a108 e1\n\
Example 2: sample_data_tapping a108 t1\n\
Example 3: sample_data_tapping a108 e1 h 320\n\
Example 4: sample_data_tapping a108 e1 h 320 tx_tristate\n\
Example 5: sample_data_tapping a108 -e1_cas_crc4 -tx_file c:\\tmp\\test_file.pcm -local_loopback\n\
Example 5: sample_data_tapping a108 -e1_cas_crc4 -dchan 16 -local_loopback\n"

	*number_of_ports_per_card = 0;
	szTxFileName[0] = '\0';

	for(i = 0; i < argc; i++){
	
		if(wp_strncasecmp(argv[i], "?", strlen(argv[i])) == 0 || wp_strncasecmp(argv[i], "/?",  strlen(argv[i])) == 0){
			printf(USAGE_STR);
			return 1;
		}else if(wp_strncasecmp(argv[i], "T1",  strlen(argv[i])) == 0){
			configuration.E1OrT1 = T1;
		}else if(wp_strncasecmp(argv[i], "E1", strlen(argv[i])) == 0){
			configuration.E1OrT1 = E1;
		}else if(wp_strncasecmp(argv[i], "a104",  strlen(argv[i])) == 0){
			*model = A104;
			*number_of_ports_per_card = NUMBER_OF_A104_PORTS;
		}else if(wp_strncasecmp(argv[i], "a108",  strlen(argv[i])) == 0){
			*model = A108;
			*number_of_ports_per_card = NUMBER_OF_A108_PORTS;
		}else if(wp_strncasecmp(argv[i], "a116",  strlen(argv[i])) == 0){
			*model = A116;
			*number_of_ports_per_card = NUMBER_OF_A116_PORTS;
		}else if(wp_strncasecmp(argv[i], "t116",  strlen(argv[i])) == 0){
			*model = T116;
			*number_of_ports_per_card = 16;
		}else if(wp_strncasecmp(argv[i], "-dchan",  strlen(argv[i])) == 0){
			if (i+1 > argc-1){
				ERR_MAIN("No Valid 'dchan' value was provided!\n");
				printf(USAGE_STR);
				return 1;
			}
			configuration.dchan=atoi(argv[i+1]);
		}else if(wp_strncasecmp(argv[i], "-chunk_ms",  strlen(argv[i])) == 0){
			if (i+1 > argc-1){
				ERR_MAIN("No Valid 'chunk_ms' value was provided!\n");
				printf(USAGE_STR);
				return 1;
			}
			configuration.chunk_ms=atoi(argv[i+1]);
		}else if(wp_strncasecmp(argv[i], "h",  strlen(argv[i])) == 0){
			if (i+1 > argc-1){
				ERR_MAIN("No Valid 'Max Cable Loss' value was provided!\n");
				printf(USAGE_STR);
				return 1;
			}
			switch(atoi(argv[i+1]))
			{
			case MCLV_43_0dB:
			case MCLV_36_0dB:
			case MCLV_30_0dB:
			case MCLV_22_5dB:
			case MCLV_18_0dB:
			case MCLV_17_5dB:
			case MCLV_12_0dB:
				configuration.HighImpedanceMode = true;
				configuration.MaxCableLoss = (MAX_CABLE_LOSS)atoi(argv[i+1]);
				break;
			default:
				configuration.HighImpedanceMode = false;
				configuration.MaxCableLoss = MCLV_12_0dB;
			}
		}else if(wp_strncasecmp(argv[i], "tx_tristate", strlen(argv[i])) == 0){
			configuration.TxTristateMode = true;
		
		}else if(wp_strncasecmp(argv[i], "bm",  strlen(argv[i])) == 0){
			if (i+1 > argc-1){
				ERR_MAIN("No Valid 'Buffer Multiplier' value was provided!\n");
				printf(USAGE_STR);
				return 1;
			}
			buffer_settings.BufferMultiplierFactor = atoi(argv[i+1]);
			if(	buffer_settings.BufferMultiplierFactor < BufferSettings::MINIMUM_BUFFER_MULTIPLIER_FACTOR ||
				buffer_settings.BufferMultiplierFactor > BufferSettings::MAXIMUM_BUFFER_MULTIPLIER_FACTOR){
				ERR_MAIN("The 'Buffer Multiplier' value of %d is invalid! Minimum is: %d, maximum is: %d! Setting to %d.\n",
					buffer_settings.BufferMultiplierFactor, 
					BufferSettings::MINIMUM_BUFFER_MULTIPLIER_FACTOR, BufferSettings::MAXIMUM_BUFFER_MULTIPLIER_FACTOR,
					BufferSettings::MINIMUM_BUFFER_MULTIPLIER_FACTOR);
				buffer_settings.BufferMultiplierFactor = BufferSettings::MINIMUM_BUFFER_MULTIPLIER_FACTOR;
				printf(USAGE_STR);
				return 1;
			}else{
				INFO_MAIN("User's 'Buffer Multiplier' value is: %d.\n", buffer_settings.BufferMultiplierFactor);
			}

		}else if(wp_strncasecmp(argv[i], "-tx_file", strlen(argv[i])) == 0){
			if (i + 1 > argc - 1) {
				INFO_MAIN("No TxFileName provided!\n");
				printf(USAGE_STR);
				return 1;
			}
			wp_snprintf(szTxFileName, sizeof(szTxFileName), "%s", argv[i + 1]);
			INFO_MAIN("Setting szTxFileName to '%s'.\n", szTxFileName);

		}else if(wp_strncasecmp(argv[i], "-local_loopback", strlen(argv[i])) == 0){
			INFO_MAIN("Enabling Local Loopback at T1/E1 level.\n");
			iLocalLoopback = 1;
			configuration.Master=1;
		}else if(wp_strncasecmp(argv[i], "-silent", strlen(argv[i])) == 0){
			iSilent = 1;
		}else if(wp_strncasecmp(argv[i], "-timeout", strlen(argv[i])) == 0){
			if (i+1 > argc-1){
				ERR_MAIN("Invalid 'Timeout' value was provided!\n");
				printf(USAGE_STR);
				return 1;
			}
			iTimeout = atoi(argv[i+1]);
		}else if(wp_strncasecmp(argv[i], "-rx2tx", strlen(argv[i])) == 0){
			INFO_MAIN("Enabling Rx-to-Tx for Rx data.\n");
			rx2tx = 1;
		}else if(wp_strncasecmp(argv[i], "-e1_cas_crc4", strlen(argv[i])) == 0){
			iForcedLineType = E1_CAS_CRC4_ON;
			configuration.E1OrT1 = E1;
		}else if(wp_strncasecmp(argv[i], "-e1_cas_no_crc", strlen(argv[i])) == 0){
			iForcedLineType = E1_CAS_CRC4_OFF;
			configuration.E1OrT1 = E1;
		}else if(wp_strncasecmp(argv[i], "-e1_ccs_crc4", strlen(argv[i])) == 0){
			iForcedLineType = E1_CCS_CRC4_ON;
			configuration.E1OrT1 = E1;
		}else if(wp_strncasecmp(argv[i], "-e1_ccs_no_crc", strlen(argv[i])) == 0){
			iForcedLineType = E1_CCS_CRC4_OFF;
			configuration.E1OrT1 = E1;
		}else if(wp_strncasecmp(argv[i], "-e1_cas_unframed", strlen(argv[i])) == 0){
			iForcedLineType = E1_CAS_UNFRAMED;
			configuration.E1OrT1 = E1;
		}else if(wp_strncasecmp(argv[i], "-e1_ccs_unframed", strlen(argv[i])) == 0){
			iForcedLineType = E1_CCS_UNFRAMED;
			configuration.E1OrT1 = E1;
		}else if(wp_strncasecmp(argv[i], "-t1_esf", strlen(argv[i])) == 0){
			iForcedLineType = T1_EXTENDED_SUPERFRAME;
			configuration.E1OrT1 = T1;
		}else if(wp_strncasecmp(argv[i], "-t1_sf", strlen(argv[i])) == 0){
			iForcedLineType = T1_SUPERFRAME;
			configuration.E1OrT1 = T1;
		}else if(wp_strncasecmp(argv[i], "-dchan_seven_bit", strlen(argv[i])) == 0){
			configuration.dchan_seven_bit = 1;
		}else if(wp_strncasecmp(argv[i], "-dchan_mtp1_filter", strlen(argv[i])) == 0){
			configuration.dchan_mtp1_filter = 1;
		}else if(wp_strncasecmp(argv[i], "-data_api_mode", strlen(argv[i])) == 0){
			configuration.api_mode = SNG_DATA_MODE;
		}else if(wp_strncasecmp(argv[i], "-chan_api_mode", strlen(argv[i])) == 0){
			configuration.api_mode = SNG_CHAN_MODE;
		}
	}//for()

	if(*number_of_ports_per_card == 0){
		printf(USAGE_STR);
		return 1;
	}

	if (szTxFileName[0] != '\0' && iForcedLineType == -1) {
		ERR_MAIN("\"Line Type\" must be provided for -tx_file option!\n");
		printf(USAGE_STR);
		return 1;
	}

	INFO_MAIN("\nCommand Line parsing summary:\n");

	if (iForcedLineType != -1) {
		configuration.Framing = (FramingType)iForcedLineType;
	}

	if (szTxFileName[0] != '\0') {
		//user wants to transmit a file - no need for Line Discovery
		INFO_MAIN("CardModel: %s, Forced Line Type: %s, TxFileName: '%s'\n\n", 
			DECODE_CARD_TYPE(*model), DECODE_FRAMING(configuration.Framing), szTxFileName);

	} else {
		//user wants to tap a line - first must run Line Discovery
		INFO_MAIN("CardModel: %s, Line Type: %s, HighImpedance: %s, MaxCableLoss: %u, TxTristateMode: %s\n\n", 
			DECODE_CARD_TYPE(*model), (configuration.E1OrT1 == T1 ? "T1":"E1"),
			(configuration.HighImpedanceMode == true ? "On":"Off"), configuration.MaxCableLoss,
			(configuration.TxTristateMode == true ? "On":"Off"));
	}
	INFO_MAIN("Press  <Enter> key to continue.\n");
	sng_getch();

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////
///	\fn		PortEventHandler(SangomaPort* p_port, unsigned int out_flags)
///	\brief	This function handles Data and Events raised by the Sangoma classes.
///	\param	p_port	Pointer to the SangomaPort object that issued this event
///	\param	out_flags	BitMap containing information about Data/Events raised by Sangoma Classes
///	\return	true if the event was successfully handled, false if an error occured
///	\author Josh Markwordt/David Rokhvarg
///	\date	5/31/2010
///////////////////////////////////////////////////////////////////////////////////////////////
bool PortEventHandler(SangomaPort* p_port, unsigned int out_flags)
{
	Alarms sangoma_alarms;
#if 0
	static int print_flag = 1;
#endif

	//CHECK IF WE WERE PASSED A VALID PORT
	bool invalid_port = (NULL == p_port);
	if (invalid_port) {
		std::cerr << "ERROR: PortEvent created on invalid port." << std::endl;
		return false;
	}
	
	if(p_port->GetIsOpen() == false){
		/* Some other port's event got signalled. Don't try to Read() on a closed port. */
		return true;
	}

	//READ FROM THE PORT THAT GENERATED THE EVENT (THIS READ NEEDS TO BE THREAD SAFE)
	//Regardless of the reason for the event always call Read().  This will either gives us the data or return
	//reason the event was triggered (e.g. PORT_IN_SYNC, PORT_OUT_OF_SYNC).
	int read_result = p_port->Read(out_flags);

	//PROCESS THE EVENT.
	//Also check if the read was successful, if not then we need to determine what the problem was.
	//DBG_MAIN("Port %u: ", p_port->GetPortNumber());
	switch(read_result)
	{
	case ReadDataAvailable:
		DBG_MAIN("ReadDataAvailable\n");
		p_port->RxCount++;
#if 0
		//As an example of how to access Rx data, print the Rx data:
		if (p_port->GetPortNumber() == 0) {
			print_data_buffer(p_port->GetRxDataBuffer(), p_port->GetRxDataLength());
		}
#endif
#if 0
		if (print_flag) {
			print_flag = 0;
			printf("rx len: %d\n", p_port->GetRxDataLength());
		}
#endif
        p_port->RxLength=p_port->GetRxDataLength();

		//As an example of how to transmit data, call Write() whith Rx data as input.
        p_port->RxDiscardCounter++;
		if (rx2tx) {
			//To avoid blocking receiver when waiting for transmitter,
			//discard 1-st hundred rx buffers before starting rx2tx 
			//(it will clear the Tx queue before we start transmitting).
			if (1 || p_port->RxDiscardCounter > 100) {
				p_port->Write(p_port->GetRxDataBuffer(), p_port->GetRxDataLength());
				if (p_port->RxDiscardCounter == 1) {
					p_port->ClearStatistics();
				}
			}
		}
		break;
	case LineConnected:
		//DBG_MAIN("LineConnected\n");
		/// \todo Handle the line connected event
		//DAVIDR: port is open, line connected --> start handling incoming data.
		p_port->RxDiscardCounter = 0;
		p_port->ClearStatistics();
		p_port->FlushBuffers();
		break;
	case LineDisconnected:
		//DBG_MAIN("LineDisconnected\n");
		//DAVIDR: the line got disconnected. If Alarams are of any interest, read them here.
		//May put a message for the user "cable disconnected".
		//
		sangoma_alarms = p_port->GetAlarms();
		/// \todo Handle the line disconnected event
		//DAVIDR Note: Some lines go up/down frequently and closing the port and starting Line type discovery
		//may cause a lot of unwanted load on the CPU. The best thing to do is to keep the port open
		//and wait for the LineConnected event, because the line type is already known and in the
		//real world it is not changing often. If the LineConnected event not received within
		//some timeout, it may make sense to close the port and start the Line type discovery.
		break;
	case ERROR_NO_DATA_AVAILABLE:
		//FIXME: This condition may occur only during Line Discovery. Since no data can be received from
		//a disconnected port, this should be treated as a Warning, not an Error. Still, should be debugged in the future.
		//For now, ignore the warning.
		std::cout << "Port: " << p_port->GetPortNumber() << ": Warning: The event triggered but no data is available\n" << std::endl;
		break;
	case ERROR_INVALID_DESTINATION_BUFFER:
		std::cerr << "Port: " << p_port->GetPortNumber() << ": Error: Unable to read because provided buffer was invalid\n" << std::endl;
		break;
	case ERROR_INVALID_DESTINATION_BUFFER_TOO_SMALL:
		std::cerr << "Port: " << p_port->GetPortNumber() << ": Error: Unable to read because provided buffer was too small to hold the data.\n" << std::endl;
		break;
	case ERROR_PORT_NOT_OPEN:
		std::cerr << "Port: " << p_port->GetPortNumber() << ": Error: The port is not Open.\n" << std::endl;
		break;
	case ERROR_DEVICE_IOCTL:
		std::cout << p_port->GetLastError();
		break;
	case TransmitBufferAvailable:
		p_port->WriteChunkOfTxFile();
		break;
	default:
		std::cerr << "Port: " << p_port->GetPortNumber() << ": Unknown Error reported by Read()\n" << std::endl;
		break;
	} // switch(read_result)

	return true;
}
#if defined(__WINDOWS__)
DWORD SangomaThreadFunc(void *p_param)
#else
void* SangomaThreadFunc(void *p_param)
#endif
{ 
	int	iResult;
    DBG_MAIN("\n%s() - start\n", __FUNCTION__); 

	//CAST TO THE OBJECT THAT WAS PASSED IN
	std::vector<SangomaPort*>* p_ports = static_cast<std::vector<SangomaPort*>*>(p_param);
	std::vector<SangomaPort*>::iterator iter;

	SangomaPort	*p_port;

	//DETERMINE THE EVENT HANDLES FOR ALL THE PORTS IN ALL OF THE CARDS
	//these handles will be passed to WaitForMultipleObjects so that we know when to read
	unsigned int total_number_of_ports_in_system = static_cast<unsigned int>( p_ports->size() );
	unsigned int port_index, number_of_open_ports = 0;
	//SETUP VARIABLES FOR PROCESSING THE EVENT
	DWORD PORT_READ_TIMEOUT = 3000;		//arbitrarily set, doesn't matter b/c we don't do anything on a time out

	sangoma_wait_obj* port_event_handles[MAX_HANDLES];
	unsigned int port_input_flags[MAX_HANDLES];
	unsigned int port_output_flags[MAX_HANDLES];
	SangomaPort* sangoma_open_ports[MAX_HANDLES];	//stores Open ports only

	if (total_number_of_ports_in_system > MAX_HANDLES) {
		fprintf(stderr,"Error Total number of ports %i > Max Hanldes %i\n",total_number_of_ports_in_system,MAX_HANDLES);
		exit(1);	
	}

	DBG_MAIN("%s():total_number_of_ports_in_system: %d\n", __FUNCTION__, total_number_of_ports_in_system); 

	//START PROCESSING IN THE THREAD
	while(gRunning)
	{
		//DavidR: Ports are getting Open and Closed during Line type Discovery, only
		//the Open ports will have a valid Event which can be waited on.
		//Go through the list of Open ports and populate the array of Sangoma Waitable objects.
		number_of_open_ports = 0;
		memset(port_event_handles, 0x00, sizeof(port_event_handles));
		memset(port_input_flags, 0x00, sizeof(port_input_flags));
		memset(port_output_flags, 0x00, sizeof(port_output_flags));
		memset(sangoma_open_ports, 0x00, sizeof(sangoma_open_ports));
		

		for (iter = p_ports->begin(); iter != p_ports->end(); ++iter) {
			p_port = *iter;

			if (p_port->GetIsOpen() == true &&
				p_port->GetEvent().EventHandle &&
				p_port->GetEvent().EventHandle != (void*)INVALID_HANDLE_VALUE) 
			{
				port_event_handles[number_of_open_ports] = p_port->GetEvent().EventHandle;
				DBG_MAIN("Port %i Handle %p\n",p_port->GetPortNumber(),p_port->GetEvent().EventHandle);

				if (szTxFileName[0] == '\0') {
					//rx and line state
					port_input_flags[number_of_open_ports] = (POLLIN | POLLPRI);
				} else {
					//tx and line state
					port_input_flags[number_of_open_ports] = (POLLOUT | POLLPRI);
				}
							
				sangoma_open_ports[number_of_open_ports] = p_port;

				number_of_open_ports++;
			}
		}

		if(!number_of_open_ports){
			WRN_MAIN("%s(): no open ports!\n", __FUNCTION__); 
			sangoma_msleep(1000);
			continue;
		}

		DBG_MAIN("%s(): number_of_open_ports: %d\n", __FUNCTION__, number_of_open_ports); 

		//WAIT FOR AN EVENT FROM THE PORT
		//We are signalled anytime there is an event.  This could be for a line connection,
		//a line disconnection, or if there is data available.

		iResult = sangoma_waitfor_many(port_event_handles,
				port_input_flags,
				port_output_flags,
				number_of_open_ports,
				PORT_READ_TIMEOUT /* Wait timeout, in milliseconds. Or SANGOMA_INFINITE_API_POLL_WAIT. */
				);

		if (iResult == SANG_STATUS_APIPOLL_TIMEOUT) {
			WRN_MAIN("%s(): Timeout\n", __FUNCTION__);
			continue;
		}

		if (iResult != SANG_STATUS_SUCCESS) {
			WRN_MAIN("sangoma_waitfor_many: iResult: %s (%d)\n", SDLA_DECODE_SANG_STATUS(iResult), iResult);
			//FIXME Do not Assert in production
			exit(1);
			break;
		}

		//DavidR: go through the array of OUT flags and do the work for those which are set.
		for(port_index = 0; port_index < number_of_open_ports; ++port_index)
		{
			/* a wait object was signaled */
			if(	port_output_flags[port_index] & POLLIN	||
				port_output_flags[port_index] & POLLPRI ||
				port_output_flags[port_index] & POLLOUT	) {

				// Retreive the signaled port from the list of open ports
				p_port = sangoma_open_ports[port_index];
				if(p_port == NULL){
					std::cout << "ERROR: invalid port index: " << port_index << std::endl;
					exit(1);
					continue;
				}
				
				// Call PortEventHandler() for each bit in port_output_flags[port_index]
				if (port_output_flags[port_index] & POLLIN) 
				{	// handle Rx Data
					if (PortEventHandler(p_port, POLLIN) == false) 
					{	// Call the event handler to take the appropriate actions
						std::cout << "ERROR: Rx Data on port " << port_index << " could not be handled properly." << std::endl;
					}
				}

				if (port_output_flags[port_index] & POLLPRI) 
				{	//handle an Event
					if (PortEventHandler(p_port, POLLPRI) == false) 
					{	// Call the event handler to take the appropriate actions
						std::cout << "ERROR: An Event on port " << port_index << " could not be handled properly." << std::endl;
					}
				}

				if (port_output_flags[port_index] & POLLOUT) 
				{	//handle an Event
					if (PortEventHandler(p_port, POLLOUT) == false) 
					{	// Call the event handler to take the appropriate actions
						std::cout << "ERROR: An Event on port " << port_index << " could not be handled properly." << std::endl;
					}
				}

			}

			if( port_output_flags[port_index] & (~port_input_flags[port_index]) ){
				WRN_MAIN("\nUnexpected port_output_flags[%d]: 0x%X\n\n", port_index, port_output_flags[port_index]);
				iResult = SANG_STATUS_IO_ERROR;
			}
		}//for()

	} // while(gRunning)
	
    DBG_MAIN( "\n%s() - end\n", __FUNCTION__); 
	
#if defined(__WINDOWS__)
    return 0; 
#else
	pthread_exit(NULL);
    return NULL;
#endif
}

void load_driver_modules()
{
#if defined(__WINDOWS__)
	//driver loaded by PnP system - do nothing here
#else
	//run "hwprobe" - if driver not loaded yet this command will load the driver
	system("wanrouter hwprobe");
#endif
}

void clear_screen()
{
#if defined(__WINDOWS__)
	system("cls");
#else
	system("clear");
#endif
}

void print_cfg_stats(void *p_param)
{
	std::vector<SangomaPort*>* p_ports = static_cast<std::vector<SangomaPort*>*>(p_param);

	INFO_MAIN("\n");
	//print port statistics/status
	for(unsigned int port_index = 0; port_index < p_ports->size(); ++port_index)
	{
		SangomaPort* p_port = p_ports->at(port_index);		//make a copy of the pointer to make it easier to work with
        bool synchronization_achieved = p_port->GetSynchronizationStatus();	//retrieve the synchronization status. DAVIDR: requires context switch to kernel-mode, should not be called too often.

		if (p_port->SyncCount == 1) {
			p_port->GetStatistics();	
		}

		Statistics stats = p_port->GetStatistics();							//Get the statistics for this port

        float receiver_level_db;
        bool receiver_level_retrieved = p_port->GetReceiverLevelInDb(receiver_level_db);
		INFO_MAIN("Port:%02d(unmapped:%02d):%20s,\tRx Count: %8u\tTx Count: %8u\tErrors: %8u\tRx Len: %i\t",
			p_port->GetPortNumber(), p_port->GetUnmappedPortNumber(),
			(synchronization_achieved ? "synchronized" : "not synchronized"), 
			stats.BytesReceived, stats.BytesSent,stats.Errors,p_port->RxLength);

        if(receiver_level_retrieved)
        {
            printf("Receiver level: %.1fdB\n", receiver_level_db);
        }
        else
        {
            printf("Receiver level: ERROR (Could not retrieve receiver level)\n");
        }
	}

}

void print_io_stats(void *p_param)
{
	//print Line Type of Ports in Sync
	std::vector<SangomaPort*>* p_ports = static_cast<std::vector<SangomaPort*>*>(p_param);

	INFO_MAIN("\n");
	for(unsigned int port_index = 0; port_index < p_ports->size(); ++port_index)
	{
		SangomaPort* p_port = p_ports->at(port_index);		//make a copy of the pointer to make it easier to work with
		bool synchronization_achieved = p_port->GetSynchronizationStatus();	//retrieve the synchronization status. DAVIDR: requires context switch to kernel-mode, should not be called too often.
		Configuration cfg;
        bool configuration_obtained = p_port->GetConfiguration(cfg);

		INFO_MAIN("Port %d: %20s,\tLineType: %s,\tFraming: %s\n",
			p_port->GetPortNumber(),
			(synchronization_achieved ? "synchronized" : "not synchronized"), 
			(cfg.E1OrT1 == E1 ? "E1" : "T1"), 
			configuration_obtained ? (DECODE_FRAMING(cfg.Framing)) : "could not obtain configuration");
	}
}

int main_loop_per_port_bank(void *p_param, Configuration & configuration)
{
	std::vector<SangomaPort*>* p_ports = static_cast<std::vector<SangomaPort*>*>(p_param);
		
    DBG_MAIN("main_loop_per_port_bank\n");

	//DETERMINE EACH PORTS CONFIGURATION
	for(unsigned int port_index = 0; port_index < p_ports->size(); ++port_index)
	{
        // CHECK WHETHER THE PORT HAS BEEN PREVIOUSLY OPENED.
        SangomaPort* p_port = p_ports->at(port_index);		//make a copy of the pointer to make it easier to work with
        time_t port_open_time = p_port->GetStatistics().TimeLastOpened;
        bool portWasPreviouslyOpened = (port_open_time != 0);
        if(portWasPreviouslyOpened)
        {
            // Check how long it has been since opening the port.
            time_t current_time;
            time(&current_time);
            double seconds_since_opening_port = difftime(current_time, port_open_time);
            const double SECONDS_REQUIRED_FOR_PORT_SYNCHRONIZATION = 22.0;
            bool port_has_been_open_long_enough_for_synchronization = (seconds_since_opening_port > SECONDS_REQUIRED_FOR_PORT_SYNCHRONIZATION);
            if(!port_has_been_open_long_enough_for_synchronization)
            {
                // Allow this port to continue attempting synchronization. Move on to the next port.
                continue;
            }
        }

		//RETRIEVE THE SYNCHRONIZATION STATUS
		bool synchronization_achieved = p_port->GetSynchronizationStatus();	//retrieve the synchronization status. DAVIDR: requires context switch to kernel-mode, should not be called too often.

		DBG_MAIN("Port %d: %s\n", port_index, (synchronization_achieved ? "synchronized" : "not synchronized")); 
		//BASED ON THE STATUS DETERMINE WHAT TO DO

		if (synchronization_achieved) {
			if (!p_port->RxCount) {
				time_t current_time;
            	time(&current_time);
				if (p_port->rx_pkt_timeout == 0) {
					time(&p_port->rx_pkt_timeout);
				} else {
					double rx_timeout_in_sec  = difftime(current_time, p_port->rx_pkt_timeout);			
					if (rx_timeout_in_sec > 5) {
						synchronization_achieved=0;
						printf("Error: Connected by no RX - going out of sync");
					}	
				}
			} else {
				p_port->rx_pkt_timeout=0;
			}
		}

		if(!synchronization_achieved)
        {
			p_port->rx_pkt_timeout=0;
			p_port->RxCount=0;
			if (iForcedLineType != -1)
			{	//user specified the Line Type - do not attempt to run Line Discovery.
				if (p_port->GetIsOpen() == false) {
					configuration.LineCoding = ON;
				
					DBG_MAIN("Port Open Foce\n");
					p_port->Open(configuration);//open the port with FORCED configuration, only a single time.

					if (szTxFileName[0] != '\0') {
						if (p_port->SetAndOpenTxFile(szTxFileName)) {
							ERR_MAIN("Failed to open file: %s\n", szTxFileName);
							exit(1);
						}
					}

					//////////////////////////////////////////
					//apply user-specfied settings to the port
					//////////////////////////////////////////
					if (iLocalLoopback) {
						p_port->LineLoopbackEnable();
					}
				}
			} else {
				//determine the next configuration to try
				unsigned int next_frame_type = p_port->next_framing_type;						//retrieve the next frame type to try
				if(configuration.E1OrT1 == T1){
					p_port->next_framing_type = (next_frame_type + 1) % NUMBER_OF_T1_FRAME_TYPES;	//move the stored index to the frame type to try next time
					configuration.Framing = T1_TYPES[next_frame_type];
					configuration.LineCoding = ON;
				}else{
					p_port->next_framing_type = (next_frame_type + 1) % NUMBER_OF_E1_FRAME_TYPES;	//move the stored index to the frame type to try next time
					configuration.Framing = E1_TYPES[next_frame_type];
					configuration.LineCoding = ON;
				}
    
				DBG_MAIN("Auto Port Close\n");
				p_port->Close();				//close the port

				DBG_MAIN("Auto Port Open\n");
				p_port->Open(configuration);	//re-open the port with the new configuration
			}
        }
	} // for(unsigned int port_index = 0; port_index < ports.size(); ++port_index)

	return 0;
}

int main_program_loop(void * p_param, void *p_param2, Configuration & configuration)
{
	//CAST TO THE OBJECT THAT WAS PASSED IN
	int timeout=0;
	std::vector<SangomaPort*>* p_ports = static_cast<std::vector<SangomaPort*>*>(p_param);
	std::vector<SangomaPort*>* p_ports_bank2 = static_cast<std::vector<SangomaPort*>*>(p_param2);
		
	std::cerr << "Errout Test \n" << std::endl;

	//CONTINUOUSLY TRY TO DETERMINE THE PORTS CONFIGURATION
	//if the port is in-sync nothing happens but while the port is out of sync
	//we cycle through the framing types to try and find the correct one.
	while( gRunning )
	{

		main_loop_per_port_bank(p_ports, configuration);
		main_loop_per_port_bank(p_ports_bank2, configuration);

		clear_screen();

	
		INFO_MAIN("\nTime: %s \n", wp_get_time_stamp());

		print_cfg_stats(&sangoma_ports);	//print statistics for all ports
		print_cfg_stats(&sangoma_ports_bank2);	//print statistics for all ports
#if 0
		print_io_stats(&sangoma_ports);	//print statistics for all ports
		print_io_stats(&sangoma_ports_bank2);	//print statistics for all ports
#endif
	

//		std::cout << "To refresh the screen press <Enter>" << std::endl;

		if (!iTimeout) {

			std::cout << "To exit, press <x> or <q>" << std::endl;

			sangoma_msleep(2000);	//sleep 2 seconds between each iteration to avoid busy loop!

			if( _kbhit() ) {
				switch(tolower(_getch()))
				{
				case 'x':
				case 'q':
					gRunning = false;
					break;
				default:
					std::cout << "Warning: Invalid command!" << std::endl;
					break;
				}
			}
		} else {
			sangoma_msleep(1000);
			timeout++;
			if (timeout >= iTimeout) {
				break;
			}
		}

	} // while( gRunning )
	
	std::cout << "Exiting Main Loop ... " << std::endl;

	sangoma_msleep(2000);	//Sleep 2 seconds before exiting to make sure the other thread exits

	return 0;
}

int G3TI_CreateThread(void *sangoma_ports_ptr)
{
#if defined(__WINDOWS__)
	DWORD	SangomaThreadId;
	HANDLE	hSangomaThread;

	//step 1 - create the single worker thread
    hSangomaThread = CreateThread( 
        NULL,                       /* no security attributes        */ 
        0,                          /* use default stack size        */ 
        (LPTHREAD_START_ROUTINE)SangomaThreadFunc, /* thread function     */ 
        sangoma_ports_ptr,				/* argument to thread function   */ 
        0,                          /* use default creation flags    */ 
        &SangomaThreadId			/* returns the thread identifier */ 
		);

	if(hSangomaThread == NULL){
		ERR_MAIN( "Failed to create Sangoma thread!!\n");
		return 1;
	}

#if 0
	//step 2 - set process priority to above normal
	if (!SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS)) {
		WRN_MAIN( "Failed to set Process priority!!\n");
	}

	//step 3 - set thread priority to real-time
	if(SetThreadPriority(hSangomaThread, THREAD_PRIORITY_TIME_CRITICAL) == FALSE){
		WRN_MAIN( "Failed to set Thread priority!!\n");
	}
#endif

	//success
	return 0;

#elif defined(__LINUX__)
# define CTHREAD_STACK_SIZE 1024 * 240
	pthread_t dwThreadId;
	pthread_attr_t attr;
	int result = -1;
	void *pParam = sangoma_ports_ptr;

	result = pthread_attr_init(&attr);
   	//pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	//pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, CTHREAD_STACK_SIZE);

	result = pthread_create(&dwThreadId, &attr, SangomaThreadFunc, pParam);
	pthread_attr_destroy(&attr);
	if (result) {
		return 1;
	}

	return 0;
#else
# error	Unsupported OS
#endif
}

#ifndef __WINDOWS__

int _kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}
#endif


int __cdecl main(int argc, char* argv[])
{
	unsigned int number_of_cards;
	struct BufferSettings buffer_settings;

	//create the configuration
	Configuration configuration;

	std::cout << "Application compiled on " << __DATE__ " " << __TIME__ << std::endl;

	// PROCESS THE SUPPLIED INPUT
	// Parse the command line arguements to retreive the card model to check for.  At this
	// time, we only expect to use either A104 or A108 or A116s in the system, but not a mixture of the
	// two.
	CardModel model;
	unsigned int number_of_ports_per_card = 0;
	//default line type is T1
	configuration.E1OrT1 = T1;
	configuration.HighImpedanceMode = false;
	configuration.MaxCableLoss = MCLV_12_0dB;//default value for both Hi Impedance and Normal mode
	configuration.TxTristateMode = false;
	configuration.Master=0;
	configuration.dchan=0;  //Disabled
    configuration.chunk_ms=0; //To preserver backward compatibility so that MTU is same for T1/E1
	configuration.api_mode=SNG_SPAN_MODE;
	configuration.dchan_seven_bit=0;
	configuration.dchan_mtp1_filter=0;


	buffer_settings.BufferMultiplierFactor = BufferSettings::MINIMUM_BUFFER_MULTIPLIER_FACTOR;// go to higher mulitiplier only if CPU usage is high
	//buffer_settings.BufferMultiplierFactor = 7;// go to higher mulitiplier only if CPU usage is high
    buffer_settings.NumberOfBuffersPerPort = BufferSettings::MINIMUM_NUMBER_OF_BUFFERS_PER_API_INTERFACE;

	if(parse_command_line_args(argc, argv, &model, &number_of_ports_per_card, configuration, buffer_settings)){
		return 1;
	}

	std::cout << "Press  <Enter> key to continue." << std::endl;
	sng_getch();

	load_driver_modules();

	std::cout << "Search for '" << (DECODE_CARD_TYPE(model)) << "' Sangoma card." << std::endl;

	number_of_cards = GetNumberOfCardsInstalled(model);
	INFO_MAIN("Found %d %s card(s).\n", number_of_cards, DECODE_CARD_TYPE(model));

	if(number_of_cards < 1){
		ERR_MAIN("No %s Sangoma cards running. Exiting...\n", DECODE_CARD_TYPE(model));
		return 1;
	}

	for (unsigned int card_number = 0; card_number < number_of_cards; card_number++) 
	{
		SangomaCard* p_card = new SangomaCard(card_number, model, buffer_settings, buffer_settings);
		sangoma_cards.push_back(p_card);

		INFO_MAIN("\nCard Driver version: %i.%i.%i.%i, Firmware version: %s, PCI Slot: %i, PCI Bus: %i\n",
			p_card->GetCardDriverVersion().Major, p_card->GetCardDriverVersion().Minor,
			p_card->GetCardDriverVersion().Build, p_card->GetCardDriverVersion().Revision,
			p_card->GetFirmwareVersion().c_str(), p_card->GetCardPciBus(), p_card->GetCardPciSlot());
	
		//Add this cards ports to the vector containing all the system's ports
		unsigned int number_of_ports = p_card->GetNumberOfPorts();
		for(unsigned int port_index = 0; port_index < number_of_ports; ++port_index)
		{
			total_number_of_ports++;
#if 0
			/* NC Debug code */
			if (total_number_of_ports > 8) {
				break;
			}
#endif
			if (total_number_of_ports > MAX_HANDLES) {
				sangoma_ports_bank2.push_back(p_card->GetPort(port_index) );
			} else {
				sangoma_ports.push_back(p_card->GetPort(port_index) );
			}
		}
	} // for (unsigned int card_number = 0; card_number < number_of_cards; card_number++)

	std::cout << "Press  <Enter> key to continue." << std::endl;
	sng_getch();

	gRunning = true;
	if (G3TI_CreateThread(&sangoma_ports))
	{
		ERR_MAIN("Failed to create receiver thread!\n");
		return 1;
	}

	if (sangoma_ports_bank2.size() > 0) {
		if (G3TI_CreateThread(&sangoma_ports_bank2))
		{
			ERR_MAIN("Failed to create receiver thread!\n");
			return 1;
		}
	}

	main_program_loop(&sangoma_ports, &sangoma_ports_bank2, configuration);
	
	gRunning = false;	


	//DELETE ALL THE SANGOMA CARDS
	for(unsigned int card_index = 0; card_index < sangoma_cards.size(); ++card_index)
	{
		delete sangoma_cards.at(card_index);
	}


	INFO_MAIN("\n%s: Done.\n", argv[0]);
	return 0;
}


