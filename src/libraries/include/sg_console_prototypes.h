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
 * @file sg_console_prototypes.h
 *
 * @details Routines for writing to the console.  This includes
 * stdout and stderr for command line applications.
 *
 * These routines take care of converting UTF-8 strings into
 * whatever locale/wchar_t versions are required for the platform.
 *
 * TODO Later we may want to expand it to include support for
 * a console window in a gui app.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_CONSOLE_PROTOTYPES_H
#define H_SG_CONSOLE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Write a binary buffer to stdout with no interpretation of any kind.
 */
void SG_console__binary__stdout(SG_context * pCtx,
								SG_byte * pBuf,
								SG_uint32 lenBuf);

//////////////////////////////////////////////////////////////////

void SG_console__raw(SG_context * pCtx,
				SG_console_stream cs, const char * szRawString);
SG_DECLARE_PRINTF_PROTOTYPE(
	void SG_console(SG_context * pCtx, SG_console_stream cs, const char * szFormat, ...),
	3, 4);

void SG_console__va(SG_context * pCtx,
					SG_console_stream cs, const char * szFormat, va_list ap);
void SG_console__flush(SG_context* pCtx, SG_console_stream cs);
void SG_console__read_stdin(SG_context* pCtx, SG_string** ppStrInput);
void SG_console__readline_stdin(SG_context* pCtx, SG_string** ppStrInput);

/**
 * Ask a Y/N question on the console.
 */
void SG_console__ask_question__yn(SG_context * pCtx,
								  const char * pszQuestion,
								  SG_console_qa qaDefault,
								  SG_bool * pbAnswer);

/**
 * Gets a password from keyboard input without printing the characters to the
 * console.
 * 
 * Does not print any sort of prompt (caller should have done so).
 */
void SG_console__get_password(SG_context * pCtx,
                              SG_string **ppPassword);

/**
 * Returns the width of the console in columns.
 * Returns 80 on any failure.
 */
SG_uint32 SG_console__get_width(void);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_CONSOLE_PROTOTYPES_H
