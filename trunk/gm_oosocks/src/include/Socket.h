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

#ifndef __SOCKET_INCLUDED
#define __SOCKET_INCLUDED

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <string>

#ifdef WIN32
	#include <winsock2.h>
#else
	#include <unistd.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
#endif

namespace TD
{
	namespace SOCK_ERROR
	{
		enum
		{
			OK = 0,
			NOT_CONNECTED,
			CONNECTION_REST,
			TIMED_OUT,
			BAD
		};

		static int Translate(int osSpec)
		{
#ifdef WIN32
			static const int OS[] = {NULL, WSAENOTCONN, WSAECONNRESET, WSAETIMEDOUT};
#else
			static const int OS[] = {NULL, ENOTCONN, ECONNRESET, ETIMEDOUT};
#endif
			static const int TRANS[] = {OK, NOT_CONNECTED, CONNECTION_REST, TIMED_OUT};

			for(int i = 0; i < sizeof(OS)/sizeof(int); i++)
			{
				if(OS[i] == osSpec)
					return TRANS[i];
			}

			return BAD;
		};
	};

/**
 * Simple C++ socket wrapper.
 */
class Socket
{
	public:	
		
		/**
		 * Creates a new socket.
		 * 
		 * @param type Type of the socket (SOCK_STREAM / SOCK_DGRAM)
		 * @param domain Domain of the socket
		 * @param protocol Protocol to use
		 */
		Socket(int type = SOCK_STREAM, int domain = AF_INET, int protocol = 0);
		
		/**
		 * Wraps a created socket.
		 * 
		 * @param sockfd Descriptor of the socket to wrap
		 * @param flag Does nothing
		 */
		Socket(int sockfd, bool flag);
		
		/**
		 * Destructor. Doesn't closes the socket!
		 */
		virtual ~Socket();
		
		/**
		 * Bind the socket to a local port.
		 * 
		 * @param port Port to listem
		 */
		void bind(int port);
		
		/**
		 * Start to listem on the binded port
		 * 
		 * @param backlog Connection queue size
		 */
		void listen(int backlog = 5);
		
		/**
		 * Blocks and waits for a connection.
		 * 
		 * @return The socket to handle the connected client
		 */
		Socket *accept();		
		
		/**
		 * Connects to a remote host.
		 * 
		 * @param server Server name or ip
		 * @param port Port of the server to connect
		 */
		void connect(const char* server, int port);
		
		void setTimeout(unsigned int timeout);
		
		/**
		 * Gets the address of the remote computer.
		 * 
		 * @return Address of the remote computer
		 */
		const char* getPeerName();
		
		/**
		 * Gets the address of the local computer.
		 * 
		 * @return Address of the local computer
		 */
		const char* getHostName();
		
		/**
		 * Blocks and send a message. May not send all the info.
		 * 
		 * @param msg Message to send
		 * @return How much chars where sent
		 */
		int send(const char* msg);
		
		/**
		 * Blocks and send a message. May not send all the info.
		 * 
		 * @param msg Message to send
		 * @return How much chars where sent
		 */
		int send(std::string& msg);
		
		/**
		 * Blocks and send all the message.
		 * 
		 * @param msg Message to send
		 * @return Lenght of the sent data
		 */		
		int sendAll(const char* msg);
		
		/**
		 * Blocks and send all the message.
		 * 
		 * @param msg Message to send
		 * @return Lenght of the sent data
		 */		
		int sendAll(std::string& msg);
		
		/**
		 * Like sendAll(msg) + sendAll(\n)
		 * 
		 * @param msg Message to send
		 * @return Lenght of the sent data
		 */
		int sendln(const char* msg);
		
		/**
		 * Like sendAll(msg) + sendAll(\n)
		 * 
		 * @param msg Message to send
		 * @return Lenght of the sent data
		 */
		int sendln(std::string& msg);
		
		/**
		 * Blocks and receive a message.
		 * 
		 * @param len Size, in chars, of the receive buffer (max info to receive)
		 * @param flags Optional flags
		 * @return Buffer with the received message.
		 */
		std::string recv(int len, int flags, std::string &outAddr);
		
		/**
		 * Blocks and receive a message (until find a line break).
		 *
		 * @param flags Optional flags
		 * @return Buffer with the received message (without the line break)
		 */
		std::string recvln(int flags, std::string &outAddr);
		
		/**
		 * Close the socket for in / out operations.
		 */ 
		void close();

		int getLastError();

	private:
		/**
		 * Returns the ip of a host (DNS lookup).
		 * 
		 * @param server Server to lookup
		 * @return Ip of the server
		 */
		struct in_addr* getHostByName(const char* server);
	
		int sockfd;
		struct sockaddr_in my_addr;
		
		int lastError;

		bool checkError(int returnCode, int ok = 0);

		int timeout;
};

}

#endif
