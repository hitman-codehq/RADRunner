
#ifndef CLIENTCOMMANDS_H
#define CLIENTCOMMANDS_H

#include "StdSocket.h"

#ifdef __amigaos__

#define SWAP(number)

#else /* ! __amigaos__ */

#define SWAP(number) Utils::swap32(number)

#endif /* ! __amigaos__ */

/**
 * The commands supported by the program.
 * Each of these commands is implemented by a matching CHandler derived class.
 */

enum TCommands
{
	EExecute,
	EGet,
	ESend,
	EShutdown
};

/**
 * Basic command structure.
 * A structure to be used for transfer between the local client and remote server, in order to specifiy a
 * command to be executed.  The structure uses fixed sized integers, for compatibility across different CPU
 * architectures.  It is the responsibility of the code using this structure to perform byte order swapping as
 * appropriate, and to send or receive the payload.  Only the size of the payload is sent with this structure.
 */

struct SCommand
{
public:

	uint32_t	m_command;	/**< Command to be performed by the server */
	uint32_t	m_size;		/**< Size in bytes of payload after structure */
};

/**
 * The base class for all command handlers.
 * All command handlers derive from, and implement, this interface, allowing them to be used without
 * concern for their concrete type or implementation.
 */

class CHandler
{
protected:

	RSocket			*m_socket;	/**< Socket on which to process the command */
	SCommand		m_command;	/**< Basic command structure to be processed */
	unsigned char	*m_payload;	/**< Buffer containing packet's payload, if any */

protected:

	int readFile(const char *a_fileName, uint32_t a_fileSize);

	bool readPayload();

	bool sendCommand();

	int sendFile(const char *a_fileName);

public:

	/** Constructor to be used when creating client instances */
	CHandler(RSocket *a_socket, uint32_t a_command) : m_socket(a_socket)
	{
		m_command.m_command = a_command;
	}

	/** Constructor to be used when creating server instances */
	CHandler(RSocket *a_socket, const SCommand &a_command) : m_socket(a_socket), m_command(a_command) { }

	virtual ~CHandler()
	{
		delete [] m_payload;
	}

	/** Method for executing the handler on the server */
	virtual void execute() = 0;

	/** Method for executing the handler on the client */
	virtual void sendRequest() = 0;
};

/**
 * Command for remotely executing an executable or script file.
 * Given the name of a file to be executed, this command sends that name to the remote host, where it is
 * executed.
 */

class CExecute : public CHandler
{
	const char	*m_fileName;	/**< The name of the file to be executed */

public:

	/** Constructor to be used when creating client instances */
	CExecute(RSocket *a_socket, const char *a_fileName) : CHandler(a_socket, EExecute), m_fileName(a_fileName) { }

	/** Constructor to be used when creating server instances */
	CExecute(RSocket *a_socket, const SCommand &a_command) : CHandler(a_socket, a_command) { }

	virtual void execute();

	virtual void sendRequest();
};

/**
 * Command for transferring a file from the remote host.
 * Given the name of a file to be fetched, this command requests that file from the remote host and transfers
 * it to the local host.
 */

class CGet : public CHandler
{
	const char	*m_fileName;	/**< The name of the file to be fetched */

public:

	/** Constructor to be used when creating client instances */
	CGet(RSocket *a_socket, const char *a_fileName) : CHandler(a_socket, EGet), m_fileName(a_fileName) { }

	/** Constructor to be used when creating server instances */
	CGet(RSocket *a_socket, const SCommand &a_command) : CHandler(a_socket, a_command) { }

	virtual void execute();

	virtual void sendRequest();
};

/**
 * Command for transferring a file to the remote host.
 * Given the name of a file to be sent, this command reads that file from the local host and transfers
 * it to the remote host.
 */

class CSend : public CHandler
{
	const char	*m_fileName;	/**< The name of the file to be sent */

public:

	/** Constructor to be used when creating client instances */
	CSend(RSocket *a_socket, const char *a_fileName) : CHandler(a_socket, ESend), m_fileName(a_fileName) { }

	/** Constructor to be used when creating server instances */
	CSend(RSocket *a_socket, const SCommand &a_command) : CHandler(a_socket, a_command) { }

	virtual void execute();

	virtual void sendRequest();
};

/**
 * Command for shutting down the remote server.
 * Sends a command to the remote server to tell it to shut itself down.
 */

class CShutdown : public CHandler
{
public:

	/** Constructor to be used when creating client instances */
	CShutdown(RSocket *a_socket) : CHandler(a_socket, EShutdown) { }

	/** Empty implementation of unused server side method */
	virtual void execute() { }

	virtual void sendRequest();
};

extern const char *g_commandNames[];

#endif /* ! CLIENTCOMMANDS_H */
