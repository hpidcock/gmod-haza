// Authour: Haza55
include("pipe_sh.lua");

require("socket");
require("glon");

if(!socket) then
	hook.Add("HUDPaint", "PIPE-Error", function()
		draw.RoundedBox(3, ScrW()/2-400, ScrH()/2-64, 800, 128, Color(0, 0, 0, 200));
		draw.SimpleText("To play on this server properly, you must download and install the gm_luasocket.", "ScoreboardText", ScrW()/2, ScrH()/2, Color(255,0,0,255), TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
	end);
end

PIPE.Net.Connection = nil;
PIPE.Net.AuthKey = nil;
PIPE.Net.Connects = 0;

local netDebug = CreateClientConVar( "pipe_debug", "0", false, false );

usermessage.Hook("PIPE-ResetEnt", function(bf)
    local entid = bf:ReadLong();
	PIPE.NetVar.Vars[entid] = {};
end);

function PIPE.Net.Connect()
	if(PIPE.Net.Connection) then
		PIPE.Net.Connection:close();
	end
	
	if(!PIPE.Net.AuthKey) then return; end
	
	if(PIPE.Net.Connects > 5) then
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
end

usermessage.Hook("PIPE-DemandConnect", function(bf)
    PIPE.Net.AuthKey = bf:ReadString();
	PIPE.Net.Connect();
end);

PIPE.Net.NextRecv = CurTime();
hook.Add("Think", "PIPE-RecvThink", function()
	if(!PIPE.Net.Connection) then return; end
	
	if(PIPE.Net.NextRecv > CurTime()) then return; end
	
	local packet, err = PIPE.Net.Connection:receive();
	
	if(!packet) then
		PIPE.Net.NextRecv = CurTime() + 0.1;
		if(err == "closed") then
			print("PIPE: Lost connection, reconnecting...");
			PIPE.Net.Connect();
		end
		return; 
	end
	
	if(string.len(packet) == 0) then return; end
	
	if(netDebug:GetBool()) then print("PIPE: Incoming msg: size " .. string.len(packet) .. " bytes."); end
	
	local typ = string.byte(packet);
	
	packet = string.Right(packet, string.len(packet) - 1);
	
	local b, tbl = pcall(glon.decode, packet);
	
	if(!b) then
		print(tbl);
		if(netDebug:GetBool()) then print("PIPE: Bad Data: " .. tostring(packet)); end
		return;
	end
	
	for k, v in pairs(tbl) do
		for c, j in pairs(v) do
			PIPE.NetVar.Vars[k] = PIPE.NetVar.Vars[k] or {};
			PIPE.NetVar.Proxies[Entity(k)] = PIPE.NetVar.Proxies[Entity(k)] or {};
			
			if(type(PIPE.NetVar.Proxies[Entity(k)][var]) == "function") then
				pcall(PIPE.NetVar.Proxies[Entity(k)][var], Entity(k), c, PIPE.NetVar.Vars[k][c], j);
			end
			
			PIPE.NetVar.Vars[k][c] = j;
		end
	end
end);

print("PIPE: Loaded!");