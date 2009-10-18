#include "GMLuaModule.h"
#include "wsocket.h"

extern "C" {
int luaopen_socket_core(lua_State *L);
int luaopen_mime_core(lua_State *L);
}

GMOD_MODULE(Init, Shutdown);

static int tcpsend(lua_State* L)
{
	t_socket sock = luaL_checkinteger(L, 1);
	const char *str = luaL_checkstring(L, 2);
	int delay = luaL_checkinteger(L, 3); // in ms

	fd_set set;
	timeval timeout;

	if(str == NULL)
		return 0;

	if(sock == SOCKET_INVALID)
		return 0;

	timeout.tv_sec = 0;
	timeout.tv_usec = delay * 1000;
	
	FD_ZERO(&set);
	FD_SET(sock, &set);

	send(sock, str, strlen(str), 0);

	select(NULL, NULL, &set, NULL, &timeout);

	return 0;
}

int Init(void)
{
	lua_State *L = (lua_State*)g_Lua->GetLuaState();

	/* Ugly hack: */
	lua_newtable(L);
	lua_pushcfunction(L, luaopen_socket_core);
	lua_setfield(L, -2, "luaopen_socket_core");
	lua_pushcfunction(L, luaopen_mime_core);
	lua_setfield(L, -2, "luaopen_mime_core");
	lua_setglobal(L, "luasocket_stuff"); /* ugly hack end */

	lua_pushcfunction(L, tcpsend);
	lua_setglobal(L, "tcpsend");

	return 0;
}

int Shutdown(void)
{
	return 0;
}