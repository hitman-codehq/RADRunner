
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
	ESend,
	EShutdown
};

// TODO: CAW - Rename execute
struct SCommand
{
public:

	uint32_t	m_command;	/* Command to be performed by the server */
	uint32_t	m_length;	/* Length of payload after structure */
};

class CHandler
{
protected:

	SCommand	m_command;	/* Basic command structure to be sent */
	RSocket		m_socket;	/* Socket on which to send the command */

protected:

	bool send();

public:

	CHandler(uint32_t a_command, RSocket &a_socket) : m_socket(a_socket)
	{
		m_command.m_command = a_command;
	}

	virtual ~CHandler() { }

	virtual void execute() = 0;
};

class CExecute : public CHandler
{
	const char	*m_fileName;	/* The name of the file to be executed */

public:

	CExecute(RSocket &a_socket, const char *a_fileName) : CHandler(EExecute, a_socket),
		m_fileName(a_fileName) { }

	virtual void execute();
};

class CSend : public CHandler
{
	const char	*m_fileName;	/* The name of the file to be sent */

public:

	CSend(RSocket &a_socket, const char *a_fileName) : CHandler(ESend, a_socket)
		, m_fileName(a_fileName) { }

	virtual void execute();
};

class CShutdown : public CHandler
{
public:

	CShutdown(RSocket &a_socket) : CHandler(EShutdown, a_socket) { }

	virtual void execute();
};

extern const char *g_commandNames[];

#endif /* ! CLIENTCOMMANDS_H */
