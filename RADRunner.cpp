
#include <StdFuncs.h>
#include <Args.h>
#include <Lex.h>
#include <StdTextFile.h>
#include <signal.h>
#include <string.h>
#include <vector>
#include "Commands.h"
#include "StdSocket.h"

#ifndef WIN32

#include <sys/select.h>

#endif /* ! WIN32 */

#define ARGS_REMOTE 0
#define ARGS_EXECUTE 1
#define ARGS_GET 2
#define ARGS_SCRIPT 3
#define ARGS_SEND 4
#define ARGS_SERVER 5
#define ARGS_SHUTDOWN 6
#define ARGS_NUM_ARGS 7

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
	"\0$VER: RADRunner 0.01 (17.04.2021)\r\n",
	NULL
};

#elif defined(__amigaos__)

static const char g_accVersion[] = "$VER: RADRunner 0.01 (17.04.2021)";

#endif /* __amigaos__ */

/* Template for use in obtaining command line parameters.  Remember to change the indexes */
/* in Scanner.h if the ordering or number of these change */
static const char g_template[] = "REMOTE,EXECUTE/K,GET/K,SCRIPT/K,SEND/K,SERVER/S,SHUTDOWN/S";

/* Signature sent by the client when connecting, to identify it as a RADRunner client */
static const char g_signature[] = "RADR";

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
	int scriptArgLength;
	RTextFile scriptFile;
	TLex scriptArgTokens(a_scriptName, static_cast<int>(strlen(a_scriptName)));

	/* The first argument is the script name so extract that separately */
	const char *scriptArgToken = scriptArgTokens.NextToken(&scriptArgLength);
	std::string script(scriptArgToken, scriptArgLength);

	/* Now iterate through the remaining arguments and save them for easy reference later */
	std::vector<std::string> arguments;

	while ((scriptArgToken = scriptArgTokens.NextToken(&scriptArgLength)) != nullptr)
	{
		arguments.push_back(std::string(scriptArgToken, scriptArgLength));
	}

	printf("Parsing script \"%s\"...\n", script.c_str());

	if (scriptFile.open(script.c_str()) == KErrNone)
	{
		const char *line;

		/* Iterate through the script, reading each line and parsing it */
		while ((line = scriptFile.GetLine()) != nullptr)
		{
			const char *commandToken;
			int commandLength;
			TLex tokens(line, static_cast<int>(strlen(line)));

			/* Extract the first token on the line */
			if ((commandToken = tokens.NextToken(&commandLength)) != nullptr)
			{
				CHandler *handler = nullptr;
				std::string command(commandToken, commandLength);

				/* If it is a comment character then continue to the next line.  We check for the character being */
				/* '#' rather than the entire string, to enable comments with no space after the '#' character */
				if (command[0] == '#')
				{
					continue;
				}

				/* Extract the argument token and put it in a string of the precise length returned, to ensure */
				/* that any trailing " is stripped */
				const char *argumentToken = tokens.NextToken(&commandLength);
				std::string argument(argumentToken, commandLength);

				char variable[] = "$1";
				size_t offset, variableIndex;

				/* Variables in the range $1 to $9 are accepted, so iterate check whether any of these are present */
				/* in the argument list and replace them with the matching argument that was passed in on the */
				/* command line */
				for (variableIndex = 0; variableIndex < 9; ++variableIndex)
				{
					variable[1] = '1' + static_cast<char>(variableIndex);

					/* There may be more than one instance of the variable so search for it in a loop */
					while ((offset = argument.find(variable)) != std::string::npos)
					{
						if (variableIndex < arguments.size())
						{
							argument.replace(offset, 2, arguments[variableIndex]);
						}
						/* If no argument was passed in for this variable, just treat it was a warning and continue */
						else
						{
							printf("Warning: No argument passed in for variable %s\n", variable);
							break;
						}
					}
				}

				if (command == "execute")
				{
					if (!argument.empty())
					{
						handler = new CExecute(&g_socket, argument.c_str());
					}
					else
					{
						Utils::Error("No value was specified for the argument \"execute\"");
					}
				}
				else if (command == "get")
				{
					if (!argument.empty())
					{
						handler = new CGet(&g_socket, argument.c_str());
					}
					else
					{
						Utils::Error("No value was specified for the argument \"get\"");
					}
				}
				else if (command == "send")
				{
					if (!argument.empty())
					{
						handler = new CSend(&g_socket, argument.c_str());
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
				else
				{
					printf("Warning: Unknown command \"%s\" found\n", command.c_str());
				}

				if (handler != nullptr)
				{
					printf("Sending request \"%s\"\n", command.c_str());
					handler->sendRequest();
					delete handler;
				}
			}
		}

		scriptFile.close();
	}
	else
	{
		Utils::Error("Unable to open script file \"%s\" for parsing", a_scriptName);
	}
}

/**
 * Launches RADRunner in server mode.
 * This method must be executed when RADRunner is launched in server mode.  It will listen on a socket for
 * incoming connections and, when received, will process whatever commands are received from the client.  At
 * the end of this, it will close the connection to the client and wait for a new connection to come in.
 *
 * @date	Sunday 17-Nov-2019 3:29 pm, Sankt Oberholz
 */

void StartServer()
{
	bool disconnect, shutdown;
	int result, selectResult, size;
	SCommand command;
	fd_set socketSet;

	printf("Starting RADRunner server\n");

	do
	{
		if ((result = g_socket.open(nullptr, 80)) == KErrNone)
		{
			printf("Listening for a client connection... ");
			fflush(stdout);

			if ((result = g_socket.listen(80)) == KErrNone)
			{
				printf("connected\n");

				disconnect = shutdown = false;

				FD_ZERO(&socketSet);
				FD_SET(g_socket.m_iSocket, &socketSet);

				/* Ensure that the client that just connected is a RADRunner client, which will always send a */
				/* signature as soon as it connects */
				char clientSignature[4];

				if (g_socket.read(clientSignature, sizeof(clientSignature)) == 4)
				{
					if (memcmp(clientSignature, g_signature, 4) != 0)
					{
						Utils::Error("Connected client is not an instance of RADRunner, closing connection");
						disconnect = true;
					}
				}

				while (!g_break && !disconnect && !shutdown)
				{
					selectResult = select(FD_SETSIZE, &socketSet, nullptr, nullptr, nullptr);

					if (selectResult > 0)
					{
						if ((size = g_socket.read(&command, sizeof(command))) > 0)
						{
							SWAP(&command.m_command);
							SWAP(&command.m_size);

							printf("Received request \"%s\"\n", g_commandNames[command.m_command]);

							CHandler *handler = nullptr;

							if (command.m_command == EExecute)
							{
								handler = new CExecute(&g_socket, command);
							}
							else if (command.m_command == EGet)
							{
								handler = new CGet(&g_socket, command);
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
							else if (command.m_command == EVersion)
							{
								handler = new CVersion(&g_socket, command);
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
 * Entry point of the program.
 * Reads the arguments passed into the program and determines whether to launch in client or server mode
 * as appropriate.
 *
 * @date	Sunday 17-Nov-2019 3:30 pm, Sankt Oberholz
 * @param	a_argc			Number of arguments passed in
 * @param	a_arcv			Array of string arguments
 * @return	RETURN_OK if successful, else RETURN_ERROR
 */

int main(int a_argc, const char *a_argv[])
{
	int result;

	/* Install a ctrl-c handler so we can handle ctrl-c being pressed and shut down the scan */
	/* properly */

	signal(SIGINT, SignalHandler);

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
				if (g_socket.open(g_args[ARGS_REMOTE], 80) == KErrNone)
				{
					/* Start by sending a signature, to identify us as a RADRunner client */
					g_socket.write(g_signature, 4);

					/* And check whether the server's protocol version is supported.  This handler will */
					/* display an error and exit if it is not */
					CHandler *handler = new CVersion(&g_socket);

					handler->sendRequest();
					delete handler;
					handler = nullptr;

					if (g_args[ARGS_SCRIPT] != nullptr)
					{
						ProcessScript(g_args[ARGS_SCRIPT]);
					}
					else
					{
						if (g_args[ARGS_EXECUTE] != nullptr)
						{
							handler = new CExecute(&g_socket, g_args[ARGS_EXECUTE]);
						}

						if (g_args[ARGS_GET] != nullptr)
						{
							handler = new CGet(&g_socket, g_args[ARGS_GET]);
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
