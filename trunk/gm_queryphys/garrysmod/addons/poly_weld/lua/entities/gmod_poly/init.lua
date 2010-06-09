--[[
	
	Developed By Sir Haza
	
	Copyright (c) Sir Haza 2010
	
]]--

hook.Add( "InitPostEntity", "Load_queryphys", function()
	require( "queryphys" )
end )

AddCSLuaFile( "cl_init.lua" )
AddCSLuaFile( "shared.lua" )
include( "shared.lua" )

/*---------------------------------------------------------
   Name: Initialize
---------------------------------------------------------*/
function ENT:Initialize()

	self:SetModel( "models/dav0r/hoverball.mdl" )
	self:PhysicsInit( SOLID_VPHYSICS )
	self:SetMoveType( MOVETYPE_VPHYSICS )
	self:SetSolid( SOLID_VPHYSICS )
	
	if( self.Mesh ) then
	
		local phys = self:GetPhysicsObject()
		
		if(phys:IsValid()) then
		
			phys:RebuildFromConvexs(self:GetPos(), self:GetAngles(), self.Mass, 0.0001, 0.0001, 1, 1, self.Mesh)
			
			phys = nil
		
		end
		
	end
	
end

function ENT:BuildWeld(entTable)
	
	for _, v in pairs(entTable) do
	
		self:MergeEntity(v)
	
	end
	
end

function ENT:MergeEntity(ent)

	if(ent:GetClass() != "prop_physics") then
	
		return
	
	end

	self.Mesh = self.Mesh or {}
	self.Children = self.Children or {}
	self.Mass = self.Mass or 0
	
	local delta = ent:GetPos() - self:GetPos()
	local phys = ent:GetPhysicsObject()
	
	if( phys:IsValid() ) then
	
		local convexCount = phys:GetConvexCount()
		
		if( convexCount and convexCount > 0 ) then
		
			local angle = phys:GetAngle()
			
			for i = 0, convexCount - 1 do
				local convex = phys:GetConvexMesh(i)
				
				for _, triangle in pairs(convex) do
				
					for index, vertex in pairs(triangle) do
					
						vertex:Rotate(angle)
						triangle[index] = vertex + delta
						
					end
					
				end
				
				table.insert(self.Mesh, convex)
			end
			
			phys:EnableCollisions(false)
			
			self.Mass = self.Mass + phys:GetMass()
			
			ent:SetParent(self)
			
			table.insert(self.Children, ent)
			
		end
		
	end
	
end

function ENT:OnRestore()

end

/*---------------------------------------------------------
   Name: OnTakeDamage
---------------------------------------------------------*/
function ENT:OnTakeDamage( dmginfo )

	self:TakePhysicsDamage( dmginfo )
	
end


/*---------------------------------------------------------
   Name: Think
---------------------------------------------------------*/
function ENT:Think()

	self:NextThink( CurTime() + 0.25 )
	
	return true
	
end

/*---------------------------------------------------------
   Name: Use
---------------------------------------------------------*/
function ENT:Use( activator, caller )

end
