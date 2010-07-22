/*////////////////////////////////////////////////////////////////////////	
//  Threaded Curl Query                                                 //	
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

#include "CThreadedCurl.h"

CThreadedCurl::CThreadedCurl(lua_State *Ls) :
	L(Ls),
	m_Thread(NULL),
	m_bRunning(true),
	m_Callback(-1),
	m_bActive(false),
	m_iRefCount(0)
{
	m_pCurl = curl_easy_init();

#ifdef WIN32
	m_Thread = CreateThread(NULL, NULL, &ThreadProc, this, NULL, NULL);
	m_hClose = CreateEvent(NULL, true, false, NULL);
#else
	pthread_create(&m_Thread, NULL, &ThreadProc, this);
#endif

	g_Curl[L].push_back(this);
}

CThreadedCurl::~CThreadedCurl(void)
{
	std::vector<CThreadedCurl *> *curlList = &g_Curl[L];

	if(curlList)
	{
		std::vector<CThreadedCurl *>::iterator itor = curlList->begin();
		while(itor != curlList->end())
		{
			if((*itor) == this)
			{
				curlList->erase(itor);
				break;
			}

			itor++;
		}
	}

	m_bRunning = false;

	if(m_Thread)
	{
#ifdef WIN32
		WaitForSingleObject(m_hClose, INFINITE);
		CloseHandle(m_Thread);
#else
		pthread_cancel(m_Thread);
#endif
	}

#ifdef WIN32
	CloseHandle(m_hClose);
#endif

	curl_easy_cleanup(m_pCurl);
	m_pCurl = NULL;

	if(m_Callback != -1)
	{
		Lua()->FreeReference(m_Callback);
		m_Callback = -1;
	}
}

static size_t IncomingData(void *buffer, size_t size, size_t nmemb, void *callResult)
{
	CurlyCallResult *result = (CurlyCallResult *)callResult;

	char *charBuffer = (char *)malloc(size * nmemb + 1);

	memcpy(charBuffer, buffer, size * nmemb);
	charBuffer[size * nmemb] = 0x00;

	result->data += charBuffer;

	free(charBuffer);
	
	return size * nmemb;
}

#ifdef WIN32
DWORD WINAPI CThreadedCurl::ThreadProc(__in LPVOID lpParameter)
#else
void *CThreadedCurl::ThreadProc(void *lpParameter)
#endif
{
	CThreadedCurl *curly = static_cast<CThreadedCurl *>(lpParameter);

	while(curly->m_bRunning)
	{
		if(curly->m_bActive)
		{
			CurlyCallResult *result = new CurlyCallResult();
			result->data = "";

			// Perform the transaction.
			curly->m_Curl_LOCK.Lock();
				curl_easy_setopt(curly->m_pCurl, CURLOPT_WRITEFUNCTION, IncomingData);
				curl_easy_setopt(curly->m_pCurl, CURLOPT_WRITEDATA, result);
				result->code = curl_easy_perform(curly->m_pCurl);
			curly->m_Curl_LOCK.Unlock();

			// Push the result.
			curly->m_outResults_LOCK.Lock();
				curly->m_outResults.push(result);
			curly->m_outResults_LOCK.Unlock();

			// We have finished.
			curly->m_bActive = false;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}

	curly->m_bActive = false;

#ifdef WIN32
	SetEvent(curly->m_hClose);
#endif

	return NULL;
}