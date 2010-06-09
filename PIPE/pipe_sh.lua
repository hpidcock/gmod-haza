// Authour: Haza55
include("libcompress.lua")
if SERVER then
	AddCSLuaFile("libcompress.lua")
end

if SERVER then
	const_ServerIP = GetConVar("ip"):GetString();
	if(const_ServerIP == "localhost") then
		const_ServerIP = "127.0.0.1";
	end
else
	const_ServerIP = "127.0.0.1";
end
const_BindPort = 27085;

PIPE_SPEEDTEST			= 1;
PIPE_NETWORKVAR 		= 2;
PIPE_USERMESSAGE 		= 3;
PIPE_DATASTREAM 		= 4;
PIPE_NETWORKVAR_FULL	= 6;
PIPE_PING				= 7;

PIPE_CL_NETVAR_REQ		= 1;

PIPE_TYPES = {};
PIPE_TYPES[PIPE_SPEEDTEST] = "PIPE_SPEEDTEST";
PIPE_TYPES[PIPE_NETWORKVAR] = "PIPE_NETWORKVAR";
PIPE_TYPES[PIPE_USERMESSAGE] = "PIPE_USERMESSAGE";
PIPE_TYPES[PIPE_DATASTREAM] = "PIPE_DATASTREAM";
PIPE_TYPES[PIPE_NETWORKVAR_FULL] = "PIPE_NETWORKVAR_FULL";
PIPE_TYPES[PIPE_PING] = "PIPE_PING";

PIPE_VALUE_NIL = "\24\27";

PIPE = {};
PIPE.Net = {};
PIPE.NetVar = {};

///////////////////////////////////////////////////////////////////////////////////////////
// Datastreams
///////////////////////////////////////////////////////////////////////////////////////////

// TODO

///////////////////////////////////////////////////////////////////////////////////////////
// User Messages
///////////////////////////////////////////////////////////////////////////////////////////

// TODO

///////////////////////////////////////////////////////////////////////////////////////////
// Networked Varibles
///////////////////////////////////////////////////////////////////////////////////////////
PIPE.NetVar.Vars = {};
if SERVER then
	PIPE.NetVar.Changes = {}; 
	PIPE.NetVar.ChangesMade = false;
end
PIPE.NetVar.Proxies = {};

function PIPE.NetVar.DeepTableCompare(old, new)
	if(new == nil) then
		return {};
	elseif(old == nil) then
		return new;
	end

	local changes = {};
	for k, v in pairs(old) do
		if(v != new[k]) then
			if(new[k] == nil) then
				changes[k] = PIPE_VALUE_NIL;
			else
				changes[k] = new[k];
			end
		end
		if(type(v) == "table" && type(new[k]) == "table") then
			changes[k] = PIPE.NetVar.DeepTableCompare(v, new[k]);
		end
	end
	
	for k, v in pairs(new) do
		if(old[k] == nil) then
			changes[k] = new[k];
		end
	end
	
	return changes;
end

local meta = FindMetaTable("Entity");

if(CLIENT) then

function meta:RequestVarUpdate()
	PIPE.NetVar.RequestUpdate({self:EntIndex()});
end

end

function meta:GetNetworkedVar(var)
	if(type(var) == "string") then
		var = string.lower(var);
	end

	local ei = self:EntIndex();
	PIPE.NetVar.Vars[ei] = PIPE.NetVar.Vars[ei] or {};
	
	return PIPE.NetVar.Vars[ei][var];
end

function meta:SetNetworkedVar(var, val)
	if(type(var) == "string") then
		var = string.lower(var);
	end
	
	if(type(val) == "number") then
		// round to 1 decimal places.
		val = math.Round(val * 10)/10;
	end
	
	if(IsEntity(val)) then
		if(ValidEntity(val)) then
			val = val:EntIndex();
		else
			val = -1;
		end
	end
	
	if(type(val) == "table") then
		val = table.Copy(val);
	end

	local ei = self:EntIndex();
	PIPE.NetVar.Vars[ei] = PIPE.NetVar.Vars[ei] or {};
	PIPE.NetVar.Proxies[ei] = PIPE.NetVar.Proxies[ei] or {};
	
	if(PIPE.NetVar.Vars[ei][var] == val && type(val) != "table") then return; end
	
	if(type(PIPE.NetVar.Proxies[ei][var]) == "function") then
		pcall(PIPE.NetVar.Proxies[ei][var], self, var, PIPE.NetVar.Vars[ei][var], val);
	end
	
	if SERVER then
		local changes = val;
		
		if(type(val) == "table") then
			changes = PIPE.NetVar.DeepTableCompare(PIPE.NetVar.Vars[ei][var], val);
		end
		
		if(val == nil) then
			changes = PIPE_VALUE_NIL;
		end
		
		PIPE.NetVar.Changes[ei] = PIPE.NetVar.Changes[ei] or {};
		PIPE.NetVar.Changes[ei][var] = changes;
		PIPE.NetVar.ChangesMade = true;
	end
	
	PIPE.NetVar.Vars[ei][var] = val;
end

function meta:SetNetworkedVarProxy(var, func)
	if(type(var) == "string") then
		var = string.lower(var);
	end

	local ei = self:EntIndex();
	PIPE.NetVar.Proxies[ei] = PIPE.NetVar.Proxies[ei] or {};
	PIPE.NetVar.Proxies[ei][var] = func;
end

function meta:GetNetworkedAngle(var, def)
	if(def == nil) then def = Angle(0, 0, 0); end
	return self:GetNetworkedVar(var) or def;
end

function meta:GetNetworkedBool(var, def)
	if(def == nil) then def = false; end
	return tobool(self:GetNetworkedVar(var)) or def;
end

function meta:GetNetworkedEntity(var, def)
	if(def == nil) then def = Entity(-1); end
	return Entity(self:GetNetworkedVar(var)) or def;
end

function meta:GetNetworkedFloat(var, def)
	if(def == nil) then def = 0; end
	return tonumber(self:GetNetworkedVar(var)) or def;
end

function meta:GetNetworkedInt(var, def)
	if(def == nil) then def = 0; end
	return tonumber(self:GetNetworkedVar(var)) or def;
end

function meta:GetNetworkedString(var, def)
	if(def == nil) then def = ""; end
	// Correct the string.
	local str = self:GetNetworkedVar(var);
	if(!str) then
		return def;
	end
	return str;
end

function meta:GetNetworkedVector(var, def)
	if(def == nil) then def = Vector(0, 0, 0); end
	return self:GetNetworkedVar(var) or def;
end

function meta:GetNWAngle(var, def)
	if(def == nil) then def = Angle(0, 0, 0); end
	return self:GetNetworkedVar(var) or def;
end

function meta:GetNWBool(var, def)
	if(def == nil) then def = false; end
	return tobool(self:GetNetworkedVar(var)) or def;
end

function meta:GetNWEntity(var, def)
	if(def == nil) then def = Entity(-1); end
	return Entity(self:GetNetworkedVar(var)) or def;
end

function meta:GetNWFloat(var, def)
	if(def == nil) then def = 0; end
	return tonumber(self:GetNetworkedVar(var)) or def;
end

function meta:GetNWInt(var, def)
	if(def == nil) then def = 0; end
	return tonumber(self:GetNetworkedVar(var)) or def;
end

function meta:GetNWString(var, def)
	if(def == nil) then def = ""; end
	// Correct the string.
	local str = self:GetNetworkedVar(var);
	if(!str) then
		return def;
	end
	return str;
end

function meta:GetNWVector(var, def)
	if(def == nil) then def = Vector(0, 0, 0); end
	return self:GetNetworkedVar(var) or def;
end

function meta:SetNetworkedAngle(var, val)
	self:SetNetworkedVar(var, val);
end

function meta:SetNetworkedBool(var, val)
	self:SetNetworkedVar(var, tobool(val));
end

function meta:SetNetworkedEntity(var, val)
	self:SetNetworkedVar(var, val or Entity(-1));
end

function meta:SetNetworkedFloat(var, val)
	self:SetNetworkedVar(var, tonumber(val));
end

function meta:SetNetworkedInt(var, val)
	self:SetNetworkedVar(var, tonumber(val));
end

function meta:SetNetworkedNumber(var, val)
	self:SetNetworkedVar(var, tonumber(val));
end

function meta:SetNetworkedString(var, val)
	// Make the string socket safe.
	if(!val) then
		self:SetNetworkedVar(var, nil);
		return;
	end
	self:SetNetworkedVar(var, val);
end

function meta:SetNetworkedVector(var, val)
	self:SetNetworkedVar(var, val);
end

function meta:SetNWAngle(var, val)
	self:SetNetworkedVar(var, val);
end

function meta:SetNWBool(var, val)
	self:SetNetworkedVar(var, tobool(val));
end

function meta:SetNWEntity(var, val)
	self:SetNetworkedVar(var, val or Entity(-1));
end

function meta:SetNWFloat(var, val)
	self:SetNetworkedVar(var, tonumber(val));
end

function meta:SetNWInt(var, val)
	self:SetNetworkedVar(var, tonumber(val));
end

function meta:SetNWString(var, val)
	// Make the string socket safe.
	if(!val) then
		self:SetNetworkedVar(var, nil);
		return;
	end
	self:SetNetworkedVar(var, val);
end

function meta:SetNWVector(var, val)
	self:SetNetworkedVar(var, val);
end

// Global Vars - They are just world netvars
function SetGlobalVar(var, val)
	Entity(0):SetNetworkedVar(var, val);
end

function GetGlobalVar(var)
	return Entity(0):GetNetworkedVar(var);
end

function GetGlobalAngle(var, def)
	return Entity(0):GetNetworkedAngle(var, def);
end

function GetGlobalBool(var, def)
	return Entity(0):GetNetworkedBool(var, def);
end

function GetGlobalEntity(var, def)
	return Entity(0):GetNetworkedEntity(var, def);
end

function GetGlobalFloat(var, def)
	return Entity(0):GetNetworkedFloat(var, def);
end

function GetGlobalInt(var, def)
	return Entity(0):GetNetworkedInt(var, def);
end

function GetGlobalString(var, def)
	return Entity(0):GetNetworkedString(var, def);
end

function GetGlobalVector(var, def)
	return Entity(0):GetNetworkedVector(var, def);
end

function SetGlobalAngle(var, val)
	Entity(0):SetNetworkedAngle(var, val);
end

function SetGlobalBool(var, val)
	Entity(0):SetNetworkedBool(var, val);
end

function SetGlobalEntity(var, val)
	Entity(0):SetNetworkedEntity(var, val);
end

function SetGlobalFloat(var, val)
	Entity(0):SetNetworkedFloat(var, val);
end

function SetGlobalInt(var, val)
	Entity(0):SetNetworkedInt(var, val);
end

function SetGlobalString(var, val)
	Entity(0):SetNetworkedString(var, val);
end

function SetGlobalVector(var, val)
	Entity(0):SetNetworkedVector(var, val);
end
