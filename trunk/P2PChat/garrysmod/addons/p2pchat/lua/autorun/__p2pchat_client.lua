// Written by Haza55

if SERVER then
	AddCSLuaFile("__p2pchat_client.lua");
	return;
end

function GCHAT(data)
	local name = data:ReadString();
	local message = data:ReadString();
	local adminonly = data:ReadBool();
	
	if(adminonly) then
		chat.AddText(Color(100, 200, 0, 255), "[AGLOBAL]", Color(240, 240, 240, 255), name .. ": ", Color(255, 10, 10, 255), message);
	else
		chat.AddText(Color(200, 100, 0, 255), "[GLOBAL]", Color(240, 240, 240, 255), name .. ": ", Color(255, 10, 10, 255), message);
	end
end
usermessage.Hook("_GCHAT", GCHAT);