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
 * @file sg_thread_prototypes.h
 * @details Prototypes for the SG_thread module.
 *
 * This module implements cross-platform utilities for dealing with threads and processes.
 *
 */

#ifndef H_SG_THREAD_PROTOTYPES_H
#define H_SG_THREAD_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Gets the ID of the process that calls this function.
 */
void SG_thread__get_current_process(
	SG_context*    pCtx,    //< [in] [out] Error and context info.
	SG_process_id* pProcess //< [out] The ID of the current process.
	);

/**
 * Gets the ID of the thread that calls this function.
 */
void SG_thread__get_current_thread(
	SG_context*   pCtx,   //< [in] [out] Error and context info.
	SG_thread_id* pThread //< [out] The ID of the current thread.
	);

/**
 * Checks if two thread IDs are equal.
 */
void SG_thread__threads_equal(
	SG_context*  pCtx,     //< [in] [out] Error and context info.
	SG_thread_id cThread1, //< [in] The first of the two thread IDs to compare.
	SG_thread_id cThread2, //< [in] The second of the two thread IDs to compare.
	SG_bool*     pEqual    //< [out] Whether or not the two given threads are equal.
	);

/**
 * Checks if a given thread ID equals the current thread ID.
 */
void SG_thread__current_thread_equals(
	SG_context*  pCtx,    //< [in] [out] Error and context info.
	SG_thread_id cThread, //< [in] The thread ID to compare against the current thread.
	SG_bool*     pEqual   //< [out] Whether or not the given thread equals the current thread.
	);

/**
 * Converts a thread ID into a string for easy display.
 * Returns a pointer to a NULL terminated string containing the thread ID.
 *
 * Note that the returned string is basically arbitrary and platform-specific.
 * Its only real use is to be different than other thread strings,
 * so that threads can be differentiated in logs/output/etc.
 *
 * The string will be stored in the given buffer.
 * The return value will therefore simply point to the given buffer.
 * The buffer must have a length of at least SG_THREAD_TO_STRING_BUFFER_LENGTH.
 * An instance of type SG_thread_to_string_buffer may be used for convenience.
 */
const char* SG_thread__sz(
	SG_context*  pCtx,    //< [in] [out] Error and context info.
	SG_thread_id cThread, //< [in] The thread ID to convert to a string.
	char*        pBuffer  //< [in] [out] Buffer to store the converted string in.
	);

END_EXTERN_C;

#endif
