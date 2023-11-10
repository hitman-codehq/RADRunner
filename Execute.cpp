
#include <StdFuncs.h>
#include <Yggdrasil/Commands.h>

#ifdef __amigaos__

#include <dos/dostags.h>

#elif defined(__unix__)

#include <unistd.h>

#endif /* __unix__ */

/**
 * Amiga OS client exit callback function.
 * This function is called in the context of a child process that has been launched by the
 * CreateNewProcTags() function when that child process is terminated.
 *
 * @date	Friday 10-Nov-2023 6:00 am, Code HQ Tokyo Tsukuda
 * @param	a_returnCode	The return code of the child process
 * @param	a_exitData		Pointer to an integer into which to write the client's exit code
 */

#ifdef __amigaos__

#ifdef __amigaos4__

void ExitFunction(int32_t a_returnCode, int32_t *a_exitData)
{
	*a_exitData = a_returnCode;
}

#else /* ! __amigaos4__ */

void ExitFunction(int32_t a_returnCode __asm("d0"), int32_t *a_exitData __asm("d1"))
{
	*a_exitData = a_returnCode;
}

#endif /* ! __amigaos4__ */

#endif /* __amigaos__ */

/**
 * Launches a command and streams its output to the client.
 * This function launches a command with dedicated stdin, stdout and stderr handles that allow the
 * control and capture of all stdio of the child process that has been launched.  It will capture all
 * output from that child process and stream it back to the client that requested the launch.
 *
 * @date	Monday 15-Feb-2021 7:19 am, Code HQ Bergmannstrasse
 * @param	a_commandName	The name of the command to be launched
 * @return	KErrNone if the command was launched successfully
 * @return	KErrNotFound if the command executable could not be found
 * @return	KErrGeneral if any other error occurred
 */

TResult CExecute::launchCommand(char *a_commandName)
{
	TResult retVal{ KErrGeneral, 0 };

#ifdef __amigaos__

	int32_t exitCode = 0;
	BPTR stdInRead = Open("Console:", MODE_OLDFILE);
	BPTR stdOutWrite = Open("PIPE:RADRunner", MODE_NEWFILE);
	BPTR stdOutRead = Open("PIPE:RADRunner", MODE_OLDFILE);

	if ((stdInRead != 0) && (stdOutRead != 0) && (stdOutWrite != 0))
	{
		/* Load the command executable into memory and use the returned segment list to create a new process */
		/* with CreateNewProcTags().  We do it like this because this is the only way to capture the child's */
		/* exit code.  Using SystemTags() results in the shell's exit code being returned, not the child's */
		BPTR segList = LoadSeg(a_commandName);

		if (segList != 0)
		{
			struct Process *process = CreateNewProcTags(NP_Seglist, (ULONG) segList, NP_Input, stdInRead,
				NP_Output, stdOutWrite, NP_ExitCode, (ULONG) ExitFunction, NP_ExitData, (ULONG) &exitCode,
				NP_Cli, TRUE, TAG_DONE);

			if (process != NULL)
			{
				char *buffer = new char[STDOUT_BUFFER_SIZE];
				LONG bytesRead;

				retVal.m_result = KErrNone;

				/* Loop around and read as much from the child's stdout as possible.  When the child exits, */
				/* the pipe will be closed and Read() will fail */
				do
				{
					if ((bytesRead = Read(stdOutRead, buffer, (STDOUT_BUFFER_SIZE - 1))) > 0)
					{
						/* NULL terminate and print the child's output, and send it to the client for display */
						/* there as well */
						buffer[bytesRead] = '\0';
						printf("%s", buffer);
						m_socket->write(buffer, bytesRead);
					}
				}
				while (bytesRead > 0);

				delete [] buffer;
				stdInRead = stdOutWrite = 0;

				retVal.m_subResult = exitCode;
			}
		}
		else
		{
			retVal.m_result = Utils::MapLastError();
		}
	}

	if (stdInRead != 0) { Close(stdInRead); }
	if (stdOutWrite != 0) { Close(stdOutWrite); }
	if (stdOutRead != 0) { Close(stdOutRead); }

#elif defined(__unix__)

	// On UNIX systems, there is no way to programmatically capture stderr, but we can redirect it
	// to stdout as a part of command invocation
	std::string command = std::string(a_commandName) + " 2>&1";

	FILE *pipe = popen(command.c_str(), "r");

	if (pipe != nullptr)
	{
		char *buffer = new char[STDOUT_BUFFER_SIZE];
		int exitCode;
		size_t bytesRead;

		retVal.m_result = KErrNone;

		/* Loop around and read as much from the child's stdout as possible.  When the child exits, */
		/* the pipe will be closed and fread() will fail */
		do
		{
			if ((bytesRead = fread(buffer, 1, (STDOUT_BUFFER_SIZE - 1), pipe)) > 0)
			{
				/* NULL terminate and print the child's output, and send it to the client for display */
				/* there as well */
				buffer[bytesRead] = '\0';
				printf("%s", buffer);
				m_socket->write(buffer, bytesRead);
			}
		}
		while (bytesRead > 0);

		delete [] buffer;

		exitCode = pclose(pipe);

		/* If the client has exited with a non-zero exit code, convert it to one of The Framework's error codes. */
		/* The error codes 126 and 127 are not in any header files, but are well-known shell error codes on Unix */
		if (WIFEXITED(exitCode))
		{
			exitCode = (WEXITSTATUS(exitCode));

			/* Exit code 126 means that the file is not in an executable format */
			if (exitCode == 126)
			{
				retVal.m_result = KErrCorrupt;
			}
			/* Exit code 127 means that the file was not found */
			else if (exitCode == 127)
			{
				retVal.m_result = KErrNotFound;
			}
			else
			{
				retVal.m_subResult = exitCode;
			}
		}
	}

#else /* ! __unix__ */

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
					HANDLE childProcess;

					/* Create the child process.  This will read from and write to the pipes we have created, */
					/* and upon exit will close its end of the pipes, so that we can detect that it has exited */
					if ((retVal.m_result = createChildProcess(a_commandName, childProcess)) == KErrNone)
					{
						char *buffer = new char[STDOUT_BUFFER_SIZE];
						BOOL success;
						DWORD bytesRead, exitCode;

						/* Loop around and read as much from the child's stdout as possible.  When the child exits, */
						/* the pipe will be closed and ReadFile() will fail */
						do
						{
							success = ReadFile(m_stdOutRead, buffer, (STDOUT_BUFFER_SIZE - 1), &bytesRead, NULL);

							if (success)
							{
								/* NULL terminate and print the child's output, and send it to the client for display */
								/* there as well */
								buffer[bytesRead] = '\0';
								printf("%s", buffer);
								m_socket->write(buffer, bytesRead);
							}
						}
						while (success && bytesRead > 0);

						delete [] buffer;

						if (GetExitCodeProcess(childProcess, &exitCode))
						{
							retVal.m_subResult = exitCode;
						}

						/* And finally, close the handle to the child process now that we have its return code */
						CloseHandle(childProcess);
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

#endif /* ! __unix__ */

	return retVal;
}

/**
 * Launches a child process.
 * Asynchronously launches a child process that uses the previously created pipes for stdin,
 * stdout and stderr.  When this method returns, the child process will be executing and may
 * be controlled using the given stdio handles.
 *
 * @date	Monday 15-Feb-2021 7:19 am, Code HQ Bergmannstrasse
 * @param	a_commandName	The name of the command to be launched
 * @return	KErrNone if the command was launched successfully
 * @return	KErrNotFound if the command executable could not be found
 */

#ifdef WIN32

int CExecute::createChildProcess(char *a_commandName, HANDLE &a_childProcess)
{
	int retVal;
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
	if (CreateProcess(NULL, a_commandName, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo))
	{
		retVal = KErrNone;
		a_childProcess = processInfo.hProcess;

		/* Close the handle to the primary thread, which we don't need */
		CloseHandle(processInfo.hThread);

		/* Close handles to the stdin and stdout pipes no longer needed by the child process.  If they */
		/* are not explicitly closed, there is no way to recognise that the child process has ended. */
		CloseHandle(m_stdOutWrite);
		m_stdOutWrite = NULL;
		CloseHandle(m_stdInRead);
		m_stdInRead = NULL;
	}
	else
	{
		retVal = Utils::MapLastError();
	}

	return retVal;
}

#endif /* WIN32 */
