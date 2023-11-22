
#include <StdFuncs.h>
#include <Args.h>
#include <Lex.h>
#include <StdTextFile.h>
#include <memory>
#include <signal.h>
#include <string.h>
#include <vector>
#include <Yggdrasil/Commands.h>

#ifndef WIN32

#include <sys/select.h>

#endif /* ! WIN32 */

#define ARGS_DIR 0
#define ARGS_EXECUTE 1
#define ARGS_GET 2
#define ARGS_PORT 3
#define ARGS_SCRIPT 4
#define ARGS_SEND 5
#define ARGS_SERVER 6
#define ARGS_SHUTDOWN 7
#define ARGS_REMOTE 8
#define ARGS_NUM_ARGS 9

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
/* in Commands.h if the ordering or number of these change */
static const char g_template[] = "DIR/K,EXECUTE/K,GET/K,PORT/K,SCRIPT/K,SEND/K,SERVER/S,SHUTDOWN/S,REMOTE";

static volatile bool g_break;		/* Set to true if when ctrl-c is hit by the user */
static RArgs g_args;				/* Contains the parsed command line arguments */

/* Written: Friday 02-Jan-2009 10:30 am */

static void SignalHandler(int /*a_signal*/)
{
	/* Signal that ctrl-c has been pressed so that we break out of the scanning routine */

	g_break = true;
}

/**
 * Processes the given script file and executes its contents.
 * Reads the script file passed in one line at a time and executes each command found.  Lines starting
 * with a # character are considered to be comments and are skipped.
 *
 * @date	Sunday 10-Jan-2021 8:11 am, Code HQ Bergmannstrasse
 * @param	a_socket		The socket to which to write client commands
 * @param	a_scriptName	Fully qualified path to the script to be parsed
 * @return	A TResult containing both results from RADRunner and from any client command launched
 */

static TResult ProcessScript(RSocket &a_socket, const char *a_scriptName)
{
	int scriptArgLength;
	RTextFile scriptFile;
	TLex scriptArgTokens(a_scriptName, static_cast<int>(strlen(a_scriptName)));
	TResult retVal{ KErrNone, 0 };

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

				if (command == "dir")
				{
					if (!argument.empty())
					{
						handler = new CDir(&a_socket, argument.c_str());
					}
					else
					{
						Utils::Error("No value was specified for the argument \"dir\"");
					}
				}
				else if (command == "execute")
				{
					if (!argument.empty())
					{
						handler = new CExecute(&a_socket, argument.c_str());
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
						handler = new CGet(&a_socket, argument.c_str());
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
						handler = new CSend(&a_socket, argument.c_str());
					}
					else
					{
						Utils::Error("No value was specified for the argument \"send\"");
					}
				}
				else if (command == "shutdown")
				{
					handler = new CShutdown(&a_socket);
				}
				else
				{
					printf("Warning: Unknown command \"%s\" found\n", command.c_str());
				}

				if (handler != nullptr)
				{
					printf("Sending request \"%s\"\n", command.c_str());
					retVal = handler->sendRequest();
					delete handler;

					if ((retVal.m_result != KErrNone) || (retVal.m_subResult != 0))
					{
						break;
					}
				}
			}
		}

		scriptFile.close();
	}
	else
	{
		Utils::Error("Unable to open script file \"%s\" for parsing", a_scriptName);
	}

	return retVal;
}

/**
 * Launches RADRunner in client mode.
 * This method must be executed when RADRunner is launched in client mode.  It will connect to the remote server
 * specified in the program arguments and then send the commands specified in the program arguments to it.  At
 * the end of this, it will close the connection to the server.
 *
 * @date	Tuesday 27-Apr-2021 6:33 am, Code HQ Bergmannstrasse
 * @param	a_port			The port to which to connect
 */

static TResult StartClient(unsigned short a_port)
{
	RSocket socket;
	TResult retVal{ KErrNone, 0 };

	if (g_args[ARGS_REMOTE] != nullptr)
	{
		if (socket.open(g_args[ARGS_REMOTE], a_port) == KErrNone)
		{
			try
			{
				/* Start by sending a signature, to identify us as a RADRunner client */
				socket.write(g_signature, 4);

				/* Handlers are stored in a shared_ptr so that they are freed if an exception */
				/* occurs, but we allocate them with new (rather than std::make_shared()) as we */
				/* need them to be zero initialised by MungWall */
				std::shared_ptr<CHandler> handler(new CVersion(&socket));

				/* Check whether the server's protocol version is supported.  This handler will */
				/* display an error and exit if it is not */
				handler->sendRequest();
				handler = nullptr;

				if (g_args[ARGS_SCRIPT] != nullptr)
				{
					retVal = ProcessScript(socket, g_args[ARGS_SCRIPT]);
				}
				else
				{
					if (g_args[ARGS_DIR] != nullptr)
					{
						handler = std::shared_ptr<CHandler>(new CDir(&socket, g_args[ARGS_DIR]));
					}
					else if (g_args[ARGS_EXECUTE] != nullptr)
					{
						handler = std::shared_ptr<CHandler>(new CExecute(&socket, g_args[ARGS_EXECUTE]));
					}
					else if (g_args[ARGS_GET] != nullptr)
					{
						handler = std::shared_ptr<CHandler>(new CGet(&socket, g_args[ARGS_GET]));
					}
					else if (g_args[ARGS_SEND] != nullptr)
					{
						handler = std::shared_ptr<CHandler>(new CSend(&socket, g_args[ARGS_SEND]));
					}
					else if (g_args[ARGS_SHUTDOWN] != nullptr)
					{
						handler = std::shared_ptr<CHandler>(new CShutdown(&socket));
					}
					else
					{
						Utils::Error("Unknown command");
					}

					if (handler != nullptr)
					{
						retVal = handler->sendRequest();
						handler = nullptr;
					}
				}
			}
			catch (RSocket::Error &a_exception)
			{
				Utils::Error(a_exception.what());
			}

			socket.close();
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

	return retVal;
}

/**
 * Launches RADRunner in server mode.
 * This method must be executed when RADRunner is launched in server mode.  It will listen on a socket for
 * incoming connections and, when received, will process whatever commands are received from the client.  At
 * the end of this, it will close the connection to the client and wait for a new connection to come in.
 *
 * @date	Sunday 17-Nov-2019 3:29 pm, Sankt Oberholz
 * @param	a_port			The port on which to listen for connections
 */

static void StartServer(unsigned short a_port)
{
	bool shutdown = false;
	int result, selectResult;
	RSocket socket;
	SCommand command;
	fd_set socketSet;

	printf("Starting RADRunner server\n");

	if ((result = socket.open(nullptr, a_port)) == KErrNone)
	{
		if ((result = socket.listen(a_port)) == KErrNone)
		{
			do
			{
				printf("Listening for a client connection... ");
				fflush(stdout);

				if ((result = socket.accept()) == KErrNone)
				{
					printf("connected\n");

					try
					{
						FD_ZERO(&socketSet);
						FD_SET(socket.m_socket, &socketSet);

						/* Ensure that the client that just connected is a RADRunner client, which will always send a */
						/* signature as soon as it connects */
						char clientSignature[4];

						socket.read(clientSignature, sizeof(clientSignature));

						if (memcmp(clientSignature, g_signature, 4) != 0)
						{
							Utils::Error("Connected client is not an instance of RADRunner, closing connection");
							shutdown = true;
						}

						while (!g_break && !shutdown)
						{
							selectResult = select(FD_SETSIZE, &socketSet, nullptr, nullptr, nullptr);

							if (selectResult > 0)
							{
								socket.read(&command, sizeof(command));

								SWAP(&command.m_command);
								SWAP(&command.m_size);

								printf("Received request \"%s\"\n", g_commandNames[command.m_command]);

								/* Handlers are stored in a shared_ptr so that they are freed if an exception */
								/* occurs, but we allocate them with new (rather than std::make_shared()) as we */
								/* need them to be zero initialised by MungWall */
								std::shared_ptr<CHandler> handler(nullptr);

								if (command.m_command == EVersion)
								{
									handler = std::shared_ptr<CHandler>(new CVersion(&socket, command));
								}
								else if (command.m_command == EDelete)
								{
									handler = std::shared_ptr<CHandler>(new CDelete(&socket, command));
								}
								else if (command.m_command == EDir)
								{
									handler = std::shared_ptr<CHandler>(new CDir(&socket, command));
								}
								else if (command.m_command == EExecute)
								{
									handler = std::shared_ptr<CHandler>(new CExecute(&socket, command));
								}
								else if (command.m_command == EFileInfo)
								{
									handler = std::shared_ptr<CHandler>(new CFileInfo(&socket, command));
								}
								else if (command.m_command == EGet)
								{
									handler = std::shared_ptr<CHandler>(new CGet(&socket, command));
								}
								else if (command.m_command == ERename)
								{
									handler = std::shared_ptr<CHandler>(new CRename(&socket, command));
								}
								else if (command.m_command == ESend)
								{
									handler = std::shared_ptr<CHandler>(new CSend(&socket, command));
								}
								else if (command.m_command == EShutdown)
								{
									printf("shutdown: Exiting\n");
									shutdown = true;
								}
								else
								{
									printf("Invalid command received: %d\n", command.m_command);
									socket.write("invalid");
								}

								if (handler != nullptr)
								{
									handler->execute();
									handler = nullptr;
								}
							}
							else if (selectResult == -1)
							{
								shutdown = true;
							}
						}
					}
					catch (RSocket::Error &a_exception)
					{
						/* If the result was 0 then the remote socket has been closed and we want to go back to listening. */
						/* Otherwise it is a "real" socket error, so display an error and shut down */
						if (a_exception.m_result != 0)
						{
							printf("Unable to perform I/O on socket (Error = %d)\n", a_exception.m_result);
							shutdown = true;
						}
					}
				}
				else
				{
					printf("failed (Error = %d)\n", result);
					shutdown = true;
				}
			} while (!g_break && !shutdown);
		}
		else
		{
			printf("Unable to listen on socket (Error = %d)\n", result);
			shutdown = true;
		}

		socket.close();
	}
	else
	{
		Utils::Error("Unable to open socket (Error = %d)", result);
		shutdown = true;
	}

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
	int port = 80, result;
	TResult clientResult{KErrNone, 0};

	/* Install a ctrl-c handler so we can handle ctrl-c being pressed and shut down the scan */
	/* properly */

	signal(SIGINT, SignalHandler);

	/* Parse the command line parameters passed in and make sure they are formatted correctly */

	if ((result = g_args.open(g_template, ARGS_NUM_ARGS, a_argv, a_argc)) == KErrNone)
	{
		if (g_args[ARGS_PORT] != nullptr)
		{
			if (Utils::StringToInt(g_args[ARGS_PORT], &port) != KErrNone)
			{
				Utils::Error("Invalid port specified, using default port %d", port);
			}
		}

		if (g_args[ARGS_SERVER] != nullptr)
		{
			StartServer(static_cast<unsigned short>(port));
		}
		else
		{
			clientResult = StartClient(static_cast<unsigned short>(port));
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

	/* In order to enable the user to differentiate between failure due to RADRunner itself having an error, and */
	/* failure due to the software launched by the execute command returning an error, we will return the standard */
	/* AmigaOS RETURN_ERROR error for RADRunner errors and the negative error returned by the execute command for */
	/* execution errors */
	if (clientResult.m_result == KErrNone)
	{
		return (clientResult.m_subResult == 0) ? RETURN_OK : -clientResult.m_subResult;
	}
	else
	{
		return RETURN_ERROR;
	}
}
