
#include <StdFuncs.h>
#include <string.h>
#include "ClientCommands.h"
#include "StdSocket.h"

const struct SCommand g_commands[] =
{
	{ "execute", 7 },
	{ "send", 4 },
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

void execute(RSocket &a_socket, const char *a_fileName)
{
	a_socket.write(g_commands[EExecute].m_command, g_commands[EExecute].m_length);
}

// TODO: CAW - Move this to somewhere better
#ifdef __amigaos__
#define SWAP(number)
#else /* ! __amigaos__ */
#define SWAP(number) Swap(number)

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Saturday 10-Oct-2020 10:25 pm, Code HQ Bergmannstrasse
 * @param	Parameter		Description
 * @return	Return value
 */

void Swap(long *a_plNumber)
{
	unsigned char temp;
	unsigned char *number = (unsigned char *) a_plNumber;

	temp = number[0];
	number[0] = number[3];
	number[3] = temp;

	temp = number[1];
	number[1] = number[2];
	number[2] = temp;
}

#endif /* ! __amigaos__ */

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

void send(RSocket &a_socket, const char *a_fileName)
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

	if (a_socket.write(g_commands[ESend].m_command, g_commands[ESend].m_length) > 0)
	{
		if ((length = a_socket.read(buffer, sizeof(buffer))) > 0) // TODO: CAW - Size
		{
			buffer[length] = '\0';

			if (strcmp(buffer, "ok") == 0)
			{
				// TODO: CAW - Use StdFuncs function for this?
				fseek(file, 0, SEEK_END);
				totalSize = ftell(file);
				fseek(file, 0, SEEK_SET);

				SWAP(&totalSize);
				a_socket.write(&totalSize, sizeof(totalSize));

				while ((size = fread(buffer, 1, sizeof(buffer), file)) > 0)
				{
					a_socket.write(buffer, size);
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
	a_socket.write(g_commands[EShutdown].m_command, g_commands[EShutdown].m_length);
}
