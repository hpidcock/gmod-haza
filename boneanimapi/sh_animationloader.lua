require("glon")

if SERVER then
AddCSLuaFile("sh_animationloader.lua")
include("boneanimlib.lua")
else
include("cl_boneanimlib.lua")
end

g_BoneList = {
	"ValveBiped.Bip01_Pelvis",
	"ValveBiped.Bip01_Spine",
	"ValveBiped.Bip01_Spine1",
	"ValveBiped.Bip01_Spine2",
	"ValveBiped.Bip01_Spine4",
	"ValveBiped.Bip01_Neck1",
	"ValveBiped.Bip01_Head1",
	"ValveBiped.Bip01_R_Clavicle",
	"ValveBiped.Bip01_R_UpperArm",
	"ValveBiped.Bip01_R_Forearm",
	"ValveBiped.Bip01_R_Hand",
	"ValveBiped.Bip01_L_Clavicle",
	"ValveBiped.Bip01_L_UpperArm",
	"ValveBiped.Bip01_L_Forearm",
	"ValveBiped.Bip01_L_Hand",
	"ValveBiped.Bip01_R_Thigh",
	"ValveBiped.Bip01_R_Calf",
	"ValveBiped.Bip01_R_Foot",
	"ValveBiped.Bip01_R_Toe0",
	"ValveBiped.Bip01_L_Thigh",
	"ValveBiped.Bip01_L_Calf",
	"ValveBiped.Bip01_L_Foot",
	"ValveBiped.Bip01_L_Toe0"}

function AnimLoader_LoadAnimFile(filename)
	if(!file.Exists(filename)) then return end

	local anim = glon.decode(file.Read(filename))
	
	AnimLoader_CompileAnimationData(anim.AnimationData)
	
	RegisterLuaAnimation(anim.AnimationName, anim.AnimationData)
	
	if SERVER then
		resource.AddFile(filename)
	end
end

function AnimLoader_CompileAnimationData(animdata)
	local length = 0
	local modifierList = {MU = 0, MR = 0, MF = 0, RU = 0, RR = 0, RF = 0}
	for _, frame in pairs(animdata.FrameData) do
		for _, bone in pairs(g_BoneList) do
			frame.BoneInfo[bone] = frame.BoneInfo[bone] or {}
			for modifier, _ in pairs(modifierList) do
				frame.BoneInfo[bone][modifier] = frame.BoneInfo[bone][modifier] or 0
			end
		end
		length = length + (1/frame.FrameRate)
	end
	if(animdata.Type == TYPE_POSTURE) then 
		animdata.TimeToArrive = length
	end
end

// Load animations here
// They are automatically loaded on the server and the client, and they are automatically sent to the client.
// All you need to do is include sh_animationloader.lua server side and client side.

//AnimLoader_LoadAnimFile("icrp/test.txt")

// To start the animation on a player do, ply:ResetLuaAnimation("animationName")
// animationName is the name you put in when you went New Animation in the editor