
#include <StdFuncs.h>
#include <string.h>
#include "ClientCommands.h"
#include "StdSocket.h"

#ifdef __amigaos__

#define SWAP(number)

#else /* ! __amigaos__ */

#define SWAP(number) Utils::swap32(number)

#endif /* ! __amigaos__ */

const char *g_commandNames[] =
{
	"execute",
	"send",
	"shutdown"
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
	struct SCommand command = { EExecute, 0 };

	a_socket.write(&command, sizeof(command));
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

void send(RSocket &a_socket, const char *a_fileName)
{
	char buffer[1024]; // TODO: CAW
	int length;
	long size;
	uint32_t totalSize;
	FILE *file;

	if ((file = fopen(a_fileName, "rb")) == nullptr)
	{
		Utils::Error("Unable to open file \"%s\"", a_fileName);

		return;
	}

	struct SCommand command = { ESend, 0 };

	if (a_socket.write(&command, sizeof(command)) > 0)
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
	struct SCommand command = { EShutdown, 0};

	a_socket.write(&command, sizeof(command));
}
