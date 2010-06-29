#define _RETAIL

#define WINDOWS_LEAN_AND_MEAN

#include <windows.h>

#include <interface.h>

#include "icvar.h"
#include "convar.h"
#include "iclient.h"
#include "inetchannel.h"
#include "inetchannelinfo.h"
#include "bitbuf.h"

#include <string>

#include "GMLuaModule.h"

GMOD_MODULE(Load, Unload)

ICvar *cvar2[2] = {NULL, NULL};

#define FCVAR_DEVELOPMENTONLY	(1<<1)

LUA_FUNCTION(SetFlags)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConCommandBase *var = cvar2[Lua()->IsClient()]->FindCommandBase(Lua()->GetString(1));

	if(!var)
	{
		Lua()->Error("cvar/ccmd not found.");
		return 0;
	}

	// m_nFlags needs to be made public in convar.h, this will only affect this dll, the class struct will stay the same but the compiler won't complain.
	int *flags = &var->m_nFlags;
	*flags = Lua()->GetInteger(2);

	return 0;
}

LUA_FUNCTION(GetFlags)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);

	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConCommandBase *var = cvar2[Lua()->IsClient()]->FindCommandBase(Lua()->GetString(1));

	if(!var)
	{
		Lua()->Error("cvar/ccmd not found.");
		return 0;
	}

	// m_nFlags needs to be made public in convar.h, this will only affect this dll, the class struct will stay the same but the compiler won't complain.
	Lua()->PushDouble((int)var->m_nFlags);

	return 1;
}

LUA_FUNCTION(ToggleFlag)  
{  
	Lua()->CheckType(1, GLua::TYPE_STRING);  
	Lua()->CheckType(2, GLua::TYPE_NUMBER);  

	if(!cvar2[Lua()->IsClient()])  
	{  
		Lua()->Error("CVar interface not loaded.");  
		return 0;  
	}  

	ConCommandBase *var = cvar2[Lua()->IsClient()]->FindCommandBase(Lua()->GetString(1));  

	if(!var)  
	{  
		Lua()->Error("cvar/ccmd not found.");  
		return 0;  
	}  

	// m_nFlags needs to be made public in convar.h, this will only affect this dll, the class struct will stay the same but the compiler won't complain.  
	int *flags = &var->m_nFlags;  

	// Thanks to hfs for this section :)
	*flags ^= Lua()->GetInteger(2);  

	Lua()->Push((*flags & Lua()->GetInteger(2)) > 0);

	return 1;  
}  

LUA_FUNCTION(GetAll)
{
	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConCommandBase *var = cvar2[Lua()->IsClient()]->GetCommands();

	if(!var)
	{
		Lua()->Error("CVar GetAll error");
		return 0;
	}

	CAutoUnRef cvarTbl = Lua()->GetNewTable();

	int incr = 1;
	while(var)
	{
		cvarTbl->SetMember(incr, var->GetName());
		var = var->GetNext();
		incr++;
	}

	Lua()->Push(cvarTbl);

	return 1;
}

LUA_FUNCTION(SetHelpText)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConCommandBase *var = cvar2[Lua()->IsClient()]->FindCommandBase(Lua()->GetString(1));

	if(!var)
	{
		Lua()->Error("cvar/ccmd not found.");
		return 0;
	}

	// This is really bad, call this too much, then you will have a memleak. Will fix one day.
	std::string tmp = Lua()->GetString(2);
	const char *mem = (const char *)malloc(tmp.size() + 1);
	memcpy((void *)mem, tmp.c_str(), tmp.size());
	((char *)mem)[tmp.size()] = 0x00;

	// m_pszHelpString needs to be made public in convar.h, this will only affect this dll, the class struct will stay the same but the compiler won't complain.
	var->m_pszHelpString = &mem[0];

	return 0;
}

LUA_FUNCTION(GetValue)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);

	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConVar *var = cvar2[Lua()->IsClient()]->FindVar(Lua()->GetString(1));

	if(!var)
	{
		Lua()->Error("cvar not found.");
		return 0;
	}

	Lua()->Push(var->GetString());

	return 1;
}

LUA_FUNCTION(IsFlagSet)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConVar *var = cvar2[Lua()->IsClient()]->FindVar(Lua()->GetString(1));

	if(!var)
	{
		Lua()->Error("cvar not found.");
		return 0;
	}

	Lua()->Push((bool)var->IsFlagSet(Lua()->GetInteger(2)));

	return 1;
}

LUA_FUNCTION(SetValue)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);

	int dtType = Lua()->GetType(2);

	if(dtType == GLua::TYPE_NIL || dtType == GLua::TYPE_INVALID)
	{
		Lua()->Error("arg[2] invalid");
		return 0;
	}

	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConVar *var = cvar2[Lua()->IsClient()]->FindVar(Lua()->GetString(1));

	if(!var)
	{
		Lua()->Error("cvar not found.");
		return 0;
	}

	switch(dtType)
	{
	case GLua::TYPE_NUMBER:
		var->SetValue((float)Lua()->GetDouble(2));
		break;
	case GLua::TYPE_BOOL:
		var->SetValue(Lua()->GetBool(2) ? 1 : 0);
		break;
	case GLua::TYPE_STRING:
		var->SetValue(Lua()->GetString(2));
		break;
	default:
		Lua()->Error("arg[2] invalid");
		return 0;
		break;
	}

	return 0;
}

LUA_FUNCTION(ResetValue)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);

	if(!cvar2[Lua()->IsClient()])
	{
		Lua()->Error("CVar interface not loaded.");
		return 0;
	}

	ConVar *var = cvar2[Lua()->IsClient()]->FindVar(Lua()->GetString(1));

	if(!var)
	{
		Lua()->Error("cvar not found.");
		return 0;
	}

	var->Revert();

	return 0;
}

/*
LUA_FUNCTION(ExecuteCommandOnClient)
{
	Lua()->CheckType(1, GLua::TYPE_NUMBER);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	if(!pClientListSize)
		return 0;

	int clientId = Lua()->GetInteger(1);
	
	for(int i = 0; i < (*pClientListSize); i++)
	{
		IClient *c = (IClient *)((size_t)(*ppClientList)[i] + 4); // Gate Keeper

		if(c->GetUserID() - 1 == clientId)
		{
			c->ExecuteStringCommand(Lua()->GetString(2));
			break;
		}
	}

	return 0;
}

LUA_FUNCTION(GetUserSetting)
{
	Lua()->CheckType(1, GLua::TYPE_NUMBER);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	if(!pClientListSize)
		return 0;

	int clientId = Lua()->GetInteger(1);

	for(int i = 0; i < (*pClientListSize); i++)
	{
		IClient *c = (IClient *)((size_t)(*ppClientList)[i] + 4); // Gate Keeper

		if(c->GetUserID() - 1 == clientId)
		{
			Lua()->Push(c->GetUserSetting(Lua()->GetString(2)));
			return 1;
		}
	}

	return 0;
}

LUA_FUNCTION(ReplicateData)
{
	Lua()->CheckType(1, GLua::TYPE_NUMBER);
	Lua()->CheckType(2, GLua::TYPE_STRING);
	Lua()->CheckType(3, GLua::TYPE_STRING);

	if(!pClientListSize)
		return 0;
	
	int clientId = Lua()->GetInteger(1);

	for(int i = 0; i < (*pClientListSize); i++)
	{
		IClient *c = (IClient *)((size_t)(*ppClientList)[i] + 4); // Gate Keeper

		if(c->GetUserID() - 1 == clientId)
		{
			INetChannel *netChan = c->GetNetChannel();
			char pckBuf[256];
			bf_write pck(pckBuf, sizeof(pckBuf));

			pck.WriteUBitLong(5, 5);
			pck.WriteByte(0x01);
			pck.WriteString(Lua()->GetString(2));
			pck.WriteString(Lua()->GetString(3));

			netChan->SendData(pck);
			
			break;
		}
	}

	return 0;
}
*/

int Load(lua_State* L)
{
	// cvar is the only(usefull) DLL export made in server.dll client.dll
	// this is better than getting a new interface to this etc.
	cvar2[Lua()->IsClient()] = *(ICvar **)GetProcAddress(GetModuleHandleA(Lua()->IsClient() ? "client.dll" : "server.dll"), "cvar");

	CAutoUnRef cvarTbl = Lua()->GetNewTable();
		cvarTbl->SetMember("SetFlags", SetFlags);
		cvarTbl->SetMember("GetFlags", GetFlags);
		cvarTbl->SetMember("SetHelpText", SetHelpText);
		cvarTbl->SetMember("GetAll", GetAll);
		cvarTbl->SetMember("ToggleFlag", ToggleFlag);
		cvarTbl->SetMember("GetValue", GetValue);
		cvarTbl->SetMember("SetValue", SetValue);
		cvarTbl->SetMember("IsFlagSet", IsFlagSet);
		cvarTbl->SetMember("ResetValue", ResetValue);
		/*cvarTbl->SetMember("ExecuteCommandOnClient", ExecuteCommandOnClient);
		cvarTbl->SetMember("GetUserSetting", GetUserSetting);
		cvarTbl->SetMember("ReplicateData", ReplicateData);*/
	Lua()->SetGlobal("cvar2", cvarTbl);

	Lua()->SetGlobal("FCVAR_DEVELOPMENTONLY", (float)FCVAR_DEVELOPMENTONLY);

	if(!cvar2[Lua()->IsClient()])
		Lua()->Error("CVar interface not loaded.");

	return 0;
}

int Unload(lua_State* L)
{
	return 0;
}