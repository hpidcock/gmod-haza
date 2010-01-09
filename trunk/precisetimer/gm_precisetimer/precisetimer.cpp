#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../common/GMLuaModule.h"

#define CPreciseTimer_T 9700

class CPreciseTimer
{
public:
	CPreciseTimer()
	{
		Reset();
	};

	void Reset()
	{
		QueryPerformanceFrequency(&m_iFrequency);
		QueryPerformanceCounter(&m_iStart);
	};

	double Time()
	{
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);

		return (double)((end.QuadPart - m_iStart.QuadPart) / (double)m_iFrequency.QuadPart);
	};

private:
	LARGE_INTEGER m_iFrequency;
	LARGE_INTEGER m_iStart;
};

GMOD_MODULE(Init, Shutdown);

static ILuaInterface *G = NULL;

LUA_FUNCTION(CPreciseTimer_New)
{
	CPreciseTimer *timer = new CPreciseTimer();

	ILuaObject *mt = G->GetMetaTable("CPreciseTimer", CPreciseTimer_T);
	G->PushUserData(mt, timer);
	mt->UnReference();

	return 1;
}

LUA_FUNCTION(CPreciseTimer_Reset)
{
	G->CheckType(1, CPreciseTimer_T);

	CPreciseTimer *timer = (CPreciseTimer *)G->GetUserData(1);
	timer->Reset();

	return 0;
}

LUA_FUNCTION(CPreciseTimer_Time)
{
	G->CheckType(1, CPreciseTimer_T);
	
	CPreciseTimer *timer = (CPreciseTimer *)G->GetUserData(1);
	G->PushDouble(timer->Time());

	return 1;
}

int Init(lua_State* L)
{
	G = Lua();

	ILuaObject *mt = G->GetMetaTable("CPreciseTimer", CPreciseTimer_T);
		ILuaObject *__index = G->GetNewTable();
			__index->SetMember("Start", CPreciseTimer_Reset);
			__index->SetMember("Reset", CPreciseTimer_Reset);
			__index->SetMember("Time", CPreciseTimer_Time);
		mt->SetMember("__index", __index);
		__index->UnReference();
	mt->UnReference();

	G->SetGlobal("CPreciseTimer", CPreciseTimer_New);

	return 0;
}

int Shutdown(lua_State* L)
{
	return 0;
}

