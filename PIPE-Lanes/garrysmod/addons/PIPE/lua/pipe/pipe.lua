// Authour: Haza55
AddCSLuaFile("pipe_cl.lua");
AddCSLuaFile("pipe_sh.lua");

include("pipe_sh.lua");

require("socket");
require("lanes");
require("glon");

const_JoinCmdDelay = 5;
const_UpdateInterval = 0.2;
const_BulkSendCount = 64;
const_SingularLimit = 10;
const_PlayersPerThread = 4;
const_MaxPlayers = MaxPlayers();

function PIPE.Msg(s)
	print(s);
	ServerLog(s);
end

PIPE.Net.Server = -1;
PIPE.Net.Connections = {};

PIPE.Net.Server = socket.bind(const_ServerIP, const_BindPort, 64);
PIPE.Net.Server:settimeout(0);
	
PIPE.Msg("PIPE: Server Bound.");

///////////////////////////////////////////////////////////////////////////////////////////
// Multithreaded Sending
///////////////////////////////////////////////////////////////////////////////////////////
function PIPE.Net.ThreadedSend(linda, send)
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

function PIPE.Net.InitThreadStates(threadCount)
	PIPE.Net.Threads = {};
	PIPE.Net.ThreadCount = threadCount;
	
	local function NewThread()
		local Thread = {};
		Thread.Linda = lanes.linda();
		Thread.Thread = lanes.gen("*", PIPE.Net.ThreadedSend)(Thread.Linda, tcpsend);
		return Thread;
	end
	
	for i = 1, threadCount do
		PIPE.Net.Threads[i] = NewThread();
	end
	
	PIPE.Msg("PIPE: Created " .. tostring(threadCount) .. " sender thread(s).");
end

PIPE.Net.InitThreadStates(math.ceil(const_MaxPlayers / const_PlayersPerThread));

function PIPE.Net.Send(typ, packets, clients)
	// This can never be a \n
	if(typ >= 0x0A) then typ = typ + 1; end
	
	for k, v in pairs(packets) do
		packets[k] = string.char(typ) .. v;
	end
	
	local sockets = {};
	for _, v in pairs(clients) do
		if(v.SOCK) then
			local threadid = math.ceil((v.INDEX / const_MaxPlayers) * PIPE.Net.ThreadCount);
			sockets[threadid] = sockets[threadid] or {};
			table.insert(sockets[threadid], v.SOCK:getfd());
		end
	end
	
	for k, v in pairs(sockets) do
		local thread = PIPE.Net.Threads[k];
	
		if(thread) then
			thread.Linda:send("packets", packets);
			thread.Linda:send("sockets", v);
		end
	end	
end

///////////////////////////////////////////////////////////////////////////////////////////
// Connection and Authentication
///////////////////////////////////////////////////////////////////////////////////////////
function PIPE.Net.InitPlayer(ply, steamid)
	PIPE.Net.Connections = PIPE.Net.Connections or {};
	
	ply.PIPE = {};
	ply.PIPE.STEAMID = steamid;
	ply.PIPE.INDEX = ply:EntIndex();
	ply.PIPE.SOCK = nil;
	ply.PIPE.AUTHKEY = tostring(util.CRC(tostring(math.random(100000000, 999999999)) .. ply.PIPE.STEAMID .. tostring(math.random(100000000, 999999999))));
	ply.PIPE.CONNECTCMDSENT = false;
	
	PIPE.Net.Connections[ply.PIPE.AUTHKEY] = ply;
	
	PIPE.Msg("PIPE: Player Connected - AuthKey set to " .. ply.PIPE.AUTHKEY);
end
hook.Add("PlayerAuthed", "PIPE-PlayerAuthed", PIPE.Net.InitPlayer);

function PIPE.Net.SendJoinCmd(ply)
	if(!ply || !ply:IsValid() || !ply:IsPlayer()) then return; end

	umsg.Start("PIPE-DemandConnect", ply);
		umsg.String(ply.PIPE.AUTHKEY);
	umsg.End();
 
	ply.PIPE.CONNECTCMDSENT = true;
	
	PIPE.Msg("PIPE: Player Initial Spawn - AuthKey sent to " .. ply.PIPE.STEAMID);
end

function PIPE.Net.DispatchJoinCmd(ply)
	if(!ply.PIPE) then return; end
	if(ply.PIPE.CONNECTCMDSENT) then return; end
	
	timer.Simple(const_JoinCmdDelay, PIPE.Net.SendJoinCmd, ply);
end
hook.Add("PlayerInitialSpawn", "PIPE-PlayerInitialSpawn", PIPE.Net.DispatchJoinCmd);

function PIPE.Net.DisconnectPlayer(ply)
	if(!ply.PIPE) then return; end
	
	PIPE.Net.Connections[ply.PIPE.AUTHKEY] = nil;
	
	if(!ply.PIPE.SOCK) then return; end

	ply.PIPE.SOCK:close();
	
	PIPE.Msg("PIPE: Player Disconnected - Closed pipe with " .. ply.PIPE.STEAMID);
end
hook.Add("PlayerDisconnected", "PIPE-PlayerDisconnected", PIPE.Net.DisconnectPlayer);

function PIPE.Net.SocketListener()
	local cl = PIPE.Net.Server:accept();
	
	if(!cl) then return; end
	
	cl:settimeout(2);
	cl:setoption("keepalive", true);
	
	local auth = cl:receive();
	
	cl:settimeout(100/1000);
	
	if(!cl) then return; end

	PIPE.Net.Connections[auth].PIPE.SOCK = cl;
	
	PIPE.Net.Send(PIPE_NETWORKVAR, PIPE.NetVar.FullUpdatePackets(), {PIPE.Net.Connections[auth].PIPE});
	
	PIPE.Msg("PIPE: Player PIPE Connected - AuthKey recv " .. PIPE.Net.Connections[auth].PIPE.AUTHKEY .. " " .. PIPE.Net.Connections[auth].PIPE.STEAMID);
end
hook.Add("Think", "PIPE-Acceptor", PIPE.Net.SocketListener);

function PIPE.Net.Shutdown()
	PIPE.Net.Server:close();
	
	for _, v in ipairs(PIPE.Net.Connections) do
		if(v.PIPE.SOCK) then
			v.PIPE.SOCK:close();
		end
	end
	
	PIPE.Msg("PIPE: ShutDown - Unbinding, and disconnecting all players.");
end
hook.Add("ShutDown", "PIPE-ShutDown", PIPE.Net.Shutdown);


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
PIPE.NetVar.NextSend = CurTime();
function PIPE.NetVar.Sender()
	if(CurTime() < PIPE.NetVar.NextSend) then return; end
	PIPE.NetVar.NextSend = CurTime() + const_UpdateInterval;
	
	if(!PIPE.NetVar.ChangesMade || table.Count(PIPE.NetVar.Changes) == 0) then return; end
	PIPE.NetVar.ChangesMade = false;

	local encoded = glon.encode(PIPE.NetVar.Changes) .. "\n";
	
	local clients = {};
	for k, v in pairs(PIPE.Net.Connections) do
		if(v.PIPE.SOCK) then
			table.insert(clients, v.PIPE);
		end
	end

	PIPE.Net.Send(PIPE_NETWORKVAR, {encoded}, clients);
	
	PIPE.NetVar.Changes = {};
end
hook.Add("Think", "PIPE-SenderThink", PIPE.NetVar.Sender);

function PIPE.NetVar.FullUpdatePackets()
	local ret = {};
	local bulkTemp = {};
	
	for k, v in pairs(PIPE.NetVar.Vars) do
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

function PIPE.NetVar.ResetEntity(ent)
	PIPE.NetVar.Vars[ent:EntIndex()] = nil;
	PIPE.NetVar.Proxies[ent] = nil;
	PIPE.NetVar.Changes[ent:EntIndex()] = nil;
	
	local recpt = RecipientFilter();
	recpt:AddAllPlayers();
	
	umsg.Start("PIPE-ResetEnt", recpt);
		umsg.Long(ent:EntIndex());
	umsg.End();
end
hook.Add("EntityRemoved", "PIPE-EntityRemoved", PIPE.NetVar.ResetEntity);