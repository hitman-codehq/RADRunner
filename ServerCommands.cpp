
#include <StdFuncs.h>
#include <File.h>
#include "ClientCommands.h"
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
	result = system(reinterpret_cast<const char *>(m_payload));

	if ((result == -1) || (result == 127))
	{
		printf("execute: Unable to launch command\n");
	}
	else if (result != 0)
	{
		// It's a bit crazy but system() behaves differently under Windows, so we have to take this
		// into account when calling it

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

	// Extract the filename from the payload
	m_fileName = reinterpret_cast<char *>(m_payload);

	int32_t result;
	TEntry entry;

	// Determine if the file exists and send the result to the remote client
	result = Utils::GetFileInfo(m_fileName, &entry);
	m_socket->write(&result, sizeof(result));

	// If the file exists then the remote client will be awaiting its transfer, so send it now
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
	RFile file;

	readPayload();

	// Extract the filename from the payload
	m_fileName = reinterpret_cast<char *>(m_payload);

	m_socket->read(&fileSize, sizeof(fileSize));
	SWAP(&fileSize);

	if (readFile(m_fileName, fileSize) == KErrNone)
	{

#ifdef __amigaos__

		Utils::setProtection(m_fileName, 0);

#elif defined(__unix__)

		// TODO: CAW - Error checking + these need to be abstracted and passed as a part of the message
		Utils::setProtection(m_fileName, (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

#endif /* __unix__ */

	}
}
