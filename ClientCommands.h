
#ifndef CLIENTCOMMANDS_H
#define CLIENTCOMMANDS_H

#include "StdSocket.h"

#ifdef __amigaos__

#define SWAP(number)

#else /* ! __amigaos__ */

#define SWAP(number) Utils::swap32(number)

#endif /* ! __amigaos__ */

enum TCommands
{
	EExecute,
	EGet,
	ESend,
	EShutdown
};

struct SCommand
{
public:

	uint32_t	m_command;	/* Command to be performed by the server */
	uint32_t	m_length;	/* Length of payload after structure */
};

class CHandler
{
protected:

	RSocket			*m_socket;	/* Socket on which to process the command */
	SCommand		m_command;	/* Basic command structure to be processed */
	unsigned char	*m_payload;	/* Buffer containing packet's payload, if any */

protected:

	int readFile(const char *a_fileName, uint32_t a_fileSize);

	bool readPayload();

	bool sendCommand();

	int sendFile(const char *a_fileName);

public:

	CHandler(RSocket *a_socket, uint32_t a_command) : m_socket(a_socket)
	{
		m_command.m_command = a_command;
	}

	CHandler(RSocket *a_socket, const SCommand &a_command) : m_socket(a_socket), m_command(a_command) { }

	virtual ~CHandler()
	{
		delete [] m_payload;
	}

	virtual void execute() = 0;

	virtual void sendRequest() = 0;
};

class CExecute : public CHandler
{
	const char	*m_fileName;	/* The name of the file to be executed */

public:

	CExecute(RSocket *a_socket, const char *a_fileName) : CHandler(a_socket, EExecute), m_fileName(a_fileName) { }

	CExecute(RSocket *a_socket, const SCommand &a_command) : CHandler(a_socket, a_command) { }

	virtual void execute();

	virtual void sendRequest();
};

class CGet : public CHandler
{
	const char	*m_fileName;	/* The name of the file to be fetched */

public:

	CGet(RSocket *a_socket, const char *a_fileName) : CHandler(a_socket, EGet), m_fileName(a_fileName) { }

	CGet(RSocket *a_socket, const SCommand &a_command) : CHandler(a_socket, a_command) { }

	virtual void execute();

	virtual void sendRequest();
};

class CSend : public CHandler
{
	const char	*m_fileName;	/* The name of the file to be sent */

public:

	CSend(RSocket *a_socket, const char *a_fileName) : CHandler(a_socket, ESend), m_fileName(a_fileName) { }

	CSend(RSocket *a_socket, const SCommand &a_command) : CHandler(a_socket, a_command) { }

	virtual void execute();

	virtual void sendRequest();
};

class CShutdown : public CHandler
{
public:

	CShutdown(RSocket *a_socket) : CHandler(a_socket, EShutdown) { }

	virtual void execute() { }

	virtual void sendRequest();
};

extern const char *g_commandNames[];

#endif /* ! CLIENTCOMMANDS_H */
