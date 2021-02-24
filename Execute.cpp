
#include <StdFuncs.h>
#include "Commands.h"

/**
 * Launches a command and streams its output to the client.
 * This function launches a command with dedicated stdin, stdout and stderr handles that allow the
 * control and capture of all stdio of the child process that has been launched.  It will capture all
 * output from that child process and stream it back to the client that requested the launch.
 *
 * @date	Monday 15-Feb-2021 7:19 am, Code HQ Bergmannstrasse
 * @param	commandName		The name of the command to be launched
 * @return	KErrNone if the command was launched successfully
 * @return	KErrNotFound if the command executable could not be found
 * @return	KErrGeneral if any other error occurred
 */

int CExecute::launchCommand(char *commandName)
{
	int retVal = KErrGeneral;
	SECURITY_ATTRIBUTES securityAttributes;

	/* Set the bInheritHandle flag so pipe handles are inherited by the child, so that it is able to read */
	/* to and write from them */
	securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttributes.bInheritHandle = TRUE;
	securityAttributes.lpSecurityDescriptor = NULL;

	/* Create pipes that can be used for stdin, stdout and stderr */
	if (CreatePipe(&m_stdOutRead, &m_stdOutWrite, &securityAttributes, 0))
	{
		if (SetHandleInformation(m_stdOutRead, HANDLE_FLAG_INHERIT, 0))
		{
			if (CreatePipe(&m_stdInRead, &m_stdInWrite, &securityAttributes, 0))
			{
				if (SetHandleInformation(m_stdInWrite, HANDLE_FLAG_INHERIT, 0))
				{
					/* Create the child process.  This will read from and write to the pipes we have created, */
					/* and upon exit will close its end of the pipes, so that we can detect that it has exited */
					if ((retVal = createChildProcess(commandName)) == 0)
					{
						char *buffer = new char[STDOUT_BUFFER_SIZE];
						struct SResponse response;
						BOOL success;
						DWORD bytesRead;

						/* Write a successful completion code, to let the client know that it should listen for the */
						/* command output to be streamed */
						response.m_result = retVal;
						SWAP(&response.m_result);
						response.m_size = 0;

						m_socket->write(&response, sizeof(response));

						/* Loop around and read as much from the child's stdout as possible.  When the child exits, */
						/* the pipe will be closed and ReadFile() will fail */
						do
						{
							success = ReadFile(m_stdOutRead, buffer, (STDOUT_BUFFER_SIZE - 1), &bytesRead, NULL);

							if (success)
							{
								/* NULL terminte and print the child's output, and send it to the client for display */
								/* there as well */
								buffer[bytesRead] = '\0';
								printf("%s", buffer);
								m_socket->write(buffer, bytesRead);
							}
						}
						while (success && bytesRead > 0);

						delete [] buffer;
					}
				}

				/* Ensure that stdin related streams are closed.  The read stream may or may not already be closed, */
				/* depending on the success of prior operations */
				CloseHandle(m_stdInWrite);
				m_stdInWrite = nullptr;

				if (m_stdInRead != nullptr)
				{
					CloseHandle(m_stdInRead);
					m_stdInRead = nullptr;
				}
			}
		}

		/* Ensure that stdout related streams are closed.  The write stream may or may not already be closed, */
		/* depending on the success of prior operations */
		CloseHandle(m_stdOutRead);
		m_stdOutRead = nullptr;

		if (m_stdOutWrite != nullptr)
		{
			CloseHandle(m_stdOutWrite);
			m_stdOutWrite = nullptr;
		}
	}

	return retVal;
}

/**
 * Launches a child process.
 * Asynchronously launches a child process that uses the previously created pipes for stdin,
 * stdout and stderr.  When this method returns, the child process will be executing and may
 * be controlled using the given stdio handles.
 *
 * @date	Monday 15-Feb-2021 7:19 am, Code HQ Bergmannstrasse
 * @param	commandName		The name of the command to be launched
 * @return	KErrNone if the command was launched successfully
 * @return	KErrNotFound if the command executable could not be found
 */

int CExecute::createChildProcess(char *commandName)
{
	int retVal = KErrNotFound;
	PROCESS_INFORMATION processInfo;
	STARTUPINFO startupInfo;

	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&startupInfo, sizeof(STARTUPINFO));

	/* Pass the handles for the stdin, stdout and stderr pipes to the client process */
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.hStdError = m_stdOutWrite;
	startupInfo.hStdOutput = m_stdOutWrite;
	startupInfo.hStdInput = m_stdInRead;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;

	/* Create the requested child process */
	if (CreateProcess(NULL, commandName, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo))
	{
		retVal = KErrNone;

		/* Close handles to the child process and its primary thread, which we don't need */
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		/* Close handles to the stdin and stdout pipes no longer needed by the child process.  If they */
		/* are not explicitly closed, there is no way to recognise that the child process has ended. */
		CloseHandle(m_stdOutWrite);
		m_stdOutWrite = NULL;
		CloseHandle(m_stdInRead);
		m_stdInRead = NULL;
	}

	return retVal;
}
