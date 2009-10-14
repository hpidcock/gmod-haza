// Authour: Haza55

AddCSLuaFile("pipe_cl.lua");
AddCSLuaFile("pipe_sh.lua");

include("pipe_sh.lua");

require("socket");
require("lanes");
require("glon");

g_PipeServer = -1;
g_PipeConnections = {};

const_JoinCmdDelay = 5;
const_UpdateInterval = 0.2; // 5 Times a second
const_BulkSendCount = 64;
const_SingularLimit = 10;

local function ThreadedSend(linda, send)
	if(!linda) then return; end
	if(!send) then return; end
	
	while(true) do
		local packets = linda:receive("packets", 0);
		local sockets = linda:receive("sockets", 0);
		
		if(packets && sockets) then
			for _, sock in pairs(sockets) do
				for _, data in pairs(packets) do
					if(sock) then
						send(sock, data);
					end
				end
			end
		end
	end
end

g_ThreadLinda = lanes.linda();  
g_Thread = lanes.gen("*", ThreadedSend)(g_ThreadLinda, _R["tcp{client}"].__index.send);

hook.Add("EntityRemoved", "PIPE-EntityRemoved", function(ent)
	g_NetVars[ent:EntIndex()] = nil;
	g_NetVarsProxies[ent] = nil;
	g_NetVarsChanged[ent:EntIndex()] = nil;
	local recpt = RecipientFilter();
	recpt:AddAllPlayers();
	umsg.Start("PIPE-ResetEnt", recpt);
		umsg.Long(ent:EntIndex());
	umsg.End();
end);

hook.Add("PlayerAuthed", "PIPE-PlayerAuthed", function(ply, steamid)
	g_PipeConnections = g_PipeConnections or {};
	
	ply._Pipe = {};
	ply._Pipe.STEAMID = steamid;
	ply._Pipe.SOCK = nil;
	ply._Pipe.AUTHKEY = tostring(util.CRC(tostring(math.random(100000000, 999999999)) .. ply._Pipe.STEAMID .. tostring(math.random(100000000, 999999999))));
	ply._Pipe.CONNECTCMDSENT = false;
	
	g_PipeConnections[ply._Pipe.AUTHKEY] = ply;
	
	print("PIPE: Player Connected - AuthKey set to ", ply._Pipe.AUTHKEY);
	ServerLog("PIPE: Player Connected - AuthKey set to " .. ply._Pipe.AUTHKEY);
end);

function PIPE_SendJoinCmd(ply)
	if(!ply || !ply:IsValid() || !ply:IsPlayer()) then return; end

	umsg.Start("PIPE-DemandConnect", ply);
		umsg.String(ply._Pipe.AUTHKEY);
	umsg.End();
 
	ply._Pipe.CONNECTCMDSENT = true;
	
	print("PIPE: Player Initial Spawn - AuthKey sent to ", ply._Pipe.STEAMID);
	ServerLog("PIPE: Player Initial Spawn - AuthKey sent to " .. ply._Pipe.STEAMID);
end

hook.Add("PlayerInitialSpawn", "PIPE-PlayerInitialSpawn", function(ply)
	if(!ply._Pipe) then return; end
	if(ply._Pipe.CONNECTCMDSENT) then return; end
	
	timer.Simple(const_JoinCmdDelay, PIPE_SendJoinCmd, ply);
end);

hook.Add("PlayerDisconnected", "PIPE-PlayerDisconnected", function(ply)
	if(!ply._Pipe) then return; end
	
	g_PipeConnections[ply._Pipe.AUTHKEY] = nil;
	
	if(!ply._Pipe.SOCK) then return; end

	ply._Pipe.SOCK:close();
	print("PIPE: Player Disconnected - Closed pipe with ", ply._Pipe.STEAMID);
	ServerLog("PIPE: Player Disconnected - Closed pipe with " .. ply._Pipe.STEAMID);
end);

g_NextSend = CurTime();

hook.Add("Think", "PIPE-SenderThink", function()
	if(CurTime() < g_NextSend) then return; end
	g_NextSend = CurTime() + const_UpdateInterval;
	
	if(!g_NetVarsAnyChanges || table.Count(g_NetVarsChanged) == 0) then return; end
	g_NetVarsAnyChanges = false;

	local encoded = glon.encode(g_NetVarsChanged) .. "\n";
	
	local sockets = {};
	for k, v in pairs(g_PipeConnections) do
		if(v._Pipe.SOCK) then
			table.insert(sockets, v._Pipe.SOCK);
		end
	end
	
	g_ThreadLinda:send("packets", {encoded});
	g_ThreadLinda:send("sockets", sockets);
	
	g_NetVarsChanged = {};
end);

function FullUpdatePackets()
	local ret = {};
	local bulkTemp = {};
	
	for k, v in pairs(g_NetVars) do
		if(Entity(k):IsWorld() or Entity(k):IsPlayer() or table.Count(v) >= const_SingularLimit) then
			table.insert(ret, glon.encode({[k]=v}) .. "\n");
		else
			bulkTemp[k] = v;
			
			if(table.Count(bulkTemp) >= const_BulkSendCount) then
				table.insert(ret, glon.encode(bulkTemp) .. "\n");
				bulkTemp = {};
			end
		end
	end
	
	if(table.Count(bulkTemp) != 0) then
		table.insert(ret, glon.encode(bulkTemp) .. "\n");
	end
	
	return ret;
end

function SendFullUpdate(sock)
	g_ThreadLinda:send("packets", FullUpdatePackets());
	g_ThreadLinda:send("sockets", {sock});
end

hook.Add("Think", "PIPE-Acceptor", function()
	local cl = g_PipeServer:accept();
	
	if(!cl) then return; end
	
	cl:settimeout(2);
	cl:setoption("keepalive", true);
	local auth = cl:receive();
	cl:settimeout(100/1000);
	
	if(!cl) then return; end
	
	if(!g_PipeConnections[auth]) then cl:close(); return; end
	
	g_PipeConnections[auth]._Pipe.SOCK = cl;
	
	SendFullUpdate(g_PipeConnections[auth]._Pipe.SOCK);
	
	print("PIPE: Player PIPE Connected - AuthKey recv", g_PipeConnections[auth]._Pipe.AUTHKEY, g_PipeConnections[auth]._Pipe.STEAMID);
	ServerLog("PIPE: Player PIPE Connected - AuthKey recv " .. g_PipeConnections[auth]._Pipe.AUTHKEY .. " " .. g_PipeConnections[auth]._Pipe.STEAMID);
end);

hook.Add("ShutDown", "PIPE-ShutDown", function()
	g_PipeServer:close();
	
	for _, v in ipairs(g_PipeConnections) do
		if(v._Pipe.SOCK) then
			v._Pipe.SOCK:close();
			v._Pipe.SOCK = nil;
		end
	end
	
	print("PIPE: ShutDown - Unbinding, and disconnecting all players.");
	ServerLog("PIPE: ShutDown - Unbinding, and disconnecting all players.");
end);

g_PipeServer = socket.bind(const_ServerIP, const_BindPort, 64);
g_PipeServer:settimeout(0);
	
print("PIPE: Server Bound!\n");
ServerLog("PIPE: Server Bound!");

