#undef _UNICODE

#include "GMLuaModule.h"
#include "CAutoUnRef.h"

#include "interface.h"
#include "eiface.h"
#include "vphysics_interface.h"

#define GAME_DLL
#include "cbase.h"

#include "vcollide_parse.h"

GMOD_MODULE(open, close);

static IPhysicsCollision *physcollision = NULL;
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

	return FIELD_VOID;
}

ILuaObject *MakeVector(lua_State *L, Vector &vec)
{
	CAutoUnRef vecCreate = Lua()->GetGlobal("Vector");
	
	vecCreate->Push();
	
	Lua()->Push(vec.x);
	Lua()->Push(vec.y);
	Lua()->Push(vec.z);
	
	Lua()->Call(3, 1);

	return Lua()->GetReturn(0);

	// I would use Vector as userdata, but it crashes ^.^
	/*
	CAutoUnRef vecMeta = Lua()->GetMetaTable("Vector", GLua::TYPE_VECTOR);

	Lua()->PushUserData(vecMeta, new Vector(vec));

	ILuaObject *obj = Lua()->GetObject();

	Lua()->Pop();

	return obj;
	*/
}

LUA_FUNCTION(GetConvexCount)
{
	Lua()->CheckType(1, GLua::TYPE_PHYSOBJ);

	IPhysicsObject *physObj = (IPhysicsObject *)Lua()->GetUserData(1);
	
	if(!physObj)
	{
		return 0;
	}

	const CPhysCollide *collide = physObj->GetCollide();

	if(!collide)
	{
		return 0;
	}

	ICollisionQuery *queryModel = physcollision->CreateQueryModel(const_cast<CPhysCollide *>(collide));

	if(queryModel)
	{
		Lua()->Push((float)queryModel->ConvexCount());
		physcollision->DestroyQueryModel(queryModel);
		return 1;
	}

	return 0;
};

LUA_FUNCTION(GetConvexMesh)
{
	Lua()->CheckType(1, GLua::TYPE_PHYSOBJ);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	IPhysicsObject *physObj = (IPhysicsObject *)Lua()->GetUserData(1);
	
	if(!physObj)
	{
		return 0;
	}

	const CPhysCollide *collide = physObj->GetCollide();

	if(!collide)
	{
		return 0;
	}

	ICollisionQuery *queryModel = physcollision->CreateQueryModel(const_cast<CPhysCollide *>(collide));

	if(queryModel)
	{
		int convexCount = queryModel->ConvexCount();
		int convex = Lua()->GetInteger(2);

		if(convex >= convexCount)
			return 0;

		int triangleCount = queryModel->TriangleCount(convex);

		Triangle *triangles = (Triangle *)malloc(triangleCount * sizeof(Triangle));

		for(int i = 0; i < triangleCount; i++)
		{
			queryModel->GetTriangleVerts(convex, i, (Vector *)&triangles[i]);
		}

		physcollision->DestroyQueryModel(queryModel);

		CAutoUnRef retTable = Lua()->GetNewTable();

		for(int i = 0; i < triangleCount; i++)
		{
			CAutoUnRef tbl = Lua()->GetNewTable();

			Triangle &triangle = triangles[i];

			CAutoUnRef objA = MakeVector(L, triangle.a);
			CAutoUnRef objB = MakeVector(L, triangle.b);
			CAutoUnRef objC = MakeVector(L, triangle.c);

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
	Lua()->CheckType(1, GLua::TYPE_PHYSOBJ);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_ANGLE);
	Lua()->CheckType(4, GLua::TYPE_NUMBER);
	Lua()->CheckType(5, GLua::TYPE_NUMBER);
	Lua()->CheckType(6, GLua::TYPE_NUMBER);
	Lua()->CheckType(7, GLua::TYPE_NUMBER);
	Lua()->CheckType(8, GLua::TYPE_NUMBER);
	Lua()->CheckType(9, GLua::TYPE_TABLE);
	
	CUtlLuaVector *table = Lua()->GetAllTableMembers(9);

	int convexCount = table->Count();
	CPhysConvex **convexList = (CPhysConvex **)malloc(sizeof(CPhysConvex *) * convexCount);

	for(int i = 0; i < table->Count(); i++)
	{
		LuaKeyValue &kv = table->Element(i);

		Lua()->Push(kv.pValue);

		if(Lua()->GetType(-1) != GLua::TYPE_TABLE)
		{
			Lua()->Pop();
			Lua()->DeleteLuaVector(table);
			Lua()->Error("PhysObj:RebuildFromConvexs convex is not a table.\n");
			return 0;
		}

		CUtlLuaVector *convexTable = Lua()->GetAllTableMembers(-1);
		Lua()->Pop();

		Vector **pointCloud = (Vector **)malloc(convexTable->Count() * sizeof(Vector *) * 3);

		int v = 0;

		for(int j = 0; j < convexTable->Count(); j++)
		{
			LuaKeyValue &ckv = convexTable->Element(j);
			CAutoUnRef a = ckv.pValue->GetMember(1);
			CAutoUnRef b = ckv.pValue->GetMember(2);
			CAutoUnRef c = ckv.pValue->GetMember(3);

			if(a->GetType() != GLua::TYPE_VECTOR ||
				b->GetType() != GLua::TYPE_VECTOR ||
				c->GetType() != GLua::TYPE_VECTOR)
			{
				Lua()->DeleteLuaVector(convexTable);
				Lua()->DeleteLuaVector(table);
				free(pointCloud);
				Lua()->Error("PhysObj:RebuildFromConvexs got a Triangle that contains a non vector. Or incorrect order.\n");
				return 0;
			}

			pointCloud[v++] = (Vector *)a->GetUserData();
			pointCloud[v++] = (Vector *)b->GetUserData();
			pointCloud[v++] = (Vector *)c->GetUserData();

			if(pointCloud[v-1] == pointCloud[v-2] ||
				pointCloud[v-2] == pointCloud[v-3] ||
				pointCloud[v-3] == pointCloud[v-1])
			{
				Lua()->DeleteLuaVector(convexTable);
				Lua()->DeleteLuaVector(table);
				free(pointCloud);
				Lua()->Error("PhysObj:RebuildFromConvexs got a Triangle that contains equal vertices.\n");
				return 0;
			}
		}

		convexList[i] = physcollision->ConvexFromVerts(pointCloud, convexTable->Count() * 3);

		free(pointCloud);

		Lua()->DeleteLuaVector(convexTable);
	}

	Lua()->DeleteLuaVector(table);

	CPhysCollide *collide = physcollision->ConvertConvexToCollide(convexList, convexCount);
	
	free(convexList);

	objectparams_t params;
	memset(&params, 0, sizeof(objectparams_t));
	params.volume = physcollision->CollideVolume(collide);
	params.mass = Lua()->GetNumber(4);
	params.inertia = Lua()->GetNumber(7);
	params.enableCollisions = true;
	params.damping = Lua()->GetNumber(5);
	params.rotInertiaLimit = Lua()->GetNumber(8);
	params.rotdamping = Lua()->GetNumber(6);

	Vector pos = *((Vector *)Lua()->GetUserData(2));
	QAngle ang = *((QAngle *)Lua()->GetUserData(3));

	IPhysicsObject *newObj = physenv->CreatePolyObject(collide, 0, pos, ang, &params);

	IPhysicsObject *old = (IPhysicsObject *)Lua()->GetUserData(1);

	CBaseEntity *ent = (CBaseEntity *)old->GetGameData();

	newObj->SetGameData(ent);

	datamap_t *data = ent->GetDataDescMap();

	size_t offset = UTIL_DataFromName(data, "m_pPhysicsObject");

	if(offset == NULL)
	{
		physenv->DestroyObject(newObj);
		Lua()->Error("PhysObj:RebuildFromConvexs got an Invalid entity.\n");
		return 0;
	}

	IPhysicsObject **physObject = (IPhysicsObject **)((size_t)ent + offset);

	if(*physObject)
	{
		physenv->DestroyObject(*physObject);
	}

	*physObject = newObj;

	return 0;
};

int open(lua_State *L)
{
	CreateInterfaceFn phys = Sys_GetFactory("vphysics.dll");

	physics = (IPhysics *)phys(VPHYSICS_INTERFACE_VERSION, NULL);
	physcollision = (IPhysicsCollision *)phys(VPHYSICS_COLLISION_INTERFACE_VERSION, NULL);

	if(!physcollision)
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

	CAutoUnRef physMeta = Lua()->GetMetaTable("PhysObj", GLua::TYPE_PHYSOBJ);
	physMeta->SetMember("GetConvexMesh", GetConvexMesh);
	physMeta->SetMember("GetConvexCount", GetConvexCount);
	physMeta->SetMember("RebuildFromConvexs", RebuildFromConvexs);

	return 0;
};

int close(lua_State *L)
{
	return 0;
};