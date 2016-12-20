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
 * @file u0085_crash.c
 *
 * @details A simple test to exercise SG_exec and confirm that
 * we can handle/debug a child process that crashes.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"

//////////////////////////////////////////////////////////////////
// we define a little trick here to prefix all global symbols (type,
// structures, functions) with our test name.  this allows all of
// the tests in the suite to be #include'd into one meta-test (without
// name collisions) when we do a GCOV run.

#define MyMain()				TEST_MAIN(u0085_crash)
#define MyDcl(name)				u0085_crash__##name
#define MyFn(name)				u0085_crash__##name

//////////////////////////////////////////////////////////////////

/**
 * This test requires the 'crash' command be defined/enabled in 'vv'.
 * This command will always take a SIGSEGV.
 */
void MyFn(test1)(SG_context * pCtx)
{
	SG_exit_status exitStatusChild;
	SG_exec_argvec * pArgVec = NULL;

	VERIFY_ERR_CHECK(  SG_exec_argvec__alloc(pCtx,&pArgVec)  );
	VERIFY_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx,pArgVec,"crash")  );

	// we use the easy version and only get back an exit status.
	// this should have caught the crash and tried to run gdb on
	// the coredump.  we didn't divert the gdb output, so we'll
	// just eyeball the test log for now.

	SG_exec__exec_sync__files(pCtx, "vv" ,pArgVec,NULL,NULL,NULL,&exitStatusChild);

	VERIFY_CTX_ERR_EQUALS("child status", pCtx, SG_ERR_ABNORMAL_TERMINATION);
	SG_context__err_reset(pCtx);

fail:
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
}

//////////////////////////////////////////////////////////////////

MyMain()
{
	TEMPLATE_MAIN_START;

#if defined(HAVE_EXEC_DEBUG_STACKTRACE)
	BEGIN_TEST(  MyFn(test1)(pCtx)  );
#else
	INFOP("u0085_crash", ("Skipping crash stacktrace test....") );
#endif

	TEMPLATE_MAIN_END;
}

//////////////////////////////////////////////////////////////////

#undef MyMain
#undef MyDcl
#undef MyFn
