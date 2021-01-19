
#include <StdFuncs.h>
#include <File.h>
#include <string.h>
#include "ClientCommands.h"
#include "StdSocket.h"

const char *g_commandNames[] =
{
	"execute",
	"get",
	"send",
	"shutdown"
};

/**
 * Requests execution of a file.
 * Requests the remote execution of an executable or script file.  The file's stdout and stderr output
 * will be displayed on the remote console only.
 *
 * @date	Wednesday 29-Jan-2020 2:13 pm, Scoot flight TR 735 to Singapore
 */

void CExecute::sendRequest()
{
	int32_t payloadLength = static_cast<int32_t>(strlen(m_fileName) + 1);
	m_command.m_length = payloadLength;

	printf("execute: Executing file \"%s\"\n", m_fileName);

	if (sendCommand())
	{
		if (m_socket->write(m_fileName, payloadLength) != payloadLength)
		{
			Utils::Error("Unable to send payload");
		}
	}
	else
	{
		Utils::Error("Unable to send request");
	}
}

/**
 * Requests a file from the remote server.
 * Transfers a file from the remote server to a local host and displays an error if the file is not able
 * to be found on the server.  If a qualified filename is specified then the path will be used on the remote
 * server but stripped on the local client, so that the file will be written into the current directory.
 *
 * @date	Saturday 16-Jan-2021 11:56 am, Code HQ Bergmannstrasse
 */

void CGet::sendRequest()
{
	int length;
	int32_t payloadLength = static_cast<int32_t>(strlen(m_fileName) + 1);
	int32_t result;
	m_command.m_length = payloadLength;

	printf("get: Requesting file \"%s\"\n", m_fileName);

	if (sendCommand())
	{
		if (m_socket->write(m_fileName, payloadLength) == payloadLength)
		{
			// Read the response to the request and if it was successful, transfer the file
			if ((length = m_socket->read(&result, (sizeof(result)))) > 0)
			{
				if (result == KErrNone)
				{
					uint32_t totalSize;

					m_socket->read(&totalSize, sizeof(totalSize));
					SWAP(&totalSize);

					// Transfer the file from the remote server, stripping any path present in its name, and
					// store it in the current directory
					readFile(Utils::filePart(m_fileName), totalSize);
				}
				else
				{
					Utils::Error("Received invalid response %d", result);
				}
			}
			else
			{
				Utils::Error("Unable to read response");
			}
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
}

/**
 * Sends a file to the remote server.
 * Transfers a file to the remote server.  If a qualified filename is specified then the path will be stripped,
 * so that the file will be written into the current directory of the remote server.
 *
 * @date	Sunday 17-Nov-2019 3:30 pm, Sankt Oberholz
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
 * Shuts down the remote server.
 * Sends a request to the remote server that it shuts itself down.
 *
 * @date	Wednesday 29-Jan-2020 2:52 pm, Scoot flight TR 735 to Singapore
 */

void CShutdown::sendRequest()
{
	printf("shutdown: Shutting down server\n");

	if (!sendCommand())
	{
		Utils::Error("Unable to send request");
	}
}
