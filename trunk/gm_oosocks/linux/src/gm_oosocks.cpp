#include "GMLuaModule.h"
#include "AutoUnRef.h"
#include "Socket.h"

GMOD_MODULE(Init, Shutdown);

#define MT_SOCKET	"OOSock"
#define TYPE_SOCKET 9753

namespace OOSock
{
	LUA_FUNCTION(__new)
	{
		g_Lua->CheckType(1, GLua::TYPE_NUMBER);

		int proto = g_Lua->GetInteger(1);

		if(proto != IPPROTO_TCP && proto != IPPROTO_UDP)
		{
			g_Lua->LuaError("Arg must be IPPROTO_TCP or IPPROTO_UDP", 1);
			return 0;
		}

		TD::Socket *sock = new TD::Socket(SOCK_STREAM, AF_INET, proto);

		AutoUnRef meta = g_Lua->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
		g_Lua->PushUserData(meta, static_cast<void *>(sock));

		return 1;
	}

	LUA_FUNCTION(__delete)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->close();

		delete sock;

		return 0;
	}

	LUA_FUNCTION(Bind)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_NUMBER);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->bind(g_Lua->GetInteger(2));

		return 0;
	}

	LUA_FUNCTION(Listen)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_NUMBER);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->listen(g_Lua->GetInteger(2));

		return 0;
	}

	LUA_FUNCTION(Accept)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		TD::Socket *newSock = sock->accept();

		if(newSock == NULL)
			return 0;

		AutoUnRef meta = g_Lua->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
		g_Lua->PushUserData(meta, static_cast<void *>(newSock));

		return 1;
	}

	LUA_FUNCTION(Connect)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_STRING);
		g_Lua->CheckType(3, GLua::TYPE_NUMBER);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->connect(g_Lua->GetString(2), g_Lua->GetInteger(3));

		return 0;
	}

	LUA_FUNCTION(SetTimeout)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_NUMBER);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->setTimeout(g_Lua->GetInteger(2));

		return 0;
	}

	LUA_FUNCTION(GetPeerName)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push(sock->getPeerName());

		return 1;
	}

	LUA_FUNCTION(GetHostName)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push(sock->getHostName());

		return 1;
	}

	LUA_FUNCTION(Send)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_STRING);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->sendAll(g_Lua->GetString(2));

		return 0;
	}

	LUA_FUNCTION(SendLine)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_STRING);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->sendln(g_Lua->GetString(2));

		return 0;
	}

	LUA_FUNCTION(Receive)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_NUMBER);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push(sock->recv(g_Lua->GetInteger(2)).c_str());

		return 1;
	}

	LUA_FUNCTION(ReceiveLine)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push(sock->recvln().c_str());
		return 1;
	}

	LUA_FUNCTION(GetLastError)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->getLastError());

		return 1;
	}

	LUA_FUNCTION(Close)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		TD::Socket *sock = static_cast<TD::Socket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->close();

		return 0;
	}
};

int Init(void)
{
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	AutoUnRef meta = g_Lua->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
	{
		AutoUnRef __index = g_Lua->GetNewTable();

		__index->SetMember("Bind", OOSock::Bind);
		__index->SetMember("Listen", OOSock::Listen);
		__index->SetMember("Accept", OOSock::Accept);
		__index->SetMember("Connect", OOSock::Connect);
		__index->SetMember("SetTimeout", OOSock::SetTimeout);
		__index->SetMember("GetPeerName", OOSock::GetPeerName);
		__index->SetMember("GetHostName", OOSock::GetHostName);
		__index->SetMember("Send", OOSock::Send);
		__index->SetMember("SendLine", OOSock::SendLine);
		__index->SetMember("Receive", OOSock::Receive);
		__index->SetMember("ReceiveLine", OOSock::ReceiveLine);
		__index->SetMember("GetLastError", OOSock::GetLastError);
		__index->SetMember("Close", OOSock::Close);

		meta->SetMember("__index", __index);
	}
	meta->SetMember("__gc", OOSock::__delete);

	g_Lua->SetGlobal("OOSock", OOSock::__new);
	g_Lua->SetGlobal("IPPROTO_TCP", (float)IPPROTO_TCP);
	g_Lua->SetGlobal("IPPROTO_UDP", (float)IPPROTO_UDP);

	g_Lua->SetGlobal("SCKERR_OK", (float)TD::SOCK_ERROR::OK);
	g_Lua->SetGlobal("SCKERR_BAD", (float)TD::SOCK_ERROR::BAD);
	g_Lua->SetGlobal("SCKERR_CONNECTION_REST", (float)TD::SOCK_ERROR::CONNECTION_REST);
	g_Lua->SetGlobal("SCKERR_NOT_CONNECTED", (float)TD::SOCK_ERROR::NOT_CONNECTED);
	g_Lua->SetGlobal("SCKERR_TIMED_OUT", (float)TD::SOCK_ERROR::TIMED_OUT);

	return 0;
}

int Shutdown(void)
{
#ifdef WIN32
	WSACleanup();
#endif

	return 0;
}