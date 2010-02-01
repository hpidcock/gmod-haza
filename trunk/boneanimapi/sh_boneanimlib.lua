--[[

Bone Animations Library
Created by William "JetBoom" Moodhe (jetboom@yahoo.com / www.noxiousnet.com)
Because I wanted custom, dynamic animations.
Give credit or reference if used in your creations.

Modifications by Haza for better animations and easier and more logical creation of them

]]

TYPE_GESTURE = 0 -- Gestures are keyframed animations that use the current position and angles of the bones. They play once and then stop automatically.
TYPE_POSTURE = 1 -- Postures are static animations that use the current position and angles of the bones. They stay that way until manually stopped. Use TimeToArrive if you want to have a posture lerp.
TYPE_STANCE = 2 -- Stances are keyframed animations that use the current position and angles of the bones. They play forever until manually stopped. Use RestartFrame to specify a frame to go to if the animation ends (instead of frame 1).
TYPE_SEQUENCE = 3 -- Sequences are keyframed animations that use the origin and angles of the entity. They play forever until manually stopped. Use RestartFrame to specify a frame to go to if the animation ends (instead of frame 1).
-- You can also use StartFrame to specify a starting frame for the first loop.

INTP_LINEAR = 0
INTP_CUBIC = 1

local Animations = {}

function GetLuaAnimations()
	return Animations
end

function RegisterLuaAnimation(sName, tInfo)
	tInfo.Interpolation = tInfo.Interpolation or INTP_LINEAR
	if tInfo.FrameData then
		for iFrame, tFrame in ipairs(tInfo.FrameData) do
			for iBoneID, tBoneTable in pairs(tFrame.BoneInfo) do
				tBoneTable.MU = tBoneTable.MU or 0
				tBoneTable.MF = tBoneTable.MF or 0
				tBoneTable.MR = tBoneTable.MR or 0
				tBoneTable.RU = tBoneTable.RU or 0
				tBoneTable.RF = tBoneTable.RF or 0
				tBoneTable.RR = tBoneTable.RR or 0
			end
		end
	end
	Animations[sName] = tInfo
end