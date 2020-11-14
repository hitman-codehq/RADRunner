
#ifndef CLIENTCOMMANDS_H
#define CLIENTCOMMANDS_H

class RSocket;

enum TCommands
{
	EExecute,
	ESend,
	EShutdown
};

struct SCommand
{
	uint32_t	m_command;	/* Command to be performed by the server */
	uint32_t	m_length;	/* Length of payload after structure */
};

extern const char *g_commandNames[];

void execute(RSocket &a_socket, const char *a_fileName);

void send(RSocket &a_socket, const char *a_fileName);

void Shutdown(RSocket &a_socket);

#endif /* ! CLIENTCOMMANDS_H */
