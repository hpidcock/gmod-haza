// Authour: Haza55
const_ServerIP = "192.168.1.3";
const_BindPort = 27080;

PIPE_SPEEDTEST		= 1;
PIPE_NETWORKVAR 	= 2;
PIPE_USERMESSAGE 	= 3;
PIPE_DATASTREAM 	= 4;

PIPE_TYPES = {};
PIPE_TYPES[PIPE_SPEEDTEST] = "PIPE_SPEEDTEST";
PIPE_TYPES[PIPE_NETWORKVAR] = "PIPE_NETWORKVAR";
PIPE_TYPES[PIPE_USERMESSAGE] = "PIPE_USERMESSAGE";
PIPE_TYPES[PIPE_DATASTREAM] = "PIPE_DATASTREAM";

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

local meta = FindMetaTable("Entity");

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

	local ei = self:EntIndex();
	PIPE.NetVar.Vars[ei] = PIPE.NetVar.Vars[ei] or {};
	PIPE.NetVar.Proxies[ei] = PIPE.NetVar.Proxies[ei] or {};
	
	if(PIPE.NetVar.Vars[ei][var] == val && type(val) != "table") then return; end
	
	if(type(PIPE.NetVar.Proxies[ei][var]) == "function") then
		pcall(PIPE.NetVar.Proxies[ei][var], self, var, PIPE.NetVar.Vars[ei][var], val);
	end
	
	PIPE.NetVar.Vars[ei][var] = val;
	if SERVER then
	PIPE.NetVar.Changes[ei] = PIPE.NetVar.Changes[ei] or {};
	PIPE.NetVar.Changes[ei][var] = val; 
	PIPE.NetVar.ChangesMade = true;
	end
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
	return string.Replace(tostring(self:GetNetworkedVar(var)), "|n|", "\n");
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
	return string.Replace(tostring(self:GetNetworkedVar(var)), "|n|", "\n");
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
	self:SetNetworkedVar(var, string.Replace(tostring(val), "\n", "|n|"));
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
	self:SetNetworkedVar(var, string.Replace(tostring(val), "\n", "|n|"));
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
