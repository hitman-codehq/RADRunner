
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
		Close();
	}

	int Open(const char *a_pccAddress);

	void Close();

	int Listen(short a_sPort);

	int Read(char *a_pcBuffer, int a_iSize);

	int Write(const char *a_pccBuffer, int a_iSize);
};

#endif /* ! STDSOCKET_H */
