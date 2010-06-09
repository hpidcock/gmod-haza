// Authour: Haza55
local _Call, _Error = pcall(function()

include("pipe_sh.lua");

require("oosocks");
require("glon");

if(!OOSock) then
	return
end

PIPE.Net.Connection = nil;
PIPE.Net.AuthKey = nil;
PIPE.Net.Connects = 0;

local bit_lshift = function(a, b) return a * (2 ^ b) end
local bit_rshift = function(a, b) return math.floor(a / (2 ^ b)) end

local netDebug = CreateClientConVar( "pipe_debug", "0", false, false );

///////////////////////////////////////////////////////////////////////////////////////////
// Connection and Authentication
///////////////////////////////////////////////////////////////////////////////////////////
function PIPE.Net.Connect()
	local _Call, _Error = pcall(function()
		if(PIPE.Net.Connection) then
			PIPE.Net.Connection = nil;
		end
		
		if(!PIPE.Net.AuthKey) then return; end
		
		if(PIPE.Net.Connects > 15) then
			hook.Add("HUDPaint", "PIPE-Error", function()
				draw.RoundedBox(3, ScrW()/2-400, ScrH()/2-64, 800, 128, Color(0, 0, 0, 200));
				draw.SimpleText("There appears to be an issue with your connection. Please contact an Admin for help.", "ScoreboardText", ScrW()/2, ScrH()/2, Color(255,0,0,255), TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
			end);
			return;
		end
		
		PIPE.Net.Connection = OOSock(IPPROTO_TCP);
		PIPE.Net.Connection:Connect(const_ServerIP, const_BindPort);
		PIPE.Net.LastPing = CurTime();
		
		PIPE.Net.Connects = PIPE.Net.Connects + 1;
		
		if(!PIPE.Net.Connection) then timer.Simple(2, PIPE.Net.Connect); return; end
		
		PIPE.Net.Connection:SendLine(PIPE.Net.AuthKey);
		
		print("PIPE: Connected to Server ", const_ServerIP .. ":" .. const_BindPort);
		
		hook.Call("PipeReady", GAMEMODE);
	end);
	
	if(!_Call) then
		RunConsoleCommand("pipe_error_log", tostring(_Error));
	end
end

function PIPE.Net.RecvJoinCmd(bf)
	local _Call, _Error = pcall(function()
		PIPE.Net.AuthKey = bf:ReadString();
		const_ServerIP = bf:ReadString();
		PIPE.Net.Connect();
	end);
	
	if(!_Call) then
		RunConsoleCommand("pipe_error_log", tostring(_Error));
	end
end
usermessage.Hook("PIPE-DemandConnect", PIPE.Net.RecvJoinCmd);

PIPE.Net.NextRecv = CurTime();
function PIPE.Net.Receiver()
	local _Call, _Error = pcall(function()
		if(!PIPE.Net.Connection) then return; end
		
		PIPE.Net.LastPing = PIPE.Net.LastPing or CurTime();
		if(CurTime() - PIPE.Net.LastPing > 30) then
			PIPE.Net.Connect();
		end
		
		if(PIPE.Net.NextRecv > CurTime()) then return; end
		
		PIPE.Net.Connection:SetTimeout(0);
		local packet = PIPE.Net.Connection:Receive(2);
		
		if(!packet || packet == "") then return; end
		
		if(string.len(packet) == 0) then return; end
		
		local highSizeByte = string.byte(packet);
		packet = string.Right(packet, string.len(packet) - 1);
		local lowSizeByte = string.byte(packet);
		packet = string.Right(packet, string.len(packet) - 1);

		local sizeToRead = bit_lshift(highSizeByte, 8) + lowSizeByte;
		sizeToRead = sizeToRead - 0x101;
		
		PIPE.Net.Connection:SetTimeout(5000);
		local packet = PIPE.Net.Connection:Receive(sizeToRead);
		
		if(string.len(packet) == 0) then return; end
		
		local typ = string.byte(packet);
		
		PIPE.LastInSize = string.len(packet);
		
		packet = string.Right(packet, string.len(packet) - 1);
		
		local b, data = pcall(LibCompress.DecompressLZW, packet);
		
		if(!b) then
			print(data);
			if(netDebug:GetBool()) then print("PIPE: Bad Packet: " .. tostring(packet)); end
			return;
		end
		
		if(netDebug:GetBool()) then print("PIPE: Incoming msg: sz-comp " .. tostring(string.len(packet)) .. " bytes. size " .. tostring(string.len(data)) .. " bytes. type " .. tostring(PIPE_TYPES[typ])); end
		
		local b, tbl = pcall(glon.decode, data);
		
		if(!b) then
			print(tbl);
			if(netDebug:GetBool()) then print("PIPE: Bad Data: " .. tostring(data)); end
			return;
		end
		
		if(typ == PIPE_NETWORKVAR) then
			PIPE.NetVar.MergeVarTable(tbl);
		elseif(typ == PIPE_NETWORKVAR_FULL) then
			for k, _ in pairs(tbl) do
				PIPE.NetVar.Vars[k] = {};
			end
			PIPE.NetVar.MergeVarTable(tbl);
		elseif(typ == PIPE_USERMESSAGE) then
		elseif(typ == PIPE_DATASTREAM) then
		elseif(typ == PIPE_SPEEDTEST) then
			local b = pcall(function()
				print("PIPE: Speed set to " .. tostring((string.len(packet) / (CurTime() - tbl.Time))))
				RunConsoleCommand("pipe_speed", (string.len(packet) / (CurTime() - tbl.Time)))
			end);
			if(!b) then
				RunConsoleCommand("pipe_speed", 100)
			end
		elseif(typ == PIPE_PING) then
			PIPE.Net.LastPing = CurTime();
		end
	end);
	
	if(!_Call) then
		RunConsoleCommand("pipe_error_log", tostring(_Error));
	end
end
hook.Add("Think", "PIPE-RecvThink", PIPE.Net.Receiver);

function PIPE.Net.Send(typ, tbl)
	local _Call, _Error = pcall(function()
		if(!PIPE.Net.Connection) then return; end
		
		local data = string.char(typ) .. LibCompress.CompressLZW(glon.encode(tbl));
		
		PIPE.Net.Connection:SendLine(data);
	end);
	
	if(!_Call) then
		RunConsoleCommand("pipe_error_log", tostring(_Error));
	end
end

function PIPE.Net.ShutDown()
	local _Call, _Error = pcall(function()
		if(PIPE.Net.Connection) then
			PIPE.Net.Connection = nil;
		end
	end);
	
	if(!_Call) then
		RunConsoleCommand("pipe_error_log", tostring(_Error));
	end
end
hook.Add("ShutDown", "PIPE-ShutDown", PIPE.Net.ShutDown)

///////////////////////////////////////////////////////////////////////////////////////////
// Networked Varibles
///////////////////////////////////////////////////////////////////////////////////////////
function PIPE.NetVar.ResetEntity(bf)
	local _Call, _Error = pcall(function()
		local entid = bf:ReadLong();
		if(entid == 0) then
			return;
		end
		PIPE.NetVar.Vars[entid] = {};
		PIPE.NetVar.Proxies[entid] = {};
	end);
	
	if(!_Call) then
		RunConsoleCommand("pipe_error_log", tostring(_Error));
	end
end
usermessage.Hook("PIPE-ResetEnt", PIPE.NetVar.ResetEntity);

function PIPE.NetVar.DeepTableMerge(tbl, changes)
	local newTbl = tbl or {};
	for k, v in pairs(changes) do
		if(v == PIPE_VALUE_NIL) then
			v = nil;
		end
		
		if(type(v) == "table") then
			newTbl[k] = PIPE.NetVar.DeepTableMerge(newTbl[k], v);
		else
			newTbl[k] = v;
		end
	end
	return newTbl;
end

function PIPE.NetVar.DeepTableCopy(oldtable, newtable)
	local oldtable = oldtable or {};
	for k, v in pairs(oldtable) do
		if(type(v) == "table") then
			newtable[k] = {};
			PIPE.NetVar.DeepTableCopy(v, newtable[k]);
		else
			newtable[k] = v;
		end
	end
end

function PIPE.NetVar.MergeVarTable(tbl)
	for k, v in pairs(tbl) do
		for c, j in pairs(v) do
			PIPE.NetVar.Vars[k] = PIPE.NetVar.Vars[k] or {};
			PIPE.NetVar.Proxies[k] = PIPE.NetVar.Proxies[k] or {};
			
			if(j == PIPE_VALUE_NIL) then
				j = nil;
			end
			
			if(type(PIPE.NetVar.Proxies[k][c]) == "function" && type(j) != "table") then
				pcall(PIPE.NetVar.Proxies[k][c], Entity(k), c, PIPE.NetVar.Vars[k][c], j);
			end
			
			if(type(j) == "table") then
				local newTable = {};
				PIPE.NetVar.DeepTableCopy(PIPE.NetVar.Vars[k][c], newTable);
				newTable = PIPE.NetVar.DeepTableMerge(newTable, j);
				if(type(PIPE.NetVar.Proxies[k][c]) == "function") then
					pcall(PIPE.NetVar.Proxies[k][c], Entity(k), c, PIPE.NetVar.Vars[k][c], newTable);
				end
				PIPE.NetVar.Vars[k][c] = newTable;
			else
				PIPE.NetVar.Vars[k][c] = j;
			end
		end
	end
end

function PIPE.NetVar.RequestUpdate(tbl)
	local _Call, _Error = pcall(function()
		PIPE.NetVar.LastRequest = PIPE.NetVar.LastRequest or 0;
		if(PIPE.NetVar.LastRequest + 1 > CurTime()) then return; end
		PIPE.NetVar.LastRequest = CurTime();
		PIPE.Net.Send(PIPE_CL_NETVAR_REQ, tbl);
	end);
	
	if(!_Call) then
		RunConsoleCommand("pipe_error_log", tostring(_Error));
	end
end

surface.CreateFont("Lucida Console", 10 * (ScrH() / 480), 400, false, false, "PIPEFont4")

hook.Add("HUDPaint", "PIPE-Debug", function()
	if(GetConVarNumber("net_graph") == 0) then return end
	
	surface.SetFont("PIPEFont4")
	local screenx = surface.GetTextSize("fps:  435  ping: 533 ms lerp 112.3 ms0/0")
	local w, h = surface.GetTextSize("pipe-in:          ")
	local y = ScrH() - 17
	
	if(GetConVarNumber("net_graph") == 1) then
		y = y - h * 5
	elseif(GetConVarNumber("net_graph") == 2) then
		y = y - h * 5
	elseif(GetConVarNumber("net_graph") == 3) then
		y = y - h * 6
	elseif(GetConVarNumber("net_graph") == 4) then
		y = y - h * 7
	end
	
	draw.SimpleTextOutlined("pipe-in:          ", "PIPEFont4", ScrW() - screenx, y, Color(0.9*255,0.9*255,0.7*255,255), TEXT_ALIGN_LEFT, TEXT_ALIGN_LEFT, 1, Color(0,0,0,255))
	
	local w2, h2 = surface.GetTextSize(tostring(PIPE.LastInSize))

	draw.SimpleTextOutlined(tostring(PIPE.LastInSize), "PIPEFont4", ScrW() - screenx + w - w2, y, Color(0.9*255,0.9*255,0.7*255,255), TEXT_ALIGN_LEFT, TEXT_ALIGN_LEFT, 1, Color(0,0,0,255))
end);

print("PIPE: Loaded!");

timer.Simple(2, RunConsoleCommand, "pipe_ready");

end);

if(!_Call) then
	RunConsoleCommand("pipe_error_log", tostring(_Error));
end