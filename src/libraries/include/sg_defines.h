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
 * @file sg_defines.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_DEFINES_H
#define H_SG_DEFINES_H

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
#elif defined(LINUX)
#elif defined(MAC)
#else
#	error "Must define platform (WINDOWS, LINUX, or MAC)."
#endif

//////////////////////////////////////////////////////////////////

#ifndef BEGIN_EXTERN_C
#	ifdef __cplusplus
#		define BEGIN_EXTERN_C	extern "C" {
#		define END_EXTERN_C		}
#	else
#		define BEGIN_EXTERN_C	/**/
#		define END_EXTERN_C		/**/
#	endif /* __cplusplus */
#endif /* EXTERN_C */

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
#define SG_EXPORT __declspec(dllexport)
#else
#define SG_EXPORT /* TODO figure out export issues on other platforms. */
#endif

//////////////////////////////////////////////////////////////////
// When we have the warning level set at the highest level, we need
// to be able to mark function parameters as unused when they are
// only there to make the prototype match.  but this works differently
// between GCC and MSVC.
//
// GCC wants us to use the __attribute__((unused)) at the point of
// the parameter declaration in the function arg list; the typical
// 'variable;' statement generates a 'statement has no effect' warning.
//
// MSVC wants us to only do the 'variable;' trick inside the function.
//
// SO, we have 2 macros.  (sigh)
//
// void foo( SG_UNUSED_PARAM(k) )
// {
//      SG_UNUSED(k);
//      ...
// }

// TODO just for fun, if you use the following definition of SG_UNUSED()
// everything compiles, but lots of things crash.  This suggests that
// we've got places where we mark something as SG_UNUSED and then use it.
// #define SG_UNUSED(x)			x=0

#define SG_UNUSED(x)			(void)(x)
#define SG_UNUSED_PARAM(x)		x

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)

#pragma warning( disable : 4127 )	// /W4 complains about do...while(0) trick
#pragma warning( disable : 4201 )	// /W4 complains about nameless struct/union in shlobj.h

#if defined(DEBUG)
#define WINDOWS_CRT_LEAK_CHECK // Turn on Windows' CRT leak checking
#define _CRTDBG_MAP_ALLOC // Include file and line numbers when possible in CRT leak dump.
#endif // DEBUG

#endif // WINDOWS

#ifndef SG_STATEMENT__BEGIN
#define SG_STATEMENT__BEGIN do {
#endif

#ifndef SG_STATEMENT__END
#define SG_STATEMENT__END } while (0)
#endif

#ifndef SG_STATEMENT
#define SG_STATEMENT( s )			SG_STATEMENT__BEGIN s SG_STATEMENT__END
#endif

#ifndef SG_NrElements
#define SG_NrElements(a)			(sizeof(a) / sizeof(a[0]))
#endif

#ifndef SG_MAX
#define SG_MAX(a,b)				(((a) > (b)) ? (a) : (b))
#define SG_MIN(a,b)				(((a) < (b)) ? (a) : (b))
#endif

#define SG_STRINGIFY(s) #s
#define SG_STRINGIFY_EXPANDED(s) SG_STRINGIFY(s)

/**
 * This macro encapsulates a convention for returning pointers
 * through arguments of a function.  If the return '** pp' is
 * NULL, it does nothing.  Otherwise, it returns the value
 * and sets it to NULL, so that later calls to NULLFREE will
 * do nothing.
 *
 * REVIEW: Jeff says: This is poorly named.  I don't have a good
 * suggestion at this time, but it makes me think that it is going
 * to do a C "return" statement...   Maybe SG_SET_AND_NULL()
 */
#define SG_RETURN_AND_NULL(val, pp) SG_STATEMENT( if (pp) { *(pp)=(val); (val)=NULL; } )

/**
 * This expands on that and either returns or frees the value.  In both cases, the value is nulled.
 *
 * REVIEW: Jeff says: This is poorly named.  I don't have a good
 * suggestion at this time, but it makes me think that it is going
 * to do a C "return" statement...   Maybe SG_SET_AND_NULL_OR_NULLFREE()
 */
#define SG_RETURN_AND_NULL_OR_NULLFREE(val,pp,pfnFree) SG_STATEMENT( if (pp) { *(pp)=(val); } else { (*(pfnFree))(val); } (val)=NULL; )

/**
 * A condition that is true if both pointers are NULL or
 * if both pointers are non-NULL.  It is false if one
 * pointer is NULL and the other is non-NULL.
 */
#define SG_BOTH_NULL_OR_NOT(p1, p2) (((p1) == NULL) == ((p2) == NULL))


//////////////////////////////////////////////////////////////////

/*

Clever hack to do compile-time assertions.  Pieced together from:

http://www.pixelbeat.org/programming/gcc/static_assert.html
and
http://stackoverflow.com/questions/174356/ways-to-assert-expressions-at-build-time-in-c

This allows you to do things like

SG_STATIC_ASSERT(sizeof(SG_int64)==8);

*/

/* Note we need the 2 concats below because arguments to ##
 * are not expanded, so we need to expand __LINE__ with one indirection
 * before doing the actual concatenation. */
#define _SG_ASSERT_CONCAT_(a, b) a##b
#define _SG_ASSERT_CONCAT(a, b) _SG_ASSERT_CONCAT_(a, b)

#if defined(WINDOWS)
#define SG_STATIC_ASSERT(expr) extern char _SG_ASSERT_CONCAT(STATIC_ASSERT_LINE_, __LINE__)[1]; \
                               extern char _SG_ASSERT_CONCAT(STATIC_ASSERT_LINE_, __LINE__)[(expr)?1:2]
#else
#define SG_STATIC_ASSERT_HELPER(expr) (!!sizeof(struct{unsigned int _SG_ASSERT_CONCAT(STATIC_ASSERT_LINE_, __LINE__): (expr)?1:-1;}))
#define SG_STATIC_ASSERT(expr) extern int (*assert_function__(void)) [SG_STATIC_ASSERT_HELPER(expr)]
#endif


//////////////////////////////////////////////////////////////////
// GCC now allows you to tag functions that take printf-style argument
// lists so the compiler can check for mismatches between format tokens
// and aguments like it does for the builtin printf, fprintf, strftime
// and etc.
//
// See -Wformat (implied by -Wall) in:
// http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
// http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html#Function-Attributes

/**
 * SG_DECLARE_PRINTF_PROTOTYPE
 * Declare a function to have a printf-style arg-list on platforms that use GCC.
 * Usage:
 *     SG_DECLARE_PRINTF_PROTOTYPE( void SG_console(SG_context * pCtx, SG_console_stream cs, const char * szFormat, ...), 3, 4);
 * where:
 *     index_of_format is the 1-based index of the format string;
 *     index_of_first_arg is the 1-based index of the first arg to be formatted.
 *
 */     
#if defined(MAC) || defined(LINUX)
#	define SG_DECLARE_PRINTF_PROTOTYPE( proto, index_of_format, index_of_first_arg )	proto __attribute__ ((format (printf, index_of_format, index_of_first_arg)))
#else
#	define SG_DECLARE_PRINTF_PROTOTYPE( proto, index_of_format, index_of_first_arg )	proto
#endif

#define SG_DRAWER_DIRECTORY_NAME ".sgdrawer"

//////////////////////////////////////////////////////////////////

#endif//H_SG_DEFINES_H
