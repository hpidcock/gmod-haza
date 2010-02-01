// Lua Bone Animation API
// Credits: 	JetBoom
//		Haza
if CLIENT then return end

include("sh_boneanimlib.lua")
AddCSLuaFile("cl_boneanimlib.lua")
AddCSLuaFile("sh_boneanimlib.lua")

-- These are here for testing purposes. You should NOT use these in any kind of production unless absolutely needed.
-- These are unreliable and unpredicted. All Lua animations should be set on the client.

local meta = _R["Entity"]
function meta:ResetLuaAnimation(sAnimation)
	umsg.Start("resetluaanim")
		umsg.Entity(self)
		umsg.String(sAnimation)
	umsg.End()
end

function meta:SetLuaAnimation(sAnimation)
	umsg.Start("setluaanim")
		umsg.Entity(self)
		umsg.String(sAnimation)
	umsg.End()
end

function meta:StopLuaAnimation(sAnimation)
	umsg.Start("stopluaanim")
		umsg.Entity(self)
		umsg.String(sAnimation)
	umsg.End()
end

function meta:StopLuaAnimationGroup(sAnimation)
	umsg.Start("stopluaanimgp")
		umsg.Entity(self)
		umsg.String(sAnimation)
	umsg.End()
end

function meta:StopAllLuaAnimations()
	umsg.Start("stopallluaanim")
		umsg.Entity(self)
	umsg.End()
end
meta = nil