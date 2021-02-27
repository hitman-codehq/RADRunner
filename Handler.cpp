
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

	/* The Framework doesn't truncate a file if it already exists, so we have to try and create it first and */
	/* if it already exists, then open it normally */
	int retVal = file.Create(a_fileName, EFileWrite);

	if (retVal == KErrAlreadyExists)
	{
		retVal = file.open(a_fileName, EFileWrite);
	}

	if (retVal == KErrNone)
	{
		int bytesRead = 0, bytesToRead, size;
		unsigned char buffer[1024]; // TODO: CAW

		/* Determine the start time so that it can be used to calculate the amount of time the transfer took */
		TTime now;
		now.HomeTime();
		TInt64 startTime = now.Int64();

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

		/* Determine the end time and the number of milliseconds taken to perform the transfer */
		now.HomeTime();
		TInt64 endTime = now.Int64();
		TInt64 total = ((endTime - startTime) / 1000);

		/* Cast the time results to integers when printing as Amiga OS doesn't support 64 bit format specifiers */
		printf("%s: Wrote %d.%d Kilobytes to file \"%s\" in %d.%d seconds\n", g_commandNames[m_command.m_command], (bytesRead / 1024),
			(bytesRead % 1024), a_fileName, static_cast<int>(total / 1000), static_cast<int>(total % 1000));

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

	/* If the command has a payload, allocate an appropriately sized buffer and read it */
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
				/* Determine the start time so that it can be used to calculate the amount of time the transfer took */
				TTime now;
				now.HomeTime();
				TInt64 startTime = now.Int64();

				while ((size = file.read(buffer, sizeof(buffer))) > 0)
				{
					m_socket->write(buffer, static_cast<int>(size));
				}

				file.close();

				/* Determine the end time and the number of milliseconds taken to perform the transfer */
				now.HomeTime();
				TInt64 endTime = now.Int64();
				TInt64 total = ((endTime - startTime) / 1000);

				/* Cast the time results to integers when printing as Amiga OS doesn't support 64 bit format specifiers */
				printf("%s: Transferred %u.%u Kilobytes in %d.%d seconds\n", g_commandNames[m_command.m_command], (entry.iSize / 1024),
					(entry.iSize % 1024), static_cast<int>(total / 1000), static_cast<int>(total % 1000));
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

/**
 * Sets the datestamp and protection bits for a file.
 * Using the information in the SFileInfo structure passed in, this method will update the datestamp and
 * protection bits on a file, whose name is also stored in said structure.
 *
 * @date	Saturday 27-Feb-2021 7:25 am, Code HQ Bergmannstrasse
 * @param	a_fileInfo		Structure containing information about the file
 */

void CHandler::setFileInformation(const SFileInfo &a_fileInfo)
{
	int result;

	/* Create a TEntry instance and use the given microseconds value to initialise its timestamp */
	/* related members, so that it can be used to set the timestamp of the file just received */
	TEntry entry(TDateTime(a_fileInfo.m_microseconds));

	if ((result = Utils::setFileDate(a_fileInfo.m_fileName, entry)) != KErrNone)
	{
		Utils::Error("Unable to set datestamp on file \"%s\" (Error %d)", a_fileInfo.m_fileName, result);
	}

#ifdef __amigaos__

	result = Utils::setProtection(a_fileInfo.m_fileName, 0);

#elif defined(__unix__)

	// TODO: CAW - These need to be abstracted and passed as a part of the message
	result = Utils::setProtection(a_fileInfo.m_fileName, (S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR));

#else /* ! __unix__ */

	result = KErrNone;

#endif /* ! __unix__ */

	if (result != KErrNone)
	{
		Utils::Error("Unable to set protection bits for file \"%s\" (Error %d)", a_fileInfo.m_fileName, result);
	}
}
