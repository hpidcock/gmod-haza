#undef _UNICODE

#include "GMLuaModule.h"
#include "AutoUnRef.h"

#include "interface.h"
#include "eiface.h"
#include "vphysics_interface.h"

#define GAME_DLL
#include "cbase.h"

#include "vcollide_parse.h"

GMOD_MODULE(open, close);

static IPhysicsCollision *physcol = NULL;
static IPhysicsEnvironment *physenv = NULL;
static IPhysics *physics = NULL;

struct Triangle
{
	Vector a;
	Vector b;
	Vector c;
};

struct TriangleP
{
	Vector *a;
	Vector *b;
	Vector *c;
};

size_t UTIL_DataFromName( datamap_t *pMap, const char *pName )
{
	while ( pMap )
	{
		for ( int i = 0; i < pMap->dataNumFields; i++ )
		{
			if(!pMap->dataDesc[i].fieldName || pMap->dataDesc[i].fieldType == FIELD_VOID)
				continue;
			if ( FStrEq( pName, pMap->dataDesc[i].fieldName ) )
			{
				return pMap->dataDesc[i].fieldOffset[0];
			}
		}
		pMap = pMap->baseMap;
	}

	g_Lua->Error("UTIL_DataFromName -> Could not find field.\n");

	return NULL;
}

fieldtype_t UTIL_FieldTypeFromName( datamap_t *pMap, const char *pName )
{
	while ( pMap )
	{
		for ( int i = 0; i < pMap->dataNumFields; i++ )
		{
			if(!pMap->dataDesc[i].fieldName || pMap->dataDesc[i].fieldType == FIELD_VOID)
				continue;
			if ( FStrEq( pName, pMap->dataDesc[i].fieldName ) )
			{
				return pMap->dataDesc[i].fieldType;
			}
		}
		pMap = pMap->baseMap;
	}

	g_Lua->Error("UTIL_FieldTypeFromName -> Could not find field.\n");

	return FIELD_VOID;
}

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

LUA_FUNCTION(GetConvexCount)
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
		g_Lua->Push((float)queryModel->ConvexCount());
		physcol->DestroyQueryModel(queryModel);
		return 1;
	}

	return 0;
};

LUA_FUNCTION(GetConvexMesh)
{
	g_Lua->CheckType(1, GLua::TYPE_PHYSOBJ);
	g_Lua->CheckType(2, GLua::TYPE_NUMBER);

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
		int convex = g_Lua->GetInteger(2);

		if(convex >= convexCount)
			return 0;

		int triangleCount = queryModel->TriangleCount(convex);

		Triangle *triangles = (Triangle *)malloc(triangleCount * sizeof(Triangle));

		for(int i = 0; i < triangleCount; i++)
		{
			queryModel->GetTriangleVerts(convex, i, (Vector *)&triangles[i]);
		}

		physcol->DestroyQueryModel(queryModel);

		AutoUnRef retTable = g_Lua->GetNewTable();

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

			retTable->SetMember(i+1, tbl);
		}

		retTable->Push();

		free(triangles);
		return 1;
	}

	return 0;
};

LUA_FUNCTION(RebuildFromConvexs)
{
	g_Lua->CheckType(1, GLua::TYPE_PHYSOBJ);
	g_Lua->CheckType(2, GLua::TYPE_VECTOR);
	g_Lua->CheckType(3, GLua::TYPE_ANGLE);
	g_Lua->CheckType(4, GLua::TYPE_NUMBER);
	g_Lua->CheckType(5, GLua::TYPE_NUMBER);
	g_Lua->CheckType(6, GLua::TYPE_NUMBER);
	g_Lua->CheckType(7, GLua::TYPE_NUMBER);
	g_Lua->CheckType(8, GLua::TYPE_NUMBER);
	g_Lua->CheckType(9, GLua::TYPE_TABLE);
	
	CUtlLuaVector *table = g_Lua->GetAllTableMembers(9);

	int convexCount = table->Count();
	CPhysConvex **convexList = (CPhysConvex **)malloc(sizeof(CPhysConvex *) * convexCount);

	for(int i = 0; i < table->Count(); i++)
	{
		LuaKeyValue &kv = table->Element(i);

		g_Lua->Push(kv.pValue);

		if(g_Lua->GetType(-1) != GLua::TYPE_TABLE)
		{
			g_Lua->Pop();
			g_Lua->DeleteLuaVector(table);
			g_Lua->Error("Convex is not a table.");
			return 0;
		}

		CUtlLuaVector *convexTable = g_Lua->GetAllTableMembers(-1);
		g_Lua->Pop();

		Vector **pointCloud = (Vector **)malloc(convexTable->Count() * sizeof(Vector *) * 3);

		int v = 0;

		for(int j = 0; j < convexTable->Count(); j++)
		{
			LuaKeyValue &ckv = convexTable->Element(j);
			AutoUnRef a = ckv.pValue->GetMember(1);
			AutoUnRef b = ckv.pValue->GetMember(2);
			AutoUnRef c = ckv.pValue->GetMember(3);

			if(a->GetType() != GLua::TYPE_VECTOR ||
				b->GetType() != GLua::TYPE_VECTOR ||
				c->GetType() != GLua::TYPE_VECTOR)
			{
				g_Lua->DeleteLuaVector(convexTable);
				g_Lua->DeleteLuaVector(table);
				free(pointCloud);
				g_Lua->Error("Triangle contains non vector. Or incorrect order.");
				return 0;
			}

			pointCloud[v++] = (Vector *)a->GetUserData();
			pointCloud[v++] = (Vector *)b->GetUserData();
			pointCloud[v++] = (Vector *)c->GetUserData();
		}

		convexList[i] = physcol->ConvexFromVerts(pointCloud, convexTable->Count() * 3);

		free(pointCloud);

		g_Lua->DeleteLuaVector(convexTable);
	}

	g_Lua->DeleteLuaVector(table);

	CPhysCollide *collide = physcol->ConvertConvexToCollide(convexList, convexCount);
	
	free(convexList);

	objectparams_t params;
	memset(&params, 0, sizeof(objectparams_t));
	params.volume = physcol->CollideVolume(collide);
	params.mass = g_Lua->GetNumber(4);
	params.inertia = g_Lua->GetNumber(7);
	params.enableCollisions = true;
	params.damping = g_Lua->GetNumber(5);
	params.rotInertiaLimit = g_Lua->GetNumber(8);
	params.rotdamping = g_Lua->GetNumber(6);

	Vector pos = *((Vector *)g_Lua->GetUserData(2));
	QAngle ang = *((QAngle *)g_Lua->GetUserData(3));

	IPhysicsObject *newObj = physenv->CreatePolyObject(collide, 0, pos, ang, &params);

	IPhysicsObject *old = (IPhysicsObject *)g_Lua->GetUserData(1);

	CBaseEntity *ent = (CBaseEntity *)old->GetGameData();

	newObj->SetGameData(ent);

	datamap_t *data = ent->GetDataDescMap();

	size_t offset = UTIL_DataFromName(data, "m_pPhysicsObject");

	IPhysicsObject **physObject = (IPhysicsObject **)((size_t)ent + offset);

	if(*physObject)
	{
		physenv->DestroyObject(*physObject);
	}

	*physObject = newObj;

	return 0;
};

int open()
{
	CreateInterfaceFn phys = Sys_GetFactory("vphysics.dll");

	physics = (IPhysics *)phys(VPHYSICS_INTERFACE_VERSION, NULL);
	physcol = (IPhysicsCollision *)phys(VPHYSICS_COLLISION_INTERFACE_VERSION, NULL);

	if(!physcol)
	{
		Msg("Could not get IPhysicsCollision\n");
		return 0;
	}

	if(!physics)
	{
		Msg("Could not get IPhysics\n");
		return 0;
	}

	physenv = physics->GetActiveEnvironmentByIndex(0);

	if(!physenv)
	{
		Msg("Could not get IPhysicsEnvironment\n");
		return 0;
	}

	AutoUnRef physMeta = g_Lua->GetMetaTable("PhysObj", GLua::TYPE_PHYSOBJ);
	physMeta->SetMember("GetConvexMesh", GetConvexMesh);
	physMeta->SetMember("GetConvexCount", GetConvexCount);
	physMeta->SetMember("RebuildFromConvexs", RebuildFromConvexs);

	return 0;
};

int close()
{
	return 0;
};