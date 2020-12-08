
#include <StdFuncs.h>
#include <string.h>
#include "ClientCommands.h"
#include "StdSocket.h"

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
 * @date	Sunday 29-Nov-2020 12:17 pm, Code HQ Bergmannstrasse
 * @param	Parameter		Description
 * @return	Return value
 */

bool CCommand::send()
{
	SWAP(&m_command.m_command);
	SWAP(&m_command.m_length);

	return (m_socket.write(&m_command, sizeof(m_command)) == sizeof(m_command));
}

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

void CExecute::execute()
{
	int32_t payloadLength = static_cast<int32_t>(strlen(m_fileName) + 1);
	m_command.m_length = payloadLength;

	if (send())
	{
		m_socket.write(m_fileName, payloadLength);
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

void CSend::execute()
{
	char buffer[1024]; // TODO: CAW
	int length;
	size_t size;
	uint32_t totalSize;
	FILE *file;

	if ((file = fopen(m_fileName, "rb")) == nullptr)
	{
		Utils::Error("Unable to open file \"%s\"", m_fileName);

		return;
	}

	int32_t payloadLength = static_cast<int32_t>(strlen(m_fileName) + 1);
	m_command.m_length = payloadLength;

	if (send())
	{
		m_socket.write(m_fileName, payloadLength);

		if ((length = m_socket.read(buffer, sizeof(buffer))) > 0) // TODO: CAW - Size
		{
			buffer[length] = '\0';

			if (strcmp(buffer, "ok") == 0)
			{
				// TODO: CAW - Use StdFuncs function for this?
				fseek(file, 0, SEEK_END);
				totalSize = ftell(file);
				fseek(file, 0, SEEK_SET);

				SWAP(&totalSize);
				// TODO: CAW - Check return value here and elsewhere
				m_socket.write(&totalSize, sizeof(totalSize));

				while ((size = fread(buffer, 1, sizeof(buffer), file)) > 0)
				{
					m_socket.write(buffer, static_cast<int>(size));
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

void CShutdown::execute()
{
	send();
}
