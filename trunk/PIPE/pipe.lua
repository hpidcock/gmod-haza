// Authour: Haza55
AddCSLuaFile("pipe_cl.lua");
AddCSLuaFile("pipe_sh.lua");

include("pipe_sh.lua");

require("threadsafesockets");
require("glon");

const_JoinCmdDelay = 5;
const_UpdateInterval = 0.3;
const_BulkSendCount = 28;
const_SingularLimit = 20;
const_PlayersPerThread = 3;
const_TCPSendTimeOut = 500;
const_MaxPlayers = MaxPlayers();

local bit_lshift = function(a, b) return a * (2 ^ b) end
local bit_rshift = function(a, b) return math.floor(a / (2 ^ b)) end

function PIPE.Msg(s)
	print(s);
	ServerLog(s .. "\n");
end

PIPE.Net.Server = nil;
PIPE.Net.Connections = {};

PIPE.Net.Server = tcplisten(const_ServerIP, const_BindPort, const_MaxPlayers);
	
PIPE.Msg("PIPE: Server Bound.");
hook.Call("PipeReady", GAMEMODE);

///////////////////////////////////////////////////////////////////////////////////////////
// Multithreaded Sending
///////////////////////////////////////////////////////////////////////////////////////////
PIPE.Net.ThreadCount = math.ceil(const_MaxPlayers / const_PlayersPerThread)

function PIPE.Net.Send(typ, packets, clients)
	// This can never be a \n
	if(typ >= 0x0A) then typ = typ + 1; end
	
	for k, v in pairs(packets) do
		packets[k] = string.char(typ) .. v;
		local len = string.len(packets[k]) + 0x101;
		local highByte = bit_rshift(len, 8);
		local lowByte = len - bit_lshift(highByte, 8);
		packets[k] = string.char(highByte) .. string.char(lowByte) .. packets[k];
	end
	
	local sockets = {};
	for _, v in pairs(clients) do
		if(v.SOCK) then
			local threadid = math.ceil((v.INDEX / const_MaxPlayers) * PIPE.Net.ThreadCount);
			sockets[threadid] = sockets[threadid] or {};
			table.insert(sockets[threadid], v.SOCK);
		end
	end
	
	for k, v in pairs(sockets) do
		tcpsend(packets, table.Count(packets), v, table.Count(v), {}, const_TCPSendTimeOut);
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
		umsg.String(const_ServerIP);
	umsg.End();
 
	ply.PIPE.CONNECTCMDSENT = true;
	
	PIPE.Msg("PIPE: Player Initial Spawn - AuthKey sent to " .. ply.PIPE.STEAMID);
end

function PIPE.Net.DispatchJoinCmd(ply, cmd, args)
	if(!ply.PIPE) then return; end
	if(ply.PIPE.CONNECTCMDSENT) then return; end
	
	timer.Simple(const_JoinCmdDelay, PIPE.Net.SendJoinCmd, ply);
end
concommand.Add("pipe_ready", PIPE.Net.DispatchJoinCmd)
--hook.Add("PlayerInitialSpawn", "PIPE-PlayerInitialSpawn", PIPE.Net.DispatchJoinCmd);

function PIPE.Net.DisconnectPlayer(ply)
	if(!ply.PIPE) then return; end
	
	PIPE.Net.Connections[ply.PIPE.AUTHKEY] = nil;
	
	if(!ply.PIPE.SOCK) then return; end

	tcpclose(ply.PIPE.SOCK);
	
	PIPE.Msg("PIPE: Player Disconnected - Closed pipe with " .. ply.PIPE.STEAMID);
end
hook.Add("PlayerDisconnected", "PIPE-PlayerDisconnected", PIPE.Net.DisconnectPlayer);

function PIPE.Net.RandomPayLoad()
	local payload = "";

	for i=1,256 do
		payload = payload .. tostring(math.random(0, 9));
	end

	return payload;
end

function PIPE.Net.SocketListener()
	local cl = tcpaccept(PIPE.Net.Server);
	
	if(!cl || cl == -1) then return; end
	
	local auth = tcprecv(cl, 1000);
	
	if(!PIPE.Net.Connections[auth]) then tcpclose(cl); return; end
	
	if(!PIPE.Net.Connections[auth].PIPE.SOCK) then tcpclose(PIPE.Net.Connections[auth].PIPE.SOCK); end
	
	PIPE.Net.Connections[auth].PIPE.SOCK = cl;
	
	PIPE.Net.Send(PIPE_SPEEDTEST, {LibCompress.CompressLZW(glon.encode({Time=CurTime(), PayLoad=PIPE.Net.RandomPayLoad()}))}, {PIPE.Net.Connections[auth].PIPE});
	
	PIPE.Msg("PIPE: Player PIPE Connected - AuthKey recv " .. PIPE.Net.Connections[auth].PIPE.AUTHKEY .. " " .. PIPE.Net.Connections[auth].PIPE.STEAMID);
end
hook.Add("Think", "PIPE-Acceptor", PIPE.Net.SocketListener);

function PIPE.Net.Receiver()
	for _, v in pairs(PIPE.Net.Connections) do
		if(v.PIPE and v.PIPE.SOCK) then
			
			local packet = tcprecv(v.PIPE.SOCK, 0);
			
			if(packet and packet != "") then
				local typ = string.byte(packet);
				packet = string.Right(packet, string.len(packet) - 1);
				
				local b, data = pcall(LibCompress.DecompressLZW, packet);
				if(b) then
					
					if(typ == PIPE_CL_NETVAR_REQ) then
						local b, tbl = pcall(glon.decode, data);
						if(b) then
							local sendTable = {};
							for k, v in pairs(tbl) do
								sendTable[v] = PIPE.NetVar.Vars[v];
							end
							PIPE.Net.Send(PIPE_NETWORKVAR_FULL, {LibCompress.CompressLZW(glon.encode(sendTable))}, {v.PIPE});
						end
					end
					
				end
			end
		end
	end
end
hook.Add("Think", "PIPE-Receiver", PIPE.Net.Receiver);

function PIPE.Net.SpeedCmd(ply, cmd, args)
	ply.PIPE.SPEED = tonumber(args[1])
	
	if(ply.PIPE.SPEED > 1500) then
		local tbl = PIPE.NetVar.FullUpdatePackets(const_BulkSendCount)
		local time = 0
		for _, v in pairs(tbl) do
			timer.Simple(time, PIPE.Net.Send, PIPE_NETWORKVAR, {v}, {ply.PIPE});
			time = time + 0.15
		end
		PIPE.Msg("PIPE: Player " .. ply:Nick() .. " is on Broadband.");
	else
		local tbl = PIPE.NetVar.FullUpdatePackets(const_BulkSendCount)
		local time = 0
		for _, v in pairs(tbl) do
			timer.Simple(time, PIPE.Net.Send, PIPE_NETWORKVAR, {v}, {ply.PIPE});
			time = time + 0.5
		end
		PIPE.Msg("PIPE: Player " .. ply:Nick() .. " is on DialUp.");
	end
end
concommand.Add("pipe_speed", PIPE.Net.SpeedCmd)

function PIPE.Net.Shutdown()
	tcpclose(PIPE.Net.Server);
	
	for _, v in ipairs(PIPE.Net.Connections) do
		if(v.PIPE.SOCK) then
			tcpclose(v.PIPE.SOCK);
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
	
	local clients = {};
	for k, v in pairs(PIPE.Net.Connections) do
		if(v.PIPE.SOCK) then
			table.insert(clients, v.PIPE);
		end
	end
	
	PIPE.Net.Send(PIPE_PING, {LibCompress.CompressLZW(glon.encode({"P"}))}, clients);
	
	if(!PIPE.NetVar.ChangesMade || table.Count(PIPE.NetVar.Changes) == 0) then return; end
	PIPE.NetVar.ChangesMade = false;

	local encoded = LibCompress.CompressLZW(glon.encode(PIPE.NetVar.Changes));

	PIPE.Net.Send(PIPE_NETWORKVAR, {encoded}, clients);
	
	PIPE.NetVar.Changes = {};
end
hook.Add("Think", "PIPE-SenderThink", PIPE.NetVar.Sender);

function PIPE.NetVar.FullUpdatePackets(bulkSendCount)
	local ret = {};
	local bulkTemp = {};
	
	for k, v in pairs(PIPE.NetVar.Vars) do
		if(Entity(k):IsWorld() or Entity(k):IsPlayer() or table.Count(v) >= const_SingularLimit) then
			table.insert(ret, LibCompress.CompressLZW(glon.encode({[k]=v})));
		else
			bulkTemp[k] = v;
			
			if(table.Count(bulkTemp) >= bulkSendCount) then
				table.insert(ret, LibCompress.CompressLZW(glon.encode(bulkTemp)));
				bulkTemp = {};
			end
		end
	end
	
	if(table.Count(bulkTemp) != 0) then
		table.insert(ret, LibCompress.CompressLZW(glon.encode(bulkTemp)));
	end
	
	return ret;
end

function PIPE.NetVar.ResetEntity(ent)
	if(ent:EntIndex() == 0) then
		return;
	end

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

function PIPE.Net.ErrorLog(ply, cmd, args)
	file.Write("pipe_error_log.txt", tostring(file.Read("pipe_error_log.txt") or "") .. string.Implode(" ", args) .. "\r\n");
end
concommand.Add("pipe_error_log", PIPE.Net.ErrorLog)