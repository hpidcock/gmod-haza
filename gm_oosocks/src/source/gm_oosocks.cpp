#define NO_SDK

#include "GMLuaModule.h"
#include "ThreadedSocket.h"
#include "CBinRead.h"
#include "CBinWrite.h"

#ifdef WIN32
#undef GetObject
#endif

GMOD_MODULE(Init, Shutdown);

std::map< lua_State *, std::vector<CThreadedSocket *> > g_Socks;

size_t GetStringSize(lua_State *L, int stackPos)
{
	return Lua()->StringLength(stackPos);
}

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

		CAutoUnRef meta = Lua()->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
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

	LUA_FUNCTION(__eq)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, TYPE_SOCKET);

		Lua()->Push(Lua()->GetUserData(1) == Lua()->GetUserData(2));

		return 1;
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

		Lua()->Push((float)sock->Bind(Lua()->GetInteger(3), CString(Lua()->GetString(2), GetStringSize(L, 2))));

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

		Lua()->Push((float)sock->Connect(CString(Lua()->GetString(2), GetStringSize(L, 2)), Lua()->GetInteger(3)));

		return 1;
	}

	LUA_FUNCTION(Send)
	{
		Lua()->CheckType(1, TYPE_SOCKET);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		CString data;
		if(Lua()->GetType(2) == GLua::TYPE_STRING)
		{
			data = CString(Lua()->GetString(2), GetStringSize(L, 2));
		}
		else if(Lua()->GetType(2) == TYPE_BINWRITE)
		{
			CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

			if(write == NULL)
				return 0;

			size_t size = 0;
			data = CString((const char *)write->GetData(size), size);
		}
		else
		{
			return 0;
		}

		if(Lua()->GetType(3) == GLua::TYPE_STRING && Lua()->GetType(4) == GLua::TYPE_NUMBER)
		{
			Lua()->Push((float)sock->Send(data, CString(Lua()->GetString(3), GetStringSize(L, 3)), Lua()->GetInteger(4)));
		}
		else
		{
			Lua()->Push((float)sock->Send(data));
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

		std::string data = std::string(Lua()->GetString(2)) + "\n";

		if(Lua()->GetType(3) == GLua::TYPE_STRING && Lua()->GetType(4) == GLua::TYPE_NUMBER)
		{
			Lua()->Push((float)sock->Send(CString(data.c_str(), data.size()), CString(Lua()->GetString(3), GetStringSize(L, 3)), Lua()->GetInteger(4)));
		}
		else
		{
			Lua()->Push((float)sock->Send(CString(data.c_str(), data.size())));
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

	LUA_FUNCTION(SetBinaryMode)
	{
		Lua()->CheckType(1, TYPE_SOCKET);
		Lua()->CheckType(2, GLua::TYPE_BOOL);

		CThreadedSocket *sock = reinterpret_cast<CThreadedSocket *>(Lua()->GetUserData(1));

		if(sock == NULL)
			return 0;

		sock->SetBinaryMode(Lua()->GetBool(2));

		return 0;
	}

	LUA_FUNCTION(STATIC_CallbackHook)
	{
		std::vector<CThreadedSocket *> *socketsList = &g_Socks[L];

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

namespace BinRead
{
	LUA_FUNCTION(__delete)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		delete read;

		return 0;
	}

	LUA_FUNCTION(ReadDouble)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->PushDouble((double)read->ReadDouble());

		return 1;
	};

	LUA_FUNCTION(ReadInt)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->Push((float)read->ReadInt());

		return 1;
	};

	LUA_FUNCTION(ReadFloat)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->Push((float)read->ReadFloat());

		return 1;
	};

	LUA_FUNCTION(ReadByte)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->Push((float)read->ReadByte());

		return 1;
	};

	LUA_FUNCTION(ReadString)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->Push(read->ReadStr());

		return 1;
	};

	LUA_FUNCTION(PeekByte)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->Push((float)read->PeekByte());

		return 1;
	};

	LUA_FUNCTION(GetSize)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->Push((float)read->GetSize());

		return 1;
	};

	LUA_FUNCTION(GetReadPosition)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		Lua()->Push((float)read->GetReadPosition());

		return 1;
	};

	LUA_FUNCTION(Rewind)
	{
		Lua()->CheckType(1, TYPE_BINREAD);

		CBinRead *read = reinterpret_cast<CBinRead *>(Lua()->GetUserData(1));

		if(read == NULL)
			return 0;

		read->Rewind();

		return 0;
	};
}

namespace BinWrite
{
	LUA_FUNCTION(__new)
	{
		CBinWrite *write = new CBinWrite(L);

		CAutoUnRef meta = Lua()->GetMetaTable(MT_BINWRITE, TYPE_BINWRITE);
		Lua()->PushUserData(meta, static_cast<void *>(write));

		return 1;
	}

	LUA_FUNCTION(__delete)
	{
		Lua()->CheckType(1, TYPE_BINWRITE);

		CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

		if(write == NULL)
			return 0;

		delete write;

		return 0;
	};

	LUA_FUNCTION(GetSize)
	{
		Lua()->CheckType(1, TYPE_BINWRITE);

		CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

		if(write == NULL)
			return 0;

		Lua()->Push((float)write->GetSize());

		return 1;
	};

	LUA_FUNCTION(WriteDouble)
	{
		Lua()->CheckType(1, TYPE_BINWRITE);
		Lua()->CheckType(2, GLua::TYPE_NUMBER);

		CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

		if(write == NULL)
			return 0;

		write->Write((double)Lua()->GetDouble(2));

		return 0;
	};

	LUA_FUNCTION(WriteFloat)
	{
		Lua()->CheckType(1, TYPE_BINWRITE);
		Lua()->CheckType(2, GLua::TYPE_NUMBER);

		CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

		if(write == NULL)
			return 0;

		write->Write((float)Lua()->GetNumber(2));

		return 0;
	};

	LUA_FUNCTION(WriteInt)
	{
		Lua()->CheckType(1, TYPE_BINWRITE);
		Lua()->CheckType(2, GLua::TYPE_NUMBER);

		CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

		if(write == NULL)
			return 0;

		write->Write((int)Lua()->GetInteger(2));

		return 0;
	};

	LUA_FUNCTION(WriteByte)
	{
		Lua()->CheckType(1, TYPE_BINWRITE);
		Lua()->CheckType(2, GLua::TYPE_NUMBER);

		CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

		if(write == NULL)
			return 0;

		write->Write((char)Lua()->GetInteger(2));

		return 0;
	};

	LUA_FUNCTION(WriteString)
	{
		Lua()->CheckType(1, TYPE_BINWRITE);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CBinWrite *write = reinterpret_cast<CBinWrite *>(Lua()->GetUserData(1));

		if(write == NULL)
			return 0;

		write->Write(Lua()->GetString(2), GetStringSize(L, 2));

		return 0;
	};
}

int Init(lua_State* L)
{
	g_Socks[L].clear();

#ifdef WIN32
	//WSADATA wsa_data;
	//WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	CAutoUnRef meta = Lua()->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
	{
		CAutoUnRef __index = Lua()->GetNewTable();

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
		__index->SetMember("SetBinaryMode", OOSock::SetBinaryMode);
		__index->SetMember("Close", OOSock::Close);

		meta->SetMember("__index", __index);
	}
	meta->SetMember("__gc", OOSock::__delete);
	meta->SetMember("__eq", OOSock::__eq);

	CAutoUnRef metaBinRead = Lua()->GetMetaTable(MT_BINREAD, TYPE_BINREAD);
	{
		CAutoUnRef __index = Lua()->GetNewTable();

		__index->SetMember("GetReadPosition", BinRead::GetReadPosition);
		__index->SetMember("GetSize", BinRead::GetSize);
		__index->SetMember("PeekByte", BinRead::PeekByte);
		__index->SetMember("ReadByte", BinRead::ReadByte);
		__index->SetMember("ReadDouble", BinRead::ReadDouble);
		__index->SetMember("ReadInt", BinRead::ReadInt);
		__index->SetMember("Rewind", BinRead::Rewind);
		__index->SetMember("ReadFloat", BinRead::ReadFloat);
		__index->SetMember("ReadString", BinRead::ReadString);

		metaBinRead->SetMember("__index", __index);
	}
	metaBinRead->SetMember("__gc", BinRead::__delete);

	CAutoUnRef metaBinWrite = Lua()->GetMetaTable(MT_BINWRITE, TYPE_BINWRITE);
	{
		CAutoUnRef __index = Lua()->GetNewTable();

		__index->SetMember("GetSize", BinWrite::GetSize);
		__index->SetMember("WriteByte", BinWrite::WriteByte);
		__index->SetMember("WriteDouble", BinWrite::WriteDouble);
		__index->SetMember("WriteInt", BinWrite::WriteInt);
		__index->SetMember("WriteFloat", BinWrite::WriteFloat);
		__index->SetMember("WriteString", BinWrite::WriteString);

		metaBinWrite->SetMember("__index", __index);
	}
	metaBinWrite->SetMember("__gc", BinWrite::__delete);

	Lua()->SetGlobal("BinWrite", BinWrite::__new);
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

	CAutoUnRef hookSystem = Lua()->GetGlobal("hook");
	CAutoUnRef hookAdd = hookSystem->GetMember("Add");
	
	hookAdd->Push();
	Lua()->Push("Think");
	Lua()->Push("__OOSOCKS_CALLBACKHOOK__");
	Lua()->Push(OOSock::STATIC_CallbackHook);

	Lua()->Call(3, 0);

	return 0;
}

int Shutdown(lua_State* L)
{
	CAutoUnRef meta = Lua()->GetMetaTable(MT_SOCKET, TYPE_SOCKET);
	Lua()->PushNil();
	meta->SetMember("__gc");

	std::vector<CThreadedSocket *> *socketsList = &g_Socks[L];

	std::vector<CThreadedSocket *> copy = *socketsList;
	std::vector<CThreadedSocket *>::iterator itor = copy.begin();
	while(itor != copy.end())
	{
		delete (CThreadedSocket * )(*itor);

		itor++;
	}

	socketsList->clear();

#ifdef WIN32
	//WSACleanup();
#endif

	return 0;
}
