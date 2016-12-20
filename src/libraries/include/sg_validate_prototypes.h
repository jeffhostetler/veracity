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
 * @file sg_validate_prototypes.h
 *
 * @details Functionality for validating string contents.
 *
 */

#ifndef H_SG_VALIDATE_PROTOTYPES_H
#define H_SG_VALIDATE_PROTOTYPES_H

BEGIN_EXTERN_C

/**
 * Validates a string's size and that it doesn't contain any invalid characters.
 */
void SG_validate__check(
	SG_context* pCtx,       //< [in] [out] Error and context info.
	const char* szValue,    //< [in] The string to validate.
	                        //<      NULL is treated as an empty string.
	SG_uint32   uMin,       //< [in] The smallest valid size.
	SG_uint32   uMax,       //< [in] The largest valid size.
	const char* szInvalids, //< [in] A string containing the characters to consider invalid.
	                        //<      NULL is treated as an empty string.
	SG_bool     bControls,  //< [in] If true, control characters (< 0x1F) are implicitly
	                        //<      included in szInvalids.
	SG_uint32*  pResult     //< [out] SG_validate__result flags describing the result.
	);

/**
 * Like SG_validate__check, except that it trims whitespace from the beginning
 * and end of the string before validating it.
 */
void SG_validate__check__trim(
	SG_context* pCtx,       //< [in] [out] Error and context info.
	const char* szValue,    //< [in] The string to validate.
	                        //<      NULL is treated as an empty string.
	SG_uint32   uMin,       //< [in] The smallest valid size.
	SG_uint32   uMax,       //< [in] The largest valid size.
	const char* szInvalids, //< [in] A string containing the characters to consider invalid.
	                        //<      NULL is treated as an empty string.
	SG_bool     bControls,  //< [in] If true, control characters (< 0x1F) are implicitly
	                        //<      included in szInvalids.
	SG_uint32*  pResult,    //< [out] SG_validate__result flags describing the result.
	char**      ppTrimmed   //< [out] The trimmed string, if it's valid.
	                        //<       Unchanged if the value is invalid.
	                        //<       May be NULL if this output is not required.
	);

/**
 * Like SG_validate__check except that it throws an error if validation fails.
 */
void SG_validate__ensure(
	SG_context* pCtx,       //< [in] [out] Error and context info.
	const char* szValue,    //< [in] The string to validate.
	                        //<      NULL is treated as an empty string.
	SG_uint32   uMin,       //< [in] The smallest valid size.
	SG_uint32   uMax,       //< [in] The largest valid size.
	const char* szInvalids, //< [in] A string containing the characters to consider invalid.
	                        //<      NULL is treated as an empty string.
	SG_bool     bControls,  //< [in] If true, control characters (< 0x1F) are implicitly
	                        //<      included in szInvalids.
	SG_error    uError,     //< [in] Error to throw if validation fails.
	const char* szName      //< [in] A name for the string being validated, to use in thrown error messages.
	);

/**
 * Like SG_validate__ensure, but uses trimming version of SG_validate__check.
 */
void SG_validate__ensure__trim(
	SG_context* pCtx,       //< [in] [out] Error and context info.
	const char* szValue,    //< [in] The string to validate.
	                        //<      NULL is treated as an empty string.
	SG_uint32   uMin,       //< [in] The smallest valid size.
	SG_uint32   uMax,       //< [in] The largest valid size.
	const char* szInvalids, //< [in] A string containing the characters to consider invalid.
	                        //<      NULL is treated as an empty string.
	SG_bool     bControls,  //< [in] If true, control characters (< 0x1F) are implicitly
	                        //<      included in szInvalids.
	SG_error    uError,     //< [in] Error to throw if validation fails.
	const char* szName,     //< [in] A name for the string being validated, to use in thrown error messages.
	char**      ppTrimmed   //< [out] The trimmed string, if it's valid.
	                        //<       Unchanged if the value is invalid.
	                        //<       May be NULL if this output is not required.
	);

/**
 * Sanitizes a string by removing/replacing invalid characters and adding/removing
 * additional characters to ensure that it meets validation requirements.
 */
void SG_validate__sanitize(
	SG_context* pCtx,         //< [in] [out] Error and context info.
	const char* szValue,      //< [in] The string to sanitize.
	                          //<      NULL is treated as an empty string.
	SG_uint32   uMin,         //< [in] The smallest valid size.
	SG_uint32   uMax,         //< [in] The largest valid size.
	const char* szInvalids,   //< [in] A string containing the characters to consider invalid.
	                          //<      NULL is treated as an empty string.
	SG_uint32   uFixFlags,    //< [in] SG_validate__result flags specifying validation results
	                          //<      that should be fixed/sanitized.  Usually want to use ALL.
	const char* szReplace,    //< [in] String to replace invalid characters with to sanitize them.
	                          //<      NULL is treated as an empty string (effectively removes invalid characters).
	const char* szAdd,        //< [in] String to add when sanitizing a string that's too short.
	                          //<      It is an error for this to be NULL or zero-length if uFixFlags contains TOO_SHORT.
	SG_string** ppSanitized   //< [out] The sanitized string.
	);

/**
 * Like SG_validate__sanitize, but trims the input string before sanitizing.
 */
void SG_validate__sanitize__trim(
	SG_context* pCtx,         //< [in] [out] Error and context info.
	const char* szValue,      //< [in] The string to sanitize.
	                          //<      NULL is treated as an empty string.
	SG_uint32   uMin,         //< [in] The smallest valid size.
	SG_uint32   uMax,         //< [in] The largest valid size.
	const char* szInvalids,   //< [in] A string containing the characters to consider invalid.
	                          //<      NULL is treated as an empty string.
	SG_uint32   uFixFlags,    //< [in] SG_validate__result flags specifying validation results
	                          //<      that should be fixed/sanitized.  Usually want to use ALL.
	const char* szReplace,    //< [in] String to replace invalid characters with to sanitize them.
	                          //<      NULL is treated as an empty string (effectively removes invalid characters).
	const char* szAdd,        //< [in] String to add when sanitizing a string that's too short.
	                          //<      It is an error for this to be NULL or zero-length if uFixFlags contains TOO_SHORT.
	SG_string** ppSanitized   //< [out] The sanitized string.
	);

/*
 * TODO: I would like to add functionality to this module to check/ensure/sanitize
 *       the uniqueness of the given value with respect to a list of disallowed
 *       values.  That probably amounts to a new parameter to most functions to
 *       specify the disallowed value list (not sure of its type, though) and a new
 *       RESULT flag like NOT_UNIQUE.
 *       A good place to start would be SG_user__uniqify_username, whose creation
 *       inspired the desire to add the functionality here, which would hopefully
 *       replace it.  The interesting dilemma, though, is that the place which
 *       calls SG_user__uniqify_username only wants to use it if sanitization results
 *       in a brand new value.  If sanitization was unnecessary, it doesn't want to
 *       bother checking/enforcing uniqueness.  That's not how you'd think to add
 *       this functionality to the sanitize function, and not necessarily what you'd
 *       want in the general case, so I'm not sure if that ends up being yet another
 *       parameter or what.
 */

END_EXTERN_C

#endif//H_SG_VALIDATE_PROTOTYPES_H
