// Authour: Haza55

include("pipe_sh.lua");

require("socket");
require("glon");

if(!socket) then
	hook.Add("HUDPaint", "PIPE-Error", function()
		draw.RoundedBox(3, ScrW()/2-400, ScrH()/2-64, 800, 128, Color(0, 0, 0, 200));
		draw.SimpleText("To play on this server properly, you must download and install the ICannt GMod Addon Installer from http://icannt.org", "ScoreboardText", ScrW()/2, ScrH()/2, Color(255,0,0,255), TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
	end);
end

g_PipeConnection = nil;
g_AuthKey = nil;

g_Connects = 0;

local netDebug = CreateClientConVar( "pipe_debug", "0", false, false );

usermessage.Hook("PIPE-ResetEnt", function(bf)
    local entid = bf:ReadLong();
	g_NetVars[entid] = {};
end);

function PIPE_Connect()
	if(g_PipeConnection) then
		g_PipeConnection:close();
	end
	
	if(!g_AuthKey) then return; end
	
	if(g_Connects > 5) then
		hook.Add("HUDPaint", "PIPE-Error", function()
			draw.RoundedBox(3, ScrW()/2-400, ScrH()/2-64, 800, 128, Color(0, 0, 0, 200));
			draw.SimpleText("There appears to be an issue with your connection. Please contact an Admin for help.", "ScoreboardText", ScrW()/2, ScrH()/2, Color(255,0,0,255), TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
		end);
		return;
	end
	
	g_PipeConnection = socket.connect(const_ServerIP, const_BindPort);
	
	g_Connects = g_Connects + 1;
	
	if(!g_PipeConnection) then timer.Simple(2, PIPE_Connect); return; end
	
	g_PipeConnection:settimeout(1);
	g_PipeConnection:setoption("keepalive", true);
	
	g_PipeConnection:send(g_AuthKey .. "\n");
	g_PipeConnection:settimeout(0);
	
	print("PIPE: Connected to Server ", const_ServerIP .. ":" .. const_BindPort);
end

usermessage.Hook("PIPE-DemandConnect", function(bf)
    g_AuthKey = bf:ReadString();
	PIPE_Connect();
end);

g_NextRecv = CurTime();

hook.Add("Think", "PIPE-RecvThink", function()
	if(!g_PipeConnection) then return; end
	
	if(g_NextRecv > CurTime()) then return; end
	g_NextRecv = CurTime() + 0.1;

	local packet, err = g_PipeConnection:receive();
	
	if(!packet) then
		if(err == "closed") then
			print("PIPE: Lost connection, reconnecting...");
			PIPE_Connect();
		end
		return; 
	end
	
	if(netDebug:GetBool()) then print("Incoming PIPE msg: size " .. string.len(packet) .. " bytes"); end
	
	local b, tbl = pcall(glon.decode, packet);
	
	if(!b) then print(tbl); return; end
	
	for k, v in pairs(tbl) do
		for c, j in pairs(v) do
			g_NetVars[k] = g_NetVars[k] or {};
			g_NetVarsProxies[Entity(k)] = g_NetVarsProxies[Entity(k)] or {};
			if(type(g_NetVarsProxies[Entity(k)][var]) == "function") then
				pcall(g_NetVarsProxies[Entity(k)][var], Entity(k), c, g_NetVars[k][c], j);
			end
			g_NetVars[k][c] = j;
		end
	end
end);

print("PIPE Loaded!");