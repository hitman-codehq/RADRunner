
#include <StdFuncs.h>
#include <File.h>
#include "Commands.h"
#include "StdSocket.h"

/**
 * Reads a file from a connected socket.
 * A convenience method used to read a file from a socket and to write its contents to a local file in
 * the file system.  This method assumes that the file has been requested and that upon reading the socket,
 * the entire file will be able to be read.
 *
 * @pre		A file has been requested from the connected endpoint
 *
 * @date	Sunday 17-Jan-2021 5:39 am, Code HQ Bergmannstrasse
 * @param	a_fileName		The name of the local file into which to write the file's contents
 * @param	a_fileSize		The size of the file in bytes
 * @return	KErrNone if successful, else one of the system errors
 */

int CHandler::readFile(const char *a_fileName, uint32_t a_fileSize)
{
	RFile file;

	printf("%s: Transferring file \"%s\" of size %u\n", g_commandNames[m_command.m_command], a_fileName, a_fileSize);

	// The Framework doesn't truncate a file if it already exists, so we have to try and create it first and
	// if it already exists, then open it normally
	int retVal = file.Create(a_fileName, EFileWrite);

	if (retVal == KErrAlreadyExists)
	{
		retVal = file.open(a_fileName, EFileWrite);
	}

	if (retVal == KErrNone)
	{
		int bytesRead = 0, bytesToRead, size;
		unsigned char buffer[1024]; // TODO: CAW

		do
		{
			bytesToRead = ((a_fileSize - bytesRead) >= sizeof(buffer)) ? sizeof(buffer) : (a_fileSize - bytesRead);
			size = m_socket->read(buffer, bytesToRead);

			if (size > 0)
			{
				file.write(reinterpret_cast<unsigned char *>(buffer), size);
				bytesRead += size;
			}
		}
		while (bytesRead < static_cast<int>(a_fileSize));

		printf("%s: Wrote %d bytes to file \"%s\"\n", g_commandNames[m_command.m_command], bytesRead, a_fileName);

		file.close();
	}
	else
	{
		Utils::Error("Unable to open file \"%s\" for writing", a_fileName);
	}

	return retVal;
}

/**
 * Reads the command's payload.
 * A convenience method that will allocate memory for the command's payload, and read the payload into that
 * memory.  If the command has no payload, no action will be performed but success will still be returned.
 *
 * @date	Sunday 10-Jan-2021 6:39 am, Code HQ Bergmannstrasse
 * @return	true if the payload was read successfully, else false
 */

bool CHandler::readPayload()
{
	bool retVal = true;

	// If the command has a payload, allocate an appropriately sized buffer and read it
	if (m_command.m_size > 0)
	{
		m_payload = new unsigned char[m_command.m_size];
		retVal = (m_socket->read(m_payload, m_command.m_size) == static_cast<int>(m_command.m_size));
	}

	return retVal;
}

/**
 * Sends a command, without payload.
 * A convenience method to send a command, ensuring that its members are in network format before sending.
 *
 * @date	Sunday 29-Nov-2020 12:17 pm, Code HQ Bergmannstrasse
 * @return	true if the command was sent successfully, else false
 */

bool CHandler::sendCommand()
{
	SCommand command = m_command;

	SWAP(&command.m_command);
	SWAP(&command.m_size);

	return (m_socket->write(&command, sizeof(command)) == sizeof(command));
}

/**
 * Writes a file to a connected socket.
 * A convenience method used to read a file and to write its contents to a connected socket.  This method
 * assumes that the remote endpoint is expecting the file.
 *
 * @pre		The connected endpoint has requested a file
 *
 * @date	Sunday 17-Jan-2021 5:50 am, Code HQ Bergmannstrasse
 * @param	a_fileName		The name of the local file from which to read the file's contents
 * @return	KErrNone if successful, else one of the system errors
 */

int CHandler::sendFile(const char *a_fileName)
{
	unsigned char buffer[1024]; // TODO: CAW
	int retVal, totalSize;
	size_t size;
	TEntry entry;

	printf("%s: Transferring file \"%s\"\n", g_commandNames[m_command.m_command], a_fileName);

	if ((retVal = Utils::GetFileInfo(a_fileName, &entry)) == KErrNone)
	{
		totalSize = entry.iSize;
		SWAP(reinterpret_cast<unsigned int *>(&totalSize));

		if (m_socket->write(&totalSize, sizeof(totalSize)) == sizeof(totalSize))
		{
			RFile file;

			if ((retVal = file.open(a_fileName, EFileRead)) == KErrNone)
			{
				while ((size = file.read(buffer, sizeof(buffer))) > 0)
				{
					m_socket->write(buffer, static_cast<int>(size));
				}

				file.close();
			}
			else
			{
				Utils::Error("Unable to open file \"%s\" for reading", a_fileName);
			}
		}
		else
		{
			Utils::Error("Unable to write file size");
		}
	}
	else
	{
		Utils::Error("Unable to query file \"%s\"", a_fileName);
	}

	return retVal;
}
