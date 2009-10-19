#include "GMLuaModule.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

GMOD_MODULE(Init, Shutdown);

struct TcpWorkerData
{
	char **data;
	int datacount;

	SOCKET *socks;
	int sockcount;

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
			_TCP_SEND(d->socks[q], data, timeout);
		}

		free(d->data[i]);
	}

	free(d->socks);
	free(d->data);
	free(d);

	return 0;
}

int tcpsend(lua_State* L)
{
	TcpWorkerData *newData = (TcpWorkerData *)malloc(sizeof(TcpWorkerData));
	memset(newData, 0, sizeof(TcpWorkerData));

	int datacount = luaL_getn(L, 1);

	newData->data = (char **)malloc(datacount * sizeof(char *));
	newData->datacount = datacount;

	for(int i = 0; i < datacount; i++)
	{
		lua_rawgeti(L, 1, i + 1);
		const char *str = lua_tostring(L, -1);
		lua_pop(L, 1);

		if(str == NULL)
			continue;

		int strl = strlen(str);
		newData->data[i] = (char *)malloc(strl + 1);
		memcpy(newData->data[i], str, strl);
		newData->data[i][strl] = 0;
	}

	int sockcount = luaL_getn(L, 2);

	newData->socks = (SOCKET *)malloc(sockcount * sizeof(SOCKET));
	newData->sockcount = sockcount;

	for(int i = 0; i < sockcount; i++)
	{
		lua_rawgeti(L, 2, i + 1);
		SOCKET sock = lua_tointeger(L, -1);
		lua_pop(L, 1);

		newData->socks[i] = sock;
	}

	newData->timeout = luaL_checkinteger(L, 3);

	QueueUserWorkItem(_TCP_SEND_WORKER, newData, WT_EXECUTELONGFUNCTION);

	return 0;
}

int tcplisten(lua_State* L)
{
	int ret = 0;
	__try
	{
		const char *ip = luaL_checkstring(L, 1);
		int port = lua_tointeger(L, 2);
		int backlog = lua_tointeger(L, 3);

		if(ip == NULL)
			return 0;

		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if(sock == INVALID_SOCKET)
		{
			lua_pushnumber(L, sock);
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

		lua_pushnumber(L, sock);

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
		SOCKET sock = lua_tointeger(L, 1);

		if(sock == INVALID_SOCKET)
			return 0;

		int client = accept(sock, NULL, NULL);
		client = client == 0 ? -1 : client;

		if(client != INVALID_SOCKET)
		{
			u_long argp = 1;
			ioctlsocket(client, FIONBIO, &argp);
		}

		lua_pushnumber(L, client);

		ret = 1;
	}
	__finally
	{
		return ret;
	}
}

int tcprecv(lua_State* L)
{	
	SOCKET sock = lua_tointeger(L, 1);
	int timeout = lua_tointeger(L, 2);

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
				lua_pushstring(L, data.c_str());
				return 1;
			}

			if(buffer != '\r')
			{
				data.append(1, buffer);
			}
		}
	}

	lua_pushstring(L, data.c_str());
	return 1;
}

int tcpclose(lua_State* L)
{
	__try
	{
		SOCKET sock = lua_tointeger(L, 1);

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

	lua_State *L = (lua_State*)g_Lua->GetLuaState();

	lua_pushcfunction(L, tcpsend);
	lua_setglobal(L, "tcpsend");
	lua_pushcfunction(L, tcplisten);
	lua_setglobal(L, "tcplisten");
	lua_pushcfunction(L, tcpaccept);
	lua_setglobal(L, "tcpaccept");
	lua_pushcfunction(L, tcprecv);
	lua_setglobal(L, "tcprecv");
	lua_pushcfunction(L, tcpclose);
	lua_setglobal(L, "tcpclose");

	return 0;
}

int Shutdown(void)
{
	WSACleanup();
	return 0;
}