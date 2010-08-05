local SOCK_ERROR = {}
SOCK_ERROR[SCKERR_OK]					= "SCKERR_OK"
SOCK_ERROR[SCKERR_BAD]					= "SCKERR_BAD"
SOCK_ERROR[SCKERR_CONNECTION_RESET]		= "SCKERR_CONNECTION_RESET"
SOCK_ERROR[SCKERR_NOT_CONNECTED]		= "SCKERR_NOT_CONNECTED"
SOCK_ERROR[SCKERR_TIMED_OUT]			= "SCKERR_TIMED_OUT"

local HTTPSTATE_RECV_REQUEST = 1
local HTTPSTATE_RECV_HEADERS = 2
local HTTPSTATE_SEND = 3
local HTTPSTATE_CLOSE = 4

WWW.HTTP = {}
WWW.Clients.List = {}

function WWW.HTTP.HEAD(sock, resource, version, headers)
	local response = ""
	local fileName = string.Explode("?", resource)[1]
	
	if(WWW.Resources.FileExists(fileName)) then
		response = "HTTP/1.1 200 OK\r\n"
		response = response .. "Connection: " .. headers["Connection"] .. "\r\n"
		response = response .. "\r\n"
	else
		response = "HTTP/1.1 404 Not Found\r\n"
		response = response .. "Connection: " .. headers["Connection"] .. "\r\n"
		response = response .. "\r\n"
	end

	sock:Send(response)
	
	if(headers["Connection"] == "keep") then
		WWW.Clients.List[WWW.Clients.FindSockIndex(sock)].State = HTTPSTATE_RECV_REQUEST
		sock:ReceiveLine()
	else
		WWW.Clients.List[WWW.Clients.FindSockIndex(sock)].State = HTTPSTATE_CLOSE
	end
end

function WWW.HTTP.GET(sock, resource, version, headers)
	local content = ""
	local response = ""
	local mimeType = "text/html; charset=UTF-8"
	
	local exploded = string.Explode("?", resource)
	local fileName = exploded[1]
	local getVars = exploded[2]
	
	if(WWW.Resources.FileExists(fileName)) then
		response = "HTTP/1.1 200 OK\r\n"
		content, mimeType = WWW.Resources.GetContent(fileName, getVars)
	else
		response = "HTTP/1.1 404 Not Found\r\n"
		content = "<html><head><title>404 Not Found</title></head><body>404 Not Found</body></html>"
	end
	
	response = response .. "Content-Length: " .. tostring(string.len(content)) .. "\r\n"
	response = response .. "Connection: " .. headers["Connection"] .. "\r\n"
	response = response .. "Content-Type: " .. mimeType .. "\r\n"
	response = response .. "\r\n"
	response = response .. content
	
	sock:Send(response)
	
	if(headers["Connection"] == "keep") then
		WWW.Clients.List[WWW.Clients.FindSockIndex(sock)].State = HTTPSTATE_RECV_REQUEST
		sock:ReceiveLine()
	else
		WWW.Clients.List[WWW.Clients.FindSockIndex(sock)].State = HTTPSTATE_CLOSE
	end
end

function WWW.HTTP.NOTALLOWED(sock, resource, version, headers)
	local content = "<html><head><title>405 Method Not Allowed</title></head><body>405 Method Not Allowed</body></html>"

	local response = "HTTP/1.1 405 Method Not Allowed\r\n"
	response = response .. "Content-Length: " .. tostring(string.len(content)) .. "\r\n"
	response = response .. "Connection: close\r\n"
	response = response .. "Content-Type: text/html; charset=UTF-8\r\n"
	response = response .. "\r\n"
	response = response .. content
	
	sock:Send(response)
	
	WWW.Clients.List[WWW.Clients.FindSockIndex(sock)].State = HTTPSTATE_CLOSE
end

function WWW.Clients.Callback(sock, call, id, error, data, peer, port)
	local index = WWW.Clients.FindSockIndex(sock)

	-----------------------------------------------------------
	-- Error Checking
	-----------------------------------------------------------
	if(error != SCKERR_OK) then
		print("WWW.Clients.Callback suffered an error (" .. SOCK_ERROR[error] .. ")\n")
		sock:Close()
		WWW.Clients.List[index] = nil
		return
	end

	-----------------------------------------------------------
	--  HTTP
	-----------------------------------------------------------
	local entry = WWW.Clients.List[index]
	
	if(!entry) then
		return
	end

	if(entry.State == HTTPSTATE_CLOSE) then
		sock:Close()
		WWW.Clients.List[index] = nil
		return
	end
	
	entry.Linger = SysTime() + 30

	if(call == SCKCALL_REC_LINE) then	
		if(entry.State == HTTPSTATE_RECV_REQUEST) then

			-- Request Line
			entry.Request = {}
			for v in string.gmatch(data, "[^%s]+") do
				table.insert(entry.Request, v)
			end
			
			entry.State = HTTPSTATE_RECV
			sock:ReceiveLine()

		elseif(entry.State == HTTPSTATE_RECV) then	
		
			-- All the headers
			if(data != "" && data != "\r\n" && data != "\r" && data != "\n") then

				entry.Headers = entry.Headers or {}
				
				for k, v in string.gmatch(data, "(%w+): (%w+)") do
					entry.Headers[k] = v
				end

				sock:ReceiveLine()

			else

				-- Execute Request
				entry.State = HTTPSTATE_SEND
				
				if(WWW.HTTP[entry.Request[1]]) then
					WWW.HTTP[entry.Request[1]](sock, entry.Request[2], entry.Request[3], entry.Headers)
				else
					WWW.HTTP.NOTALLOWED(sock, entry.Request[2], entry.Request[3], entry.Headers)
				end
				
			end
			
		else
			print("WWW.Clients.Callback: Receving when supposed to be sending.\n")
		end
	end
end

function WWW.Clients.FindSockIndex(sock)
	for k, v in pairs(WWW.Clients.List) do
		if(v.Sock == sock) then
			return k
		end
	end
end

function WWW.Clients.Add(sock)
	local sockEntry = {}
	sockEntry.Sock = sock
	sockEntry.Sock:SetCallback(WWW.Clients.Callback)
	sockEntry.Sock:ReceiveLine()
	
	sockEntry.State = HTTPSTATE_RECV_REQUEST
	
	sockEntry.Linger = SysTime() + 30
	
	table.insert(WWW.Clients.List, sockEntry)
end

function WWW.Clients.CheckClients()
	for k, v in pairs(WWW.Clients.List) do
		if(v.Linger < SysTime()) then
			v.Sock:Close()
			WWW.Clients.List[k] = nil
		end
	end
end
hook.Add("Think", "CheckWWWClients", WWW.Clients.CheckClients)