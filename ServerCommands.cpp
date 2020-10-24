
#include <StdFuncs.h>
#include "ServerCommands.h"
#include "StdSocket.h"
#include <sys/stat.h>

#ifdef __amigaos__

#define SWAP(number)

#else /* ! __amigaos__ */

#define SWAP(number) Utils::swap32(number)

#endif /* ! __amigaos__ */

#ifdef __unix__

static const char outfileName[] = "./outfile";

#else /* ! __unix__ */

static const char outfileName[] = "outfile.exe";

#endif /* ! __unix__ */

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
void ExecuteServer()
{
	printf("execute: Executing command \"%s\"\n", outfileName);
	system(outfileName);
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

void ReceiveFile(RSocket &a_socket)
{
	char buffer[1024]; // TODO: CAW
	uint32_t fileSize;
	std::string message;
	FILE *file;

	message = "ok";
	a_socket.write(message.c_str(), message.length());

	a_socket.read(&fileSize, sizeof(fileSize));
	SWAP(&fileSize);

	printf("Receiving file of size %u\n", fileSize);

	file = fopen(outfileName, "wb");

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

		printf("ReceiveFile: Wrote %d bytes to file \"%s\"\n", bytesRead, outfileName);

		fclose(file);

#ifdef __amigaos__

		Utils::setProtection(outfileName, 0);

#elif defined(__unix__)

		// TODO: CAW - Error checking + these need to be abstracted and passed as a part of the message
		Utils::setProtection(outfileName, (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

#endif /* __unix__ */

	}
}
