
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
	RFile file;

	printf("%s: Sending file \"%s\"\n", g_commandNames[m_command.m_command], m_fileName);

	if (file.open(m_fileName, EFileRead) != KErrNone)
	{
		Utils::Error("Unable to open file \"%s\"", m_fileName);

		return;
	}

	file.close();

	// Strip any path component from the file as we want it to be written to the current directory
	// in the destination
	const char *fileName = Utils::filePart(m_fileName);

	// Send the length of just the filename as the payload length
	int32_t payloadLength = static_cast<int32_t>(strlen(fileName) + 1);
	m_command.m_length = payloadLength;

	if (sendCommand())
	{
		if (m_socket->write(fileName, payloadLength) == payloadLength)
		{
			sendFile(m_fileName);
		}
		else
		{
			Utils::Error("Unable to send payload");
		}
	}
	else
	{
		Utils::Error("Unable to send request");
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
