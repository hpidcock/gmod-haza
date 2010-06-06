#include "GMLuaModule.h"
#include "AutoUnRef.h"

#undef _UNICODE

#include "interface.h"
#include "eiface.h"
#include "vphysics_interface.h"

//#include "tier0/memdbgon.h"

GMOD_MODULE(open, close);

static IPhysicsCollision *physcol = NULL;

struct Triangle
{
	Vector a;
	Vector b;
	Vector c;
};

ILuaObject *MakeVector(Vector &vec)
{
	AutoUnRef vecCreate = g_Lua->GetGlobal("Vector");
	
	vecCreate->Push();
	
	g_Lua->Push(vec.x);
	g_Lua->Push(vec.y);
	g_Lua->Push(vec.z);
	
	g_Lua->Call(3, 1);

	return g_Lua->GetReturn(0);

	// I would use Vector as userdata, but it crashes ^.^
	/*
	AutoUnRef vecMeta = g_Lua->GetMetaTable("Vector", GLua::TYPE_VECTOR);

	g_Lua->PushUserData(vecMeta, new Vector(vec));

	ILuaObject *obj = g_Lua->GetObject();

	g_Lua->Pop();

	return obj;
	*/
}

LUA_FUNCTION(GetMesh)
{
	g_Lua->CheckType(1, GLua::TYPE_PHYSOBJ);

	IPhysicsObject *physObj = (IPhysicsObject *)g_Lua->GetUserData(1);
	
	if(!physObj)
	{
		return 0;
	}

	const CPhysCollide *collide = physObj->GetCollide();

	if(!collide)
	{
		return 0;
	}

	ICollisionQuery *queryModel = physcol->CreateQueryModel(const_cast<CPhysCollide *>(collide));

	if(queryModel)
	{
		int convexCount = queryModel->ConvexCount();
		int triangleCount = 0;

		for(int i = 0; i < convexCount; i++)
		{
			triangleCount += queryModel->TriangleCount(i);
		}

		Triangle *triangles = (Triangle *)malloc(triangleCount * sizeof(Triangle));

		int tri = 0;

		for(int i = 0; i < convexCount; i++)
		{
			int triangleItorCount = queryModel->TriangleCount(i);
			for(int j = 0; j < triangleItorCount; j++)
			{
				queryModel->GetTriangleVerts(i, j, (Vector *)&triangles[tri]);
				tri++;
			}
		}
		
		physcol->DestroyQueryModel(queryModel);

		AutoUnRef retTable = g_Lua->GetNewTable();

		int index = 1;
		for(int i = 0; i < triangleCount; i++)
		{
			AutoUnRef tbl = g_Lua->GetNewTable();

			Triangle &triangle = triangles[i];

			AutoUnRef objA = MakeVector(triangle.a);
			AutoUnRef objB = MakeVector(triangle.b);
			AutoUnRef objC = MakeVector(triangle.c);

			tbl->SetMember(1, objA);
			tbl->SetMember(2, objB);
			tbl->SetMember(3, objC);

			retTable->SetMember(index, tbl);

			index++;
		}

		retTable->Push();

		free(triangles);
		return 1;
	}

	return 0;
};

int open()
{
	CreateInterfaceFn phys = Sys_GetFactory("vphysics.dll");

	physcol = (IPhysicsCollision *)phys(VPHYSICS_COLLISION_INTERFACE_VERSION, NULL);

	if(!physcol)
	{
		Msg("Could not get IPhysicsCollision\n");
		return 0;
	}

	AutoUnRef physMeta = g_Lua->GetMetaTable("PhysObj", GLua::TYPE_PHYSOBJ);
	physMeta->SetMember("GetMesh", GetMesh);

	return 0;
}

int close()
{
	return 0;
}