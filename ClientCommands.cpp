
#include <StdFuncs.h>
#include <StdSocket.h>
#include "ClientCommands.h"

const struct SCommand g_commands[] =
{
	{ "sendfile", 8 },
	{ "quit", 4 }
};

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

void Quit(RSocket &a_socket)
{
	a_socket.Write(g_commands[EQuit].m_command, g_commands[EQuit].m_length);
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

void SendFile(RSocket &a_socket)
{
	char buffer[1024]; // TODO: CAW
	int length, result;
	long bytesWritten, size, totalSize;
	FILE *file;

	if (a_socket.Write(g_commands[ESendFile].m_command, g_commands[ESendFile].m_length) > 0)
	{
		if ((length = a_socket.Read(buffer, sizeof(buffer))) > 0) // TODO: CAW - Size
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
						a_socket.Write(buffer, size);
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
