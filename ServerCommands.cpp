
#include <StdFuncs.h>
#include <File.h>
#include "ClientCommands.h"
#include "StdSocket.h"
#include <sys/stat.h>

#ifdef __amigaos__

#include <sys/wait.h>

#endif /* __amigaos__ */

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Wednesday 29-Jan-2020 2:24 pm, Scoot flight TR 735 to Singapore
 * @param	Parameter		Description
 * @return	Return value
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
		// TODO: CAW - Fix this for Windows
		printf("execute: Command failed, return code = %d\n", result); //WEXITSTATUS(result));
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
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Wednesday 29-Jan-2020 12:38 pm, Scoot flight TR 735 to Singapore
 * @param	Parameter		Description
 * @return	Return value
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
