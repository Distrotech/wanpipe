#ifndef _PORT_BUFFER_H
#define _PORT_BUFFER_H

#include <stdlib.h>
#include <malloc.h>

// Forward declarations to limit the third-party Sangoma includes to the .cpp file only.
struct wp_api_hdr;
typedef wp_api_hdr wp_api_hdr_t;

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

	wp_api_hdr_t	*ApiHeader;		///< Will store the size of the Rx/Tx DATA at DataBuffer coming To/From User.

public:
	PortBuffer(unsigned int size);
	~PortBuffer(void);

	unsigned char* GetPortBuffer();

	unsigned int GetPortBufferSize();

	unsigned char* GetUserDataBuffer();

	unsigned int GetUserDataLength();

	void SetUserDataLength(unsigned int user_data_length);

	unsigned int GetMaximumUserDataBufferSize();

private:
    PortBuffer(const PortBuffer& object_to_copy); // Made private to disallow copying. 
	PortBuffer& operator=(const PortBuffer& rhs); // Made private to disallow copying. 
};

#endif //_PORT_BUFFER_H
