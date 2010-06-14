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
	
	if( self.Mesh && table.Count(self.Mesh) > 0 ) then
	
		self:SetModel( "models/dav0r/hoverball.mdl" )
		self:PhysicsInit( SOLID_VPHYSICS )
		self:SetMoveType( MOVETYPE_VPHYSICS )
		self:SetSolid( SOLID_VPHYSICS )
	
		local phys = self:GetPhysicsObject()
		
		if(phys:IsValid()) then
		
			phys:RebuildFromConvexs(self:GetPos(), self:GetAngles(), self.Mass, 0.001, 0.001, 1, 1, self.Mesh)
			
			phys = nil
		
		end
		
	end
	
end

/*---------------------------------------------------------
   Name: BuildWeld
---------------------------------------------------------*/
function ENT:BuildWeld(entTable)
	
	for _, v in pairs(entTable) do
	
		self:MergeEntity(v)
	
	end
	
end

/*---------------------------------------------------------
   Name: MergeEntity
---------------------------------------------------------*/
function ENT:MergeEntity(ent)

	if(ent:GetClass() != "prop_physics" || ent:GetParent():IsValid()) then
	
		return
	
	end

	self.Mesh = self.Mesh or {}
	self.Children = self.Children or {}
	self.Mass = self.Mass or 0
	
	local delta = ent:GetPos() - self:GetPos()
	local phys = ent:GetPhysicsObject()
	
	if( phys:IsValid() ) then
	
		constraint.RemoveAll(ent)
	
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

/*---------------------------------------------------------
   Name: OnRestore
---------------------------------------------------------*/
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

/*---------------------------------------------------------
   Name: PreEntityCopy
---------------------------------------------------------*/
function ENT:PreEntityCopy()

	local info = {}
	
	info.Children = {}
	
	for _, v in pairs(self.Children) do
	
		local child = {}
		child.Class = v:GetClass()
		child.Model = v:GetModel()
		child.Pos = v:GetPos() - self:GetPos()
		child.Pos:Rotate(-1 * self:GetAngles())
		child.Ang = v:GetAngles() - self:GetAngles()
		child.Mat = v:GetMaterial()
		child.Skin = v:GetSkin()
		
		table.insert(info.Children, child)
		
	end
	
	info.Mass = self.Mass
	
	info.Frozen = !self:GetPhysicsObject():IsMoveable()
	
	duplicator.StoreEntityModifier(self.Entity, "PolyDupe", info)
	
end

/*---------------------------------------------------------
   Name: PostEntityPaste
---------------------------------------------------------*/
function ENT:PostEntityPaste(ply, ent, createdEnts)

	if(ent.EntityMods and ent.EntityMods.PolyDupe) then
	
		local entList = {}
		
		for _, v in pairs(ent.EntityMods.PolyDupe.Children) do
			
			local prop = ents.Create(v.Class)
			
			prop:SetModel(v.Model)
			
			local pos = Vector(v.Pos.x, v.Pos.y, v.Pos.z)
			pos:Rotate(self:GetAngles())
			pos = pos + self:GetPos()
			
			prop:SetPos(pos)
			prop:SetAngles(v.Ang + self:GetAngles())
			
			prop:Spawn()
			
			prop:SetMaterial(v.Mat)
			prop:SetSkin(v.Skin)
			
			if(SPropProtection) then
			
				SPropProtection.PlayerMakePropOwner(ply, prop)
				
			end
			
			table.insert(entList, prop)
			
		end
		
		self.Mass = ent.EntityMods.PolyDupe.Mass
		
		self:BuildWeld(entList)
		
		self:Spawn()
		
		if(ent.EntityMods.PolyDupe.Frozen) then
		
			ent:GetPhysicsObject():EnableMotion(false)
		
		end
		
	end
	
end
