if SERVER then
	AddCSLuaFile("init.lua");
	include("pipe/pipe.lua");
else
	include("pipe/pipe_cl.lua");
end