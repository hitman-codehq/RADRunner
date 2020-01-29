
#include <StdFuncs.h>
#include <StdSocket.h>
#include "ServerCommands.h"
#include <sys/stat.h>
#include <unistd.h>

/**
 * Short description.
 * Long multi line description.
 *
 * @pre		Some precondition here
 *
 * @date	Wednesday 29-Jan-2020 12:38 pm, Scoot flight TODO to Singapore
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

    file = fopen("outfile", "wb");

    if (file != nullptr)
    {
        int bytesRead = 0, bytesToRead, size, totalSize = 8432; // TODO: CAW - This should be sent by the client

        do
        {
            bytesToRead = ((totalSize - bytesRead) >= sizeof(buffer)) ? sizeof(buffer) : (totalSize - bytesRead); // TODO: CAW
            printf("Reading %d bytes\n", bytesToRead);
            size = a_socket.Read(buffer, bytesToRead); // TODO: CAW - Error checking all through here

            if (size > 0)
            {
                fwrite(buffer, 1, size, file);
                printf("Read %d bytes\n", size);
                bytesRead += size;
            }

            printf("bytesRead = %d\n", bytesRead);
        }
        while (bytesRead < totalSize); // TODO: CAW - Handle failure

        fclose(file);

        // TODO: CAW - Error checking
        Utils::SetProtection("outfile", (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

        execl("outfile", "");
    }
}
