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
 * @file sg_utf8.c
 *
 * @details Unicode and UTF-8 functions and wrappers for ICU functions.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "sg_lib__private.h"

#if !defined(SG_IOS)
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

SG_STATIC_ASSERT( sizeof(SG_uint16) == sizeof(UChar) );
SG_STATIC_ASSERT( sizeof(SG_int32) == sizeof(UChar32) );
#endif

#if defined(LINUX)
#include <locale.h>		// need setlocale(3)
#include <langinfo.h>	// need nl_langinfo(3)
#endif

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#define TRACE_UTF8 0
#else
#define TRACE_UTF8 0
#endif

//////////////////////////////////////////////////////////////////

const SG_byte UTF8_BOM[3] = {0xEF, 0xBB, 0xBF};

struct _sg_lib_utf8__global_data
{
	SG_utf8_converter *		pLocaleEnvCharSetConverter;				// an exact converter, no substitutions allowed
#if defined(LINUX)
	SG_utf8_converter *		pSubstitutingLocaleEnvCharSetConverter;	// a substituting converter
#endif
};

#if defined(MAC)
/**
 * Intern an OS-supplied string (might be an entryname or pathname).
 *
 * On the MAC, getcwd(), readdir(), and etc give us UTF-8 NFD strings.
 * (Actually, it's their custom/bastardized version of Unicode 3.2,
 * but I digress.)
 *
 * Internally, we always try to use UTF-8 NFC.
 *
 * This routine uses the apple-specific routines to give us clean UTF-8 NFC.
 *
 * We ***assume*** that the user's environment locale charset is UTF-8.
 * The 'locale' command was added in 10.4, but I'm not sure anyone uses it
 * for character encoding stuff ***and*** it is not clear that it would
 * affect open(), stat(), and friends.
 *
 */
static void _sg_utf8_mac__intern_from_os_buffer__sz(SG_context * pCtx,
													SG_string * pString,
													const char * szOS)
{
	char * szNFC = NULL;
	SG_uint32 lenNFC;

	// We ***assume*** that the OS gives us proper/canonical/fully-decomposed NFD
	// so we don't have to break it down any further or clean it up before we
	// compute the NFC.  The returned length ***DOES NOT*** include the trailing NULL.

	SG_ERR_CHECK(  SG_apple_unicode__compute_nfc__utf8(pCtx, szOS,strlen(szOS),&szNFC,&lenNFC)  );

	// let the string adopt the buffer.  this is like a SG_string__set__sz() but
	// lets us avoid another malloc and memcpy.

	SG_ERR_CHECK(  SG_string__adopt_buffer(pCtx, pString,szNFC,lenNFC+1)  );
	szNFC = NULL;

	return;

fail:
	SG_NULLFREE(pCtx, szNFC);
}

/**
 * Extern one of our internal UTF-8 NFC strings into something the OS is expecting.
 *
 * On the MAC, getcwd() and friends return us (apple's version of) UTF-8 NFD.
 * We can use either NFC or NFD strings with open() and friends because Apple
 * takes care of forcing input arguments to NFD.  So we don't have to here.
 *
 * Note: we might want to in the future for logging or something.
 *
 */
static void _sg_utf8_mac__extern_to_os_buffer__sz(SG_context * pCtx,
												  const char * szUtf8,
												  char ** pszOS)
{
	SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx,szUtf8,pszOS)  );
}
#endif

#if defined(LINUX) || defined(MAC)
static SG_bool _is_7bit_clean__sz(const char * sz)
{
	const SG_byte * p = (const SG_byte *)sz;

	while (*p)
		if (*p++ > 0x7f)
			return SG_FALSE;

	return SG_TRUE;
}

#if defined(LINUX)
static void _sg_utf8_locale__intern_from_os_buffer__sz(SG_context * pCtx,
													   SG_string * pString,
													   const char * szOS);
#endif

void SG_utf8__intern_from_os_buffer__sz(SG_context * pCtx,
										SG_string * pThis,
										const char * szOS)
{
	SG_NULLARGCHECK_RETURN(pThis);

	if (!szOS || !*szOS)
	{
		SG_ERR_CHECK_RETURN(  SG_string__clear(pCtx, pThis)  );
		return;
	}

	if (_is_7bit_clean__sz(szOS))
	{
		SG_ERR_CHECK_RETURN(  SG_string__set__sz(pCtx,pThis,szOS)  );
		return;
	}

#if defined(MAC)
	SG_ERR_CHECK_RETURN(  _sg_utf8_mac__intern_from_os_buffer__sz(pCtx,pThis,szOS)  );
	return;
#endif
#if defined(LINUX)
	SG_ERR_CHECK_RETURN(  _sg_utf8_locale__intern_from_os_buffer__sz(pCtx,pThis,szOS)  );
	return;
#endif
}

#if defined(LINUX)
/**
 * Extern one of our internal UTF-8 NFC strings into something the OS is expecting.
 *
 * On Linux, have to deal with the user's locale.  When our locale is UTF-8, we just
 * duplicate it and return -- we assume that we have NFC stored interally and that
 * they probably want NFC in the filesystem.  (We may actually have NFD if they
 * were sneaky when they created the file, but we allow that.)
 *
 */
static void _sg_utf8_locale__extern_to_os_buffer__sz(SG_context * pCtx,
													 const char * szUtf8,
													 char ** pszOS)
{
	SG_bool bEnvIsUtf8;

	SG_ERR_CHECK_RETURN(  SG_utf8__is_locale_env_charset_utf8(pCtx, &bEnvIsUtf8)  );

	if (bEnvIsUtf8)
	{
		SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx,szUtf8,pszOS)  );
		return;
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_utf8__to_locale_env_charset__sz(pCtx,szUtf8,pszOS)  );
		return;
	}
}
#endif

void SG_utf8__extern_to_os_buffer__sz(SG_context * pCtx,
									  const char * szUtf8,
									  char ** pszOS)
{
	SG_NULLARGCHECK_RETURN(szUtf8);
	SG_NULLARGCHECK_RETURN(pszOS);

#if defined(MAC)
	SG_ERR_CHECK_RETURN(  _sg_utf8_mac__extern_to_os_buffer__sz(pCtx,szUtf8,pszOS)  );
	return;
#endif
#if defined(LINUX)
	SG_ERR_CHECK_RETURN(  _sg_utf8_locale__extern_to_os_buffer__sz(pCtx,szUtf8,pszOS)  );
	return;
#endif
}
#endif

void sg_lib_utf8__global_initialize(SG_context * pCtx,
									sg_lib_utf8__global_data ** ppUtf8GlobalData)
{
	// Initialize library-global variables used by SG_utf8.c
	//
	// We use a set of global variables to deal with expensive
	// to create ICU datastructures that we will constantly be
	// using.
	//
	// Warning: the usual warning regarding global variables
	// and threads (which we currently don't have).  However,
	// the stuff in sg_lib_utf8__global_data deals with the
	// locale and the character encoding conventions of the
	// user's environment -- this DOES NOT VARY by thread --
	// so I think it is OK to have a single global instance
	// with pre-loaded converters; we DO NOT need this instance
	// data to be tucked inside the SG_context.
	//
	// We return a pointer to the set of global variables because
	// I want SG_lib.c to be the central place where all global
	// pointers are kept.
	//
	// This routine should only be called by SG_lib__global_initialize().

	sg_lib_utf8__global_data * pData = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pData)  );


#if defined(WINDOWS) || defined(MAC)
	SG_ERR_CHECK(  SG_utf8_converter__alloc(pCtx, NULL,&pData->pLocaleEnvCharSetConverter)  );
#endif
#if defined(LINUX)
	// get user's locale charset from the environment
	// and create the proper/necessary ICU converter
	// to convert entrynames between that charset and
	// UTF-8.
	//
	// we will use this a lot on LOCALE-based systems (such
	// as Linux.  Internally we store all file/directory
	// names in UTF-8.  These need to be converted to the
	// locale's charset before we access the filesystem.
	// We also need to convert filenames given on the
	// command line argument to UTF-8.  We also (probably)
	// need to convert filenames before/after we present
	// the normal file chooser dialogs.

	setlocale(LC_CTYPE,"");
	SG_ERR_CHECK(  SG_utf8_converter__alloc(pCtx, nl_langinfo(CODESET),&pData->pLocaleEnvCharSetConverter)  );

	// don't load substituting converter until we need it.
#endif

	// TODO are there any other global variables that we need?

	*ppUtf8GlobalData = pData;
	return;

fail:
	SG_ERR_IGNORE(  sg_lib_utf8__global_cleanup(pCtx,&pData)  );
}

void sg_lib_utf8__global_cleanup(SG_context * pCtx,
								 sg_lib_utf8__global_data ** ppUtf8GlobalData)
{
	sg_lib_utf8__global_data * pData;

	if (!ppUtf8GlobalData || !*ppUtf8GlobalData)
		return;

	pData = *ppUtf8GlobalData;

	SG_UTF8_CONVERTER_NULLFREE(pCtx, pData->pLocaleEnvCharSetConverter);
#if defined(LINUX)
	SG_UTF8_CONVERTER_NULLFREE(pCtx, pData->pSubstitutingLocaleEnvCharSetConverter);
#endif

	SG_NULLFREE(pCtx, pData);

	*ppUtf8GlobalData = NULL;

#if defined(DEBUG) && !defined(SG_IOS)
	// "When an application is terminating, it may optionally call the function u_cleanup(void) ,
	// which will free any heap storage that has been allocated and held by the ICU library. The main
	// benefit of u_cleanup() occurs when using memory leak checking tools while debugging or testing
	// an application. Without u_cleanup(), memory being held by the ICU library will be reported as
	// leaks." -- http://userguide.icu-project.org/design

	// "The use of u_cleanup() just before an application terminates is optional, but it should be
	// called only once for performance reasons. The primary benefit is to eliminate reports of
	// memory or resource leaks originating in ICU code from the results generated by heap analysis
	// tools." -- http://icu-project.org/apiref/icu4c/uclean_8h.html#a93f27d0ddc7c196a1da864763f2d8920
	u_cleanup();
#endif
}

// Helper function for SG_utf8__import_buffer.
static void _sg_utf8__try_import(SG_context * pCtx, const SG_byte * pBuf, SG_uint32 bufLen, const char * szEncoding, char ** ppSz, SG_utf8_converter ** ppConverter)
{
    // convert the given raw buffer into utf-8 assuming that the character encoding of the raw buffer is szEncoding.
    //
    // buffer will be zero-terminated.
    //
    // return SG_ERR_OK if successful.
    // return SG_ERR_ILLEGAL_CHARSET_CHAR if file isn't in this format
    // return other errors as appropriate (like SG_ERR_MALLOCFAILED)

    SG_utf8_converter * pConverter = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_ASSERT(pBuf!=NULL);
    SG_ASSERT(szEncoding!=NULL);
    SG_ASSERT(ppSz!=NULL);
    SG_ASSERT(ppConverter!=NULL);

#if TRACE_UTF8
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Utf8:TryImport: [Attempting %s]\n", szEncoding)  );
#endif

    SG_ERR_CHECK(  SG_utf8_converter__alloc(pCtx, szEncoding, &pConverter)  );
    SG_ERR_CHECK(  SG_utf8_converter__from_charset__buf_len(pCtx, pConverter, pBuf, bufLen, ppSz)  );

    *ppConverter = pConverter;

    return;
fail:
    SG_UTF8_CONVERTER_NULLFREE(pCtx, pConverter);
}



//////////////////////////////////////////////////////////////////

/**
 * Data used by _contains_char_callback.
 */
typedef struct _contains_char_callback__data
{
	SG_int32 iChar;     //< [in] The character we're checking for.
	SG_bool  bContains; //< [out] Set to SG_TRUE if iChar is found.
	//<       Should probably be initialized to SG_FALSE.
}
_contains_char_callback__data;

/**
 * SG_utf8__foreach_character_callback that checks if any of the iterated characters
 * matches a specific character.
 */
static void _contains_char_callback(
									SG_context* pCtx,     //< [in] [out] Error and context info.
									SG_int32    iChar,    //< [in] The current character.
									void*       pUserData //< [in] Pointer to _contains_char_callback__data with additional parameters.
									)
{
	_contains_char_callback__data* pData = (_contains_char_callback__data*)pUserData;
	
	SG_NULLARGCHECK(pUserData);
	
	// check if this is the character we're looking for
	if (iChar == pData->iChar)
	{
		pData->bContains = SG_TRUE;
		// would be nice to have the caller stop iterating when this happens
	}
	
fail:
	return;
}

void SG_utf8__contains_char(
							SG_context* pCtx,
							const char* szString,
							SG_int32    iChar,
							SG_bool*    pContains
							)
{
	_contains_char_callback__data cData;
	cData.bContains = SG_FALSE;
	
	SG_NULLARGCHECK(pContains);
	
	// run through szString looking for iChar
	if (szString != NULL)
	{
		cData.iChar = iChar;
		SG_ERR_CHECK(  SG_utf8__foreach_character(pCtx, szString, _contains_char_callback, (void*)&cData)  );
	}
	
	*pContains = cData.bContains;
	
fail:
	return;
}



/**
 * Data used by _shares_characters_callback.
 */
typedef struct _shares_characters_callback__data
{
	const char* szChars;   //< [in] The set of characters we're checking for as a string.
	SG_bool     bContains; //< [out] Set to SG_TRUE if any of the characters are found.
	//<       Should probably be initialized to SG_FALSE.
}
_shares_characters_callback__data;

/**
 * SG_utf8__foreach_character_callback that checks if any of the iterated characters
 * appear in a given string.
 */
static void _shares_characters_callback(
										SG_context* pCtx,     //< [in] [out] Error and context info.
										SG_int32    iChar,    //< [in] The current character.
										void*       pUserData //< [in] Pointer to _shares_characters_callback__data with additional parameters.
										)
{
	_shares_characters_callback__data* pData     = (_shares_characters_callback__data*)pUserData;
	SG_bool                            bContains = SG_FALSE;
	
	SG_NULLARGCHECK(pUserData);
	
	// check if pData->szChars contains the current character
	SG_ERR_CHECK(  SG_utf8__contains_char(pCtx, pData->szChars, iChar, &bContains)  );
	if (bContains != SG_FALSE)
	{
		pData->bContains = SG_TRUE;
		// would be nice to have the caller stop iterating when this happens
	}
	
fail:
	return;
}

/**
 * Checks if two UTF8 strings share any characters in common.
 */
void SG_utf8__shares_characters(
								SG_context* pCtx,
								const char* szLeft,
								const char* szRight,
								SG_bool*    pShares
								)
{
	_shares_characters_callback__data cData;
	cData.bContains = SG_FALSE;
	
	SG_NULLARGCHECK(pShares);
	
	if (szLeft != NULL && szRight != NULL)
	{
		// iterate through each character in szLeft
		// and check if the character appears in szRight
		cData.szChars = szRight;
		SG_ERR_CHECK(  SG_utf8__foreach_character(pCtx, szLeft, _shares_characters_callback, (void*)&cData)  );
	}
	
	*pShares = cData.bContains;
	
fail:
	return;
}

#ifndef SG_IOS

struct _sg_utf8_converter
{
	SG_bool					bIsCharSetUtf8;
	char *					szCharSetName;
	UConverter *			pIcuConverter;
};

//////////////////////////////////////////////////////////////////

/**
 * Like SG_ERR_CHECK(), but for dealing with an ICU error.
 * We require that you declare "UErrorCode ue" at the top
 * of your function.  Upon error, we also update "err".
 *
 * The statement "s" is probably an assignment/function call
 * because ICU routines usually return a length or something
 * and take "&ue" as their last argument.
 *
 * WARNING: UErrorCodes are handled a little differently by ICU than
 * WARNING: other libraries.  With ICU routines, error code arguments
 * WARNING: are ***IN and OUT***.  You need to initialize the code to
 * WARNING: OK before passing it to any functions.  The ICU functions
 * WARNING: check the incomming error code before doing anything.  If
 * WARNING: the incomming error code is an error, the function will just
 * WARNING: return ***WITHOUT DOING ANYTHING***.
 * WARNING:
 * WARNING: The idea is that you can call a series of functions and
 * WARNING: only have to check the error code at the end -- rather than
 * WARNING: between each step.
 *
 */
#define SG_UE_ERR_CHECK(s)		SG_STATEMENT( ue=U_ZERO_ERROR;  s  ; if (U_FAILURE(ue)) { SG_ERR_THROW( SG_ERR_ICU(ue) ); } )

//////////////////////////////////////////////////////////////////

SG_uint32 SG_utf8__length_in_bytes(const char* s)
{
	/*
	  strlen on a utf8 string returns the number of bytes, not
	  characters.  in this case, that's what we want.  we also
	  add 1 for the zero byte at the end.
	*/

	return 1 + (SG_uint32)strlen(s);
}

void SG_utf8__length_in_characters__sz(SG_context * pCtx, const char* s, SG_uint32* pResult)
{
	SG_ERR_CHECK_RETURN(  SG_utf8__length_in_characters__buflen(pCtx, s, SG_utf8__length_in_bytes(s), pResult)  );
}

void SG_utf8__length_in_characters__buflen(SG_context * pCtx, const char* s, SG_uint32 lenInput, SG_uint32* pResult)
{
	// TODO Eric, was there an ICU function that you took
	// this from that or is this original?

	int32_t i = 0;
	int32_t len = (int32_t)lenInput;

	SG_uint32 result = 0;

	while (i<len)
	{
		int32_t c;

		U8_NEXT(s, i, len, c);

		if (c == 0)
		{
			break;
		}

		if (c < 0)
		{
			SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  );
		}

		result++;
	}

	*pResult = result;
}

int  SG_utf8__compare(const char* s1, const char* s2)
{
	// TODO see about replacing the body of this function
	// with a call to an ICU function.
	/*
	  strcmp of two utf8 strings is wrong.  they need to be compared one
	  codepoint at a time
	  TODO see comment below about normalization
	*/

	int32_t i1 = 0;
	int32_t i2 = 0;
	int32_t len1 = (int32_t)SG_utf8__length_in_bytes(s1);
	int32_t len2 = (int32_t)SG_utf8__length_in_bytes(s2);

	while ((i1 < len1) && (i2 < len2))
	{
		SG_int32 c1;
		SG_int32 c2;

		U8_NEXT(s1, i1, len1, c1);
		U8_NEXT(s2, i2, len2, c2);

		if ((c1 == 0) && (c2 == 0))
		{
			break;
		}

		if (c1 < c2)
		{
			return -1;
		}
		else if (c1 > c2)
		{
			return 1;
		}
	}

	return 0;
}

int SG_utf8__compare__lexicographical(
    SG_context * pCtx,
    const void * pArg1,
    const void * pArg2,
    void * pVoidCallerData
    )
{
    const char * sz1 = NULL;
    const char * sz2 = NULL;

    SG_UNUSED(pCtx);
    SG_ASSERT(pArg1!=NULL);
    SG_ASSERT(pArg2!=NULL);
    SG_UNUSED(pVoidCallerData);

    sz1 = *(const char * const *)pArg1;
    sz2 = *(const char * const *)pArg2;

    //todo: this is not what we want to do. See SPRAWL-783.
    return SG_stricmp(sz1, sz2);
}

/*

  TODO

  We may still have a problem with the issue of normalization.
  Ed was the one who first pointed out that Mac's notion of
  utf8 was different.  We assumed/understood that this meant
  that the Mac was simply using utf8 differently and that
  the utf8 encoding of a given set of codepoints is not
  unique and therefore comparison of utf8 strings must be
  done one codepoint at a time.

  But the information on this subject isn't clear and may
  need more research.  It seems that this may not be just
  a utf8 issue, but rather, a codepoint level issue, which
  involves the need to convert to normalized form.

  See the links below.  The Wikipedia article says that
  Mac uses NFD normalization whereas everybody else uses
  NFC normalization.

  http://developer.apple.com/qa/qa2001/qa1173.html

  http://en.wikipedia.org/wiki/UTF-8

  http://unicode.org/faq/normalization.html

  http://www.unicode.org/unicode/reports/tr15/
  http://unicode.org/reports/tr15/

  http://www.icu-project.org/userguide/normalization-ex.html

  http://www.flexiguided.de/publications.utf8proc.en.html

Also, here is some stuff from the subversion project:

[svn] / trunk / notes / unicode-composition-for-filenames 	Repository: svn
ViewVC logotype
View of /trunk/notes/unicode-composition-for-filenames

Parent Directory Parent Directory | Revision Log Revision Log
Revision 30048 - (download) (annotate)
Tue Mar 25 21:58:49 2008 UTC (2 weeks ago) by dionisos
File size: 8879 byte(s)

* notes/unicode-composition-for-filenames: New file describing NFC/NFD issues.

    1                                                                 -*- Text -*-
    2
    3
    4 Content
    5 =======
    6
    7  * Context
    8  * Issue description
    9  * Pre-resolution state of affairs
   10    - Single platform
   11    - Multi-platform: Windows + MacOS X
   12  * Proposed support library
   13    - Assumptions
   14    - Options
   15  * Proposed normal form
   16  * Possible solutions
   17    - Normalization of path-input on MacOS X
   18    - Normalization of path-input everywhere
   19    - Comparison routines (client side)
   20    - Comparison routines (everywhere)
   21  * Short term (ie before 2.0) solution
   22  * Long term solution (ie 2.0+)
   23  * References
   24
   25
   26 Context
   27 =======
   28
   29 Within Unicode, some characters - with diacritical marks - can be
   30 represented in 2 forms: Normal Form Composed (NFC) or Normal Form
   31 Decomposed (NFD).  A string of unicode characters can contain any
   32 mixture of both forms.
   33
   34 This problem explicitly does not concern itself with invisible
   35 characters, spaces or other characters unlikely to be present in
   36 filenames.  Please note that this issue is explicitly excluding
   37 NFKC/NFKD (compatibility) normal forms, because they remove
   38 for example formatting (meaning they are lossy?).
   39
   40
   41 Because there are 2 forms for representing (some) characters in Unicode,
   42 it's possible to produce different sequences of codepoints meaning to
   43 indicate the same sequence of characters [1].  UTF-8, the internal
   44 Unicode encoding of choice for Subversion, encodes codepoints in (a
   45 series of) bytes (octets).  Because the sequences of codepoints specifying
   46 a character may differ, so may the resulting UTF-8.  Hence, we end up
   47 with more than one way to specify the same path.
   48
   49
   50 The following table specifies behaviour of OSes related to handling
   51 of Unicode filenames:
   52
   53
   54           Accepts   Gives back    See
   55 MacOS X     *          NFD(*)     [2]
   56 Linux       *        <input>
   57 Windows     *        <input>
   58 Others      ?           ?
   59
   60 *) There are some remarks to be made regarding full or partial
   61   NFD here, but the essential thing is: If you send in NFC, don't
   62   expect it back!
   63
   64
   65 Issue description
   66 =================
   67
   68 From the above issue description, 2 problems follow:
   69
   70  1) We can't generally depend on the OS to give us back the
   71      exact filename we gave it
   72  2) The same filename may be encoded in different codepoints
   73
   74 Issue #1 is mainly a client side issue, something which might be
   75 resolved in the client side libraries (client/subr/wc).
   76
   77 Issue #2 is much broader than that, especially given the fact that
   78 we already have lots of populated repositories "out there": it means
   79 we cannot depend on a filename coming from the operating system - even
   80 though different from the one in the repository - to name a different
   81 file.  This has repository (ie. server-side) impact.
   82
   83
   84 Pre-resolution state of affairs
   85 ===============================
   86
   87 This section serves to describe the problems to be expected in different
   88 combinations of client/server OSes.  As indicated in the table in the
   89 context section, Linux and Windows are expected to behave equally. This
   90 section therefor leaves out the consideration of Linux as a separate
   91 system.
   92
   93 The platforms below are strictly client side: the server side problems
   94 mentioned in the issue description section solely relates to the repository,
   95 which can be located at any server platform.
   96
   97
   98 Single platform
   99 ---------------
  100 This can be multiple MacOSX machines or multiple Windows machines.  In this
  101 scenario, no interoperability problems are to be expected.
  102
  103
  104 Multi-platform: Windows + MacOSX
  105 --------------------------------
  106 Consider a file which contains one or more precomposed (NFC) characters
  107 being committed from Windows.  When the MacOSX developer updates, a
  108 file is written in NFC form, but as stated in the context section, Mac
  109 recodes that to NFD.  Now, when comparing what comes from the disk (NFD)
  110 with what's in the entries file (NFC), results in a missing file (the
  111 NFC encoded one) and an unversioned file (the NFD encoded one).  Both of
  112 these files look exactly the same to the person reading the Subversion
  113 output on the screen. [==> confusion!]
  114
  115 Committing a file the other way around might be less problematic, since
  116 Windows is capable of storing NFD filenames.
  117
  118
  119 Proposed support library
  120 ========================
  121
  122 Assumptions
  123 -----------
  124 The main assumption is that we'll keep using APR for character set
  125 conversion, meaning that the recoding solution to choose would not need
  126 to provide any other functionality than recoding.
  127
  128 Options
  129 -------
  130 There are 2 options (that I'm aware of [dionisos]) for choosing a library
  131 which supports the required functionality:
  132
  133 1) ICU - International Component for Unicode [3]
  134    a library with a very wide range of targeted functions, with a
  135    memory footprint to match.  In order to be able to use it, we'd need
  136    to trim this library down significantly.
  137 2) utf8proc - a library for processing UTF-8 encoded unicode strings
  138    a library specifically targeted at a limited number of operations
  139    to be performed on UTF-8 encoded strings.  It consists of 2 .c and
  140    1 .h file, with a total source size of 1MB (compiled less than 0.5MB).
  141
  142 From these 2, under the given assumption, it only makes sense to use
  143 utf8proc.
  144
  145
  146 Proposed normal form
  147 ====================
  148
  149 The proposed internal normal 'normal form' should be NFC, if only if it
  150 were because it's the most compact form of the two: when allocating memory
  151 to store a conversion result, it won't be necessary (ever) to allocate more
  152 than the size of the input buffer.
  153
  154 This would give the maximum performance from utf8proc, which requires 2
  155 recoding runs when the buffer is too small: 1 to retrieve the required
  156 buffer size, the second to actually store the result.
  157
  158
  159 Possible solutions
  160 ==================
  161
  162 Several options are available for resolution of this problem, each
  163 with its pros and cons, to be outlined below.
  164
  165  1) Normalization of (path) input on MacOSX
  166     Since the Mac seems to be the only platform which mutilates its
  167     pathname input to be NFD, this seems like a logical (low impact)
  168     solution.
  169  2) Normalization of (path) input on all platforms
  170     Since paths can't differ only in encoding if we standardize on
  171     encoding, this seems like a logical (relatively low) impact solution.
  172  3) Normalization of path input in the client and server
  173     On the server side, non-normalized paths may have become part
  174     of the repository.  We can achieve full in-memory standardization
  175     by converting any path coming from the repository as well as the
  176     client.
  177  4) Client and server-side path comparison routines
  178     Because paths read from the repository may be used to access said
  179     repository, possibly by calculating hash values, paths from can't be
  180     munged (repository-side).  To eliminate the effect, we acknowledge
  181     we're not going to be 'clean': we'll always need path comparison
  182     routines.
  183
  184 Solution (1) has a very strong CON: it will break all pre-existing
  185 MacOSX-only workshops.  Consider a client which starts sending NFC
  186 encoded paths in an environment where all paths have been NFD encoded
  187 until that time - without proper support in the server.  This would
  188 result in commits with NFC encoded paths to files for which the path
  189 in the repository is NFD encoded: breakage.
  190
  191 Solution (2) has the same problem as solution (1) on MacOSX, but
  192 on the upside it prevents new NFD paths from entering into the repository
  193 (for sufficiently broad definitions of 'client' [think mod_dav_svn]).
  194
  195 As already stated, solution (3) may prevent paths from being found, if
  196 the retrieval mechanism is hash-based.  Meaning this could break any
  197 repository backend using hashing to store information about paths.
  198 (Don't we store locks in FSFS based on hashing?)
  199
  200 Solution (4) defines no internal standard representation, assuming it's
  201 not possible to maintain a clean in-memory state, given all problems
  202 found in the earlier solutions.  Instead, it requires all path comparisons
  203 to be performed using special NFC/NFD encoding aware functions.
  204
  205
  206 Short term solution
  207 ===================
  208
  209 Given the above, the short term (before 2.0) solution should be to
  210 use path comparison routines as stated in solution (4).
  211
  212
  213 Long term solution
  214 ==================
  215
  216 The long term (2.0+) solution would be to use option (2), which ensures
  217 recoding of all input paths into the 'normal' normal form (NFC).  In that
  218 case, it'll no longer require the use of specialised path comparison
  219 routines (although that might still be desired for other design
  220 considerations).
  221
  222
  223
  224 References
  225 ==========
  226
  227 1) UAX #15: Unicode normalization forms
  228    http://unicode.org/reports/tr15/
  229 2) Apple Technical Q&A: Path encodings in VFS
  230    http://developer.apple.com/qa/qa2001/qa1173.html
  231 3) ICU - International Component for Unicode
  232    http://www-306.ibm.com/software/globalization/icu/index.jsp
  233 4) utf8proc - a library targeted at processing UTF-8 encoded unicode strings
  234    http://www.flexiguided.de/publications.utf8proc.en.html

svnadmin@svn.collab.net
	ViewVC Help
Powered by ViewVC 1.0.5


*/

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
// WARNING: wchar_t is ***NOT*** portable (it is different sizes on different
// WARNING: platforms).  Nothing in the library should use wchar_t strings
// WARNING: EXCEPT for code which calls the "W" version Windows-specific
// WARNING: filesystem operations (like CreateFileW).  This should be localized
// WARNING: to sg_dir, sg_file, and sg_fsobj.

void SG_utf8__extern_to_os_buffer__wchar(SG_context * pCtx,
										 const char * szUtf8,
										 wchar_t ** ppBufResult,	// caller needs to free this
										 SG_uint32 * pLenResult)
{
	// convert the given utf8 (null-terminated) string into a wchar
	// (null-terminated) unicode buffer.  also returns the length (in
	// wchars not bytes) of the resulting buffer (INCLUDING THE NULL).
	//
	// WARNING: this does basic character encoding conversion.  if you need
	// WARNING: any of the UNC stuff and/or are using this for windows
	// WARNING: pathnames, see SG_pathname__to_unc_wchar() instead.

	int result, lenNeeded;
	wchar_t * pBuf;

	SG_NONEMPTYCHECK_RETURN(szUtf8);
	SG_NULLARGCHECK_RETURN(ppBufResult);

	*ppBufResult = NULL;

	// computer size of the output buffer required.

	lenNeeded = MultiByteToWideChar(CP_UTF8,
									0,
									szUtf8, -1,
									NULL, 0);
	if (lenNeeded == 0)
	{
		SG_ERR_THROW_RETURN(  SG_ERR_GETLASTERROR(GetLastError())  );
	}

	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, lenNeeded,pBuf)  );		// caller must free this

	result = MultiByteToWideChar(CP_UTF8,
								 0,
								 szUtf8, -1,
								 pBuf,lenNeeded);
	if (result == 0)
	{
		SG_error errTemp = SG_ERR_GETLASTERROR(GetLastError());		// remember error from Multi...() not from free()
		SG_NULLFREE(pCtx, pBuf);
		SG_ERR_THROW_RETURN(  errTemp  );
	}

	*ppBufResult = pBuf;
	if (pLenResult)
		*pLenResult = result;
}

/**
 * Convert wchar_t buffer into an allocated UTF8 buffer.
 * The caller must free this buffer.
 * Optionally returns the length of the buffer <b>including the terminating
 * NULL in bytes</b> not in code-points.
 */
static void _sg_utf8__from_wchar(SG_context * pCtx,
								 const wchar_t * pBufUnicode,
								 char ** pszUtf8Result,		// caller needs to free this
								 SG_uint32 * pLenResult)
{
	// convert the given unicode (null-terminated) wchar string into a
	// (null-terminated) utf8 string.  also returns the length (in
	// bytes (not code-points)) of the resulting pathname (INCLUDING THE NULL).

	int result, lenNeeded;
	char * pBuf;

	SG_NONEMPTYCHECK_RETURN(pBufUnicode);
	SG_NULLARGCHECK_RETURN(pszUtf8Result);

	*pszUtf8Result = NULL;

	// computer size of the output buffer required.

	lenNeeded = WideCharToMultiByte(CP_UTF8,
									0,
									pBufUnicode, -1,
									NULL, 0,
									NULL, NULL);
	if (lenNeeded == 0)
	{
		SG_ERR_THROW_RETURN(  SG_ERR_GETLASTERROR(GetLastError())  );
	}

	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, lenNeeded,pBuf)  );		// caller must free this

	result = WideCharToMultiByte(CP_UTF8,
								 0,
								 pBufUnicode, -1,
								 pBuf, lenNeeded,
								 NULL, NULL);
	if (result == 0)
	{
		SG_error errTemp = SG_ERR_GETLASTERROR(GetLastError());		// remember error from Multi...() not from free()
		SG_NULLFREE(pCtx, pBuf);
		SG_ERR_THROW_RETURN(  errTemp  );
	}

	*pszUtf8Result = pBuf;
	if (pLenResult)
		*pLenResult = result;
}

/**
 * We assume that the caller used a "W" version of a Win32 API routine
 * to generate this wchar_t string and that it contains (Windows version
 * of) UTF-16/UCS-2 data.  We assume that this is NFC.
 *
 * NOTE: Technically, it is possible that this might be NFD but we assume
 * that no one in their right minds would use NFD (since it looks like
 * garbage in Windows Explorer).  If they do give us NFD, we support it
 * (and will later warn them during the portability/collider checks).
 * It may bite them, but we permit it.
 *
 * Further, we assume that since we have UTF-16/UCS-2 data, it is completely
 * locale independent.
 *
 */
void SG_utf8__intern_from_os_buffer__wchar(SG_context* pCtx, SG_string * pString, const wchar_t * pBufUcs2)
{
	char * szUtf8 = NULL;
	SG_uint32 lenUtf8Buffer;

	SG_NULLARGCHECK_RETURN(pString);
	SG_NULLARGCHECK_RETURN(pBufUcs2);

	if (!*pBufUcs2)
	{
		SG_ERR_CHECK_RETURN(  SG_string__clear(pCtx,pString)  );
		return;
	}

	// convert UTF-16/UCS-2 wchar_t data into UTF-8.  This will allocate a
	// buffer that we must free.  The length returned includes the trailing NULL.

	SG_ERR_CHECK(  _sg_utf8__from_wchar(pCtx, pBufUcs2,&szUtf8,&lenUtf8Buffer)  );

	// let the string adopt the buffer.  this is like a SG_string__set__sz() but
	// lets us avoid another malloc and memcpy.

	SG_ERR_CHECK(  SG_string__adopt_buffer(pCtx, pString,szUtf8,lenUtf8Buffer)  );
	szUtf8 = NULL;

	return;

fail:
	SG_NULLFREE(pCtx, szUtf8);
}

#endif

//////////////////////////////////////////////////////////////////

void SG_utf8__to_utf32__sz(SG_context * pCtx,
						   const char * szUtf8,
						   SG_int32 ** ppBufResult,	// caller needs to free this
						   SG_uint32 * pLenResult)
{
	// TODO see about replacing the body of this function
	// with a call to an ICU function.

	SG_uint32 len_bytes = SG_utf8__length_in_bytes(szUtf8);

	SG_ERR_CHECK_RETURN(  SG_utf8__to_utf32__buflen(pCtx, szUtf8, len_bytes, ppBufResult, pLenResult)  );
}

void SG_utf8__to_utf32__buflen(SG_context * pCtx,
							   const char * szUtf8,
							   SG_uint32 len_bytes_input,
							   SG_int32 ** ppBufResult,	// caller needs to free this
							   SG_uint32 * pLenResult)
{
	// TODO see about replacing the body of this function
	// with a call to an ICU function.

	/*
	  convert the given utf8 (null-terminated) string into a null-terminated
	  utf32 string.  also returns the length (in characters not bytes) of the
	  resulting buffer (NOT including the null).
	*/

	SG_uint32 len_chars = 0;
	SG_int32* pBuf32;
	SG_uint32 i;
	int32_t ndx;
	int32_t len_bytes = (int32_t)len_bytes_input;

	SG_NONEMPTYCHECK_RETURN(szUtf8);
	SG_NULLARGCHECK_RETURN(ppBufResult);

	SG_ERR_CHECK_RETURN(  SG_utf8__length_in_characters__buflen(pCtx,
																szUtf8,
																len_bytes_input,
																&len_chars)  );
	if (len_chars == 0)
		SG_ERR_THROW_RETURN(  SG_ERR_UNSPECIFIED  );

	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, len_chars+1,pBuf32)  );		// caller must free this

	for (i=0, ndx=0; i<len_chars; i++)
	{
		SG_int32 c;

		U8_NEXT(szUtf8, ndx, len_bytes, c);

		pBuf32[i] = c;
	}
	pBuf32[i] = 0;

	*ppBufResult = pBuf32;
	if (pLenResult)
	{
		*pLenResult = len_chars;
	}
}

void SG_utf8__from_utf32(SG_context * pCtx,
						 const SG_int32 * pBufUnicode,
						 char ** pszUtf8Result,		// caller needs to free this
						 SG_uint32 * pLenResult)
{
	// TODO see about replacing the body of this function
	// with a call to an ICU function.

	/*
	  convert the given utf-32 (null-terminated) string into a
	  (null-terminated) utf8 string.  also returns the length (in
	  bytes (not code-points)) of the resulting pathname (including the null).
	*/

	int32_t len_bytes;
	uint8_t * pBuf8;
	int32_t i;
	int32_t ndx;
	int32_t * pBuf32 = (int32_t *)pBufUnicode;

	SG_NONEMPTYCHECK_RETURN(pBufUnicode);
	SG_NULLARGCHECK_RETURN(pszUtf8Result);

	*pszUtf8Result = NULL;

	len_bytes = 0;

	for (i=0; pBuf32[i]; i++)
	{
		int32_t len_this_codepoint = U8_LENGTH(pBuf32[i]);
		len_bytes += len_this_codepoint;
	}

	len_bytes += 1; // for the terminating null

	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, (SG_uint32)len_bytes,pBuf8)  );		// caller must free this

	for (i=0, ndx=0; pBuf32[i]; i++)
	{
		UBool bError = FALSE;
		U8_APPEND(pBuf8, ndx, len_bytes, pBuf32[i], bError);
		if (bError)
		{
			SG_NULLFREE(pCtx, pBuf8);
			SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  );
		}
	}

	*pszUtf8Result = (char *)pBuf8;
	if (pLenResult)
	{
		*pLenResult = (SG_uint32)len_bytes;
	}
}

//////////////////////////////////////////////////////////////////

void SG_utf8__sprintf_utf8_with_escape_sequences(SG_context * pCtx,
												 const char * szUtf8In,
												 char ** ppBufReturned)
{
	// convert the given string into a minimal c-style escaped string.
	// that is, create a "\xXX" character sequence for everything above
	// 0x7f and for all non-printables (control chars and DEL) on the
	// US-ASCII page.
	//
	// we will allocate a buffer to contain the formatted version.  You
	// must free this.

	const char * szIn;
	char * pBufReturned = NULL;
	char * pBuf;
	SG_uint32 lenNeeded;

	SG_NULLARGCHECK_RETURN(szUtf8In);

	lenNeeded = 0;
	szIn = szUtf8In;
	while (*szIn)
	{
		if ((*szIn >= 0x20)  && (*szIn < 0x7f))
			lenNeeded++;
		else
			lenNeeded += 4;
		szIn++;
	}
	lenNeeded++;		// for null

	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, lenNeeded,pBufReturned)  );

	szIn = szUtf8In;
	pBuf = pBufReturned;
	while (*szIn)
	{
		if ((*szIn >= 0x20)  && (*szIn < 0x7f))
		{
			*pBuf++ = *szIn++;
		}
		else
		{
			*pBuf++ = '\\';
			*pBuf++ = 'x';
			pBuf = SG_hex__format_uint8(pBuf,(SG_uint8)*szIn++);
		}
	}
	*pBuf = 0;

	*ppBufReturned = pBufReturned;
}

//////////////////////////////////////////////////////////////////

void SG_utf8__next_char(
	SG_context* pCtx,
	SG_byte*    pBytes,
	SG_uint32   uSize,
	SG_uint32   uIndex,
	SG_int32*   pChar,
	SG_uint32*  pIndex
	)
{
	SG_int32 iSize  = (SG_int32)uSize;
	SG_int32 iIndex = (SG_int32)uIndex;
	SG_int32 iChar  = 0;

	SG_UNUSED(pCtx);

	U8_NEXT(pBytes, iIndex, iSize, iChar);

	if (pChar != NULL)
	{
		*pChar = iChar;
	}
	if (pIndex != NULL)
	{
		*pIndex = (SG_uint32)iIndex;
	}
}

void SG_utf8__foreach_character(SG_context * pCtx,
								const char * szUtf8,
								SG_utf8__foreach_character_callback * pfn,
								void * pVoidCallbackData)
{
	// apply pfn to each character c in string.
	//
	// where the input is a UTF-8 string and we call the function
	// on each character (not byte) as a 32-bit unicode character.

	int32_t i, len;

	SG_NULLARGCHECK_RETURN(szUtf8);
	SG_NULLARGCHECK_RETURN(pfn);

	i = 0;
	len = (int32_t)SG_utf8__length_in_bytes(szUtf8);
	while (i<len)
	{
		SG_int32 c;

		U8_NEXT(szUtf8, i, len, c);

		if (c == 0)
			return;

		SG_ERR_CHECK_RETURN(  (*pfn)(pCtx,(SG_uint32)c,pVoidCallbackData)  );
	}
}

//////////////////////////////////////////////////////////////////

void SG_utf32__to_lower_char(SG_context * pCtx, SG_int32 ch32In, SG_int32 * pCh32Out)
{
	SG_NULLARGCHECK_RETURN(pCh32Out);

	*pCh32Out = (SG_int32)u_tolower((UChar32)ch32In);
}

//////////////////////////////////////////////////////////////////

/**
 * Allocate and setup an ICU Character Set converter for the given
 * character set.
 *
 */
void SG_utf8_converter__alloc(SG_context * pCtx,
							  const char * szCharSetName,
							  SG_utf8_converter ** ppConverter)
{
	UErrorCode ue;
	SG_utf8_converter * pConverter = NULL;

	SG_NULLARGCHECK_RETURN(ppConverter);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pConverter)  );

	if (!szCharSetName || !*szCharSetName || (ucnv_compareNames(szCharSetName,"utf8")==0))
	{
		pConverter->bIsCharSetUtf8 = SG_TRUE;
		SG_ERR_CHECK(  SG_STRDUP(pCtx, "utf8",&pConverter->szCharSetName)  );
	}
	else
	{
		pConverter->bIsCharSetUtf8 = SG_FALSE;
		SG_ERR_CHECK(  SG_STRDUP(pCtx, szCharSetName,&pConverter->szCharSetName)  );
		SG_UE_ERR_CHECK(  pConverter->pIcuConverter = ucnv_open(szCharSetName,&ue)  );
		SG_UE_ERR_CHECK(  ucnv_setFromUCallBack(pConverter->pIcuConverter, UCNV_FROM_U_CALLBACK_STOP,NULL,NULL,NULL,&ue)  );
		SG_UE_ERR_CHECK(  ucnv_setToUCallBack(pConverter->pIcuConverter, UCNV_TO_U_CALLBACK_STOP,NULL,NULL,NULL,&ue)  );
	}

	*ppConverter = pConverter;

	return;

fail:
	SG_UTF8_CONVERTER_NULLFREE(pCtx, pConverter);
}

#if defined(LINUX)
void SG_utf8_converter__alloc_substituting(SG_context * pCtx,
										   const char * szCharSetName,
										   SG_utf8_converter ** ppConverter)
{
	// create a substituting converter that will use ICU-style "%UXXXX"
	// sequences for undefined/illegal characters.  non-bmp characters
	// create a surrogate pair %UD84D%UDC56.

	UErrorCode ue;
	SG_utf8_converter * pConverter = NULL;

	SG_NULLARGCHECK_RETURN(ppConverter);
	SG_NONEMPTYCHECK_RETURN(szCharSetName);

	if (ucnv_compareNames(szCharSetName,"utf8")==0)
		SG_ERR_THROW_RETURN(  SG_ERR_INVALIDARG  );

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pConverter)  );

	pConverter->bIsCharSetUtf8 = SG_FALSE;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, szCharSetName,&pConverter->szCharSetName)  );
	SG_UE_ERR_CHECK(  pConverter->pIcuConverter = ucnv_open(szCharSetName,&ue)  );
	SG_UE_ERR_CHECK(  ucnv_setFromUCallBack(pConverter->pIcuConverter, UCNV_FROM_U_CALLBACK_ESCAPE,UCNV_ESCAPE_ICU,NULL,NULL,&ue)  );
	SG_UE_ERR_CHECK(  ucnv_setToUCallBack(pConverter->pIcuConverter, UCNV_TO_U_CALLBACK_ESCAPE,UCNV_ESCAPE_ICU,NULL,NULL,&ue)  );

	*ppConverter = pConverter;

	return;

fail:
	SG_UTF8_CONVERTER_NULLFREE(pCtx, pConverter);
}
#endif

void SG_utf8_converter__free(SG_context * pCtx, SG_utf8_converter * pConverter)
{
	if (!pConverter)
		return;

	SG_NULLFREE(pCtx, pConverter->szCharSetName);

	if (pConverter->pIcuConverter)
	{
		ucnv_close(pConverter->pIcuConverter);
		pConverter->pIcuConverter = NULL;
	}

	SG_free(pCtx, pConverter);
}

void SG_utf8_converter__is_charset_utf8(SG_context * pCtx,
										const SG_utf8_converter * pConverter,
										SG_bool * pbIsCharSetUtf8)
{
	SG_NULLARGCHECK_RETURN(pConverter);
	SG_NULLARGCHECK_RETURN(pbIsCharSetUtf8);

	// We only actually create an ICU converter when we need to convert to/from a
	// legacy character encoding.  If we were created for UTF-8, then we don't
	// need an actual ICU converter.

	SG_ASSERT( ((pConverter->bIsCharSetUtf8 == SG_TRUE) == (pConverter->pIcuConverter == NULL)) );

	*pbIsCharSetUtf8 = pConverter->bIsCharSetUtf8;
}

SG_bool SG_utf8_converter__utf8(const SG_utf8_converter * pConverter)
{
	return pConverter!=NULL && pConverter->bIsCharSetUtf8;
}

void SG_utf8_converter__get_canonical_name(SG_context * pCtx,
										   const SG_utf8_converter * pConverter,
										   const char ** pszCanonicalName)
{
	UErrorCode ue;
	const char * p;

	SG_NULLARGCHECK_RETURN(pConverter);
	SG_NULLARGCHECK_RETURN(pszCanonicalName);

	if (pConverter->bIsCharSetUtf8)
		p = pConverter->szCharSetName;
	else
		SG_UE_ERR_CHECK(  p = ucnv_getName(pConverter->pIcuConverter,&ue)  );

	*pszCanonicalName = p;
	return;

fail:
	return;
}

void SG_utf8_converter__from_charset__sz(SG_context * pCtx,
										 SG_utf8_converter * pConverter,
										 const char * szCharSetBuffer,
										 char ** pszUtf8BufferResult)
{
	UErrorCode ue;
	char * pBufAllocated = NULL;

	SG_NULLARGCHECK_RETURN(pConverter);
	SG_NULLARGCHECK_RETURN(szCharSetBuffer);
	SG_NULLARGCHECK_RETURN(pszUtf8BufferResult);

	if (pConverter->bIsCharSetUtf8)
	{
		// the CharSet is UTF-8, so we don't have to do any conversion.
		// all we need to do is validate and duplicate it.
		//
		// we cheat and use the length_in_characters function
		// because it will validate as it counts.

		SG_uint32 lenInputCharsUnused;

		SG_ERR_CHECK(  SG_utf8__length_in_characters__sz(pCtx, szCharSetBuffer, &lenInputCharsUnused)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, szCharSetBuffer,&pBufAllocated)  );
	}
	else
	{
		// the CharSet is not UTF-8, so we have to actually do a conversion.

		int32_t lenSrcBytes = (int32_t)strlen(szCharSetBuffer);
		int32_t lenDest;

		ue = U_ZERO_ERROR;
		lenDest = ucnv_toAlgorithmic(UCNV_UTF8,
									 pConverter->pIcuConverter,
									 NULL,0,
									 szCharSetBuffer,lenSrcBytes,
									 &ue);
		if (ue == U_INVALID_CHAR_FOUND || ue == U_TRUNCATED_CHAR_FOUND || ue == U_ILLEGAL_CHAR_FOUND)
			SG_ERR_THROW(  SG_ERR_ILLEGAL_CHARSET_CHAR  );

		if ((ue != U_ZERO_ERROR) && (ue != U_BUFFER_OVERFLOW_ERROR))
			SG_ERR_THROW(  SG_ERR_ICU(ue)  );

		SG_ERR_CHECK(  SG_allocN(pCtx, lenDest+2,pBufAllocated)  );

		ue = U_ZERO_ERROR;
		SG_UE_ERR_CHECK(  lenDest = ucnv_toAlgorithmic(UCNV_UTF8,
													   pConverter->pIcuConverter,
													   pBufAllocated,lenDest+1,
													   szCharSetBuffer,lenSrcBytes,
													   &ue)  );
	}

	*pszUtf8BufferResult = pBufAllocated;
	pBufAllocated = NULL;

	return;

fail:
	SG_NULLFREE(pCtx, pBufAllocated);
}

void SG_utf8_converter__from_charset__buf_len(
    SG_context * pCtx,
    SG_utf8_converter * pConverter,
    const SG_byte * pBuffer,
    SG_uint32 bufferLength,
    char ** ppResult)
{
    char * pResult = NULL;
	char * pResultNoBOM = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pConverter);
    SG_NULLARGCHECK_RETURN(pBuffer);
    SG_NULLARGCHECK_RETURN(ppResult);

    if (pConverter->bIsCharSetUtf8)
    {
        // the CharSet is UTF-8, so we don't have to do any conversion.
        // all we need to do is validate and duplicate it.
        //
        // we cheat and use the length_in_characters function
        // because it will validate as it counts.

        SG_uint32 dummy;

		/* Serialized data may have signatures, but in-memory strings shouldn't:
		http://userguide.icu-project.org/unicode#TOC-Serialized-Formats
		So we strip the UTF-8 BOM, if it's there. 
		This code is intentionally duplicated below, because we can spare ourselves
		the extra malloc and memcpy in this pass-through case.
		*/
		SG_STATIC_ASSERT(3 == sizeof(UTF8_BOM));
		if ( (bufferLength >= 3) && 
			 (pBuffer[0] == UTF8_BOM[0]) && 
			 (pBuffer[1] == UTF8_BOM[1]) && 
			 (pBuffer[2] == UTF8_BOM[2]))
		{
			pBuffer += 3;
			bufferLength -= 3;
		}

        SG_ERR_CHECK(  SG_allocN(pCtx, bufferLength+1, pResult)  );
        memcpy(pResult, pBuffer, bufferLength);
        pResult[bufferLength] = '\0';

        SG_utf8__length_in_characters__sz(pCtx, pResult, &dummy);

        if(SG_context__err_equals(pCtx, SG_ERR_UTF8INVALID))
            SG_ERR_RESET_THROW(SG_ERR_ILLEGAL_CHARSET_CHAR);
        else
            SG_ERR_CHECK_CURRENT;
    }
    else
    {
        // the CharSet is not UTF-8, so we have to actually do a conversion.
        int32_t lenDest;
        UErrorCode ue = U_ZERO_ERROR;

        lenDest = ucnv_toAlgorithmic(
            UCNV_UTF8,
            pConverter->pIcuConverter,
            NULL,0,
            (const char *)pBuffer, bufferLength,
            &ue);

        if(ue==U_INVALID_CHAR_FOUND || ue==U_TRUNCATED_CHAR_FOUND || ue==U_ILLEGAL_CHAR_FOUND)
            SG_ERR_THROW(SG_ERR_ILLEGAL_CHARSET_CHAR);

        if((ue!=U_ZERO_ERROR) && (ue!=U_BUFFER_OVERFLOW_ERROR))
            SG_ERR_THROW(  SG_ERR_ICU(ue)  );

        SG_ERR_CHECK(  SG_allocN(pCtx, lenDest+1, pResult)  );

        ue = U_ZERO_ERROR;
        SG_UE_ERR_CHECK(  lenDest = ucnv_toAlgorithmic(
            UCNV_UTF8,
            pConverter->pIcuConverter,
            pResult,lenDest,
            (const char *)pBuffer,bufferLength,
            &ue)  );

		pResult[lenDest] = '\0';

		/* Serialized data may have signatures, but in-memory strings shouldn't:
		http://userguide.icu-project.org/unicode#TOC-Serialized-Formats
		So we strip the UTF-8 BOM, if it's there. 
		This code is intentionally duplicated above, because we can spare ourselves
		the extra malloc and memcpy in that pass-through case.
		*/
		if ( (lenDest >= 3) && 
			 (((SG_byte)pResult[0]) == UTF8_BOM[0]) && 
			 (((SG_byte)pResult[1]) == UTF8_BOM[1]) && 
			 (((SG_byte)pResult[2]) == UTF8_BOM[2]))
		{
			lenDest -= 3;
			SG_ERR_CHECK(  SG_allocN(pCtx, lenDest+1, pResultNoBOM)  );
			memcpy(pResultNoBOM, ((SG_byte*)pResult) + 3, lenDest);
			pResultNoBOM[lenDest]= '\0';
			SG_NULLFREE(pCtx, pResult);
			pResult = pResultNoBOM;
		}
	}

    *ppResult = pResult;

    return;
fail:
    SG_NULLFREE(pCtx, pResult);
	SG_NULLFREE(pCtx, pResultNoBOM);
}

void SG_utf8_converter__to_charset__sz__sz(SG_context * pCtx,
										   SG_utf8_converter * pConverter,
										   const char * szUtf8Buffer,
										   char ** pszCharSetBufferResult)
{
	// convert a UTF-8 string into a CharSet/CodePage-based string.

	UErrorCode ue;
	char * pBufAllocated = NULL;

	SG_NULLARGCHECK_RETURN(pConverter);
	SG_NULLARGCHECK_RETURN(szUtf8Buffer);
	SG_NULLARGCHECK_RETURN(pszCharSetBufferResult);

	if (pConverter->bIsCharSetUtf8)
	{
		// the CharSet is UTF-8, so we don't have to do any conversion.
		// all we need to do is duplicate it.
		//
		// TODO we really don't need to validate it because we assume that
		// TODO it was validated when internalized into UTF-8, but let's
		// TODO do a little sanity checking during debugging.

		SG_uint32 lenInputCharsUnused;

		SG_ERR_CHECK(  SG_utf8__length_in_characters__sz(pCtx, szUtf8Buffer, &lenInputCharsUnused)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, szUtf8Buffer,&pBufAllocated)  );
	}
	else
	{
		// the CharSet is not UTF-8, so we have to actually do a conversion.

		int32_t lenSrcBytes = (int32_t)strlen(szUtf8Buffer);
		int32_t lenDest;

		ue = U_ZERO_ERROR;
		lenDest = ucnv_fromAlgorithmic(pConverter->pIcuConverter,
									   UCNV_UTF8,
									   NULL,0,
									   szUtf8Buffer,lenSrcBytes,
									   &ue);
		if (ue == U_INVALID_CHAR_FOUND)
			SG_ERR_THROW(  SG_ERR_UNMAPPABLE_UNICODE_CHAR  );

		if ((ue != U_ZERO_ERROR) && (ue != U_BUFFER_OVERFLOW_ERROR))
			SG_ERR_THROW(  SG_ERR_ICU(ue)  );

		SG_ERR_CHECK(  SG_allocN(pCtx, lenDest+2,pBufAllocated)  );

		ue = U_ZERO_ERROR;
		SG_UE_ERR_CHECK(  lenDest = ucnv_fromAlgorithmic(pConverter->pIcuConverter,
														 UCNV_UTF8,
														 pBufAllocated,lenDest+1,
														 szUtf8Buffer,lenSrcBytes,
														 &ue)  );
	}

	*pszCharSetBufferResult = pBufAllocated;
	pBufAllocated = NULL;

	return;

fail:
	SG_NULLFREE(pCtx, pBufAllocated);
}

void SG_utf8_converter__to_charset__string__buf_len(
	SG_context * pCtx,
	SG_utf8_converter * pConverter,
	SG_string * pString,
	SG_byte ** ppBuf,
	SG_uint32 * pBufLen
	)
{
	UErrorCode ue;
	char * pBufAllocated = NULL;
	SG_uint32 lenBufAllocated = 0;

	SG_NULLARGCHECK_RETURN(pConverter);
	SG_NULLARGCHECK_RETURN(pString);
	SG_NULLARGCHECK_RETURN(ppBuf);
	SG_NULLARGCHECK_RETURN(pBufLen);

	if (pConverter->bIsCharSetUtf8)
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pString), &pBufAllocated)  );
		lenBufAllocated = SG_STRLEN(pBufAllocated);
	}
	else
	{
		SG_int32 lenSrcBytes;
		int32_t lenDest;
		
		if(SG_string__length_in_bytes(pString)>=SG_INT32_MAX)
			SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "String is too long to be encoded by ICU."));
		lenSrcBytes = (SG_int32)SG_string__length_in_bytes(pString);

		ue = U_ZERO_ERROR;
		lenDest = ucnv_fromAlgorithmic(pConverter->pIcuConverter,
									   UCNV_UTF8,
									   NULL,0,
									   SG_string__sz(pString),lenSrcBytes,
									   &ue);
		if (ue == U_INVALID_CHAR_FOUND)
			SG_ERR_THROW(  SG_ERR_UNMAPPABLE_UNICODE_CHAR  );

		if ((ue != U_ZERO_ERROR) && (ue != U_BUFFER_OVERFLOW_ERROR))
			SG_ERR_THROW(  SG_ERR_ICU(ue)  );

		SG_ERR_CHECK(  SG_allocN(pCtx, lenDest, pBufAllocated)  );

		ue = U_ZERO_ERROR;
		SG_UE_ERR_CHECK(  lenDest = ucnv_fromAlgorithmic(pConverter->pIcuConverter,
														 UCNV_UTF8,
														 pBufAllocated,lenDest,
														 SG_string__sz(pString),lenSrcBytes,
														 &ue)  );
		lenBufAllocated = (SG_uint32)lenDest;
	}

	*ppBuf = (SG_byte*)pBufAllocated;
	*pBufLen = lenBufAllocated;

	return;

fail:
	SG_NULLFREE(pCtx, pBufAllocated);
}

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

void SG_utf8__is_locale_env_charset_utf8(SG_context * pCtx,
										 SG_bool * pbIsEnvCharSetUtf8)
{
	sg_lib_utf8__global_data * pData;

	SG_ERR_CHECK_RETURN(  sg_lib__fetch_utf8_global_data(pCtx, &pData)  );

	SG_ERR_CHECK_RETURN(  SG_utf8_converter__is_charset_utf8(pCtx,
															 pData->pLocaleEnvCharSetConverter,
															 pbIsEnvCharSetUtf8)  );
}

void SG_utf8__get_locale_env_charset_canonical_name(SG_context * pCtx,
													const char ** pszCanonicalName)
{
	sg_lib_utf8__global_data * pData;

	SG_ERR_CHECK_RETURN(  sg_lib__fetch_utf8_global_data(pCtx, &pData)  );

	SG_ERR_CHECK_RETURN(  SG_utf8_converter__get_canonical_name(pCtx,
																pData->pLocaleEnvCharSetConverter,
																pszCanonicalName)  );
}

void SG_utf8__from_locale_env_charset__sz(SG_context * pCtx,
										  const char * szCharSetBuffer,
										  char ** pszUtf8BufferResult)
{
	sg_lib_utf8__global_data * pData;

	SG_ERR_CHECK_RETURN(  sg_lib__fetch_utf8_global_data(pCtx, &pData)  );

	SG_ERR_CHECK_RETURN(  SG_utf8_converter__from_charset__sz(pCtx,
															  pData->pLocaleEnvCharSetConverter,
															  szCharSetBuffer,
															  pszUtf8BufferResult)  );
}

void SG_utf8__to_locale_env_charset__sz(SG_context * pCtx,
										const char * szUtf8Buffer,
										char ** pszCharSetBufferResult)
{
	sg_lib_utf8__global_data * pData;

	SG_ERR_CHECK_RETURN(  sg_lib__fetch_utf8_global_data(pCtx, &pData)  );

	SG_ERR_CHECK_RETURN(  SG_utf8_converter__to_charset__sz__sz(pCtx,
																pData->pLocaleEnvCharSetConverter,
																szUtf8Buffer,
																pszCharSetBufferResult)  );
}

#if defined(LINUX) && defined(DEBUG)
void SG_utf8_debug__set_locale_env_charset(SG_context * pCtx,
										   const char * szCharSetName)
{
	SG_utf8_converter * pConverterNew = NULL;
	SG_utf8_converter * pConverterOld = NULL;
	sg_lib_utf8__global_data * pData;

	SG_ERR_CHECK_RETURN(  sg_lib__fetch_utf8_global_data(pCtx, &pData)  );

	SG_ERR_CHECK_RETURN(  SG_utf8_converter__alloc(pCtx, szCharSetName,&pConverterNew)  );

	pConverterOld = pData->pLocaleEnvCharSetConverter;
	pData->pLocaleEnvCharSetConverter = pConverterNew;

	SG_UTF8_CONVERTER_NULLFREE(pCtx, pConverterOld);

	// clear the substituting converter and again wait until
	// we need it to recreate it.

	SG_UTF8_CONVERTER_NULLFREE(pCtx, pData->pSubstitutingLocaleEnvCharSetConverter);
}
#endif

//////////////////////////////////////////////////////////////////

const char * SG_utf8__get_icu_error_msg(SG_error e)
{
	UErrorCode ue = (UErrorCode)SG_ERR_ICU_VALUE(e);

	return u_errorName(ue);
}

//////////////////////////////////////////////////////////////////

#if defined(LINUX)
/**
 * Intern an OS-supplied string (might be an entryname or pathname).
 *
 * On Linux, getcwd(), readdir(), and etc give us the raw 8-bit entryname
 * as stored on disk without any interpretation.  The entrynames might be
 * locale-based or might be UTF-8 -- that is, what the user sees when they
 * type /bin/ls is a feature of /bin/ls and depends upon their locale settings.
 *
 * Internally, we always try to use UTF-8 NFC.
 *
 * Since NFD is only defined for Unicode and not for legacy character encodings
 * (and no one in their right minds would use UTF-8 NFD on Linux), we only
 * worry about converting the incomming entryname from the locale to UTF-8.
 *
 * If the user's locale is UTF-8, we accept the string as is ***assuming***
 * that it is NFC.  NOTE: This technically allows them to create a NFD UTF-8
 * entryname and us accept it.  We support this, but will warn them later
 * during the portability/collider checks -- it may bite them later, but we
 * let them do it.
 *
 */
static void _sg_utf8_locale__intern_from_os_buffer__sz(SG_context * pCtx,
													   SG_string * pString,
													   const char * szOS)
{
	SG_bool bEnvIsUtf8;
	char * szUtf8 = NULL;

	SG_ERR_CHECK_RETURN(  SG_utf8__is_locale_env_charset_utf8(pCtx, &bEnvIsUtf8)  );
	if (bEnvIsUtf8)
	{
		SG_ERR_CHECK_RETURN(  SG_string__set__sz(pCtx,pString,szOS)  );
		return;
	}

	SG_ERR_CHECK_RETURN(  SG_utf8__from_locale_env_charset__sz(pCtx, szOS,&szUtf8)  );

	// let the string adopt the buffer.  this is like a SG_string__set__sz() but
	// lets us avoid another malloc and memcpy.

	SG_ERR_CHECK(  SG_string__adopt_buffer(pCtx, pString,szUtf8,strlen(szUtf8)+1)  );
	szUtf8 = NULL;

	return;

fail:
	SG_NULLFREE(pCtx, szUtf8);
}
#endif

//////////////////////////////////////////////////////////////////

#if defined(LINUX)
void SG_utf8__extern_to_os_buffer_with_substitutions(SG_context * pCtx,
													 const char * szUtf8,
													 char ** pszOS)
{
	SG_bool bEnvIsUtf8;
	sg_lib_utf8__global_data * pData;
	const char * szCanonicalName;

	SG_NULLARGCHECK_RETURN(szUtf8);
	SG_NULLARGCHECK_RETURN(pszOS);

	SG_ERR_CHECK_RETURN(  SG_utf8__is_locale_env_charset_utf8(pCtx, &bEnvIsUtf8)  );

	if (bEnvIsUtf8)
	{
		SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx,szUtf8,pszOS)  );
		return;
	}

	SG_ERR_CHECK_RETURN(  sg_lib__fetch_utf8_global_data(pCtx, &pData)  );

	if (!pData->pSubstitutingLocaleEnvCharSetConverter)
	{
		SG_ERR_CHECK_RETURN(  SG_utf8__get_locale_env_charset_canonical_name(pCtx, &szCanonicalName)  );
		SG_ERR_CHECK_RETURN(  SG_utf8_converter__alloc_substituting(pCtx,
																	szCanonicalName,
																	&pData->pSubstitutingLocaleEnvCharSetConverter)  );
	}

	SG_ERR_CHECK_RETURN(  SG_utf8_converter__to_charset__sz__sz(pCtx,
																pData->pSubstitutingLocaleEnvCharSetConverter,
																szUtf8,
																pszOS)  );
}
#endif

//////////////////////////////////////////////////////////////////

void SG_utf8__import_buffer(
	SG_context * pCtx,
	const SG_byte * pBuf_unsigned,
	SG_uint32 bufLen,
	char ** ppSz,
	SG_encoding * pEncoding
	)
{
	const signed char * pBuf_signed = (const signed char *)pBuf_unsigned;
	SG_bool zeros = SG_FALSE;
	SG_bool success = SG_FALSE;
	char * pSz = NULL;
	const char * szEncodingName = NULL; // Pointer we don't own.
	SG_encoding encoding;
	
	UErrorCode ue = U_ZERO_ERROR;
	UCharsetDetector * ucsd = NULL;
	
	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pBuf_unsigned);
	SG_NULLARGCHECK_RETURN(ppSz);
	
	SG_zero(encoding);
	
	// First check for a signature.
	SG_UE_ERR_CHECK(  szEncodingName = ucnv_detectUnicodeSignature((const /*unqualified*/ char *)pBuf_unsigned, bufLen, NULL, &ue)  );
#if TRACE_UTF8
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Utf8:ImportBuffer: EncodingSignature: %s\n",
							   ((szEncodingName) ? szEncodingName : "(*none*)"))  );
#endif
	if(szEncodingName!=NULL)
	{
		_sg_utf8__try_import(pCtx, pBuf_unsigned, bufLen, szEncodingName, &pSz, &encoding.pConverter);
		if(!SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_ERR_CHECK(  SG_STRDUP(pCtx, szEncodingName, &encoding.szName)  );
			encoding.signature = SG_TRUE;
			success = SG_TRUE;
		}
		else if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
			SG_context__err_reset(pCtx);
		else
			SG_ERR_RETHROW;
	}
	
	// No signature found? Next try a few encodings that should reliably fail if incorrect.
	if(!success)
	{
		// Check 7-bit ASCII first since SG_textfilediff3 treats this as a special case.
		SG_uint32 i = 0;
		while(i<bufLen && pBuf_signed[i]>0)
			++i;
#if TRACE_UTF8
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Utf8:ImportBuffer: [i %d][len %d] before US-ASCII\n", i, bufLen)  );
#endif
		if(i==bufLen)
		{
			SG_ERR_CHECK(  _sg_utf8__try_import(pCtx, pBuf_unsigned, bufLen, "US-ASCII", &pSz, &encoding.pConverter)  );
			SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter); // Special case: null this out.
			encoding.szName = NULL; // Special case: set to NULL.
			success = SG_TRUE;
		}
		else while(i<bufLen && !zeros)
		{
			zeros = (pBuf_unsigned[i]==0);
			++i;
		}
	}
	if(!success && !zeros)
	{
		// Next check UTF-8, but only if there were no zeros (seems to terminate the input rather than triggering it as being invalid).
		_sg_utf8__try_import(pCtx, pBuf_unsigned, bufLen, "UTF-8", &pSz, &encoding.pConverter);
		if(!SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_ERR_CHECK(  SG_STRDUP(pCtx, "UTF-8", &encoding.szName)  );
			success = SG_TRUE;
		}
		else if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
			SG_context__err_reset(pCtx);
		else
			SG_ERR_RETHROW;
	}
	if(!success && zeros)
	{
		// Finally, check others.
		// (But only because ucsdet_detect() fails miserably at detecting them without a signature.)
		// (And only if there are zeros, because there would have to be in order for there
		// to be a line break, which we'll check for to weed out false positives.)
		const char * FORMAT_LIST[] = {"UTF-16BE", "UTF-16LE", "UTF-32BE", "UTF-32LE"};
		SG_uint32 iFormat = 0;
		while(iFormat<SG_NrElements(FORMAT_LIST) && !success)
		{
			_sg_utf8__try_import(pCtx, pBuf_unsigned, bufLen, FORMAT_LIST[iFormat], &pSz, &encoding.pConverter);
			if(!SG_CONTEXT__HAS_ERR(pCtx))
			{
				// Sanity check. Find a line break.
				SG_uint32 iLinebreak = 0;
				SG_uint32 len = 0;

				len = SG_STRLEN(pSz);
				while(iLinebreak<len && pSz[iLinebreak]!=SG_CR && pSz[iLinebreak]!=SG_LF)
					++iLinebreak;

				if(iLinebreak<len)
				{
					//SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s succeeded with line break.\n", FORMAT_LIST[iFormat])  );
					SG_ERR_CHECK(  SG_STRDUP(pCtx, FORMAT_LIST[iFormat], &encoding.szName)  );
					success = SG_TRUE;
				}
				else
				{
					//SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s would have succeeded, but had no line break.\n", FORMAT_LIST[iFormat])  );
					SG_NULLFREE(pCtx, pSz);
					SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter);
				}
			}
			else if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
				SG_context__err_reset(pCtx);
			else
				SG_ERR_RETHROW;
			++iFormat;
		}
	}
	
	// None of those worked? Finally do a best guess based on the actual contents of the buffer.
	if(!success)
	{
		const UCharsetMatch ** ucm = NULL;
		int32_t matchesFound = 0;
		int32_t iMatch = 0;

		SG_UE_ERR_CHECK(  ucsd = ucsdet_open(&ue)  );
		SG_UE_ERR_CHECK(  ucsdet_setText(ucsd, (const /*unqualified*/ char *)pBuf_unsigned, bufLen, &ue)  );
		SG_UE_ERR_CHECK(  ucm = ucsdet_detectAll(ucsd, &matchesFound, &ue)  );

		while(iMatch<matchesFound && !success)
		{
			SG_UE_ERR_CHECK(  szEncodingName = ucsdet_getName(ucm[iMatch], &ue)  );
			_sg_utf8__try_import(pCtx, pBuf_unsigned, bufLen, szEncodingName, &pSz, &encoding.pConverter);
			if(!SG_CONTEXT__HAS_ERR(pCtx))
			{
				// UTF-16 (and maybe UTF-32) encodings have a problem of not getting detected if
				// they don't have a signature. It *looks* like the first zero byte is terminating
				// the input, and we don't get an error that the full input (bufLen bytes) isn't
				// being used. So we'll do a sanity check:
				// In the most extreme case, every input character was 4 bytes long and got
				// translated to only 1 byte, so the minimum number of output bytes for a valid
				// conversion is bufLen/4. We'll even leave a little wiggle room for any unicode
				// normalization stuff that ICU might have done or whatever.
				if(SG_STRLEN(pSz) >= bufLen/5)
				{
					//SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "ICU guess #%i/%i was %s... And it was a big success!\n", iMatch+1, matchesFound, szEncodingName)  );
					SG_ERR_CHECK(  SG_STRDUP(pCtx, szEncodingName, &encoding.szName)  );
					success = SG_TRUE;
				}
				else
				{
					//SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "ICU guess #%i/%i was %s... But it came up short.\n", iMatch+1, matchesFound, szEncodingName)  );
					SG_NULLFREE(pCtx, pSz);
					SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter);
				}
			}
			else if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
			{
				//SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "ICU guess #%i/%i was %s... But that was bogus.\n", iMatch+1, matchesFound, szEncodingName)  );
				SG_context__err_reset(pCtx);
			}
			else
			{
				//SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "ICU guess #%i/%i was %s... But it totally bombed out!\n", iMatch+1, matchesFound, szEncodingName)  );
				SG_ERR_RETHROW;
			}

			++iMatch;
		}

		ucsdet_close(ucsd);
		ucsd = NULL;
	}

	// Still nothing?! Check UTF-16 and UTF-32 again, without the check for a line break.
	if(!success)
	{
		// We'll take the shortest successful interpretation, since that will be the most likely one to be
		// valid text (... In most all languages except those of south and east Asia anyways. My apologies
		// to south and east Asia.).
		const char * FORMAT_LIST[] = {"UTF-16BE", "UTF-16LE", "UTF-32BE", "UTF-32LE"};
		SG_uint32 iFormat;
		for(iFormat = 0; iFormat<SG_NrElements(FORMAT_LIST); ++iFormat)
		{
			char * pSz2 = NULL;
			SG_utf8_converter * pConverter2 = NULL;
			_sg_utf8__try_import(pCtx, pBuf_unsigned, bufLen, FORMAT_LIST[iFormat], &pSz2, &pConverter2);
			if(!SG_CONTEXT__HAS_ERR(pCtx))
			{
				if(!success || SG_STRLEN(pSz2)<SG_STRLEN(pSz))
				{
					if(success)
					{
						SG_NULLFREE(pCtx, pSz);
						SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter);
						SG_NULLFREE(pCtx, encoding.szName);
					}
					pSz = pSz2;
					encoding.pConverter = pConverter2;
					SG_ERR_CHECK(  SG_STRDUP(pCtx, FORMAT_LIST[iFormat], &encoding.szName)  );
					success = SG_TRUE;
				}
				else
				{
					SG_NULLFREE(pCtx, pSz2);
					SG_UTF8_CONVERTER_NULLFREE(pCtx, pConverter2);
				}
			}
			else if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
				SG_context__err_reset(pCtx);
			else
				SG_ERR_RETHROW;
		}
	}

	// No dice? Let's return the appropriate error message.
	if(!success)
		SG_ERR_THROW(SG_ERR_ILLEGAL_CHARSET_CHAR); //todo: better error code?
	
	*ppSz = pSz;
	if(pEncoding!=NULL)
		*pEncoding = encoding;
	else
	{
		SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter);
		SG_NULLFREE(pCtx, encoding.szName);
	}
	
	return;
fail:
	SG_NULLFREE(pCtx, encoding.szName);
	SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter);
	SG_NULLFREE(pCtx, pSz);
	if(ucsd!=NULL)
		ucsdet_close(ucsd);
}

#else // SG_IOS

struct _sg_utf8_converter
{
	SG_bool					bIsCharSetUtf8;
	char *					szCharSetName;
};

/**
 * Allocate and setup an ICU Character Set converter for the given
 * character set.
 *
 */
void SG_utf8_converter__alloc(SG_context * pCtx,
							  const char * szCharSetName,
							  SG_utf8_converter ** ppConverter)
{
	SG_utf8_converter * pConverter = NULL;

    SG_UNUSED(szCharSetName);

	SG_NULLARGCHECK_RETURN(ppConverter);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pConverter)  );

    pConverter->bIsCharSetUtf8 = SG_TRUE;
    SG_ERR_CHECK(  SG_STRDUP(pCtx, "utf8",&pConverter->szCharSetName)  );

	*ppConverter = pConverter;

	return;

fail:
	SG_UTF8_CONVERTER_NULLFREE(pCtx, pConverter);
}

void SG_utf8_converter__free(SG_context * pCtx, SG_utf8_converter * pConverter)
{
	if (!pConverter)
		return;

	SG_NULLFREE(pCtx, pConverter->szCharSetName);

	SG_free(pCtx, pConverter);
}

SG_bool SG_utf8_converter__utf8(const SG_utf8_converter * pConverter)
{
	return pConverter!=NULL && pConverter->bIsCharSetUtf8;
}

void SG_utf8_converter__to_charset__string__buf_len(
	SG_context * pCtx,
	SG_utf8_converter * pConverter,
	SG_string * pString,
	SG_byte ** ppBuf,
	SG_uint32 * pBufLen
	)
{
	char * pBufAllocated = NULL;
	SG_uint32 lenBufAllocated = 0;

	SG_NULLARGCHECK_RETURN(pConverter);
	SG_NULLARGCHECK_RETURN(pString);
	SG_NULLARGCHECK_RETURN(ppBuf);
	SG_NULLARGCHECK_RETURN(pBufLen);

    SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pString), &pBufAllocated)  );
    lenBufAllocated = SG_STRLEN(pBufAllocated);

	*ppBuf = (SG_byte*)pBufAllocated;
	*pBufLen = lenBufAllocated;

	return;

fail:
	SG_NULLFREE(pCtx, pBufAllocated);
}

void SG_utf8__import_buffer(
	SG_context * pCtx,
	const SG_byte * pBuf_unsigned,
	SG_uint32 bufLen,
	char ** ppSz,
	SG_encoding * pEncoding
	)
{
	const char * pBuf = (const char *)pBuf_unsigned;
	SG_bool zeros = SG_FALSE;
	SG_bool success = SG_FALSE;
	char * pSz = NULL;
	SG_encoding encoding;
	
	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pBuf);
	SG_NULLARGCHECK_RETURN(ppSz);
	
	SG_zero(encoding);
	
	// No signature found? Next try a few encodings that should reliably fail if incorrect.
	if(!success)
	{
		// Check 7-bit ASCII first since SG_textfilediff3 treats this as a special case.
		SG_uint32 i = 0;
		while(i<bufLen && pBuf[i]>0)
			++i;
		if(i==bufLen)
		{
			SG_ERR_CHECK(  _sg_utf8__try_import(pCtx, pBuf, bufLen, "US-ASCII", &pSz, &encoding.pConverter)  );
			SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter); // Special case: null this out.
			encoding.szName = NULL; // Special case: set to NULL.
			success = SG_TRUE;
		}
		else while(i<bufLen && !zeros)
		{
			zeros = (pBuf[i]==0);
			++i;
		}
	}
	if(!success && !zeros)
	{
		// Next check UTF-8, but only if there were no zeros (seems to terminate the input rather than triggering it as being invalid).
		_sg_utf8__try_import(pCtx, pBuf, bufLen, "UTF-8", &pSz, &encoding.pConverter);
		if(!SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_ERR_CHECK(  SG_STRDUP(pCtx, "UTF-8", &encoding.szName)  );
			success = SG_TRUE;
		}
		else if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
			SG_context__err_reset(pCtx);
		else
			SG_ERR_RETHROW;
	}
	
	// No dice? Let's return the appropriate error message.
	if(!success)
		SG_ERR_THROW(SG_ERR_ILLEGAL_CHARSET_CHAR); //todo: better error code?
	
	*ppSz = pSz;
	if(pEncoding!=NULL)
		*pEncoding = encoding;
	else
	{
		SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter);
		SG_NULLFREE(pCtx, encoding.szName);
	}
	
	return;
fail:
	SG_NULLFREE(pCtx, encoding.szName);
	SG_UTF8_CONVERTER_NULLFREE(pCtx, encoding.pConverter);
	SG_NULLFREE(pCtx, pSz);
}

void SG_utf8_converter__from_charset__buf_len(
    SG_context * pCtx,
    SG_utf8_converter * pConverter,
    const SG_byte * pBuffer,
    SG_uint32 bufferLength,
    char ** ppResult)
{
    char * pResult = NULL;
	char * pResultNoBOM = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pConverter);
    SG_NULLARGCHECK_RETURN(pBuffer);
    SG_NULLARGCHECK_RETURN(ppResult);

    /* Serialized data may have signatures, but in-memory strings shouldn't:
    http://userguide.icu-project.org/unicode#TOC-Serialized-Formats
    So we strip the UTF-8 BOM, if it's there. 
    This code is intentionally duplicated below, because we can spare ourselves
    the extra malloc and memcpy in this pass-through case.
    */
    SG_STATIC_ASSERT(3 == sizeof(UTF8_BOM));
    if ( (bufferLength >= 3) && 
         (pBuffer[0] == UTF8_BOM[0]) && 
         (pBuffer[1] == UTF8_BOM[1]) && 
         (pBuffer[2] == UTF8_BOM[2]))
    {
        pBuffer += 3;
        bufferLength -= 3;
    }

    SG_ERR_CHECK(  SG_allocN(pCtx, bufferLength+1, pResult)  );
    memcpy(pResult, pBuffer, bufferLength);
    pResult[bufferLength] = '\0';

    *ppResult = pResult;

    return;
fail:
    SG_NULLFREE(pCtx, pResult);
	SG_NULLFREE(pCtx, pResultNoBOM);
}

/*

Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const SG_uint8 utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

static SG_uint32 utf8_decode(SG_uint32* state, SG_uint32* codep, SG_uint32 byte) 
{
  SG_uint32 type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state*16 + type];
  return *state;
}

struct _foreach_count_data
{
    SG_uint32 count;
};

static void _foreach_count(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_int32    iChar,    //< [in] The current character.
	void*       pUserData //< [in] Pointer to _shares_characters__contains_char__data with additional parameters.
	)
{
	struct _foreach_count_data* pData = (struct _foreach_count_data*)pUserData;

	SG_NULLARGCHECK(pUserData);

    pData->count++;

fail:
	return;
}

void SG_utf8__foreach_character(SG_context * pCtx,
								const char * szUtf8,
								SG_utf8__foreach_character_callback * pfn,
								void * pVoidCallbackData)
{
    SG_uint32 codepoint;
    SG_uint32 state = 0;
    SG_uint8* s = (SG_uint8*) szUtf8;

    for (; *s; ++s)
    {
        if (!utf8_decode(&state, &codepoint, *s))
        {
            SG_ERR_CHECK_RETURN(  (*pfn)(pCtx,(SG_uint32)codepoint,pVoidCallbackData)  );
        }
    }

    if (state != UTF8_ACCEPT)
    {
            SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  );
    }
}

void SG_utf8__next_char(
						SG_context* pCtx,
						SG_byte*    pBytes,
						SG_uint32   uSize,
						SG_uint32   uIndex,
						SG_int32*   pChar,
						SG_uint32*  pIndex
						)
{
	SG_int32 iSize  = (SG_int32)uSize;
	SG_int32 iIndex = (SG_int32)uIndex;
    SG_uint32 codepoint;
    SG_uint32 state = 0;
    SG_uint8* s = (SG_uint8*) pBytes + uIndex;
	
	SG_UNUSED(pCtx);
	
	do {
		utf8_decode(&state, &codepoint, s[iIndex]);
		++iIndex;
	}	while ((state != UTF8_ACCEPT) && (state != UTF8_REJECT) && (iIndex - iSize));

	if (state != UTF8_ACCEPT)
    {
		SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  );
    }

	if (pChar != NULL)
	{
		*pChar = codepoint;
	}
	if (pIndex != NULL)
	{
		*pIndex = (SG_uint32)(iIndex);
	}
}

void SG_utf8__length_in_characters__sz(SG_context * pCtx, const char* s, SG_uint32* pResult)
{
    struct _foreach_count_data d;

    d.count = 0;

	SG_ERR_CHECK(  SG_utf8__foreach_character(pCtx, s, _foreach_count, (void*)&d)  );

    *pResult = d.count;

fail:
    ;
}

void SG_utf8__to_utf32__sz(SG_context * pCtx,
						   const char * szUtf8,
						   SG_int32 ** ppBufResult,	// caller needs to free this
						   SG_uint32 * pLenResult)
{
    // TODO fix this.  it'll do for now since I don't think it ever gets called on iOS anyway
    SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

void SG_utf8__from_utf32(SG_context * pCtx,
						 const SG_int32 * pBufUnicode,
						 char ** pszUtf8Result,		// caller needs to free this
						 SG_uint32 * pLenResult)
{
    // TODO fix this.  it'll do for now since I don't think it ever gets called on iOS anyway
    SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

#endif // SG_IOS

