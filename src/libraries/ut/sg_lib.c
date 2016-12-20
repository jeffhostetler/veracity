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
 * @file sg_lib.c
 *
 * @details Initialization/Cleanup of library global data.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "sg_lib__private.h"

//////////////////////////////////////////////////////////////////

struct _sg_lib__global_data
{
	sg_lib_utf8__global_data * pUtf8GlobalData;
	SG_string * pVersion;
};

static sg_lib__global_data gLibData = {NULL, 0};
static SG_bool gbLibDataInitialized = SG_FALSE;

// TODO remove this and the call to it, below.
sg_repo__vtable* SG_fs3__get_vtable(void);

//////////////////////////////////////////////////////////////////

void SG_lib__global_initialize(SG_context * pCtx)
{
	if (gbLibDataInitialized)
		SG_ERR_THROW_RETURN(SG_ERR_ALREADY_INITIALIZED);

#if defined(WINDOWS) && defined(DEBUG)

	// Send run-time warnings and errors to stderr (they go to debugger output by default)
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

#if defined(WINDOWS_CRT_LEAK_CHECK)
	// Turn on Windows' C runtime library debug leak checking.
	// This causes _CrtDumpMemoryLeaks() to be called when our process exits.
	// You could accomplish the same thing by adding a call to _CrtDumpMemoryLeaks
	// to _sg_mem_dump_atexit.
	{
		int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		flags |= _CRTDBG_LEAK_CHECK_DF;
		_CrtSetDbgFlag(flags);
	}
#endif // WINDOWS_CRT_LEAK_CHECK

#if defined(SG_REAL_BUILD)
	// Send assertion failures to stderr (don't raise a dialog) when performing an official build.
	// SG_REAL_BUILD gets defined by CMAKE if there's an environment variable SPRAWL_BUILDER = 1.
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif // SG_REAL_BUILD

#endif // WINDOWS && DEBUG

	memset(&gLibData,0,sizeof(gLibData));

	// WARNING: We assume that it is possible for the caller to have created
	// WARNING: a properly-populated SG_context before the library is initialized.
	// WARNING:
	// WARNING: That is, that the creation of the SG_context MUST NOT require any
	// WARNING: character encoding, locale, and/or UTF-8 stuff.
	// WARNING:
	// WARNING: Also, there the SG_context creation MUST NOT require us to read
	// WARNING: any local settings.
	//
	// WARNING: We also assume that stuff in SG_malloc.c can be called before the
	// WARNING: library is initialized.

	// We assume that the character encoding/locale stuff should be initialized first
	// before anyone tries to access the disk (for local settings and etc) because
	// all of the SG_fsobj__ and SG_file__ routines convert UTF-8 SG_pathnames into
	// locale-based char* or wchar_t* buffers before accessing the filesystem.

	SG_ERR_CHECK(  sg_lib_utf8__global_initialize(pCtx, &gLibData.pUtf8GlobalData)  );
	SG_ERR_CHECK(  sg_pathname__global_initialize(pCtx)  );
	SG_ERR_CHECK(  sg_closet__global_initialize(pCtx)  );
	SG_ERR_CHECK(  sg_localsettings__global_initialize(pCtx)  );

	SG_ERR_CHECK(  SG_log__global_init(pCtx)  );

#ifndef SG_IOS
	SG_ERR_CHECK(  SG_curl__global_init(pCtx)  );
#endif

	// TODO remove this and its declaration, above.
    SG_ERR_CHECK(  SG_repo__install_implementation(pCtx,  SG_fs3__get_vtable())  );


	SG_ERR_CHECK(  SG_zing__init_template_caches(pCtx)  );

    {
        SG_int64 itime = -1;
        SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &itime)  );
        SG_random__seed((SG_uint32) itime);
    }

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &gLibData.pVersion)  );
	#ifdef BUILD_CSID
		#ifdef WD_DIRTY
			#define STATUSMARK "+"
		#else
			#define STATUSMARK ""
		#endif
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, gLibData.pVersion,
			"%s.%s.%s.%s (%.10s%s) [%d-bit %s]%s",
			MAJORVERSION, MINORVERSION, REVVERSION, BUILDNUMBER, BUILD_CSID, STATUSMARK,
			(SG_uint32)(sizeof(void*) * 8), SG_PLATFORM, DEBUGSTRING)  );
	#else
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, gLibData.pVersion,
			"%s.%s.%s.%s [%d-bit %s]%s",
			MAJORVERSION, MINORVERSION, REVVERSION, BUILDNUMBER,
			(SG_uint32)(sizeof(void*) * 8), SG_PLATFORM, DEBUGSTRING)  );
	#endif

	gbLibDataInitialized = SG_TRUE;

	return;

fail:
	// TODO 2013/01/07 I'm going to turn this off
	// TODO            and let the caller call cleanup
	// TODO            in *their* fail block.
	// TODO            This allows their failure code to
	// TODO            call SG_console() with the existing
	// TODO            UTF8 context/converters setup.
	// SG_ERR_IGNORE(  SG_lib__global_cleanup(pCtx)  );
	return;
}

void SG_lib__global_cleanup(SG_context * pCtx)
{
#ifndef SG_IOS
	SG_curl__global_cleanup();
	SG_uridispatch__global_cleanup(pCtx);
	SG_jscore__shutdown(pCtx);
#endif
    SG_zing__now_free_all_cached_templates(pCtx);
    SG_repo__free_implementation_plugin_list(pCtx);
	SG_log__global_cleanup();
	sg_localsettings__global_cleanup(pCtx);
	sg_closet__global_cleanup(pCtx);
	sg_pathname__global_cleanup(pCtx);
	sg_lib_utf8__global_cleanup(pCtx, &gLibData.pUtf8GlobalData);

	SG_STRING_NULLFREE(pCtx, gLibData.pVersion);

	memset(&gLibData,0,sizeof(gLibData));
	gbLibDataInitialized = SG_FALSE;
}

void SG_lib__version(SG_context * pCtx, const char **ppVersion)
{
	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppVersion);
	if(!gbLibDataInitialized)
		SG_ERR_THROW2_RETURN(SG_ERR_UNINITIALIZED, (pCtx, "sglib has not been initialized"));
	*ppVersion = SG_string__sz(gLibData.pVersion);
}

//////////////////////////////////////////////////////////////////

void sg_lib__fetch_utf8_global_data(SG_context * pCtx, sg_lib_utf8__global_data ** ppUtf8GlobalData)
{
	SG_NULLARGCHECK_RETURN(ppUtf8GlobalData);

	if (!gLibData.pUtf8GlobalData)
		SG_ERR_THROW_RETURN(SG_ERR_UNINITIALIZED);

	*ppUtf8GlobalData = gLibData.pUtf8GlobalData;
}
