
#include <StdFuncs.h>
#include <File.h>
#include <StdSocket.h>
#include <string.h>
#include "Commands.h"

#ifdef __amigaos__

#include <sys/wait.h>

#endif /* __amigaos__ */

/**
 * Executes a file.
 * Runs the executable or script file specified by the client, displaying its stdout and stderr output
 * on the console.
 *
 * @date	Wednesday 29-Jan-2020 2:24 pm, Scoot flight TR 735 to Singapore
 */

void CExecute::execute()
{
	readPayload();

	printf("execute: Executing command \"%s\"\n", m_payload);

	SResponse response;
	response.m_size = 0;
	response.m_result = launchCommand(reinterpret_cast<char *>(m_payload));

	/* Write a failure completion code, to let the client know that it should not listen for the */
	/* command output to be streamed */
	if (response.m_result != KErrNone)
	{
		SWAP(&response.m_result);
		m_socket->write(&response, sizeof(response));
	}

	/* If the client was launched successfully then send two NULL terminators in a row.  This is the */
	/* signal to the client that the stdout output stream has ended */
	if (response.m_result == KErrNone)
	{
		char terminators[2] = { 0, 0 };
		m_socket->write(terminators, sizeof(terminators));
	}
	else if ((response.m_result == -1) || (response.m_result == 127))
	{
		printf("execute: Unable to launch command\n");
	}
	else if (response.m_result != 0)
	{
		/* It's a bit crazy but launching behaves differently under Windows, so we have to take this */
		/* into account */

#ifdef WIN32

		printf("execute: Command failed, return code = %d\n", response.m_result);

#else /* WIN32 */

		printf("execute: Command failed, return code = %d\n", WEXITSTATUS(response.m_result));

#endif /* WIN32 */

	}
}

/**
 * Sends a file to the remote client.
 * Transfers a file, if it exists, to the remote client.  If the file does not exist then an error will be
 * sent instead, to indicate this.
 *
 * @date	Saturday 16-Jan-2021 11:54 am, Code HQ Bergmannstrasse
 */

void CGet::execute()
{
	readPayload();

	/* Extract the filename from the payload */
	m_fileName = reinterpret_cast<char *>(m_payload);

	int result;
	SResponse response;
	TEntry entry;

	/* Determine if the file exists and send the result to the remote client */
	result = response.m_result = Utils::GetFileInfo(m_fileName, &entry);
	SWAP(&response.m_result);

	/* If the file exists then send a response and a payload, containing the file's timestamp */
	if (result == KErrNone)
	{
		/* Strip any path component from the file as we want it to be written to the current directory */
		/* in the destination */
		const char *fileName = Utils::filePart(m_fileName);

		/* Include the size of just the filename in the payload size */
		int32_t payloadSize = static_cast<int32_t>(sizeof(SFileInfo) + strlen(fileName) + 1);

		/* Allocate an SFileInfo structure of a size large enough to hold the file's name */
		SFileInfo *fileInfo = reinterpret_cast<SFileInfo *>(new unsigned char [payloadSize]);

		/* Initialise it with the file's name and timestamp */
		fileInfo->m_microseconds = entry.iModified.Int64();
		SWAP64(&fileInfo->m_microseconds);
		strcpy(fileInfo->m_fileName, fileName);

		/* And finally send the response and its payload */
		response.m_size = payloadSize;
		SWAP(&response.m_size);

		m_socket->write(&response, sizeof(response));
		m_socket->write(fileInfo, payloadSize);

		delete [] reinterpret_cast<unsigned char *>(fileInfo);
	}
	/* Otherwise just send a response with an empty payload */
	else
	{
		response.m_size = 0;
		m_socket->write(&response, sizeof(response));
	}

	/* If the file exists then the remote client will be awaiting its transfer, so send it now */
	if (response.m_result  == KErrNone)
	{
		sendFile(m_fileName);
	}
}

/**
 * Receives a file from the remote client.
 * Transfers a file from the remote client and writes it to a local file.  The file will be written
 * into the current directory.
 *
 * @date	Wednesday 29-Jan-2020 12:38 pm, Scoot flight TR 735 to Singapore
 */

void CSend::execute()
{
	readPayload();

	/* Extract the file's information from the payload */
	SFileInfo *fileInfo = reinterpret_cast<SFileInfo *>(m_payload);
	SWAP64(&fileInfo->m_microseconds);
	m_fileName = fileInfo->m_fileName;

	/* Transfer the file from the remote client */
	if (readFile(m_fileName) == KErrNone)
	{
		/* And set its datestamp and protection bits */
		setFileInformation(*fileInfo);
	}
}

/**
 * Sends the server's protocol version.
 * Sends the currently supported protocol version to the client.  No validation checking is done as it is
 * the client's responsibility to determine compatibility.
 *
 * @date	Saturday 06-Feb-2021 7:02 am, Code HQ Bergmannstrasse
 */

void CVersion::execute()
{
	readPayload();

	uint32_t serverVersion = ((PROTOCOL_MAJOR << 16) | PROTOCOL_MINOR);
	SWAP(&serverVersion);
	m_socket->write(&serverVersion, sizeof(serverVersion));
}
