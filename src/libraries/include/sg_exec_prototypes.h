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
 * @file sg_exec_prototypes.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_EXEC_PROTOTYPES_H
#define H_SG_EXEC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Exec the given command with the given arguments in a subordinate
 * process.  That is, we fork and exec it.
 *
 * pArgVec contains a vector of SG_string pointers; one for each
 * argument.  This vector DOES NOT contain the program name in [0].
 * We will take care of that if it is appropriate for the platform
 * when we build the "char ** argv".
 *
 * pArgVec is optional, if you don't have any arguments, you
 * may pass NULL.
 *
 * pFileStdIn, pFileStdOut, and pFileStdErr point to opened files that
 * will be bound to the STDIN, STDOUT, and STDERR of the subordinate process.
 * These files should be opened for reading/writing as is
 * appropriate and positioned correctly.
 *
 * If these are NULL, the child will borrow the STDIN, STDOUT, and STDERR
 * of the current process (if that is possible on this platform).
 * NOTE: setting these to NULL may not make sense in a GUI app
 * since there probably isn't anyone watching them.
 *
 * If the child process is successfully started and it terminates
 * normally (with or without error), our return value is SG_ERR_OK
 * and the child's exit status is returned in pChildExitStatus.
 *
 * If the child crashes, we throw the details.
 *
 */
void SG_exec__exec_sync__files(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_exit_status * pChildExitStatus);

/**
 * Call this version if you want to micro-manage the exit-vs-crash-vs-whatever.
 *
 * We return an EXEC_RESULT for the child which indicates whether
 * we tried to start it and/or whether it crashed, or exited normally.
 * If we could not start it or it crashed, the pChildExitStatus is
 * undefined (since it doesn't have one).
 */
void SG_exec__exec_sync__files__details(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_exit_status * pChildExitStatus,
	SG_exec_result * per,
	SG_process_id * pPid);

/**
 * Exec the given command with the given arguments in a background
 * process.  That is, we fork and exec it.  We do not wait for the return status.
 *
 * pArgVec contains a vector of SG_string pointers; one for each
 * argument.  This vector DOES NOT contain the program name in [0].
 * We will take care of that if it is appropriate for the platform
 * when we build the "char ** argv".
 *
 * pArgVec is optional, if you don't have any arguments, you
 * may pass NULL.
 *
 * pFileStdIn, pFileStdOut, and pFileStdErr point to opened files that
 * will be bound to the STDIN, STDOUT, and STDERR of the subordinate process.
 * These files should be opened for reading/writing as is
 * appropriate and positioned correctly.
 *
 * If these are NULL, the child will borrow the STDIN, STDOUT, and STDERR
 * of the current process (if that is possible on this platform).
 * NOTE: setting these to NULL may not make sense in a GUI app
 * since there probably isn't anyone watching them.
 *
 * If the child process is successfully started our return value is SG_ERR_OK.
 *
 * If we cannot fork(), the child cannot be started with exec(),
 * or the child abnormally terminates (gets signaled, dumps core,
 * etc), we return an error code (and we do not set the child's
 * exit status).
 */
void SG_exec__exec_async__files(
	SG_context* pCtx,
	const char * pCmd,
	const SG_exec_argvec * pArgVec,
	SG_file * pFileStdIn,
	SG_file * pFileStdOut,
	SG_file * pFileStdErr,
	SG_process_id * pProcessID);

/**
 * Call this to query the exit status of a process spawned with an
 * exec_async call. This function will not block.

On Mac and Linux:
-----------------
 * If the process has not yet terminated, *pChildResult will be set to
 * SG_EXEC_RESULT__UNKNOWN.
 *
 * If the process exited normally, *pChildResult will be set to
 * SG_EXEC_RESULT__NORMAL, and the exit status will be stored in the
 * optional *pChildExitStatus, if provided.
 *
 * If the process was terminated abnormally (ie by a signal), *pChildResult
 * will be set to a bitwise or of SG_EXEC_RESULT__ABNORMAL and the
 * signal. It will be set with SG_EXEC_RESULT__SET_ABNORMAL() and the
 * signal can be retreived using SG_EXEC_RESULT__GET_SUB_VALUE().

On Windows:
-----------
 * If the process is still running, *pChildResult will be set to
 * SG_EXEC_RESULT__UNKNOWN.
 * 
 * If the process is not still running, *pChildResult will be set to
 * SG_EXEC_RESULT__NORMAL and *pChildExitStatus will be set to 0, regardless
 * of how the process terminated or what its exit status was.
 */
void SG_exec__async_process_result(
    SG_context* pCtx,
    SG_process_id processID,
    SG_exec_result * pChildResult,
    SG_exit_status * pChildExitStatus);

/**
 * Execute the specified command using system().
 * 
 * The exit status of the child process must be checked manually, its error
 * state isn't stuffed into pCtx such that the usual SG_ERR_CHECK stuff would
 * catch it.
 */
void SG_exec__system(
	SG_context* pCtx,
	const char* pszCmdLine,
	SG_exit_status* pExitStatus);

/**
 * Convenience wrapper around SG_exec__build_command and SG_exec__system.
 */
void SG_exec__system__argvec(
	SG_context*           pCtx,       //< [in] [out] Error and context info.
	const char*           szFilename, //< [in] Executable filename to run.
	const SG_exec_argvec* pArgs,      //< [in] List of arguments to pass to szFilename.
	SG_exit_status*       pExit       //< [out] Exit status returned by szFilename.
	);

/**
 * Builds a single shell command from an executable filename and list of
 * arguments.  Uses SG_exec__escape_arg on each argument to ensure proper
 * escaping for the shell.
 */
void SG_exec__build_command(
	SG_context*           pCtx,       //< [in] [out] Error and context info.
	const char*           szFilename, //< [in] Executable filename the command should run.
	const SG_exec_argvec* pArgs,      //< [in] Arguments the command should pass to szFilename.
	                                  //<      NULL is interpreted as an empty list.
	SG_string**           ppCommand   //< [out] The built command.
	);

/**
 * Escapes a string for use as an argument on a shell command line.  Properly
 * handles spaces and other special characters to make sure the shell interprets
 * the escaped string to have the same value as the original unescaped string.
 */
void SG_exec__escape_arg(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	const char* szArg,    //< [in] The string to escape.
	SG_string** ppEscaped //< [out] The escaped string, owned by caller.
	);

//////////////////////////////////////////////////////////////////

#if 1 && defined(DEBUG)
#if defined(MAC) || defined(LINUX)

#define HAVE_EXEC_DEBUG_STACKTRACE

void SG_exec_debug__get_stacktrace(SG_context * pCtx,
								   const char * pszPathToExec,
								   SG_process_id pid);
#endif
#endif

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_EXEC_PROTOTYPES_H
