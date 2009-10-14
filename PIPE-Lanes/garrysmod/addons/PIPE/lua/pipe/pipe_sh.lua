// Authour: Haza55

const_ServerIP = "192.168.1.3";
const_BindPort = 27080;

g_NetVars = {};
if SERVER then
g_NetVarsChanged = {}; 
g_NetVarsAnyChanges = false;
end
g_NetVarsProxies = {};

local meta = FindMetaTable("Entity");

function meta:GetNetworkedVar(var)
	if(type(var) == "string") then
		var = string.lower(var);
	end

	local ei = self:EntIndex();
	g_NetVars[ei] = g_NetVars[ei] or {};
	
	return g_NetVars[ei][var];
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
	g_NetVars[ei] = g_NetVars[ei] or {};
	g_NetVarsProxies[self] = g_NetVarsProxies[self] or {};
	
	if(g_NetVars[ei][var] == val) then return; end
	
	if(type(g_NetVarsProxies[self][var]) == "function") then
		pcall(g_NetVarsProxies[self][var], self, var, g_NetVars[ei][var], val);
	end
	
	g_NetVars[ei][var] = val;
	if SERVER then
	g_NetVarsChanged[ei] = g_NetVarsChanged[ei] or {};
	g_NetVarsChanged[ei][var] = val; 
	g_NetVarsAnyChanges = true;
	end
end

function meta:SetNetworkedVarProxy(var, func)
	if(type(var) == "string") then
		var = string.lower(var);
	end

	g_NetVarsProxies[self] = g_NetVarsProxies[self] or {};
	g_NetVarsProxies[self][var] = func;
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
	return string.Replace(tostring(self:GetNetworkedVar(var)), "|n|", "\n") or def;
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
	return string.Replace(tostring(self:GetNetworkedVar(var)), "|n|", "\n") or def;
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

function GetGlobalAngle(var)
	return Entity(0):GetNetworkedAngle(var);
end

function GetGlobalBool(var)
	return Entity(0):GetNetworkedBool(var);
end

function GetGlobalEntity(var)
	return Entity(0):GetNetworkedEntity(var);
end

function GetGlobalFloat(var)
	return Entity(0):GetNetworkedFloat(var);
end

function GetGlobalInt(var)
	return Entity(0):GetNetworkedInt(var);
end

function GetGlobalString(var)
	return Entity(0):GetNetworkedString(var);
end

function GetGlobalVector(var)
	return Entity(0):GetNetworkedVector(var);
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
