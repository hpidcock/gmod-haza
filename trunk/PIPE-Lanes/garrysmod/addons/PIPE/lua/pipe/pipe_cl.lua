// Authour: Haza55
include("pipe_sh.lua");

require("socket");
require("glon");

if(!socket) then
	hook.Add("HUDPaint", "PIPE-Error", function()
		draw.RoundedBox(3, ScrW()/2-400, ScrH()/2-64, 800, 128, Color(0, 0, 0, 200));
		draw.SimpleText("To play on this server properly, you must download and install the ICannt GMod Addon Installer from http://tinyurl.com/icrp-mod", "ScoreboardText", ScrW()/2, ScrH()/2, Color(255,0,0,255), TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
	end);
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
	if(PIPE.Net.Connection) then
		PIPE.Net.Connection:close();
	end
	
	if(!PIPE.Net.AuthKey) then return; end
	
	if(PIPE.Net.Connects > 15) then
		hook.Add("HUDPaint", "PIPE-Error", function()
			draw.RoundedBox(3, ScrW()/2-400, ScrH()/2-64, 800, 128, Color(0, 0, 0, 200));
			draw.SimpleText("There appears to be an issue with your connection. Please contact an Admin for help.", "ScoreboardText", ScrW()/2, ScrH()/2, Color(255,0,0,255), TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
		end);
		return;
	end
	
	PIPE.Net.Connection = socket.connect(const_ServerIP, const_BindPort);
	
	PIPE.Net.Connects = PIPE.Net.Connects + 1;
	
	if(!PIPE.Net.Connection) then timer.Simple(2, PIPE.Net.Connect); return; end
	
	PIPE.Net.Connection:settimeout(1);
	PIPE.Net.Connection:setoption("keepalive", true);
	
	PIPE.Net.Connection:send(PIPE.Net.AuthKey .. "\n");
	PIPE.Net.Connection:settimeout(0);
	
	print("PIPE: Connected to Server ", const_ServerIP .. ":" .. const_BindPort);
	
	hook.Call("PipeReady", GAMEMODE);
end

function PIPE.Net.RecvJoinCmd(bf)
    PIPE.Net.AuthKey = bf:ReadString();
	const_ServerIP = bf:ReadString();
	PIPE.Net.Connect();
end
usermessage.Hook("PIPE-DemandConnect", PIPE.Net.RecvJoinCmd);

PIPE.Net.NextRecv = CurTime();
function PIPE.Net.Receiver()
	if(!PIPE.Net.Connection) then return; end
	
	if(PIPE.Net.NextRecv > CurTime()) then return; end
	
	local packet, err = PIPE.Net.Connection:receive(3);
	
	if(!packet) then
		PIPE.Net.NextRecv = CurTime() + 0.1;
		if(err == "closed") then
			print("PIPE: Lost connection, reconnecting...");
			PIPE.Net.Connect();
		end
		return; 
	end
	
	if(string.len(packet) == 0) then return; end
	
	local highSizeByte = string.byte(packet);
	packet = string.Right(packet, string.len(packet) - 1);
	local midSizeByte = string.byte(packet);
	packet = string.Right(packet, string.len(packet) - 1);
	local lowSizeByte = string.byte(packet);
	packet = string.Right(packet, string.len(packet) - 1);

	local sizeToRead = bit_lshift(highSizeByte, 16) + bit_lshift(midSizeByte, 8) + lowSizeByte;
	sizeToRead = sizeToRead - 0x10101;
	
	local packet, err = PIPE.Net.Connection:receive(sizeToRead);
	
	if(!packet) then
		PIPE.Net.NextRecv = CurTime() + 0.1;
		if(err == "closed") then
			print("PIPE: Lost connection, reconnecting...");
			PIPE.Net.Connect();
		end
		return; 
	end
	
	if(string.len(packet) == 0) then return; end
	
	local typ = string.byte(packet);
	
	if(netDebug:GetBool()) then print("PIPE: Incoming msg: size " .. string.len(packet) .. " bytes. type " .. PIPE_TYPES[typ]); end
	
	PIPE.LastInSize = string.len(packet);
	
	packet = string.Right(packet, string.len(packet) - 1);
	
	local b, data = pcall(LibCompress.DecompressLZW, packet);
	
	if(!b) then
		print(data);
		if(netDebug:GetBool()) then print("PIPE: Bad Packet: " .. tostring(packet)); end
		return;
	end
	
	local b, tbl = pcall(glon.decode, data);
	
	if(!b) then
		print(tbl);
		if(netDebug:GetBool()) then print("PIPE: Bad Data: " .. tostring(data)); end
		return;
	end
	
	if(typ == PIPE_NETWORKVAR) then
		PIPE.NetVar.MergeVarTable(tbl);
	elseif(typ == PIPE_USERMESSAGE) then
	elseif(typ == PIPE_DATASTREAM) then
	elseif(typ == PIPE_SPEEDTEST) then
		print("PIPE: Speed set to " .. tostring((string.len(packet) / (CurTime() - tbl.Time))))
		RunConsoleCommand("pipe_speed", (string.len(packet) / (CurTime() - tbl.Time)))
	end
end
hook.Add("Think", "PIPE-RecvThink", PIPE.Net.Receiver);

function PIPE.Net.ShutDown()
	if(PIPE.Net.Connection) then
		PIPE.Net.Connection:close();
	end
end
hook.Add("ShutDown", "PIPE-ShutDown", PIPE.Net.ShutDown)

///////////////////////////////////////////////////////////////////////////////////////////
// Networked Varibles
///////////////////////////////////////////////////////////////////////////////////////////
function PIPE.NetVar.ResetEntity(bf)
    local entid = bf:ReadLong();
	PIPE.NetVar.Vars[entid] = {};
	PIPE.NetVar.Proxies[entid] = {};
end
usermessage.Hook("PIPE-ResetEnt", PIPE.NetVar.ResetEntity);

function PIPE.NetVar.MergeVarTable(tbl)
	for k, v in pairs(tbl) do
		for c, j in pairs(v) do
			PIPE.NetVar.Vars[k] = PIPE.NetVar.Vars[k] or {};
			PIPE.NetVar.Proxies[k] = PIPE.NetVar.Proxies[k] or {};
			
			if(type(PIPE.NetVar.Proxies[k][c]) == "function") then
				pcall(PIPE.NetVar.Proxies[k][c], Entity(k), c, PIPE.NetVar.Vars[k][c], j);
			end
			
			PIPE.NetVar.Vars[k][c] = j;
		end
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