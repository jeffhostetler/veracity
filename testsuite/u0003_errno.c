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

// testsuite/unittests/u0003_errno.c
// some trivial tests to exercise sg_errno.h
//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "unittests.h"

#define VERIFY_STR_EQUAL(t,desc,expected,got)    VERIFYP_COND(t,(strcmp((expected),(got)) == 0), ("%s: expected '%s', got '%s'", (desc), (expected), (got)))

//////////////////////////////////////////////////////////////////

int u0003_errno__test_errno(void)
{
	char buf[SG_ERROR_BUFFER_SIZE];

#if defined(DEBUG)
	SG_bool bDetailed = SG_TRUE;
#else
	SG_bool bDetailed = SG_FALSE;
#endif

	// verify argument validation.

	SG_error__get_message(SG_ERR_UNSPECIFIED, bDetailed, NULL, 0);

	// verify that we get the expected message.

	SG_error__get_message(SG_ERR_UNSPECIFIED,bDetailed,buf,sizeof(buf));
	if (bDetailed)
		VERIFY_STR_EQUAL("test_errno", "SG_ERR_UNSPECIFIED", "Error 1 (sglib): Unspecified error", buf);
	else
		VERIFY_STR_EQUAL("test_errno", "SG_ERR_UNSPECIFIED", "Error: Unspecified error", buf);

	// verify that we get a properly truncated message for a short buffer.

	SG_error__get_message(SG_ERR_UNSPECIFIED,bDetailed,buf,10);
	if (bDetailed)
		VERIFY_STR_EQUAL("test_errno",  "SG_ERR_UNSPECIFIED(10)", "Error 1 (", buf);
	else
		VERIFY_STR_EQUAL("test_errno",  "SG_ERR_UNSPECIFIED(10)", "Error: Un", buf);


	SG_error__get_message(SG_ERR_UNSPECIFIED,bDetailed,buf,1);
	VERIFY_STR_EQUAL("test_errno",  "SG_ERR_UNSPECIFIED(1)", "", buf);

	return 1;
}

int u0003_errno__get_message(void)
{
	// we now have different types of errors.  verify that we get the proper message.

	char buf[SG_ERROR_BUFFER_SIZE];
#if defined(DEBUG)
	SG_bool bDetailed = SG_TRUE;
#else
	SG_bool bDetailed = SG_FALSE;
#endif

	// errno-based messages will vary by platform.
	SG_error__get_message(SG_ERR_ERRNO(ENOENT), bDetailed, buf, sizeof(buf));
	VERIFY_STR_EQUAL("get_message", "ENOENT", "No such file or directory", buf);

#if defined(WINDOWS)
	// GetLastError-based messages
	SG_error__get_message(SG_ERR_GETLASTERROR(ERROR_PATH_NOT_FOUND), bDetailed, buf, sizeof(buf));
	VERIFY_STR_EQUAL("get_message", "ERROR_PATH_NOT_FOUND", "The system cannot find the path specified", buf);
#endif

	// sg-library-based error codes.
	SG_error__get_message(SG_ERR_INVALIDARG, bDetailed, buf, sizeof(buf));
	if (bDetailed)
		VERIFY_STR_EQUAL("get_message", "SG_ERR_INVALIDARG", "Error 2 (sglib): Invalid argument", buf);
	else
		VERIFY_STR_EQUAL("get_message", "SG_ERR_INVALIDARG", "Error: Invalid argument", buf);

	return 1;
}

TEST_MAIN(u0003_errno)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0003_errno__test_errno()  );
	BEGIN_TEST(  u0003_errno__get_message()  );

	TEMPLATE_MAIN_END;
}



