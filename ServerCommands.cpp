
#include <StdFuncs.h>
#include "ServerCommands.h"
#include "StdSocket.h"
#include <sys/stat.h>
#include <unistd.h>

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
	std::string message;
	FILE *file;

    message = "ok";
    a_socket.Write(message.c_str(), message.length());

    file = fopen(outfileName, "wb");

    if (file != nullptr)
    {
        int bytesRead = 0, bytesToRead, size, totalSize = 8144; // TODO: CAW - This should be sent by the client

        do
        {
            bytesToRead = ((totalSize - bytesRead) >= sizeof(buffer)) ? sizeof(buffer) : (totalSize - bytesRead); // TODO: CAW
            size = a_socket.Read(buffer, bytesToRead); // TODO: CAW - Error checking all through here

            if (size > 0)
            {
                fwrite(buffer, 1, size, file);
                bytesRead += size;
            }
        }
        while (bytesRead < totalSize); // TODO: CAW - Handle failure

        printf("send: Wrote %d bytes to file \"%s\"\n", bytesRead, outfileName);

        fclose(file);

#ifdef __unix__

        // TODO: CAW - Error checking + these need to be abstracted and passed as a part of the message
        Utils::SetProtection(outfileName, (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

#endif /* __unix__ */

    }
}
