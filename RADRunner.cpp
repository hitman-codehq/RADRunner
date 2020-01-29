
#include <StdFuncs.h>
#include <Args.h>
#include <StdSocket.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> // TODO: CAW - UNIX only
#include <unistd.h> // TODO: CAW - UNIX only
#include "DirWrapper.h"

#define ARGS_SERVER 0
#define ARGS_NUM_ARGS 1

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

struct SCommand
{
	const char	*m_command;
	int			m_length;
};

/* Template for use in obtaining command line parameters.  Remember to change the indexes */
/* in Scanner.h if the ordering or number of these change */

static const char g_template[] = "SERVER/S";

static volatile bool g_break;		/* Set to true if when ctrl-c is hit by the user */
static RArgs g_args;				/* Contains the parsed command line arguments */

static const struct SCommand g_commands[] =
{
	{ "sendfile", 8 },
	{ "quit", 4 }
};

enum TCommands
{
	ESendFile,
	EQuit
};

/* Written: Friday 02-Jan-2009 10:30 am */

static void SignalHandler(int /*a_iSignal*/)
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
 * @date	Sunday 17-Nov-2019 3:43 pm, Sankt Oberholz
 * @param	Parameter		Description
 * @return	Return value
 */

void Quit()
{
	g_socket.Write(g_commands[EQuit].m_command, g_commands[EQuit].m_length);
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

void SendFile()
{
	char buffer[1024]; // TODO: CAW
	int length, result;
	long bytesWritten, size, totalSize;
	FILE *file;

	if (g_socket.Write(g_commands[ESendFile].m_command, g_commands[ESendFile].m_length) > 0)
	{
		if ((length = g_socket.Read(buffer, sizeof(buffer))) > 0) // TODO: CAW - Size
		{
			buffer[length] = '\0';
			printf("*** Received %s\n", buffer);

			if (strcmp(buffer, "ok") == 0)
			{
				// TODO: CAW - If this is not found, it causes the server to go into a loop
				file = fopen("test", "rb");

				if (file != nullptr)
				{
					// TODO: CAW - Use StdFuncs function for this?
					fseek(file, 0, SEEK_END);
					totalSize = ftell(file);
					fseek(file, 0, SEEK_SET);

					printf("File size is %ld\n", totalSize);
					bytesWritten = 0;

					while ((size = fread(buffer, 1, sizeof(buffer), file)) > 0)
					{
						g_socket.Write(buffer, size);
						printf("Wrote %ld bytes\n", size);
						bytesWritten += size;
					}

					printf("Wrote %ld bytes\n", bytesWritten);

					fclose(file);
				}
				else
				{
					printf("Unable to open file\n");
				}
			}
		}
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
	bool quit;
	char buffer[1024];
	//char message[1024];
	int length;
	std::string message;
	FILE *file;

	printf("Starting server...\n");

	if (g_socket.Open(NULL) == KErrNone)
	{
		printf("Connected ok!\n");

		if (g_socket.Listen(80) == KErrNone)
		{
			printf("Listened ok!\n");

			quit = false;

			do
			{
				if ((length = g_socket.Read(buffer, sizeof(buffer))) > 0)
				{
					buffer[length] = '\0';
					printf("Received request (Length = %d), %s\n", length, buffer);

					if (strcmp(buffer, g_commands[ESendFile].m_command) == 0)
					{
						message = "ok";
						g_socket.Write(message.c_str(), message.length());

						file = fopen("outfile", "wb");

						if (file != nullptr)
						{
							int bytesRead = 0, bytesToRead, size, totalSize = 8432; // TODO: CAW - This should be sent by the client

							do
							{
								bytesToRead = ((totalSize - bytesRead) >= sizeof(buffer)) ? sizeof(buffer) : (totalSize - bytesRead); // TODO: CAW
								printf("Reading %d bytes\n", bytesToRead);
								size = g_socket.Read(buffer, bytesToRead); // TODO: CAW - Error checking all through here

								if (size > 0)
								{
									fwrite(buffer, 1, size, file);
									printf("Read %d bytes\n", size);
									bytesRead += size;
								}

								printf("bytesRead = %d\n", bytesRead);
							}
							while (bytesRead < totalSize); // TODO: CAW - Handle failure

							fclose(file);

							// TODO: CAW - Error checking
							Utils::SetProtection("outfile", (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

							execl("outfile", "");
						}
					}
					else if (strcmp(buffer, g_commands[EQuit].m_command) == 0)
					{
						quit = true;
						printf("Quit received, shutting down...\n");
					}
					else
					{
						message = "invalid"; // TODO: CAW - Write these strings directly
						g_socket.Write(message.c_str(), message.length());
					}
				}
			}
			while (!g_break && !quit);
		}

		if (g_break)
		{
			printf("Received ctrl-c, shutting down\n");
		}

		g_socket.Close();
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

int main(int a_iArgC, const char *a_ppcArgV[])
{
	char *Source, *Dest;
	int Length, Result;

	/* Install a ctrl-c handler so we can handle ctrl-c being pressed and shut down the scan */
	/* properly */

	signal(SIGINT, SignalHandler);

	/* Parse the command line parameters passed in and make sure they are formatted correctly */

	if ((Result = g_args.Open(g_template, ARGS_NUM_ARGS, a_ppcArgV, a_iArgC)) == KErrNone)
	{
		if (g_args[ARGS_SERVER] != nullptr)
		{
			StartServer();
		}
		else
		{
			if (g_socket.Open("localhost") == KErrNone)
			{
				printf("Connected ok!\n");

				SendFile();
				Quit();

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
