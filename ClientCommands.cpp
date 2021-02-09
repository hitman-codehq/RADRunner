
#include <StdFuncs.h>
#include <File.h>
#include <string.h>
#include "Commands.h"
#include "StdSocket.h"

const char *g_commandNames[] =
{
	"execute",
	"get",
	"send",
	"shutdown",
	"version"
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
	int32_t payloadSize = static_cast<int32_t>(strlen(m_fileName) + 1);

	printf("execute: Executing file \"%s\"\n", m_fileName);

	m_command.m_size = payloadSize;

	if (sendCommand())
	{
		if (m_socket->write(m_fileName, payloadSize) != payloadSize)
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
	int size;
	int32_t payloadSize = static_cast<int32_t>(strlen(m_fileName) + 1);
	int32_t result;

	printf("get: Requesting file \"%s\"\n", m_fileName);

	m_command.m_size = payloadSize;

	if (sendCommand())
	{
		if (m_socket->write(m_fileName, payloadSize) == payloadSize)
		{
			// Read the response to the request and if it was successful, transfer the file
			if ((size = m_socket->read(&result, (sizeof(result)))) > 0)
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
	TEntry entry;

	printf("%s: Sending file \"%s\"\n", g_commandNames[m_command.m_command], m_fileName);

	/* If the file doesn't exist then bail out immediately */
	if (Utils::GetFileInfo(m_fileName, &entry) != KErrNone)
	{
		Utils::Error("Unable to open file \"%s\"", m_fileName);

		return;
	}

	/* If the file specified is actually a directory then the server will be waiting for data that will */
	/* never arrive, so check for this and bail out immediately */
	if (entry.IsDir())
	{
		Utils::Error("\"%s\" is a directory", m_fileName);

		return;
	}

	// Strip any path component from the file as we want it to be written to the current directory
	// in the destination
	const char *fileName = Utils::filePart(m_fileName);

	// Send the size of just the filename as the payload Size
	int32_t payloadSize = static_cast<int32_t>(sizeof(SFileInfo) + strlen(fileName) + 1);
	m_command.m_size = payloadSize;

	if (sendCommand())
	{
		// Allocate an SFileInfo structure of a size large enough to hold the file's name
		struct SFileInfo *fileInfo = reinterpret_cast<struct SFileInfo *>(new unsigned char [payloadSize]);

		// And initialise it with the file's name and timestamp
		fileInfo->m_microseconds = entry.iModified.Int64();
		SWAP64(&fileInfo->m_microseconds);
		strcpy(fileInfo->m_fileName, fileName);

		if (m_socket->write(fileInfo, payloadSize) == payloadSize)
		{
			sendFile(m_fileName);
		}
		else
		{
			Utils::Error("Unable to send payload");
		}

		delete [] reinterpret_cast<unsigned char *>(fileInfo);
	}
	else
	{
		Utils::Error("Unable to send request");
	}
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

/**
 * Verfies the client's protocol version.
 * Requests the remote server's protocol version, and shuts the client down if it is not supported.
 *
 * @date	Saturday 06-Feb-2021 6:51 am, Code HQ Bergmannstrasse
 */

void CVersion::sendRequest()
{
	if (sendCommand())
	{
		uint32_t serverVersion, version = ((PROTOCOL_MAJOR << 16) | PROTOCOL_MINOR);

		if (m_socket->read(&serverVersion, sizeof(serverVersion)) == sizeof(serverVersion))
		{
			SWAP(&serverVersion);

			if (serverVersion != version)
			{
				printf("version: Incompatible server version detected, shutting down\n");
				exit(1);
			}
		}
		else
		{
			Utils::Error("Unable to read response");
		}
	}
	else
	{
		Utils::Error("Unable to send request");
	}
}
