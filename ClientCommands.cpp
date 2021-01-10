
#include <StdFuncs.h>
#include <File.h>
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
 * @date	Wednesday 29-Jan-2020 2:13 pm, Scoot flight TR 735 to Singapore
 * @param	Parameter		Description
 * @return	Return value
 */

void CExecute::sendRequest()
{
	int32_t payloadLength = static_cast<int32_t>(strlen(m_fileName) + 1);
	m_command.m_length = payloadLength;

	if (sendCommand())
	{
		m_socket->write(m_fileName, payloadLength);
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

void CSend::sendRequest()
{
	char buffer[1024]; // TODO: CAW
	int length;
	size_t size;
	uint32_t totalSize;
	RFile file;

	if ((file.open(m_fileName, EFileRead)) != KErrNone)
	{
		Utils::Error("Unable to open file \"%s\"", m_fileName);

		return;
	}

	// Strip any path component from the file as we want it to be written to the current directory
	// in the destination
	const char *fileName = Utils::filePart(m_fileName);

	// Send the length of just the filename as the payload length
	int32_t payloadLength = static_cast<int32_t>(strlen(fileName) + 1);
	m_command.m_length = payloadLength;

	if (sendCommand())
	{
		m_socket->write(fileName, payloadLength);

		if ((length = m_socket->read(buffer, (sizeof(buffer) - 1))) > 0)
		{
			buffer[length] = '\0';

			if (strcmp(buffer, "ok") == 0)
			{
				TEntry entry;

				if (Utils::GetFileInfo(m_fileName, &entry) == KErrNone)
				{
					totalSize = entry.iSize;

					SWAP(&totalSize);
					m_socket->write(&totalSize, sizeof(totalSize));

					while ((size = file.read(reinterpret_cast<unsigned char *>(buffer), sizeof(buffer))) > 0)
					{
						m_socket->write(buffer, static_cast<int>(size));
					}
				}
			}
		}
	}

	file.close();
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

void CShutdown::sendRequest()
{
	sendCommand();
}
