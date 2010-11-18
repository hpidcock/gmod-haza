// Written by Haza55

if(CLIENT) then return end

// Config

// Comma sperated table of all the other servers. This does not include this server.
// Format is {"IP", Port}
local const_P2PPeers = {{"10.10.1.126", 28076}, {"230.11.210.126", 28067}};

// This servers IP address and a spare port. Try to make the port differnt than what is here, if not people could possibly DOS your server.
local const_P2PListenIP = "10.10.1.126";
local const_P2PListenPort = 28030;

// This needs to be unique to this server and no other.
local const_P2PUniqueName = "Sandbox#1";

// This needs to be secret, but every server must have the same password.
local const_P2PPassword = "averylongpw122323432";

// These are the chat prefixes used for sending global messages.
local const_P2PChatPrefix = "]]";
local const_P2PAdminChatPrefix = "[[";

////////////////////////////////////////////////////////////////////
// Do Not Edit Below Here!
////////////////////////////////////////////////////////////////////
require("oosocks");
require("glon");

if(!OOSock) then
	Error("P2P - gm_oosocks not found!");
	return;
end

local g_Socket = nil;

local g_RecvQue = {};

local g_PeerId = util.CRC(const_P2PUniqueName);
local g_NetworkPassword = util.CRC(const_P2PPassword);

local g_MessageID = 0;

g_Socket = OOSock(IPPROTO_UDP);

function P2PCallback(socket, callType, callId, err, data, peer, peerPort)
    if(callType == SCKCALL_BIND && err == SCKERR_OK) then
        print("P2P - Bound.");
        socket:ReceiveDatagram();
    end
     
    if(callType == SCKCALL_SEND && err == SCKERR_OK) then
        //print("P2P - Sent.");
    end
     
    if(callType == SCKCALL_REC_DATAGRAM && err == SCKERR_OK) then
        //print("P2P - Got '" .. data .. "'  from  " .. peer .. ":" .. tostring(peerPort));
		socket:ReceiveDatagram();
		P2PRecv(tostring(data));
    end
     
    if(err != SCKERR_OK) then
		Error("P2P - Socket is dead Err: " .. err); 
    end
end

g_Socket:SetCallback(P2PCallback);
g_Socket:Bind(const_P2PListenIP, const_P2PListenPort);

function P2PShutdown()
	g_Socket:Close();
	g_Socket = nil;
end
hook.Add("ShutDown", "P2PShutdown", P2PShutdown);

function P2PEnque(plyName, plySteamID, text, adminonly)
	g_MessageID = g_MessageID + 1;
	
	local message = {};
	message[1] = g_NetworkPassword;
	message[2] = util.CRC(g_MessageID .. os.time() .. g_PeerId);
	message[3] = tonumber(os.time());
	message[4] = tostring(plyName);
	message[5] = tostring(plySteamID);
	message[6] = tostring(text);
	message[7] = tobool(adminonly);
	
	local valid, encoded = pcall(glon.encode, message);
	
	if(!valid) then
		print(encoded); 
		return; 
	end
	
	for _, v in ipairs(const_P2PPeers) do
		g_Socket:Send(encoded, tostring(v[1]), tonumber(v[2]));
	end
end

function P2PRecv(datagram)	
	if(!datagram) then return; end
	
	local valid, decoded = pcall(glon.decode, datagram);
	
	if(!valid) then
		print(decoded); 
		return; 
	end
	
	if(type(decoded) != "table") then return; end
	
	if(decoded[1] != g_NetworkPassword) then return; end
	
	// Incase we get the same message twice, for some strange reason.
	g_RecvQue[decoded[2]] = decoded;
end

local g_NextProcess = 0;
function P2PProcess()
	if(CurTime() < g_NextProcess) then return; end
	g_NextProcess = CurTime() + 1.0;
	
	// Sort so that the oldest message is on top. Remember message[3] = os.time();
	// I hope to god all your server's clocks are correct.
	table.SortByMember(g_RecvQue, 3);
	
	for _, v in pairs(g_RecvQue) do
		ChatGlobal(v[4], v[5], v[6], v[7]);
	end
	
	g_RecvQue = {};
end
hook.Add("Think", "P2PProcess", P2PProcess);

function ChatGlobal(name, steam, message, adminonly)
	ServerLog("(Global) " .. name .. "<" .. steam .. ">: " .. message);
	print("(Global) " .. name .. "<" .. steam .. ">: " .. message);
	
	local filter = RecipientFilter();
	
	if(adminonly) then
		for _, v in ipairs(player.GetAll()) do
			if(v:IsAdmin()) then
				filter:AddPlayer(v);
			end
		end
	else
		filter:AddAllPlayers();
	end
	
	umsg.Start("_GCHAT", filter);
		umsg.String(tostring(name));
		umsg.String(tostring(message));
		umsg.Bool(tobool(adminonly));
	umsg.End();
end

function P2PPlayerSay(ply, text, all)
	if(string.Left(text, string.len(const_P2PChatPrefix)) == const_P2PChatPrefix) then
		local clean = string.Trim(string.Right(text, string.len(text) - string.len(const_P2PChatPrefix)));
		ChatGlobal(ply:Nick(), ply:SteamID(), clean, false);
		P2PEnque(ply:Nick(), ply:SteamID(), clean, false);
		return "";
	end
	
	if(ply:IsAdmin() && string.Left(text, string.len(const_P2PAdminChatPrefix)) == const_P2PAdminChatPrefix) then
		local clean = string.Trim(string.Right(text, string.len(text) - string.len(const_P2PAdminChatPrefix)));
		ChatGlobal(ply:Nick(), ply:SteamID(), clean, true);
		P2PEnque(ply:Nick(), ply:SteamID(), clean, true);
		return "";
	end
end
hook.Add("PlayerSay", 1, P2PPlayerSay);