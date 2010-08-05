local SOCK_ERROR = {}
SOCK_ERROR[SCKERR_OK]					= "SCKERR_OK"
SOCK_ERROR[SCKERR_BAD]					= "SCKERR_BAD"
SOCK_ERROR[SCKERR_CONNECTION_RESET]		= "SCKERR_CONNECTION_RESET"
SOCK_ERROR[SCKERR_NOT_CONNECTED]		= "SCKERR_NOT_CONNECTED"
SOCK_ERROR[SCKERR_TIMED_OUT]			= "SCKERR_TIMED_OUT"

function WWW.Listen.Callback(sock, call, id, error, data, peer, port)
	-----------------------------------------------------------
	-- Error Checking
	-----------------------------------------------------------
	if(error != SCKERR_OK) then
		print("WWW.Listen.Callback suffered an error (" .. SOCK_ERROR[error] .. ")\n")
		sock:Close()
		WWW.Listen.Sock = nil
		return
	end
	
	-----------------------------------------------------------
	-- Socket Setup
	-----------------------------------------------------------
	if(call == SCKCALL_BIND) then
		sock:Listen(5)
	end
	
	if(call == SCKCALL_LISTEN) then
		sock:Accept()
	end
	
	-----------------------------------------------------------
	-- Acceptor
	-----------------------------------------------------------
	if(call == SCKCALL_ACCEPT) then
        WWW.Clients.Add(data)
		sock:Accept()
    end
end

function WWW.Listen.Start(ip, port)
	WWW.Listen.Sock = OOSock(IPPROTO_TCP)
	
	WWW.Listen.Sock:SetCallback(WWW.Listen.Callback)
	WWW.Listen.Sock:Bind(ip or "", port or 80)
end