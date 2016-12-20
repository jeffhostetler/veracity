/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 *
 * @file sg_exec.c
 *
 * @details Routines for shelling-out or otherwise executing a
 * child or subordinate process.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#if defined(MAC) || defined(LINUX)
#include <unistd.h>
#include <sys/wait.h>
#endif

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#	define TRACE_EXEC			0
#else
#	define TRACE_EXEC			0
#endif

//////////////////////////////////////////////////////////////////

void _sg_exec__get_last(SG_context* pCtx, const char * sz, const char ** ppResult)
{
	const char * pResult = NULL;
	SG_NULLARGCHECK_RETURN(sz);
	SG_NULLARGCHECK_RETURN(ppResult);
	pResult = sz;
	while(*sz!='\0')
	{
		if('/'==*sz || '\\'==*sz)
			pResult = sz+1;
		sz += 1;
	}
	*ppResult = pResult;
}

void _sg_exec__get_last__string(SG_context* pCtx, const char * sz, SG_string ** ppResult)
{
	SG_ERR_CHECK_RETURN(  _sg_exec__get_last(pCtx, sz, &sz)  );
	SG_ERR_CHECK_RETURN(  SG_STRING__ALLOC__BUF_LEN(pCtx, ppResult, (const SG_byte*)sz, SG_STRLEN(sz))  );
}

#if defined(MAC) || defined(LINUX)
/**
 * Run the child process of synchronous fork/exec.
 * Our caller has already forked.  We are called to
 * do whatever file descriptor table juggling is required
 * and to exec the given command.
 *
 * THIS FUNCTION DOES NOT RETURN.  IT EITHER EXEC() THE
 * GIVEN COMMAND OR IT EXITS WITH AN EXIT STATUS.
 *
 * WE ARE GIVEN A SG_CONTEXT because we need to have one
 * for the routines that we call, but we cannot return a
 * stacktrace to our caller because we do not return.
 */
void _sync__run_child__files(
	SG_context* pCtx,
	const char * pCmd,
	const char ** pArgv,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr)
{
	int status;
	int fd;
	SG_bool bExecAttempted = SG_FALSE;

	if (pFileStdIn)
	{
		SG_ERR_CHECK(  SG_file__get_fd(pCtx,pFileStdIn,&fd)  );
		(void)close(STDIN_FILENO);
		status = dup2(fd,STDIN_FILENO);
		if (status == -1)
			SG_ERR_THROW(  SG_ERR_ERRNO(errno)  );
	}
	else
	{
		// TODO i'm thinking we should let the subordinate process borrow the
		// TODO STDIN of the parent, but i don't have a good reason to say that.
		// TODO
		// TODO Decide if we like this behavior.  Note that if we are in a GUI
		// TODO application, this may be of little use.
	}

	if (pFileStdOut)
	{
		SG_ERR_CHECK(  SG_file__get_fd(pCtx,pFileStdOut,&fd)  );
		(void)close(STDOUT_FILENO);
		status = dup2(fd,STDOUT_FILENO);
		if (status == -1)
			SG_ERR_THROW(  SG_ERR_ERRNO(errno)  );
	}
	else
	{
		// TODO again, i'm going to let the subordinate process borrow the parent's
		// TODO STDOUT.  This might be useful for our command-line client.
	}

	if (pFileStdErr)
	{
		SG_ERR_CHECK(  SG_file__get_fd(pCtx,pFileStdErr,&fd)  );
		(void)close(STDERR_FILENO);
		status = dup2(fd,STDERR_FILENO);
		if (status == -1)
			SG_ERR_THROW(  SG_ERR_ERRNO(errno)  );
	}
	else
	{
		// TODO again, i'm going to let the subordinate process borrow the parent's
		// TODO STDERR.  This might be useful for our command-line client.
	}

	// close all other files (including the other copy of the descriptors
	// that we just dup'd).  this prevents external applications from mucking
	// with any databases that the parent may have open....

	for (fd=STDERR_FILENO+1; fd < (int)FD_SETSIZE; fd++)
		close(fd);

	//////////////////////////////////////////////////////////////////

	execvp( pCmd, (char * const *)pArgv );

	//////////////////////////////////////////////////////////////////

	bExecAttempted = SG_TRUE;	// if we get here, exec() failed.
	SG_ERR_THROW(  SG_ERR_ERRNO(errno)  );

	// fall thru and print something about why the child could not be started.

fail:
#if TRACE_EXEC
	{
		SG_string * pStrErr = NULL;

		SG_context__err_to_string(pCtx, SG_TRUE, &pStrErr);	// IT IS IMPORTANT THAT THIS NOT CALL SG_ERR_IGNORE() because it will hide the info we are reporting.

		// TODO 2010/05/13 We've alredy closed STDERR in the child process, so
		// TODO            this message isn't really going anywhere....

		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   ("execvp() %s:\n"
									"\tCommand: %s\n"
									"\tErrorInfo:\n"
									"%s\n"),
								   ((bExecAttempted)
									? "returned with error"
									: "not attempted because of earlier errors in child"),
								   pCmd,
								   SG_string__sz(pStrErr))  );
		SG_STRING_NULLFREE(pCtx, pStrErr);
		fflush(stderr);
	}
#else
	SG_UNUSED(bExecAttempted);

#endif

	// we we the child.  use _exit() so that none of the atexit() stuff gets run.
	// for example, on Mac/Linux this could cause the debug memory leak report
	// to run and dump a bogus report to stderr.

	_exit(-1);	// an exit of -1 should get mapped to 255 in the parent
}

static void _sync__run_parent__files(SG_context* pCtx,
									 int pid,
									 SG_exit_status * pChildExitStatus,
									 SG_exec_result * per)
{
	int my_errno;
	int statusChild = 0;
	int result;

	//////////////////////////////////////////////////////////////////
	// HACK There is a problem just calling waitpid() on the Mac when
	// HACK in gdb.  We frequently get an interrupted system call.
	// HACK This causes us to abort everything.  I've think I've also
	// HACK seen this from a normal shell.  I think the problem is a
	// HACK race of some kind.
	// HACK
	// HACK The following is an attempt to spin a little until the child
	// HACK gets going enough so that we can actuall wait on it.  I think.
	//
	// TODO consider adding a counter or clock to this so that we don't
	// TODO get stuck here infinitely.  I don't think it'll happen, but
	// TODO it doesn't hurt....
	//////////////////////////////////////////////////////////////////

	while (1)
	{
		result = waitpid(pid,&statusChild,0);
		if (result == pid)
			goto done_waiting;

		my_errno = errno;

#if TRACE_EXEC
		SG_ERR_IGNORE(  SG_console(pCtx,
								   SG_CS_STDERR,
								   "waitpid: [result %d][errno %d]\n",
								   result,my_errno)  );
#endif

		switch (my_errno)
		{
		case 0:
#if 1 || TRACE_EXEC
			if (result > 0)
			{
				// TODO 2011/02/09 if we get no error and a result > 0
				// TODO            it probably means we reaped an async
				// TODO            child.  we should let this loop again
				// TODO            (until we get an ECHILD or the child
				// TODO            that we are expecting), right?
				SG_ERR_IGNORE(  SG_console(pCtx,
										   SG_CS_STDERR,
										   "waitpid: reaper a different child? [result %d][errno %d]\n",
										   result,my_errno)  );
			}
#endif
		default:
		case ECHILD:		// we don't have a child ? ? ?
			*per = SG_EXEC_RESULT__UNKNOWN;
			*pChildExitStatus = SG_EXIT_STATUS_BOGUS;
			return;

		case EDEADLK:		// resource deadlock avoided
		case EAGAIN:		// resource temporarily unavailable
		case EINTR:			// interrupted system call
			break;			// try again
		}
	}

done_waiting:

	if (WIFEXITED(statusChild))
	{
		// child exited properly (with or without error)

		SG_exit_status child_exit_status = WEXITSTATUS(statusChild);

#if TRACE_EXEC
		SG_ERR_IGNORE(  SG_console(pCtx,
								   SG_CS_STDERR,
								   "waitpid: child exited: [pid %d] [0x%x] [exit_status %d]\n",
								   pid, statusChild, child_exit_status)  );
#endif

		if (child_exit_status == 255)		// -1 from child
		{
			// this usually means that the child could not exec the program
			// for whatever reason, such as no access to the exe or the exe
			// did not exist.
			// 
			// we can't tell at this point if it was a EACCESS or a ENOENT.

			*per = SG_EXEC_RESULT__FAILED;
			*pChildExitStatus = SG_EXIT_STATUS_BOGUS;
		}
		else
		{
			*per = SG_EXEC_RESULT__NORMAL;
			*pChildExitStatus = child_exit_status;
		}
	}
	else
	{
		// child crashed or was killed: WTERMSIG() or WCOREDUMP() or ...

#if 1 || TRACE_EXEC
		SG_ERR_IGNORE(  SG_console(pCtx,
							   SG_CS_STDERR,
								   "waitpid: child crashed/killed: [pid %d] [0x%x] [signaled %x][stopped %x] [?term? %d] [?stop? %d]\n",
								   pid,
								   statusChild,
								   WIFSIGNALED(statusChild),WIFSTOPPED(statusChild),
								   ((WIFSIGNALED(statusChild)) ? WTERMSIG(statusChild) : -1),
								   ((WIFSTOPPED(statusChild)) ? WSTOPSIG(statusChild) : -1))  );
#endif

		*per = SG_EXEC_RESULT__SET_ABNORMAL( (WTERMSIG(statusChild)) ); // i only really care about signals
		*pChildExitStatus = SG_EXIT_STATUS_BOGUS;
	}
}

void SG_exec__exec_async__files(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_process_id * pProcessID)
{
	// The caller passes a SG_vector of SG_string for ARGV.
	// They DO NOT include the program-to-exec in the vector.

	SG_uint32 nrArgsGiven = 0;
	SG_uint32 nrArgsTotal;
	SG_uint32 kArg;
	const char ** pArgv = NULL;
	const char * pStrCmdEntryname = NULL;
	int pid;

	SG_NULLARGCHECK_RETURN(pCmd);
	SG_NULLARGCHECK_RETURN(pProcessID);

	if (pArgVec)
		SG_ERR_CHECK_RETURN(  SG_exec_argvec__length(pCtx,pArgVec,&nrArgsGiven)  );
	nrArgsTotal = nrArgsGiven + 2;		// +1 for program, +1 for terminating null.

	// We build a platform-appropriate "char * argv[]" to give to execv().

	SG_ERR_CHECK_RETURN(  SG_alloc(pCtx,nrArgsTotal,sizeof(char *),&pArgv)  );

	// On Linux/Mac, we want argv[0] to be the ENTRYNAME of the program.

	// TODO I'm thinking we should convert each arg from utf-8 to an os-buffer
	// TODO aka locale buffer and give that to exec().

	SG_ERR_CHECK(  _sg_exec__get_last(pCtx,pCmd,&pStrCmdEntryname)  );
	pArgv[0] = pStrCmdEntryname;

	for (kArg=0; kArg<nrArgsGiven; kArg++)
	{
		const SG_string * pStr_k;

		SG_ERR_CHECK(  SG_exec_argvec__get(pCtx,pArgVec,kArg,&pStr_k)  );
		if (!pStr_k || (SG_string__length_in_bytes(pStr_k) == 0))
		{
			// allow for blank or empty cell as a "" entry.  (though,
			// i'm not sure why.)

			pArgv[kArg+1] = "";
		}
		else
		{
			// TODO utf-8 --> os-buffer ??
			pArgv[kArg+1] = SG_string__sz(pStr_k);
		}
	}

	// pArgv[nrArgsTotal-1] should already be NULL

#if TRACE_EXEC
	{
		SG_uint32 k;
		SG_ERR_IGNORE(  SG_console(pCtx,
								   SG_CS_STDERR,
								   "Exec [%s]\n",
								   pCmd)  );
		for (k=0; k<nrArgsTotal; k++)
		{
			SG_ERR_IGNORE(  SG_console(pCtx,
									   SG_CS_STDERR,
									   "     Argv[%d] %s\n",
									   k,pArgv[k])  );
		}
	}
#endif

	// just incase we have anything buffered in our output FILE streams
	// (and the child is sharing them), flush them out before the child
	// gets started.

	fflush(stdout);
	fflush(stderr);

	pid = fork();
	if (pid == -1)
	{
		SG_ERR_THROW(  SG_ERR_ERRNO(errno)  );
	}
	else if (pid == 0)
	{
		// we are in the child process.
		//
		// note that we ***CANNOT*** use SG_ERR_CHECK() and the normal
		// goto fail, cleanup, and return -- whether we succeed or fail,
		// we must exit when we are finished.

		_sync__run_child__files(pCtx,pCmd,pArgv,pFileStdIn,pFileStdOut,pFileStdErr);
		_exit(-1);	// not reached
		return;		// not reached
	}

	// otherwise, we are the parent process.
	//For now, do nothing.
	*pProcessID = (SG_process_id)pid;

	// fall thru to common cleanup.

fail:
	SG_NULLFREE(pCtx, pArgv);
}

void SG_exec__async_process_result(
    SG_context* pCtx,
    SG_process_id processID,
    SG_exec_result * pChildResult,
    SG_exit_status * pChildExitStatus)
{
	pid_t pid;
	int status;
	
	SG_NULLARGCHECK_RETURN(pChildResult);
    
	pid = waitpid(processID, &status, WNOHANG);
	
	if(pid==0)
	{
		*pChildResult = SG_EXEC_RESULT__UNKNOWN;
	}
	else if(pid==-1)
	{
		SG_ERR_THROW_RETURN(  SG_ERR_ERRNO(errno)  );
	}
	else
	{
		SG_ASSERT(pid==(pid_t)processID);
		if(WIFEXITED(status))
		{
			*pChildResult = SG_EXEC_RESULT__NORMAL;
			if(pChildExitStatus)
			{
				*pChildExitStatus = (SG_exit_status)(WEXITSTATUS(status));
			}
		}
		else if(WIFSIGNALED(status))
		{
			*pChildResult = SG_EXEC_RESULT__SET_ABNORMAL(WTERMSIG(status));
		}
		else
		{
			*pChildResult = SG_EXEC_RESULT__UNKNOWN;
		}
		
	}
}

void SG_exec__exec_sync__files__details(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_exit_status * pChildExitStatus,
	SG_exec_result * per,
	SG_process_id * pPid)
{
	// The caller passes a SG_vector of SG_string for ARGV.
	// They DO NOT include the program-to-exec in the vector.

	SG_uint32 nrArgsGiven = 0;
	SG_uint32 nrArgsTotal;
	SG_uint32 kArg;
	const char ** pArgv = NULL;
	const char * pStrCmdEntryname = NULL;
	int pid;
	SG_exit_status exitStatus = 0;
	SG_exec_result execResult = SG_EXEC_RESULT__UNKNOWN;

	SG_NULLARGCHECK_RETURN(pCmd);
	SG_NULLARGCHECK_RETURN(pChildExitStatus);
	SG_NULLARGCHECK_RETURN(per);
	SG_NULLARGCHECK_RETURN(pPid);

	if (pArgVec)
		SG_ERR_CHECK_RETURN(  SG_exec_argvec__length(pCtx,pArgVec,&nrArgsGiven)  );
	nrArgsTotal = nrArgsGiven + 2;		// +1 for program, +1 for terminating null.

	// We build a platform-appropriate "char * argv[]" to give to execv().

	SG_ERR_CHECK_RETURN(  SG_alloc(pCtx,nrArgsTotal,sizeof(char *),&pArgv)  );

	// On Linux/Mac, we want argv[0] to be the ENTRYNAME of the program.

	// TODO I'm thinking we should convert each arg from utf-8 to an os-buffer
	// TODO aka locale buffer and give that to exec().

	SG_ERR_CHECK(  _sg_exec__get_last(pCtx,pCmd,&pStrCmdEntryname)  );
	pArgv[0] = pStrCmdEntryname;

	for (kArg=0; kArg<nrArgsGiven; kArg++)
	{
		const SG_string * pStr_k;

		SG_ERR_CHECK(  SG_exec_argvec__get(pCtx,pArgVec,kArg,&pStr_k)  );
		if (!pStr_k || (SG_string__length_in_bytes(pStr_k) == 0))
		{
			// allow for blank or empty cell as a "" entry.  (though,
			// i'm not sure why.)

			pArgv[kArg+1] = "";
		}
		else
		{
			// TODO utf-8 --> os-buffer ??
			pArgv[kArg+1] = SG_string__sz(pStr_k);
		}
	}

	// pArgv[nrArgsTotal-1] should already be NULL

#if TRACE_EXEC
	{
		SG_uint32 k;
		SG_ERR_IGNORE(  SG_console(pCtx,
								   SG_CS_STDERR,
								   "Exec [%s]\n",
								   pCmd)  );
		for (k=0; k<nrArgsTotal; k++)
		{
			SG_ERR_IGNORE(  SG_console(pCtx,
									   SG_CS_STDERR,
									   "     Argv[%d] %s\n",
									   k,pArgv[k])  );
		}
	}
#endif

	// just incase we have anything buffered in our output FILE streams
	// (and the child is sharing them), flush them out before the child
	// gets started.

	fflush(stdout);
	fflush(stderr);

	pid = fork();
	if (pid == -1)
	{
		SG_ERR_THROW(  SG_ERR_ERRNO(errno)  );
	}
	else if (pid == 0)
	{
		// we are in the child process.
		//
		// note that we ***CANNOT*** use SG_ERR_CHECK() and the normal
		// goto fail, cleanup, and return -- whether we succeed or fail,
		// we must exit when we are finished.

		_sync__run_child__files(pCtx,pCmd,pArgv,pFileStdIn,pFileStdOut,pFileStdErr);
		_exit(-1);	// not reached
		return;		// not reached
	}

	// otherwise, we are the parent process.

	SG_ERR_CHECK(  _sync__run_parent__files(pCtx,pid,&exitStatus,&execResult)  );

	// if have an normal setup/argcheck error and don't get
	// to the point where we try to start the child, we
	// throw just like any other sglib function.  this has
	// already been handled before we did the fork.
	//
	// if we actually try to start the child, we return an exec_result
	// describing what happened to the child.

#if TRACE_EXEC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Exec [result %x : %d][status %d] [%s]\n",
							   (execResult & SG_EXEC_RESULT__MASK_TYPE),
							   (execResult & SG_EXEC_RESULT__MASK_VALUE),
							   exitStatus,
							   pCmd)  );
#endif

	*pChildExitStatus = exitStatus;
	*per = execResult;
	*pPid = pid;

fail:
	SG_NULLFREE(pCtx, pArgv);
}

//////////////////////////////////////////////////////////////////

#include "sg_exec__debug_stacktrace.h"

//////////////////////////////////////////////////////////////////

void SG_exec__exec_sync__files(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_exit_status * pChildExitStatus)
{
	SG_exit_status childExitStatus;
	SG_exec_result execResult;
	SG_process_id pid;

	SG_ERR_CHECK(  SG_exec__exec_sync__files__details(pCtx, pCmd, pArgVec,
													  pFileStdIn, pFileStdOut, pFileStdErr,
													  &childExitStatus, &execResult, &pid)  );
	
	// since caller didn't want to micro-manage and only
	// wants to have to deal with the exit-status of a
	// regular child that started, ran, and exited
	// properly (with or without an error status), we
	// throw if there was a problem.

	switch (execResult & SG_EXEC_RESULT__MASK_TYPE)
	{
	default:
	case SG_EXEC_RESULT__UNKNOWN:
	case SG_EXEC_RESULT__FAILED:
		SG_ERR_THROW2(  SG_ERR_EXEC_FAILED,
						(pCtx, "%s", pCmd)  );
		break;

	case SG_EXEC_RESULT__ABNORMAL:
#if defined(HAVE_EXEC_DEBUG_STACKTRACE)
		SG_ERR_IGNORE(  SG_exec_debug__get_stacktrace(pCtx, pCmd, pid)  );
#endif
		SG_ERR_THROW2(  SG_ERR_ABNORMAL_TERMINATION,
						(pCtx, "sig(%d): %s",
						 SG_EXEC_RESULT__GET_SUB_VALUE(execResult),
						 pCmd)  );
		break;

	case SG_EXEC_RESULT__NORMAL:
		*pChildExitStatus = childExitStatus;
		break;
	}

fail:
	return;
}
#endif // defined(MAC) || defined(LINUX)

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)

void SG_exec__exec_async__files(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_process_id * pProcessID)
{
	// The caller passes a SG_vector of SG_string for ARGV.
	// They DO NOT include the program-to-exec in the vector.

	SG_string * pStrCmdLine = NULL;
	wchar_t * pWcharCmdLine = NULL;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
	DWORD dwCreationFlags;
	//DWORD dwChildExitStatus;
	HANDLE hFileStdIn, hFileStdOut, hFileStdErr;

	SG_NULLARGCHECK_RETURN(pCmd);
	SG_NULLARGCHECK_RETURN(pProcessID);

	memset(&pi,0,sizeof(pi));
	memset(&si,0,sizeof(si));

	SG_ERR_CHECK_RETURN(  SG_exec__build_command(pCtx, pCmd, pArgVec, &pStrCmdLine)  );

	// convert UTF-8 version of the command pathname and the command line to wchar_t.
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx,SG_string__sz(pStrCmdLine),&pWcharCmdLine,NULL)  );

    si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;

	// deal with STDIN/STDOUT/STDERR in the child/subordinate process.
	//
	// if the caller gave us open files, we use the file handles within
	// them; otherwise, we clone our STDIN and friends.
	// we must assume that the files provided by the caller are correctly
	// opened and positioned....

	if (pFileStdIn)
		SG_ERR_CHECK(  SG_file__get_handle(pCtx,pFileStdIn,&hFileStdIn)  );
	else
		hFileStdIn = GetStdHandle(STD_INPUT_HANDLE);

	if (pFileStdOut)
		SG_ERR_CHECK(  SG_file__get_handle(pCtx,pFileStdOut,&hFileStdOut)  );
	else
		hFileStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (pFileStdErr)
		SG_ERR_CHECK(  SG_file__get_handle(pCtx,pFileStdErr,&hFileStdErr)  );
	else
		hFileStdErr = GetStdHandle(STD_ERROR_HANDLE);

	if (!DuplicateHandle(GetCurrentProcess(),hFileStdIn,GetCurrentProcess(),
						 &si.hStdInput,0,TRUE,DUPLICATE_SAME_ACCESS))
	{
		//	SG_ERR_THROW(  SG_ERR_GETLASTERROR(GetLastError())  );
	}

	if (!DuplicateHandle(GetCurrentProcess(),hFileStdOut,GetCurrentProcess(),
						 &si.hStdOutput,0,TRUE,DUPLICATE_SAME_ACCESS))
	{
		//	SG_ERR_THROW(  SG_ERR_GETLASTERROR(GetLastError())  );
	}

	if (!DuplicateHandle(GetCurrentProcess(),hFileStdErr,GetCurrentProcess(),
						 &si.hStdError,0,TRUE,DUPLICATE_SAME_ACCESS))
	{
		//	SG_ERR_THROW(  SG_ERR_GETLASTERROR(GetLastError())  );
	}

#if defined(DEBUG)
	//for debug builds, start in a new, easy-to-kill window
	dwCreationFlags = 0;
#else
	//for release builds, don't bother with a new window.
	dwCreationFlags = CREATE_NO_WINDOW;
#endif

	if (!CreateProcessW(NULL,pWcharCmdLine,
						NULL,NULL,
						TRUE,dwCreationFlags,
						NULL,NULL,
						&si,&pi))
		SG_ERR_THROW2(  SG_ERR_GETLASTERROR(GetLastError()),
						(pCtx,
						 "Cannot execute command [%s] with args [%s]",
						 pCmd,
						 SG_string__sz(pStrCmdLine))  );

	*pProcessID = (SG_process_id) pi.dwProcessId;
	// fall thru to common cleanup.

fail:

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	SG_STRING_NULLFREE(pCtx, pStrCmdLine);
	SG_NULLFREE(pCtx, pWcharCmdLine);

	CloseHandle(si.hStdInput);
	CloseHandle(si.hStdOutput);
	CloseHandle(si.hStdError);
}

void SG_exec__async_process_result(
    SG_context* pCtx,
    SG_process_id processID,
    SG_exec_result * pChildResult,
    SG_exit_status * pChildExitStatus)
{
	HANDLE processHandle;

	SG_NULLARGCHECK_RETURN(pChildResult);
	SG_UNUSED(pChildExitStatus);

	processHandle = OpenProcess(SYNCHRONIZE, 0, (DWORD)processID);
	if (processHandle==NULL)
	{
		*pChildResult = SG_EXEC_RESULT__NORMAL;
		*pChildExitStatus = 0;
	}
	else
	{
		if(WaitForSingleObject(processHandle, 0)==WAIT_TIMEOUT)
		{
			*pChildResult = SG_EXEC_RESULT__UNKNOWN;
		}
		else
		{
			*pChildResult = SG_EXEC_RESULT__NORMAL;
			*pChildExitStatus = 0;
		}
		CloseHandle(processHandle);
	}
}

void SG_exec__exec_sync__files__details(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_exit_status * pChildExitStatus,
	SG_exec_result * per,
	SG_process_id * pPid)
{
	// The caller passes a SG_vector of SG_string for ARGV.
	// They DO NOT include the program-to-exec in the vector.

	SG_string * pStrCmdLine = NULL;
	wchar_t * pWcharCmdLine = NULL;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
	DWORD dwCreationFlags;
	DWORD dwChildExitStatus;
	HANDLE hFileStdIn, hFileStdOut, hFileStdErr;

	SG_NULLARGCHECK_RETURN(pCmd);
	SG_NULLARGCHECK_RETURN(pChildExitStatus);
	// per is optional
	// pPid is optional

	memset(&pi,0,sizeof(pi));
	memset(&si,0,sizeof(si));

	SG_ERR_CHECK_RETURN(  SG_exec__build_command(pCtx, pCmd, pArgVec, &pStrCmdLine)  );

	// convert UTF-8 version of the command pathname and the command line to wchar_t.
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx,SG_string__sz(pStrCmdLine),&pWcharCmdLine,NULL)  );

    si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;

	// deal with STDIN/STDOUT/STDERR in the child/subordinate process.
	//
	// if the caller gave us open files, we use the file handles within
	// them; otherwise, we clone our STDIN and friends.
	// we must assume that the files provided by the caller are correctly
	// opened and positioned....

	if (pFileStdIn)
		SG_ERR_CHECK(  SG_file__get_handle(pCtx,pFileStdIn,&hFileStdIn)  );
	else
		hFileStdIn = GetStdHandle(STD_INPUT_HANDLE);

	if (pFileStdOut)
		SG_ERR_CHECK(  SG_file__get_handle(pCtx,pFileStdOut,&hFileStdOut)  );
	else
		hFileStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (pFileStdErr)
		SG_ERR_CHECK(  SG_file__get_handle(pCtx,pFileStdErr,&hFileStdErr)  );
	else
		hFileStdErr = GetStdHandle(STD_ERROR_HANDLE);

	if (hFileStdIn != NULL && !DuplicateHandle(GetCurrentProcess(),hFileStdIn,GetCurrentProcess(),
						 &si.hStdInput,0,TRUE,DUPLICATE_SAME_ACCESS))
		SG_ERR_THROW(  SG_ERR_GETLASTERROR(GetLastError())  );

	if (hFileStdOut != NULL && !DuplicateHandle(GetCurrentProcess(),hFileStdOut,GetCurrentProcess(),
						 &si.hStdOutput,0,TRUE,DUPLICATE_SAME_ACCESS))
		SG_ERR_THROW(  SG_ERR_GETLASTERROR(GetLastError())  );

	if (hFileStdErr != NULL && !DuplicateHandle(GetCurrentProcess(),hFileStdErr,GetCurrentProcess(),
						 &si.hStdError,0,TRUE,DUPLICATE_SAME_ACCESS))
		SG_ERR_THROW(  SG_ERR_GETLASTERROR(GetLastError())  );

	// errors that happen before we try to create the child process
	// throw like normal because they are a problem that the parent
	// had.  just like the code before the fork() in the linux/mac
	// version.

	dwCreationFlags = 0;
	if (!CreateProcessW(NULL,pWcharCmdLine,
						NULL,NULL,
						TRUE,dwCreationFlags,
						NULL,NULL,
						&si,&pi))
	{
		DWORD dwErr = GetLastError();
		SG_error e = SG_ERR_GETLASTERROR(dwErr);

		// I'm going to say that if CreateProcessW() fails that that
		// is equivalent to the execvp() failing after the fork()
		// such that the child process exists and exits it -1 because
		// it can't do anything else.
		//
		// here we just return a normalized failed-to-start.

#if TRACE_EXEC
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "CreateProcessW failed %x for '%s %s'\n",
								   dwErr, pCmd, SG_string__sz(pStrCmdLine))  );
#endif

		if (per)
		{
			*per = SG_EXEC_RESULT__FAILED;
			*pChildExitStatus = SG_EXIT_STATUS_BOGUS;

			if (pPid)
				*pPid = 0;
		}
		else
		{
			// throw a compound error message (with the nice details
			// that the OS gave us, but cross-platform normalized for
			// our callers).

			char buf[SG_ERROR_BUFFER_SIZE];

			SG_error__get_message(e, SG_TRUE, buf, sizeof(buf));
			SG_ERR_THROW2(  SG_ERR_EXEC_FAILED,
							(pCtx, "%s: %s %s", buf, pCmd, SG_string__sz(pStrCmdLine))  );
		}

		goto fail;
	}

	if (WaitForSingleObject(pi.hProcess,INFINITE) != WAIT_OBJECT_0)
	{
		DWORD dwErr = GetLastError();
		SG_error e = SG_ERR_GETLASTERROR(dwErr);

		// I'm going to say that if WaitForSingleObject() fails that
		// that is equivalent to the waitpid() failing with ECHILD.
		// This probably can't happen, but the net-net is that we don't
		// know what happened to the process.
		//
		// normalize this to unknown.

#if TRACE_EXEC
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "WaitForSingleObject failed %x for '%s %s'\n",
								   dwErr, pCmd, SG_string__sz(pStrCmdLine))  );
#endif
		if (per)
		{
			*per = SG_EXEC_RESULT__UNKNOWN;
			*pChildExitStatus = SG_EXIT_STATUS_BOGUS;

			if (pPid)
				*pPid = 0;
		}
		else
		{
			char buf[SG_ERROR_BUFFER_SIZE];

			SG_error__get_message(e, SG_TRUE, buf, sizeof(buf));
			SG_ERR_THROW2(  SG_ERR_EXEC_FAILED,
							(pCtx, "%s: %s %s", buf, pCmd, SG_string__sz(pStrCmdLine))  );
		}

		goto fail;
	}

	if (!GetExitCodeProcess(pi.hProcess,&dwChildExitStatus))
	{
		DWORD dwErr = GetLastError();
		SG_error e = SG_ERR_GETLASTERROR(dwErr);

		// this probably can't happen either, but if it does
		// we still don't know what happened to the process.
		//
		// again, we want to normalize this.

#if TRACE_EXEC
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "GetExitCodeProcess failed %x for '%s %s'\n",
								   dwErr, pCmd, SG_string__sz(pStrCmdLine))  );
#endif

		if (per)
		{
			*per = SG_EXEC_RESULT__UNKNOWN;
			*pChildExitStatus = SG_EXIT_STATUS_BOGUS;

			if (pPid)
				*pPid = 0;
		}
		else
		{
			char buf[SG_ERROR_BUFFER_SIZE];

			SG_error__get_message(e, SG_TRUE, buf, sizeof(buf));
			SG_ERR_THROW2(  SG_ERR_EXEC_FAILED,
							(pCtx, "%s: %s %s", buf, pCmd, SG_string__sz(pStrCmdLine))  );
		}

		goto fail;
	}

	// TODO 2011/02/10 dwChildExitStatus can be either the exit value
	// TODO            if the process exited, an unhandled exception,
	// TODO            or the value sent by another process calling
	// TODO            TerminateProcess.
	// TODO
	// TODO            figure out if there is a way to distinguish
	// TODO            these and return (__ABNORMAL,__STATUS_BOGUS)
	// TODO            for them (and maybe put something in the
	// TODO            value portion).

	if (per)
		*per = SG_EXEC_RESULT__NORMAL;
	if (pPid)
		*pPid = (SG_process_id)pi.dwProcessId;
	*pChildExitStatus = (SG_exit_status)dwChildExitStatus;

	// fall thru to common cleanup.

fail:

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	SG_STRING_NULLFREE(pCtx, pStrCmdLine);
	SG_NULLFREE(pCtx, pWcharCmdLine);

	CloseHandle(si.hStdInput);
	CloseHandle(si.hStdOutput);
	CloseHandle(si.hStdError);
}

void SG_exec__exec_sync__files(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_exit_status * pChildExitStatus)
{
	SG_ERR_CHECK_RETURN(  SG_exec__exec_sync__files__details(pCtx, pCmd, pArgVec,
															 pFileStdIn, pFileStdOut, pFileStdErr,
															 pChildExitStatus, NULL, NULL)  );
}
#endif

void SG_exec__system(
	SG_context* pCtx,
	const char* pszCmdLine,
	SG_exit_status* pExitStatus)
{
	int exitStatus = 0;

#if defined(WINDOWS)
	SG_string* pstr = NULL;
	wchar_t* pwszCmdLine = NULL;

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pstr, "\"%s\"", pszCmdLine)  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstr), &pwszCmdLine, NULL)  );

	exitStatus = _wsystem(pwszCmdLine);

	SG_STRING_NULLFREE(pCtx, pstr);
	SG_NULLFREE(pCtx, pwszCmdLine);
#else
	SG_UNUSED(pCtx);
	exitStatus = system(pszCmdLine);
#endif 

	if (pExitStatus)
		*pExitStatus = exitStatus;

	return;

#if defined(WINDOWS)
fail:
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_NULLFREE(pCtx, pwszCmdLine);
#endif
}

void SG_exec__system__argvec(
	SG_context*           pCtx,
	const char*           szFilename,
	const SG_exec_argvec* pArgs,
	SG_exit_status*       pExit
	)
{
	SG_string* sCommand = NULL;

	SG_ERR_CHECK(  SG_exec__build_command(pCtx, szFilename, pArgs, &sCommand)  );
	SG_ERR_CHECK(  SG_exec__system(pCtx, SG_string__sz(sCommand), pExit)  );

fail:
	SG_STRING_NULLFREE(pCtx, sCommand);
	return;
}

void SG_exec__build_command(
	SG_context*           pCtx,
	const char*           szFilename,
	const SG_exec_argvec* pArgs,
	SG_string**           ppCommand
	)
{
	SG_string* sCommand = NULL;
	SG_string* sEscaped = NULL;
	SG_uint32  uCount   = 0u;
	SG_uint32  uIndex   = 0u;

	SG_NULLARGCHECK(szFilename);
	SG_NULLARGCHECK(ppCommand);

	// start the command with an escaped copy of szFilename
	SG_ERR_CHECK(  SG_exec__escape_arg(pCtx, szFilename, &sEscaped)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &sCommand, sEscaped)  );
	SG_STRING_NULLFREE(pCtx, sEscaped);

	// run through each argument in the list
	if (pArgs != NULL)
	{
		SG_ERR_CHECK(  SG_exec_argvec__length(pCtx, pArgs, &uCount)  );
		for (uIndex = 0u; uIndex < uCount; ++uIndex)
		{
			const SG_string* sArg = NULL;

			// get the argument, escape it, and append that to the command
			SG_ERR_CHECK(  SG_exec_argvec__get(pCtx, pArgs, uIndex, &sArg)  );
			SG_ERR_CHECK(  SG_exec__escape_arg(pCtx, SG_string__sz(sArg), &sEscaped)  );
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sCommand, " ")  );
			SG_ERR_CHECK(  SG_string__append__string(pCtx, sCommand, sEscaped)  );
			SG_STRING_NULLFREE(pCtx, sEscaped);
		}
	}

#if TRACE_EXEC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_exec__build_command: [%s]\n",
							   SG_string__sz(sCommand))  );
#endif

	*ppCommand = sCommand;
	sCommand = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sCommand);
	SG_STRING_NULLFREE(pCtx, sEscaped);
	return;
}

#if defined(WINDOWS)
/**
 * Apply special quoting rules for backslashes on Windows.
 *
 * When the exec'd application gets started its command line will be parsed using
 * special rules for n-backslashes followed by a double-quote.
 * So we have to pre-compensate for that.
 * 
 * See http://msdn.microsoft.com/en-us/library/windows/desktop/bb776391(v=vs.85).aspx
 * See the docs for "CommandLineToArgvW".
 *
 * NOTE: We DO NOT just blindly double all backslashes.
 * 
 * See W8519.
 *
 * TODO 2012/07/11 Not sure how well this plays with the "" trick
 * TODO            in _quote_arg().
 *
 */
static void _quote_backslashes(SG_context * pCtx,
							   SG_string ** ppString)
{
	SG_string * pStringIn  = *ppString;
	SG_string * pStringNew = NULL;
	const char * pszIn = SG_string__sz(pStringIn);
	SG_uint32 nrBS = 0;
	SG_byte bufBS[2] = { '\\', '\\' };

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringNew)  );
	
	while (*pszIn)
	{
		if (*pszIn == '\\')
		{
			nrBS++;
			pszIn++;
		}
		else if (nrBS > 0)
		{
			// n-backslashes followed by a double-quote get doubled.
			// n-backslashes followed by any other char do not get doubled.

			SG_uint32 x = ((*pszIn == '"') ? 2 : 1);
			SG_uint32 k;

			for (k=0; k<nrBS; k++)
				SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pStringNew, bufBS, x)  );
			nrBS = 0;
			SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pStringNew, (SG_byte *)pszIn, 1)  );
			pszIn++;
		}
		else
		{
			SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pStringNew, (SG_byte *)pszIn, 1)  );
			pszIn++;
		}
	}

	SG_ASSERT( (nrBS==0) );		// since caller just appended a final double-quote

#if 1 && defined(DEBUG)
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "QuoteBackslashes: [%s] --> [%s]\n",
							   SG_string__sz(pStringIn), SG_string__sz(pStringNew))  );
#endif

	// steal the caller's string and give them ours.

	SG_STRING_NULLFREE(pCtx, pStringIn);
	*ppString = pStringNew;
	pStringNew = NULL;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pStringNew);
}
#endif

/**
 * Adds quotes to an argument to ensure that it's treated as a single value by
 * the shell.  Escapes any existing quotes in the value to make sure that they're
 * preserved.
 */
static void _quote_arg(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	const char* szArg,   //< [in] The argument to quote.
	SG_string** ppQuoted //< [out] The quoted result.
	)
{
#if defined(WINDOWS)
	// Windows uses double quotes and escapes existing ones by pairing them.
	static const SG_byte aQuote[]        = { '"' };
	static const SG_byte aEscapedQuote[] = { '"', '"' };
#else
	// sh uses single quotes and doesn't allow escaping existing ones.
	// However, because sh concatenates adjacent strings, we can effectively
	// escape the single quotes with the following: '"'"'
	// which will result in a double-quoted string (containing a single quote)
	// being concatenated with the single-quoted strings before and after it.
	static const SG_byte aQuote[]        = { '\'' };
	static const SG_byte aEscapedQuote[] = { '\'', '"', '\'', '"', '\'' };
#endif

	SG_string* sQuoted = NULL;

	// allocate a copy of the argument
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sQuoted, szArg)  );

	// escape any quotes that already exist in it
	SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, sQuoted, aQuote, SG_NrElements(aQuote), aEscapedQuote, SG_NrElements(aEscapedQuote), SG_UINT32_MAX, SG_TRUE, NULL)  );

	// surround the entire thing with quotes
	SG_ERR_CHECK(  SG_string__insert__buf_len(pCtx, sQuoted, 0u, aQuote, SG_NrElements(aQuote))  );
	SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, sQuoted, aQuote, SG_NrElements(aQuote))  );

#if defined(WINDOWS)
	if (strchr(SG_string__sz(sQuoted), '\\'))
		SG_ERR_CHECK(  _quote_backslashes(pCtx, &sQuoted)  );
#endif

	// return the quoted string
	*ppQuoted = sQuoted;
	sQuoted = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sQuoted);
	return;
}

void SG_exec__escape_arg(
	SG_context* pCtx,
	const char* szArg,
	SG_string** ppEscaped
	)
{

#define SEPARATOR_CHARS ":="
#define QUOTE_CHARS "'\""
#if defined(WINDOWS)
#	define FLAG_CHARS "/-"
#else
#	define FLAG_CHARS "-"
#endif

	SG_string* sEscaped = NULL;

	// check if this argument indicates a flag/switch
	if (szArg != NULL && *szArg && strchr(FLAG_CHARS, szArg[0]) != NULL)
	{
		const char* szValue = NULL;

		// this argument is a flag/switch, such as: --flag=a long string
		// Naively we would just quote it as: "--flag=a long string"
		// but it turns out that some programs can't parse that correctly.
		// Therefore, we want to only quote the flag's value, not its name,
		// and end up with this: --flag="a long string"
		// In the case of a valueless flag, we won't quote it at all.

		// search for the separator between the flag and its value
		szValue = strpbrk(szArg, SEPARATOR_CHARS QUOTE_CHARS);
		if (szValue == NULL)
		{
			// no separator or quote found
			// This is a valueless flag, no quoting necessary.
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sEscaped, szArg)  );
		}
		else if (strchr(QUOTE_CHARS, szValue[0]) != NULL)
		{
			// we found a quote before we found a separator
			// we'll assume that it's already quoted correctly
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sEscaped, szArg)  );
		}
		else if (strchr(QUOTE_CHARS, szValue[1]) != NULL)
		{
			// there's already a quote directly after the separator
			// we'll assume that the flag's value is already quoted correctly
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sEscaped, szArg)  );
		}
		else if (szValue[1] == '\0')
		{
			// the separator was the last character in the string
			// we'll assume it's actually part of the flag name
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sEscaped, szArg)  );
		}
		else
		{
			// this is a flag with a value
			// quote the value part and prepend the unquoted name and separator
			// the +1 in both places accounts for the separator
			SG_ERR_CHECK(  _quote_arg(pCtx, szValue + 1, &sEscaped)  );
			SG_ERR_CHECK(  SG_string__insert__buf_len(pCtx, sEscaped, 0u, (SG_byte*)szArg, (SG_uint32)(szValue - szArg + 1))  );
		}
	}
	else
	{
		// argument doesn't appear to be a flag/switch, quote the whole thing
		SG_ERR_CHECK(  _quote_arg(pCtx, szArg, &sEscaped)  );
	}

	*ppEscaped = sEscaped;
	sEscaped = NULL;

#undef SEPARATOR_CHARS
#undef QUOTE_CHARS
#undef FLAG_CHARS

fail:
	SG_STRING_NULLFREE(pCtx, sEscaped);
	return;
}
