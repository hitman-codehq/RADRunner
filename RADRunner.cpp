
#include <StdFuncs.h>
#include <Args.h>
#include <Lex.h>
#include <StdTextFile.h>
#include <signal.h>
#include <string.h>
#include "ClientCommands.h"
#include "DirWrapper.h"
#include "StdSocket.h"

#ifndef WIN32

#include <sys/select.h>

#endif /* ! WIN32 */

#define ARGS_REMOTE 0
#define ARGS_EXECUTE 1
#define ARGS_SCRIPT 2
#define ARGS_SEND 3
#define ARGS_SERVER 4
#define ARGS_SHUTDOWN 5
#define ARGS_NUM_ARGS 6

#ifdef __amigaos4__

/* Lovely version structure.  Only Amiga makes it possible! */

static const struct Resident g_ROMTag __attribute__((used)) =
{
	RTC_MATCHWORD,
	(struct Resident *) &g_ROMTag,
	(struct Resident *) (&g_ROMTag + 1),
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

static const char g_template[] = "REMOTE,EXECUTE/K,SCRIPT/K,SEND/K,SERVER/S,SHUTDOWN/S";

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
 * Processes the given script file and executes its contents.
 * Reads the script file passed in one line at a time and executes each command found.  Lines starting
 * with a # character are considered to be comments and are skipped.
 *
 * @date	Sunday 10-Jan-2021 8:11 am, Code HQ Bergmannstrasse
 * @param	a_scriptName	Fully qualified path to the script to be parsed
 */

void ProcessScript(const char *a_scriptName)
{
	RTextFile script;

	printf("Parsing script \"%s\"...\n", a_scriptName);

	if (script.open(a_scriptName) == KErrNone)
	{
		const char *line;

		// Iterate through the script, reading each line and parsing it
		while ((line = script.GetLine()) != nullptr)
		{
			int length;
			TLex tokens(line, static_cast<int>(strlen(line)));

			const char *commandToken;

			// Extract the first token on the line
			if ((commandToken = tokens.NextToken(&length)) != nullptr)
			{
				CHandler *handler = nullptr;
				std::string command(commandToken, length);

				const char *argumentToken = tokens.NextToken(&length);

				// If it is a comment character then ignore it and continue to the next line
				if (command == "#")
				{
					continue;
				}
				else if (command == "execute")
				{
					if (argumentToken != nullptr)
					{
						handler = new CExecute(&g_socket, argumentToken);
					}
					else
					{
						Utils::Error("No value was specified for the argument \"execute\"");
					}
				}
				else if (command == "send")
				{
					if (argumentToken != nullptr)
					{
						handler = new CSend(&g_socket, argumentToken);
					}
					else
					{
						Utils::Error("No value was specified for the argument \"send\"");
					}
				}
				else if (command == "shutdown")
				{
					handler = new CShutdown(&g_socket);
				}

				if (handler != nullptr)
				{
					printf("Sending request \"%s\"\n", command.c_str());
					handler->sendRequest();
					delete handler;
				}
			}
		}

		script.close();
	}
	else
	{
		Utils::Error("Unable to open script file \"%s\" for parsing", a_scriptName);
	}
}

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
							SWAP(&command.m_command);
							SWAP(&command.m_length);

							printf("Received request \"%s\"\n", g_commandNames[command.m_command]);

							CHandler *handler = nullptr;

							if (command.m_command == EExecute)
							{
								handler = new CExecute(&g_socket, command);
							}
							else if (command.m_command == ESend)
							{
								handler = new CSend(&g_socket, command);
							}
							else if (command.m_command == EShutdown)
							{
								shutdown = true;
								printf("shutdown: Exiting\n");
							}
							else
							{
								printf("Invalid command received: %d\n", command.m_command);
								g_socket.write("invalid");
							}

							if (handler != nullptr)
							{
								handler->execute();
								delete handler;
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
#ifdef __amigaos4__
struct Library *IntuitionBase;
struct IconIFace *IIcon;
struct IntuitionIFace *IIntuition;
struct UtilityIFace *IUtility;
#else /* ! __amigaos4__ */
struct IntuitionBase *IntuitionBase;
#endif /* ! __amigaos4__ */
struct Library *UtilityBase;

int main(int a_argc, const char *a_argv[])
{
	int result;

	/* Install a ctrl-c handler so we can handle ctrl-c being pressed and shut down the scan */
	/* properly */

	signal(SIGINT, SignalHandler);

#ifdef __amiga__
	// TODO: CAW - Move these
	IconBase = OpenLibrary("icon.library", 40);
#ifdef __amigaos4__
	IntuitionBase = OpenLibrary("intuition.library", 40);
	IIcon = (struct IconIFace *) GetInterface(IconBase, "main", 1, NULL);
	IIntuition = (struct IntuitionIFace *) GetInterface(IntuitionBase, "main", 1, NULL);
	IUtility = (struct UtilityIFace *) GetInterface(UtilityBase, "main", 1, NULL);
#else /* ! __amigaos4__ */
	IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 40);
#endif /* ! __amigaos4__ */
	UtilityBase = OpenLibrary("utility.library", 40);
#endif /* __amiga__ */

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
					if (g_args[ARGS_SCRIPT] != nullptr)
					{
						ProcessScript(g_args[ARGS_SCRIPT]);
					}
					else
					{
						CHandler *handler = nullptr;

						if (g_args[ARGS_EXECUTE] != nullptr)
						{
							handler = new CExecute(&g_socket, g_args[ARGS_EXECUTE]);
						}

						if (g_args[ARGS_SEND] != nullptr)
						{
							handler = new CSend(&g_socket, g_args[ARGS_SEND]);
						}

						if (g_args[ARGS_SHUTDOWN] != nullptr)
						{
							handler = new CShutdown(&g_socket);
						}

						if (handler != nullptr)
						{
							handler->sendRequest();
							delete handler;
						}
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
