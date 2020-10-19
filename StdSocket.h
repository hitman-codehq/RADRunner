
#ifndef STDSOCKET_H
#define STDSOCKET_H

class RSocket
{
private:

	int		m_iServerSocket;

public:

	int		m_iSocket;

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
};

#endif /* ! STDSOCKET_H */
