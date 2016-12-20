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
 * @file    sg_log_typedefs.h
 * @details Typedefs for the SG_log module.
 *          See the prototypes header for more information.
 */

#ifndef H_SG_LOG_TYPEDEFS_H
#define H_SG_LOG_TYPEDEFS_H

BEGIN_EXTERN_C;

/*
**
** Reporting Interface
** See prototypes file for more information.
**
*/

/**
 * Flags used by various portions of the logging system.
 */
typedef enum _sg_log__flag
{
	// flags for anything
	SG_LOG__FLAG__NONE                      =       0, //< The empty/none flag value.

	// flags for both operations and values
	SG_LOG__FLAG__VERBOSE                   = 1 <<  0, //< The operation/value contains data that an average user usually doesn't care about.

	// flags for operations
	SG_LOG__FLAG__CAN_CANCEL                = 1 <<  1, //< The operation checks for and honors cancel requests.

	// flags for values
	SG_LOG__FLAG__INPUT                     = 1 <<  2, //< Value is an input parameter to the operation.
	SG_LOG__FLAG__INTERMEDIATE              = 1 <<  3, //< Value is calculated and used during the operation.
	SG_LOG__FLAG__OUTPUT                    = 1 <<  4, //< Value is a result from the operation.

	// flags for handlers
	SG_LOG__FLAG__MULTIPLE                  = 1 <<  5, //< Allow the same handler/data pair to be registered several times.
	SG_LOG__FLAG__HANDLE_OPERATION__VERBOSE = 1 <<  6, //< This handler wants to receive operations flagged as verbose.
	SG_LOG__FLAG__HANDLE_OPERATION__NORMAL  = 1 <<  7, //< This handler wants to receive operations not flagged as verbose.
	SG_LOG__FLAG__HANDLE_OPERATION__ALL     = SG_LOG__FLAG__HANDLE_OPERATION__VERBOSE
	                                        | SG_LOG__FLAG__HANDLE_OPERATION__NORMAL,
	SG_LOG__FLAG__HANDLE_MESSAGE__VERBOSE   = 1 <<  8, //< This handler wants to receive verbose messages.
	SG_LOG__FLAG__HANDLE_MESSAGE__INFO      = 1 <<  9, //< This handler wants to receive info messages.
	SG_LOG__FLAG__HANDLE_MESSAGE__WARNING   = 1 << 10, //< This handler wants to receive warning messages.
	SG_LOG__FLAG__HANDLE_MESSAGE__ERROR     = 1 << 11, //< This handler wants to receive error messages.
	SG_LOG__FLAG__HANDLE_MESSAGE__ALL       = SG_LOG__FLAG__HANDLE_MESSAGE__VERBOSE
	                                        | SG_LOG__FLAG__HANDLE_MESSAGE__INFO
	                                        | SG_LOG__FLAG__HANDLE_MESSAGE__WARNING
	                                        | SG_LOG__FLAG__HANDLE_MESSAGE__ERROR,
	SG_LOG__FLAG__DETAILED_MESSAGES         = 1 << 12, //< Always receive the detailed form of reported messages.
	                                                   //< For example, messages derived from an SG_context will include an error code and stack trace.

	// last, for iteration purposes
	SG_LOG__FLAG__LAST                      = 1 << 13
}
SG_log__flag;

/*
 * A few handler "types", that are just pre-defined combinations of
 * which types of messages and operations to handle.  Basically like
 * pre-defined verbosity levels.
 */
#define SG_LOG__FLAG__HANDLER_TYPE__ALL    (SG_LOG__FLAG__HANDLE_MESSAGE__ALL | SG_LOG__FLAG__HANDLE_OPERATION__ALL)
#define SG_LOG__FLAG__HANDLER_TYPE__NORMAL (SG_LOG__FLAG__HANDLE_MESSAGE__INFO | SG_LOG__FLAG__HANDLE_MESSAGE__WARNING | SG_LOG__FLAG__HANDLE_MESSAGE__ERROR | SG_LOG__FLAG__HANDLE_OPERATION__NORMAL)
#define SG_LOG__FLAG__HANDLER_TYPE__QUIET  (SG_LOG__FLAG__HANDLE_MESSAGE__WARNING | SG_LOG__FLAG__HANDLE_MESSAGE__ERROR)

/*
 * A pseudo-flag that sets a handler to receive detailed messages in debug
 * builds and standard/brief messages in other builds.
 */
#if defined(DEBUG)
	#define SG_LOG__FLAG__DETAILED_DEBUG SG_LOG__FLAG__DETAILED_MESSAGES
#else
	#define SG_LOG__FLAG__DETAILED_DEBUG 0
#endif

/**
 * Different types of messages that can be reported.
 */
typedef enum _sg_log__message_type
{
	SG_LOG__MESSAGE__VERBOSE, //< Information that the average user generally doesn't care about.
	SG_LOG__MESSAGE__INFO,    //< Information that an average user usually cares about.
	SG_LOG__MESSAGE__WARNING, //< Information about a possible error condition that was ignored or recovered from.
	SG_LOG__MESSAGE__ERROR,   //< Information about an irrecoverable error condition that caused a failure.
	SG_LOG__MESSAGE__COUNT,   //< Number of elements in this enum, for iteration.
}
SG_log__message_type;


/*
**
** Receiving Interface
** See prototypes file for more information.
**
*/

/**
 * Data about a single operation.
 */
typedef struct _sg_log__operation SG_log__operation;

/**
 * Data about a stack of operations.
 */
typedef struct _sg_log__stack SG_log__stack;

/**
 * Methods of altering an operation.
 */
typedef enum _sg_log__operation_change
{
	SG_LOG__OPERATION__CREATED,        //< The operation was created.
	SG_LOG__OPERATION__PUSHED,         //< The operation was pushed onto the stack.
	SG_LOG__OPERATION__DESCRIBED,      //< The operation's description was changed.
	SG_LOG__OPERATION__VALUE_SET,      //< One of the operation's values was added/changed.
	SG_LOG__OPERATION__STEPS_SET,      //< The operation's total step count was changed.
	SG_LOG__OPERATION__STEP_DESCRIBED, //< The operation's current step's description was changed.
	SG_LOG__OPERATION__STEPS_FINISHED, //< The operation's finished step count was changed.
	SG_LOG__OPERATION__COMPLETED,      //< The operation was completed.
	SG_LOG__OPERATION__POPPED,         //< The operation was popped from the stack.
	SG_LOG__OPERATION__COUNT,          //< Number of elements in this enum, for iteration.
}
SG_log__operation_change;

/**
 * Type of callback function used to initialize the handler's config/state data.
 */
typedef void SG_log__handler__init(
	SG_context* pCtx, //< [in] [out] Error and context information.
	void*       pThis //< [in] The handler data to initialize.
	);

/**
 * Type of callback function used to notify a handler that the current operation has changed.
 */
typedef void SG_log__handler__operation(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	void*                    pThis,      //< [in] The handler's data.
	const SG_log__operation* pOperation, //< [in] The current operation.
	                                     //<      In the case of COMPLETED, this operation will no longer be on the stack.
	SG_log__operation_change eChange,    //< [in] The change that just occurred to the operation.
	SG_bool*                 pCancel     //< [out] Set to true if the handler wishes to cancel the operation.
	);

/**
 * Type of callback function used to notify a handler about a reported message.
 */
typedef void SG_log__handler__message(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	void*                    pThis,      //< [in] The handler's data.
	const SG_log__operation* pOperation, //< [in] The current operation.
	                                     //<      Might be NULL if there is no current operation.
	SG_log__message_type     eType,      //< [in] The type of message reported.
	const SG_string*         pMessage,   //< [in] The reported message.
	SG_bool*                 pCancel     //< [out] Set to true if the handler wishes to cancel the operation.
	);

/**
 * Type of callback function used to set a handlers "silenced" flag.
 */
typedef void SG_log__handler__silenced(
	SG_context* pCtx, //< [in] [out] Error and context information.
	void* pThis,      //< [in] The handler's data.
	SG_bool bSilenced //< [in] New value for the silenced flag.
	);

/**
 * Type of callback function used to destroy the user data provided with the handler.
 * This gives the handler a chance to cleanup its state.
 */
typedef void SG_log__handler__destroy(
	SG_context* pCtx, //< [in] [out] Error and context information.
	void*       pThis //< [in] The handler data to destroy.
	);

/**
 * A handler that can receive and process progress reports.
 * Any of the functions may be NULL if they should not be called.
 */
typedef struct _sg_log__handler
{
	SG_log__handler__init*      fInit;      //< Function to call to initialize the handler's data.
	SG_log__handler__operation* fOperation; //< Function to call when the current operation changes.
	SG_log__handler__message*   fMessage;   //< Function to call when a message is reported.
	SG_log__handler__silenced*  fSilenced;  //< Function to call to alter the silecned flag.
	SG_log__handler__destroy*   fDestroy;   //< Function to call to destroy the handler's data.
}
SG_log__handler;

/**
 * A structure containing statistics about the data handled by a log handler.
 */
typedef struct _sg_log__stats
{
	SG_uint32 uOperations_Total;                      //< Number of complete operations handled.
	SG_uint32 uOperations_Successful;                 //< Number of complete operations that returned SG_ERR_OK.
	SG_uint32 uOperations_Failed;                     //< Number of complete operations that returned an error.
	SG_uint32 uMessages_Total;                        //< Number of total messages handled.
	SG_uint32 uMessages_Type[SG_LOG__MESSAGE__COUNT]; //< Number of messages of each type handled.
}
SG_log__stats;


END_EXTERN_C;

#endif
