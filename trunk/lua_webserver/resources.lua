local function CleanPath(path)
	return string.Replace(string.Replace(string.Replace(path or "", "..", ""), "\\", "/"), "//", "/")
end

function WWW.Resources.FileExists(filename)
	local path = "wwwroot/" .. filename
	path = CleanPath(path)
	return file.Exists(path) || file.IsDir(path)
end

function WWW.Resources.GetContent(filename, getvars)
	local path = CleanPath("wwwroot/" .. filename)
	
	if(path == "wwwroot/" || file.IsDir(path)) then
		if(file.Exists(path .. "/index.html")) then
			path = path .. "/index.html"
			path = CleanPath(path)
		elseif(file.Exists(path .. "/index.l")) then
			path = path .. "/index.l"
			path = CleanPath(path)
		else
			return "<html><head><title>Directory Listing Not Allowed</title></head><body><strong>Directory Listing Not Allowed</strong></body></html>", "text/html; charset=UTF-8"
		end
	end
	
	local fileExt = string.GetExtensionFromFilename(path)

	local mimeType = nil
	
	for _, mime in pairs(WWW.MIMETypes) do
		if(table.HasValue(mime.Extensions, string.lower(fileExt))) then
			mimeType = mime
			break
		end
	end
	
	if(!mimeType || mimeType.Allowed != true) then
		return "<html><head><title>Bad MIME Type</title></head><body><strong>Bad MIME Type</strong></body></html>", "text/html; charset=UTF-8"
	end
	
	if(mimeType.Mode == "lua") then
		local script = hIO.Read("data/" .. path)
		local content = ""
		
		_GET = {}
		for k, v in string.gmatch(getvars or "", "(%w+)=(%w+)&?") do
			_GET[k] = v
		end
		
		echo = function(text) content = content .. tostring(text) end
		RunString(script)
		echo = nil
		_GET = nil
		
		return content, mimeType.Type
	elseif(mimeType.Mode == "text") then
		return hIO.Read("data/" .. path) or "", mimeType.Type
	elseif(mimeType.Mode == "binary") then
		return hIO.Read("data/" .. path) or "", mimeType.Type
	end
	
	return "<html><head><title>Bad MIME Type</title></head><body><strong>Bad MIME Type</strong></body></html>", "text/html; charset=UTF-8"
end