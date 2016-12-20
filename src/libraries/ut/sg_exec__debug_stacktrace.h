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

//////////////////////////////////////////////////////////////////
// some debug routines to catch a crashing child process and
// try to do a post-mortem using the coredump.
//////////////////////////////////////////////////////////////////

#if defined(HAVE_EXEC_DEBUG_STACKTRACE)
#if defined(MAC)
void SG_exec_debug__get_stacktrace(SG_context * pCtx,
								   const char * pszPathToExec,
								   SG_process_id pid)
{
	SG_string * pStringCore = NULL;
	SG_pathname * pPathCore = NULL;
	SG_tempfile * pTempFileScript = NULL;
	SG_exec_argvec * pArgVec = NULL;
	SG_tempfile * pTempStdOut = NULL;
	SG_tempfile * pTempStdErr = NULL;
	SG_string * pStringErr = NULL;
	SG_string * pStringOut = NULL;
	SG_exit_status gdbExitStatus;
	SG_exec_result gdbExecResult;
	SG_process_id pidGdb;
	SG_bool bFound;

	// on the MAC they put all the core files in /cores
	// (if you have 'ulimit -c unlimited' set in your
	// environment).

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStringCore, "/cores/core.%d", (SG_uint32)pid)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathCore, SG_string__sz(pStringCore))  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathCore, &bFound, NULL, NULL)  );
	if (bFound)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Possible core file found: %s\n",
								   SG_pathname__sz(pPathCore))  );

		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFileScript)  );
		SG_ERR_CHECK(  SG_file__write__sz(pCtx,
										  pTempFileScript->pFile,
										  ("bt full\n"		// consider "thread apply all bt full\n"
										   "quit\n"))  );
		SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFileScript)  );

		SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx, &pArgVec)  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-batch")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-e")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, pszPathToExec)  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-c")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathCore))  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-x")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pTempFileScript->pPathname))  );

		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempStdOut)  );
		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempStdErr)  );

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Running post-mortem:\n")  );

		SG_ERR_CHECK(  SG_exec__exec_sync__files__details(pCtx,
														  "/usr/bin/gdb", pArgVec,
														  NULL, pTempStdOut->pFile, pTempStdErr->pFile,
														  &gdbExitStatus, &gdbExecResult, &pidGdb)  );
		SG_ERR_CHECK( SG_file__fsync(pCtx, pTempStdOut->pFile)   );
		SG_ERR_CHECK( SG_file__fsync(pCtx, pTempStdErr->pFile)   );

		SG_ERR_CHECK( SG_file__seek(pCtx, pTempStdOut->pFile, 0)   );
		SG_ERR_CHECK( SG_file__seek(pCtx, pTempStdErr->pFile, 0)   );

		SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pStringErr) );
		SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pStringOut) );
		SG_ERR_IGNORE(  SG_file__read_utf8_file_into_string(pCtx, pTempStdErr->pFile, pStringErr)  );	// allow this to fail incase the output is huge
		SG_ERR_IGNORE(  SG_file__read_utf8_file_into_string(pCtx, pTempStdOut->pFile, pStringOut)  );	// allow this to fail incase the output is huge

		SG_ERR_CHECK( SG_tempfile__close_and_delete(pCtx, &pTempStdErr) );
		SG_ERR_CHECK( SG_tempfile__close_and_delete(pCtx, &pTempStdOut) );

		if (SG_string__length_in_bytes(pStringOut) > 0)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   ("GDB stdout:\n"
										"================================\n"
										"%s\n"
										"================================\n"),
									   SG_string__sz(pStringOut))  );
		if (SG_string__length_in_bytes(pStringErr) > 0)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   ("GDB stderr:\n"
										"================================\n"
										"%s\n"
										"================================\n"),
									   SG_string__sz(pStringErr))  );
	}
	else
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Did not find expected core file: %s\n",
								   SG_pathname__sz(pPathCore))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringCore);
	SG_PATHNAME_NULLFREE(pCtx, pPathCore);
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempStdErr) );
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempStdOut) );
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempFileScript) );
	SG_STRING_NULLFREE(pCtx, pStringOut);
	SG_STRING_NULLFREE(pCtx, pStringErr);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
}
#endif

#if defined(LINUX)
void SG_exec_debug__get_stacktrace(SG_context * pCtx,
								   const char * pszPathToExec,
								   SG_process_id pid)
{
	SG_string * pStringCore = NULL;
	SG_pathname * pPathCore = NULL;
	SG_tempfile * pTempFileScript = NULL;
	SG_exec_argvec * pArgVec = NULL;
	SG_tempfile * pTempStdOut = NULL;
	SG_tempfile * pTempStdErr = NULL;
	SG_string * pStringErr = NULL;
	SG_string * pStringOut = NULL;
	SG_exit_status gdbExitStatus;
	SG_exec_result gdbExecResult;
	SG_process_id pidGdb;
	SG_bool bFound;

	// By default on Linux, core files are not created.
	// use "ulimit -c unlimited" to enable them.
	// Also by default, core files are written in the
	// current directory of the process at the time of
	// the crash.  This makes it hard for us to find
	// the coredump, so run the following commands on
	// your system as root:
	//
	// # mkdir /cores
	// # chmod 777 /cores
	// # echo "/cores/core.%p" > /proc/sys/kernel/core_pattern
	//
	// this will cause all coredumps to be written to /cores
	// like on a Mac.

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStringCore, "/cores/core.%d", (SG_uint32)pid)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathCore, SG_string__sz(pStringCore))  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathCore, &bFound, NULL, NULL)  );
	if (bFound)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Possible core file found: %s\n",
								   SG_pathname__sz(pPathCore))  );

		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFileScript)  );
		SG_ERR_CHECK(  SG_file__write__sz(pCtx,
										  pTempFileScript->pFile,
										  ("bt full\n"		// consider "thread apply all bt full\n"
										   "quit\n"))  );
		SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFileScript)  );

		SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx, &pArgVec)  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-batch")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-c")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathCore))  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-x")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pTempFileScript->pPathname))  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, pszPathToExec)  );

		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempStdOut)  );
		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempStdErr)  );

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Running post-mortem:\n")  );

		SG_ERR_CHECK(  SG_exec__exec_sync__files__details(pCtx,
														  "/usr/bin/gdb", pArgVec,
														  NULL, pTempStdOut->pFile, pTempStdErr->pFile,
														  &gdbExitStatus, &gdbExecResult, &pidGdb)  );
		SG_ERR_CHECK( SG_file__fsync(pCtx, pTempStdOut->pFile)   );
		SG_ERR_CHECK( SG_file__fsync(pCtx, pTempStdErr->pFile)   );

		SG_ERR_CHECK( SG_file__seek(pCtx, pTempStdOut->pFile, 0)   );
		SG_ERR_CHECK( SG_file__seek(pCtx, pTempStdErr->pFile, 0)   );

		SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pStringErr) );
		SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pStringOut) );
		SG_ERR_IGNORE(  SG_file__read_utf8_file_into_string(pCtx, pTempStdErr->pFile, pStringErr)  );	// allow this to fail incase the output is huge
		SG_ERR_IGNORE(  SG_file__read_utf8_file_into_string(pCtx, pTempStdOut->pFile, pStringOut)  );	// allow this to fail incase the output is huge

		SG_ERR_CHECK( SG_tempfile__close_and_delete(pCtx, &pTempStdErr) );
		SG_ERR_CHECK( SG_tempfile__close_and_delete(pCtx, &pTempStdOut) );

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("GDB stdout:\n"
									"================================\n"
									"%s\n"
									"================================\n"),
								   SG_string__sz(pStringOut))  );
		if (SG_string__length_in_bytes(pStringErr) > 0)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   ("GDB stderr:\n"
										"================================\n"
										"%s\n"
										"================================\n"),
									   SG_string__sz(pStringErr))  );
	}
	else
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Did not find expected core file: %s\n",
								   SG_pathname__sz(pPathCore))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringCore);
	SG_PATHNAME_NULLFREE(pCtx, pPathCore);
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempStdErr) );
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempStdOut) );
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempFileScript) );
	SG_STRING_NULLFREE(pCtx, pStringOut);
	SG_STRING_NULLFREE(pCtx, pStringErr);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
}
#endif
#endif
