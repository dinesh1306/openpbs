/*
 * Copyright (C) 1994-2018 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of the PBS Professional("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * PBS Pro is free software. You can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * For a copy of the commercial license terms and conditions,
 * go to: (http://www.pbspro.com/UserArea/agreement.html)
 * or contact the Altair Legal Department.
 *
 * Altair’s dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of PBS Pro and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair’s trademarks, including but not limited to "PBS™",
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
 * trademark licensing policies.
 *
 */
/**
 * @file    win_remote_shell.c
 *
 * @brief
 *  Handles Windows remote shell commands
 *
 */
#include <pbs_config.h>   /* the master config generated by configure */
#include <pbs_version.h>
#include "win.h"
#include "win_remote_shell.h"


/**
 * @brief
 *	Handle console output.
 *
 * @param[in] buf - buffer to be sent to stdout.
 *
 * @return  void
 *
 */
void
std_output(char *buf)
{
        if(buf == NULL)
                return;
	fprintf(stdout, "%s", buf);
	fflush(stdout);
}
/**
 * @brief
 *	Handle console error.
 *
 * @param[in] buf - buffer to be sent to stderr.
 *
 * @return  void
 *
 */
void
std_error(char *buf)
{
        if(buf == NULL)
                return;
	fprintf(stderr, "%s", buf);
	fflush(stderr);
}
/**
 * @brief
 *	Disconnect the named pipe and close it's handle.
 *
 * @param[in] hpipe - handle to the named pipe.
 *
 * @return  void
 *
 */
void
disconnect_close_pipe(HANDLE hpipe)
{
	if (hpipe != INVALID_HANDLE_VALUE) {
		DisconnectNamedPipe(hpipe);
		CloseHandle(hpipe);
	}
}
/**
 * @brief
 *	Create pipes at local host to redirect a process's standard input, output and error.
 *  If pipe creation fails for any of the required standard pipes, return error.
 *
 * @param[in/out]	psi - Pointer to STARTUPINFO corresponding to the process
 *			      whose standard input, output and error handles need to be redirected.
 * @param[in] pipename_append - Appendix to the pipename.
 * @param[in] is_interactive - Should process's stdin be redirected? redirect stdin if non-zero.
 *
 * @return  int
 * @retval	0 - no error
 * @retval	!0 - error number, describing error while trying to create std pipes
 *
 */
int
create_std_pipes(STARTUPINFO* psi, char *pipename_append, int is_interactive)
{
	char stdoutpipe[PIPENAME_MAX_LENGTH] = "";
	char stderrpipe[PIPENAME_MAX_LENGTH] = "";
	char stdinpipe[PIPENAME_MAX_LENGTH] = "";
	SECURITY_ATTRIBUTES SecAttrib = {0};
	SECURITY_DESCRIPTOR SecDesc;
	int err = 0;

	if (psi == NULL || pipename_append == NULL)
		return -1;

	/* Assign NULL DACL to the scurity descriptor, allowing all access to the pipe */
	if (InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION) == 0) {
		err = GetLastError();
		return err;
	}
	if (SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, FALSE) == 0) {
		err = GetLastError();
		return err;
	}
	SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
	SecAttrib.lpSecurityDescriptor = &SecDesc;;
	SecAttrib.bInheritHandle = TRUE;

	/* Use Process's standard input/output/error handles */
	psi->dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	psi->hStdOutput = INVALID_HANDLE_VALUE;
	psi->hStdInput = INVALID_HANDLE_VALUE;
	psi->hStdError = INVALID_HANDLE_VALUE;

	/* stdout pipe name */
	snprintf(stdoutpipe,
		PIPENAME_MAX_LENGTH - 1,
		"\\\\.\\pipe\\%s%s",
		INTERACT_STDOUT,
		pipename_append);
	/* Create stdout pipe.
	 * This pipe needs an outbound access e.g. this end of pipe can
	 * "only write"(GENERIC_WRITE) and client on the other end can "only
	 * read"(GENERIC_READ).
	 * Enable blocking mode using PIPE_WAIT, so that any subsequent ConnectNamedPipe()
	 * call, waits indefinitely for a client to connect to this pipe.
	 */
	psi->hStdOutput = CreateNamedPipe(
		stdoutpipe,
		PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		0,
		0,
		(DWORD)-1,
		&SecAttrib);
	if (psi->hStdOutput == INVALID_HANDLE_VALUE) {
		err = GetLastError();
		return err;
	}

	/* stderr pipe name */
	snprintf(stderrpipe,
		PIPENAME_MAX_LENGTH - 1,
		"\\\\.\\pipe\\%s%s",
		INTERACT_STDERR,
		pipename_append);
	/* Create stderr pipe.
	 * This pipe needs an outbound access e.g. this end of pipe can
	 * "only write"(GENERIC_WRITE) and client on the other end can "only
	 * read"(GENERIC_READ).
	 * Enable blocking mode using PIPE_WAIT, so that any subsequent ConnectNamedPipe()
	 * call, waits indefinitely for a client to connect to this pipe.
	 */
	psi->hStdError = CreateNamedPipe(
		stderrpipe,
		PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		0,
		0,
		(DWORD)-1,
		&SecAttrib);
	if (psi->hStdError == INVALID_HANDLE_VALUE) {
		err = GetLastError();
		/* Close already opened output handle upon error */
		CloseHandle(psi->hStdOutput);
		return err;
	}

	/* Create stdin pipe if it is an interactive process */
	if (is_interactive) {
		/* stdin pipe name */
		snprintf(stdinpipe,
			PIPENAME_MAX_LENGTH - 1,
			"\\\\.\\pipe\\%s%s",
			INTERACT_STDIN,
			pipename_append);
		/* Create stdin pipe.
		 * This pipe needs an inbound access e.g. this end of pipe can
		 * "only read"(GENERIC_READ) and client on the other end can "only
		 * write"(GENERIC_WRITE).
		 * Enable blocking mode using PIPE_WAIT, so that any subsequent ConnectNamedPipe()
		 * call, waits indefinitely for a client to connect to this pipe.
		 */
		psi->hStdInput = CreateNamedPipe(
			stdinpipe,
			PIPE_ACCESS_INBOUND,
			PIPE_TYPE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			0,
			0,
			(DWORD)-1,
			&SecAttrib);
		if (psi->hStdInput == INVALID_HANDLE_VALUE) {
			err = GetLastError();
			/* Close already opened output,error handles upon error */
			CloseHandle(psi->hStdOutput);
			CloseHandle(psi->hStdError);
			return err;
		}
	}
	return 0;
}
/**
 * @brief
 *	Connect to the named pipe, avoiding connection race that
 *  can occur if the named pipe client connects between CreateNamedPipe() and ConnectNamedPipe()
 *  calls. ConnectNamedPipe() in this case fails with error ERROR_PIPE_CONNECTED.
 *  We need to ignore this error at named pipe server after calling ConnectNamedPipe().
 *
 *
 * @param[in]	hPipe - Handle to the named pipe.
 * @param[in]   pOverlapped - A pointer to an OVERLAPPED structure.
 *
 * @return  int
 * @retval	0 - success
 * @retval	!0 - error number, while trying to connect named pipe
 *
 */
int
do_ConnectNamedPipe(HANDLE hPipe, LPOVERLAPPED pOverlapped)
{
	int err = 0;
	/* A client can connect between CreateNamedPipe and ConnectNamedPipe
	 * calls, in this case ConnectNamedPipe() call will return failure(0) with
	 * error ERROR_PIPE_CONNECTED, ignore this error.
	 */
	if (ConnectNamedPipe(hPipe, pOverlapped) == 0) {
		err = GetLastError();
		if (err != ERROR_PIPE_CONNECTED)
			return err;
	}
	return 0;
}
/**
 * @brief
 *	This function tries to wait for named pipe to be available at named pipe server, making sure that
 *  the named pipe exists and is available.
 *  Connect to the named pipe, avoiding connection race that can occur if named pipe
 *  client connects between CreateNamedPipe() and ConnectNamedPipe()
 *  calls i.e. WaitNamedPipe() at client gets called before ConnectNamedPipe() call at server. WaitNamedPipe()
 *  in such case returns succesfully but subsequent CreateFile() call fails with ERROR_PIPE_NOT_CONNECTED.
 *  Retry CreateFile() multiple times untill the named pipe gets connected at named pipe server or untill the max retry.
 *
 *  Also, if the Pipe is not yet created at server, the call to WaitNamedPipe() at client fails with error
 *  ERROR_FILE_NOT_FOUND. We need to retry WaitNamedPipe() multiple times at the client to make sure that the
 *  named pipe is created at named server.
 *
 * @param[in]	pipename - Name of the named pipe.
 * @param[in]   timeout - timeout for wait
 * @param[in]   readwrite_accessflags - read/write access flags for the named pipe
 *
 * @return  HANDLE
 * @retval	a valid handle to the pipe - success
 * @retval	INVALID_HANDLE_VALUE - Failed to obtain a valid handle to the named pipe.
 *
 */
HANDLE
do_WaitNamedPipe(char *pipename, DWORD timeout, DWORD readwrite_accessflags)
{
	HANDLE hpipe_cmdshell = INVALID_HANDLE_VALUE;
	int err = 0;
	int i = 0;
	int j = 0;
	int retry = 10;
	int retry2 = 10;

	SECURITY_ATTRIBUTES SecAttrib = {0};
	SECURITY_DESCRIPTOR SecDesc;

	if (pipename == NULL)
		return INVALID_HANDLE_VALUE;

	if (InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION) == 0)
                return INVALID_HANDLE_VALUE;

	if (SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, TRUE) == 0)
                return INVALID_HANDLE_VALUE;

	SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
	SecAttrib.lpSecurityDescriptor = &SecDesc;;
	SecAttrib.bInheritHandle = TRUE;

	while (i++ < retry) {
		/* Connect to the remote process pipe */
		if (WaitNamedPipe(pipename, timeout)) {
			while (j < retry2) {
				hpipe_cmdshell = CreateFile(
					pipename,
					readwrite_accessflags,
					0,
					&SecAttrib,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
					NULL);
				err = GetLastError();
				if ((ERROR_PIPE_NOT_CONNECTED) == err) {
					j++;
					Sleep(1000);
					continue;
				}
				else
					break;
			}
			break;
		}
		/* Wait untill the pipe gets available */
		else if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			Sleep(1000);
			continue;
		}
	}
	return hpipe_cmdshell;
}
/**
 * @brief
 *	Connect to pipes that redirect a process's stdin, stdout, stderr.
 *
 *
 * @param[in]	psi - Pointer to STARTUPINFO corresponding to the process
 *		      whose standard input, output and error handles are redirected to named pipes.
 * @param[in]   is_interactive - Is process's stdin being redirected? Connect to stdin pipe, if non-zero.
 *
 * @return  	int
 * @retval	0 - success
 * @retval	!0 && > 0 - error number, while trying to connect std pipes
 * @retval	-1 - invalid arguments
 *
 */
int
connectstdpipes(STARTUPINFO* psi, int is_interactive)
{
	int err = 0;

	if (psi == NULL)
		return -1;

	/* Waiting for client to connect to stdout pipe */
	if ((err = do_ConnectNamedPipe(psi->hStdOutput, NULL)) != 0) {
		return err;
	}
	if (is_interactive) {
		/* Waiting for client to connect to stdin pipe */
		if ((err = do_ConnectNamedPipe(psi->hStdInput, NULL)) != 0) {
			return err;
		}
	}
	/* Waiting for client to connect to stderr pipe */
	if ((err = do_ConnectNamedPipe(psi->hStdError, NULL)) != 0) {
		return err;
	}
	return 0;
}

/**
 * @brief
 *	Execute the command indicated by "command".
 *  Wait for the command process to exit.
 *
 * @param[in]	psi - Pointer to STARTUPINFO corresponding to the process
 * @param[in]	command - the command to be executed
 * @param[out]  p_returncode - the exit code of the command process
 * @param[in]	is_gui_job - is this a GUI command needed to be launched in active user session?
 * @param[in]	show_window - whether to show or hide window?
 * @param[in]	username - Run GUI command as this user, should be NULL for non-GUI commands.
 *
 * @return      int
 * @retval	0 - success
 * @retval	!0 && > 0 - error number, while trying to run the command process
 * @retval	-1 - invalid function arguments
 *
 */
DWORD
run_command_si_blocking(STARTUPINFO *psi, char *command, DWORD *p_returncode, int is_gui_command , int show_window, char *username)
{
	PROCESS_INFORMATION pi;	
	HANDLE	hJob = INVALID_HANDLE_VALUE;

	if (command == NULL || p_returncode == NULL || psi == NULL)
		return -1;

	*p_returncode = 0;
	psi->wShowWindow = show_window;
	
	if(is_gui_command) {
		/* Launch GUI application in active user session */
		HANDLE hUserToken = INVALID_HANDLE_VALUE;
		static int activesessionid = 0;
		static char * active_user = NULL;
		
		/* Launch the process on interactive desktop */
		psi->lpDesktop = "winsta0\\default";
		ZeroMemory(&pi, sizeof(pi));
		
		/* Get current active user session id */
		activesessionid = get_activesessionid(0, username);
		if (activesessionid == -1) {
			/* No active session so no need to start new process */
			return 0;
		}

		active_user = get_usernamefromsessionid(activesessionid, NULL);
		if (active_user == NULL) {
			/* No active user so no need to start new process */
			return 0;
		}

		/* Get current active user's token */
		hUserToken = get_activeusertoken(activesessionid);
		if (hUserToken == INVALID_HANDLE_VALUE) {
			return 1;
		}
		/* 
		** GUI jobs are launched in different user session and thus can not be part of the same job object.
		** Create a new job object for the new session.
		*/

		hJob = CreateJobObject(NULL, NULL);
		/* Run the given command in active user's session using active user's token (hUserToken)*/
		if (CreateProcessAsUser(hUserToken, NULL,
			command,
			NULL,
			NULL,
			TRUE,
			CREATE_NEW_CONSOLE,
			NULL,
			NULL,
			psi,
			&pi)) {
				AssignProcessToJobObject(hJob, pi.hProcess);
				/* Wait for command process to exit */
				WaitForSingleObject(pi.hProcess, INFINITE);
				GetExitCodeProcess(pi.hProcess, p_returncode);
				/* Terminate all processes associated with the job object */
				TerminateJobObject(hJob, 0);
				close_valid_handle(&(pi.hProcess));
				close_valid_handle(&(pi.hThread));
		}
		else
			return (GetLastError());
		CloseHandle(hUserToken);
	}
	else {
		/* Run the command process */
		if (CreateProcess(
		NULL,
		command,
		NULL,
		NULL,
		TRUE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		psi,
		&pi)) {
			AssignProcessToJobObject(hJob, pi.hProcess);
			/* Wait for command process to exit */
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hProcess, p_returncode);
			/* Terminate all processes associated with the job object */
			TerminateJobObject(hJob, 0);
			close_valid_handle(&(pi.hProcess));
			close_valid_handle(&(pi.hThread));
		}
		else
			return (GetLastError());
	}
	return 0;
}

/**
 * @brief
 *	Connect or disconnect resource at remote host.
 *
 * @param[in]	remote_host - name of the remote host
 * @param[in]	remote_resourcename - name of the remote resource
 * @param[in]	bEstablish - connect or disconnect. connect if true, disconnect otherwise
 *
 * @return      BOOL
 * @retval	TRUE - success
 * @retval	FALSE - failure
 *
 */
BOOL
connect_remote_resource(const char *remote_host, const char *remote_resourcename, BOOL bEstablish)
{
	char remote_resource_path[PIPENAME_MAX_LENGTH] = {0};
	DWORD rc = 0;
	/* Prepare remote resource name e.g. \\<remote_host>\<remote_resourcename> */
	sprintf(remote_resource_path, "\\\\%s\\%s", remote_host, remote_resourcename);
	/* Disconnect or connect to the resource, based on bEstablish */
	if (bEstablish) {
		NETRESOURCE nr;
		nr.dwType = RESOURCETYPE_ANY;
		nr.lpLocalName = NULL;
		nr.lpRemoteName = (LPTSTR)&remote_resource_path;
		nr.lpProvider = NULL;
		/* Establish connection to remote resource without username/pwd */
		rc = WNetAddConnection2(&nr, NULL, NULL, FALSE);
		if (rc == NO_ERROR || rc == ERROR_ALREADY_ASSIGNED)
			return TRUE;
	}
	else {
		rc = WNetCancelConnection2(remote_resource_path, 0, TRUE);/* Disconnect resource */
		if(rc == NO_ERROR || rc == ERROR_NOT_CONNECTED)
			return TRUE;
	}	

	SetLastError(rc);
	return FALSE;
}
/**
 * @brief
 *	Generic function for handling the remote stdout/stderr via pipe.
 *
 * @param[in]	hPipe_remote_std - a HANDLE to pipe handling stdout/stderr.
 * @param[in]	oe_handler - pointer to a function that handles stdout/stderr.
 *
 * @return      int
 * @retval      -1, no data or broken pipe
 * @retval      0, successfully read
 *
 */
int
handle_stdoe_pipe(HANDLE hPipe_remote_std, void (*oe_handler)(char*))
{
	char readbuf[READBUF_SIZE] = {0};
	DWORD dwRead = 0;
	DWORD dwAvail = 0;
	DWORD dwErr = 0;
	DWORD dw_rc = 0;

	if (hPipe_remote_std == INVALID_HANDLE_VALUE)
		return -1;

	strncpy(readbuf, "", READBUF_SIZE - 1);
	dwRead = 0;
	/*
	 * Peek into the named pipe and see if there is any data to read, if yes, read it.
	 * ReadFile() should never block as we call it only when we know for certain that
	 * there is data to be read from pipe.
	 */
	if ((dw_rc = PeekNamedPipe(hPipe_remote_std, NULL, 0, NULL, &dwAvail, NULL)) && dwAvail > 0) {
		if (!ReadFile(hPipe_remote_std, readbuf, READBUF_SIZE, &dwRead, NULL) || dwRead == 0) {
			dwErr = GetLastError();
			if (dwErr == ERROR_NO_DATA) {
				return -1;
			}
		}
		if (dwRead != 0) {
			readbuf[ dwRead / sizeof(char) ] = '\0';
			/* Send it to our stdoe handler */
			oe_handler(readbuf);
		}
		/* When ReadFile returns with broken pipe, valid data may still be returned so break only after handling any data */
		if (dwErr == ERROR_BROKEN_PIPE || dwErr == ERROR_PIPE_NOT_CONNECTED) {
			return -1;
		}
	}
	else if (dw_rc == 0) { /* PeekNamedPipe() fails */
		dwErr = GetLastError();
		if (dwErr == ERROR_NO_DATA || dwErr == ERROR_BROKEN_PIPE || dwErr == ERROR_PIPE_NOT_CONNECTED) {
			return -1;
		}
	}
	return 0;
}
/**
 * @brief
 *	Function for listening the remote stdout/stderr pipe. Remote process will send it's stdout/stderr to these pipes.
 *
 * @param[in]	hpipe_remote_stdout - a HANDLE to pipe handling stdout.
 * @param[in]	hpipe_remote_stderr - a HANDLE to pipe handling stderr.
 *
 * @return  void
 *
 */
void
listen_remote_stdouterr_pipes(HANDLE hpipe_remote_stdout, HANDLE hpipe_remote_stderr)
{
	if (hpipe_remote_stdout == INVALID_HANDLE_VALUE || hpipe_remote_stderr == INVALID_HANDLE_VALUE)
		return;

	for (;;) {
		if (handle_stdoe_pipe(hpipe_remote_stderr, std_error) == -1)
			break;
		if (handle_stdoe_pipe(hpipe_remote_stdout, std_output) == -1)
			break;
	}
}

/**
 * @brief
 *	Thread function for listening to console If the user types in something, this function
 *  will pass it to the remote host's command shell. ReadConsole() returns after pressing the ENTER key.
 *
 * @param[in]	p - Thread argument. Expected to contain pointer to a HANDLE to remote stdin pipe.
 *
 * @return  void
 *
 */
void
listen_remote_stdinpipe_thread(void *p)
{
	HANDLE hconsole_input = INVALID_HANDLE_VALUE;
	char inputbuf[CONSOLE_BUFSIZE] = {0};
	DWORD nBytesRead = 0;
	DWORD nBytesWrote = 0;
	HANDLE hpipe_remote_stdin = INVALID_HANDLE_VALUE;
	int rc = 0;

	hconsole_input = GetStdHandle(STD_INPUT_HANDLE);
	if (hconsole_input == INVALID_HANDLE_VALUE || p == NULL)
		return;
	hpipe_remote_stdin = *((HANDLE*)p);

	for (;;) {
		/* Read the user input on console */
		if (!ReadConsole(hconsole_input, inputbuf, CONSOLE_BUFSIZE, &nBytesRead, NULL)) {
			DWORD dwErr = GetLastError();
			if (dwErr == ERROR_NO_DATA)
				break;
		}
		/* Write the console input to remote stdin pipe */
		if (!WriteFile(hpipe_remote_stdin, inputbuf, nBytesRead, &nBytesWrote, NULL) || nBytesRead != nBytesWrote)
			break;
		/* Exit the for loop, if "exit" is typed */
		if (!_strnicmp(inputbuf, "exit", strlen("exit")) &&
			(inputbuf[strlen("exit")] == '\r'
			|| inputbuf[strlen("exit")] == '\t'
			|| inputbuf[strlen("exit")] == ' ')) {
			break;
		}
	}
	ExitThread(0);
}

/**
 * @brief
 *	Start threads to listen on pipes that connect to remote command's standard out/err/in.
 *
 * @param[in]	phout - Pointer to handle to the pipe connected to remote command's standard output.
 * @param[in]	pherror - Pointer to handle to the pipe connected to remote command's standard error.
 * @param[in]	phin - Pointer to handle to the pipe connected to remote command's standard input.
 *
 * @return  void
 *
 */
void
listen_remote_stdpipes(HANDLE *phout, HANDLE *pherror, HANDLE *phin)
{
	HANDLE hconsole_input_thread = INVALID_HANDLE_VALUE;
        int wait_timeout = 5000;
	/* Start a thread to listen to write remote command's standard input */
	if (phin)
		hconsole_input_thread = _beginthread(listen_remote_stdinpipe_thread, 0, phin);
	listen_remote_stdouterr_pipes(*phout, *pherror);
	/* Wait upto 5 seconds for the standard input pipe thread to exit */
        WaitForSingleObject(hconsole_input_thread, wait_timeout);
	close_valid_handle(&(hconsole_input_thread));
}

/**
 * @brief
 *	Wait for the named pipes that redirect remote process's stdin/stdout/stderr.
 *
 * @param[in]	remote_host - name of the remote host
 * @param[in]	pipename_append - appendix to standard pipe name
 * @param[in]	connect_stdin - whether to connect stdin or not. Connect if true.
 *
 * @return  BOOL
 * @retval	TRUE - success
 * @retval	FALSE - failure
 *
 */
BOOL
execute_remote_shell_command(char *remote_host, char *pipename_append, BOOL connect_stdin)
{
	char stdout_pipe[PIPENAME_MAX_LENGTH] = {0};
	char stdin_pipe[PIPENAME_MAX_LENGTH] = {0};
	char stderr_pipe[PIPENAME_MAX_LENGTH] = {0};
	int retry = 0;
        int max_retry = 10;
	int retry_interval = 1000; /* interval between each retry */
	HANDLE hPipe_remote_stdout = INVALID_HANDLE_VALUE;
	HANDLE hPipe_remote_stdin = INVALID_HANDLE_VALUE;
	HANDLE hPipe_remote_stderr = INVALID_HANDLE_VALUE;
	SECURITY_ATTRIBUTES SecAttrib = {0};
	SECURITY_DESCRIPTOR SecDesc;

	if (InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION) == 0) {
		return FALSE;
	}
	if (SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, FALSE) == 0) {
		return FALSE;
	}
	SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
	SecAttrib.lpSecurityDescriptor = &SecDesc;;
	SecAttrib.bInheritHandle = TRUE;

	/* Pipe that redirects stdout of remote process */
	snprintf(stdout_pipe, PIPENAME_MAX_LENGTH - 1, "\\\\%s\\pipe\\%s%s", remote_host, INTERACT_STDOUT, pipename_append);
	/* Pipe that redirects stderr of remote process */
	snprintf(stderr_pipe, PIPENAME_MAX_LENGTH - 1, "\\\\%s\\pipe\\%s%s", remote_host, INTERACT_STDERR, pipename_append);
	/* Pipe that redirects user input to the remote process */
	snprintf(stdin_pipe, PIPENAME_MAX_LENGTH - 1, "\\\\%s\\pipe\\%s%s", remote_host, INTERACT_STDIN, pipename_append);
	while (retry++ < max_retry) {
		/* Wait for the stdout pipe to be available. Open an handle to it once it is available. */
		if (hPipe_remote_stdout == INVALID_HANDLE_VALUE)
			hPipe_remote_stdout = do_WaitNamedPipe(stdout_pipe, NMPWAIT_WAIT_FOREVER, GENERIC_READ);
		/* Wait for the stderr pipe to be available. Open an handle to it once it is available. */
		if (hPipe_remote_stderr == INVALID_HANDLE_VALUE)
			hPipe_remote_stderr = do_WaitNamedPipe(stderr_pipe, NMPWAIT_WAIT_FOREVER, GENERIC_READ);
		if (connect_stdin) {
			/* Wait for the stdin pipe to be available. Open an handle to it once it is available. */
			if (hPipe_remote_stdin == INVALID_HANDLE_VALUE)
				hPipe_remote_stdin = do_WaitNamedPipe(stdin_pipe, NMPWAIT_WAIT_FOREVER, GENERIC_WRITE);
		}
		if (connect_stdin) {
			if (hPipe_remote_stdin != INVALID_HANDLE_VALUE
				&& hPipe_remote_stdout != INVALID_HANDLE_VALUE
				&& hPipe_remote_stderr != INVALID_HANDLE_VALUE)
				break;
		}
		else if (hPipe_remote_stdout != INVALID_HANDLE_VALUE && hPipe_remote_stderr != INVALID_HANDLE_VALUE)
			break;
		/* One of the pipes failed, try it again after <retry_interval> milliseconds */
		Sleep(retry_interval);
	}

	if (hPipe_remote_stdout == INVALID_HANDLE_VALUE || hPipe_remote_stderr == INVALID_HANDLE_VALUE)
		return FALSE;
	if (connect_stdin && hPipe_remote_stdin == INVALID_HANDLE_VALUE)
		return FALSE;
	/*
	 * Listen to these pipes.
	 * Read the redirected stdout and write to the stdout.
	 * Read the redirected stderr and write to the stderr.
	 * Read the user input and redirect it to the stdin pipe.
	 */
	if (connect_stdin)
		listen_remote_stdpipes(&hPipe_remote_stdout, &hPipe_remote_stderr, &hPipe_remote_stdin);
	else
		listen_remote_stdpipes(&hPipe_remote_stdout, &hPipe_remote_stderr, NULL);
	close_valid_handle(&(hPipe_remote_stdout));
	close_valid_handle(&(hPipe_remote_stderr));
	close_valid_handle(&(hPipe_remote_stdin));
	return TRUE;
}

/**
 * @brief
 *	Execute a command at remote shell after connecting to IPC$ at remote host.
 *
 * @param[in]	remote_host - name of the remote host
 * @param[in]	pipename_append - appendix to standard pipe name
 * @param[in]	connect_stdin - whether to connect stdin or not. Connect if true.
 *
 * @return  int
 * @retval	0 - success
 * @retval	-1 - failure
 *
 */
int
remote_shell_command(char *remote_host, char *pipename_append, BOOL connect_stdin)
{
	char console_title[CONSOLE_TITLE_BUFSIZE] = "";

	/* Set console title */
	sprintf(console_title, "Connecting to %s...", remote_host);
	SetConsoleTitle(console_title);

	/* Connect to remote host's IPC$ */
	if (!connect_remote_resource(remote_host, "IPC$", TRUE)) {
		fprintf(stderr, "Couldn't connect to host %s\n", remote_host);
		return -1;
	}
	/* Start the process at remote shell */
        if(execute_remote_shell_command(remote_host, pipename_append, connect_stdin) == FALSE) {
                fprintf(stderr, "Couldn't execute remote shell at host %s\n", remote_host);
                return -1;
        }
	if(connect_remote_resource(remote_host, "IPC$", FALSE) == FALSE)
                return -1;
	return 0;
}
