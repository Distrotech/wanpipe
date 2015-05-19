#ifndef _mem_h
#define _mem_h


#define COMMAND_NACK     0x00
#define COMMAND_ACK      0x01
#define COMMAND_READ_PM  0x02
#define COMMAND_WRITE_PM 0x03
#define COMMAND_READ_EE  0x04
#define COMMAND_WRITE_EE 0x05
#define COMMAND_READ_CM  0x06
#define COMMAND_WRITE_CM 0x07
#define COMMAND_RESET    0x08
#define COMMAND_READ_ID  0x09

#define PM30F_ROW_SIZE 32
#define PM33F_ROW_SIZE 64*8
#define EE30F_ROW_SIZE 16

enum eFamily
{
	dsPIC30F,
	dsPIC33F,
	PIC24H,
	PIC24F
};


class mem_cMemRow
{
public:

	enum eType
	{
		Program,
		EEProm,
		Configuration
	};
	mem_cMemRow(eType Type, unsigned int StartAddr, int RowNumber, eFamily Family);

	int InsertData(unsigned int Address, char * pData);
	int FormatData(void);
	int SendData  (void*);

private:
	char		*m_pBuffer;
	unsigned int	m_Address;
	int		m_bEmpty;
	eType		m_eType;
	unsigned short	m_Data[PM33F_ROW_SIZE*2];
	int		m_RowNumber;
	eFamily		m_eFamily;
	int		m_RowSize;
};


#endif
