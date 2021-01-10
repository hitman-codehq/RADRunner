
#include <StdFuncs.h>
#include "StdSocket.h"

#if defined(__unix__) || defined(__amigaos__)

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>

#define closesocket(socket) ::close(socket)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#else /* ! defined(__unix__) || defined(__amigaos__) */

#include <ws2tcpip.h>

#endif /* ! defined(__unix__) || defined(__amigaos__) */

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Wednesday 29-Jan-2020 4:47 pm, Scoot flight TR 735 to Singapore
 * @param	Parameter		Description
 * @return	Return value
 */

RSocket::RSocket()
{
	m_iServerSocket = m_iSocket = INVALID_SOCKET;
}

/**
 * Opens a socket in either client or server mode.
 * Depending on whether a host name is specified, this method will open a socket for use either as a client
 * or a server.  If a host is specified, it will be resolved and a connection made to the remote server.
 * The host can be either a host name such as codehq.org or an IP address.  If no host is specified, the
 * socket will be opened in a state suitable for listening.
 *
 * @date	Saturday 11-Feb-2017 4:37 pm, Code HQ Habersaathstrasse
 * @param	a_pccHost		The name of the host, an IP address or NULL
 * @return	KErrNone if successful
 * @return	KErrGeneral if the socket could not be opened
 * @return	KErrHostNotFound if the host could not be resolved
 */

int RSocket::open(const char *a_pccHost)
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
					SockAddr.sin_port = htons(80);
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
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Saturday 11-Feb-2017 5:41 pm, Code HQ Habersaathstrasse
 * @param	Parameter		Description
 * @return	Return value
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
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Sunday 12-Feb-2017 7:53 am, Code HQ Habersaathstrasse
 * @param	Parameter		Description
 * @return	Return value
 */

int RSocket::Listen(short a_sPort)
{
	int RetVal;
	socklen_t ClientSize;
	SOCKET Socket;
	struct sockaddr_in Server, Client;

	RetVal = KErrGeneral;

	Server.sin_family = AF_INET;
	Server.sin_port = htons(a_sPort);
	Server.sin_addr.s_addr = INADDR_ANY;

	if (bind(m_iSocket, (struct sockaddr *) &Server, sizeof(Server)) != SOCKET_ERROR)
	{
		if (listen(m_iSocket, 1) == 0)
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
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Saturday 11-Feb-2017 5:59 pm, Code HQ Habersaathstrasse
 * @param	Parameter		Description
 * @return	Return value
 */

int RSocket::read(void *a_pvBuffer, int a_iSize)
{
	return(recv(m_iSocket, (char *) a_pvBuffer, a_iSize, 0));
}

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Saturday 11-Feb-2017 5:55 pm, Code HQ Habersaathstrasse
 * @param	Parameter		Description
 * @return	Return value
 */

int RSocket::write(const void *a_pcvBuffer, int a_iSize)
{
	return(send(m_iSocket, (char *) a_pcvBuffer, a_iSize, 0));
}

/**
 * Writes a string to the socket.
 * A convenience method that writes a NULL terminated string to the socket, including the NULL
 * terminator itself.
 *
 * @date	Saturday 11-Feb-2017 5:55 pm, Code HQ Habersaathstrasse
 * @param	a_pccBuffer		A pointer to the NULL terminated string to be written
 * @return	The number of bytes written to the socket
 */

int RSocket::write(const char *a_pccBuffer)
{
	return(write((const void *) a_pccBuffer, (int) (strlen(a_pccBuffer) + 1)));
}
