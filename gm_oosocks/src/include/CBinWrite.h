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

#define MT_BINWRITE "BinWrite_OOS"
#define TYPE_BINWRITE 9789

#include "GMLuaModule.h"

#include <string.h>

#ifndef __CBINWRITE_H__
#define __CBINWRITE_H__

class CBinWrite
{
public:
	CBinWrite(lua_State *Ls) :
		L(Ls),
		m_pData(NULL),
		m_iDataSize(0),
		m_iWritePosition(0)
	{
		m_iDataSize = 512;
		m_pData = (unsigned char *)malloc(m_iDataSize);
	};

	~CBinWrite(void)
	{
		if(m_pData)
		{
			free(m_pData);
			m_pData = NULL;
			m_iDataSize = 0;
		}
	};

	const unsigned char *GetData(size_t &o_Size)
	{
		o_Size = m_iWritePosition;
		return m_pData;
	};

	void Write(double val)
	{
		if(m_pData == NULL)
			return;

		FitData(sizeof(double));
		double *setVal = (double *)&m_pData[m_iWritePosition];
		*setVal = val;
		m_iWritePosition += sizeof(double);
	};

	void Write(int val)
	{
		if(m_pData == NULL)
			return;

		FitData(sizeof(int));
		int *setVal = (int *)&m_pData[m_iWritePosition];
		*setVal = val;
		m_iWritePosition += sizeof(int);
	};

	void Write(short val)
	{
		if(m_pData == NULL)
			return;

		FitData(sizeof(short));
		short *setVal = (short *)&m_pData[m_iWritePosition];
		*setVal = val;
		m_iWritePosition += sizeof(short);
	};

	void Write(float val)
	{
		if(m_pData == NULL)
			return;

		FitData(sizeof(float));
		float *setVal = (float *)&m_pData[m_iWritePosition];
		*setVal = val;
		m_iWritePosition += sizeof(float);
	};

	void Write(char val)
	{
		if(m_pData == NULL)
			return;

		FitData(sizeof(char));
		char *setVal = (char *)&m_pData[m_iWritePosition];
		*setVal = val;
		m_iWritePosition += sizeof(char);
	};
	
	void Write(const char *str, size_t len)
	{
		if(m_pData == NULL)
			return;
		
		FitData(len);
		char *setVal = (char *)&m_pData[m_iWritePosition];
		memcpy(setVal, str, len);
		m_iWritePosition += len;
	};

	size_t GetSize(void)
	{
		return m_iWritePosition;
	};

	void FitData(size_t dataLen)
	{
		m_iDataSize = m_iWritePosition + dataLen;
		m_pData = (unsigned char *)realloc(m_pData, m_iDataSize);
	};

private:
	lua_State *L;

	unsigned char *m_pData;
	size_t m_iDataSize;

	size_t m_iWritePosition;
};

#endif // __CBINWRITE_H__
