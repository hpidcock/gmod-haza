--[[
	
	Developed By Sir Haza
	
	Copyright (c) Sir Haza 2010
	
]]--

ENT.RenderGroup = RENDERGROUP_BOTH

include('shared.lua')

/*---------------------------------------------------------
   Name: Initialize
---------------------------------------------------------*/
function ENT:Initialize()

	local phys = self:GetPhysicsObject()
	
	if(phys and phys:IsValid()) then
	
		phys:EnableCollisions(false)
	
	end

end


/*---------------------------------------------------------
   Name: Draw
---------------------------------------------------------*/
function ENT:Draw()

	self.BaseClass.Draw( self )	
		
end

/*---------------------------------------------------------
   Name: DrawTranslucent
   Desc: Draw translucent
---------------------------------------------------------*/
function ENT:DrawTranslucent()

	self.BaseClass.DrawTranslucent( self )
	
end


/*---------------------------------------------------------
   Name: Think
   Desc: Client Think - called every frame
---------------------------------------------------------*/
function ENT:Think()

	local mainPhys = self:GetPhysicsObject()
	
	local possibleChildren = ents.FindByClass("prop_physics")

	for _, v in pairs(possibleChildren) do
	
		if(v:GetParent() == self.Entity) then
		
			local phys = v:GetPhysicsObject()
			
			if(phys and phys:IsValid()) then
			
				phys:EnableCollisions(true)
				
				if(mainPhys:IsAsleep()) then
				
					phys:Sleep()
				
				else
				
					phys:Wake()
					
				end
			
			end
			
		end
		
	end
	
end

