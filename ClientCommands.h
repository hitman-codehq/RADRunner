
#ifndef CLIENTCOMMANDS_H
#define CLIENTCOMMANDS_H

class RSocket;

enum TCommands
{
	ESendFile,
	EQuit
};

struct SCommand
{
	const char	*m_command;
	int			m_length;
};

extern const struct SCommand g_commands[];

void Quit(RSocket &a_socket);

void SendFile(RSocket &a_socket);

#endif /* ! CLIENTCOMMANDS_H */
