
#include <StdFuncs.h>
#include <Args.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "ClientCommands.h"
#include "DirWrapper.h"
#include "ServerCommands.h"
#include "StdSocket.h"

#ifndef WIN32

#include <sys/select.h>

#endif /* ! WIN32 */

#define ARGS_REMOTE 0
#define ARGS_SEND 1
#define ARGS_EXECUTE 2
#define ARGS_SERVER 3
#define ARGS_SHUTDOWN 4
#define ARGS_NUM_ARGS 5

#ifdef __amigaos4__

/* Lovely version structure.  Only Amiga makes it possible! */

static const struct Resident g_ROMTag __attribute__((used)) =
{
	RTC_MATCHWORD,
	(struct Resident *) &g_oROMTag,
	(struct Resident *) (&g_oROMTag + 1),
	RTF_AUTOINIT,
	0,
	NT_LIBRARY,
	0,
	"RADRunner",
	"\0$VER: RADRunner 0.01 (17.11.2019)\r\n",
	NULL
};

/* Use a large stack, as copying is highly recursive and can use a lot of stack space */

static const char __attribute__((used)) g_stackCookie[] = "$STACK:262144";

#endif /* __amigaos4__ */

/* Template for use in obtaining command line parameters.  Remember to change the indexes */
/* in Scanner.h if the ordering or number of these change */

static const char g_template[] = "REMOTE/A,SEND,EXECUTE/S,SERVER/S,SHUTDOWN/S";

static volatile bool g_break;		/* Set to true if when ctrl-c is hit by the user */
static RArgs g_args;				/* Contains the parsed command line arguments */

/* Written: Friday 02-Jan-2009 10:30 am */

static void SignalHandler(int /*a_signal*/)
{
	/* Signal that ctrl-c has been pressed so that we break out of the scanning routine */

	g_break = true;
}

static RSocket g_socket;

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Sunday 17-Nov-2019 3:29 pm, Sankt Oberholz
 * @param	Parameter		Description
 * @return	Return value
 */

void StartServer()
{
	bool disconnect, shutdown;
	int length, result, selectResult;
	struct SCommand command;
	fd_set socketSet;
	std::string message;

	printf("Starting RADRunner server\n");

	do
	{
		if ((result = g_socket.open(nullptr)) == KErrNone)
		{
			printf("Listening for a client connection... ");
			fflush(stdout);

			if ((result = g_socket.Listen(80)) == KErrNone)
			{
				printf("connected\n");

				disconnect = shutdown = false;

				FD_ZERO(&socketSet);
				FD_SET(g_socket.m_iSocket, &socketSet);

				do
				{
					selectResult = select(FD_SETSIZE, &socketSet, nullptr, nullptr, nullptr);

					if (selectResult > 0)
					{
						if ((length = g_socket.read(&command, sizeof(command))) > 0)
						{
							printf("Received request \"%s\"\n", g_commandNames[command.m_command]);

							if (command.m_command == EExecute)
							{
								ExecuteServer();
							}
							else if (command.m_command == ESend)
							{
								ReceiveFile(g_socket);
							}
							else if (command.m_command == EShutdown)
							{
								shutdown = true;
								printf("shutdown: Exiting\n");
							}
							else
							{
								printf("Invalid command received: %d\n", command.m_command);

								message = "invalid"; // TODO: CAW - Write these strings directly
								g_socket.write(message.c_str(), message.length());
							}
						}
						else
						{
							disconnect = true;

							printf("Client disconnected, ending session\n");
						}
					}
					else if (selectResult == -1)
					{
						disconnect = true;
					}
				}
				while (!g_break && !disconnect && !shutdown);
			}
			else
			{
				printf("failed (Error = %d)!\n", result);

				shutdown = true;
			}

			g_socket.close();
		}
		else
		{
			Utils::Error("Unable to open socket (Error = %d)", result);

			shutdown = true;
		}

	}
	while (!g_break && !shutdown);

	if (g_break)
	{
		printf("Received ctrl-c, exiting\n");
	}
}

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Sunday 17-Nov-2019 3:30 pm, Sankt Oberholz
 * @param	Parameter		Description
 * @return	Return value
 */

struct Library *IconBase;
struct IntuitionBase *IntuitionBase;

int main(int a_argc, const char *a_argv[])
{
	int result;

	/* Install a ctrl-c handler so we can handle ctrl-c being pressed and shut down the scan */
	/* properly */

	signal(SIGINT, SignalHandler);

#ifdef __amigaos__

	// TODO: CAW - Move these
	IconBase = OpenLibrary("icon.library", 40);
	IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 40);

#endif /* __amigaos__ */

	/* Parse the command line parameters passed in and make sure they are formatted correctly */

	if ((result = g_args.open(g_template, ARGS_NUM_ARGS, a_argv, a_argc)) == KErrNone)
	{
		if (g_args[ARGS_SERVER] != nullptr)
		{
			StartServer();
		}
		else
		{
			if (g_args[ARGS_REMOTE] != nullptr)
			{
				if (g_socket.open(g_args[ARGS_REMOTE]) == KErrNone)
				{
					if (g_args[ARGS_EXECUTE] != nullptr)
					{
						execute(g_socket, g_args[ARGS_EXECUTE]);
					}

					if (g_args[ARGS_SEND] != nullptr)
					{
						send(g_socket, g_args[ARGS_SEND]);
					}

					if (g_args[ARGS_SHUTDOWN] != nullptr)
					{
						Shutdown(g_socket);
					}

					g_socket.close();
				}
				else
				{
					Utils::Error("Cannot connect to %s", g_args[ARGS_REMOTE]);
				}
			}
			else
			{
				Utils::Error("REMOTE argument must be specified");
			}

		}

		g_args.close();
	}
	else
	{
		if (result == KErrNotFound)
		{
			Utils::Error("Required argument missing");
		}
		else
		{
			Utils::Error("Unable to read command line arguments");
		}
	}

	return((result == KErrNone) ? RETURN_OK : RETURN_ERROR);
}
