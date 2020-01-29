
#include <StdFuncs.h>
#include <Args.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include "ClientCommands.h"
#include "DirWrapper.h"
#include "ServerCommands.h"
#include "StdSocket.h"

#define ARGS_EXECUTE 0
#define ARGS_SEND 1
#define ARGS_SERVER 2
#define ARGS_SHUTDOWN 3
#define ARGS_NUM_ARGS 4

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

static const char g_template[] = "EXECUTE/S,SEND/M,SERVER/S,SHUTDOWN/S";

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
	char buffer[1024]; // TODO: CAW
	int length, selectResult;
	fd_set socketSet;
	std::string message;

	printf("Starting RADRunner server\n");

	do
	{
		if (g_socket.Open(NULL) == KErrNone)
		{
			printf("Listening for a client connection... ");
			fflush(stdout);

			if (g_socket.Listen(80) == KErrNone)
			{
				printf("connected\n");

				disconnect = shutdown = false;

				FD_ZERO(&socketSet);
				FD_SET(g_socket.m_iSocket, &socketSet);

				do
				{
					selectResult = select(FD_SETSIZE, &socketSet, NULL, NULL, NULL);

					if (selectResult > 0)
					{
						if ((length = g_socket.Read(buffer, sizeof(buffer))) > 0)
						{
							buffer[length] = '\0';
							printf("Received request \"%s\"\n", buffer);

							if (strcmp(buffer, g_commands[EExecute].m_command) == 0)
							{
								ExecuteServer();
							}
							else if (strcmp(buffer, g_commands[ESend].m_command) == 0)
							{
								ReceiveFile(g_socket);
							}
							else if (strcmp(buffer, g_commands[EShutdown].m_command) == 0)
							{
								shutdown = true;
								printf("shutdown: Exiting\n");
							}
							else
							{
								message = "invalid"; // TODO: CAW - Write these strings directly
								printf("Invalid command received: %s\n", buffer);
								g_socket.Write(message.c_str(), message.length());
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
				printf("connection failed!\n");
			}

			g_socket.Close();
		} // TODO: CAW - else fail
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

int main(int a_argc, const char *a_argv[])
{
	char *Source, *Dest;
	int Length, Result; // TODO: CAW - Rename these

	/* Install a ctrl-c handler so we can handle ctrl-c being pressed and shut down the scan */
	/* properly */

	signal(SIGINT, SignalHandler);

	/* Parse the command line parameters passed in and make sure they are formatted correctly */

	if ((Result = g_args.Open(g_template, ARGS_NUM_ARGS, a_argv, a_argc)) == KErrNone)
	{
		if (g_args[ARGS_SERVER] != nullptr)
		{
			StartServer();
		}
		else
		{
			if (g_socket.Open("localhost") == KErrNone)
			{
				if (g_args[ARGS_EXECUTE] != nullptr)
				{
					Execute(g_socket, g_args[ARGS_EXECUTE]);
				}

				if (g_args[ARGS_SEND] != nullptr)
				{
					Send(g_socket, g_args[ARGS_SEND]);
				}

				if (g_args[ARGS_SHUTDOWN] != nullptr)
				{
					Shutdown(g_socket);
				}

				g_socket.Close();
			}
			else
			{
				printf("Error: Cannot connect to localhost\n"); // TODO: CAW
			}
		}

		g_args.Close();
	}
	else
	{
		if (Result == KErrNotFound)
		{
			Utils::Error("Required argument missing");
		}
		else
		{
			Utils::Error("Unable to read command line arguments");
		}
	}

	return((Result == KErrNone) ? RETURN_OK : RETURN_ERROR);
}
