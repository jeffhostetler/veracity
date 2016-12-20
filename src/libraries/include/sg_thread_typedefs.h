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
 * @file sg_thread_typedefs.h
 * @details Typedefs for the SG_thread module.
 *          See the prototypes header for more information.
 *
 */

#ifndef H_SG_THREAD_TYPEDEFS_H
#define H_SG_THREAD_TYPEDEFS_H

BEGIN_EXTERN_C;

/**
 * Type of unique IDs associated with a thread.
 *
 * Note that thread IDs are only necessarily unique within their process.
 * WARNING: Don't use the equality operator on these, use SG_thread__threads_equal instead.
 */
#if defined(WINDOWS)
	typedef DWORD SG_thread_id;
#endif
#if defined(MAC) || defined(LINUX)
	typedef pid_t SG_thread_id;
#endif

/**
 * Length of buffer needed to store a thread ID converted into a string.
 * Current length assumes that sizeof(SG_thread_id) <= 4.
 */
#define SG_THREAD_TO_STRING_BUFFER_LENGTH 9
SG_STATIC_ASSERT(SG_THREAD_TO_STRING_BUFFER_LENGTH >= ((sizeof(SG_thread_id) * 2) + 1));

/**
 * A type of buffer used to convert a thread ID into a string for display.
 */
typedef char SG_thread_to_string_buffer[SG_THREAD_TO_STRING_BUFFER_LENGTH];

END_EXTERN_C;

#endif
