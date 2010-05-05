#include "GMLuaModule.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

#include "AutoUnRef.h"

GMOD_MODULE(Init, Shutdown);

struct TcpWorkerData
{
	char **data;
	int datacount;

	SOCKET *socks;
	int sockcount;
	char **dataSocketSpecific;

	int timeout;
};

void _TCP_SEND(SOCKET sock, const char* data, int timeout)
{
	__try
	{
		if(data == NULL)
			return;

		if(sock == INVALID_SOCKET || sock == NULL)
			return;

		send(sock, data, strlen(data), 0);

		WSAPOLLFD poll;
		poll.fd = sock;
		poll.events = POLLWRNORM;

		WSAPoll(&poll, 1, timeout);
	}
	__finally
	{
		return;
	}
}

DWORD WINAPI _TCP_SEND_WORKER(LPVOID param)
{
	TcpWorkerData *d = (TcpWorkerData *)param;

	if(d == NULL)
		return 0;

	int timeout = d->timeout;

	for(int i = 0; i < d->datacount; i++)
	{
		if(d->data[i] == NULL)
			continue;

		const char *data = d->data[i];

		for(int q = 0; q < d->sockcount; q++)
		{
			if(d->dataSocketSpecific[q] != NULL)
			{
				int dataLen = strlen(data);
				int dataLenSS = strlen(d->dataSocketSpecific[q]);

				char *newData = (char *)malloc(dataLen + dataLenSS + 1);

				memcpy(newData, data, dataLen);
				memcpy(newData + dataLen, d->dataSocketSpecific[q], dataLenSS);
				newData[dataLen + dataLenSS] = 0;

				_TCP_SEND(d->socks[q], newData, timeout);

				free(newData);
				free(d->dataSocketSpecific[q]);
				d->dataSocketSpecific[q] = NULL;
			}
			else
			{
				_TCP_SEND(d->socks[q], data, timeout);
			}
		}

		free(d->data[i]);
	}

	free(d->socks);
	free(d->dataSocketSpecific);
	free(d->data);
	free(d);

	return 0;
}

// tcpsend(table of datastrings, table of sockets, table of datastrings that are sent once unique to a socket index, timeout of send)
int tcpsend(lua_State* L)
{
	TcpWorkerData *newData = (TcpWorkerData *)malloc(sizeof(TcpWorkerData));
	memset(newData, 0, sizeof(TcpWorkerData));

	AutoUnRef data = g_Lua->GetObject(1);

	data->Push();
	AutoUnRef table = g_Lua->GetGlobal("table");
	AutoUnRef tableCount = table->GetMember("Count");
	tableCount->Push();
	g_Lua->Call(1, 1);

	int datacount = g_Lua->GetNumber();
	g_Lua->Pop();
		
	newData->data = (char **)malloc(datacount * sizeof(char *));
	newData->datacount = datacount;

	for(int i = 0; i < datacount; i++)
	{
		const char *str = data->GetMemberStr((float)i);

		if(str == NULL)
			continue;

		int strl = strlen(str);
		newData->data[i] = (char *)malloc(strl + 1);
		memcpy(newData->data[i], str, strl);
		newData->data[i][strl] = 0;
	}

	AutoUnRef socks = g_Lua->GetObject(2);
	
	socks->Push();
	tableCount->Push();
	g_Lua->Call(1, 1);

	int sockcount = g_Lua->GetNumber();

	newData->socks = (SOCKET *)malloc(sockcount * sizeof(SOCKET));
	newData->dataSocketSpecific = (char **)malloc(sockcount * sizeof(char *));
	newData->sockcount = sockcount;

	AutoUnRef socksData = g_Lua->GetObject(3);

	for(int i = 0; i < sockcount; i++)
	{
		// Socket
		AutoUnRef sockL = socks->GetMember((float)i);
		SOCKET sock = sockL->GetInt();

		newData->socks[i] = sock;

		// Socket Specific Data
		const char *str = socksData->GetMemberStr((float)i + 1);

		if(str == NULL)
		{
			newData->dataSocketSpecific[i] = NULL;
			continue;
		}

		int strl = strlen(str);
		newData->dataSocketSpecific[i] = (char *)malloc(strl + 1);
		memcpy(newData->dataSocketSpecific[i], str, strl);
		newData->dataSocketSpecific[i][strl] = 0;
	}

	newData->timeout = g_Lua->GetInteger(4);

	QueueUserWorkItem(_TCP_SEND_WORKER, newData, WT_EXECUTELONGFUNCTION);

	return 0;
}

int tcplisten(lua_State* L)
{
	int ret = 0;
	__try
	{
		const char *ip = g_Lua->GetString(1);
		int port = g_Lua->GetInteger(2);
		int backlog = g_Lua->GetInteger(3);

		if(ip == NULL)
			return 0;

		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if(sock == INVALID_SOCKET)
		{
			g_Lua->Push((float)sock);
			ret = 1;
			return 0;
		}

		sockaddr_in service;
		service.sin_family = AF_INET;
		service.sin_addr.s_addr = inet_addr(ip);
		service.sin_port = htons(port);

		if(bind(sock, (sockaddr *) &service, sizeof(service)) != 0)
		{
			closesocket(sock);
			return 0;
		}

		if(listen(sock, backlog) != 0)
		{
			closesocket(sock);
			return 0;
		}

		u_long argp = 1;
		ioctlsocket(sock, FIONBIO, &argp);

		g_Lua->Push((float)sock);

		ret = 1;
	}
	__finally
	{
		return ret;
	}
}

int tcpaccept(lua_State* L)
{
	int ret = 0;
	__try
	{
		SOCKET sock = g_Lua->GetInteger(1);

		if(sock == INVALID_SOCKET)
			return 0;

		int client = accept(sock, NULL, NULL);
		client = client == 0 ? -1 : client;

		if(client != INVALID_SOCKET)
		{
			u_long argp = 1;
			ioctlsocket(client, FIONBIO, &argp);
		}

		g_Lua->Push((float)client);

		ret = 1;
	}
	__finally
	{
		return ret;
	}
}

int tcprecv(lua_State* L)
{	
	SOCKET sock = g_Lua->GetInteger(1);
	int timeout = g_Lua->GetInteger(2);

	if(sock == INVALID_SOCKET)
		return 0;
	
	char buffer;
	std::string data = "";
	int r = 1;

	WSAPOLLFD poll;
	poll.fd = sock;
	poll.events = POLLRDNORM;

	WSAPoll(&poll, 1, timeout);

	while(r == 1)
	{
		r = recv(sock, &buffer, 1, 0);

		if(r == 1)
		{
			if(buffer == '\n')
			{
				g_Lua->Push(data.c_str());
				return 1;
			}

			if(buffer != '\r')
			{
				data.append(1, buffer);
			}
		}
	}

	g_Lua->Push(data.c_str());
	return 1;
}

int tcpclose(lua_State* L)
{
	__try
	{
		SOCKET sock = g_Lua->GetInteger(1);

		if(sock == INVALID_SOCKET)
			return 0;

		closesocket(sock);

		return 0;
	}
	__finally
	{
		return 0;
	}

	return 0;
}

int Init(void)
{
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 0;

	g_Lua->SetGlobal("tcpsend", tcpsend);
	g_Lua->SetGlobal("tcplisten", tcplisten);
	g_Lua->SetGlobal("tcpaccept", tcpaccept);
	g_Lua->SetGlobal("tcprecv", tcprecv);
	g_Lua->SetGlobal("tcpclose", tcpclose);

	return 0;
}

int Shutdown(void)
{
	WSACleanup();
	return 0;
}