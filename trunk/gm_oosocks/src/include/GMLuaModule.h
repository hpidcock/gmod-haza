//=============================================================================//
//  ___  ___   _   _   _    __   _   ___ ___ __ __
// |_ _|| __| / \ | \_/ |  / _| / \ | o \ o \\ V /
//  | | | _| | o || \_/ | ( |_n| o ||   /   / \ / 
//  |_| |___||_n_||_| |_|  \__/|_n_||_|\\_|\\ |_|  2006
//										 
//=============================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ILuaInterface.h"

#ifdef WIN32
#undef GetObject
#endif

#ifndef __GMLUAMODULE_H__
#define __GMLUAMODULE_H__

typedef struct lua_State lua_State;
typedef int (*lua_CFunction) (lua_State *L);

extern ILuaInterface* g_Lua;

// You should place this at the top of your module

#ifdef WIN32
#define GMOD_EXPORT __declspec(dllexport)
#else
#define GMOD_EXPORT __attribute__ ((visibility("default")))
#endif

#define GMOD_MODULE( _startfunction_, _closefunction_ ) \
	ILuaInterface* g_Lua = NULL;						\
	int _startfunction_( void );						\
	int _closefunction_( void );						\
	extern "C" int GMOD_EXPORT gmod_open( ILuaInterface* i ) \
	{													\
		g_Lua = i;										\
		return _startfunction_();						\
	}													\
	extern "C" int GMOD_EXPORT gmod_close( int i )		\
	{													\
		g_Lua = NULL;									\
		_closefunction_();								\
		return 0;										\
	}

static void Msg( char *format, ... )
{
	if (!g_Lua) return;

	va_list		argptr;
	char		string[4096];
	va_start ( argptr, format );
	vsprintf_s( string, 4096, format, argptr );
	va_end ( argptr );

	ILuaObject* msg = g_Lua->GetGlobal( "Msg" );
	if ( !msg || !msg->isFunction() ) return;

	msg->Push();
	g_Lua->Push( string );
	g_Lua->Call( 1 );
}

#define LUA_FUNCTION( _function_ ) static int _function_( lua_State* )

#endif // __GMLUAMODULE_H__