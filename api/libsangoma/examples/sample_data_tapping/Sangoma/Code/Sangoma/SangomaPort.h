///////////////////////////////////////////////////////////////////////////////////////////////
///	\file	SangomaPort.h
///	\brief	This file contains the declaration of the SangomaPort class
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _SANGOMA_PORT_H
#define _SANGOMA_PORT_H

#ifdef WIN32
#define __WINDOWS__
#include <windows.h>
#endif

#include <string>
#include "PortBuffer.h"

// Forward declarations to limit the third-party Sangoma includes to the .cpp file only.
struct sangoma_wait_obj;
typedef sangoma_wait_obj sangoma_wait_obj_t;
class sangoma_interface;
#ifdef LINUX
typedef pthread_mutex_t CRITICAL_SECTION;
#endif
struct wanpipe_api;
typedef wanpipe_api wanpipe_api_t;

namespace Sangoma {

// Read/Write error codes
const int ERROR_NO_DATA_AVAILABLE = -1;						///< Error code indicating that a SangomaPort::Read was attempted when no data was available (indicates a possible abuse of the API by polling for data instead of waiting for the DataAvailableEvent)
const int ERROR_INVALID_DESTINATION_BUFFER = -2;			///< Error code indicating that the destination buffer provided to SangomaPort::Read was NULL
const int ERROR_INVALID_SOURCE_BUFFER = -3;					///< Error code indicating that the source buffer provided to SangomaPort::Write was NULL
const int ERROR_NO_DATA_WRITTEN = -4;						///< Error code indicating that the SangomaPort::Write command failed
const int ERROR_PORT_NOT_OPEN = -5;							///< Error code indicating that the Read or Write command failed because the port has not been opened.  Call SangomaPort::Open() successfully before attempting to Read or Write.
const int ERROR_INVALID_DESTINATION_BUFFER_TOO_SMALL = -6;	///< Error code indicating that the destination buffer provided to SangomaPort::Read was to small to contain a full buffer's worth of data (configuration error)
const int ERROR_INVALID_READ_REQUEST = -7;					///< Error code indicating that the input to SangomaPort::Read was invalid
/// \todo Add more detailed read/write failure codes as available from the API (more detail is better)
const int ERROR_DEVICE_IOCTL = -8;							///< Error code indicating that DeviceIoControl() for API Device Driver failed. Check '/System32/drivers/wanpipelog.txt' messages log for details.
const int ERROR_INVALID_TX_DATA_LENGTH = -8;				///< Error code indicating that SangomaPort::Write failed because user data was too long.


const std::string NO_ERROR_OCCURRED = "";					///< String indicating that no error occurred on the last operation (returned by GetLastError() when the last operation was successful)
const unsigned int MAX_SANGOMA_BUFFER_SIZE = 8188;			///< Maximum Sangoma buffer size in bytes
const unsigned int ALARM_WAIT_TIME_MS = 30000;				///< The number of milliseconds that the Sangoma API must wait in order to be able to reliably report the absence of alarms.
															///		If GetAlarms() is called before this amount of time has elapsed after setting a configuration, the card will always
															///		report no alarms, giving a false positive that we have found synchronization.

///////////////////////////////////////////////////////////////////////////////////////////////
///	\enum	E1OrT1
///	\brief	Enumeration indicating whether a port is in E1 or T1 mode
///////////////////////////////////////////////////////////////////////////////////////////////
enum E1OrT1 {
	E1=32,			///< Indicates that the port is configured as an E1 port
	T1=24			///< Indicates that the port is configured as a T1 port
};

///////////////////////////////////////////////////////////////////////////////////////////////
///	\enum	FramingType
///	\brief	Enumeration indicating the framing type and signaling of the port
///////////////////////////////////////////////////////////////////////////////////////////////

enum FramingType {
	//DAVIDR: i added the UNFRAMED E1 types
	E1_CCS_UNFRAMED,		///< Indicates that Common Channel Signalling (CCS) with Unframed E1, all 32 timeslots available for the E1 port
	E1_CAS_UNFRAMED,		///< Indicates that Channel Associated Signaling (CAS) with Unframed E1, all 32 timeslots available for the E1 port

	E1_CAS_CRC4_ON,			///< Indicates that Channel Associated Signaling (CAS) with CRC4 is the framing type for the E1 port
	E1_CAS_CRC4_OFF,		///< Indicates that Channel Associated Signaling (CAS) with no CRC4 is the framing type for the E1 port
	E1_CCS_CRC4_ON,			///< Indicates that Common Channel Signalling (CCS) with CRC4 is the framing type for the E1 port
	E1_CCS_CRC4_OFF,		///< Indicates that Common Channel Signalling (CCS) with no CRC4 is the framing type for the E1 port

	T1_EXTENDED_SUPERFRAME,	///< Indicates that Extended Superframe (ESF, also called D5 framing) is the framing type for the T1 port
	T1_SUPERFRAME			///< Indicates that Superframe (also called D4 or D3/D4 framing) is the framing type for the T1 port
};

#define DECODE_FRAMING(framing)\
		(framing == E1_CCS_UNFRAMED)? "E1_CCS_UNFRAMED" : \
		(framing == E1_CAS_UNFRAMED)? "E1_CAS_UNFRAMED" : \
		(framing == E1_CAS_CRC4_ON) ? "E1_CAS_CRC4_ON" : \
		(framing == E1_CAS_CRC4_OFF)? "E1_CAS_CRC4_OFF" : \
		(framing == E1_CCS_CRC4_ON) ? "E1_CCS_CRC4_ON" : \
		(framing == E1_CCS_CRC4_OFF)? "E1_CCS_CRC4_OFF" : \
		(framing == T1_EXTENDED_SUPERFRAME) ? "T1_EXTENDED_SUPERFRAME" : \
		(framing == T1_SUPERFRAME)	? "T1_SUPERFRAME" : "???"

#define DECODE_CARD_TYPE(type) \
		(type == A104)? "A104" : \
		(type == A108)? "A108" : \
		(type == A116)? "A116" : \
		(type == T116)? "T116" : "Invalid Card" 
		

//DAVIDR: On E1 line Sangoma card provides access to timeslot 0 ONLY if Framing is "Unframed E1".

///////////////////////////////////////////////////////////////////////////////////////////////
///	\enum	LineCoding
///	\brief	Enumeration indicating whether line coding is on or off for the port
///////////////////////////////////////////////////////////////////////////////////////////////
enum LineCoding {
	OFF,		///< Indicates that line coding is off (meaning set to the default AMI value)
	ON			///< Indicates that line coding is on (meaning B8ZS for T1, and HDB3 for E1)
};

/**********************************************************
DAVIDR: Receiver Sensitivity (Max Cable Loss Allowed) (dB)
The user will have the following option to choose:
1. HI IMPEDANCE mode:
30 dB
22.5 dB
17.5 dB
12 dB

Depending on selection, use the following values:
300
225
175
120

2. NORMAL mode:
12 dB
18 dB
30 dB
36 dB (for T1)
43 dB (for E1)

Depending on selection, use the following values:
120
180
300
360
430
**********************************************************/
enum MAX_CABLE_LOSS{
	MCLV_43_0dB=430,
	MCLV_36_0dB=360,
	MCLV_30_0dB=300,
	MCLV_22_5dB=225,
	MCLV_18_0dB=180,
	MCLV_17_5dB=175,
	MCLV_12_0dB=120
};

typedef enum SNG_API_MODE{
	SNG_SPAN_MODE=0,
	SNG_CHAN_MODE=1,
	SNG_DATA_MODE=2
} sng_api_mode_t;

///////////////////////////////////////////////////////////////////////////////////////////////
///	\struct	Configuration
///	\brief	Structure containing all settings necessary to configure a Sangoma port
///////////////////////////////////////////////////////////////////////////////////////////////
struct Configuration {
	Sangoma::E1OrT1  E1OrT1;  ///< Configures the port as either E1 or T1
	FramingType  Framing;  ///< Framing type and signalling of the E1/T1 port
	Sangoma::LineCoding  LineCoding;  ///< Indicates whether line coding is ON or OFF
	bool  HighImpedanceMode;  ///< true to indicate High Impedance mode is to be enabled, false to disable it
	MAX_CABLE_LOSS  MaxCableLoss;  ///< If High Impedance is true, indicates value of external resistor.
	bool  TxTristateMode;  ///< If true, indicates transmitter is disabled on T1/E1 level.
	bool  Master;		///< If true, configure for master clock
	int   dchan;        ///< Integer value of a dchan 
	int   chunk_ms;     ///< Integer value of ms chunk size default 20 (160bytes per timeslot)
	sng_api_mode_t api_mode;
	int   dchan_seven_bit;
	int   dchan_mtp1_filter;
	
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///	\fn		Configuration::Configuration()
    ///	\brief	Constructor (for the Configuration structure)
    ///	\author	J. Markwordt
    ///	\date	10/04/2007
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Configuration():
        E1OrT1(E1),
	    Framing(E1_CAS_CRC4_ON),
	    LineCoding(OFF),
	    HighImpedanceMode(true),
	    TxTristateMode(false),
        Master(false),
		dchan(0)
    {}
};

///////////////////////////////////////////////////////////////////////////////////////////////
///	\struct	Statistics
///	\brief	Structure containing all statistics related to a Sangoma port
///////////////////////////////////////////////////////////////////////////////////////////////
struct Statistics {
	// The following time values are to be calculated using the time() function found in time.h.  This
	// is standard ANSI C and returns the current time as the number of seconds elapsed since midnight
	// on January 1, 1970.  Operations involving elapsed time can then be done by subtracting from
	// current time (or some other instant) and converting to hours, minutes and seconds
	// The function struct tm * localtime(const time_t * timer); will convert the time to a tm structure
	// that is broken into date and time fields

	// Statistics maintained in software by the SangomaPort class
	time_t	TimeLastOpened;					///< System time of the last time this port was opened
	time_t	TimeLastClosed;					///< System time of the last time this port was closed
	time_t	TimeLastFoundSync;				///< System time of the last time this port found sync with a valid configuration
	time_t	TimeLastLostSync;				///< System time of the last time this port lost sync when already in sync

	unsigned int OpenCount;			///< Count of the number of times the port was opened (i.e. number of times SangomaPort::Open() returned successfully)
	unsigned int CloseCount;		///< Count of the number of times the port was closed (i.e. number of times SangomaPort::Close() returned successfully)
	unsigned int FoundSyncCount;	///< Count of the number of times synchronization was found on the port (i.e. number of times sync was found when the port was opened)
	unsigned int LostSyncCount;		///< Count of the number of times synchronization was lost on the port (i.e. number of times the LineDisconnected event was raised, since both indicate a loss of sync on the line)

	// Statistics maintained in hardware, a command will have to be sent to the port to retrieve this data
	unsigned int	BytesReceived;			///< Number of bytes received on this port
	unsigned int	BytesSent;				///< Number of bytes transmitted from this port
	unsigned int	Errors;	   	 			///< Number of bytes transmitted from this port

	///////////////////////////////////////////////////////////////////////////////////////////////
    ///	\fn		Statistics::Statistics()
    ///	\brief	Constructor (for the Statistics structure)
    ///	\author	J. Markwordt
    ///	\date	10/04/2007
    ///////////////////////////////////////////////////////////////////////////////////////////////
    Statistics():
     	TimeLastOpened(0),
	    TimeLastClosed(0),
	    TimeLastFoundSync(0),
	    TimeLastLostSync(0),
	    OpenCount(0),
	    CloseCount(0),
	    FoundSyncCount(0),
	    LostSyncCount(0),
	    BytesReceived(0),
	    BytesSent(0),
        Errors(0)
    {}
};

///////////////////////////////////////////////////////////////////////////////////////////////
///	\struct	Alarms
///	\brief	Structure containing a series off boolean flags to indicate the presence of Sangoma
///			hardware alarms.  If true, indicates that the given alarm is present, if false indicates
///			that the alarm is not present.
///////////////////////////////////////////////////////////////////////////////////////////////
struct Alarms {
	bool LossOfSignal;					///< Indicates that the Loss of Signal (ALOSV) alarm is present if true
	bool ReceiveLossOfSignal;			///< Indicates that the Receive Loss of Signal (LOS) alarm is present if true
	bool AlternateLossOfSignalStatus;	///< Indicates that the Alternate loss of Signal Status (ALTLOS) alarm is present if true
	bool OutOfFrame;					///< Indicates that the Out of Frame (OOF) alarm is present if true
	bool Red;							///< Indicates that the Telco Red Alarm condition (RED) is present if true
	bool AlarmIndicationSignal;			///< Indicates that the Alarm Indication Signal (AIS) is present if true
	bool LossOfSignalingMultiframe;		///< Indicates that the Loss of Signaling Multiframe (OOSMFV) alarm is present if true
	bool LossOfCrcMultiframe;			///< Indicates that the Loss of CRC Multiframe (OOCMFV) alarm is present if true
	bool OutOfOfflineFrame;				///< Indicates that the Out of Off-Line Frame (OOOFV) alarm is present if true
	bool ReceiveLossOfSignalV;			///< Indicates that the Receive Loss of Signal (RAIV) alarm is present if true
	bool Yellow;						///< Indicates that the Receive Telco Yellow Alarm (YEL) is present if true

	Alarms();							///< Constructor
	Alarms(bool value)					///< Constructor
	{
		LossOfSignal = value;
		ReceiveLossOfSignal = value;
		AlternateLossOfSignalStatus = value;
		OutOfFrame = value;
		Red = value;
		AlarmIndicationSignal = value;
		LossOfSignalingMultiframe = value;
		LossOfCrcMultiframe = value;
		OutOfOfflineFrame = value;
		ReceiveLossOfSignalV = value;
		Yellow = value;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////
/// \struct	DriverVersion
///	\brief	Structure containing the driver version number
///////////////////////////////////////////////////////////////////////////////////////////////
struct DriverVersion {
	unsigned int Major;		///< The major version number of the driver (i.e. the 1 in 1.2.3.4)
	unsigned int Minor;		///< The minor version number of the driver (i.e. the 2 in 1.2.3.4)
	unsigned int Build;		///< The build version number of the driver (i.e. the 3 in 1.2.3.4)
	unsigned int Revision;	///< The revision version number of the driver (i.e. the 4 in 1.2.3.4)

	DriverVersion();		///< Constructor

    std::string ToString();	// G3 specific change
};

///////////////////////////////////////////////////////////////////////////////////////////////
///	\enum	PortEventType
///	\brief	Enumeration indicating the type of event that that triggered a call to the PortEventHandler
///////////////////////////////////////////////////////////////////////////////////////////////
enum PortEventType {
	Unsignalled,		///< Default value, indicates that the event has not been signalled (should not be seen in actual events that are triggered)
	ReadDataAvailable,	///< Indicates that the read buffer is full (SangomaPort::Read() can be called to service the buffer)
	LineConnected,		///< Indicates that a port that was unoccupied has had an E1/T1 line plugged in
	LineDisconnected,	///< Indicates that a port that had an E1/T1 line plugged in has had the line disconnected
	TransmitBufferAvailable ///< Indicated that a port has at least one free Transmit buffer.
};

///////////////////////////////////////////////////////////////////////////////////////////////
///	\struct	PortEvent
///	\brief	Structure representing an event that occurred on a port within the card
///////////////////////////////////////////////////////////////////////////////////////////////
struct PortEvent {
	PortEventType EventType;			///< Status code indicating the type of event that occured
	sangoma_wait_obj_t *EventHandle;	///< Handle to the Sangoma waitable object

	PortEvent();				///< Constructor
};

///////////////////////////////////////////////////////////////////////////////////////////////
///	\struct BufferSettings
///	\brief	Structure containing settings for configuring the host memory buffers assigned to a
///			Sangoma port.  Data is transferred to host memory via DMA, and an event is set to indicate
///			that the buffers are full and read to be read, so it is inportant to configure the host
///			memory buffers to the needs of your application.  Sangoma defines a MRU/MTU of 8188 bytes
///			(the maximum size of an HDLC frame) and assigns buffers of this size to a port (although
///			data is always read or written in even frames of 8184 bytes for T1 and 8160 for E1
///			(32 timeslots).
///
///			The size of the buffers will be configurable in multiples of this 8188 bytes enforced
///			by hardware, such that 8188 * BufferMultiplierFactor = BufferSize (i.e. a BufferMultiplierFactor
///			of 4 will result in BufferSize = 4 * 8188 = 32752 byes).
///
///			Multiple buffers are assigned for transmit and receiving to queue a certain amount of
///			data.  These are configured by setting NumberOfBuffersPerPort.  If you intend to only
///			use a SangomaPort for either receive or transmit, the NumberOfBuffersPerPort for that
///			set of buffers can be set to 0 (SangomaPort also has special constructors which are
///			for receive-only or transmit-only ports).
///////////////////////////////////////////////////////////////////////////////////////////////
struct BufferSettings {
	unsigned int BufferMultiplierFactor;	///< Number to multiply the MAX_SANGOMA_BUFFER_SIZE by in order to get the size of the buffers allocated in host memory
	unsigned int NumberOfBuffersPerPort;	///< Number of buffers in host memory that will be dedicated to with receiving or transmitting data on this port.

    const static unsigned int MAXIMUM_BUFFER_MULTIPLIER_FACTOR;
    const static unsigned int MINIMUM_BUFFER_MULTIPLIER_FACTOR;
    const static unsigned int MINIMUM_NUMBER_OF_BUFFERS_PER_API_INTERFACE;

    // G3 specific change start
    /// \fn     BufferSettings::BufferSettings()
    /// \brief  Constructor to set the members to default values
    BufferSettings()
    {
        Reset();
    }

    /// \fn     BufferSettings::Reset()
    /// \brief  Set the members to default values
    void Reset()
    {
        BufferMultiplierFactor = 0;
        NumberOfBuffersPerPort = 0;
    }
    // G3 specific change end
};

///////////////////////////////////////////////////////////////////////////////////////////////
///	\class	SangomaPort
///	\brief	This class encapsulates the functionality of a physical Sangoma port found on the
///			A104 and A108 card models.
///	\author	J. Markwordt
///	\date	10/03/2007
///////////////////////////////////////////////////////////////////////////////////////////////
class SangomaPort {
public:
	SangomaPort(
        const unsigned int card_number,
        const unsigned int port_number,
		const BufferSettings& ReceiveBufferSettings,
		const BufferSettings& TransmitBufferSettings);

	~SangomaPort();

	bool Open(const Configuration& configuration);
	bool Close();

	int Read(unsigned int event_flag);				///< Reads Rx data or an Event and places them in the ReceiveBuffer.
													///< Returns reason for wait object being signaled

	unsigned char *GetRxDataBuffer();
	unsigned int GetRxDataLength();

	int Write(unsigned char* p_source_buffer, const unsigned int bytes_to_write);	///< Writes the specified number of bytes from the source buffer to the port

    bool GetSynchronizationStatus();

    unsigned int GetCardNumber() const {return CardNumber;}
	unsigned int GetPortNumber() const;				///< Returns this port's number

	bool GetConfiguration(Configuration& current_configuration);

    PortEvent GetEvent() const;						///< Return the PortEvent associated with this port
	bool GetIsOpen();							///< Return true if this port is currently open, false otherwise

    DriverVersion GetDriverVersion(); ///< Returns this port's driver version number
	Statistics GetStatistics();						///< Get the statistics for this port
	void ClearStatistics();							///< Clear the statistics for this port
	void FlushBuffers();							///< Clear the statistics for this port

	Alarms GetAlarms();								///< Retrieves the alarms from the hardware for this port

    bool GetReceiverLevelInDb(float& receiver_level_in_db);

	std::string GetLastError() const;				///< Returns a description of the last error that occurred (NO_ERROR_OCCURRED if the last command was successful)

	PortBuffer *pReceiveBuffer; ///< Buffers allocated to receive data for this port
	PortBuffer *pTransmitBuffer; ///< Buffers allocated to transmit data for this port

	unsigned int GetUnmappedPortNumber();

	int SetAndOpenTxFile(const char *szTxFileName);		///< Open a file which will be continuesly transmitted

    int GetTransmitMtu() { return TxMtu; } ///< Provides the chunk size to be used each time data is written to the port.  This changes based on the frame type.
	int WriteChunkOfTxFile();						///< Transmit a chunk of the TxFile

	int LineLoopbackEnable();	///< Enable line loopback for testing. All transmitted data will be also received locally. Requires the Port to be in Master clock mode.
	int LineLoopbackDisable();	///< Disable line loopback which was enabled by SangomaPort::LineLoopbackEnable()

	unsigned int RxDiscardCounter;				///< Number of rx buffers to discard after LineConnected event (the buffers may contain idle data).
	unsigned int RxCount;						///< Number of rx buffers to discard after LineConnected event (the buffers may contain idle data).
	unsigned int SyncCount;						///< Number of rx buffers to discard after LineConnected event (the buffers may contain idle data).
	unsigned int RxLength;						///< Number of rx buffers to discard after LineConnected event (the buffers may contain idle data).
 	unsigned int next_framing_type;
	time_t rx_pkt_timeout;

private:
	// Copy constructor and assigment operator are made private
	// and left unimplemented to ensure that the SangomaPort object cannot be copied
	SangomaPort(const SangomaPort& port);
	SangomaPort& operator=(const SangomaPort& rhs);

	unsigned int CardNumber;						///< The number of the SangomaCard that this port is associated with (zero-indexed)
	unsigned int PortNumber;						///< The number assigned to this port (zero-indexed)
	PortEvent Event;								///< Event used to trigger port events with
	bool IsOpen;									///< true if this port is currently open, false otherwise
	Configuration CurrentConfiguration;				///< The current configuration of this port
	Statistics PortStatistics;						///< Statistics related to this port (made member because some are maintained by the class, some are maintained by the hardware)

	std::string LastError;							///< String description of the last error that occurred (should be set if an error occurs in any SangomaPort member functions, or NO_ERROR_OCCURRED if successful)

	BufferSettings ReceiveBufferSettings; ///< These are the settings for the buffer that holds the data when it is received.
    BufferSettings TransmitBufferSettings; ///< These are the settings for the buffer that holds the data that is to be transmitted.

	sangoma_interface *pSangomaInterface;

	CRITICAL_SECTION cs_CriticalSection;
	void EnterPortCriticalSection();
	void LeavePortCriticalSection();
	int TryEnterPortCriticalSection();
	int SetPortConfiguration(const Configuration & config);

	/*! API header for rx data */
	wp_api_hdr_t	*rxhdr;

	/*! API header for tx data */
	wp_api_hdr_t	*txhdr;

	/*! API command structure used to execute API commands. This command structure is used with libsangoma library */
	wanpipe_api_t	*wp_api;

	FILE	*pTxFile;
	int		TxMtu;
};

}// namespace Sangoma

#endif //_SANGOMA_PORT_H
