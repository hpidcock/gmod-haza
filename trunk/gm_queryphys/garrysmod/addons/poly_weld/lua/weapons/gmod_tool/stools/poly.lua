--[[
	
	Developed By Sir Haza
	
	Copyright (c) Sir Haza 2010
	
]]--

TOOL.Category		= "Constraints"
TOOL.Name			= "#Poly_Weld"
TOOL.Command		= nil
TOOL.ConfigName		= nil

if(CLIENT) then

	language.Add("Poly_weld", "Poly Weld")
	language.Add("Tool_poly_name", "Poly Weld")
	language.Add("Tool_poly_desc", "Permanent Welds")
	language.Add("Tool_poly_0", "Left Click to Select    Right Click to Deselect    Reload to Create Weld")
	
end

/*---------------------------------------------------------
   Name:	LeftClick
   Desc:	Select
---------------------------------------------------------*/  
function TOOL:LeftClick( trace )

	if(!trace.Entity) then return false end
	if(!trace.Entity:IsValid()) then return false end
	if(trace.Entity:IsPlayer()) then return false end
	
	if(CLIENT) then return true end

	local entList = constraint.GetAllConstrainedEntities( trace.Entity )
	
	self.Selected = self.Selected or {}
	
	for _, v in pairs(entList) do
	
		if(ValidEntity(v)) then
		
			v:SetColor(255, 0, 0, 255)
			self.Selected[v] = true
			
		end
		
	end
	
	return true
		
end

/*---------------------------------------------------------
   Name:	RightClick
   Desc:	Deselect
---------------------------------------------------------*/  
function TOOL:RightClick( trace )

	if(!trace.Entity) then return false end
	if(!trace.Entity:IsValid()) then return false end
	if(trace.Entity:IsPlayer()) then return false end

	if(CLIENT) then return true end

	local entList = constraint.GetAllConstrainedEntities(trace.Entity)
	
	self.Selected = self.Selected or {}
	
	for _, v in pairs(entList) do
	
		if(ValidEntity(v)) then
		
			v:SetColor(255, 255, 255, 255)
			
		end
		
		self.Selected[v] = nil
		
	end
	
	return true
	
end

/*---------------------------------------------------------
   Name:	Reload
   Desc:	Create Weld
---------------------------------------------------------*/  
function TOOL:Reload( trace )

	if(CLIENT) then return true end
	
	if(table.Count(self.Selected) <= 1) then return end

	local entList = {}

	for v, _ in pairs(self.Selected) do
	
		if(ValidEntity(v)) then
		
			v:SetColor(255, 255, 255, 255)
			table.insert(entList, v)
			
			undo.ReplaceEntity(v, Entity(-1))
			cleanup.ReplaceEntity(v, Entity(-1))
			
		end
		
	end
	
	self.Selected = {}
	
	local ent = ents.Create("gmod_poly")
	
	ent:SetPos(trace.HitPos)
	ent:BuildWeld(entList)
	ent:Spawn()
	
	undo.Create("prop")
		
		undo.AddEntity(ent)
		undo.SetPlayer(self:GetOwner())
		
	undo.Finish()
	
	cleanup.Add(self:GetOwner(), "Poly Weld", ent)
	
	return true
	
end
