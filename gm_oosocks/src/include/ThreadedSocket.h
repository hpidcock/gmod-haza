/*////////////////////////////////////////////////////////////////////////	
//  Threaded Lua Socket                                                 //	
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

#define MT_SOCKET "OOSock"
#define TYPE_SOCKET 9753
#define MAX_SOCK_OPS 64

#include "GMLuaModule.h"

#include "CBinRead.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#endif

#include "CString.h"
#include <string>
#include <queue>
#include <map>

#include <assert.h>

#ifdef WIN32
#undef assert
extern "C"
{
	_CRTIMP void __cdecl _wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line);
}

#define assert(_Expression) (void)( (!!(_Expression)) || (_wassert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )
#endif

#ifdef WIN32
#pragma once
#endif

#ifdef WIN32
typedef int socklen_t;
#endif

#ifndef __THREADEDSOCKET_H__
#define __THREADEDSOCKET_H__

class CThreadedSocket;
extern std::map< lua_State *, std::vector<CThreadedSocket *> > g_Socks;

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

namespace SOCK_ERROR
{
	enum
	{
		OK = 0,
		NOT_CONNECTED,
		CONNECTION_RESET,
		TIMED_OUT,
		BAD
	};

	static int Translate(int osSpec)
	{
#ifdef WIN32
		static const int OS[] = {NULL, WSAENOTCONN, WSAECONNRESET, WSAETIMEDOUT};
#else
		static const int OS[] = {NULL, ENOTCONN, ECONNRESET, ETIMEDOUT};
#endif
		static const int TRANS[] = {OK, NOT_CONNECTED, CONNECTION_RESET, TIMED_OUT};

		for(int i = 0; i < sizeof(OS)/sizeof(int); i++)
		{
			if(OS[i] == osSpec)
				return TRANS[i];
		}

		return BAD;
	};
};

namespace SOCK_CALL
{
	enum CALL
	{
		CONNECT = 0,
		REC_SIZE,
		REC_LINE,
		SEND,
		BIND,
		ACCEPT,
		LISTEN,
		REC_DATAGRAM
	};

	struct SockCallResult
	{
		unsigned int callId;
		CALL call;
		int error;
		int secondary;
		int sockout;
		CString data;
		CString peer;
	};

	struct SockCall
	{
		unsigned int callId;
		CALL call;
		int integer;
		CString string;
		CString peer;
	};
};

class CThreadedSocket
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
		// We ignore Recv Calls.
		assert(m_iRefCount >= 0);
		return m_iRefCount == 0 && m_inCalls.size() == 0 && m_outResults.size() == 0;
	};

	CThreadedSocket(lua_State *L, int protocol)
	{
		this->L = L;

		m_bBinaryMode = false;

		m_iRefCount = 0;

		m_Callback = -1;

		m_iCallCounter = 0;

		m_iSocket = socket(AF_INET, (protocol == IPPROTO_TCP) ? SOCK_STREAM : SOCK_DGRAM, protocol);

		char yes = 1;
		setsockopt(m_iSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

		m_Addr.sin_family = AF_INET;

		m_bRunning = true;

#ifdef WIN32
		m_Thread = CreateThread(NULL, NULL, &ThreadProc, this, NULL, NULL);
		m_hClose = CreateEvent(NULL, true, false, NULL);
#else
		pthread_create(&m_Thread, NULL, &ThreadProc, this);
		m_hClose = false;
#endif

		g_Socks[L].push_back(this);
	};

	CThreadedSocket(lua_State *L, int sock, bool x)
	{	
		this->L = L;

		m_bBinaryMode = false;

		m_iRefCount = 0;

		m_Callback = -1;

		m_iCallCounter = 0;

		m_iSocket = sock;

		m_bRunning = true;

#ifdef WIN32
		m_Thread = CreateThread(NULL, NULL, &ThreadProc, this, NULL, NULL);
		m_hClose = CreateEvent(NULL, true, false, NULL);
#else
		pthread_create(&m_Thread, NULL, &ThreadProc, this);
		m_hClose = false;
#endif

		g_Socks[L].push_back(this);
	};

	~CThreadedSocket(void)
	{
		assert(m_iRefCount == 0);
		assert(m_bRunning == true);

		std::vector<CThreadedSocket *> *socketsList = &g_Socks[L];

		std::vector<CThreadedSocket *>::iterator itor = socketsList->begin();
		while(itor != socketsList->end())
		{
			if((*itor) == this)
			{
				socketsList->erase(itor);
				break;
			}

			itor++;
		}

		if(m_Callback != -1)
		{
			Lua()->FreeReference(m_Callback);
			m_Callback = -1;
		}

		m_bRunning = false;
#ifdef WIN32
		assert(WaitForSingleObject(m_hClose, INFINITE) == WAIT_OBJECT_0);
		DWORD exitValue = NULL;
		while(GetExitCodeThread(m_Thread, &exitValue) && exitValue == STILL_ACTIVE)
		{
		}
		assert(CloseHandle(m_hClose));
		assert(CloseHandle(m_Thread));
#else
		while(!m_hClose)
		{
		}
		pthread_cancel(m_Thread);
#endif

		Close();

		SOCK_CALL::SockCall *cleanupCall = NULL;
		while((cleanupCall = PopCall()) != NULL)
		{
			delete cleanupCall;
		}

		cleanupCall = NULL;
		while((cleanupCall = PopCallRecv()) != NULL)
		{
			delete cleanupCall;
		}

		SOCK_CALL::SockCallResult *cleanupResult = NULL;
		while((cleanupResult = PopResult()) != NULL)
		{
			delete cleanupResult;
		}
	};

	int Send(CString data)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::SEND;
		call->string = data;

		call->peer = CString("", 0);
		call->integer = 0;

		PushCall(call);

		return m_iCallCounter;
	};

	int Send(CString data, CString peer, int peerPort)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::SEND;
		call->string = data;
		call->peer = peer;
		call->integer = peerPort;

		PushCall(call);

		return m_iCallCounter;
	};

	int Receive(int len)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::REC_SIZE;
		call->integer = len;

		call->string = CString("", 0);
		call->peer = CString("", 0);

		PushCallRecv(call);

		return m_iCallCounter;
	};

	int ReceiveLine(void)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::REC_LINE;

		call->integer = 0;
		call->string = CString("", 0);
		call->peer = CString("", 0);

		PushCallRecv(call);

		return m_iCallCounter;
	};

	int ReceiveDatagram(void)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::REC_DATAGRAM;

		call->integer = 0;
		call->string = CString("", 0);
		call->peer = CString("", 0);

		PushCallRecv(call);

		return m_iCallCounter;
	};

	int Bind(int port, CString ip)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::BIND;
		call->integer = port;
		call->string = ip;

		call->peer = CString("", 0);

		PushCall(call);

		return m_iCallCounter;
	};

	int Connect(CString ip, int port)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::CONNECT;
		call->integer = port;
		call->string = ip;

		call->peer = CString("", 0);

		PushCall(call);

		return m_iCallCounter;
	};

	int Accept(void)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::ACCEPT;

		call->integer = 0;
		call->string = CString("", 0);
		call->peer = CString("", 0);

		PushCall(call);

		return m_iCallCounter;
	};

	int Listen(int backlog)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::LISTEN;
		call->integer = backlog;

		call->string = CString("", 0);
		call->peer = CString("", 0);

		PushCall(call);

		return m_iCallCounter;
	};

	void SetBinaryMode(bool mode)
	{
		m_bBinaryMode = mode;
	};

	void InvokeCallbacks(void)
	{
		SOCK_CALL::SockCallResult *result = NULL;
		while((result = PopResult()) != NULL)
		{
			if(m_Callback != -1)
			{
				CAutoUnRef meta = Lua()->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
				this->Ref();

				Lua()->PushReference(m_Callback);
				Lua()->PushUserData(meta, static_cast<void *>(this));
				Lua()->Push((float)result->call);
				Lua()->Push((float)result->callId);
				Lua()->Push((float)result->error);
				if(result->call == SOCK_CALL::ACCEPT)
				{
					if(result->error == SOCK_ERROR::OK)
					{
						CThreadedSocket *newSock = new CThreadedSocket(L, result->sockout, true);
						newSock->Ref();
						Lua()->PushUserData(meta, static_cast<void *>(newSock));
					}
					else
					{
						Lua()->PushNil();
					}
					Lua()->Push(result->peer.Str());
					Lua()->Push((float)result->secondary);
				}
				else
				{
					if(m_bBinaryMode)
					{
						CAutoUnRef metaBR = Lua()->GetMetaTable(MT_BINREAD, TYPE_BINREAD);
						CBinRead *br = new CBinRead(L);
						br->SetData((const unsigned char *)result->data.Str(), result->data.Size());
						Lua()->PushUserData(metaBR, static_cast<void *>(br));
					}
					else
					{
						Lua()->Push(result->data.Str());
					}
					Lua()->Push(result->peer.Str());
					Lua()->Push((float)result->secondary);
				}
				Lua()->Call(7, 0);
			}
			delete result;
		}

		if(CanDelete())
			delete this;
	};

	void SetCallback(int i)
	{
		m_Callback = i;
	};

	void Close(void)
	{
#ifdef WIN32
		closesocket(m_iSocket);		
#else
		close(m_iSocket);
#endif
	};

protected:
	void PushCall(SOCK_CALL::SockCall *call)
	{
		m_inCalls_LOCK.Lock();
		m_inCalls.push(call);
		m_inCalls_LOCK.Unlock();
	};

	void PushCallRecv(SOCK_CALL::SockCall *call)
	{
		m_inCalls_LOCK.Lock();
		m_inCallsRecv.push(call);
		m_inCalls_LOCK.Unlock();
	};

	void PushResult(SOCK_CALL::SockCallResult *result)
	{
		m_outResults_LOCK.Lock();
		m_outResults.push(result);
		m_outResults_LOCK.Unlock();
	};

	SOCK_CALL::SockCallResult *PopResult(void)
	{
		SOCK_CALL::SockCallResult *result = NULL;

		m_outResults_LOCK.Lock();

		if(m_outResults.size() > 0)
		{
			result = m_outResults.front();
			m_outResults.pop();
		}

		m_outResults_LOCK.Unlock();

		return result;
	};

	SOCK_CALL::SockCall *PopCall(void)
	{
		SOCK_CALL::SockCall *result = NULL;

		m_inCalls_LOCK.Lock();

		if(m_inCalls.size() > 0)
		{
			result = m_inCalls.front();
			m_inCalls.pop();
		}

		m_inCalls_LOCK.Unlock();

		return result;
	};

	SOCK_CALL::SockCall *PopCallRecv(void)
	{
		SOCK_CALL::SockCall *result = NULL;

		m_inCalls_LOCK.Lock();

		if(m_inCallsRecv.size() > 0)
		{
			result = m_inCallsRecv.front();
			m_inCallsRecv.pop();
		}

		m_inCalls_LOCK.Unlock();

		return result;
	};

	struct in_addr* GetHostByName(const char* server)
	{
		struct hostent *h = gethostbyname(server);

		if(h == NULL)
			return NULL;

		return (struct in_addr*)h->h_addr;	
	}

	int CheckError(int returnCode, int ok)
	{
		if(returnCode >= ok)
			return SOCK_ERROR::OK;

#ifdef WIN32
		int error = WSAGetLastError();
#else
		int error = errno;
#endif

		return SOCK_ERROR::Translate(error);
	}


#ifdef WIN32
	static DWORD WINAPI ThreadProc(__in LPVOID lpParameter)
#else
	static void *ThreadProc(void *lpParameter)
#endif
	{
		CThreadedSocket *socket = static_cast<CThreadedSocket *>(lpParameter);
		if(socket == NULL)
			return NULL;

		bool alternatingBuffer = false;

		std::queue<SOCK_CALL::SockCall *> queueCalls;
		std::queue<SOCK_CALL::SockCall *> queueRecvCalls;

		while(socket->m_bRunning)
		{
			for(int ittor = 0; ittor < MAX_SOCK_OPS; ittor++)
			{
				// Copy over calls.
				while(socket->m_inCalls.size() > 0)
				{
					queueCalls.push(socket->PopCall());
				}

				while(socket->m_inCallsRecv.size() > 0)
				{
					queueRecvCalls.push(socket->PopCallRecv());
				}

				std::queue<SOCK_CALL::SockCall *> *callBuffer = NULL;
				if(alternatingBuffer)
				{
					callBuffer = &queueCalls;
				}
				else
				{
					callBuffer = &queueRecvCalls;
				}

				SOCK_CALL::SockCall *call = NULL;
				if(callBuffer->size() > 0)
				{
					call = callBuffer->front();
				}

				if(call)
				{
					switch(call->call)
					{
					case SOCK_CALL::CONNECT:
						{
							socket->m_Addr.sin_port = htons(call->integer);
							socket->m_Addr.sin_addr = *socket->GetHostByName(call->string.Str());

							memset(socket->m_Addr.sin_zero, NULL, sizeof(socket->m_Addr.sin_zero));

							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							result->error = connect(socket->m_iSocket, (struct sockaddr *)&socket->m_Addr, sizeof(socket->m_Addr));

							result->error = socket->CheckError(result->error, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					case SOCK_CALL::REC_SIZE:
						{
							fd_set read;
							memset(&read, NULL, sizeof(fd_set));
							FD_SET(socket->m_iSocket, &read);

							timeval t;
							t.tv_sec = 0;
							t.tv_usec = 0;

							select(socket->m_iSocket + 1, &read, 0, 0, &t);

							if(!(FD_ISSET(socket->m_iSocket, &read)))
								break;

							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							int count = 0;
							char *buffer = (char *)malloc(call->integer + 1);
							sockaddr addr;
							socklen_t addrSz = sizeof(sockaddr);

							result->error = recvfrom(socket->m_iSocket, buffer, call->integer, 0, &addr, &addrSz);

							size_t length = result->error;

							if(result->error < 0)
							{
								length = 0;
							}

							result->data = CString(buffer, length);

							free(buffer);

							char *peer = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);
							result->peer = CString(peer, strlen(peer));
							result->secondary = ntohs(((sockaddr_in *)&addr)->sin_port);

							result->error = socket->CheckError(result->error, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					case SOCK_CALL::REC_LINE:
						{
							fd_set read;
							memset(&read, NULL, sizeof(fd_set));
							FD_SET(socket->m_iSocket, &read);

							timeval t;
							t.tv_sec = 0;
							t.tv_usec = 0;

							select(socket->m_iSocket + 1, &read, 0, 0, &t);

							if(!(FD_ISSET(socket->m_iSocket, &read)))
								break;

							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							char buffer = 0;
							std::string fullBuffer = "";

							sockaddr addr;
							socklen_t addrSz = sizeof(sockaddr);

							while((result->error = recvfrom(socket->m_iSocket, &buffer, 1, 0, &addr, &addrSz)) == 1)
							{
								if(buffer == '\n')
									break;

								fullBuffer.append(1, buffer);

								addrSz = sizeof(sockaddr);
							}

							result->data = CString(fullBuffer.c_str(), fullBuffer.size());

							char *peer = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);
							result->peer = CString(peer, strlen(peer));
							result->secondary = ntohs(((sockaddr_in *)&addr)->sin_port);

							result->error = socket->CheckError(result->error, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					case SOCK_CALL::REC_DATAGRAM:
						{
							fd_set read;
							memset(&read, NULL, sizeof(fd_set));
							FD_SET(socket->m_iSocket, &read);

							timeval t;
							t.tv_sec = 0;
							t.tv_usec = 0;

							select(socket->m_iSocket + 1, &read, 0, 0, &t);

							if(!(FD_ISSET(socket->m_iSocket, &read)))
								break;

							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							int count = 0;
							char *buffer = (char *)malloc(1500);
							sockaddr addr;
							socklen_t addrSz = sizeof(sockaddr);

							result->error = recvfrom(socket->m_iSocket, buffer, 1500, 0, &addr, &addrSz);

							if(result->error == 0)
							{
								// No Data
								free(buffer);
								break;
							}

							size_t length = result->error;

							if(result->error < 0)
							{
								length = 0;
							}

							result->data = CString(buffer, length);

							free(buffer);

							char *peer = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);
							result->peer = CString(peer, strlen(peer));
							result->secondary = ntohs(((sockaddr_in *)&addr)->sin_port);

							result->error = socket->CheckError(result->error, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					case SOCK_CALL::SEND:
						{
							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							CString msg = call->string;
							int total = 0;
							int len = msg.Size();
							int bytesleft = len;
							int n = -1;

							struct sockaddr *addr = NULL;

							if(call->peer.Size() != 0)
							{
								socket->m_Addr.sin_port = htons(call->integer);
								socket->m_Addr.sin_addr = *socket->GetHostByName(call->peer.Str());

								memset(socket->m_Addr.sin_zero, NULL, sizeof(socket->m_Addr.sin_zero));

								addr = (struct sockaddr *)&socket->m_Addr;
							}

							while(total < len)
							{
								if(addr == NULL)
									n = send(socket->m_iSocket, msg.Str() + total, bytesleft, 0);
								else
									n = sendto(socket->m_iSocket, msg.Str() + total, bytesleft, 0, addr, sizeof(sockaddr_in));
								if(n <= 0)
									break;
								total += n;
								bytesleft -= n;
							}

							result->error = n;

							result->error = socket->CheckError(result->error, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					case SOCK_CALL::BIND:
						{
							socket->m_Addr.sin_port = htons(call->integer);

							if(call->string.Size() != 0)
								socket->m_Addr.sin_addr = *socket->GetHostByName(call->string.Str());
							else
								socket->m_Addr.sin_addr.s_addr = INADDR_ANY;

							memset(socket->m_Addr.sin_zero, NULL, sizeof(socket->m_Addr.sin_zero));

							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							result->error = bind(socket->m_iSocket, (struct sockaddr *)&socket->m_Addr, sizeof(socket->m_Addr));

							result->error = socket->CheckError(result->error, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					case SOCK_CALL::LISTEN:
						{
							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							result->error = listen(socket->m_iSocket, call->integer);

							result->error = socket->CheckError(result->error, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					case SOCK_CALL::ACCEPT:
						{
							fd_set read;
							memset(&read, NULL, sizeof(fd_set));
							FD_SET(socket->m_iSocket, &read);

							timeval t;
							t.tv_sec = 0;
							t.tv_usec = 0;

							select(socket->m_iSocket + 1, &read, 0, 0, &t);

							if(!(FD_ISSET(socket->m_iSocket, &read)))
								break;

							struct sockaddr_in their_addr;
							socklen_t size = sizeof(their_addr);

							SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
							result->call = call->call;
							result->callId = call->callId;

							result->sockout = accept(socket->m_iSocket, (struct sockaddr*)&their_addr, &size);

							char *peer = inet_ntoa(((sockaddr_in *)&their_addr)->sin_addr);
							result->peer = CString(peer, strlen(peer));
							result->secondary = ntohs(((sockaddr_in *)&their_addr)->sin_port);

							result->error = socket->CheckError(result->sockout, 0);

							socket->PushResult(result);

							callBuffer->pop();
							delete call;
						}
						break;
					default:
						{
							callBuffer->pop();
							delete call;
						}
						break;
					}
				}

				alternatingBuffer = !alternatingBuffer;
			}
#ifdef WIN32
			Sleep(1);
#else
			usleep(100);
#endif
		}

#ifdef WIN32
		SetEvent(socket->m_hClose);
#else
		socket->m_hClose = true;
#endif

		return NULL;
	};

private:
	lua_State *L;

	bool m_bRunning;

	int m_iSocket;
	struct sockaddr_in m_Addr;
	unsigned int m_iCallCounter;

	std::queue<SOCK_CALL::SockCall *> m_inCalls;
	std::queue<SOCK_CALL::SockCall *> m_inCallsRecv;
	CMutexLock m_inCalls_LOCK;

	std::queue<SOCK_CALL::SockCallResult *> m_outResults;
	CMutexLock m_outResults_LOCK;

	int m_Callback;

#ifdef WIN32
	HANDLE m_Thread;
	HANDLE m_hClose;
#else
	pthread_t m_Thread;
	bool m_hClose;
#endif

	bool m_bBinaryMode;
};

#endif // __THREADEDSOCKET_H__
