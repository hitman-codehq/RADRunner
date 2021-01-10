
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
	std::string message;
	RFile file;

	readPayload();

	// Extract the filename from the payload
	char *fileName = reinterpret_cast<char *>(m_payload);

	message = "ok";
	m_socket->write(message.c_str(), static_cast<int>(message.length()));

	m_socket->read(&fileSize, sizeof(fileSize));
	SWAP(&fileSize);

	printf("send: Receiving file \"%s\" of size %u\n", fileName, fileSize);

	// The Framework doesn't truncate a file if it already exists, so we have to try and create it first and
	// if it already exists, then open it normally
	int result = file.Create(fileName, EFileWrite);

	if (result == KErrAlreadyExists)
	{
		result = file.open(fileName, EFileWrite);
	}

	if (result == KErrNone)
	{
		int bytesRead = 0, bytesToRead, size;
		unsigned char buffer[1024]; // TODO: CAW

		do
		{
			bytesToRead = ((fileSize - bytesRead) >= sizeof(buffer)) ? sizeof(buffer) : (fileSize - bytesRead);
			size = m_socket->read(buffer, bytesToRead); // TODO: CAW - Error checking all through here

			if (size > 0)
			{
				file.write(reinterpret_cast<unsigned char *>(buffer), size);
				bytesRead += size;
			}
		}
		while (bytesRead < static_cast<int>(fileSize)); // TODO: CAW - Handle failure

		printf("send: Wrote %d bytes to file \"%s\"\n", bytesRead, fileName);

		file.close();

#ifdef __amigaos__

		Utils::setProtection(fileName, 0);

#elif defined(__unix__)

		// TODO: CAW - Error checking + these need to be abstracted and passed as a part of the message
		Utils::setProtection(fileName, (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

#endif /* __unix__ */

	}
}
