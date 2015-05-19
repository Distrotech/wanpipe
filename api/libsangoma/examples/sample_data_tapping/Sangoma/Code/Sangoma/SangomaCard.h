///////////////////////////////////////////////////////////////////////////////////////////////
/// \mainpage Sangoma API Design Document
///
/// \section overview Overview
///
///		The Sangoma API defines several C++ classes that provide an interface to perform commands
///		on the Sangoma A104 and A108 E1/T1 cards.  Each physical PCI card is represented by a
///		\link Sangoma::SangomaCard \endlink object which contains the appropriate number of \link Sangoma::SangomaPort \endlink
///		objects to encapsulate their behaviour.
///
///		The Sangoma API will transfer data read from the physical port to host memory using Direct
///		Memory Access (DMA), filling a series of buffers and then raising an event to alert calling
///		programs that data is available to be read.  By eliminating polling for data, this will
///		improve efficiency.
///
///	\subsection card_object SangomaCard
///		A SangomaCard object represents a physical Sangoma A104 or A108 PCI E1/T1 card containing
///		4 or 8 E1/T1 ports respectively.  The relationship between SangomaCard and SangomaPorts can
///		be seen below for the Sangoma A104:
///
///		\image html SangomaA104Overview.jpg
///
///		and for the Sangoma A108:
///
///		\image html SangomaA108Overview.jpg
///
///		Any behaviour which occurs at the card level and access to the SangomaPort objects (which do
///		most of the actual work) are controlled through this class.
///
///	\subsection port_object SangomaPort
///		A SangomaPort represents a physical E1/T1 port and encapsulates the ability to set the
///		configuration of the port, read, write and retrieve hardware alarms and statistics.  Note
///		that since multiple E1/T1 lines are multiplexed into a single physical port on the Sangoma
///		A108, there will be two SangomaPort objects for each physical port of the A108 (as seen in the
///		figure above).
///
///		SangomaPorts can be accessed through SangomaCard::GetPort() and then manipulated directly.
///
///	\subsection port_buffers SangomaPort Buffers
///		Each SangomaPort contains software buffers to store data read from the physical card.
///		Buffers are allocated as multiples of 8188 bytes (8188 bytes is the size of the Sangoma
///		card’s hardware buffer.  The software buffers use the same size to simplify the processing.).
///		Therefore the size of a single buffer will be set by the BufferMultiplierFactor, such that the
///		size of a buffer is 8188 * BufferMultiplierFactor bytes.  SangomaPorts can have multiple buffers.
///		The number of buffers is controlled by NumberOfBuffersPerPort.
///
///		When a buffer becomes full the ReadDataAvailable event is signaled to indicate data is
///		available for retrieval.  Note that a full buffer does not necessarily use the fully
///		allocated size.  It always contains full E1 (32 bytes) or T1 (24 bytes) frames.  Therefore
///		a small portion at the end of each buffer may be left unused.
///
///		The diagram below provides an example of the buffers contained in a SangomaPort.
///
///		\image html SangomaPortBuffers.jpg
///
///		To store data the SangomaPort will cycle through its buffers in a circular fashion so that when the
///		current buffer becomes full data can be stored in the next buffer while the previous one
///		is waiting to be read.  The diagram below shows how the buffers will be used within the
///		SangomaPort.
///
///		\image html SangomaPortBuffersCycle.jpg
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
///	\file	SangomaCard.h
///	\brief	This file contains the declaration of the SangomaCard class
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _SANGOMA_CARD_H
#define _SANGOMA_CARD_H

#include <string>
#include <vector>
#include "SangomaPort.h"

const unsigned int NUMBER_OF_A104_PORTS = 4;	///< The number of ports in a Sangoma A104 card
const unsigned int NUMBER_OF_A108_PORTS = 8;	///< The number of ports in a Sangoma A108 card
const unsigned int NUMBER_OF_A116_PORTS = 16;	///< The number of ports in a Sangoma A116 card
const unsigned int NUMBER_OF_T116_PORTS = 16;	///< The number of ports in a Sangoma T116 card

///////////////////////////////////////////////////////////////////////////////////////////////
///	\namespace	Sangoma
///	\brief		The Sangoma namespace is used to wrap all Sangoma classes to identify them as a
///				package and to allow simple names for structures, constants, enums and other
///				identifiers without having to worry about creating naming conflicts with other
///				packages.
///////////////////////////////////////////////////////////////////////////////////////////////
namespace Sangoma {

// G3 specific change begin.
#if defined WIN32
const std::string EXPECTED_DRIVER_VERSION = "7.0.0.0";	///< This is the expected Sangoma card driver version.
#elif defined LINUX
const std::string EXPECTED_DRIVER_VERSION = "7.0.1.0";
#endif
// G3 specific change end.

///////////////////////////////////////////////////////////////////////////////////////////////
///	\enum	CardModel
///	\brief	Enumeration indicating the model of Sangoma card that this SangomaCard is
///////////////////////////////////////////////////////////////////////////////////////////////
enum CardModel {
	A104,			///< Indicates the Sangoma A104 card model
	A108,			///< Indicates the Sangoma A108 card model
	A116,			///< Indicates the Sangoma A116 card model
	T116            ///< Indicates the Sangoma T116 card model
};

unsigned int GetNumberOfCardsInstalled(const CardModel model);	///< Checks for the number of cards of the model type provided are installed


///////////////////////////////////////////////////////////////////////////////////////////////
///	\class	SangomaCard
///	\brief	This class encapsulates the functionality of a physical Sangoma card (models A104
///			or A108).
///	\author	J. Markwordt
///	\date	10/03/2007
///////////////////////////////////////////////////////////////////////////////////////////////
class SangomaCard {
public:
	explicit SangomaCard(const unsigned int card_number, const CardModel model,
		const BufferSettings & ReceiveBufferSettings,
		const BufferSettings & TransmitBufferSettings);	///< Constructor
	~SangomaCard();											///< Destructor

	unsigned int GetCardNumber() const;						///< Returns the card number of this Sangoma card
	CardModel GetModel() const;								///< Returns the model of Sangoma card that this object represents (A104 or A108)
	unsigned int GetNumberOfPorts() const;					///< Returns the number of ports in this card
	SangomaPort* GetPort(const unsigned int port_number);	///< Returns a pointer to the port requested

	DriverVersion GetCardDriverVersion();			///< Returns the card's driver version number
	std::string GetSerialNumber() const;			///< Returns the card's serial number
	std::string GetFirmwareVersion() const;			///< Returns the card's firmware version
	unsigned int GetCardPciBus() const;				///< Returns the number of the PCI bus that the card is located in
	unsigned int GetCardPciSlot() const;			///< Returns the number of the PCI bus that the card is located in

	std::string GetLastError() const;				///< Returns string description of the last error that occured for this card

private:
	// Copy constructor and assigment operator are made private
	// and left unimplemented to ensure that the SangomaCard object cannot be copied
	SangomaCard(const SangomaCard& port);			///< Copy Constructor (do not implement)
	SangomaCard& operator=(const SangomaCard& rhs);	///< Assignment Operator (do not implement)

	unsigned int CardNumber;						///< Number of this card in the system (zero indexed)
	CardModel Model;								///< The model of Sangoma card that this object represents (A104 or A108)
	unsigned int NumberOfPorts;						///< The number of SangomaPorts found in this card
	std::vector<SangomaPort*> Ports;				///< Vector of the SangomaPorts contained in this card
	std::string LastError;							///< String description of the last error that occurred (should be set if an error occurs in any SangomaCard member functions, or empty if successful)
};

}// namespace Sangoma

#endif //_SANGOMA_CARD_H
