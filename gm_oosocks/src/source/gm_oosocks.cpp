#include "GMLuaModule.h"
#include "ThreadedSocket.h"

#ifdef WIN32
#undef GetObject
#endif

GMOD_MODULE(Init, Shutdown);

std::vector<CThreadedSocket *> sockets;

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

		CThreadedSocket *sock = new CThreadedSocket(proto);

		AutoUnRef meta = g_Lua->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
		sock->Ref();
		g_Lua->PushUserData(meta, static_cast<void *>(sock));

		return 1;
	}

	LUA_FUNCTION(__delete)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->UnRef();

		return 0;
	}

	LUA_FUNCTION(SetCallback)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_FUNCTION);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->SetCallback(g_Lua->GetReference(2));

		return 0;
	}

	LUA_FUNCTION(Bind)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_STRING);
		g_Lua->CheckType(3, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->Bind(g_Lua->GetInteger(3), g_Lua->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(Listen)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->Listen(g_Lua->GetInteger(2)));

		return 1;
	}

	LUA_FUNCTION(Accept)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->Accept());

		return 1;
	}

	LUA_FUNCTION(Connect)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_STRING);
		g_Lua->CheckType(3, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->Connect(g_Lua->GetString(2), g_Lua->GetInteger(3)));

		return 1;
	}

	LUA_FUNCTION(Send)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_STRING);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->Send(g_Lua->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(SendLine)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_STRING);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->Send(std::string(g_Lua->GetString(2)) + "\n"));

		return 1;
	}

	LUA_FUNCTION(Receive)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);
		g_Lua->CheckType(2, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->Receive(g_Lua->GetInteger(2)));

		return 1;
	}

	LUA_FUNCTION(ReceiveLine)
	{
		g_Lua->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(g_Lua->GetUserData(1));

		if(sock == NULL)
			return 0;

		g_Lua->Push((float)sock->ReceiveLine());

		return 1;
	}

	LUA_FUNCTION(STATIC_CallbackHook)
	{
		std::vector<CThreadedSocket *> copy = sockets;
		std::vector<CThreadedSocket *>::iterator itor = copy.begin();
		while(itor != copy.end())
		{
			(*itor)->InvokeCallbacks();

			itor++;
		}
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
		__index->SetMember("Send", OOSock::Send);
		__index->SetMember("SendLine", OOSock::SendLine);
		__index->SetMember("Receive", OOSock::Receive);
		__index->SetMember("ReceiveLine", OOSock::ReceiveLine);
		__index->SetMember("SetCallback", OOSock::SetCallback);

		meta->SetMember("__index", __index);
	}
	meta->SetMember("__gc", OOSock::__delete);

	g_Lua->SetGlobal("OOSock", OOSock::__new);
	g_Lua->SetGlobal("IPPROTO_TCP", (float)IPPROTO_TCP);
	g_Lua->SetGlobal("IPPROTO_UDP", (float)IPPROTO_UDP);

	g_Lua->SetGlobal("SCKERR_OK", (float)SOCK_ERROR::OK);
	g_Lua->SetGlobal("SCKERR_BAD", (float)SOCK_ERROR::BAD);
	g_Lua->SetGlobal("SCKERR_CONNECTION_RESET", (float)SOCK_ERROR::CONNECTION_RESET);
	g_Lua->SetGlobal("SCKERR_NOT_CONNECTED", (float)SOCK_ERROR::NOT_CONNECTED);
	g_Lua->SetGlobal("SCKERR_TIMED_OUT", (float)SOCK_ERROR::TIMED_OUT);

	g_Lua->SetGlobal("SCKCALL_CONNECT", (float)SOCK_CALL::CONNECT);
	g_Lua->SetGlobal("SCKCALL_LISTEN", (float)SOCK_CALL::LISTEN);
	g_Lua->SetGlobal("SCKCALL_BIND", (float)SOCK_CALL::BIND);
	g_Lua->SetGlobal("SCKCALL_ACCEPT", (float)SOCK_CALL::ACCEPT);
	g_Lua->SetGlobal("SCKCALL_REC_LINE", (float)SOCK_CALL::REC_LINE);
	g_Lua->SetGlobal("SCKCALL_REC_SIZE", (float)SOCK_CALL::REC_SIZE);
	g_Lua->SetGlobal("SCKCALL_SEND", (float)SOCK_CALL::SEND);

	AutoUnRef hookSystem = g_Lua->GetGlobal("hook");
	AutoUnRef hookAdd = hookSystem->GetMember("Add");
	
	hookAdd->Push();
	g_Lua->Push("Think");
	g_Lua->Push("__OOSOCKS_CALLBACKHOOK__");
	g_Lua->Push(OOSock::STATIC_CallbackHook);

	g_Lua->Call(3, 0);

	return 0;
}

int Shutdown(void)
{
#ifdef WIN32
	WSACleanup();
#endif

	return 0;
}
