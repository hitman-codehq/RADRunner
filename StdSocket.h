
#ifndef STDSOCKET_H
#define STDSOCKET_H

#ifdef WIN32

#include <winsock2.h>

#else /* ! WIN32 */

typedef int SOCKET;

#endif /* ! WIN32 */

class RSocket
{
private:

	SOCKET	m_iServerSocket;

public:

	SOCKET	m_iSocket;

public:

	RSocket();

	~RSocket()
	{
		close();
	}

	int open(const char *a_pccAddress);

	void close();

	int Listen(short a_sPort);

	int read(void *a_pvBuffer, int a_iSize);

	int write(const void *a_pcvBuffer, int a_iSize);

	int write(const char *a_pccBuffer);
};

#endif /* ! STDSOCKET_H */
