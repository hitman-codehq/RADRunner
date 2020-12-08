
#include <StdFuncs.h>
#include "ClientCommands.h"
#include "ServerCommands.h"
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

// TODO: CAW - This name is as bad as the American girl's English (like, like) in the seat behind me
void ExecuteServer(RSocket &a_socket, struct SCommand *a_command)
{
	char buffer[1024]; // TODO: CAW
	int result;

	a_socket.read(buffer, a_command->m_length);

	printf("execute: Executing command \"%s\"\n", buffer);
	result = system(buffer);

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

void ReceiveFile(RSocket &a_socket, struct SCommand *a_command)
{
	char buffer[1024]; // TODO: CAW
	uint32_t fileSize;
	std::string fileName, message;
	FILE *file;

	a_socket.read(buffer, a_command->m_length);

	// Extract the filename from the payload
	fileName = buffer;

	message = "ok";
	a_socket.write(message.c_str(), message.length());

	a_socket.read(&fileSize, sizeof(fileSize));
	SWAP(&fileSize);

	printf("send: Receiving file \"%s\" of size %u\n", fileName.c_str(), fileSize);

	file = fopen(buffer, "wb");

	if (file != nullptr)
	{
		int bytesRead = 0, bytesToRead, size;

		do
		{
			bytesToRead = ((fileSize - bytesRead) >= sizeof(buffer)) ? sizeof(buffer) : (fileSize - bytesRead);
			size = a_socket.read(buffer, bytesToRead); // TODO: CAW - Error checking all through here

			if (size > 0)
			{
				fwrite(buffer, 1, size, file);
				bytesRead += size;
			}
		}
		while (bytesRead < static_cast<int>(fileSize)); // TODO: CAW - Handle failure

		printf("send: Wrote %d bytes to file \"%s\"\n", bytesRead, fileName.c_str());

		fclose(file);

#ifdef __amigaos__

		Utils::setProtection(fileName.c_str(), 0);

#elif defined(__unix__)

		// TODO: CAW - Error checking + these need to be abstracted and passed as a part of the message
		Utils::setProtection(fileName.c_str, (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

#endif /* __unix__ */

	}
}
