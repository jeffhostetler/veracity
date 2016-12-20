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
 * @file    sg_log_prototypes.h
 * @details Prototypes for the SG_log module.
 *
 * This module provides functionality for emitting log and progress output from the program.
 * It maintains a stack of active "operations" and information about each.
 * It provides functions to manipulate the stack and also report on its current state.
 *
 * The operation stack is analagous to the actual call stack, but not as fine-grained.
 * A function that wishes to report progress pushes an operation onto the stack.
 * The newly pushed operation becomes the "current" operation.
 * While it's running, the function manipulates the current operation and reports on its progress.
 * When it's finished, the function pops the current operation off the stack.
 * If one function that reports progress calls another, then the operations will be inherently nested.
 *
 * Registered handlers are notified of changes to the stack, and can report them to the user.
 * Different types of handlers may report them in different ways (to a log file, STDOUT, a GUI, etc).
 * The host application usually registers its desired handlers at program startup.
 */

#ifndef H_SG_LOG_PROTOTYPES_H
#define H_SG_LOG_PROTOTYPES_H

BEGIN_EXTERN_C;

/*
**
** Reporting Interface
** Used by code that wishes to report output/progress to the user.
**
** The minimum work necessary to report data/progress is to push/pop an operation.
** Make sure that you ALWAYS pop any operation that you push, or the stack will unbalance.
** Unless otherwise noted, only call the other functions from INSIDE your push/pop calls.
** Otherwise you'll end up manipulating someone else's operation (or worse, if no other operations exist).
**
** None of the other reporting functions are required.
** However, the more of them that you can call with meaningful information, the more useful the resulting output will be.
** Additionally, all of the other reporting functions can be called in any order within the push/pop.
**
** It's generally fine to call these functions repeatedly with the same value, if that's convenient
** (as opposed to storing additional state to ensure that they are only called once per value).
** Handlers will only be notified about a value that actually changes.
**
*/

/**
 * Pushes a new operation onto the stack of running operations.
 *
 * Note: You should generally use the SG_log__push_operation macro instead of calling this directly.
 */
void SG_log__push_operation__internal(
	SG_context* pCtx,          //< [in] [out] Error and context information.
	const char* szDescription, //< [in] A description of the operation (see set_operation for guidelines).
	                           //<      This may be NULL if necessary, but not recommended.
	                           //<      Caller retains ownership of this pointer.
	                           //<      Must remain valid until the operation is popped.
	SG_uint32   uFlags,        //< [in] Combination of SG_log__flag values to associate with the operation.
	const char* szFile,        //< [in] Filename containing the call to this function.
	                           //<      Can be NULL if unavailable.
	SG_uint32   uLine          //< [in] The line number in szFile of the call to this function.
	                           //<      Can be zero if unavailable.
	);

/**
 * Wrapper around SG_log__push_operation__internal that automatically captures file/line information.
 */
#define SG_log__push_operation(pCtx, szDescription, uFlags) SG_log__push_operation__internal(pCtx, szDescription, uFlags, __FILE__, __LINE__)

/**
 * Sets a description for the current operation.
 * This function is usually unnecessary, passing the description to push_operation is preferred.
 * Using this is only helpful if you pass NULL to push_operation, or want to change the original description later.
 *
 * Operations should usually be described with a simple verb and noun (i.e. "Initializing data").
 * Use the present tense for verbs, because the user will see the description while processing occurs.
 * Keep descriptions reasonably short (a phrase, rather than a whole sentence).
 * Avoid using extraneous punctuation or whitespace like newlines, ellipses, or enclosing brackets/parentheses.
 * Avoid including values in the description (this is why no printf-style version of this function exists).
 * Use set_value or report_message for displaying text with values.
 * Following these guidelines will help ensure that the description works well in a variety of handlers.
 */
void SG_log__set_operation(
	SG_context* pCtx,         //< [in] [out] Error and context information.
	const char* szDescription //< [in] Description of the current operation (see function description for guidelines).
	                          //<      Caller retains ownership of this pointer.
	                          //<      Must remain valid until the current operation is popped.
	);

/*
 * Sets a name/value pair associated with the current operation.
 * This is generally data that the operation is going to use during
 * execution, which might also be of interest to the user.
 * An operation's values operate as a map/dictionary with unique name keys.
 * Setting a value with an already existing name will overwrite the old value.
 * A single operation cannot set more than SG_LOG__MAX_OPERATION_VALUES values.
 */
void SG_log__set_value__variant(
	SG_context*       pCtx,    //< [in] [out] Error and context information.
	const char*       szName,  //< [in] Name of the value.
	const SG_variant* pValue,  //< [in] The value to set.
	                           //<      If this value is a pointer, then the caller retains ownership.
	                           //<      If this value is a pointer, it must remain valid until the current operation is popped.
	SG_uint32         uFlags   //< [in] Combination of SG_log__flag values to associate with the value.
	);

/*
 * Convenient wrappers around SG_log__set_value__variant for various types.
 */
void SG_log__set_value__bool   (SG_context* pCtx, const char* szName, SG_bool           bValue,  SG_uint32 uFlags);
void SG_log__set_value__int    (SG_context* pCtx, const char* szName, SG_int64          iValue,  SG_uint32 uFlags);
void SG_log__set_value__double (SG_context* pCtx, const char* szName, double            dValue,  SG_uint32 uFlags);
void SG_log__set_value__sz     (SG_context* pCtx, const char* szName, const char*       szValue, SG_uint32 uFlags);
void SG_log__set_value__string (SG_context* pCtx, const char* szName, const SG_string*  pValue,  SG_uint32 uFlags);
void SG_log__set_value__vhash  (SG_context* pCtx, const char* szName, const SG_vhash*   pValue,  SG_uint32 uFlags);
void SG_log__set_value__varray (SG_context* pCtx, const char* szName, const SG_varray*  pValue,  SG_uint32 uFlags);

/**
 * Like SG_log__set_value__variant, except it allows the value to be identified
 * by a name/index pair, rather than just a name.  This is convenient for logging
 * the values in an array-like structure.
 */
void SG_log__set_value_index__variant(
	SG_context*       pCtx,    //< [in] [out] Error and context information.
	const char*       szName,  //< [in] Name of the value.
	SG_uint32         uIndex,  //< [in] Index of the value.
	const SG_variant* pValue,  //< [in] The value to set.
	                           //<      If this value is a pointer, then the caller retains ownership.
	                           //<      If this value is a pointer, it must remain valid until the current operation is popped.
	SG_uint32         uFlags   //< [in] Combination of SG_log__flag values to associate with the value.
	);

/*
 * Convenient wrappers around SG_log__set_value_index__variant for various types.
 */
void SG_log__set_value_index__bool   (SG_context* pCtx, const char* szName, SG_uint32 uIndex, SG_bool           bValue,  SG_uint32 uFlags);
void SG_log__set_value_index__int    (SG_context* pCtx, const char* szName, SG_uint32 uIndex, SG_int64          iValue,  SG_uint32 uFlags);
void SG_log__set_value_index__double (SG_context* pCtx, const char* szName, SG_uint32 uIndex, double            dValue,  SG_uint32 uFlags);
void SG_log__set_value_index__sz     (SG_context* pCtx, const char* szName, SG_uint32 uIndex, const char*       szValue, SG_uint32 uFlags);
void SG_log__set_value_index__string (SG_context* pCtx, const char* szName, SG_uint32 uIndex, const SG_string*  pValue,  SG_uint32 uFlags);
void SG_log__set_value_index__vhash  (SG_context* pCtx, const char* szName, SG_uint32 uIndex, const SG_vhash*   pValue,  SG_uint32 uFlags);
void SG_log__set_value_index__varray (SG_context* pCtx, const char* szName, SG_uint32 uIndex, const SG_varray*  pValue,  SG_uint32 uFlags);

/**
 * Sets the total number of steps required to complete the current operation.
 */
void SG_log__set_steps(
	SG_context* pCtx,  //< [in] [out] Error and context information.
	SG_uint32   uSteps,//< [in] The total number of steps in the current operation.
	                   //<      Pass zero to indicate that the number of steps is unknown.
	const char* szUnit //< [in] The unit for the steps, e.g. MB, kB, blobs.
	                   //       NULL or empty string are valid when there is no unit.
					   //       Caller retains ownership of the pointer, which must
					   //       remain valid until the current operation is popped.         
	);

/**
 * Sets a description for the current step.
 *
 * Steps should usually be described with a single value.
 * Often the value is representative of a loop index, be it an actual number or something like a current filename.
 * Avoid using extraneous punctuation or whitespace like newlines, ellipses, or enclosing brackets/parentheses.
 * Avoid including other values in the description (this is why no printf-style version of this function exists).
 * Use set_value or report_message for displaying text with values.
 * Following these guidelines will help ensure that the description works well in a variety of handlers.
 */
void SG_log__set_step(
	SG_context* pCtx,         //< [in] [out] Error and context information.
	const char* szDescription //< [in] A description of the current step of the current operation (see function description for guidelines).
	                          //<      Caller retains ownership of this pointer.
	                          //<      Must remain valid until the current operation is popped.
	);

/**
 * Increases the number of steps that are considered finished.
 * By the time the operation is completed, this total should equal the number given to set_steps.
 * If set_steps was never called (or passed zero), this function can still be used to indicate relative progress.
 */
void SG_log__finish_steps(
	SG_context* pCtx,  //< [in] [out] Error and context information.
	SG_uint32   uSteps //< [in] The amount to increase the number of finished steps by.
	);

/**
 * Convenient shortcut for finishing a single step.
 */
#define SG_log__finish_step(pCtx) SG_log__finish_steps(pCtx, 1u)

/**
 * Sets the number of steps that have been finished so far on the current operation.
 * This overrides the number of steps that were finished before (unlike finish_step(s), which increases that number).
 *
 * Depending on the operation's infrastructure, it may be more convenient to call this than finish_step(s).
 * Because sometimes it's easier to know the new total than it is to know the difference between the new total and the old one.
 */
void SG_log__set_finished(
	SG_context* pCtx,  //< [in] [out] Error and context information.
	SG_uint32   uSteps //< [in] The number of steps in the current operation that are finished.
	);

/**
 * Completes the current operation and removes it from the stack.
 * This will also record the context's current error state as the operation's result.
 *
 * WARNING: Don't wrap this with SG_ERR_IGNORE.
 *          If you do, the current error state recorded for the operation's result will be incorrect.
 */
void SG_log__pop_operation(
	SG_context* pCtx //< [in] [out] Error and context information.
	);

/**
 * Checks if someone has requested that the current operations be cancelled.
 * This can be called without having pushed your own operation onto the stack.
 *
 * If you're checking this, then you will best comply with handlers by:
 * 1) responding to a true return value by throwing SG_ERR_CANCEL.
 * 2) specifying the CAN_CANCEL flag on your operation(s).
 *
 * See SG_log__check_cancel for a shortcut that takes care of #1.
 */
void SG_log__cancel_requested(
	SG_context* pCtx,   //< [in] [out] Error and context information.
	SG_bool*    pCancel //< [out] True if cancellation has been requested, false if it hasn't.
	);

/**
 * Simple shortcut that throws SG_ERR_CANCEL if SG_log__cancel_requested returns true.
 */
void SG_log__check_cancel(
	SG_context* pCtx //< [in] [out] Error and context information.
	);

/**
 * Reports a message with information about the current operation.
 * This (and its wrappers) can be called without having pushed your own operation onto the stack.
 *
 * TODO: It would be handy to wrap this in a macro that captures file/line, like push_operation.
 *       However, that will require a variadic macro that allows zero arguments,
 *       which may not be possible in C on all the necessary compilers.
 */
SG_DECLARE_PRINTF_PROTOTYPE(
void SG_log__report_message(
	SG_context*          pCtx,     //< [in] [out] Error and context information.
	SG_log__message_type eType,    //< [in] The type of the message being reported.
	const char*          szFormat, //< [in] printf-style format string for the message.
	...                            //< [in] Values for placeholders in szFormat.
	)
, 3, 4);

/**
 * Implementation of SG_log__report_message.
 */
void SG_log__report_message__v(
	SG_context*          pCtx,     //< [in] [out] Error and context information.
	SG_log__message_type eType,    //< [in] The type of the message being reported.
	const char*          szFormat, //< [in] printf-style format string for the message.
	va_list              pArgs     //< [in] Values for placeholders in szFormat.
	);

/*
 * Convenient shortcuts for SG_log__report_message with a particular type.
 */
SG_DECLARE_PRINTF_PROTOTYPE(void SG_log__report_verbose(SG_context* pCtx, const char* szFormat, ...), 2, 3);
SG_DECLARE_PRINTF_PROTOTYPE(void SG_log__report_info   (SG_context* pCtx, const char* szFormat, ...), 2, 3);
SG_DECLARE_PRINTF_PROTOTYPE(void SG_log__report_warning(SG_context* pCtx, const char* szFormat, ...), 2, 3);
SG_DECLARE_PRINTF_PROTOTYPE(void SG_log__report_error  (SG_context* pCtx, const char* szFormat, ...), 2, 3);

/**
 * Convenient wrapper for SG_log__report_message that reports the current error state of the SG_context.
 *
 * WARNING: Don't wrap calls to this (or its wrappers) in SG_ERR_IGNORE.
 *          If you do, the current error state will be hidden.
 */
void SG_log__report_message__current_error(
	SG_context*          pCtx, //< [in] [out] Error and context information.
	                           //<            Also the context to report the error state of.
	SG_log__message_type eType //< [in] The type to report the error state as.
	);

/*
 * Convenient shortcuts for SG_log__report_message__current_error with a particular type.
 */
#define SG_log__report_verbose__current_error(pCtx) SG_log__report_message__current_error(pCtx, SG_LOG__MESSAGE__VERBOSE)
#define SG_log__report_info__current_error(pCtx)    SG_log__report_message__current_error(pCtx, SG_LOG__MESSAGE__INFO)
#define SG_log__report_warning__current_error(pCtx) SG_log__report_message__current_error(pCtx, SG_LOG__MESSAGE__WARNING)
#define SG_log__report_error__current_error(pCtx)   SG_log__report_message__current_error(pCtx, SG_LOG__MESSAGE__ERROR)

/**
 * For handlers that support being silenced, sets the value of the silenced flag.
 */
void SG_log__silence(
	SG_context* pCtx,
	SG_bool bSilence
	);

/*
**
** Receiving Interface
** Used by code that is receiving progress reports from running operations and doing something with them.
**
** This interface revolves around the typedefs of functions that handlers must implement.
** It also contains functions to retrieve data about operations and the current stack.
**
** A "handler" is a collection of functions that can be called to report data/progress.
** Each type of handler typically also has an associated config/state data structure.
** This structure is generally used to:
** 1) Configure options on the handler as to how it uses the reported progress data.
** 2) Give the handler a place to store internal state.
**
** A handler's functions might be called from many different threads simultaneously.
** In other words, handler implementations are expected to be thread safe.
** Handlers can distinguish between threads by checking the given SG_context's thread ID.
**
** Handler functions may call functions in the reporting interface if they desire.
** The log system will ensure that this doesn't lead to infinite recursion by not calling
** a handler while it is already handling a notification.
*/

/**
 * Gets basic data about an operation.
 */
void SG_log__operation__get_basic(
	SG_context*              pCtx,          //< [in] [out] Error and context information.
	const SG_log__operation* pOperation,    //< [in] The operation to get data about.
	const char**             ppDescription, //< [out] The operation's description.
	const char**             ppStep,        //< [out] Description of the operation's current step.
	SG_uint32*               uFlags         //< [out] The flags associated with the operation.
	);

/**
 * Gets timing data about an operation.
 * Note: All times are in milliseconds.  "Absolute" times are relative to 1/1/1970 0:00:00.000 UTC
 */
void SG_log__operation__get_time(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	SG_int64*                pStart,     //< [out] Time that the operation was pushed.
	SG_int64*                pLatest,    //< [out] Last time that any logging occurred on the stack.
	                                     //<       When called in response to a logging operation, this will be the time it occurred.
	                                     //<       Same pLatest you'd get from SG_log__stack__get_time.
	SG_int64*                pElapsed    //< [out] Time elapsed since the operation started.
	                                     //<       Equivalent to *pLatestTime - *pStartTime.
	);

/**
 * Gets source code data about an operation.
 */
void SG_log__operation__get_source(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	const char**             ppFile,     //< [out] Source file that pushed the operation.
	                                     //<       Set to NULL if the info is unavailable (such as in Release builds).
	SG_uint32*               pLine       //< [out] Line in the source file that pushed the operation.
	                                     //<       Unchanged if NULL is returned through ppFile.
	);

/**
 * Gets progress information about an operation.
 */
void SG_log__operation__get_progress(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	const char**             ppszUnit,   //< [out] The step unit, NULL if none.
	SG_uint32*               pFinished,  //< [out] Number of steps in the operation that are currently finished.
	SG_uint32*               pTotal,     //< [out] Total number of steps in the operation.
	SG_uint32*               pPercent,   //< [out] Completion percentage in the range [0,100].
	SG_uint32                uDefault    //< [in] Percent value to return if total step count is unknown.
	);

/**
 * Gets the number of values associated with an operation.
 */
void SG_log__operation__get_value_count(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	SG_uint32*               pCount      //< [out] The number of values in the operation.
	);

/**
 * Gets an operation value by index.
 * Use SG_log__operation__get_value_count to get the number of values.
 */
void SG_log__operation__get_value__index(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	SG_uint32                uIndex,     //< [in] The index of the value to retrieve.
	                                     //<      Must be less than pOperation->uValueCount.
	const char**             ppName,     //< [out] The name of the retrieved value.
	const SG_variant**       ppValue,    //< [out] The retrieved value.
	                                     //<       WARNING: Pointer value types should be considered const!
	SG_uint32*               pFlags      //< [out] The flags associated with the value.
	);

/**
 * Gets an operation value by name.
 */
void SG_log__operation__get_value__name(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	const char*              szName,     //< [in] The name of the value to retrieve.
	const SG_variant**       ppValue,    //< [out] The retrieved value.
	                                     //<       Set to NULL if the value wasn't found.
	                                     //<       WARNING: Pointer value types should be considered const!
	SG_uint32*               pFlags      //< [out] The flags associated with the value.
	);

/**
 * Gets the operation value that most recently changed.
 * Use this when handling SG_LOG__OPERATION__VALUE_SET.
 */
void SG_log__operation__get_value__latest(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	const char**             ppName,     //< [out] The name of the retrieved value.
	const SG_variant**       ppValue,    //< [out] The retrieved value.
	SG_uint32*               pFlags      //< [out] The flags associated with the value.
	);

/**
 * Gets result data about an operation.
 */
void SG_log__operation__get_result(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to get data about.
	SG_error*                pResult     //< [out] The operation's return/error code.
	);

/**
 * Gets process/thread data about the current operation stack.
 */
void SG_log__stack__get_threading(
	SG_context*    pCtx,     //< [in] [out] Error and context information.
	SG_process_id* pProcess, //< [out] ID of the process the stack is running in.
	SG_thread_id*  pThread   //< [out] ID of the thread the stack is running in.
	);

/**
 * Gets the number of operations on the current operation stack.
 */
void SG_log__stack__get_count(
	SG_context* pCtx,  //< [in] [out] Error and context information.
	SG_uint32*  pCount //< [out] The size of the current operation stack.
	);

/**
 * Retrieves all operations on the stack.
 * The top-most operation is last in the returned vector.
 */
void SG_log__stack__get_operations(
	SG_context* pCtx,  //< [in] [out] Error and context information.
	SG_vector** ppvOps //< [out] The operations currently on the stack.
	);

void SG_log__stack__get_operations__minimum_age(
	SG_context* pCtx,
    SG_int64 minimum_age,
	SG_vector** ppvOps
	);

/**
 * Gets timing data about the current operation stack.
 * Note: All times are in milliseconds.  "Absolute" times are relative to 1/1/1970 0:00:00.000 UTC.
 */
void SG_log__stack__get_time(
	SG_context* pCtx,   //< [in] [out] Error and context information.
	SG_int64*   pLatest //< [out] The last time that any logging occurred on the stack.
	);


/*
**
** Setup Interface
** Used by application-level code that is configuring progress reporting.
**
** This interface mainly just allows the application to manage the set of registered handlers.
** Applications will generally want to register handlers during startup, and unregister them during shutdown.
**
** Note that the list of registered handlers is global to the entire process.
** Also note that these functions can be safely called before global init and after global shutdown.
** This allows the startup/shutdown process to report progress normally.
**
*/

/**
 * Initializes global logger data.
 *
 * This can be called (and therefore the log system can be used)
 * before the main sg_lib__global_init call, though that function
 * also calls this one, so the only time you need to call this
 * directly is if you want to use the log system before calling
 * sg_lib__global_init (so that the init process itself can be
 * logged, for example).  If you do call this explicitly, make
 * sure you do it before you spawn any threads, because the log
 * system needs to setup thread-safety mechanisms before other
 * threads use it.
 *
 * This function can be called multiple times, as long as
 * SG_log__global_cleanup is called an equal number of times.
 */
void SG_log__global_init(
	SG_context* pCtx //< [in] [out] Error and context information.
	);

/**
 * Cleans up global logger data.
 * Should be called before before process exit.
 *
 * This can be called after sg_lib__global_cleanup, allowing
 * the cleanup process itself to use the logging system.
 * However, you only need to explicitly call this if you also
 * explicitly called SG_log__global_init.  See that function's
 * comments for more detail.  Similarly to that function, this
 * one should be called after other threads have ended so that
 * they don't try to log anything after the system is cleaned up.
 *
 * This function must be called as many times as
 * SG_log__global_init was called.
 */
void SG_log__global_cleanup(
	void
	);

/**
 * Registers a handler to start receiving progress reports.
 * This could fail if the handler can't initialize the given data correctly.
 */
void SG_log__register_handler(
	SG_context*            pCtx,         //< [in] [out] Error and context information.
	const SG_log__handler* pHandler,     //< [in] The handler to register.
	                                     //<      Caller retains ownership.
	                                     //<      Must remain valid as long as the handler is registered.
	void*                  pHandlerData, //< [in] The data that the handler should use.
	                                     //<      Caller retains ownership.
	                                     //<      Must remain valid as long as the handler is registered.
	SG_log__stats*         pStats,       //< [in] A structure to update with stats about the handler's usage.
	                                     //<      NULL if no stats are needed.
	                                     //<      Note: It's up to the caller to initialize with starting values!
	                                     //<            Usually you'll want to just use a simple memset to zero.
	SG_uint32              uFlags        //< [in] Flags to associate with the registered handler.
	);

/**
 * Removes a handler from the list of handlers receiving progress reports.
 */
void SG_log__unregister_handler(
	SG_context*            pCtx,        //< [in] [out] Error and context information.
	const SG_log__handler* pHandler,    //< [in] The handler to unregister.
	void*                  pHandlerData //< [in] The data of the handler to unregister.
	);

/**
 * Unregisters all currently registered handlers.
 * Make sure you call this before program shutdown (or manually unregister each registered handler).
 */
void SG_log__unregister_all_handlers(
	SG_context* pCtx //< [in] [out] Error and context information.
	);


/*
**
** Internal Interface
** Used by other sglib code internally, not needed for SG_log users.
**
*/

/**
 * Initializes a context for use with SG_log.
 * The rest of the interface relies on the given context having been initialized with this function.
 * This function should basically just be called by SG_context__alloc.
 */
SG_error SG_log__init_context(
	SG_context* pCtx //< [in] [out] The context to initialize.
	);

/**
 * Destroys the SG_log data in a context.
 * After calling this function, the given context can no longer be used with SG_log__* functions.
 * This function should basically just be called by SG_context__free.
 */
void SG_log__destroy_context(
	SG_context* pCtx //< [in] [out] The context to free.
	);

END_EXTERN_C;

#endif
