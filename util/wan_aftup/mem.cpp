#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "mem.h"

extern int   WriteCommBlock (void*, char *pBuffer ,  int BytesToWrite);
extern int   ReceiveData (void*, char *pBuffer ,  int BytesToReceive);

/******************************************************************************/
mem_cMemRow::mem_cMemRow(eType Type, unsigned int StartAddr, int RowNumber, eFamily Family)
{
	int Size = 0;



	m_RowNumber = RowNumber;
	m_eFamily    = Family;
	m_eType     = Type;
	m_bEmpty    = 1;
	

	if(m_eType == Program)
	{
		if(m_eFamily == dsPIC30F)
		{
			m_RowSize = PM30F_ROW_SIZE;
		}
		else
		{
			m_RowSize = PM33F_ROW_SIZE;
		}
	}
	else
	{
		m_RowSize = EE30F_ROW_SIZE;
	}

	if(m_eType == Program)
	{
		Size = m_RowSize * 4;	// ALEX 3;
		m_Address = StartAddr + RowNumber * m_RowSize * 2;
	}
	if(m_eType == EEProm)
	{
		Size = m_RowSize * 2;
		m_Address = StartAddr + RowNumber * m_RowSize * 2;
	}
	if(m_eType == Configuration)
	{
		Size = 4;	// ALEX 3;
		m_Address = StartAddr + RowNumber * 2;
	}

	m_pBuffer   = (char *)malloc(Size);

	memset(m_Data, 0xFFFF, sizeof(unsigned short)*PM33F_ROW_SIZE*2);	
}
/******************************************************************************/
int mem_cMemRow::InsertData(unsigned int Address, char * pData)
{
	if(Address < m_Address)
	{
		return 0;
	}

	if((m_eType == Program) && (Address >= (m_Address + m_RowSize * 2)))
	{
		return 0;
	}

	if((m_eType == EEProm) && (Address >= (m_Address + m_RowSize * 2)))
	{
		return 0;
	}

	if((m_eType == Configuration) && (Address >= (m_Address + 2)))
	{
		return 0;
	}

	m_bEmpty    = 0;

	sscanf(pData, "%4hx", &(m_Data[Address - m_Address]));
	
	return 1;
}
/******************************************************************************/
int mem_cMemRow::FormatData(void)
{
	if(m_bEmpty == 1)
	{
		return 0;
	}

	if(m_eType == Program)
	{
		for(int Count = 0; Count < m_RowSize; Count += 1)
		{
			m_pBuffer[0 + Count * 4/*ALEX 3*/] = (m_Data[Count * 2]     >> 8) & 0xFF;
			m_pBuffer[1 + Count * 4/*ALEX 3*/] = (m_Data[Count * 2])          & 0xFF;
			m_pBuffer[2 + Count * 4/*ALEX 3*/] = (m_Data[Count * 2 + 1] >> 8) & 0xFF;

			m_pBuffer[3 + Count * 4/*ALEX 3*/] = (m_Data[Count * 2 + 1]) & 0xFF;	// ALEX
		}
	}
	else if(m_eType == Configuration)
	{
		m_pBuffer[0] = (m_Data[0]  >> 8) & 0xFF;
		m_pBuffer[1] = (m_Data[0])       & 0xFF;
		m_pBuffer[2] = (m_Data[1]  >> 8) & 0xFF;

		m_pBuffer[3] = (m_Data[1]) & 0xFF;	// ALEX
	}
	else
	{
		for(int Count = 0; Count < m_RowSize; Count++)
		{
			m_pBuffer[0 + Count * 2] = (m_Data[Count * 2] >> 8) & 0xFF;
			m_pBuffer[1 + Count * 2] = (m_Data[Count * 2])      & 0xFF;
		}
	}
	return 0;
}
/******************************************************************************/
int mem_cMemRow::SendData(void *arg)
{
	int	cnt = 0;
	char	Buffer[4] = {0,0,0,0};

	if((m_bEmpty == 1) && (m_eType != Configuration))
	{
		return 0;
	}

	while(Buffer[0] != COMMAND_ACK)
	{
		if(m_eType == Program)
		{
			Buffer[0] = COMMAND_WRITE_PM;
			Buffer[1] = (m_Address)       & 0xFF;
			Buffer[2] = (m_Address >> 8)  & 0xFF;
			Buffer[3] = (m_Address >> 16) & 0xFF;

			if (WriteCommBlock(arg, Buffer, 4)) return -EIO;
			if (WriteCommBlock(arg, m_pBuffer, m_RowSize * 4)) return -EIO;
		}
		else if(m_eType == EEProm)
		{
			Buffer[0] = COMMAND_WRITE_EE;
			Buffer[1] = (m_Address)       & 0xFF;
			Buffer[2] = (m_Address >> 8)  & 0xFF;
			Buffer[3] = (m_Address >> 16) & 0xFF;

			if (WriteCommBlock(arg, Buffer, 4)) return -EIO;
			if (WriteCommBlock(arg, m_pBuffer, m_RowSize * 2)) return -EIO;
		}
		else if((m_eType == Configuration) && (m_RowNumber == 0))
		{
			Buffer[0] = COMMAND_WRITE_CM;
			Buffer[1] = (char)(m_bEmpty)& 0xFF;
			Buffer[2] = m_pBuffer[0];
			Buffer[3] = m_pBuffer[1];

			if (WriteCommBlock(arg, Buffer, 4)) return -EIO;
			
		}
		else if((m_eType == Configuration) && (m_RowNumber != 0))
		{
			if((m_eFamily == dsPIC30F) && (m_RowNumber == 7))
			{
				return 0;
			}
			Buffer[0] = (char)(m_bEmpty)& 0xFF;
			Buffer[1] = m_pBuffer[0];
			Buffer[2] = m_pBuffer[1];

			if (WriteCommBlock(arg, Buffer, 3)) return -EIO;
		}
		else
		{
			return -EINVAL;
		}

		if (ReceiveData(arg, Buffer, 1)) return -EIO;
		if (++cnt > 1000){
			return -EBUSY;
		}
	}

	return 0;
}
