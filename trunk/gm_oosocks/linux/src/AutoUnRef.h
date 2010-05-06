#include "GMLuaModule.h"

#ifndef __AUTOUNREF_H__
#define __AUTOUNREF_H__

class AutoUnRef
{
public:
	AutoUnRef(ILuaObject *obj)
	{
		m_obj = obj;
	};

	~AutoUnRef()
	{
		if(!m_obj)
			return;

		m_obj->UnReference();
	};

	ILuaObject* operator -> () const
	{
		return m_obj;
	};

	operator ILuaObject*()
	{
		return m_obj;
	};

	operator ILuaObject*() const
	{
		return m_obj;
	};

	const AutoUnRef& operator=(const ILuaObject *obj)
	{
		m_obj = (ILuaObject *)obj;
	}

private:
	ILuaObject *m_obj;
};

#endif // __AUTOUNREF_H__