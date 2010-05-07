/*
	This file is part of TDSocket.

	TDSocket is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	TDSocket is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with TDSocket.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Socket.h"

#ifdef WIN32
	typedef int socklen_t;
#endif

namespace TD
{

Socket::Socket(int type, int domain, int protocol)
{	
	lastError = SOCK_ERROR::OK;

	sockfd = socket(domain, type, protocol);
	
	if(checkError(sockfd))
		return;

	char yes = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	
	my_addr.sin_family = domain;

	timeout = 0;
}

Socket::Socket(int new_fd, bool flag)
{
	lastError = SOCK_ERROR::OK;

	sockfd = new_fd;
	timeout = 0;
}

Socket::~Socket()
{
	#ifdef WIN32
		closesocket(sockfd);		
	#else
		::close(sockfd);
	#endif
}

void Socket::bind(int port)
{
	lastError = SOCK_ERROR::OK;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

    int n = ::bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    
	checkError(n);
}

void Socket::listen(int backlog)
{
	lastError = SOCK_ERROR::OK;
	int n = ::listen(sockfd, backlog);
	
	checkError(n);
}

Socket *Socket::accept()
{
	lastError = SOCK_ERROR::OK;
	fd_set read;
	memset(&read, NULL, sizeof(fd_set));
	FD_SET(sockfd, &read);

	timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;

	select(0, &read, 0, 0, &t);

	if(!(FD_ISSET(sockfd, &read)))
		return NULL;

	int new_fd;
	struct sockaddr_in their_addr;
	
	socklen_t size = sizeof(their_addr);

	new_fd = ::accept(sockfd, (struct sockaddr*)&their_addr, &size);
	
	if(checkError(new_fd))
		return NULL;

    return new Socket(new_fd, true);
}

struct in_addr* Socket::getHostByName(const char* server)
{
	lastError = SOCK_ERROR::OK;
	struct hostent *h;
	h = gethostbyname(server);
	
	if(checkError(h == NULL, 1))
		return NULL;

	return (struct in_addr*)h->h_addr;	
}

const char* Socket::getPeerName()
{	
	lastError = SOCK_ERROR::OK;
	struct sockaddr_in peer;
	socklen_t size = sizeof(peer);
	
	int n = getpeername(sockfd, (struct sockaddr*)&peer, &size);
	
	if(checkError(n))
		return "";
	
	return inet_ntoa(peer.sin_addr);
}

const char* Socket::getHostName()
{
	lastError = SOCK_ERROR::OK;
	char name[256];
	
	int n = gethostname(name, sizeof(name));
	
	if(checkError(n))
		return "";

	in_addr *addr = getHostByName(name);

	if(addr == NULL)
		return "";
	
	return inet_ntoa(*addr);
}

void Socket::connect(const char* server, int port)
{
	lastError = SOCK_ERROR::OK;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr = *getHostByName(server);
    memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
    
    int n = ::connect(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    
	checkError(n);
}

int Socket::send(std::string& msg)
{
	lastError = SOCK_ERROR::OK;
	int ret;
 	ret = ::send(sockfd, msg.c_str(), msg.size(), 0);

	checkError(ret, 1);

    return ret;
}

void Socket::setTimeout(unsigned int timeout)
{
	this->timeout = timeout;
}

int Socket::send(const char* msg)
{
	lastError = SOCK_ERROR::OK;
	int ret;
 	ret = ::send(sockfd, msg, strlen(msg), 0);
	checkError(ret, 1);

    return ret;
}

int Socket::sendAll(std::string& msg)
{
	lastError = SOCK_ERROR::OK;
	int total = 0;
	int len = msg.size();
    int bytesleft = len;
    int n = -1;

    while(total < len)
    {
    	n = ::send(sockfd, msg.c_str() + total, bytesleft, 0);
        if(checkError(n, 1))
			return len;
        total += n;
        bytesleft -= n;
    }
    
    return len;
}

int Socket::sendAll(const char* msg)
{
	lastError = SOCK_ERROR::OK;
	int total = 0;
	int len = strlen(msg);
    int bytesleft = len;
    int n = -1;

    while(total < len)
    {
    	n = ::send(sockfd, msg + total, bytesleft, 0);
        if(checkError(n, 1))
			return len;
        total += n;
        bytesleft -= n;
    }

    return len;
}

int Socket::sendln(const char* msg)
{
	return (sendAll(msg) + sendAll("\n"));
}

int Socket::sendln(std::string& msg)
{
	return (sendAll(msg) + sendAll("\n"));
}

std::string Socket::recv(int len, int flags, std::string &outAddr)
{
	lastError = SOCK_ERROR::OK;
	fd_set read;
	memset(&read, NULL, sizeof(fd_set));
	FD_SET(sockfd, &read);

	timeval t;
	t.tv_sec = 0;
	t.tv_usec = timeout;

	select(0, &read, 0, 0, &t);

	if(!(FD_ISSET(sockfd, &read)))
		return "";

	int count = 0;
	char *buffer = (char *)malloc(len + 1);
	sockaddr addr;
	int addrSz = sizeof(sockaddr);

	int n = ::recvfrom(sockfd, buffer, len, 0, &addr, &addrSz);

	checkError(n);

	if(n >= 0)
		buffer[n] = '\0';

	std::string retStr = buffer;

	free(buffer);

	outAddr = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);

	return retStr;
}

std::string Socket::recvln(int flags, std::string &outAddr)
{
	lastError = SOCK_ERROR::OK;
	fd_set read;
	memset(&read, NULL, sizeof(fd_set));
	FD_SET(sockfd, &read);

	timeval t;
	t.tv_sec = 0;
	t.tv_usec = timeout;

	select(0, &read, 0, 0, &t);

	if(!(FD_ISSET(sockfd, &read)))
		return "";

	int count = 0;
	char buffer = 0;

	std::string data = "";

	sockaddr addr;
	int addrSz = sizeof(sockaddr);

	while((count = ::recvfrom(sockfd, &buffer, 1, 0, &addr, &addrSz)) == 1)
	{
		if(buffer == '\n')
			return data;

		data.append(1, buffer);

		addrSz = sizeof(sockaddr);
	}

	checkError(count);

	outAddr = inet_ntoa(((sockaddr_in *)&addr)->sin_addr);

	return data;
}

void Socket::close()
{
	lastError = SOCK_ERROR::OK;

}

bool Socket::checkError(int returnCode, int ok)
{
	if(returnCode >= ok)
		return false;

#ifdef WIN32
	int error = WSAGetLastError();
#else
	int error = errno;
#endif

	lastError = SOCK_ERROR::Translate(error);

	return lastError != SOCK_ERROR::OK;
}

int Socket::getLastError()
{
	return lastError;
}

}

