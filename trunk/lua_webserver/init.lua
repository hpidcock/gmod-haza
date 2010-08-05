require("oosocks")
require("hio")

WWW = {}
WWW.Listen = {}
WWW.Clients = {}
WWW.MIMETypes = {}
WWW.Resources = {}

function WWW.LoadMIME()
	WWW.MIMETypes = {}

	local mimeScripts = file.FindInLua("lua_webserver/mime/*.lua")
	for _, v in pairs(mimeScripts) do
		MIME = {}
			include("lua_webserver/mime/" .. v)
			table.insert(WWW.MIMETypes, MIME)
		MIME = nil
	end
end

include("resources.lua")
include("client.lua")
include("listen.lua")

WWW.LoadMIME()
WWW.Listen.Start("", 80)