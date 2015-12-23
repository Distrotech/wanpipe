#ifndef _PORT_BUFFER_H
#define _PORT_BUFFER_H

#include "sangoma_interface.h"

#include <stdlib.h>
#include <malloc.h>

///////////////////////////////////////////////////////////////////////////////////////////////
///	\class	PortBuffer
///	\brief	Structure containing a buffer for the port to read from or write to. The
///			size of buffer is indicated so that it can be allocated dynamically
///	\author	David Rokhvarg (davidr@sangoma.com)
///	\date	11/13/2007
///////////////////////////////////////////////////////////////////////////////////////////////
class PortBuffer {
	unsigned char *Buffer;			///< Pointer to dynamically allocated buffer of BufferSize size.
	unsigned int BufferSize;		///< The size of the unsigned char buffer that was allocated at Buffer.
	
	wp_api_hdr_t	ApiHeader;		///< Will store the size of the Rx/Tx DATA at DataBuffer coming To/From User.

public:
	PortBuffer(unsigned int size);
	~PortBuffer(void);

	///////////////////////////////////////////////////////////////////////////////////////////////
	///	\fn		PortBuffer::PortBuffer()
	///	\brief	Returns pointer to Buffer.
	///	\return	pointer to Buffer.
	///	\author	David Rokhvarg (davidr@sangoma.com)
	///	\date	11/15/2007
	///////////////////////////////////////////////////////////////////////////////////////////////
	unsigned char *GetPortBuffer();

	///////////////////////////////////////////////////////////////////////////////////////////////
	///	\fn		PortBuffer::GetPortBufferSize()
	///	\brief	Returns the total Buffer size.
	///	\return	Buffer size.
	///	\author	David Rokhvarg (davidr@sangoma.com)
	///	\date	11/15/2007
	///////////////////////////////////////////////////////////////////////////////////////////////
	unsigned int GetPortBufferSize();

	///////////////////////////////////////////////////////////////////////////////////////////////
	///	\fn		PortBuffer::GetUserDataBuffer()
	///	\brief	Returns pointer to buffer containng user data.
	///			When data is recieved, this buffer will hold rx data.
	///			When data is transmitted, this buffer should contain tx data before Write() is called.
	///	\return	pointer to buffer containing user data.
	///	\author	David Rokhvarg (davidr@sangoma.com)
	///	\date	11/15/2007
	///////////////////////////////////////////////////////////////////////////////////////////////
	unsigned char *GetUserDataBuffer();

	///////////////////////////////////////////////////////////////////////////////////////////////
	///	\fn		PortBuffer::GetUserDataLength()
	///	\brief	Returns current length of user data at UserDataBuffer.
	///			When data is recieved, GetUserDataLength() will return length of rx data.
	///	\return	current length of user data at UserDataBuffer..
	///	\author	David Rokhvarg (davidr@sangoma.com)
	///	\date	11/15/2007
	///////////////////////////////////////////////////////////////////////////////////////////////
	unsigned int GetUserDataLength();

	///////////////////////////////////////////////////////////////////////////////////////////////
	///	\fn		PortBuffer::SetUserDataLength()
	///	\brief	Set lenght of user data at UserDataBuffer.
	///			When data is transmitted, SetUserDataLength() will indicate to Sangoma API how many
	///			should be transmitted.
	///	\return	there is no return value.
	///	\author	David Rokhvarg (davidr@sangoma.com)
	///	\date	11/15/2007
	///////////////////////////////////////////////////////////////////////////////////////////////
	void SetUserDataLength(unsigned int user_data_length);

	///////////////////////////////////////////////////////////////////////////////////////////////
	///	\fn		PortBuffer::GetMaximumUserDataBufferSize()
	///	\brief	Returns maximum size of user data buffer. This is the maximum length of user data
	///			which can be transmitted/received in one buffer.
	///	\return	maximum size of user data buffer.
	///	\author	David Rokhvarg (davidr@sangoma.com)
	///	\date	11/15/2007
	///////////////////////////////////////////////////////////////////////////////////////////////
	unsigned int GetMaximumUserDataBufferSize();
};

#endif //_PORT_BUFFER_H
