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
 * @file sg_validate_typedefs.h
 *
 * @details Typedefs used when validating string contents.
 *
 */

#ifndef H_SG_VALIDATE_TYPEDEFS_H
#define H_SG_VALIDATE_TYPEDEFS_H

BEGIN_EXTERN_C

/*
 * Handy groupings of characters.
 */
#define SG_VALIDATE__CHARS__CONTROL "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
#define SG_VALIDATE__CHARS__SPACE " \t"
#define SG_VALIDATE__CHARS__NEWLINE "\r\n"
#define SG_VALIDATE__CHARS__BOUNDARY SG_VALIDATE__SPACE_CHARS SG_VALIDATE__NEWLINE_CHARS
#define SG_VALIDATE__CHARS__URL "#%/?"
#define SG_VALIDATE__CHARS__QUERYSTRING "&="
#define SG_VALIDATE__CHARS__HTML "&'\"<>"
#define SG_VALIDATE__CHARS__WIN_PATH "*\\|:\"<>/?"
#define SG_VALIDATE__CHARS__PRINTF_FORMAT "%"

/*
 * Characters that are currently banned in general from most object names.
 */
#define SG_VALIDATE__BANNED_CHARS__URL "#%/?"
#define SG_VALIDATE__BANNED_CHARS__SHELL "`~!$^&*()=[]{}\\|;:'\"<>"
#define SG_VALIDATE__BANNED_CHARS SG_VALIDATE__BANNED_CHARS__URL SG_VALIDATE__BANNED_CHARS__SHELL

/**
 * Possible results of validating a string value.
 * Multiple results are possible on a single value.
 */
typedef enum SG_validate__result
{
	SG_VALIDATE__RESULT__VALID             = 0,      //< String is valid.
	SG_VALIDATE__RESULT__TOO_SHORT         = 1 << 0, //< String's length is too small.
	SG_VALIDATE__RESULT__TOO_LONG          = 1 << 1, //< String's length is too large.
	SG_VALIDATE__RESULT__INVALID_CHARACTER = 1 << 2, //< String contains an invalid character.
	SG_VALIDATE__RESULT__CONTROL_CHARACTER = 1 << 3, //< String contains a control character, which is invalid.
	SG_VALIDATE__RESULT__LAST              = 1 << 4, //< Last value in the enum, for iteration.

	/**
	 * Convenient mask of all result flags.
	 */
	SG_VALIDATE__RESULT__ALL = SG_VALIDATE__RESULT__LAST - 1u
}
SG_validate__result;

END_EXTERN_C

#endif//H_SG_VALIDATE_TYPEDEFS_H
