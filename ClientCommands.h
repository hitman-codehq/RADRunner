
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
	const char	*m_command;
	int			m_length;
};

extern const struct SCommand g_commands[];

void execute(RSocket &a_socket, const char *a_fileName);

void send(RSocket &a_socket, const char *a_fileName);

void Shutdown(RSocket &a_socket);

#endif /* ! CLIENTCOMMANDS_H */
