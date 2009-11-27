#include "winlite.h"

#undef _UNICODE

#include <interface.h>
#include "icvar.h"
#include "convar.h"

#include "detours.h"

#include "GMLuaModule.h"

extern "C"
{
	#include "lua.h"
}

GMOD_MODULE(Init, Shutdown);

static ICvar *g_pCVar = NULL;

lua_State *(lua_newstateOriginal)(lua_Alloc, void *);
lua_State *(* lua_newstateDetour)(lua_Alloc, void *);
lua_State *(* lua_newstateTramp)(lua_Alloc, void *);

DETOUR_TRAMPOLINE_EMPTY(lua_State *(lua_newstateOriginal)(lua_Alloc, void *));

static lua_State *currentState = NULL;

lua_State *lua_newstateNew(lua_Alloc f, void *ud)
{
	lua_State* L = lua_newstateOriginal(f, ud);
	
	currentState = L;

	return L;
}

typedef struct LoadS {
	const char *s;
	size_t size;
} LoadS;

static const char *getS (lua_State *L, void *ud, size_t *size) {
	LoadS *ls = (LoadS *)ud;
	(void)L;
	if (ls->size == 0) return NULL;
	*size = ls->size;
	ls->size = 0;
	return ls->s;
}

void LoadLua(const CCommand &command)
{
	if(currentState == NULL)
		return;

	// util.RelativePathToFull(  path )

	lua_getglobal(currentState, "util");
	lua_pushstring(currentState, "RelativePathToFull");
	lua_gettable(currentState, -2);

	lua_pushstring(currentState, command.ArgS());
	if(lua_pcall(currentState, 1, 1, 0) != 0)
	{
		lua_pop(currentState, 1);
		return;
	}

	const char *fullpath = lua_tostring(currentState, -1);
	lua_pop(currentState, 2);

	FILE *f = fopen(fullpath, "rb");

	if(f == NULL)
		return;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);

	char *data = (char *)malloc(size);
	
	fread(data, 1, size, f);

	fclose(f);

	// The game will think any calls from this code are coming from the base gamemodes cl_init.lua
	//luaL_loadbuffer(currentState, data, size, "gamemodes\\base\\gamemode\\cl_init.lua");

	LoadS ls;
	ls.s = data;
	ls.size = size;
	lua_load(currentState, getS, &ls, "gamemodes\\base\\gamemode\\cl_init.lua");

	lua_pcall(currentState, 0, 0, 0);

	free(data);
}

static ConCommand *cmd;

int Init(void)
{
	lua_newstateDetour = &lua_newstateNew;
	lua_newstateTramp = &lua_newstateOriginal;

	// Detour the lua_newstate function from lua_shared.dll so we can capture the lua_State when its created.
	DetourFunctionWithEmptyTrampoline(*(PBYTE *) &lua_newstateTramp, (PBYTE) &lua_newstate, *(PBYTE *) &lua_newstateDetour);

	g_pCVar = *(ICvar **)GetProcAddress(GetModuleHandleA("client.dll"), "cvar");

	cmd = new ConCommand("lua_se2_load", LoadLua, "", FCVAR_UNREGISTERED);

	g_pCVar->RegisterConCommand(cmd);

	return 0;
}

int Shutdown(void)
{
	DetourRemove(*(PBYTE*) &lua_newstateOriginal, *(PBYTE*) &lua_newstateDetour);

	g_pCVar->UnregisterConCommand(cmd);

	delete cmd;

	return 0;
}