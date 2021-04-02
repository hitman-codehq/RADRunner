
#ifndef STDSOCKET_H
#define STDSOCKET_H

#ifdef WIN32

#include <winsock2.h>

#else /* ! WIN32 */

typedef int SOCKET;

#endif /* ! WIN32 */

/**
 * A class providing synchronous socket communications.
 * This class provides basic synchronous socket communications, such as sending and receiving of data,
 * as well as automatic host name resolution when opening new sockets that reference a host name.  It
 * can be used in either client or server mode, where in server mode it can listen for and accept
 * incoming connections.
 */

class RSocket
{
private:

	SOCKET	m_iServerSocket;	/**< The socket on which to listen for connections */

public:

	SOCKET	m_iSocket;			/**< The socket with which data to transfer data */

public:

	RSocket();

	~RSocket()
	{
		close();
	}

	int open(const char *a_pccAddress, unsigned short a_usPort);

	void close();

	int listen(unsigned short a_usPort);

	int read(void *a_pvBuffer, int a_iSize, bool a_bReadAll = true);

	int write(const void *a_pcvBuffer, int a_iSize);

	int write(const char *a_pccBuffer);
};

#endif /* ! STDSOCKET_H */
