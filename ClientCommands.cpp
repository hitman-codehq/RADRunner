
#include <StdFuncs.h>
#include <string.h>
#include "ClientCommands.h"
#include "StdSocket.h"

const struct SCommand g_commands[] =
{
	{ "execute", 11 },
	{ "send", 8 },
	{ "shutdown", 8 }
};

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Wednesday 29-Jan-2020 2:13 pm, Scoot flight TR 735 to Singapore
 * @param	Parameter		Description
 * @return	Return value
 */

void Execute(RSocket &a_socket, const char *a_fileName)
{
	a_socket.Write(g_commands[EExecute].m_command, g_commands[EExecute].m_length);
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

void Send(RSocket &a_socket, const char *a_fileName)
{
	char buffer[1024]; // TODO: CAW
	int length, result;
	long size, totalSize;
	FILE *file;

	if ((file = fopen(a_fileName, "rb")) == nullptr)
	{
		Utils::Error("Unable to open file \"%s\"", a_fileName);

		return;
	}

	if (a_socket.Write(g_commands[ESend].m_command, g_commands[ESend].m_length) > 0)
	{
		if ((length = a_socket.Read(buffer, sizeof(buffer))) > 0) // TODO: CAW - Size
		{
			buffer[length] = '\0';

			if (strcmp(buffer, "ok") == 0)
			{
				// TODO: CAW - Use StdFuncs function for this?
				fseek(file, 0, SEEK_END);
				totalSize = ftell(file);
				fseek(file, 0, SEEK_SET);

				while ((size = fread(buffer, 1, sizeof(buffer), file)) > 0)
				{
					a_socket.Write(buffer, size);
				}
			}
		}
	}

	fclose(file);
}

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Wednesday 29-Jan-2020 2:52 pm, Scoot flight TR 735 to Singapore
 * @param	Parameter		Description
 * @return	Return value
 */

void Shutdown(RSocket &a_socket)
{
	a_socket.Write(g_commands[EShutdown].m_command, g_commands[EShutdown].m_length);
}
