
#include <StdFuncs.h>
#include "StdSocket.h"

#if defined(__unix__) || defined(__amigaos__)

#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define closesocket(socket) ::close(socket)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#else /* ! defined(__unix__) || defined(__amigaos__) */

#include <ws2tcpip.h>

#endif /* ! defined(__unix__) || defined(__amigaos__) */

/**
 * RSocket constructor.
 * Initialises the socket to a state ready for connection.
 *
 * @date	Wednesday 29-Jan-2020 4:47 pm, Scoot flight TR 735 to Singapore
 */

RSocket::RSocket()
{
	m_iServerSocket = m_iSocket = INVALID_SOCKET;
}

/**
 * Opens a socket in either client or server mode.
 * Depending on whether a host name is specified, this method will open a socket for use either as a client
 * or a server.  If a host is specified, it will be resolved and a connection made to the it.  The host can
 * be either a host name such as codehq.org or an IP address.  If no host is specified, the socket will be
 * opened in a state suitable for listening.
 *
 * @date	Saturday 11-Feb-2017 4:37 pm, Code HQ Habersaathstrasse
 * @param	a_pccHost		The name of the host, an IP address or NULL
 * @param	a_usPort		The port to which to connect
 * @return	KErrNone if successful
 * @return	KErrGeneral if the socket could not be opened
 * @return	KErrHostNotFound if the host could not be resolved
 */

int RSocket::open(const char *a_pccHost, unsigned short a_usPort)
{
	int RetVal;
	struct hostent *HostEnt;
	struct in_addr *InAddr;
	struct sockaddr_in SockAddr;

	RetVal = KErrGeneral;

#ifdef WIN32

	WSAData WSAData;

	if (WSAStartup(MAKEWORD(2, 2), &WSAData) == 0)

#endif /* WIN32 */

	{
		if ((m_iSocket = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET)
		{
			if (a_pccHost)
			{
				if ((HostEnt = gethostbyname(a_pccHost)) != nullptr)
				{
					InAddr = (struct in_addr *) HostEnt->h_addr_list[0];

					SockAddr.sin_family = HostEnt->h_addrtype;
					SockAddr.sin_port = htons(a_usPort);
					SockAddr.sin_addr = *InAddr;

					if (connect(m_iSocket, (struct sockaddr *) &SockAddr, sizeof(SockAddr)) >= 0)
					{
						RetVal = KErrNone;
					}
				}
				else
				{
					RetVal = KErrHostNotFound;
				}
			}
			else
			{
				/* When running as a host, enable SO_LINGER to ensure that socket is cleanly closed and can thus be */
				/* immediately reopened for the next client connection */
				struct linger Linger = { 1, 0 };

				if (setsockopt(m_iSocket, SOL_SOCKET, SO_LINGER, (const char *) &Linger, sizeof(Linger)) == 0)
				{
					RetVal = KErrNone;
				}
			}
		}
	}

	return(RetVal);
}

/**
 * Closes any open connections.
 * Closes any currently open sockets, setting them back to a disconnected state.
 *
 * @date	Saturday 11-Feb-2017 5:41 pm, Code HQ Habersaathstrasse
 */

void RSocket::close()
{
	if (m_iSocket != INVALID_SOCKET)
	{
		closesocket(m_iSocket);
		m_iSocket = INVALID_SOCKET;
	}

	if (m_iServerSocket != INVALID_SOCKET)
	{
		closesocket(m_iServerSocket);
		m_iServerSocket = INVALID_SOCKET;
	}

#ifdef WIN32

	WSACleanup();

#endif

}

/**
 * Listens for incoming connections.
 * Binds the socket to a local address and the given port number and listens for incoming connections
 * on it.  When a new connection is accepted, the connected socket will be put into use as the active
 * socket, and data can then be read from, and written to, the remote host.
 *
 * @pre		The socket has been opened with open()
 *
 * @date	Sunday 12-Feb-2017 7:53 am, Code HQ Habersaathstrasse
 * @param	a_usPort		The port on which to listen for connections
 * @return	KErrNone if successful, else KErrGeneral if host connection failed
 */

int RSocket::listen(unsigned short a_usPort)
{
	int RetVal;
	socklen_t ClientSize;
	SOCKET Socket;
	struct sockaddr_in Server, Client;

	RetVal = KErrGeneral;

	Server.sin_family = AF_INET;
	Server.sin_port = htons(a_usPort);
	Server.sin_addr.s_addr = INADDR_ANY;

	if (bind(m_iSocket, (struct sockaddr *) &Server, sizeof(Server)) != SOCKET_ERROR)
	{
		if (::listen(m_iSocket, 1) == 0)
		{
			ClientSize = sizeof(Client);
			Socket = accept(m_iSocket, (struct sockaddr *) &Client, &ClientSize);

			if (Socket != INVALID_SOCKET)
			{
				RetVal = KErrNone;

				m_iServerSocket = m_iSocket;
				m_iSocket = Socket;
			}
		}
	}

	return(RetVal);
}

/**
 * Reads data from the socket.
 * Reads the number of bytes requested from the socket.  This method can operated in a "read all" mode and
 * a "read waiting" mode, as specified by the a_bReadAll parameter.  If this is specified as true then the
 * entire number of bytes specified by a_iSize will be read, even if this involves blocking.  If this is
 * false then only the number of bytes waiting on the socket will be read.
 *
 * @pre		The socket has been opened with open()
 *
 * @date	Saturday 11-Feb-2017 5:59 pm, Code HQ Habersaathstrasse
 * @param	a_pvBuffer		Pointer to the buffer into which to read the data
 * @param	a_iSize			The number of bytes to be read
 * @param	a_bReadAll		true to read entire number of bytes specified
 * @return	The number of bytes received, -1 if an error occurred or 0 if socket was closed
 */

int RSocket::read(void *a_pvBuffer, int a_iSize, bool a_bReadAll)
{
	char *buffer = (char *) a_pvBuffer;
	int bytesToRead, retVal = 0, size;

	if (a_bReadAll)
	{
		/* Loop around, trying to read however many bytes are left to be read, starting with the total number */
		/* of bytes and gradually reducing the size until all have been read */
		do
		{
			bytesToRead = (a_iSize - retVal);

			if ((size = recv(m_iSocket, (buffer + retVal), bytesToRead, 0)) > 0)
			{
				retVal += size;
			}
			else
			{
				retVal = size;
				break;
			}
		}
		while (retVal < a_iSize);
	}
	else
	{
		retVal = recv(m_iSocket, buffer, a_iSize, 0);
	}

	if (retVal < 0)
	{
		throw Error("Unable to read from socket", retVal);
	}
	else if (retVal == 0)
	{
		throw RSocket::Error("Socket closed by remote host", retVal);
	}

	return retVal;
}

/**
 * Writes data to the socket.
 * Writes the number of bytes requested to the socket.  The entire number of bytes specified by a_iSize
 * will be written, even if this involves blocking.
 *
 * @pre		The socket has been opened with open()
 *
 * @date	Saturday 11-Feb-2017 5:55 pm, Code HQ Habersaathstrasse
 * @param	a_pcvBuffer		Pointer to the buffer from which to write the data
 * @param	a_iSize			The number of bytes to be written
 * @return	The number of bytes sent, or -1 if an error occurred
 */

int RSocket::write(const void *a_pcvBuffer, int a_iSize)
{
	const char *buffer = (const char *) a_pcvBuffer;
	int bytesToWrite, retVal = 0, size;

	/* Loop around, trying to write however many bytes are left to be written, starting with the total number */
	/* of bytes and gradually reducing the size until all have been written */
	do
	{
		bytesToWrite = (a_iSize - retVal);

		if ((size = send(m_iSocket, (buffer + retVal), bytesToWrite, 0)) > 0)
		{
			retVal += size;
		}
		else
		{
			retVal = size;
			break;
		}
	}
	while (retVal < a_iSize);

	if (retVal <= 0)
	{
		throw Error("Unable to write to socket", retVal);
	}
	else if (retVal == 0)
	{
		throw Error("Socket closed by remote host", retVal);
	}

	return retVal;
}

/**
 * Writes a string to the socket.
 * A convenience method that writes a NULL terminated string to the socket, including the NULL
 * terminator itself.
 *
 * @date	Sunday 10-Jan-2021 7:30 am, Code HQ Bergmannstrasse
 * @param	a_pccBuffer		A pointer to the NULL terminated string to be written
 * @return	The number of bytes written to the socket
 */

int RSocket::write(const char *a_pccBuffer)
{
	return(write((const void *) a_pccBuffer, (int) (strlen(a_pccBuffer) + 1)));
}
