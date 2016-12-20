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

// testsuite/unittests/unittests.h
// Commonly used declations for unit tests.
//////////////////////////////////////////////////////////////////

#ifndef H_UNITTESTS_H
#define H_UNITTESTS_H

//////////////////////////////////////////////////////////////////

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

#define WC__GET_DEPTH(bRecursive)  ((bRecursive) ? SG_INT32_MAX : 0)

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
#	pragma warning ( disable : 4996 ) // disable compiler warnings for non _s versions of things like sprintf.
#	if defined(CODE_ANALYSIS)
#		pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS) // disable all the static analysis warnings in test code
#	endif
#endif

//////////////////////////////////////////////////////////////////

static int g_cPass = 0;
static int g_cFail = 0;
static int g_cSumPass = 0;
static int g_cSumFail = 0;

//////////////////////////////////////////////////////////////////

SG_DECLARE_PRINTF_PROTOTYPE(  const char * _my_sprintf_a(const char * szFormat, ...),  1, 2  );
SG_DECLARE_PRINTF_PROTOTYPE(  const char * _my_sprintf_b(const char * szFormat, ...),  1, 2  );

/**
 * print a nice separator label to mark the start of a test case within a test.
 *
 * this CANNOT use a SG_context.
 */
void _begin_test_label(const char * szFile, int lineNr,
					   const char * szTestName)
{
	fprintf(stderr,
			"\n"
			"\n"
			"//////////////////////////////////////////////////////////////////////////////////\n"
			"BeginTest  %s:%d %s\n",
			szFile,lineNr,
			szTestName);
}

/**
 * print an info message.
 *
 * this CANNOT use a SG_context.
 */
void _report_info(const char * szTestName,
				  const char * szFile, int lineNr,
				  const char * szInfo)
{
	fprintf(stderr,
			"Info       %s:%d [%s][%s]\n",
			szFile,lineNr,
			szTestName,
			szInfo);
}

/**
 * print "TestFailed..." message.
 *
 * this CANNOT use a SG_context.
 */
int _report_testfail(const char * szTestName,
					 const char * szFile, int lineNr,
					 const char * szReason)
{
	fprintf(stderr,
			"TestFailed %s:%d [%s][%s]\n",
			szFile,lineNr,
			szTestName,
			szReason);
	g_cFail++;
	g_cSumFail++;

	return 0;
}

/**
 * print "TestFailed..." message.
 *
 * this CANNOT use a SG_context.
 */
int _report_testfail2(const char * szTestName,
					  const char * szFile, int lineNr,
					  const char * szReason,
					  const char * szReason2)
{
	fprintf(stderr,
			"TestFailed %s:%d [%s][%s][%s]\n",
			szFile,lineNr,
			szTestName,
			szReason,
			szReason2);
	g_cFail++;
	g_cSumFail++;

	return 0;
}

/**
 * print "TestFailed..." message.
 *
 * this CANNOT use a SG_context.
 */
int _report_testfail3(const char * szTestName,
					  const char * szFile, int lineNr,
					  const char * szReason,
					  const char * szReason2,
					  const char * szReason3)
{
	fprintf(stderr,
			"TestFailed %s:%d [%s][%s][%s][%s]\n",
			szFile,lineNr,
			szTestName,
			szReason,
			szReason2,
			szReason3);
	g_cFail++;
	g_cSumFail++;

	return 0;
}

//////////////////////////////////////////////////////////////////

/**
 * print "TestFailed..." message when we also have an error number.
 *
 * this CANNOT use a SG_context.
 */
int _report_testfail_err(const char * szTestName,
						 const char * szFile, int lineNr,
						 SG_error err,
						 const char * szReason)
{
	char bufError[SG_ERROR_BUFFER_SIZE+1];

	SG_error__get_message(err,SG_TRUE,bufError,SG_ERROR_BUFFER_SIZE);

	fprintf(stderr,
			"TestFailed %s:%d [%s][%s] [error: %s]\n",
			szFile,lineNr,
			szTestName,
			szReason,
			bufError);
	g_cFail++;
	g_cSumFail++;

	return 0;
}

/**
 * print "TestFailed..." message when we also have an error number.
 *
 * this CANNOT use a SG_context.
 */
int _report_testfail2_err(const char * szTestName,
						  const char * szFile, int lineNr,
						  SG_error err,
						  const char * szReason,
						  const char * szReason2)
{
	char bufError[SG_ERROR_BUFFER_SIZE+1];

	SG_error__get_message(err,SG_TRUE,bufError,SG_ERROR_BUFFER_SIZE);

	fprintf(stderr,
			"TestFailed %s:%d [%s][%s][%s] [error: %s]\n",
			szFile,lineNr,
			szTestName,
			szReason,
			szReason2,
			bufError);
	g_cFail++;
	g_cSumFail++;

	return 0;
}

//////////////////////////////////////////////////////////////////

/**
 * print "TestFailed..." message and detailed info from pCtx.
 */
int _report_testfail_ctx(SG_context * pCtx,
						 const char * szTestName,
						 const char * szFile, int lineNr,
						 const char * szReason)
{
	fprintf(stderr,
			"TestFailed %s:%d [%s][%s]\n",
			szFile,lineNr,
			szTestName,
			szReason);

	// we assume that this never changes pCtx.
	(void) SG_context__err_to_console(pCtx, SG_CS_STDERR);

	g_cFail++;
	g_cSumFail++;

	return 0;
}

/**
 * print "TestFailed..." message and detailed info from pCtx.
 */
int _report_testfail2_ctx(SG_context * pCtx,
						  const char * szTestName,
						  const char * szFile, int lineNr,
						  const char * szReason,
						  const char * szReason2)
{
	fprintf(stderr,
			"TestFailed %s:%d [%s][%s][%s]\n",
			szFile,lineNr,
			szTestName,
			szReason,
			szReason2);

	// we assume that this never changes pCtx.
	(void) SG_context__err_to_console(pCtx, SG_CS_STDERR);

	g_cFail++;
	g_cSumFail++;

	return 0;
}

//////////////////////////////////////////////////////////////////

void _incr_pass(void)
{
	g_cPass++;
	g_cSumPass++;
}

/**
 * Verify that pCtx contains SG_ERR_OK.
 * If not, dump the current error information.
 *
 * Use this when you expect to have an OK result.
 */
int _verify_ctx_is_ok(const char * szName, SG_context * pCtx, const char * szFile, int lineNr, const char * szReason2)
{
	if (!SG_context__has_err(pCtx))
	{
		_incr_pass();
		return 1;
	}
	else
	{
		SG_context__err_stackframe_add(pCtx,szFile,lineNr);
		if (szReason2 && *szReason2)
			_report_testfail2_ctx(pCtx,szName,szFile,lineNr,"VERIFY_CTX_IS_OK",szReason2);
		else
			_report_testfail_ctx(pCtx,szName,szFile,lineNr,"VERIFY_CTX_IS_OK");

		return 0;
	}
}

/**
 * Verify that pCtx contains some (any) error.
 * If not, we do not dump the pCtx because it doesn't contain anything.
 */
int _verify_ctx_has_err(const char * szName, SG_context * pCtx, const char * szFile, int lineNr, const char * szReason2)
{
	if (SG_context__has_err(pCtx))
	{
		_incr_pass();
		return 1;
	}
	else
	{
		// like a throw2

		SG_context__err__generic(pCtx, SG_ERR_ASSERT, szFile, lineNr);
		SG_context__err_set_description(pCtx, "%s: No error when an error was expected: %s",
										szName, ((szReason2) ? szReason2 : "unspecified"));
		_report_testfail(szName,szFile,lineNr,"VERIFY_CTX_HAS_ERR expected an error but was OK");
		return 0;
	}
}

/**
 * Verify that pCtx contains *exactly* the given error value.
 * If now, we print the expected value and the observed value
 * (and if the observed value is an error, any error info we have).
 */
int _verify_ctx_err_equals(const char * szName,
						   SG_context * pCtx, SG_error errExpected,
						   const char * szFile, int lineNr,
						   const char * szReason2)
{
	SG_error errObserved;

	SG_context__get_err(pCtx,&errObserved);
	if (errObserved == errExpected)
	{
		_incr_pass();
		return 1;
	}
	else
	{
		char bufError_expected[SG_ERROR_BUFFER_SIZE+1];
		char bufError_observed[SG_ERROR_BUFFER_SIZE+1];
		const char * szReason1;

		SG_error__get_message(errExpected,SG_TRUE,bufError_expected,SG_ERROR_BUFFER_SIZE);
		if (SG_IS_ERROR(errObserved))
			SG_error__get_message(errObserved,SG_TRUE,bufError_observed,SG_ERROR_BUFFER_SIZE);
		else
			SG_ERR_IGNORE(  SG_strcpy(pCtx, bufError_observed, SG_ERROR_BUFFER_SIZE, "No Error")  );

		szReason1 = _my_sprintf_b("VERIFY_CTX_ERR_EQUALS [expected: %s][observed: %s]",
								  bufError_expected,
								  bufError_observed);

		if (SG_IS_ERROR(errObserved))
			SG_context__err_stackframe_add(pCtx,szFile,lineNr);
		else
			SG_context__err__generic(pCtx, SG_ERR_UNSPECIFIED, szFile, lineNr);

		if (szReason2 && *szReason2)
			_report_testfail2_ctx(pCtx,szName,szFile,lineNr,szReason1,szReason2);
		else
			_report_testfail_ctx(pCtx,szName,szFile,lineNr,szReason1);

		return 0;
	}
}

//////////////////////////////////////////////////////////////////

#define MAX_MESSAGE		20480

/**
 * inline version of sprintf that can be used inside VERIFY* macros.
 *
 * YOU CANNOT USE THIS TWICE INSIDE ONE MACRO (because of the static buffer).
 *
 * this CANNOT use a SG_context.
 */
const char * _my_sprintf_a(const char * szFormat, ...)
{
	// not thread-safe

	static char buf[MAX_MESSAGE];
	va_list ap;

	va_start(ap,szFormat);

	memset(buf,0,MAX_MESSAGE);

#if defined(WINDOWS)
	vsnprintf_s(buf,MAX_MESSAGE-1,_TRUNCATE,szFormat,ap);
#else
	vsnprintf(buf,MAX_MESSAGE-1,szFormat,ap);
#endif

	va_end(ap);

	return buf;
}

/**
 * inline version of sprintf that can be used inside VERIFY* macros.
 *
 * YOU CANNOT USE THIS TWICE INSIDE ONE MACRO (because of the static buffer).
 *
 * this CANNOT use a SG_context.
 */
const char * _my_sprintf_b(const char * szFormat, ...)
{
	// not thread-safe

	static char buf[MAX_MESSAGE];
	va_list ap;

	va_start(ap,szFormat);

	memset(buf,0,MAX_MESSAGE);

#if defined(WINDOWS)
	vsnprintf_s(buf,MAX_MESSAGE-1,_TRUNCATE,szFormat,ap);
#else
	vsnprintf(buf,MAX_MESSAGE-1,szFormat,ap);
#endif

	va_end(ap);

	return buf;
}

/**
 * write results to STDOUT and STDERR and prepare for EXIT.
 *
 * this CANNOT use a SG_context.
 */
int _report_and_exit(const char * szProgramName)
{
	// we write results to both output streams because
	// the makefile diverts one to a file and one to the
	// console.
	// TODO this ends up with two copies of everything in the cmake logfile

	int bFailed = (g_cSumFail > 0);			// true if there were *any* errors
	int len = (int)strlen(szProgramName);
	int i;

	fprintf(stdout, "%s", szProgramName);
	fprintf(stderr, "%s", szProgramName);
	for (i=0; i<(40-len); i++)
	{
		fputc(' ', stdout);
		fputc(' ', stderr);
	}
	fprintf(stdout, "%6d passed", g_cSumPass);
	fprintf(stderr, "%6d passed", g_cSumPass);
	if (g_cSumFail > 0)
	{
		fprintf(stdout, "     %6d FAIL", g_cSumFail);	// overall error count (whole program)
		fprintf(stderr, "     %6d FAIL", g_cSumFail);
	}
	fputc('\n', stdout);
	fputc('\n', stderr);

	g_cPass = 0;
	g_cFail = 0;

	// we do not reset g_cSumPass -- it is intented to be a cummulative success count.
	// we do not reset g_cSumFail -- it is intented to be a cummulative error count.

#ifdef DEBUG
	if (SG_mem__check_for_leaks())
	{
		bFailed = 1;
	}
#endif

	return bFailed;
}

/**
 * Write results for 1 of u1000_repo_script's scripts to STDOUT and STDERR.
 * We also include info for any error recorded in pCtx.
 *
 * We DO NOT throw/report any errors, so pCtx is READONLY.
 */
void _report_and_exit_script(const char * szProgramName, const char * szScriptName, const SG_context * pCtx)
{
	// we write results to both output streams because
	// the makefile diverts one to a file and one to the
	// console.
	// TODO this ends up with two copies of everything in the cmake logfile
	//
	// here we write the error count for a single script.

	int len = (int)strlen(szProgramName) + 1 + (int)strlen(szScriptName);
	int i;

	fprintf(stdout, "%s:%s", szProgramName,szScriptName);
	fprintf(stderr, "%s:%s", szProgramName,szScriptName);
	for (i=0; i<(50-len); i++)
	{
		fputc(' ', stdout);
		fputc(' ', stderr);
	}
	fprintf(stdout, "%6d passed", g_cPass);
	fprintf(stderr, "%6d passed", g_cPass);
	if (g_cFail > 0)
	{
		fprintf(stdout, "     %6d FAIL", g_cFail);		// error count for this script
		fprintf(stderr, "     %6d FAIL", g_cFail);
	}
	if (SG_context__has_err(pCtx))
	{
		SG_error err;
		char bufError[SG_ERROR_BUFFER_SIZE+1];

		SG_context__get_err(pCtx,&err);
		SG_error__get_message(err,SG_TRUE,bufError,SG_ERROR_BUFFER_SIZE);
		fprintf(stdout, " %s",bufError);
		fprintf(stderr, " %s",bufError);
	}
	fputc('\n', stdout);
	fputc('\n', stderr);

	g_cPass = 0;
	g_cFail = 0;

	// we do not reset g_cSumPass -- it is intented to be a cummulative success count.
	// we do not reset g_cSumFail -- it is intented to be a cummulative error count.

	// TODO do we want to check for leaks here or not?
	// TODO Jeff: no because the script driver may have memory allocated.
}

//////////////////////////////////////////////////////////////////

#ifndef SG_STATEMENT
#define SG_STATEMENT( s )			do { s } while (0)
#endif

//////////////////////////////////////////////////////////////////
// INFO() -- output generic info into report that's unassociated
// with a pass/fail.

#define INFO2(name,info)				SG_STATEMENT( _report_info((name),__FILE__,__LINE__, (info)); )
#define INFOP(name,va)					SG_STATEMENT( _report_info((name),__FILE__,__LINE__, _my_sprintf_a va); )

//////////////////////////////////////////////////////////////////

// VERIFY_CTX_{OK,ERR,ERR_IS}() verify that pCtx is OK or ERROR or a specific ERROR.
// VERIFYP_CTX_{OK,ERR,ERR_IS}() do the same, but also allow a custom sprintf-message to be recorded.
//
// The _OK version is used when we think the test should succeed; we print the info from the pCtx.
// The _ERR and _ERR_IS versions only print the message
// Bad results write the "TestFailed..." messages seen in stderr.
// Execution continues afterward.
//
// You probably want to do a SG_context__reset(pCtx) after the {ERR,ERR_IS} forms.

#define VERIFY_CTX_IS_OK(name, pCtx)                           _verify_ctx_is_ok((name),(pCtx),__FILE__,__LINE__,NULL)
#define VERIFY_CTX_HAS_ERR(name, pCtx)                         _verify_ctx_has_err((name),(pCtx),__FILE__,__LINE__,NULL)
#define VERIFY_CTX_ERR_EQUALS(name, pCtx, errExpected)         _verify_ctx_err_equals((name),(pCtx),(errExpected),__FILE__,__LINE__,NULL)

#define VERIFYP_CTX_IS_OK(name, pCtx, va)                      _verify_ctx_is_ok((name),(pCtx),__FILE__,__LINE__, _my_sprintf_a va)
#define VERIFYP_CTX_HAS_ERR(name, pCtx, va)                    _verify_ctx_has_err((name),(pCtx),__FILE__,__LINE__, _my_sprintf_a va)
#define VERIFYP_CTX_ERR_EQUALS(name, pCtx, errExpected, va)    _verify_ctx_err_equals((name),(pCtx),(errExpected),__FILE__,__LINE__, _my_sprintf_a va)


// VERIFY_ERR_CHECK*() -- like SG_ERR_CHECK() and SG_ERR_CHECK_RETURN() but with logging.
// _CHECK    logs and jumps to FAIL
// _DISCARD  logs and RESETS the ctx and allows the code to CONTINUE
// _RETURN   logs and RETURNS

#define VERIFY_ERR_CHECK(expr)			SG_STATEMENT( SG_ASSERT(!SG_context__has_err(pCtx)); expr; if (0 == _verify_ctx_is_ok("VERIFY_ERR_CHECK",        pCtx,__FILE__,__LINE__,#expr)) { goto fail;                   } )
#define VERIFY_ERR_CHECK_DISCARD(expr)  SG_STATEMENT( SG_ASSERT(!SG_context__has_err(pCtx)); expr; if (0 == _verify_ctx_is_ok("VERIFY_ERR_CHECK_DISCARD",pCtx,__FILE__,__LINE__,#expr)) { SG_context__err_reset(pCtx); } )
#define VERIFY_ERR_CHECK_RETURN(expr)	SG_STATEMENT( SG_ASSERT(!SG_context__has_err(pCtx)); expr; if (0 == _verify_ctx_is_ok("VERIFY_ERR_CHECK_RETURN", pCtx,__FILE__,__LINE__,#expr)) { return 0;                    } )

// assert that pCtx has an error and reset the context.
#define VERIFY_ERR_CHECK_HAS_ERR_DISCARD(expr)	SG_STATEMENT( SG_ASSERT(!SG_context__has_err(pCtx)); expr; _verify_ctx_has_err("VERIFY_ERR_CHECK_HAS_ERR_DISCARD",pCtx,__FILE__,__LINE__,#expr); SG_context__err_reset(pCtx); )

// assert that pCtx has the named error.
#define VERIFY_ERR_CHECK_ERR_EQUALS(expr,errExpected)			SG_STATEMENT( SG_ASSERT(!SG_context__has_err(pCtx)); expr; if (0 == _verify_ctx_err_equals("VERIFY_ERR_CHECK_ERR_EQUALS",        pCtx,(errExpected),__FILE__,__LINE__,#expr)) { goto fail;                   } )
#define VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(expr,errExpected)	SG_STATEMENT( SG_ASSERT(!SG_context__has_err(pCtx)); expr;          _verify_ctx_err_equals("VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD",pCtx,(errExpected),__FILE__,__LINE__,#expr);   SG_context__err_reset(pCtx);   )
#define VERIFY_ERR_CHECK_ERR_EQUALS_RETURN(expr,errExpected)	SG_STATEMENT( SG_ASSERT(!SG_context__has_err(pCtx)); expr; if (0 == _verify_ctx_err_equals("VERIFY_ERR_CHECK_ERR_EQUALS_RETURN", pCtx,(errExpected),__FILE__,__LINE__,#expr)) { return 0;                    } )

// VERIFY*_COND()        -- verify the condition, log the error and continue
// VERIFY*_COND_RETURN() -- verify the condition, log the error and RETURN if it's false.

#define VERIFY_COND(name,cond)                  SG_STATEMENT( if ((cond)) { g_cPass++; g_cSumPass++; } else        _report_testfail ((name),__FILE__,__LINE__, #cond); )
#define VERIFY_COND_FAIL(name,cond)             SG_STATEMENT( if ((cond)) { g_cPass++; g_cSumPass++; } else {      _report_testfail ((name),__FILE__,__LINE__, #cond); goto fail; } )
#define VERIFY_COND_RETURN(name,cond)			SG_STATEMENT( if ((cond)) { g_cPass++; g_cSumPass++; } else return _report_testfail ((name),__FILE__,__LINE__, #cond); )

#define VERIFYP_COND(name,cond,va)              SG_STATEMENT( if ((cond)) { g_cPass++; g_cSumPass++; } else        _report_testfail2((name),__FILE__,__LINE__, #cond, _my_sprintf_a va); )
#define VERIFYP_COND_FAIL(name,cond,va)         SG_STATEMENT( if ((cond)) { g_cPass++; g_cSumPass++; } else {      _report_testfail2((name),__FILE__,__LINE__, #cond, _my_sprintf_a va); goto fail; } )
#define VERIFYP_COND_RETURN(name,cond,va)       SG_STATEMENT( if ((cond)) { g_cPass++; g_cSumPass++; } else return _report_testfail2((name),__FILE__,__LINE__, #cond, _my_sprintf_a va); )


// If you don't use this function, #define the following before including this header
// (To surpress gcc warning "warning: ‘_sg_unittests__extract_data_dir_from_args’ defined but not used")
#ifndef OMIT_EXTRACT_DATA_DIR_FROM_ARGS_FUNCTION

#if defined(WINDOWS)
static void _sg_unittests__extract_data_dir_from_args(SG_context * pCtx, int argc, wchar_t ** argv, SG_pathname ** ppDataDir)
#else
static void _sg_unittests__extract_data_dir_from_args(SG_context * pCtx, int argc, char ** argv, SG_pathname ** ppDataDir)
#endif
{
    SG_getopt * pGetopt = NULL;
    SG_pathname * pDataDir = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_ARGCHECK_RETURN(argc==2, argc);
    SG_NULLARGCHECK_RETURN(argv);
    SG_NULLARGCHECK_RETURN(ppDataDir);

    SG_ERR_CHECK(  SG_getopt__alloc(pCtx, argc, argv, &pGetopt)  );
    SG_ARGCHECK(pGetopt->count_args==2, argv);
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pDataDir, pGetopt->paszArgs[1])  );
    SG_GETOPT_NULLFREE(pCtx, pGetopt);

    *ppDataDir = pDataDir;

    return;
fail:
    SG_PATHNAME_NULLFREE(pCtx, pDataDir);
    SG_GETOPT_NULLFREE(pCtx, pGetopt);
}
#endif


//////////////////////////////////////////////////////////////////
// BEGIN_TEST -- log that we are starting the test and then run it.

#define BEGIN_TEST(s)			SG_STATEMENT(  _begin_test_label(__FILE__,__LINE__, #s);  SG_context__err_reset(pCtx); s; VERIFY_COND("test failed with a dirty context", !SG_context__has_err(pCtx)); SG_context__err_reset(pCtx);  )

//////////////////////////////////////////////////////////////////
// TEST_MAIN() -- define main() to allow us to do some tricks --
// run test as standalone test or as part of a group (for code
// coverage purposes).

#ifdef GCOV_TEST
#define TEST_MAIN(fn) int fn(SG_context * pCtx, const SG_pathname * pDataDir)
#else
#if defined(WINDOWS)
#define TEST_MAIN(fn) int wmain(int argc, wchar_t ** argv)
#else
#define TEST_MAIN(fn) int main(int argc, char ** argv)
#endif
#endif

//////////////////////////////////////////////////////////////////
// TEST_MAIN_WITH_ARGS() -- define main() to allow us to do some tricks --
// run test as standalone test or as part of a group (for code
// coverage purposes).

#ifdef GCOV_TEST
#define TEST_MAIN_WITH_ARGS(fn)		int fn(int argc, char**argv)
#else
#if defined(WINDOWS)
#define TEST_MAIN_WITH_ARGS(fn) int wmain(int argc, wchar_t ** argv)
#else
#define TEST_MAIN_WITH_ARGS(fn) int main(int argc, char ** argv)
#endif
#endif

//////////////////////////////////////////////////////////////////
// The following templates should be at the top and bottom of the
// body of the TEST_MAIN() function in each unit test (possibly with
// the exception of u0063_error_context (which has special stuff to
// test the SG_context code)).

#ifdef GCOV_TEST
#define TEMPLATE_MAIN_START SG_UNUSED(pDataDir); // Not necessarily unused...
#else
#define TEMPLATE_MAIN_START									\
	SG_context * pCtx = NULL;								\
	SG_pathname * pDataDir = NULL;							\
	SG_context__alloc(&pCtx);								\
	VERIFY_ERR_CHECK_RETURN(  SG_lib__global_initialize(pCtx)  );	\
	SG_ERR_IGNORE(  _sg_unittests__extract_data_dir_from_args(pCtx, argc, argv, &pDataDir)  );
#endif

#ifdef GCOV_TEST
#define TEMPLATE_MAIN_END return (_report_and_exit(__FILE__));
#else
#define TEMPLATE_MAIN_END							\
	SG_PATHNAME_NULLFREE(pCtx, pDataDir);			\
	SG_lib__global_cleanup(pCtx);					\
	SG_CONTEXT_NULLFREE(pCtx);						\
	return (_report_and_exit(__FILE__));
#endif

//////////////////////////////////////////////////////////////////

int unittest__alloc_unique_pathname_in_cwd(SG_context * pCtx,
										   SG_pathname ** ppPathname)
{
	// allocate a pathname containing a non-existent string.
	// caller can use the result to generate a temp file or directory.
	//
	// caller must free returned pathname.

	char buf[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname * p = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx,&p)  );
	VERIFY_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx,p)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx,buf,sizeof(buf), 32)  );

	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,p,buf)  );

	*ppPathname = p;
	return 1;

fail:
	SG_PATHNAME_NULLFREE(pCtx, p);
	return 0;
}

int unittest__alloc_unique_pathname(SG_context * pCtx,
									const char * utf8Path,
									SG_pathname ** ppPathname)
{
	// allocate a pathname containing a non-existent string.
	// caller can use the result to generate a temp file or directory.
	//
	// we assume, but do not verify, that the given path refers to something
	// that resembles a directory.  we do not verify that it exists on disk
	// either.
	//
	// caller must free returned pathname.

	char bufTid[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname * p = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx,&p)  );
	VERIFY_ERR_CHECK(  SG_pathname__set__from_sz(pCtx,p,utf8Path)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx,bufTid,sizeof(bufTid), 32)  );

	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,p,bufTid)  );

	*ppPathname = p;
	return 1;

fail:
	SG_PATHNAME_NULLFREE(pCtx, p);
	return 0;
}

int unittest__alloc_unique_pathname_in_dir(SG_context * pCtx,
										   const char * szParentDirectory,
										   SG_pathname ** ppPathnameUnique)
{
	// allocate a pathname containing a non-existent string.
	// this is something of the form <parent_directory>/<tid>
	//
	// caller can use the result to generate a temp file or directory.
	//
	// caller must free returned pathname.

	SG_pathname * p = NULL;
	char bufTid[SG_TID_MAX_BUFFER_LENGTH];

	VERIFY_ERR_CHECK_RETURN(  SG_PATHNAME__ALLOC__SZ(pCtx,&p,szParentDirectory)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx,bufTid,sizeof(bufTid), 32)  );

	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,p,bufTid)  );

	*ppPathnameUnique = p;
	return 1;

fail:
	SG_PATHNAME_NULLFREE(pCtx, p);
	return 0;
}

void unittest__get_nonexistent_pathname(SG_context * pCtx, SG_pathname ** ppPathname)
{
	// TODO we now have 4 routines abstracted from the various u00xx tests
	// TODO that all do pretty much the same thing.  (and a bunch of ones
	// TODO in the individual tests that were cloned when a new test was
	// TODO created.)
	// TODO
	// TODO at some point, we should consolidate these....

	unittest__alloc_unique_pathname_in_cwd(pCtx,ppPathname);
}

//////////////////////////////////////////////////////////////////

/**
 * Are we ignoring our backup files?  That is, is there a value in
 * "ignores" in "localsettings" that would cause add/remove/status
 * commands to ignore (not report) our "~sg00~" suffixed files?
 */
void unittest__get__ignore_backup_files(SG_context * pCtx, const SG_pathname * pPathWorkingDir, SG_bool * pbIgnore)
{
	static const char * pszTestIgnorable = "foo.c~sg00~";

	SG_wc_status_flags statusFlags;
	SG_pathname * pPath_old_cwd = NULL;

	// The PendingTree version of this test took pPathWorkingDir and
	// could inspect any WD without regard for the location of our CWD.
	// The WC version assumes that the CWD is inside the WD.  So we 'cd'
	// in for the duration of this operation.

	SG_ERR_CHECK(  SG_pathname__alloc(pCtx, &pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPathWorkingDir)  );

	SG_ERR_CHECK(  SG_wc__get_item_status_flags(pCtx, NULL, pszTestIgnorable, SG_FALSE, SG_FALSE, &statusFlags, NULL)  );
	*pbIgnore = ((statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED) != 0);
	
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPath_old_cwd)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_old_cwd);
}

/**
 * Turn on/off the ignoring of our "~sg00~" backup files in add/remove/status.
 */
void unittests__set__ignore_backup_files(SG_context * pCtx, const SG_pathname * pPathWorkingDir, SG_bool bIgnore)
{
	SG_pathname * pPath_sgignores = NULL;
	SG_file * pFile = NULL;
	SG_bool bCurrent;
	const char * pszPattern;

	// TODO 2012/07/05 I'm going to assume that the passed in directory is the WD top.
	// TODO            If not, 'cd' into the given directory and then call SG_wc__get_wc_top()
	// TODO            to get the actual root of the WD.

	// add a line to "@/.sgignores" to ignore/not-ignore our backup files.
	// 
	// always write a .sgignores file (even if the pattern is already
	// present in a lower scope) so that our tests are consistent
	// (so that .sgignores will appear as an ADDED file consistently).

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_sgignores, pPathWorkingDir, "@/.sgignores")  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_sgignores, SG_FILE_OPEN_OR_CREATE | SG_FILE_TRUNC | SG_FILE_RDWR, 0600, &pFile)  );

	if (bIgnore)
		pszPattern = "*~sg*~\n";
	else
		pszPattern = "-*~sg*~\n";
	
	SG_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32)strlen(pszPattern), (SG_byte *)pszPattern, NULL)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);

	// confirm that we affected things.

	SG_ERR_CHECK_RETURN(  unittest__get__ignore_backup_files(pCtx, pPathWorkingDir, &bCurrent)  );
	VERIFYP_COND( "check set_ignore_backup_files", (bCurrent == bIgnore), ("[bIgnore %d][bCurrent %d]", bIgnore, bCurrent) );

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath_sgignores);
}

//////////////////////////////////////////////////////////////////
// The simple checkin/checkout tests and wrappers were written before
// we had named branches and the whole attached/detached concept.

#define UT__IS_DETACHED SG_FALSE
#define UT__ATTACH_TO   "master"

//////////////////////////////////////////////////////////////////

/**
 * Do a basic/trivial commit and don't bother the caller the the gazillion args.
 *
 */
void unittests_pendingtree__simple_commit(SG_context * pCtx,
										  const SG_pathname * pPathWorkingDir,
										  char ** ppsz_hid_cset)
{
	SG_wc_commit_args ca;
	memset(&ca, 0, sizeof(ca));
	ca.bDetached = UT__IS_DETACHED;
	ca.pszUser = NULL;		// SG_AUDIT__WHO__FROM_SETTINGS
	ca.pszWhen = NULL;		// SG_AUDIT__WHEN__NOW
	ca.pszMessage = "This is a comment.";
	ca.pfnPrompt = NULL;
	ca.psaInputs = NULL;	// a complete (non-partial) commit
	ca.depth = WC__GET_DEPTH(SG_TRUE);
	ca.psaAssocs = NULL;
	ca.bAllowLost = SG_FALSE;
	ca.psaStamps = NULL;
	// TODO 2011/11/03 deal with subrepo-related args.

	SG_ERR_CHECK(  SG_wc__commit(pCtx, pPathWorkingDir, &ca, SG_FALSE, NULL, ppsz_hid_cset)  );

fail:
	;
}

/**
 * Do a basic/trivial commit and don't bother the caller the the gazillion args.
 */
void unittests_pendingtree__simple_commit_with_args(SG_context * pCtx,
													const SG_pathname * pPathWorkingDir,
													SG_uint32 count_args, const char ** paszArgs,
													char ** ppsz_hid_cset)
{
	SG_stringarray * psaArgs = NULL;
	SG_wc_commit_args ca;

	SG_ERR_CHECK(  SG_util__convert_argv_into_stringarray(pCtx, count_args, paszArgs, &psaArgs)  );

	memset(&ca, 0, sizeof(ca));
	ca.bDetached = UT__IS_DETACHED;
	ca.pszUser = NULL;		// SG_AUDIT__WHO__FROM_SETTINGS
	ca.pszWhen = NULL;		// SG_AUDIT__WHEN__NOW
	ca.pszMessage = "This is a comment.";
	ca.pfnPrompt = NULL;
	ca.psaInputs = psaArgs;
	ca.depth = WC__GET_DEPTH(SG_TRUE);
	ca.psaAssocs = NULL;
	ca.bAllowLost = SG_FALSE;
	ca.psaStamps = NULL;
	// TODO 2011/11/03 deal with subrepo-related args.

	SG_ERR_CHECK(  SG_wc__commit(pCtx, pPathWorkingDir, &ca, SG_FALSE, NULL, ppsz_hid_cset)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

//////////////////////////////////////////////////////////////////

/**
 * Another wrapper for the same reason.
 */
void unittests_workingdir__create_and_checkout(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const SG_pathname* pPathDirPutTopLevelDirInHere,
    const char* psz_spec_hid_cs_baseline
	)
{
	// callers of this routine are probably not playing with submodules,
	// so we don't expect any submodule problems.

	SG_rev_spec * pRevSpec = NULL;

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_spec_hid_cs_baseline)  );

	SG_ERR_CHECK(  SG_wc__checkout(pCtx,
								   pszDescriptorName,
								   SG_pathname__sz(pPathDirPutTopLevelDirInHere),
								   pRevSpec,
								   UT__ATTACH_TO, NULL,
								   NULL)  );

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
}

/**
 * Another wrapper for the same reason.
 */
void unittests_workingdir__do_export(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const SG_pathname* pPathDirPutTopLevelDirInHere,
    const char* psz_spec_hid_cs_baseline
	)
{
	SG_rev_spec* pRevSpec = NULL;
	SG_varray * pva_wd_parents = NULL;

	if (!psz_spec_hid_cs_baseline)
	{
		// if they don't give us a HID, we ***ASSUME*** that we are
		// in a WD and can get the current baseline from it.

		VERIFY_ERR_CHECK(  SG_wc__get_wc_parents__varray(pCtx, NULL, &pva_wd_parents)  );
		VERIFY_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_wd_parents, 0, &psz_spec_hid_cs_baseline)  );
	}

	SG_ERR_CHECK(  SG_rev_spec__alloc(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_spec_hid_cs_baseline)  );

	SG_ERR_CHECK(  SG_vv2__export(pCtx,
								  pszDescriptorName, 
								  SG_pathname__sz(pPathDirPutTopLevelDirInHere),
								  pRevSpec,
								  NULL)  );

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_VARRAY_NULLFREE(pCtx, pva_wd_parents);
}

//////////////////////////////////////////////////////////////////

void unittests__wc_merge(SG_context * pCtx,
						 const SG_pathname * pPathWorkingDir,
						 const char * pszHidCSet_Other)
{
	SG_wc_merge_args mergeArgs;
	SG_pathname * pPath_old_cwd = NULL;
	SG_rev_spec * pRevSpec = NULL;
	SG_vhash * pvhStats = NULL;
	const char * pszSynopsys = NULL;
	char * pszHidTarget_Computed = NULL;

	VERIFY_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	VERIFY_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszHidCSet_Other)  );

	SG_ERR_CHECK(  SG_pathname__alloc(pCtx, &pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPathWorkingDir)  );

	memset(&mergeArgs, 0, sizeof(mergeArgs));
	mergeArgs.pRevSpec = pRevSpec;

	VERIFY_ERR_CHECK(  SG_wc__merge(pCtx, NULL, &mergeArgs, SG_FALSE,
									NULL, &pvhStats, &pszHidTarget_Computed, NULL)  );
	VERIFYP_COND("MergeTarget", (strcmp(pszHidTarget_Computed,pszHidCSet_Other)==0),
				 ("Expect computed target [%s] to match requested [%s]",
				  pszHidTarget_Computed, pszHidCSet_Other));
	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhStats, "synopsys", &pszSynopsys)  );
	INFOP("Stats", ("%s", pszSynopsys)  );

	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPath_old_cwd)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_old_cwd);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_NULLFREE(pCtx, pszHidTarget_Computed);
}
	
//////////////////////////////////////////////////////////////////

void unittests__wc_status__1(SG_context * pCtx,
							 const SG_pathname * pPathWorkingDir,
							 const char * pszItem,
							 const char * pszMessage,
							 SG_uint32 * pNrChanges,
							 SG_varray ** ppvaStatus)
{
	SG_pathname * pPath_old_cwd = NULL;
	SG_varray * pvaStatus = NULL;
	SG_uint32 count;

	SG_ERR_CHECK(  SG_pathname__alloc(pCtx, &pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPathWorkingDir)  );

	SG_ERR_CHECK(  SG_wc__status(pCtx,
								 NULL,
								 pszItem,
								 WC__GET_DEPTH(SG_TRUE),	// bRecursive
								 SG_FALSE, // bListUnchanged
								 SG_FALSE, // bNoIgnores
								 SG_FALSE, // bNoTSC
								 SG_FALSE, // bListSparse
								 SG_FALSE, // bListReserved
								 SG_TRUE,  // bNoSort -- don't bother
								 &pvaStatus,
								 NULL)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus, &count)  );

	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPath_old_cwd)  );

	if (pszMessage)
		INFOP("unittests__wc_status",("%s [computed %d]", pszMessage, count));

	*pNrChanges = count;
	if (ppvaStatus)
	{
		*ppvaStatus = pvaStatus;
		pvaStatus = NULL;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_old_cwd);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}

void unittests__wc_status__all(SG_context * pCtx,
							   const SG_pathname * pPathWorkingDir,
							   const char * pszMessage,
							   SG_uint32 * pNrChanges,
							   SG_varray ** ppvaStatus)
{
	SG_ERR_CHECK_RETURN(  unittests__wc_status__1(pCtx, pPathWorkingDir, "@/",
												  pszMessage,
												  pNrChanges, ppvaStatus)  );
}

//////////////////////////////////////////////////////////////////

/**
 * do a historical status on 2 csets.  we take pPathWorkingDir
 * so that we can get the repo name.
 *
 */
void unittests__vv2_status(SG_context * pCtx,
						   const SG_pathname * pPathWorkingDir,
						   const char * pszHid1,
						   const char * pszHid2,
						   const char * pszMessage,
						   SG_uint32 * pNrChanges,
						   SG_varray ** ppvaStatus)
{
	SG_pathname * pPath_old_cwd = NULL;
	SG_varray * pvaStatus = NULL;
	SG_rev_spec * pRevSpec = NULL;
	SG_uint32 count;

	SG_ERR_CHECK(  SG_pathname__alloc(pCtx, &pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPathWorkingDir)  );

	SG_ERR_CHECK(  SG_rev_spec__alloc(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszHid1)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszHid2)  );

	SG_ERR_CHECK(  SG_vv2__status(pCtx, NULL, pRevSpec,
								  NULL, SG_INT32_MAX,
								  SG_TRUE,	// bNoSort -- don't bother
								  &pvaStatus, NULL)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus, &count)  );

	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPath_old_cwd)  );

	if (pszMessage)
		INFOP("unittests__vv_status",("%s [computed %d]", pszMessage, count));

	*pNrChanges = count;
	if (ppvaStatus)
	{
		*ppvaStatus = pvaStatus;
		pvaStatus = NULL;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_old_cwd);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
}

//////////////////////////////////////////////////////////////////

struct _compare_status_cb_data
{
	const SG_varray * pvaOther;
	const char * pszMessage;
};

/**
 * We get called once for each ITEM in the source VARRAY.
 * Verify that there is a corresponding ITEM (with the same
 * FLAGS) in the other VARRAY.
 *
 */
static SG_varray_foreach_callback _compare_status_cb;

static void _compare_status_cb(SG_context * pCtx,
							   void * pVoidData,
							   const SG_varray * pvaThis,
							   SG_uint32 ndx,
							   const SG_variant * pv_k)
{
	struct _compare_status_cb_data * pData = (struct _compare_status_cb_data *)pVoidData;
	SG_vhash * pvhItem_k;
	SG_vhash * pvhStatus_k;
	const char * pszPath_k;
	const char * pszPath_k__after_prefix;
	const char * pszGid_k;
	SG_int64 alias_k = -1;
	SG_int64 i64_flags_k;
	SG_uint32 j, count_other;
	SG_int_to_string_buffer buf64_a;
	SG_int_to_string_buffer buf64_1;
	SG_int_to_string_buffer buf64_2;
	SG_bool bHasAlias_k;

	SG_UNUSED( pvaThis );
	SG_UNUSED( ndx );

	// alias_k = array[k].alias;		// if defined
	// gid_k   = array[k].gid;
	// path_k  = array[k].path;
	// flags_k = array[k].status.flags;

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv_k, &pvhItem_k)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(       pCtx, pvhItem_k,   "gid",    &pszGid_k)  );
	SG_ERR_CHECK(  SG_vhash__has(           pCtx, pvhItem_k,   "alias",  &bHasAlias_k)  );
	if (bHasAlias_k)
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhItem_k,   "alias",  &alias_k)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(       pCtx, pvhItem_k,   "path",   &pszPath_k)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(    pCtx, pvhItem_k,   "status", &pvhStatus_k)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(    pCtx, pvhStatus_k, "flags",  &i64_flags_k)  );

	SG_ASSERT(  (pszPath_k[0]=='@')  );
	if (pszPath_k[1] == '/')
		pszPath_k__after_prefix = &pszPath_k[2];	// "@/foo..."
	else
		pszPath_k__after_prefix = &pszPath_k[3];	// "@b/foo..."

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaOther, &count_other)  );
	for (j=0; j<count_other; j++)
	{
		SG_vhash * pvhItemOther_j;
		SG_vhash * pvhStatusOther_j;
		SG_int64 aliasOther_j = -1;
		SG_int64 i64_flagsOther_j;
		const char * pszPathOther_j;
		const char * pszGidOther_j;
		SG_bool bHasAliasOther_j;
		const char * pszPathOther_j__after_prefix;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaOther, j, &pvhItemOther_j)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItemOther_j,   "gid",   &pszGidOther_j)  );

		if (strcmp(pszGidOther_j, pszGid_k) == 0)
		{
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhItemOther_j,   "alias", &bHasAliasOther_j)  );
			if (bHasAliasOther_j)
				SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhItemOther_j,   "alias", &aliasOther_j)  );

			if (bHasAlias_k && bHasAliasOther_j)
				VERIFYP_COND( "compare_status_cb", (aliasOther_j == alias_k),
							  ("mismatch: %s [gid %s] alias mismatch [k %s][j %s]",
							   pData->pszMessage,
							   pszGid_k,
							   SG_uint64_to_sz(alias_k,buf64_1),
							   SG_uint64_to_sz(aliasOther_j,buf64_2)) );

			SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvhItemOther_j,   "path",   &pszPathOther_j)  );
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItemOther_j,   "status", &pvhStatusOther_j)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatusOther_j, "flags",  &i64_flagsOther_j)  );

			// TODO 2011/12/22 Think about adding content-hid, attrbits, etc...

			SG_ASSERT(  (pszPathOther_j[0]=='@')  );
			if (pszPathOther_j[1] == '/')
				pszPathOther_j__after_prefix = &pszPathOther_j[2];		// "@/foo..."
			else
				pszPathOther_j__after_prefix = &pszPathOther_j[3];		// "@b/foo..."

			if ((i64_flagsOther_j == i64_flags_k)
				&& (strcmp(pszPathOther_j__after_prefix, pszPath_k__after_prefix) == 0))
			{
			}
			else
			{
				VERIFYP_COND("compare_status_cb",
							 (0),
							 ("mismatch: %s [alias %s][%s 0x%s][%s 0x%s]",
							  pData->pszMessage,
							  SG_uint64_to_sz(alias_k,buf64_a),
							  pszPath_k, SG_uint64_to_sz__hex(i64_flags_k, buf64_1),
							  pszPathOther_j, SG_uint64_to_sz__hex(i64_flagsOther_j, buf64_2)));
			}
			
			return;
		}
	}

	VERIFYP_COND("compare_status_cb",
				 (0),
				 ("mismatch: %s [alias %s][%s 0x%s] not present in other",
				  pData->pszMessage,
				  SG_uint64_to_sz(alias_k,buf64_a),
				  pszPath_k, SG_uint64_to_sz__hex(i64_flags_k, buf64_1)));

fail:
	return;
}

void unittests__compare_status(SG_context * pCtx,
							   const SG_varray * pvaStatus1,
							   const SG_varray * pvaStatus2,
							   const char * pszMessage)
{
	SG_uint32 count1, count2;
	struct _compare_status_cb_data data;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus1, &count1)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus2, &count2)  );
	
	INFOP("unittests__compare_status",("%s [count1 %d][count2 %d]", pszMessage, count1, count2));

	// iterate over status1 and lookup the corresponding item in status2.
	// and vice versa.  this does twice the work, but i don't have to
	// worry about copying/sorting/uniquing/etc.

	data.pvaOther = pvaStatus2;
	data.pszMessage = "1-vs-2";
	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus1, _compare_status_cb, (void *)&data)  );

	data.pvaOther = pvaStatus1;
	data.pszMessage = "2-vs-1";
	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus2, _compare_status_cb, (void *)&data)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

struct _expect_in_status_gid_data
{
	const char * pszGid;
	const char * pszRepoPath;
	SG_wc_status_flags statusFlags;
	SG_bool bFound;
	const char * szFile;
	int lineNr;
};

/**
 * We get called once for each item in the given canonical
 * status.  Look for an item with the requested GID.  Verify
 * that it has the expected set of flags.  
 *
 */
static SG_varray_foreach_callback _expect_in_status_gid_cb;

static void _expect_in_status_gid_cb(SG_context * pCtx,
								 void * pVoidData,
								 const SG_varray * pvaThis,
								 SG_uint32 ndx,
								 const SG_variant * pv_k)
{
	struct _expect_in_status_gid_data * pData = (struct _expect_in_status_gid_data *)pVoidData;
	SG_vhash * pvhItem_k;
	SG_vhash * pvhStatus_k;
	const char * pszPath_k;
	const char * pszGid_k;
	SG_int64 i64_flags_k;
	SG_int_to_string_buffer buf64_1;
	SG_int_to_string_buffer buf64_2;

	SG_UNUSED( pvaThis );
	SG_UNUSED( ndx );

	if (pData->bFound)		// need a clean way to tell caller to stop iteration....
		return;

	// gid_k   = array[k].gid;
	// path_k  = array[k].path;
	// flags_k = array[k].status.flags;

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv_k, &pvhItem_k)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(       pCtx, pvhItem_k,   "gid",    &pszGid_k)  );
	if (strcmp(pszGid_k, pData->pszGid) != 0)
		return;

	pData->bFound = SG_TRUE;

	SG_ERR_CHECK(  SG_vhash__get__sz(       pCtx, pvhItem_k,   "path",   &pszPath_k)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(    pCtx, pvhItem_k,   "status", &pvhStatus_k)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(    pCtx, pvhStatus_k, "flags",  &i64_flags_k)  );

	VERIFYP_COND("expect_in_status__gid", (((SG_wc_status_flags)i64_flags_k) == pData->statusFlags),
				 ("%s:%d: Unexpected flags [observed %s][expected %s]: %s %s",
				  pData->szFile, pData->lineNr,
				  SG_uint64_to_sz__hex(i64_flags_k,buf64_1),
				  SG_uint64_to_sz__hex(pData->statusFlags,buf64_2),
				  pData->pszGid,
				  pszPath_k)  );

	if (pData->pszRepoPath)
		VERIFYP_COND("expect_in_status__gid", (strcmp(pszPath_k, pData->pszRepoPath)==0),
					 ("%s:%d: Unexpected repo-path [observed %s][expected %s]: %s",
					  pData->szFile, pData->lineNr,
					  pszPath_k, pData->pszRepoPath, pData->pszGid)  );

fail:
	return;
}

/**
 * Find an expected item in the given status by GID
 * and confirm that it has the expected statusFlags
 * and optionally the expected repo-path.
 *
 * Since the GID is always unique, we can use this
 * in cases where we may have ambiguous repo-paths
 * (more than one item with the same repo-path).
 *
 */
void unittests__expect_in_status__gid(SG_context * pCtx,
									  const char * szFile, int lineNr,
									  const SG_varray * pvaStatus,
									  const char * pszGid,
									  const char * pszRepoPath,
									  SG_wc_status_flags statusFlags)
{
	struct _expect_in_status_gid_data data;
	SG_int_to_string_buffer buf64_1;

	data.pszGid = pszGid;
	data.pszRepoPath = pszRepoPath;
	data.statusFlags = statusFlags;
	data.bFound = SG_FALSE;
	data.szFile = szFile;
	data.lineNr = lineNr;

	VERIFY_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus, _expect_in_status_gid_cb, (void *)&data)  );

	VERIFYP_COND("expect_in_status__gid", (data.bFound),
				 ("%s:%d: Item not found: %s %s %s",
				  szFile, lineNr,
				  pszGid,
				  pszRepoPath,
				  SG_uint64_to_sz__hex(statusFlags,buf64_1)));

fail:
	return;
}

//////////////////////////////////////////////////////////////////

struct _expect_in_status_path_data
{
	const char * pszNotGid;
	const char * pszRepoPath;
	SG_wc_status_flags statusFlags;
	SG_bool bFound;
	const char * szFile;
	int lineNr;
};

/**
 * We get called once for each item in the given canonical
 * status.  Look for an item with the requested repo-path.
 * Verify that it has the expected set of flags.  
 *
 */
static SG_varray_foreach_callback _expect_in_status_path_cb;

static void _expect_in_status_path_cb(SG_context * pCtx,
								 void * pVoidData,
								 const SG_varray * pvaThis,
								 SG_uint32 ndx,
								 const SG_variant * pv_k)
{
	struct _expect_in_status_path_data * pData = (struct _expect_in_status_path_data *)pVoidData;
	SG_vhash * pvhItem_k;
	SG_vhash * pvhStatus_k;
	const char * pszPath_k;
	const char * pszGid_k = NULL;
	SG_int64 i64_flags_k;
	SG_int_to_string_buffer buf64_1;
	SG_int_to_string_buffer buf64_2;

	SG_UNUSED( pvaThis );
	SG_UNUSED( ndx );

	if (pData->bFound)		// need a clean way to tell caller to stop iteration....
		return;

	// gid_k   = array[k].gid;
	// path_k  = array[k].path;
	// flags_k = array[k].status.flags;

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv_k, &pvhItem_k)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(       pCtx, pvhItem_k,   "path",   &pszPath_k)  );
	if (strcmp(pszPath_k, pData->pszRepoPath) != 0)
		return;

	if (pData->pszNotGid)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(       pCtx, pvhItem_k,   "gid",    &pszGid_k)  );
		if (strcmp(pszGid_k, pData->pszNotGid) == 0)
			return;
	}
	
	// NOTE: this test can only supports a 2-way ambiguous repo-path.
	// NOTE: if 3 files (2 deleted and 1 added) all have the same repo-path
	// NOTE: we can't use this test to confirm it.  we'd need to take a
	// NOTE: vector of GIDs to exclude.

	pData->bFound = SG_TRUE;

	SG_ERR_CHECK(  SG_vhash__get__vhash(    pCtx, pvhItem_k,   "status", &pvhStatus_k)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(    pCtx, pvhStatus_k, "flags",  &i64_flags_k)  );

	VERIFYP_COND("expect_in_status__path", (((SG_wc_status_flags)i64_flags_k) == pData->statusFlags),
				 ("%s:%d: Unexpected flags [observed %s][expected %s]: %s %s",
				  pData->szFile, pData->lineNr,
				  SG_uint64_to_sz__hex(i64_flags_k,buf64_1),
				  SG_uint64_to_sz__hex(pData->statusFlags,buf64_2),
				  pszPath_k, pszGid_k)  );

fail:
	return;
}

/**
 * Find an expected item in the given status by REPO-PATH
 * and confirm that it has the expected statusFlags
 * and optionally that it DOES NOT HAVE the given GID.
 *
 * Since we can have multiple items with the same repo-path
 * (but only 1 that currently owns it), we verify the status
 * matches the desired path and optionally also verify that
 * it does not have a specific GID.
 *
 */
void unittests__expect_in_status__path(SG_context * pCtx,
									   const char * szFile, int lineNr,
									   const SG_varray * pvaStatus,
									   const char * pszRepoPath,
									   SG_wc_status_flags statusFlags,
									   const char * pszNotGid)
{
	struct _expect_in_status_path_data data;
	SG_int_to_string_buffer buf64_1;

	data.pszNotGid = pszNotGid;
	data.pszRepoPath = pszRepoPath;
	data.statusFlags = statusFlags;
	data.bFound = SG_FALSE;
	data.szFile = szFile;
	data.lineNr = lineNr;

	VERIFY_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus, _expect_in_status_path_cb, (void *)&data)  );

	VERIFYP_COND("expect_in_status__path", (data.bFound),
				 ("%s:%d: Item not found: %s %s %s",
				  szFile, lineNr,
				  pszRepoPath,
				  SG_uint64_to_sz__hex(statusFlags,buf64_1),
				  pszNotGid));

fail:
	return;
}

//////////////////////////////////////////////////////////////////

#define SG_DAGNUM__TESTING__NOTHING   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__TESTING, \
        SG_DAGNUM__SET_DAGID(1, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__NOTHING, \
        ((SG_uint64) 0)))))
#define SG_DAGNUM__TESTING__NOTHING_SZ "0000000010401000" // u008_misc_utils__test_dagnums verifies that this is correct

#define SG_DAGNUM__TESTING2__NOTHING   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__TESTING, \
        SG_DAGNUM__SET_DAGID(2, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__NOTHING, \
        ((SG_uint64) 0)))))
#define SG_DAGNUM__TESTING2__NOTHING_SZ "0000000010402000" // u008_misc_utils__test_dagnums verifies that this is correct

#endif//H_UNITTESTS_H
