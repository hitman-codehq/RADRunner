
#include "StdFuncs.h"
#include "StdSocket.h"

#ifdef WIN32

#include <winsock.h>

#endif /* WIN32 */

#ifdef __unix__

#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#endif /* __unix__ */

#define closesocket(socket) close(socket)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

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
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Saturday 11-Feb-2017 4:37 pm, Code HQ Habersaathstrasse
 * @param	Parameter		Description
 * @return	Return value
 */

int RSocket::Open(const char *a_pccAddress)
{
	int RetVal;
	struct hostent *HostEnt;
	struct in_addr *InAddr;
	struct sockaddr_in SockAddr;

#ifdef WIN32

	WSAData WSAData;

#endif /* WIN32 */

	RetVal = KErrGeneral;

	//if (WSAStartup(MAKEWORD(2, 2), &WSAData) == 0)
	{
		if ((m_iSocket = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET)
		{
			if (a_pccAddress)
			{
				if ((HostEnt = gethostbyname(a_pccAddress)) != NULL)
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
			}
			else
			{
				RetVal = KErrNone;
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

void RSocket::Close()
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
	int RetVal, Socket;
	socklen_t ClientSize;
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

int RSocket::Read(char *a_pcBuffer, int a_iSize)
{
	return(recv(m_iSocket, a_pcBuffer, a_iSize, 0));
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

int RSocket::Write(const char *a_pccBuffer, int a_iSize)
{
	return(send(m_iSocket, a_pccBuffer, a_iSize, 0));
}
