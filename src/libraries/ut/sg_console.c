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
 * @file sg_console.c
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#if defined(MAC) || defined(LINUX)
#include <sys/ioctl.h>
#include <termios.h>
#else
#include <io.h>
#endif

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
#define OS_CHAR_T		wchar_t
#else
#define OS_CHAR_T		char
#endif

//this version will not do any sprintf style replacement.  It's best to use this one when you
//know that you don't need replacement.  Otherwise, you will get an error if your string
//contains a % sign.
//
// TODO 2012/07/10 the name __raw() is a little misleading because
// TODO            we still deal with locale issues (and maybe CRLF/LF
// TODO            issues).  really, it just means that we don't treat
// TODO            the given string as a format string (with no args).
// TODO
// TODO            we should rename this __nofmt() or something.
//
void SG_console__raw(SG_context * pCtx,
				SG_console_stream cs, const char * szRawString)
{
	SG_ERR_CHECK(SG_console(pCtx, cs, "%s", szRawString)  );
fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_console(SG_context * pCtx,
				SG_console_stream cs, const char * szFormat, ...)
{
	// we are given a UTF-8 string and want to format it and write it to
	// a stream using the appropriate character encoding for the platform.

	va_list ap;

	va_start(ap,szFormat);
	SG_console__va(pCtx,cs,szFormat,ap);
	va_end(ap);

	SG_ERR_CHECK_RETURN_CURRENT;
}

#if defined(WINDOWS)
static void _sg_console__write(SG_context * pCtx,
							   SG_console_stream cs, const SG_string * pString)
{
	//////////////////////////////////////////////////////////////////
	// On Windows, STDOUT and STDERR are opened in TEXT (translated)
	// mode (this causes LF <--> CRLF translation).  For historical
	// reasons this is said to be the "ANSI" encoding.  I think this
	// means the system-default ANSI code page.
	//
	// With VS2005, fopen() allows you to specify a Coded Character Set
	// CCS={UTF-8,UTF-16LE,UNICODE} with the mode args.  Adding one of
	// these CCS args changes the translation applied to the stream and
	// may cause a BOM to be read/written.
	//
	// I have yet to find a way to change the CCS setting for an already
	// opened stream (such as STDOUT and STDERR).  _setmode() can be
	// used to change the TEXT/BINARY setting, but it doesn't say anything
	// about changing the CCS.
	//
	// http://msdn.microsoft.com/en-us/library/yeby3zcb.aspx
	// http://msdn.microsoft.com/en-us/library/c4cy2b8e.aspx
	// http://msdn.microsoft.com/en-us/library/883tf19a.aspx
	// http://msdn.microsoft.com/en-us/library/x99tb11d.aspx
	//
	// Also, _wfopen() allows one to use a wide-char pathname to open
	// a file -- it does ***NOT*** imply anything about the translation
	// to be applied to the contents of the file; for that, you have to
	// use one of the CCS= args.
	//
	// fwprintf() writes the contents of a wide-char buffer to the stream.
	// they translate the buffer using the translation settings on the stream.
	// therefore, for STDOUT and STDERR, (which are ANSI TEXT), they
	// take the buffer and call wctomb() which uses the current CODE PAGE
	// set in the user's LOCALE to convert the buffer.
	//
	// http://msdn.microsoft.com/en-us/library/wabhak7d.aspx
	//
	// According to the following link, it is ***NOT*** possible to
	// set the user's locale to UTF-8 (because it requires more than
	// 2 bytes per character).
	//
	// http://msdn.microsoft.com/en-us/library/x99tb11d.aspx
	//
	// THEREFORE, for STDOUT and STDERR, our nice-n-neat UTF-8 output
	// will get converted into some CODE PAGE LOCALE (probably with
	// '?' substitutions for code points that are not in the code page).
	// I THINK THIS IS THE ONLY PLACE WHERE WE USE THE USER'S LOCALE
	// CODE PAGE ON WINDOWS.
	//
	//
	// TODO When we want to support reporting to a GUI window and/or logging
	// TODO to a custom logfile, we can do it CORRECTLY using either UTF-8.
	//
	//////////////////////////////////////////////////////////////////

	wchar_t * pBuf = NULL;
	const char * szInString;

	// allow null or blank string to silently succeed.

	if (!pString)
		return;
	szInString = SG_string__sz(pString);
	if (!szInString || !*szInString)
		return;

	// Convert UTF-8 input into UNICODE/UTF-16/UCS-2 wchar_t buffer.
	// Then let the OS mangle the wchar_t buffer as it needs to.

	SG_ERR_CHECK_RETURN(  SG_utf8__extern_to_os_buffer__wchar(pCtx, szInString,&pBuf,NULL)  );

	// WARNING: for GUI apps, stdout and stderr are closed before wmain()
	// WARNING: gets called.  so we need to do something else.

	switch (cs)
	{
	default:
	case SG_CS_STDERR:
		fwprintf_s(stderr,L"%s",pBuf);
		break;

	case SG_CS_STDOUT:
		fwprintf_s(stdout,L"%s",pBuf);
		break;
	}

#ifdef DEBUG
	OutputDebugString(pBuf); // Show console messages in VS debug output window.
#endif

	SG_NULLFREE(pCtx, pBuf);
}
#else
static void _sg_console__write(SG_context * pCtx,
							   SG_console_stream cs, const SG_string * pString)
{
	//////////////////////////////////////////////////////////////////
	// On Linux/Mac, we should write messages to STDOUT and STDERR using
	// the user's locale.  That is, I don't think we should blindly write
	// UTF-8, unless that is their locale setting.
	//////////////////////////////////////////////////////////////////

	char * pBuf = NULL;
	SG_uint32 lenBuf;
	const char * szInString;

	// allow null or blank string to silently succeed.

	if (!pString)
		return;
	szInString = SG_string__sz(pString);
	if (!szInString || !*szInString)
		return;

#if defined(MAC)
	SG_ERR_CHECK_RETURN(  SG_utf8__extern_to_os_buffer__sz(pCtx,szInString,&pBuf)  );
#endif
#if defined(LINUX)
	SG_utf8__extern_to_os_buffer__sz(pCtx,szInString,&pBuf);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx,SG_ERR_UNMAPPABLE_UNICODE_CHAR))
		{
			// silently eat the error and try to use the lossy converter.
			SG_context__err_reset(pCtx);
			SG_ERR_CHECK_RETURN(  SG_utf8__extern_to_os_buffer_with_substitutions(pCtx,szInString,&pBuf)  );
		}
		else
		{
			SG_ERR_RETHROW_RETURN;
		}
	}
#endif

	lenBuf = SG_STRLEN(pBuf);

	switch (cs)
	{
	default:
	case SG_CS_STDERR:
		fwrite(pBuf,sizeof(char),lenBuf,stderr);
		break;

	case SG_CS_STDOUT:
		fwrite(pBuf,sizeof(char),lenBuf,stdout);
		break;
	}

	SG_NULLFREE(pCtx, pBuf);
}
#endif

#if defined(WINDOWS)
static void _sg_console__grow_wchar_buf(SG_context* pCtx, wchar_t** ppThis, SG_uint32 iCurrSize, SG_uint32 iGrowBy)
{
	wchar_t* pwcsNew;
	SG_uint32 iSizeNew;

	SG_ARGCHECK_RETURN(ppThis, ppThis);

	iSizeNew = iCurrSize + iGrowBy;

	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx,iSizeNew,pwcsNew)  );

	if (*ppThis)
	{
		if (iCurrSize > 0)
		{
			if ( wmemmove_s(pwcsNew, iSizeNew, *ppThis, iCurrSize) )
				SG_ERR_THROW_RETURN(errno);
		}
		SG_NULLFREE(pCtx, *ppThis);
	}

	*ppThis = pwcsNew;
}
#else
static void _sg_console__grow_char_buf(SG_context* pCtx, char** ppThis, SG_uint32 iCurrSize, SG_uint32 iGrowBy)
{
	char* pszNew;
	SG_uint32 iSizeNew;

	SG_ARGCHECK_RETURN(ppThis, ppThis);

	iSizeNew = iCurrSize + iGrowBy;

	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx,iSizeNew,pszNew)  );

	if (*ppThis)
	{
		if (iCurrSize > 0)
			memmove(pszNew,*ppThis,iCurrSize*sizeof(char));
		SG_NULLFREE(pCtx, *ppThis);
	}

	*ppThis = pszNew;
}
#endif

void SG_console__va(SG_context * pCtx,
					SG_console_stream cs, const char * szFormat, va_list ap)
{
	// we are given a UTF-8 string and want to format it and write it to
	// a stream using the appropriate character encoding for the platform.

	SG_string * pString = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pString)  );
	SG_ERR_CHECK(  SG_string__vsprintf(pCtx,pString,szFormat,ap)  );

	SG_ERR_CHECK(  _sg_console__write(pCtx,cs,pString)  );

	SG_STRING_NULLFREE(pCtx, pString);
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

void SG_console__flush(SG_context* pCtx, SG_console_stream cs)
{
	SG_UNUSED(pCtx);

	// TODO: propagate errors?
	switch (cs)
	{
		default:
		case SG_CS_STDERR:
			fflush(stderr);
			break;

		case SG_CS_STDOUT:
			fflush(stdout);
			break;
	}
}

void SG_console__read_stdin(SG_context* pCtx, SG_string** ppStrInput)
{
	SG_uint32 iCount = 0;
	SG_uint32 iMax = 128;

#if defined(WINDOWS)
	wint_t retval;
	wchar_t* pwcs;

	SG_ERR_CHECK(  SG_allocN(pCtx, iMax, pwcs)  );

	while((retval = fgetwc(stdin)) != WEOF)
	{
		pwcs[iCount++] = (wchar_t) retval;

		if (iCount == iMax)
		{
			SG_ERR_CHECK(  _sg_console__grow_wchar_buf(pCtx, &pwcs, iMax, iMax)  );
			iMax = iCount + iMax;
		}
	}
	pwcs[iCount] = 0;

	SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, *ppStrInput, pwcs)  );

    // fall through
fail:
	SG_NULLFREE(pCtx, pwcs);

#else

	char* pszInput;
	int retval;

	SG_ERR_CHECK(  SG_allocN(pCtx, iMax, pszInput)  );

	while((retval = fgetc(stdin)) != EOF)
	{
		pszInput[iCount++] = (char) retval;

		if (iCount == iMax)
		{
			SG_ERR_CHECK(  _sg_console__grow_char_buf(pCtx, &pszInput, iMax, iMax)  );
			iMax = iCount + iMax;
		}
	}
	pszInput[iCount] = 0;

	SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__sz(pCtx, *ppStrInput, pszInput)  );

    // fall through
fail:
	SG_NULLFREE(pCtx, pszInput);

#endif
}

void SG_console__readline_stdin(SG_context* pCtx, SG_string** ppStrInput)
{
	SG_string *pStrInput = NULL;
	SG_uint32 iCount = 0;
	SG_uint32 iMax = 128;

#if defined(WINDOWS)
	wint_t retval;
	wchar_t* pwcs;

	SG_ERR_CHECK(  SG_allocN(pCtx, iMax, pwcs)  );

	while(((retval = fgetwc(stdin)) != '\n') && (retval != WEOF))
	{
		pwcs[iCount++] = (wchar_t) retval;

		if (iCount == iMax)
		{
			SG_ERR_CHECK(  _sg_console__grow_wchar_buf(pCtx, &pwcs, iMax, iMax)  );
			iMax = iCount + iMax;
		}
	}
	pwcs[iCount] = 0;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrInput)  );
	SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pStrInput, pwcs)  );

	*ppStrInput = pStrInput;
	pStrInput = NULL;

    // fall through
fail:
	SG_NULLFREE(pCtx, pwcs);

#else

	char* pszInput;
	int retval;

	SG_ERR_CHECK(  SG_allocN(pCtx, iMax, pszInput)  );

	while(((retval = fgetc(stdin)) != '\n') && (retval != EOF) && (retval != EINTR))
	{
		pszInput[iCount++] = (char) retval;

		if (iCount == iMax)
		{
			SG_ERR_CHECK(  _sg_console__grow_char_buf(pCtx, &pszInput, iMax, iMax)  );
			iMax = iCount + iMax;
		}
	}
	pszInput[iCount] = 0;
	
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrInput)  );
	SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__sz(pCtx, pStrInput, pszInput)  );

	*ppStrInput = pStrInput;
	pStrInput = NULL;

    // fall through
fail:
	SG_NULLFREE(pCtx, pszInput);

#endif

	SG_STRING_NULLFREE(pCtx, pStrInput);
}

//////////////////////////////////////////////////////////////////

void SG_console__ask_question__yn(SG_context * pCtx,
								  const char * pszQuestion,
								  SG_console_qa qaDefault,
								  SG_bool * pbAnswer)
{
	SG_string * pStrInput = NULL;
	const char * pszAnswer;

	SG_NONEMPTYCHECK_RETURN(pszQuestion);

	while (1)
	{
		switch (qaDefault)
		{
		case SG_QA_DEFAULT_YES:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s [Yn]: ", pszQuestion)  );
			SG_ERR_CHECK(  SG_console__readline_stdin(pCtx, &pStrInput)  );
			pszAnswer = SG_string__sz(pStrInput);
			if ((pszAnswer[0] == 0) || ((pszAnswer[0] == 'y')&&(pszAnswer[1] == '\0')) || ((pszAnswer[0] == 'Y')&&(pszAnswer[1] == '\0')))
			{
				*pbAnswer = SG_TRUE;
				goto done;
			}
			else if (((pszAnswer[0] == 'n')&&(pszAnswer[1] == '\0')) || ((pszAnswer[0] == 'N')&&(pszAnswer[1] == '\0')))
			{
				*pbAnswer = SG_FALSE;
				goto done;
			}
			break;

		case SG_QA_DEFAULT_NO:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s [yN]: ", pszQuestion)  );
			SG_ERR_CHECK(  SG_console__readline_stdin(pCtx, &pStrInput)  );
			pszAnswer = SG_string__sz(pStrInput);
			if ((pszAnswer[0] == 0) || ((pszAnswer[0] == 'n')&&(pszAnswer[1] == '\0')) || ((pszAnswer[0] == 'N')&&(pszAnswer[1] == '\0')))
			{
				*pbAnswer = SG_FALSE;
				goto done;
			}
			else if (((pszAnswer[0] == 'y')&&(pszAnswer[1] == '\0')) || ((pszAnswer[0] == 'Y')&&(pszAnswer[1] == '\0')))
			{
				*pbAnswer = SG_TRUE;
				goto done;
			}
			break;

		default:				// quiets compiler
		case SG_QA_NO_DEFAULT:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s [yn]: ", pszQuestion)  );
			SG_ERR_CHECK(  SG_console__readline_stdin(pCtx, &pStrInput)  );
			pszAnswer = SG_string__sz(pStrInput);
			if (((pszAnswer[0] == 'y')&&(pszAnswer[1] == '\0')) || ((pszAnswer[0] == 'Y')&&(pszAnswer[1] == '\0')))
			{
				*pbAnswer = SG_TRUE;
				goto done;
			}
			else if (((pszAnswer[0] == 'n')&&(pszAnswer[1] == '\0')) || ((pszAnswer[0] == 'N')&&(pszAnswer[1] == '\0')))
			{
				*pbAnswer = SG_FALSE;
				goto done;
			}
			break;
		}

		SG_STRING_NULLFREE(pCtx, pStrInput);
	}

done:
	;
fail:
	SG_STRING_NULLFREE(pCtx, pStrInput);
}

void SG_console__get_password(SG_context * pCtx,
                              SG_string **ppPassword)
{
#if defined(MAC) || defined(LINUX)
	struct termios original, sans_echo;
	tcgetattr(STDIN_FILENO, &original);
	sans_echo = original;
	sans_echo.c_lflag &= ~(ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &sans_echo);
	SG_console__readline_stdin(pCtx, ppPassword);
	tcsetattr(STDIN_FILENO, TCSANOW, &original);
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\n")  );
#endif

#if defined(WINDOWS)
	HANDLE h_stdin;
	DWORD original;
	h_stdin = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(h_stdin, &original);
	SetConsoleMode(h_stdin, original & ~(ENABLE_ECHO_INPUT));
	SG_console__readline_stdin(pCtx, ppPassword);
	SetConsoleMode(h_stdin, original);
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\n")  );
#endif
}

/* Get the console width in columns. Returns 80 on any failure. */
SG_uint32 SG_console__get_width(void)
{
	SG_uint32 width = 80; /* default to 80 columns */

#if defined(WINDOWS)

	CONSOLE_SCREEN_BUFFER_INFO sbi = {0};

	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi))
		width =sbi.dwSize.X; 

#elif defined(MAC) || defined(LINUX)

	struct winsize w;
	if (-1 != ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
    {
        if (w.ws_col)
        {
            width = w.ws_col;
        }
    }

#endif

	return width;
}

//////////////////////////////////////////////////////////////////

/**
 * Write a binary buffer to stdout with no interpretation of any kind.
 */
void SG_console__binary__stdout(SG_context * pCtx,
								SG_byte * pBuf,
								SG_uint32 lenBuf)
{
	SG_int32 lenWrote;
	SG_int32 temp_fd = -1;

#if defined(WINDOWS)
	FILE* pf = NULL;
	errno_t my_err = -1;
	SG_int32 old_fd = -1;

	/* Save stdout state so we can revert to it when we're done. */
	(void)fflush(stdout);
	old_fd = _dup(_fileno(stdout));
 	if (old_fd == -1)
 		SG_ERR_THROW(errno);

	/* On Windows, this hack allows splatting arbitrary data to the console.
	Using _setmode or _dup/_dup2 on stdout doesn't alter its "_textmode" (see _write_nolock in CRT's write.c),
	leaving it in a state where _write()-ing an odd number of bytes ((len & 1) == 1)) asserts.
	Note that this breaks redirection, which is why e.g. cat has an --output argument. */
	my_err = freopen_s(&pf, "CON", "w", stdout);
	if (my_err)
		SG_ERR_THROW(my_err);

	temp_fd = _fileno(pf);
#else
	temp_fd = fileno(stdout);
#endif

#if defined(WINDOWS)
	lenWrote = _write(temp_fd, pBuf, lenBuf);
#else
	lenWrote = write(temp_fd, pBuf, lenBuf);
#endif

	if (lenWrote < 0)
		SG_ERR_THROW2(errno, (pCtx, "failed to write binary data to console"));
	else if ((SG_uint32)lenWrote < lenBuf)
		SG_ERR_THROW2(SG_ERR_INCOMPLETEWRITE, (pCtx, "failed to write all binary data to console"));

	/* fall through */
fail:
#if defined(WINDOWS)
	/* Put stdout back the way it was. */
	if (pf)
		(void)fflush(pf);
	if (old_fd != -1)
	{
		(void)_dup2(old_fd, _fileno(stdout));
		(void)_close(old_fd);
	}
	(void)clearerr(stdout);
#endif
	;
}
