#include <windows.h>

#include <psapi.h>

#undef _UNICODE

#include <interface.h>
#include "icvar.h"
#include "convar.h"

#include "GMLuaModule.h"

#include "IMenuSystem.h"
#include "CLuaConsoleDisplayFunc.h"

#include "Color.h"

#include "AutoUnRef.h"

GMOD_MODULE(Init, Shutdown);

static ICvar *cvar = NULL;
static ICvar *g_pCVar = NULL;
static IMenuSystem001 *menusystem = NULL;
static CLuaConsoleDisplayFunc *consoledisplay = NULL;

void CLuaConsoleDisplayFunc::ColorPrint( const Color& clr, const char *pMessage )
{
	AutoUnRef table = g_Lua->GetGlobal("ConsoleTextBuffer");

	if(table->isNil())
	{
		table = g_Lua->GetNewTable();
		g_Lua->SetGlobal("ConsoleTextBuffer", table);
	}

	AutoUnRef gTable = g_Lua->GetGlobal("table");
	AutoUnRef gInsert = gInsert->GetMember("insert");
	
	gInsert->Push();
	table->Push();

	AutoUnRef message = g_Lua->GetNewTable();
	message->SetMember((float)1, pMessage);

	AutoUnRef color = g_Lua->GetNewTable();
	color->SetMember("r", (float)clr.r());
	color->SetMember("g", (float)clr.g());
	color->SetMember("b", (float)clr.b());
	color->SetMember("a", (float)clr.a());

	message->SetMember((float)2, color);

	message->Push();

	g_Lua->Call(2);
}

void CLuaConsoleDisplayFunc::Print( const char *pMessage )
{
	AutoUnRef table = g_Lua->GetGlobal("ConsoleTextBuffer");

	if(table->isNil())
	{
		table = g_Lua->GetNewTable();
		g_Lua->SetGlobal("ConsoleTextBuffer", table);
	}

	AutoUnRef gTable = g_Lua->GetGlobal("table");
	AutoUnRef gInsert = gInsert->GetMember("insert");
	
	gInsert->Push();
	table->Push();

	AutoUnRef message = g_Lua->GetNewTable();
	message->SetMember((float)1, pMessage);

	AutoUnRef color = g_Lua->GetNewTable();
	color->SetMember("r", (float)255);
	color->SetMember("g", (float)255);
	color->SetMember("b", (float)255);
	color->SetMember("a", (float)255);

	message->SetMember((float)2, color);

	message->Push();

	g_Lua->Call(2);
}

void CLuaConsoleDisplayFunc::DPrint( const char *pMessage )
{
	AutoUnRef table = g_Lua->GetGlobal("ConsoleTextBuffer");

	if(table->isNil())
	{
		table = g_Lua->GetNewTable();
		g_Lua->SetGlobal("ConsoleTextBuffer", table);
	}

	AutoUnRef gTable = g_Lua->GetGlobal("table");
	AutoUnRef gInsert = gInsert->GetMember("insert");
	
	gInsert->Push();
	table->Push();

	AutoUnRef message = g_Lua->GetNewTable();
	message->SetMember((float)1, pMessage);

	AutoUnRef color = g_Lua->GetNewTable();
	color->SetMember("r", (float)255);
	color->SetMember("g", (float)0);
	color->SetMember("b", (float)0);
	color->SetMember("a", (float)255);

	message->SetMember((float)2, color);

	message->Push();

	g_Lua->Call(2);
}

void DumpVTable(const char* name, const char* modulename, void *object)
{
	MODULEINFO mi;
	memset(&mi, 0, sizeof(MODULEINFO));

	GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(modulename), &mi, sizeof(MODULEINFO));	

	void ***mem = (void ***)object;
	size_t base = (size_t)mi.lpBaseOfDll;
	size_t imgsz = (size_t)mi.SizeOfImage;
	size_t dlloffsetbase = 0x10000000;

	Msg("Dumping %s VTable @ dlloffset %p:\n", name, (void *)((size_t)(*mem) - base + dlloffsetbase));

	for(int i = 0; (size_t)(*mem)[i] - base < imgsz; i++)
	{
		Msg("\tindex=%d addr=%p dlloffset=%p\n", i, (*mem)[i], (void *)((size_t)(*mem)[i] - base + dlloffsetbase));
	}
	
	Msg("\n");
}

int Init(void)
{
	g_pCVar = *(ICvar **)GetProcAddress(GetModuleHandleA("client.dll"), "cvar");
	cvar = g_pCVar;

	CreateInterfaceFn GetInterface = Sys_GetFactory("menusystem.dll");

	menusystem = (IMenuSystem001 *)GetInterface("MenuSystem001", NULL);

	consoledisplay = new CLuaConsoleDisplayFunc();

	//DumpVTable("IMenuSystem", "menusystem.dll", menusystem);

	cvar->InstallConsoleDisplayFunc(consoledisplay);

	return 0;
}

int Shutdown(void)
{
	if(!consoledisplay)
		return 0;

	cvar->RemoveConsoleDisplayFunc(consoledisplay);

	delete consoledisplay;

	return 0;
}