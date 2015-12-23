///////////////////////////////////////////////////////////////////////////////////////////////
/// \file	ChannelizedBuffer.h
///	\brief	This file contains the definition of the ChannelizedBuffer class.  This class provides
///			a set of dedicated buffers (1 survey, 8 data)  for up to 32 timeslots of raw E1 or 
///			T1 data.
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CHANNELIZED_BUFFER_H
#define _CHANNELIZED_BUFFER_H

#include <vector>

typedef enum {UNSURVEYED_FRAMING=0, E1_FRAMING=32, T1_FRAMING=24} E1OrT1Framing;				///< Type of port (i.e., E1 or T1). This can also be used as the frame size

///////////////////////////////////////////////////////////////////////////////////////////////
///	\class		ChannelizedBuffer
///	\brief		This class provides a set of dedicated buffers (1 survey, 8 data)  for up 
///				to 32 timeslots of raw E1 or T1 data. It accepts data inserted from a L1
///				source. The first data byte must be timeslot 0 for this routine to work
///				correctly.
///	\author		Josh Markwordt
///	\date       07/22/2005
///////////////////////////////////////////////////////////////////////////////////////////////
class ChannelizedBuffer  
{
public:
	explicit ChannelizedBuffer(const unsigned int buffer_size);		///< Constructor
	ChannelizedBuffer(const ChannelizedBuffer &other);		///< Copy Constructor
	~ChannelizedBuffer();									///< Destructor
	void Reset(const bool clear_buffers);					///< Resets the buffers so that data will be inserted at the beginning

	void InsertAndChannelizeFramedData(unsigned char *p_source_buf, const unsigned int num_bytes_to_insert, const E1OrT1Framing frame_size)
	{

	}
};

#endif //_CHANNELIZED_BUFFER_H
