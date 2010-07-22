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

#define MT_CURLY "Curly"
#define TYPE_CURLY 9754

#include "GMLuaModule.h"

#include "curl/curl.h"

#ifdef WIN32
	#include <Windows.h>
#else
	#include <pthread.h>
#endif

#include <string>
#include <queue>
#include <vector>
#include <map>

#ifdef WIN32
	#pragma once
#endif

#ifndef __CTHREADEDCURL_H__
#define __CTHREADEDCURL_H__

class CThreadedCurl;
extern std::map<lua_State *, std::vector<CThreadedCurl *>> g_Curl;

class CMutexLock
{
public:
	CMutexLock()
	{
#ifdef WIN32
		InitializeCriticalSection(&m_Lock);
#else
		m_Lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	};

	void Lock(void)
	{
#ifdef WIN32
		EnterCriticalSection(&m_Lock);
#else
		pthread_mutex_lock(&m_Lock);
#endif
	};

	void Unlock(void)
	{
#ifdef WIN32
		LeaveCriticalSection(&m_Lock);
#else
		pthread_mutex_unlock(&m_Lock);
#endif
	};

private:
	CMutexLock(const CMutexLock &other) { };
	
#ifdef WIN32
	CRITICAL_SECTION m_Lock;
#else
	pthread_mutex_t m_Lock;
#endif
};

#define CURLE_THREAD_ACTIVE CURLE_OBSOLETE4

struct CurlyCallResult
{
	CURLcode code;
	std::string data;
};

namespace CurlyOption
{
	
}

class CThreadedCurl
{
public:
	int m_iRefCount;

	void Ref()
	{
		m_iRefCount++;
	};

	void UnRef()
	{
		m_iRefCount--;

		if(!CanDelete())
			return;

		delete this;
	};

	bool CanDelete()
	{
		return m_iRefCount <= 0 && !m_bActive && m_outResults.size() == 0;
	};

	////////////////////////
	// CThreadedCurl      //
	////////////////////////
	CThreadedCurl(lua_State *L);
	~CThreadedCurl(void);

	////////////////////////
	// Exported Functions //
	////////////////////////
	void SetCallback(int i)
	{
		if(m_Callback != -1)
			Lua()->FreeReference(m_Callback);

		m_Callback = i;
	};

	int Perform(void)
	{
		if(m_bActive)
			return CURLE_THREAD_ACTIVE;

		m_bActive = true;

		return CURLE_OK;
	};

	int SetUrl(std::string url)
	{
		if(m_bActive)
			return CURLE_THREAD_ACTIVE;

		m_Curl_LOCK.Lock();
			CURLcode result = curl_easy_setopt(m_pCurl, CURLOPT_URL, url.c_str());
		m_Curl_LOCK.Unlock();

		return result;
	};

	int SetOptNumber(int option, int value)
	{
		if(m_bActive)
			return CURLE_THREAD_ACTIVE;

		if(option >= CURLOPTTYPE_OBJECTPOINT)
			return CURLE_BAD_FUNCTION_ARGUMENT;

		m_Curl_LOCK.Lock();
			CURLcode result = curl_easy_setopt(m_pCurl, (CURLoption) option, value);
		m_Curl_LOCK.Unlock();

		return result;
	};

	int SetUsername(std::string username)
	{
		if(m_bActive)
			return CURLE_THREAD_ACTIVE;

		m_Curl_LOCK.Lock();
			CURLcode result = curl_easy_setopt(m_pCurl, CURLOPT_USERNAME, username.c_str());
		m_Curl_LOCK.Unlock();

		return result;
	};

	int SetPassword(std::string username)
	{
		if(m_bActive)
			return CURLE_THREAD_ACTIVE;

		m_Curl_LOCK.Lock();
			CURLcode result = curl_easy_setopt(m_pCurl, CURLOPT_PASSWORD, username.c_str());
		m_Curl_LOCK.Unlock();

		return result;
	};

	int SetCookies(std::string cookies)
	{
		if(m_bActive)
			return CURLE_THREAD_ACTIVE;

		m_Curl_LOCK.Lock();
			CURLcode result = curl_easy_setopt(m_pCurl, CURLOPT_COOKIE, cookies.c_str());
		m_Curl_LOCK.Unlock();

		return result;
	};

	int SetPostFields(const char *postFields, int size)
	{
		if(m_bActive)
			return CURLE_THREAD_ACTIVE;

		m_Curl_LOCK.Lock();
			CURLcode result = curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, size);
			if(result == CURLE_OK)
			{
				result = curl_easy_setopt(m_pCurl, CURLOPT_COPYPOSTFIELDS, postFields);
			}
		m_Curl_LOCK.Unlock();

		return result;
	};

	void InvokeCallbacks(void)
	{
		CurlyCallResult *result = NULL;
		while((result = PopResult()) != NULL)
		{
			if(m_Callback != -1)
			{
				CAutoUnRef meta = Lua()->GetMetaTable(MT_CURLY, TYPE_CURLY);
				this->Ref();

				Lua()->PushReference(m_Callback);
				Lua()->PushUserData(meta, static_cast<void *>(this));
				Lua()->PushDouble(result->code);
				Lua()->Push(result->data.c_str());
				Lua()->Call(3, 0);
			}

			delete result;
		}

		if(CanDelete())
			delete this;
	};

private:
	CurlyCallResult *PopResult(void)
	{
		CurlyCallResult *result = NULL;
		
		m_outResults_LOCK.Lock();

		if(m_outResults.size() > 0)
		{
			result = m_outResults.front();
			m_outResults.pop();
		}

		m_outResults_LOCK.Unlock();

		return result;
	};

#ifdef WIN32
	static DWORD WINAPI ThreadProc(__in LPVOID lpParameter);
#else
	static void *ThreadProc(void *lpParameter);
#endif

	CThreadedCurl(const CThreadedCurl &other);
	CThreadedCurl(void);

	lua_State *L;

	int m_Callback;

#ifdef WIN32
	HANDLE m_Thread;
	HANDLE m_hClose;
#else
	pthread_t m_Thread;
#endif

	bool m_bRunning;
	bool m_bActive;

	CURL *m_pCurl;
	CMutexLock m_Curl_LOCK;

	std::queue<CurlyCallResult *> m_outResults;
	CMutexLock m_outResults_LOCK;
};

#endif // __CTHREADEDCURL_H__