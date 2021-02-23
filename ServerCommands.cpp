
#include <StdFuncs.h>
#include <File.h>
#include "Commands.h"
#include "StdSocket.h"
#include <sys/stat.h>

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
	int result;

	readPayload();

	printf("execute: Executing command \"%s\"\n", m_payload);

#ifdef WIN32

	result = launchCommand(reinterpret_cast<char *>(m_payload));

	/* Write a failure completion code, to let the client know that it should not listen for the */
	/* command output to be streamed */
	if (result != KErrNone)
	{
		m_socket->write(&result, sizeof(result));
	}

#else /* ! WIN32 */

	result = system(reinterpret_cast<const char *>(m_payload));

	/* Write the completion code, to let the client know whether it should listen for the command */
	/* output to be streamed */
	m_socket->write(&result, sizeof(result));

#endif /* ! WIN32 */

	/* If the client was launched successfully then send two NULL terminators in a row.  This is the */
	/* signal to the client that the stdout output stream has ended */
	if (result == KErrNone)
	{
		char terminators[2] = { 0, 0 };
		m_socket->write(terminators, sizeof(terminators));
	}
	else if ((result == -1) || (result == 127))
	{
		printf("execute: Unable to launch command\n");
	}
	else if (result != 0)
	{
		/* It's a bit crazy but launching behaves differently under Windows, so we have to take this */
		/* into account */

#ifdef WIN32

		printf("execute: Command failed, return code = %d\n", result);

#else /* WIN32 */

		printf("execute: Command failed, return code = %d\n", WEXITSTATUS(result));

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

	int32_t result;
	TEntry entry;

	/* Determine if the file exists and send the result to the remote client */
	result = Utils::GetFileInfo(m_fileName, &entry);
	m_socket->write(&result, sizeof(result));

	/* If the file exists then the remote client will be awaiting its transfer, so send it now */
	if (result  == KErrNone)
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
	uint32_t fileSize;

	readPayload();

	/* Extract the file's information from the payload */
	struct SFileInfo *fileInfo = reinterpret_cast<struct SFileInfo *>(m_payload);
	SWAP64(&fileInfo->m_microseconds);
	m_fileName = fileInfo->m_fileName;

	m_socket->read(&fileSize, sizeof(fileSize));
	SWAP(&fileSize);

	if (readFile(m_fileName, fileSize) == KErrNone)
	{
		/* Create a TEntry instance and use the transferred microseconds value to initialise its timestamp */
		/* related members, so that it can be used to set the timestamp of the file just received */
		TEntry entry(TDateTime(fileInfo->m_microseconds));

		Utils::setFileDate(m_fileName, entry);

#ifdef __amigaos__

		Utils::setProtection(m_fileName, 0);

#elif defined(__unix__)

		// TODO: CAW - Error checking + these need to be abstracted and passed as a part of the message
		Utils::setProtection(m_fileName, (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

#endif /* __unix__ */

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
	if (readPayload())
	{
		uint32_t serverVersion = ((PROTOCOL_MAJOR << 16) | PROTOCOL_MINOR);

		SWAP(&serverVersion);

		if (m_socket->write(&serverVersion, sizeof(serverVersion)) != sizeof(serverVersion))
		{
			Utils::Error("Unable to send server version");
		}
	}
}
