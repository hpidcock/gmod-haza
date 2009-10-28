#include "winlite.h"

#include <psapi.h>

#undef _UNICODE

#include <interface.h>
#include "icvar.h"
#include "convar.h"

#include "detours.h"

#include "GMLuaModule.h"

#include "IMenuSystem.h"
#include "CLuaConsoleDisplayFunc.h"

#include "Color.h"

extern "C"
{
	#include "lua.h"
}

GMOD_MODULE(Init, Shutdown);

static ICvar *cvar = NULL;
static ICvar *g_pCVar = NULL;
static IMenuSystem001 *menusystem = NULL;
static CLuaConsoleDisplayFunc *consoledisplay = NULL;
static lua_State *L = NULL;

void CLuaConsoleDisplayFunc::ColorPrint( const Color& clr, const char *pMessage )
{
	if(!L)
		return;

	lua_getglobal(L, "table");
	lua_pushstring(L, "insert");
	lua_gettable(L, -2);

	lua_getglobal(L, "ConsoleTextBuffer");
	if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
		lua_setglobal(L, "ConsoleTextBuffer");
		lua_getglobal(L, "ConsoleTextBuffer");
	}

	lua_newtable(L);

	lua_pushstring(L, pMessage);

	lua_rawseti(L, -2, 1);

	lua_getglobal(L, "Color");
	lua_pushnumber(L, clr.r());
	lua_pushnumber(L, clr.g());
	lua_pushnumber(L, clr.b());
	lua_pushnumber(L, clr.a());
	lua_pcall(L, 4, 1, 0);

	lua_rawseti(L, -2, 2);

	lua_pcall(L, 2, 0, 0);

	lua_pop(L, 1);
}

void CLuaConsoleDisplayFunc::Print( const char *pMessage )
{
	if(!L)
		return;

	lua_getglobal(L, "table");
	lua_pushstring(L, "insert");
	lua_gettable(L, -2);

	lua_getglobal(L, "ConsoleTextBuffer");
	if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
		lua_setglobal(L, "ConsoleTextBuffer");
		lua_getglobal(L, "ConsoleTextBuffer");
	}

	lua_newtable(L);

	lua_pushstring(L, pMessage);

	lua_rawseti(L, -2, 1);

	lua_getglobal(L, "Color");
	lua_pushnumber(L, 255);
	lua_pushnumber(L, 255);
	lua_pushnumber(L, 255);
	lua_pushnumber(L, 255);
	lua_pcall(L, 4, 1, 0);

	lua_rawseti(L, -2, 2);

	lua_pcall(L, 2, 0, 0);

	lua_pop(L, 1);
}

void CLuaConsoleDisplayFunc::DPrint( const char *pMessage )
{
	if(!L)
		return;

	lua_getglobal(L, "table");
	lua_pushstring(L, "insert");
	lua_gettable(L, -2);

	lua_getglobal(L, "ConsoleTextBuffer");
	if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
		lua_setglobal(L, "ConsoleTextBuffer");
		lua_getglobal(L, "ConsoleTextBuffer");
	}

	lua_newtable(L);

	lua_pushstring(L, pMessage);

	lua_rawseti(L, -2, 1);

	lua_getglobal(L, "Color");
	lua_pushnumber(L, 255);
	lua_pushnumber(L, 0);
	lua_pushnumber(L, 0);
	lua_pushnumber(L, 255);
	lua_pcall(L, 4, 1, 0);

	lua_rawseti(L, -2, 2);

	lua_pcall(L, 2, 0, 0);

	lua_pop(L, 1);
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

	L = (lua_State *)g_Lua->GetLuaState();

	return 0;
}

int Shutdown(void)
{
	if(!consoledisplay)
		return 0;

	cvar->RemoveConsoleDisplayFunc(consoledisplay);

	delete consoledisplay;

	L = NULL;

	return 0;
}