/*////////////////////////////////////////////////////////////////////////	
//  Binary Reading Object                                               //	
//                                                                      //	
//  Copyright (c) 2010 Harry Pidcock                                    //	
//                                                                      //	
//  Permission is hereby granted, free of charge, to any person         //	
//  obtaining a copy of this software and associated documentation      //	
//  files (the "Software"), to deal in the Software without             //	
//  restriction, including without limitation the rights to use,        //	
//  copy, modify, merge, publish, distribute, sublicense, and/or sell   //	
//  copies of the Software, and to permit persons to whom the           //	
//  Software is furnished to do so, subject to the following            //	
//  conditions:                                                         //	
//                                                                      //	
//  The above copyright notice and this permission notice shall be      //	
//  included in all copies or substantial portions of the Software.     //	
//                                                                      //	
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,     //	
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES     //	
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND            //	
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT         //	
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,        //	
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING        //	
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR       //	
//  OTHER DEALINGS IN THE SOFTWARE.                                     //	
////////////////////////////////////////////////////////////////////////*/	

#define MT_BINREAD "BinRead"
#define TYPE_BINREAD 9756

#include "GMLuaModule.h"

#include <string.h>

#ifndef __CBINREAD_H__
#define __CBINREAD_H__

class CBinRead
{
public:
	CBinRead(lua_State *Ls) :
		L(Ls),
		m_pData(NULL),
		m_iDataSize(0),
		m_iReadPosition(0)
	{
	};

	~CBinRead(void)
	{
		if(m_pData)
		{
			free(m_pData);
			m_pData = NULL;
			m_iDataSize = 0;
		}
	};

	void SetData(const unsigned char *data, size_t size)
	{
		if(m_pData)
			free(m_pData);

		m_iDataSize = size;
		m_pData = (unsigned char *)malloc(m_iDataSize + sizeof(__int64));

		memcpy(m_pData, data, m_iDataSize);
		memset(&m_pData[m_iDataSize], NULL, sizeof(__int64)); // This is to prevent C-String functions overflowing. And from over reading.
	};

	const unsigned char *GetData(size_t &o_Size)
	{
		o_Size = m_iDataSize;
		return m_pData;
	};

	double ReadDouble(void)
	{
		if(m_pData == NULL || m_iReadPosition >= m_iDataSize)
			return 0;

		double ret = *(double *)&m_pData[m_iReadPosition];
		m_iReadPosition += sizeof(double);
		return ret;
	};

	int ReadInt(void)
	{
		if(m_pData == NULL || m_iReadPosition >= m_iDataSize)
			return 0;

		int ret = *(int *)&m_pData[m_iReadPosition];
		m_iReadPosition += sizeof(int);
		return ret;
	};

	int ReadFloat(void)
	{
		if(m_pData == NULL || m_iReadPosition >= m_iDataSize)
			return 0;

		float ret = *(float *)&m_pData[m_iReadPosition];
		m_iReadPosition += sizeof(float);
		return ret;
	};

	unsigned char ReadByte(void)
	{
		if(m_pData == NULL || m_iReadPosition >= m_iDataSize)
			return 0;

		unsigned char ret = *(unsigned char *)&m_pData[m_iReadPosition];
		m_iReadPosition += sizeof(unsigned char);
		return ret;
	};

	unsigned char PeekByte(void)
	{
		if(m_pData == NULL || m_iReadPosition >= m_iDataSize)
			return 0;

		return *(unsigned char *)&m_pData[m_iReadPosition];
	};

	void Rewind(void)
	{
		m_iReadPosition = 0;
	};

	size_t GetSize(void)
	{
		return m_iDataSize;
	};

	size_t GetReadPosition(void)
	{
		return m_iReadPosition;
	};

private:
	lua_State *L;

	unsigned char *m_pData;
	size_t m_iDataSize;

	size_t m_iReadPosition;
};

#endif // __CBINREAD_H__