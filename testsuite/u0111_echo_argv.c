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
 * @file u0111_echo_argv.c
 *
 * @details This 'test' is intended to be invoked from vscript
 * via sg.exec() and confirm that (argc,argv) arrives as intended.
 * It is not a stand-alone test like the others.
 *
 * That is, does all of the quoting, escaping, and etc in JavaScript
 * and in the JS glue layer and/or SG_exec() get correctly expressed
 * in the actual OS exec(), system(), CreateProcessW() or whatever
 * from vscript such that main() or wmain() of this program receives
 * the same set of args.
 *
 * This is mainly a problem when SG_exec() needs to build a command
 * line (for CreateProcessW() or system()) rather than using execvp().
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "unittests.h"

//////////////////////////////////////////////////////////////////

/**
 *
 */
TEST_MAIN(u0111_echo_argv)
{
	int k;

	TEMPLATE_MAIN_START;

	fprintf(stdout, "u0111_echo_argv: [argc %d]\n", argc);
	for (k=0; k<argc; k++)
	{
#if defined(WINDOWS)
		fprintf(stdout, "argv[%d]: %ls\n", k, argv[k]);		// argv is wchar_t on Windows.
#else
		fprintf(stdout, "argv[%d]: %s\n", k, argv[k]);
#endif
	}
	for (k=0; k<argc; k++)
	{
#if defined(WINDOWS)
		wchar_t * p = argv[k];
#else
		char * p = argv[k];
#endif
		fprintf(stdout, "argv[%d]:", k);
		while (*p)
			fprintf(stdout, " %02x", *p++);
		fprintf(stdout, "\n");
	}

	// Normal usage (so that we can report test failures)
	// is that argv[1] be an integer containing the expected
	// number of args following it -- not counting argv[0] or argv[1].
	//
	// So the following commands should pass:
	//     u0111                    -- argc==1
	//     u0111 0                  -- argc==2
	//     u0111 1 hello            -- argc==3
	//     u0111 2 hello there
	//
	// Allow bogus usage when run from the testsuite directly
	// (instead of from vscript).  The testsuite/CMakeLists.txt
	// passes the CWD at arg 1.
	//

	if (argc > 1)
	{
		int r, n;

#if defined(WINDOWS)
		r = swscanf(argv[1], L"%d", &n);
#define SKIP_FMT "argv[1] == %ls"
#else
		r = sscanf(argv[1], "%d", &n);
#define SKIP_FMT "argv[1] == %s"
#endif
		if (r == 1)
			VERIFY_COND("argv[1]==(argc-2)", (n == (argc-2)));
		else
			INFOP("Skipping test", (SKIP_FMT, argv[1]));
	}

	TEMPLATE_MAIN_END;
}

