if SERVER then
	AddCSLuaFile("__pipe_init.lua");
	include("pipe/pipe.lua");
else
	include("pipe/pipe_cl.lua");
end