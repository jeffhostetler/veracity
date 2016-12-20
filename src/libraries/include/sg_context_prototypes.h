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
 * @file sg_context_prototypes.h
 *
 */
#ifndef H_SG_CONTEXT_PROTOTYPES_H
#define H_SG_CONTEXT_PROTOTYPES_H

BEGIN_EXTERN_C;

// A macro for creating a "sidecar" context, for callbacks and whatnot, when you already have a valid "main" context.
// DO NOT USE THIS for allocing the "main" context.
#define SG_CTX_ALLOC_W_ERR_CHECK(pp)	SG_STATEMENT(	SG_error _ctx_alloc_err;					\
														_ctx_alloc_err = SG_context__alloc(pp);		\
														if (SG_IS_ERROR(_ctx_alloc_err))			\
															SG_ERR_THROW(_ctx_alloc_err);			)


SG_error SG_context__alloc(SG_context** ppCtx);
SG_error SG_context__alloc__temporary(SG_context** ppCtx);

#if defined(DEBUG)
#define SG_CONTEXT__ALLOC(ppNew)	SG_STATEMENT(	SG_context * _pCtxNew = NULL;											\
														SG_context__alloc(&_pCtxNew);									\
														_sg_mem__set_caller_data(_pCtxNew,__FILE__,__LINE__,"SG_context");	\
														*(ppNew) = _pCtxNew;													)

#define SG_CONTEXT__ALLOC__TEMPORARY(ppNew)	SG_STATEMENT(	SG_context * _pCtxNew = NULL;											\
														SG_context__alloc__temporary(&_pCtxNew);									\
														_sg_mem__set_caller_data(_pCtxNew,__FILE__,__LINE__,"SG_context__temporary");	\
														*(ppNew) = _pCtxNew;													)
#else
#define SG_CONTEXT__ALLOC(ppNew)	SG_context__alloc(ppNew)
#define SG_CONTEXT__ALLOC__TEMPORARY(ppNew)	SG_context__alloc__temporary(ppNew)
#endif

/**
 * To keep the "foo__free(pCtx, pFoo)" form, we have to accept 2 args.
 * The 2nd is the one we are being asked to free.  The first should
 * probably be NULL.
 */
void SG_context__free(SG_context * pCtxToFree);

/**
 * Gets the ID of the process that the context is associated with.
 */
void SG_context__get_process(SG_context* pCtx, SG_process_id* pProcess);

/**
 * Gets the ID of the thread that the context is associated with.
 */
void SG_context__get_thread(SG_context* pCtx, SG_thread_id* pThread);

/**
 * Checks if the thread ID associated with the context equals the given thread ID.
 */
void SG_context__thread_equals(SG_context* pCtx, SG_thread_id cThread, SG_bool* pEquals);

/**
* SG_context__err: Set error state new_err with additional information szDescription.
* Description is additional error information, different from the message associated with the error code.
* To set the description printf-style, see SG_context__err_Set_description.
* pCtx must be non-null and NOT currently indicate an error state.
* Description will be truncated at 1023 bytes.
* The return code indicates the success or failure setting error state, and should not be propagated.
*/
SG_error SG_context__err(SG_context* pCtx, SG_error new_err, const char * szFileName, int iLine, const char* szDescription);

/**
* Set error state new_err with no additional information.
* pCtx must be non-null and NOT currently indicate an error state.
* The return code indicates the success or failure setting error state and should not be propagated.
*/
SG_error SG_context__err__generic(SG_context* pCtx, SG_error new_err, const char * szFileName, int iLine);

/**
* Sets the top-evel error to SG_ERR_SIGINT.
* Overwrites any existing error state.
* The return code indicates the success or failure setting error state and should not be propagated.
*/
SG_error SG_context__err__SIGINT(SG_context* pCtx, const char * szFileName, int iLine);

/**
* Replace error code in existing error state.
* pCtx must be non-null and currently indicate an error state.
* The return code indicates the success or failure setting error state and should not be propagated.
*/
SG_error SG_context__err_replace(SG_context* pCtx, SG_error new_err, const char * szFileName, int iLine);

/**
* Add information to an error state, printf-style.  This sets the same field as szDescription in SG_context__err.
* This method exists primarily so error-handling  macros can have an argument that gets passed entirely to it,
* e.g. SG_ERR_CHECK_INFO(err, (pCtx, "blah %s", szBlah));
* pCtx must be non-null and SHOULD currently indicate an error state.
* The return code indicates the success or failure of adding error info and should not be propagated.
*/
SG_DECLARE_PRINTF_PROTOTYPE( SG_error SG_context__err_set_description(SG_context* pCtx, const char* szFormat, ...), 2, 3);

/**
* Retrieve the error description string in pCtx.
* This is the context-specifix description string, not the string associated with an error code.
* The return code indicates the success or failure of retrieving the description and should not be propagated.
*
* You DO NOT own this string.
*/
SG_error SG_context__err_get_description(const SG_context* pCtx, const char** ppszInfo);

/**
* Retrieve the SG_error in pCtx.
* pCtx must be non-null.
* The return code indicates the success or failure of retrieving the error code and should not be propagated.
*/
SG_error SG_context__get_err(const SG_context* pCtx, SG_error* pErr);

/**
* Returns true if pCtx's error code equals err.
* Returns false if pCtx is NULL.
*/
SG_bool SG_context__err_equals(const SG_context* pCtx, SG_error err);

/**
* Determine if the provided SG_context indicates an error state.
* pCtx must be non-null.
* Returns a bool for ease of use in SG_ERR_CHECK-type macros.
*/
SG_bool SG_context__has_err(const SG_context* pCtx);
SG_bool SG_context__top_level_has_err(const SG_context* pCtx);

#define SG_CONTEXT__HAS_ERR(pCtx)	( SG_IS_ERROR( (pCtx)->errValues[ (pCtx)->level ] ) )

/**
* Determine if the buffer for holding the stack trace is full.
* SG_context__err_stackframe_add just stops adding information when the buffer is full:
* you probably don't want to call this, it's here for diagnostic/testing purposes.
*/
SG_bool SG_context__stack_is_full(const SG_context* pCtx);

/**
* Clear the error state in pCtx.
* Succeeds regardless of the current error state.
* pCtx must be non-null.
*
* We only clear the error value at the current error-on-error level
* (This allows fail-block code to do whatever it needs without affecting the original error).
*/
void SG_context__err_reset(SG_context* pCtx);

/**
* Add a stack frame to the error state.
* pCtx must be non-null and SHOULD currently indicate an error state.
* The return code indicates the success or failure of adding the stack frame and should not be propagated.
*/
SG_error SG_context__err_stackframe_add(SG_context* pCtx, const char* szFilename, SG_uint32 linenum);

SG_error SG_context__err_to_string(SG_context* pCtx, 
								   SG_bool bDetailed, // < When false, the source and error number are omitted for sglib errors and there's no stack trace.
								   SG_string** ppErrString);

/** Prints detailed error messages to the console in debug builds, non-detailed for release. */
SG_error SG_context__err_to_console(SG_context* pCtx, 
									SG_console_stream cs);

/**
 * Push an error-on-error level.  Level 0 is the primary level to be used for
 * all error reporting on the original error that we encounter.
 *
 * Level 1 and above are used to allow us to do arbitrarily-complex tasks in fail-blocks
 * without allocating a new context or breaking normal-context code that needs to be able
 * to set/rely on an error.  Each level gets its own SG_error value.  When the fail-block
 * is completed, we pop the level.  When operating at a higher level, we do not alter the
 * lower level.  This allows the fail-block to do what it needs without trashing the original
 * error -- so we can do the cleanup and then pop back and return the original error to the
 * caller.
 *
 * Note that levels > 0 only have an SG_error value; we silently eat any stackframes and
 * error description.  We only record that information for level 0.
 *
 * IT IS VERY IS IMPORTANT THAT pushes AND pops BE BALANCED.  DO NOT DO ANYTHING BETWEEN
 * THE 2 THAT COULD RETURN TO DO A GOTO PAST THE POP.
 */
void SG_context__push_level(SG_context * pCtx);

/**
 * Pop an error-on-error level.
 */
void SG_context__pop_level(SG_context * pCtx);

END_EXTERN_C;

#endif//H_SG_CONTEXT_PROTOTYPES_H

