
#include <StdFuncs.h>
#include <File.h>
#include <FileUtils.h>
#include <StdSocket.h>
#include <Yggdrasil/Commands.h>
#include <string.h>

#ifdef __amigaos__

#include <sys/wait.h>

#define WRITE_INT(Dest, Value) \
	*Dest = Value & 0xff; \
	*(Dest + 1) = (Value >> 8) & 0xff; \
	*(Dest + 2) = (Value >> 16) & 0xff; \
	*(Dest + 3) = (Value >> 24) & 0xff

#else /* ! __amigaos__ */

#define WRITE_INT(Dest, Value) \
	*Dest = (Value >> 24) & 0xff; \
	*(Dest + 1) = (Value >> 16) & 0xff; \
	*(Dest + 2) = (Value >> 8) & 0xff; \
	*(Dest + 3) = Value & 0xff

#endif /* ! __amigaos__ */

/**
 * Deletes a file.
 * Deletes the file specified by the client.
 *
 * @date	Wednesday 08-Mar-2023 7:47 am, Code HQ Tokyo Tsukuda
 */

void CDelete::execute()
{
	readPayload();

	/* Extract the filename from the payload */
	char *fileName = reinterpret_cast<char *>(m_payload);

	printf("delete: Deleting file \"%s\"\n", fileName);

	RFileUtils fileUtils;
	SResponse response;

	/* Delete the file and return the result to the client */
	response.m_result = fileUtils.deleteFile(fileName);
	SWAP(&response.m_result);

	response.m_size = 0;

	m_socket->write(&response, sizeof(response));
}

/**
 * Sends a directory listing to the remote client.
 * Lists the contents of the requested directory and returns the list of files to the client.
 *
 * @date	Wednesday 11-Jan-2023 6:52 am, Code HQ Tokyo Tsukuda
 */

void CDir::execute()
{
	readPayload();

	/* Extract the filename from the payload */
	m_directoryName = reinterpret_cast<char *>(m_payload);

	printf("dir: Listing contents of directory \"%s\"\n", m_directoryName);

	int result;
	RDir dir;
	SResponse response;
	TEntryArray *entries;

	/* Scan the specified directory and build a list of files that it contains */
	if ((result = dir.open(m_directoryName)) == KErrNone)
	{
		if ((result = dir.read(entries, EDirSortNameAscending)) == KErrNone)
		{
			char *payload;
			size_t offset = 0;
			uint32_t payloadSize = 1;

			/* Iterate through the list of files and determine the amount of memory required to store the filenames */
			/* and file metadata */
			const TEntry *entry = entries->getHead();

			while (entry != nullptr)
			{
				payloadSize += static_cast<uint32_t>(strlen(entry->iName) + 1 + sizeof(TEntry::iSize));
				entry = entries->getSucc(entry);
			}

			/* Allocate a buffer large enough to hold the response payload and fill it with the file information */
			payload = new char[payloadSize];

			entry = entries->getHead();

			while (entry != nullptr)
			{
				memcpy(payload + offset, entry->iName, strlen(entry->iName) + 1);
				offset += strlen(entry->iName) + 1;
				WRITE_INT((payload + offset), entry->iSize);
				offset += sizeof(entry->iSize);

				entry = entries->getSucc(entry);
			}

			*(payload + offset) = '\0';

			response.m_result = result;
			SWAP(&response.m_result);

			response.m_size = payloadSize;
			SWAP(&response.m_size);

			m_socket->write(&response, sizeof(response));
			m_socket->write(payload, payloadSize);

			delete [] payload;
		}

		dir.close();
	}

	/* If the directory could not be read, just send a response with an empty payload */
	if (result != KErrNone)
	{
		response.m_result = result;
		response.m_size = 0;
		SWAP(&response.m_result);

		m_socket->write(&response, sizeof(response));
	}
}

/**
 * Executes a file.
 * Runs the executable or script file specified by the client, displaying its stdout and stderr output
 * on the console.
 *
 * @date	Wednesday 29-Jan-2020 2:24 pm, Scoot flight TR 735 to Singapore
 */

void CExecute::execute()
{
	readPayload();

	printf("execute: Executing command \"%s\"\n", m_payload);

	SResponse response;
	response.m_size = 0;
	response.m_result = launchCommand(reinterpret_cast<char *>(m_payload));

	/* Write a failure completion code, to let the client know that it should not listen for the */
	/* command output to be streamed */
	if (response.m_result != KErrNone)
	{
		SWAP(&response.m_result);
		m_socket->write(&response, sizeof(response));
	}

	/* If the client was launched successfully then send two NULL terminators in a row.  This is the */
	/* signal to the client that the stdout output stream has ended */
	if (response.m_result == KErrNone)
	{
		char terminators[2] = { 0, 0 };
		m_socket->write(terminators, sizeof(terminators));
	}
	else if ((response.m_result == -1) || (response.m_result == 127))
	{
		printf("execute: Unable to launch command\n");
	}
	else if (response.m_result != 0)
	{
		/* It's a bit crazy but launching behaves differently under Windows, so we have to take this */
		/* into account */

#ifdef WIN32

		printf("execute: Command failed, return code = %d\n", response.m_result);

#else /* WIN32 */

		printf("execute: Command failed, return code = %d\n", WEXITSTATUS(response.m_result));

#endif /* WIN32 */

	}
}

/**
 * Determines detailed file information and returns it to the client.
 * Sends information about a filesystem object to the client, allowing it to determine such things
 * as the size of the object, whether it is a file or a directory, its size etc.
 *
 * @date	Thursday 23-Feb-2023 10:04 am, Code HQ Tokyo Tsukuda
 */

void CFileInfo::execute()
{
	readPayload();

	/* Extract the filename from the payload */
	m_fileName = reinterpret_cast<char *>(m_payload);

	printf("fileinfo: Querying information about file \"%s\"\n", m_payload);

	int result;
	SFileInfo *fileInfo;
	SResponse response;
	TEntry entry;

	/* Determine if the file exists and send the result to the remote client */
	result = response.m_result = getFileInformation(m_fileName, fileInfo);
	SWAP(&response.m_result);

	/* If the file exists then send a response and a payload, containing the file's timestamp */
	if (result == KErrNone)
	{
		/* Include the size of the filename in the payload size */
		int32_t payloadSize = static_cast<int32_t>(sizeof(SFileInfo) + strlen(entry.iName) + 1);

		/* And send the response and its payload */
		response.m_size = payloadSize;
		SWAP(&response.m_size);

		m_socket->write(&response, sizeof(response));
		m_socket->write(fileInfo, payloadSize);

		delete[] reinterpret_cast<unsigned char *>(fileInfo);
	}
	/* Otherwise just send a response with an empty payload */
	else
	{
		response.m_size = 0;
		m_socket->write(&response, sizeof(response));
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

	int result;
	SResponse response;
	TEntry entry;

	/* Determine if the file exists and send the result to the remote client */
	result = response.m_result = Utils::GetFileInfo(m_fileName, &entry);
	SWAP(&response.m_result);

	/* If the file exists then send a response and a payload, containing the file's timestamp */
	if (result == KErrNone)
	{
		/* Strip any path component from the file as we want it to be written to the current directory */
		/* in the destination */
		const char *fileName = Utils::filePart(m_fileName);

		/* Include the size of just the filename in the payload size */
		int32_t payloadSize = static_cast<int32_t>(sizeof(SFileInfo) + strlen(fileName) + 1);

		/* Allocate an SFileInfo structure of a size large enough to hold the file's name */
		SFileInfo *fileInfo = reinterpret_cast<SFileInfo *>(new unsigned char [payloadSize]);

		/* Initialise it with the file's name and timestamp */
		fileInfo->m_microseconds = entry.iModified.Int64();
		SWAP64(&fileInfo->m_microseconds);
		strcpy(fileInfo->m_fileName, fileName);

		/* And finally send the response and its payload */
		response.m_size = payloadSize;
		SWAP(&response.m_size);

		m_socket->write(&response, sizeof(response));
		m_socket->write(fileInfo, payloadSize);

		delete [] reinterpret_cast<unsigned char *>(fileInfo);
	}
	/* Otherwise display an error and just send a response with an empty payload */
	else
	{
		Utils::Error("fileinfo: Unable to query information about file \"%s\"", m_fileName);

		response.m_size = 0;
		m_socket->write(&response, sizeof(response));
	}

	/* If the file exists then the remote client will be awaiting its transfer, so send it now */
	if (response.m_result  == KErrNone)
	{
		sendFile(m_fileName);
	}
}

/**
 * Renames a file.
 * Renames the file specified by the client.
 *
 * @date	Saturday 11-Mar-2023 6:47 am, Code HQ Tokyo Tsukuda
 */

void CRename::execute()
{
	readPayload();

	/* Extract the old and new filenames from the payload */
	char *oldName = reinterpret_cast<char *>(m_payload);
	char *newName = reinterpret_cast<char *>(m_payload + strlen(oldName) + 1);

	printf("rename: Renaming file \"%s\" to \"%s\"\n", oldName, newName);

	RFileUtils fileUtils;
	SResponse response;

	/* Rename the file and return the result to the client */
	response.m_result = fileUtils.renameFile(oldName, newName);
	SWAP(&response.m_result);

	response.m_size = 0;

	m_socket->write(&response, sizeof(response));
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
	readPayload();

	/* Extract the file's information from the payload */
	SFileInfo *fileInfo = reinterpret_cast<SFileInfo *>(m_payload);
	SWAP64(&fileInfo->m_microseconds);
	m_fileName = fileInfo->m_fileName;

	/* Transfer the file from the remote client */
	if (readFile(m_fileName) == KErrNone)
	{
		/* And set its datestamp and protection bits */
		setFileInformation(*fileInfo);
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
	readPayload();

	uint32_t serverVersion = ((PROTOCOL_MAJOR << 16) | PROTOCOL_MINOR);
	SWAP(&serverVersion);
	m_socket->write(&serverVersion, sizeof(serverVersion));
}
