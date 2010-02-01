// Lua Bone Animation API
// Credits: 	JetBoom
//		Haza
if SERVER then return end

include("sh_boneanimlib.lua")

usermessage.Hook("resetluaanim", function(um)
	local ent = um:ReadEntity()
	local anim = um:ReadString()
	if ent:IsValid() then
		ent:ResetLuaAnimation(anim)
	end
end)

usermessage.Hook("setluaanim", function(um)
	local ent = um:ReadEntity()
	local anim = um:ReadString()
	if ent:IsValid() then
		ent:SetLuaAnimation(anim)
	end
end)

usermessage.Hook("stopluaanim", function(um)
	local ent = um:ReadEntity()
	local anim = um:ReadString()
	if ent:IsValid() then
		ent:StopLuaAnimation(anim)
	end
end)

usermessage.Hook("stopluaanimgp", function(um)
	local ent = um:ReadEntity()
	local animgroup = um:ReadString()
	if ent:IsValid() then
		ent:StopLuaAnimationGroup(animgroup)
	end
end)

usermessage.Hook("stopallluaanim", function(um)
	local ent = um:ReadEntity()
	if ent:IsValid() then
		ent:StopAllLuaAnimations()
	end
end)

local TYPE_GESTURE = TYPE_GESTURE
local TYPE_POSTURE = TYPE_POSTURE
local TYPE_STANCE = TYPE_STANCE
local TYPE_SEQUENCE = TYPE_SEQUENCE

local INTP_LINEAR = INTP_LINEAR
local INTP_CUBIC = INTP_CUBIC

local Animations = GetLuaAnimations()

local tOrigin = {MU = 0, MR = 0, MF = 0, RU = 0, RR = 0, RF = 0}

local function LinearInterpolation(a, b, frac)
	return a + (b - a) * frac
end

// Thanks to deco da coder for part of this routine.
// A fit of 1 will be entirely linear, 0 will be entirely cubic.
local function CubicInterpolation(y0, y1, y2, y3, frac, fit)
	local a0 = y3 - y2 - y0 + y1
	local a1 = y0 - y1 - a0
	local a2 = y2 - y0
	return ((a0*frac^3 + a1*frac^2 + a2*frac + y1) * (2 - 2*fit) + LinearInterpolation(y1, y2, frac) * (fit * 2)) / 2
end

local function LuaBuildBonePositions(pl, iNumBones, iNumPhysBones)
	local tLuaAnimations = pl.LuaAnimations
	for sGestureName, tGestureTable in pairs(tLuaAnimations) do
		local iCurFrame = tGestureTable.Frame
		local tFrameData = tGestureTable.FrameData[iCurFrame]
		local fFrameDelta = tGestureTable.FrameDelta
		
		local iInterpolationType = tGestureTable.Interpolation
		
		// Previous Frames and Next frame for interpolation
		local tPrevFrameData = tGestureTable.FrameData[tGestureTable.LastFrame] or {BoneInfo = {}}
		local tPrevPrevFrameData = tGestureTable.FrameData[(tGestureTable.LastFrame or 0) - 1] or tPrevFrameData
		
		local tNextFrameData = tGestureTable.FrameData[iCurFrame + 1] or tFrameData
		
		if tGestureTable.ShouldPlay and not tGestureTable.ShouldPlay(pl, sGestureName, tGestureTable, iCurFrame, tFrameData, fFrameDelta) then
			pl:StopLuaAnimation(sGestureName)
		elseif not tGestureTable.PreCallback or not tGestureTable.PreCallback(pl, sGestureName, tGestureTable, iCurFrame, tFrameData, fFrameDelta) then
			if tGestureTable.Type == TYPE_GESTURE then
				local fFrameDelta = tGestureTable.FrameDelta
				for iBoneIDS, tBoneInfo in pairs(tFrameData.BoneInfo) do
					local iBoneID = iBoneIDS
					if type(iBoneID) ~= "number" then
						iBoneID = pl:LookupBone(iBoneID)
					end

					local vCurBonePos, aCurBoneAng = pl:GetBonePosition(iBoneID)
					if vCurBonePos then
						local mBoneMatrix = pl:GetBoneMatrix(iBoneID)
						if not tBoneInfo.Callback or not tBoneInfo.Callback(pl, iNumBones, iNumPhysBones, mBoneMatrix, iBoneID, vCurBonePos, aCurBoneAng, fFrameDelta) then
							local tBoneInfoLast = tPrevFrameData.BoneInfo[iBoneIDS] or tOrigin
							local tBoneInfoLastLast = tPrevPrevFrameData.BoneInfo[iBoneIDS] or tBoneInfoLast
							local tBoneInfoNext = tNextFrameData.BoneInfo[iBoneIDS] or tBoneInfo
							
							local vUp = aCurBoneAng:Up()
							local vRight = aCurBoneAng:Right()
							local vForward = aCurBoneAng:Forward()
							
							if(iInterpolationType == INTP_LINEAR) then
								mBoneMatrix:Translate(LinearInterpolation(tBoneInfoLast.MU, tBoneInfo.MU, fFrameDelta) * vUp +
													  LinearInterpolation(tBoneInfoLast.MR, tBoneInfo.MR, fFrameDelta) * vRight +
													  LinearInterpolation(tBoneInfoLast.MF, tBoneInfo.MF, fFrameDelta) * vForward)
								mBoneMatrix:Rotate(Angle(LinearInterpolation(tBoneInfoLast.RR, tBoneInfo.RR, fFrameDelta), 
														 LinearInterpolation(tBoneInfoLast.RU, tBoneInfo.RU, fFrameDelta), 
														 LinearInterpolation(tBoneInfoLast.RF, tBoneInfo.RF, fFrameDelta)))
							elseif(iInterpolationType == INTP_CUBIC) then
								local fit = tBoneInfo.CubicFit or 0.5
								mBoneMatrix:Translate(CubicInterpolation(tBoneInfoLastLast.MU, tBoneInfoLast.MU, tBoneInfo.MU, tBoneInfoNext.MU, fFrameDelta, fit) * vUp +
													  CubicInterpolation(tBoneInfoLastLast.MR, tBoneInfoLast.MR, tBoneInfo.MR, tBoneInfoNext.MR, fFrameDelta, fit) * vRight +
													  CubicInterpolation(tBoneInfoLastLast.MF, tBoneInfoLast.MF, tBoneInfo.MF, tBoneInfoNext.MF, fFrameDelta, fit) * vForward)
								mBoneMatrix:Rotate(Angle(CubicInterpolation(tBoneInfoLastLast.RR, tBoneInfoLast.RR, tBoneInfo.RR, tBoneInfoNext.RR, fFrameDelta, fit), 
														 CubicInterpolation(tBoneInfoLastLast.RU, tBoneInfoLast.RU, tBoneInfo.RU, tBoneInfoNext.RU, fFrameDelta, fit), 
														 CubicInterpolation(tBoneInfoLastLast.RF, tBoneInfoLast.RF, tBoneInfo.RF, tBoneInfoNext.RF, fFrameDelta, fit)))
							else
								ErrorNoHalt("Unknown interpolation type " .. tonumber(iInterpolationType))
							end
							pl:SetBoneMatrix(iBoneID, mBoneMatrix)
						end
					end
				end

				tGestureTable.FrameDelta = tGestureTable.FrameDelta + FrameTime() * tFrameData.FrameRate
				if tGestureTable.FrameDelta > 1 then
					tGestureTable.LastFrame = iCurFrame
					tGestureTable.Frame = iCurFrame + 1
					tGestureTable.FrameDelta = 0
					if tGestureTable.Frame > #tGestureTable.FrameData then
						pl:StopLuaAnimation(sGestureName)
					end
				end
			elseif tGestureTable.Type == TYPE_POSTURE then
				if fFrameDelta < 1 and tGestureTable.TimeToArrive then
					fFrameDelta = math.min(1, fFrameDelta + FrameTime() * (1 / tGestureTable.TimeToArrive))
					tGestureTable.FrameDelta = fFrameDelta
				end

				for iBoneID, tBoneInfo in pairs(tFrameData.BoneInfo) do
					if type(iBoneID) ~= "number" then
						iBoneID = pl:LookupBone(iBoneID)
					end

					local vCurBonePos, aCurBoneAng = pl:GetBonePosition(iBoneID)
					if vCurBonePos then
						local mBoneMatrix = pl:GetBoneMatrix(iBoneID)
						if not tBoneInfo.Callback or not tBoneInfo.Callback(pl, iNumBones, iNumPhysBones, mBoneMatrix, iBoneID, vCurBonePos, aCurBoneAng, fFrameDelta) then
							local vUp = aCurBoneAng:Up()
							local vRight = aCurBoneAng:Right()
							local vForward = aCurBoneAng:Forward()
							mBoneMatrix:Translate(fFrameDelta * tBoneInfo.MU * vUp + fFrameDelta * tBoneInfo.MR * vRight + fFrameDelta * tBoneInfo.MF * vForward)
							mBoneMatrix:Rotate(Angle(fFrameDelta * tBoneInfo.RR, fFrameDelta * tBoneInfo.RU, fFrameDelta * tBoneInfo.RF))
							pl:SetBoneMatrix(iBoneID, mBoneMatrix)
						end
					end
				end
			elseif tGestureTable.Type == TYPE_STANCE then
				local fFrameDelta = tGestureTable.FrameDelta
				for iBoneIDS, tBoneInfo in pairs(tFrameData.BoneInfo) do
					local iBoneID = iBoneIDS
					if type(iBoneID) ~= "number" then
						iBoneID = pl:LookupBone(iBoneID)
					end

					local vCurBonePos, aCurBoneAng = pl:GetBonePosition(iBoneID)
					if vCurBonePos then
						local mBoneMatrix = pl:GetBoneMatrix(iBoneID)
						if not tBoneInfo.Callback or not tBoneInfo.Callback(pl, iNumBones, iNumPhysBones, mBoneMatrix, iBoneID, vCurBonePos, aCurBoneAng, fFrameDelta) then
							local tBoneInfoLast = tPrevFrameData.BoneInfo[iBoneIDS] or tOrigin
							local tBoneInfoLastLast = tPrevPrevFrameData.BoneInfo[iBoneIDS] or tBoneInfoLast
							local tBoneInfoNext = tNextFrameData.BoneInfo[iBoneIDS] or tBoneInfo
							
							local vUp = aCurBoneAng:Up()
							local vRight = aCurBoneAng:Right()
							local vForward = aCurBoneAng:Forward()
							
							if(iInterpolationType == INTP_LINEAR) then
								mBoneMatrix:Translate(LinearInterpolation(tBoneInfoLast.MU, tBoneInfo.MU, fFrameDelta) * vUp +
													  LinearInterpolation(tBoneInfoLast.MR, tBoneInfo.MR, fFrameDelta) * vRight +
													  LinearInterpolation(tBoneInfoLast.MF, tBoneInfo.MF, fFrameDelta) * vForward)
								mBoneMatrix:Rotate(Angle(LinearInterpolation(tBoneInfoLast.RR, tBoneInfo.RR, fFrameDelta), 
														 LinearInterpolation(tBoneInfoLast.RU, tBoneInfo.RU, fFrameDelta), 
														 LinearInterpolation(tBoneInfoLast.RF, tBoneInfo.RF, fFrameDelta)))
							elseif(iInterpolationType == INTP_CUBIC) then
								local fit = tBoneInfo.CubicFit or 0.5
								mBoneMatrix:Translate(CubicInterpolation(tBoneInfoLastLast.MU, tBoneInfoLast.MU, tBoneInfo.MU, tBoneInfoNext.MU, fFrameDelta, fit) * vUp +
													  CubicInterpolation(tBoneInfoLastLast.MR, tBoneInfoLast.MR, tBoneInfo.MR, tBoneInfoNext.MR, fFrameDelta, fit) * vRight +
													  CubicInterpolation(tBoneInfoLastLast.MF, tBoneInfoLast.MF, tBoneInfo.MF, tBoneInfoNext.MF, fFrameDelta, fit) * vForward)
								mBoneMatrix:Rotate(Angle(CubicInterpolation(tBoneInfoLastLast.RR, tBoneInfoLast.RR, tBoneInfo.RR, tBoneInfoNext.RR, fFrameDelta, fit), 
														 CubicInterpolation(tBoneInfoLastLast.RU, tBoneInfoLast.RU, tBoneInfo.RU, tBoneInfoNext.RU, fFrameDelta, fit), 
														 CubicInterpolation(tBoneInfoLastLast.RF, tBoneInfoLast.RF, tBoneInfo.RF, tBoneInfoNext.RF, fFrameDelta, fit)))
							else
								ErrorNoHalt("Unknown interpolation type " .. tonumber(iInterpolationType))
							end
							pl:SetBoneMatrix(iBoneID, mBoneMatrix)
						end
					end
				end

				tGestureTable.FrameDelta = tGestureTable.FrameDelta + FrameTime() * tFrameData.FrameRate
				if tGestureTable.FrameDelta > 1 then
					tGestureTable.LastFrame = iCurFrame
					tGestureTable.Frame = iCurFrame + 1
					tGestureTable.FrameDelta = 0
					if tGestureTable.Frame > #tGestureTable.FrameData then
						tGestureTable.Frame = tGestureTable.RestartFrame or 1
					end
				end
			else
				local fFrameDelta = tGestureTable.FrameDelta
				for iBoneIDS, tBoneInfo in pairs(tFrameData.BoneInfo) do
					local iBoneID = iBoneIDS
					if type(iBoneID) ~= "number" then
						iBoneID = pl:LookupBone(iBoneID)
					end

					local vCurBonePos, aCurBoneAng = pl:GetBonePosition(iBoneID)
					if vCurBonePos then
						local mBoneMatrix = pl:GetBoneMatrix(iBoneID)
						if not tBoneInfo.Callback or not tBoneInfo.Callback(pl, iNumBones, iNumPhysBones, mBoneMatrix, iBoneID, vCurBonePos, aCurBoneAng, fFrameDelta) then
							local tBoneInfoLast = tPrevFrameData.BoneInfo[iBoneIDS] or tOrigin
							local tBoneInfoLastLast = tPrevPrevFrameData.BoneInfo[iBoneIDS] or tBoneInfoLast
							local tBoneInfoNext = tNextFrameData.BoneInfo[iBoneIDS] or tBoneInfo
							
							local vUp = aCurBoneAng:Up()
							local vRight = aCurBoneAng:Right()
							local vForward = aCurBoneAng:Forward()
							
							if(iInterpolationType == INTP_LINEAR) then
								mBoneMatrix:Translate(LinearInterpolation(tBoneInfoLast.MU, tBoneInfo.MU, fFrameDelta) * vUp +
													  LinearInterpolation(tBoneInfoLast.MR, tBoneInfo.MR, fFrameDelta) * vRight +
													  LinearInterpolation(tBoneInfoLast.MF, tBoneInfo.MF, fFrameDelta) * vForward)
								mBoneMatrix:Rotate(Angle(LinearInterpolation(tBoneInfoLast.RR, tBoneInfo.RR, fFrameDelta), 
														 LinearInterpolation(tBoneInfoLast.RU, tBoneInfo.RU, fFrameDelta), 
														 LinearInterpolation(tBoneInfoLast.RF, tBoneInfo.RF, fFrameDelta)))
							elseif(iInterpolationType == INTP_CUBIC) then
								local fit = tBoneInfo.CubicFit or 0.5
								mBoneMatrix:Translate(CubicInterpolation(tBoneInfoLastLast.MU, tBoneInfoLast.MU, tBoneInfo.MU, tBoneInfoNext.MU, fFrameDelta, fit) * vUp +
													  CubicInterpolation(tBoneInfoLastLast.MR, tBoneInfoLast.MR, tBoneInfo.MR, tBoneInfoNext.MR, fFrameDelta, fit) * vRight +
													  CubicInterpolation(tBoneInfoLastLast.MF, tBoneInfoLast.MF, tBoneInfo.MF, tBoneInfoNext.MF, fFrameDelta, fit) * vForward)
								mBoneMatrix:Rotate(Angle(CubicInterpolation(tBoneInfoLastLast.RR, tBoneInfoLast.RR, tBoneInfo.RR, tBoneInfoNext.RR, fFrameDelta, fit), 
														 CubicInterpolation(tBoneInfoLastLast.RU, tBoneInfoLast.RU, tBoneInfo.RU, tBoneInfoNext.RU, fFrameDelta, fit), 
														 CubicInterpolation(tBoneInfoLastLast.RF, tBoneInfoLast.RF, tBoneInfo.RF, tBoneInfoNext.RF, fFrameDelta, fit)))
							else
								ErrorNoHalt("Unknown interpolation type " .. tonumber(iInterpolationType))
							end
							pl:SetBoneMatrix(iBoneID, mBoneMatrix)
						end
					end
				end

				tGestureTable.FrameDelta = tGestureTable.FrameDelta + FrameTime() * tFrameData.FrameRate
				if tGestureTable.FrameDelta > 1 then
					tGestureTable.LastFrame = iCurFrame
					tGestureTable.Frame = iCurFrame + 1
					tGestureTable.FrameDelta = 0
					if tGestureTable.Frame > #tGestureTable.FrameData then
						tGestureTable.Frame = tGestureTable.RestartFrame or 1
					end
				end
			end

			if tGestureTable.Callback then
				tGestureTable.Callback(pl, sGestureName, tGestureTable, iCurFrame, tFrameData, fFrameDelta)
			end
		end
	end
end

hook.Add("UpdateAnimation", "LuaAnimationSequenceReset", function(pl)
	if pl.InSequence then
		pl:SetSequence("reference")
	end
end)

local meta = _R["Entity"]
function meta:ResetLuaAnimation(sAnimation)
	local animtable = Animations[sAnimation]
	if animtable then
		self.LuaAnimations = self.LuaAnimations or {}
		local desgroup = animtable.Group
		if desgroup then
			for animname, tab in pairs(self.LuaAnimations) do
				if tab.Group == desgroup then
					self.LuaAnimations[animname] = nil
				end
			end
		end

		local framedelta = 0
		if animtable.Type == TYPE_POSTURE and not animtable.TimeToArrive then
			framedelta = 1
		end

		if animtable.Type == TYPE_SEQUENCE then
			self.InSequence = true
		end

		self.LuaAnimations[sAnimation] = {Frame = animtable.StartFrame or 1, FrameDelta = framedelta, FrameData = animtable.FrameData, Type = animtable.Type, RestartFrame = animtable.RestartFrame, TimeToArrive = animtable.TimeToArrive, Callback = animtable.Callback, ShouldPlay = animtable.ShouldPlay, PreCallback = animtable.PreCallback, Interpolation = animtable.Interpolation}
		self.BuildBonePositions = LuaBuildBonePositions
	end
end

function meta:SetLuaAnimation(sAnimation)
	if self.LuaAnimations and self.LuaAnimations[sAnimation] then return end

	self:ResetLuaAnimation(sAnimation)
end

function meta:StopLuaAnimation(sAnimation)
	local anims = self.LuaAnimations
	if anims and anims[sAnimation] then
		if anims[sAnimation].Type == TYPE_SEQUENCE then
			local count = 0
			for _, tab in pairs(anims) do
				if tab.Type == TYPE_SEQUENCE then
					count = count + 1
				end
			end
			if count <= 1 then
				self.InSequence = nil
			end
		end

		anims[sAnimation] = nil
		if table.Count(anims) <= 0 then
			self.LuaAnimations = nil
			self.BuildBonePositions = nil
		end
	end
end

function meta:StopLuaAnimationGroup(sGroup)
	local tAnims = self.LuaAnimations
	if tAnims then
		for animname, animtable in pairs(tAnims) do
			if animtable.Group == sGroup then
				self:StopLuaAnimation(animname)
			end
		end
	end
end

function meta:StopAllLuaAnimations()
	if self.LuaAnimations then
		for name in pairs(self.LuaAnimations) do
			self:StopLuaAnimation(name)
		end
	end
end
meta = nil