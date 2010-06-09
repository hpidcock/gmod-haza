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

#include "GMLuaModule.h"
#include "AutoUnRef.h"

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

#include <string>
#include <queue>

#ifdef WIN32
	#pragma once
#endif

#ifdef WIN32
	typedef int socklen_t;
#endif

#ifndef __THREADEDSOCKET_H__
#define __THREADEDSOCKET_H__

class CThreadedSocket;
extern std::vector<CThreadedSocket *> sockets;

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
		std::string data;
		std::string peer;
	};

	struct SockCall
	{
		unsigned int callId;
		CALL call;
		int integer;
		std::string string;
		std::string peer;
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
		return m_iRefCount <= 0 && m_inCalls.size() == 0 && m_inCallsRecv.size() == 0 && m_outResults.size() == 0;
	};

	CThreadedSocket(int protocol)
	{
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
#endif

		sockets.push_back(this);
	};

	CThreadedSocket(int sock, bool x)
	{	
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
#endif

		sockets.push_back(this);
	};

	~CThreadedSocket(void)
	{
		std::vector<CThreadedSocket *>::iterator itor = sockets.begin();
		while(itor != sockets.end())
		{
			if((*itor) == this)
			{
				sockets.erase(itor);
				break;
			}

			itor++;
		}

		if(m_Callback != -1)
		{
			g_Lua->FreeReference(m_Callback);
			m_Callback = -1;
		}

		m_bRunning = false;
#ifdef WIN32
		WaitForSingleObject(m_hClose, INFINITE);
		CloseHandle(m_hClose);
		CloseHandle(m_Thread);
#else
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

	int Send(std::string data)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::SEND;
		call->string = data;

		call->peer = "";
		call->integer = 0;

		PushCall(call);

		return m_iCallCounter;
	};

	int Send(std::string data, std::string peer, int peerPort)
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

		call->string = "";
		call->peer = "";

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
		call->string = "";
		call->peer = "";

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
		call->string = "";
		call->peer = "";

		PushCallRecv(call);

		return m_iCallCounter;
	};

	int Bind(int port, std::string ip = "")
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::BIND;
		call->integer = port;
		call->string = ip;
		
		call->peer = "";

		PushCall(call);

		return m_iCallCounter;
	};

	int Connect(std::string ip, int port)
	{
		m_iCallCounter++;

		SOCK_CALL::SockCall *call = new SOCK_CALL::SockCall();
		call->callId = m_iCallCounter;
		call->call = SOCK_CALL::CONNECT;
		call->integer = port;
		call->string = ip;
		
		call->peer = "";

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
		call->string = "";
		call->peer = "";

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

		call->string = "";
		call->peer = "";

		PushCall(call);

		return m_iCallCounter;
	};

	void InvokeCallbacks(void)
	{
		SOCK_CALL::SockCallResult *result = NULL;
		while((result = PopResult()) != NULL)
		{
			if(m_Callback != -1)
			{
				AutoUnRef meta = g_Lua->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
				this->Ref();

				g_Lua->PushReference(m_Callback);
				g_Lua->PushUserData(meta, static_cast<void *>(this));
				g_Lua->Push((float)result->call);
				g_Lua->Push((float)result->callId);
				g_Lua->Push((float)result->error);
				if(result->call == SOCK_CALL::ACCEPT)
				{
					if(result->error == SOCK_ERROR::OK)
					{
						CThreadedSocket *newSock = new CThreadedSocket(result->secondary, true);
						newSock->Ref();
						g_Lua->PushUserData(meta, static_cast<void *>(newSock));
					}
					else
						g_Lua->Push(false);
				}
				else
				{
					g_Lua->Push(result->data.c_str());
				}
				g_Lua->Push(result->peer.c_str());
				g_Lua->Call(6, 0);
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

		while(socket->m_bRunning)
		{
			socket->m_inCalls_LOCK.Lock();
			SOCK_CALL::SockCall *call = NULL;

			if(alternatingBuffer)
			{
				if(socket->m_inCalls.size() > 0)
					call = socket->m_inCalls.front();
			}
			else
			{
				if(socket->m_inCallsRecv.size() > 0)
					call = socket->m_inCallsRecv.front();
			}

			socket->m_inCalls_LOCK.Unlock();

			if(call)
			{
				switch(call->call)
				{
				case SOCK_CALL::CONNECT:
					{
						socket->m_Addr.sin_port = htons(call->integer);
						socket->m_Addr.sin_addr = *socket->GetHostByName(call->string.c_str());

						memset(socket->m_Addr.sin_zero, NULL, sizeof(socket->m_Addr.sin_zero));
		    
						SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
						result->call = call->call;
						result->callId = call->callId;

						result->error = connect(socket->m_iSocket, (struct sockaddr *)&socket->m_Addr, sizeof(socket->m_Addr));

						result->error = socket->CheckError(result->error, 0);

						socket->PushResult(result);
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
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

						select(0, &read, 0, 0, &t);

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


						if(result->error >= 0)
							buffer[result->error] = '\0';
						else
							buffer[0] = '\0';

						result->data = buffer;

						free(buffer);

						result->peer = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);
						result->peer += ":";
						char number[16] = {0};
						sprintf(number, "%d", ((sockaddr_in *)&addr)->sin_port);
						result->peer += number;

						result->error = socket->CheckError(result->error, 0);

						socket->PushResult(result);
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
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

						select(0, &read, 0, 0, &t);

						if(!(FD_ISSET(socket->m_iSocket, &read)))
							break;

						SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
						result->call = call->call;
						result->callId = call->callId;

						char buffer = 0;

						result->data = "";

						sockaddr addr;
						socklen_t addrSz = sizeof(sockaddr);

						while((result->error = recvfrom(socket->m_iSocket, &buffer, 1, 0, &addr, &addrSz)) == 1)
						{
							if(buffer == '\n')
								break;

							result->data.append(1, buffer);

							addrSz = sizeof(sockaddr);
						}

						result->peer = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);
						result->peer += ":";
						char number[16] = {0};
						sprintf(number, "%d", ((sockaddr_in *)&addr)->sin_port);
						result->peer += number;

						result->error = socket->CheckError(result->error, 0);

						socket->PushResult(result);
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
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

						select(0, &read, 0, 0, &t);

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

						if(result->error >= 0)
							buffer[result->error] = '\0';
						else
							buffer[0] = '\0';

						result->data = buffer;

						free(buffer);

						result->peer = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);
						result->peer += ":";
						char number[16] = {0};
						sprintf(number, "%d", ((sockaddr_in *)&addr)->sin_port);
						result->peer += number;

						result->error = socket->CheckError(result->error, 0);

						socket->PushResult(result);
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
					}
					break;
				case SOCK_CALL::SEND:
					{
						SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
						result->call = call->call;
						result->callId = call->callId;

						std::string msg = call->string;
						int total = 0;
						int len = msg.size();
						int bytesleft = len;
						int n = -1;

						struct sockaddr *addr = NULL;

						if(call->peer != "")
						{
							socket->m_Addr.sin_port = htons(call->integer);
							socket->m_Addr.sin_addr = *socket->GetHostByName(call->peer.c_str());

							memset(socket->m_Addr.sin_zero, NULL, sizeof(socket->m_Addr.sin_zero));

							addr = (struct sockaddr *)&socket->m_Addr;
						}

						while(total < len)
						{
							if(addr == NULL)
    							n = send(socket->m_iSocket, msg.c_str() + total, bytesleft, 0);
							else
								n = sendto(socket->m_iSocket, msg.c_str() + total, bytesleft, 0, addr, sizeof(sockaddr_in));
							if(n <= 0)
								break;
							total += n;
							bytesleft -= n;
						}

						result->error = n;

						result->error = socket->CheckError(result->error, 0);

						socket->PushResult(result);
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
					}
					break;
				case SOCK_CALL::BIND:
					{
						socket->m_Addr.sin_port = htons(call->integer);

						if(call->string != "")
							socket->m_Addr.sin_addr = *socket->GetHostByName(call->string.c_str());
						else
							socket->m_Addr.sin_addr.s_addr = INADDR_ANY;

						memset(socket->m_Addr.sin_zero, NULL, sizeof(socket->m_Addr.sin_zero));
		    
						SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
						result->call = call->call;
						result->callId = call->callId;

						result->error = bind(socket->m_iSocket, (struct sockaddr *)&socket->m_Addr, sizeof(socket->m_Addr));

						result->error = socket->CheckError(result->error, 0);

						socket->PushResult(result);
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
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
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
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

						select(0, &read, 0, 0, &t);

						if(!(FD_ISSET(socket->m_iSocket, &read)))
							break;

						struct sockaddr_in their_addr;
						socklen_t size = sizeof(their_addr);
						
						SOCK_CALL::SockCallResult *result = new SOCK_CALL::SockCallResult();
						result->call = call->call;
						result->callId = call->callId;

						result->secondary = accept(socket->m_iSocket, (struct sockaddr*)&their_addr, &size);

						result->error = socket->CheckError(result->secondary, 0);

						socket->PushResult(result);
						
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
					}
					break;
				default:
					{
						socket->m_inCalls_LOCK.Lock();
						if(alternatingBuffer)
							socket->m_inCalls.pop();
						else
							socket->m_inCallsRecv.pop();
						delete call;
						socket->m_inCalls_LOCK.Unlock();
					}
					break;
				}
			}

			alternatingBuffer = !alternatingBuffer;
			
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
		}

		SetEvent(socket->m_hClose);

		return NULL;
	};

private:
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
#endif
};

#endif // __THREADEDSOCKET_H__
