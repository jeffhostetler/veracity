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
 * @file sg_exec_typedefs.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_EXEC_TYPEDEFS_H
#define H_SG_EXEC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Exit status from a child process.  This is command-specific and
 * somewhat platform-specific.  I have these typedefs so that we
 * don't confuse them with SG_error error codes (and as opposed to
 * adding a different SG_ERR_ domain).
 *
 * Generally, we can only test for 0 and non-zero.
 * The command-line diff/merge utilities typically return 0,1,2,3
 * to indicate the result whether there were changes, conflicts, errors.
 * YMMV.
 *
 * On Linux/Mac the exit status returned from waitpid() gives us
 * the low 8 bits of what the child passed to exit (assuming that
 * the child exited normally).
 */
typedef SG_int32 SG_exit_status;

#define SG_EXIT_STATUS_OK		((SG_exit_status) 0)
#define SG_EXIT_STATUS_BOGUS	((SG_exit_status) -1)

/**
 * Exec result.  This is an indication of whether or not the exec
 * succeeded and/or did the child finish.  It DOES NOT indicate the
 * exit status.
 */
typedef SG_uint32 SG_exec_result;

#define SG_EXEC_RESULT__NORMAL     ((SG_exec_result)0x000000)   // child started and completed normally
#define SG_EXEC_RESULT__FAILED     ((SG_exec_result)0x010000)   // child could not be started
#define SG_EXEC_RESULT__ABNORMAL   ((SG_exec_result)0x020000)   // child crashed or was killed (OR'd with SIG)
#define SG_EXEC_RESULT__UNKNOWN    ((SG_exec_result)0x040000)   // no child ?
#define SG_EXEC_RESULT__MASK_TYPE  ((SG_exec_result)0xff0000)   // main result bits
#define SG_EXEC_RESULT__MASK_VALUE ((SG_exec_result)0x00ffff)	// optional additional info (like SIG)

#define SG_EXEC_RESULT__SET_ABNORMAL(v)    (SG_EXEC_RESULT__ABNORMAL | ((v) & SG_EXEC_RESULT__MASK_VALUE))
#define SG_EXEC_RESULT__GET_SUB_VALUE(r)   ((r) & SG_EXEC_RESULT__MASK_VALUE)

/**
 * Process Identifier.
 */
typedef SG_uint64 SG_process_id;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_EXEC_TYPEDEFS_H
