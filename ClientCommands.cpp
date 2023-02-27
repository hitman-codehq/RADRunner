
#include <StdFuncs.h>
#include <File.h>
#include <StdSocket.h>
#include <Yggdrasil/Commands.h>
#include <string.h>

/**
 * Requests the deletion of a file.
 * This command is a part of the protocol used by RADRunner, but currently, only the server-side
 * version of this command is implemented.
 *
 * @date	Wednesday 08-Mar-2023 7:50 am, Code HQ Tokyo Tsukuda
 */

void CDelete::sendRequest()
{
}

/**
 * Requests a directory listing.
 * Requests the remote server list the contents of the specified directory.  The name of the directory
 * to be listed is passed in as the payload.  If "" or "." is passed in, the contents of the current
 * directory will be listed.
 *
 * @date	Wednesday 11-Jan-2023 6:52 am, Code HQ Tokyo Tsukuda
 */

void CDir::sendRequest()
{
	int32_t payloadSize = static_cast<int32_t>(strlen(m_directoryName) + 1);

	printf("dir: Requesting directory listing for \"%s\"\n", m_directoryName);

	m_command.m_size = payloadSize;
	sendCommand();
	m_socket->write(m_directoryName, payloadSize);
	readResponse();

	if (m_response.m_result == KErrNone)
	{
		char *name;
		uint32_t size;
		unsigned char *payload = m_responsePayload, *payloadEnd = m_responsePayload + m_response.m_size;

		/* Iterate through the file information in the payload and display its contents.  Provided the payload */
		/* is structured correctly, we could just check for it being ended by NULL terminator, but in the */
		/* interest of safety, we'll also check that we haven't overrun the end */
		while (payload < payloadEnd && *payload != '\0')
		{
			name = reinterpret_cast<char *>(payload);
			payload += strlen(name) + 1;
			STREAM_INT(size, payload);
			payload += sizeof(size);
			printf("%s %d\n", name, size);

			ASSERTM((payload < payloadEnd), "CDir::sendRequest() => Payload contents do not match its size");
		}
	}
	else
	{
		Utils::Error("Received invalid response %d", m_response.m_result);
	}
}

/**
 * Requests execution of a file.
 * Requests the remote execution of an executable or script file.  The file's stdout and stderr output
 * will be displayed on the remote console and will also be returned to the client, for display there.
 *
 * @date	Wednesday 29-Jan-2020 2:13 pm, Scoot flight TR 735 to Singapore
 */

void CExecute::sendRequest()
{
	int32_t payloadSize = static_cast<int32_t>(strlen(m_fileName) + 1);

	printf("execute: Executing file \"%s\"\n", m_fileName);

	m_command.m_size = payloadSize;
	sendCommand();
	m_socket->write(m_fileName, payloadSize);

	SResponse response;

	/* Read the response to the request and if it was successful, stream the response data */
	m_socket->read(&response, sizeof(response));

	SWAP(&response.m_result);

	if (response.m_result == KErrNone)
	{
		bool done = false;
		char *buffer = new char[STDOUT_BUFFER_SIZE];
		int bytesRead;

		/* Loop around and read the remote command's stdout and stderr output and print it out.  This */
		/* will be terminated by two NULL terminators in a row, so search each received line for these */
		/* and, when found, break out of the loop */
		do
		{
			if ((bytesRead = m_socket->read(buffer, (STDOUT_BUFFER_SIZE - 1), false)) > 0)
			{
				buffer[bytesRead] = '\0';
				printf("%s", buffer);

				/* If at least two bytes have been received then check for them both being NULL */
				if (bytesRead >= 2)
				{
					for (int index = 0; index < (bytesRead - 1); ++index)
					{
						if ((buffer[index] == 0) && (buffer[index + 1] == 0))
						{
							printf("execute: Command complete\n");
							done = true;
						}
					}
				}
			}
			else
			{
				done = true;
			}
		}
		while (!done);

		delete [] buffer;
	}
	else
	{
		Utils::Error("Received invalid response %d", response.m_result);
	}
}

/**
 * Requests detailed file information from the server.
 * This command is a part of the protocol used by RADRunner, but currently, only the server-side
 * version of this command is implemented.
 *
 * @date	Friday 24-Feb-2023 8:10 am, Code HQ Tokyo Tsukuda
 */

void CFileInfo::sendRequest()
{
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
	int32_t payloadSize = static_cast<int32_t>(strlen(m_fileName) + 1);

	printf("get: Requesting file \"%s\"\n", m_fileName);

	m_command.m_size = payloadSize;
	sendCommand();
	m_socket->write(m_fileName, payloadSize);
	readResponse();

	if (m_response.m_result == KErrNone)
	{
		/* Extract the file's information from the payload */
		SFileInfo *fileInfo = reinterpret_cast<SFileInfo *>(m_responsePayload);
		SWAP64(&fileInfo->m_microseconds);

		/* Transfer the file from the remote server */
		if (readFile(fileInfo->m_fileName) == KErrNone)
		{
			/* And set its datestamp and protection bits */
			setFileInformation(*fileInfo);
		}
	}
	else
	{
		Utils::Error("Received invalid response %d", m_response.m_result);
	}
}

/**
 * Requests the renaming of a file.
 * This command is a part of the protocol used by RADRunner, but currently, only the server-side
 * version of this command is implemented.
 *
 * @date	Saturday 11-Mar-2023 6:47 am, Code HQ Tokyo Tsukuda
 */

void CRename::sendRequest()
{
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

	printf("send: Sending file \"%s\"\n", m_fileName);

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

	/* Strip any path component from the file as we want it to be written to the current directory */
	/* in the destination */
	const char *fileName = Utils::filePart(m_fileName);

	/* Include the size of just the filename in the payload size */
	int32_t payloadSize = static_cast<int32_t>(sizeof(SFileInfo) + strlen(fileName) + 1);
	m_command.m_size = payloadSize;

	sendCommand();

	/* Allocate an SFileInfo structure of a size large enough to hold the file's name */
	SFileInfo *fileInfo = reinterpret_cast<SFileInfo *>(new unsigned char [payloadSize]);

	/* Initialise it with the file's name and timestamp */
	fileInfo->m_microseconds = entry.iModified.Int64();
	SWAP64(&fileInfo->m_microseconds);
	strcpy(fileInfo->m_fileName, fileName);

	/* And finally send the payload and the file itself */
	m_socket->write(fileInfo, payloadSize);

	sendFile(m_fileName);

	delete [] reinterpret_cast<unsigned char *>(fileInfo);
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

	sendCommand();
}

/**
 * Verfies the client's protocol version.
 * Requests the remote server's protocol version, and shuts the client down if it is not supported.
 *
 * @date	Saturday 06-Feb-2021 6:51 am, Code HQ Bergmannstrasse
 */

void CVersion::sendRequest()
{
	sendCommand();

	uint32_t serverVersion, version = ((PROTOCOL_MAJOR << 16) | PROTOCOL_MINOR);
	m_socket->read(&serverVersion, sizeof(serverVersion));
	SWAP(&serverVersion);

	if (serverVersion != version)
	{
		throw std::runtime_error("Incompatible server version detected, shutting down");
	}
}
