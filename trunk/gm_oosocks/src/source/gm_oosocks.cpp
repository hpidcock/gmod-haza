#define NO_SDK

#include "GMLuaModule.h"
#include "ThreadedSocket.h"

#ifdef WIN32
#undef GetObject
#endif

GMOD_MODULE(Init, Shutdown);

std::vector<CThreadedSocket *> *sockets[2];

namespace OOSock
{
	LUA_FUNCTION(__new)
	{
		Lua()->CheckType(1, GLua::TYPE_NUMBER);

		int proto = Lua()->GetInteger(1);

		if(proto != IPPROTO_TCP && proto != IPPROTO_UDP)
		{
			Lua()->LuaError("Arg must be IPPROTO_TCP or IPPROTO_UDP", 1);
			return 0;
		}

		CThreadedSocket *sock = new CThreadedSocket(L, proto);

		AutoUnRef meta = Lua()->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
		sock->Ref();
		Lua()->PushUserData(meta, static_cast<void *>(sock));

		return 1;
	}

	LUA_FUNCTION(__delete)
	{
		Lua()->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->UnRef();

		return 0;
	}

	LUA_FUNCTION(SetCallback)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_FUNCTION);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->SetCallback(Lua()->GetReference(2));

		return 0;
	}

	LUA_FUNCTION(Bind)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_STRING);
		Lua()->CheckType(3, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		Lua()->Push((float)sock->Bind(Lua()->GetInteger(3), Lua()->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(Listen)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		Lua()->Push((float)sock->Listen(Lua()->GetInteger(2)));

		return 1;
	}

	LUA_FUNCTION(Accept)
	{
		Lua()->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		Lua()->Push((float)sock->Accept());

		return 1;
	}

	LUA_FUNCTION(Connect)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_STRING);
		Lua()->CheckType(3, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		Lua()->Push((float)sock->Connect(Lua()->GetString(2), Lua()->GetInteger(3)));

		return 1;
	}

	LUA_FUNCTION(Send)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		if(Lua()->GetType(3) == GLua::TYPE_STRING && Lua()->GetType(4) == GLua::TYPE_NUMBER)
		{
			Lua()->Push((float)sock->Send(Lua()->GetString(2), Lua()->GetString(3), Lua()->GetInteger(4)));
		}
		else
		{
			Lua()->Push((float)sock->Send(Lua()->GetString(2)));
		}

		return 1;
	}

	LUA_FUNCTION(SendLine)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		if(Lua()->GetType(3) == GLua::TYPE_STRING && Lua()->GetType(4) == GLua::TYPE_NUMBER)
		{
			Lua()->Push((float)sock->Send(std::string(Lua()->GetString(2)) + "\n", Lua()->GetString(3), Lua()->GetInteger(4)));
		}
		else
		{
			Lua()->Push((float)sock->Send(std::string(Lua()->GetString(2)) + "\n"));
		}

		return 1;
	}

	LUA_FUNCTION(Receive)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_NUMBER);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		Lua()->Push((float)sock->Receive(Lua()->GetInteger(2)));

		return 1;
	}

	LUA_FUNCTION(ReceiveLine)
	{
		Lua()->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		Lua()->Push((float)sock->ReceiveLine());

		return 1;
	}

	LUA_FUNCTION(Close)
	{
		Lua()->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->Close();

		return 0;
	}

	LUA_FUNCTION(ReceiveDatagram)
	{
		Lua()->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		Lua()->Push((float)sock->ReceiveDatagram());

		return 1;
	}

	LUA_FUNCTION(STATIC_CallbackHook)
	{
		std::vector<CThreadedSocket *> *socketsList = NULL;
		if(Lua()->IsClient())
		{
			socketsList = sockets[0];
		}
		else
		{
			socketsList = sockets[1];
		}

		std::vector<CThreadedSocket *> copy = *socketsList;
		std::vector<CThreadedSocket *>::iterator itor = copy.begin();
		while(itor != copy.end())
		{
			(*itor)->InvokeCallbacks();

			itor++;
		}
		return 0;
	}
};

int Init(lua_State* L)
{
	if(Lua()->IsClient())
	{
		sockets[0] = new std::vector<CThreadedSocket *>();
	}
	else
	{
		sockets[1] = new std::vector<CThreadedSocket *>();
	}

#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	AutoUnRef meta = Lua()->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
	{
		AutoUnRef __index = Lua()->GetNewTable();

		__index->SetMember("Bind", OOSock::Bind);
		__index->SetMember("Listen", OOSock::Listen);
		__index->SetMember("Accept", OOSock::Accept);
		__index->SetMember("Connect", OOSock::Connect);
		__index->SetMember("Send", OOSock::Send);
		__index->SetMember("SendLine", OOSock::SendLine);
		__index->SetMember("Receive", OOSock::Receive);
		__index->SetMember("ReceiveLine", OOSock::ReceiveLine);
		__index->SetMember("ReceiveDatagram", OOSock::ReceiveDatagram);
		__index->SetMember("SetCallback", OOSock::SetCallback);
		__index->SetMember("Close", OOSock::Close);

		meta->SetMember("__index", __index);
	}
	meta->SetMember("__gc", OOSock::__delete);

	Lua()->SetGlobal("OOSock", OOSock::__new);
	Lua()->SetGlobal("IPPROTO_TCP", (float)IPPROTO_TCP);
	Lua()->SetGlobal("IPPROTO_UDP", (float)IPPROTO_UDP);

	Lua()->SetGlobal("SCKERR_OK", (float)SOCK_ERROR::OK);
	Lua()->SetGlobal("SCKERR_BAD", (float)SOCK_ERROR::BAD);
	Lua()->SetGlobal("SCKERR_CONNECTION_RESET", (float)SOCK_ERROR::CONNECTION_RESET);
	Lua()->SetGlobal("SCKERR_NOT_CONNECTED", (float)SOCK_ERROR::NOT_CONNECTED);
	Lua()->SetGlobal("SCKERR_TIMED_OUT", (float)SOCK_ERROR::TIMED_OUT);

	Lua()->SetGlobal("SCKCALL_CONNECT", (float)SOCK_CALL::CONNECT);
	Lua()->SetGlobal("SCKCALL_LISTEN", (float)SOCK_CALL::LISTEN);
	Lua()->SetGlobal("SCKCALL_BIND", (float)SOCK_CALL::BIND);
	Lua()->SetGlobal("SCKCALL_ACCEPT", (float)SOCK_CALL::ACCEPT);
	Lua()->SetGlobal("SCKCALL_REC_LINE", (float)SOCK_CALL::REC_LINE);
	Lua()->SetGlobal("SCKCALL_REC_SIZE", (float)SOCK_CALL::REC_SIZE);
	Lua()->SetGlobal("SCKCALL_SEND", (float)SOCK_CALL::SEND);
	Lua()->SetGlobal("SCKCALL_REC_DATAGRAM", (float)SOCK_CALL::REC_DATAGRAM);

	AutoUnRef hookSystem = Lua()->GetGlobal("hook");
	AutoUnRef hookAdd = hookSystem->GetMember("Add");
	
	hookAdd->Push();
	Lua()->Push("Think");
	Lua()->Push("__OOSOCKS_CALLBACKHOOK__");
	Lua()->Push(OOSock::STATIC_CallbackHook);

	Lua()->Call(3, 0);

	return 0;
}

int Shutdown(lua_State* L)
{
	std::vector<CThreadedSocket *> *socketsList = NULL;
	if(Lua()->IsClient())
	{
		socketsList = sockets[0];
	}
	else
	{
		socketsList = sockets[1];
	}

	std::vector<CThreadedSocket *> copy = *socketsList;
	std::vector<CThreadedSocket *>::iterator itor = copy.begin();
	while(itor != copy.end())
	{
		delete (*itor);

		itor++;
	}

#ifdef WIN32
	WSACleanup();
#endif

	if(Lua()->IsClient())
	{
		delete sockets[0];
		sockets[0] = NULL;
	}
	else
	{
		delete sockets[1];
		sockets[1] = NULL;
	}

	return 0;
}
