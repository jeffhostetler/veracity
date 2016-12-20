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
 * @file    sg_log.c
 * @details Implements the SG_log module.
 */

#include <sg.h>
#include <math.h>


/*
**
** Macros
**
*/

/**
 * Handy macro for unlocking a mutex in a fail label.
 * If the context is already in an error state, it ignores any error from unlocking the mutex.
 *
 * This legitimately uses SG_ERR_CHECK_RETURN, despite being in a fail block, 
 * because we're in common cleanup code and it's already verified that 
 * we're not in an error state.
 */
#define _ERR_UNLOCK(pCtx, pMutex)                                \
	if (SG_CONTEXT__HAS_ERR(pCtx))                               \
	{                                                            \
		SG_mutex__unlock__bare(pMutex);                          \
	}                                                            \
	else                                                         \
	{                                                            \
		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, pMutex)  ); \
	}


/*
**
** Types
**
*/

/**
 * The maximum number of different values that can be set on a single operation.
 */
#define MAX_VALUES 32

/**
 * Data about a single operation that is on the stack of operations.
 *
 * Note: This structure is explicitly designed to not contain objects that dynamically allocate.
 */
struct _sg_log__operation
{
	SG_int64    iStartTime;               //< Time that the operation was created/started (milliseconds since 1970).
	SG_uint32   uFlags;                   //< Flags associated with the operation.
	const char* szFile;                   //< The filename of the function call that created this operation.
	SG_uint32   uLine;                    //< The line in szFile of the function call that created this operation.
	const char* szDescription;            //< Description of the operation (might be empty).
	const char* szStep;                   //< Description of the current step (might be empty).
	const char* szStepUnit;               //< The unit for steps, e.g. MB, kB, blobs. NULL/empty is valid.
	SG_uint32   uTotalSteps;              //< Total number of steps in the operation, or zero if unknown.
	SG_uint32   uFinishedSteps;           //< Number of steps that have been reported finished so far.
	SG_uint32   uValueCount;              //< Number of values currently associated with the operation (current length of aValue*).
	SG_uint32   uValueLatest;             //< Index of the value that was added/modified most recently (invalid if uValueCount == 0u).
	const char* aValueNames[MAX_VALUES];  //< Names of the operation's values.  Parallel with aValue*.
	SG_variant  aValueValues[MAX_VALUES]; //< Values of the operation's values.  Parallel with aValue*.
	SG_uint32   aValueFlags[MAX_VALUES];  //< Flags of the operation's values.  Parallel with aValue*.
	SG_error    eResult;                  //< The operation's completion/exit result.
};

/**
 * The maximum depth of the operation stack.
 */
#define MAX_DEPTH 32

/**
 * A stack of running operations.
 *
 * Note: This structure is explicitly designed to not contain objects that dynamically allocate.
 */
struct _sg_log__stack
{
	SG_log__operation aOperations[MAX_DEPTH]; //< Stack of currently running operations.
	SG_uint32         uCount;                 //< Number of operations on the stack (top-most is at uCount-1).
	SG_bool           bCancelling;            //< Whether or not we're currently cancelling the operations.
	SG_int64          iLatestTime;            //< Time that the stack (or an operation in it) was last changed (milliseconds since 1970).
};

/**
 * A stack of handlers being notified about something.
 *
 * We keep this stack so that we never notify a handler about operations/messages
 * that were created while it was already handling an operation/message.
 * This allows handlers to use the reporting interface like any other system would
 * without having to worry about infinite recursion.
 *
 * Note: This structure is explicitly designed to not contain objects that dynamically allocate.
 */
typedef struct _sg_log__handlers
{
	const SG_log__handler* aHandlers[MAX_DEPTH]; //< The stack of handlers we're currently notifying about something.
	SG_uint32              uCount;               //< Number of handlers on the stack (top-most is at uCount-1).
}
SG_log__handlers;

/**
 * The data that we store in an SG_context.
 *
 * Note: Dynamic memory should be avoided in SG_contexts,
 *       so this struct doesn't contain any dynamically allocating objects.
 */
typedef struct _sg_log__context
{
	SG_log__stack    cStack;           //< Stack of currently running operations.
	SG_log__handlers cCurrentHandlers; //< Stack of handlers currently being notified.
}
SG_log__context;

/**
 * Data about a single registered handler.
 */
typedef struct _SG_log__handler_registration
{
	const SG_log__handler*                pHandler; //< The registered handler.
	void*                                 pData;    //< Handler-specific data.
	SG_log__stats*                        pStats;   //< Statistics structure to update.
	                                                //< NULL if no stats are being kept.
	SG_uint32                             uFlags;   //< Flags used to register the handler.
	struct _SG_log__handler_registration* pNext;    //< Next handler in the global list.

}
SG_log__handler_registration;


/*
**
** Globals
**
*/

/**
 * A counter of how many times the log system has been initialized.
 */
static SG_uint32 guInitialized = 0u;

/**
 * Head pointer of a global linked list of registered loggers.
 */
static SG_log__handler_registration* gpFirstHandlerRegistration = NULL;

/**
 * Mutex that controls access to gpFirstHandlerRegistration.
 */
static SG_mutex gcFirstHandlerRegistration_Mutex;


/*
**
** Internal Functions
**
*/

/**
 * Updates the stack's latest time to the current time.
 */
static void _update_stack_time(
	SG_context* pCtx
	)
{
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &pCtx->pLogContext->cStack.iLatestTime)  );

fail:
	return;
}

/**
 * Retrieves the current operation in the given context.
 */
static void _get_current_operation(
	SG_context*         pCtx,        //< [in] [out] The context to get the current operation from.
	                                 //<            Also error and context information.
	SG_bool             bAllowNull,  //< [in] Whether or not returning a NULL operation is valid.
	                                 //<      If false, then an empty stack will result in a thrown error.
	SG_log__operation** ppOperation  //< [out] The retrieved operation.
	)
{
	SG_log__operation* pOperation = NULL;

	SG_NULLARGCHECK(ppOperation);

	SG_ASSERT(pCtx->pLogContext->cStack.uCount <= MAX_DEPTH);

	if (pCtx->pLogContext->cStack.uCount == 0u)
	{
		if (bAllowNull == SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "There is no current operation."));
		}
		else
		{
			pOperation = NULL;
		}
	}
	else
	{
		pOperation = pCtx->pLogContext->cStack.aOperations + (pCtx->pLogContext->cStack.uCount - 1u);
	}

	*ppOperation = pOperation;

fail:
	return;
}

/**
 * Gets the index of an operation value with a given name.
 * Returns SG_TRUE if the value was found, or SG_FALSE otherwise.
 */
static SG_bool _get_value_index(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to lookup a value in.
	const char*              szName,     //< [in] The name of the value to look up.
	SG_uint32*               pIndex      //< [out] The index of the specified value.
	)
{
	SG_uint32 uIndex = 0u;
	SG_UNUSED(pCtx);
	for (uIndex = 0u; uIndex < pOperation->uValueCount; ++uIndex)
	{
		if (strcmp(szName, pOperation->aValueNames[uIndex]) == 0)
		{
			*pIndex = uIndex;
			return SG_TRUE;
		}
	}
	return SG_FALSE;
}

/**
 * Checks if a given handler is in the stack of handlers currently being notified.
 * Returns SG_TRUE if the handler is in the stack or SG_FALSE if it isn't.
 *
 * If a handler is in this stack, it should never be sent any notifications.
 * This avoids infinite recursion that might occur if a handler reports its own messages/operations.
 */
static SG_bool _handler_in_current_stack(
	SG_context*            pCtx,    //< [in] [out] Context to get the current handler stack from.
	                                //<            Also error and context information.
	const SG_log__handler* pHandler //< [in] The handler to check the stack for.
	)
{
	SG_uint32 uIndex;

	for (uIndex = 0u; uIndex < pCtx->pLogContext->cCurrentHandlers.uCount; ++uIndex)
	{
		if (pHandler == pCtx->pLogContext->cCurrentHandlers.aHandlers[uIndex])
		{
			return SG_TRUE;
		}
	}

	return SG_FALSE;
}

/**
 * Pushes a handler onto the stack of handlers currently being notified.
 *
 * Handlers on this stack will never be notified about operations/messages.
 */
static void _push_current_handler(
	SG_context*            pCtx,    //< [in] [out] Context to manipulate the current handler stack in.
	                                //<            Also error and context information.
	const SG_log__handler* pHandler //< [in] The handler to push onto the current stack.
	)
{
	// make sure there's enough room on the stack to push another handler
	if (pCtx->pLogContext->cCurrentHandlers.uCount >= MAX_DEPTH)
	{
		SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "Not enough room on the current handler stack to push another handler"));
	}

	// place the handler in the stack
	pCtx->pLogContext->cCurrentHandlers.aHandlers[pCtx->pLogContext->cCurrentHandlers.uCount] = pHandler;
	pCtx->pLogContext->cCurrentHandlers.uCount += 1u;

fail:
	return;
}

/**
 * Pops a handler off the stack of handlers currently being notified.
 *
 * This will allow the handler to start receiving notifications again.
 */
static void _pop_current_handler(
	SG_context*             pCtx,     //< [in] [out] Context to manipulate the current handler stack in.
	                                  //<            Also error and context information.
	const SG_log__handler** ppHandler //< [out] The handler that was popped off the stack.
	)
{
	// make sure the stack isn't empty
	SG_ARGCHECK(pCtx->pLogContext->cCurrentHandlers.uCount > 0u, pCtx->pLogContext->cCurrentHandlers);

	// remove the handler from the stack
	pCtx->pLogContext->cCurrentHandlers.uCount -= 1u;

	// return the top handler if they want it
	if (ppHandler != NULL)
	{
		*ppHandler = pCtx->pLogContext->cCurrentHandlers.aHandlers[pCtx->pLogContext->cCurrentHandlers.uCount];
	}

fail:
	return;
}

/**
 * Checks if a registered handler wants to receive a given operation.
 */
static SG_bool _handler_wants_operation(
	const SG_log__handler_registration* pRegistration, //< The registration to check.
	const SG_log__operation*            pOperation     //< The operation to check for.
	)
{
	if ((pOperation->uFlags & SG_LOG__FLAG__VERBOSE) != 0)
	{
		return ((pRegistration->uFlags & SG_LOG__FLAG__HANDLE_OPERATION__VERBOSE) != 0) ? SG_TRUE : SG_FALSE;
	}
	else
	{
		return ((pRegistration->uFlags & SG_LOG__FLAG__HANDLE_OPERATION__NORMAL) != 0) ? SG_TRUE : SG_FALSE;
	}
}

/**
 * Notifies all registered handlers of a change to the current operation.
 */
static void _notify_handlers__operation(
	SG_context*              pCtx,       //< [in] [out] Error and context information.
	const SG_log__operation* pOperation, //< [in] The operation to notify the handlers about.
	                                     //<      If NULL, then the current operation will be used.
	SG_log__operation_change eChange     //< [in] The type of change that occurred to the operation.
	)
{
	SG_log__handler_registration* pRegistration = gpFirstHandlerRegistration;

	// if they didn't specify an operation, look up the current one
	if (pOperation == NULL)
	{
		SG_log__operation* pMutableOperation = NULL;
		SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pMutableOperation)  );
		pOperation = pMutableOperation;
	}

	// run through the set of registered handlers
	while (pRegistration != NULL)
	{
		SG_bool                bCancel        = SG_FALSE;
		const SG_log__handler* pPoppedHandler = NULL;

		// only give the operation to this handler if:
		// 1) it has an operation callback
		// 2) it wants to receive the operation
		// 3) it's not already in the operation stack (this avoids re-entering its operation callback)
		if (
			   pRegistration->pHandler->fOperation != NULL
			&& _handler_wants_operation(pRegistration, pOperation) == SG_TRUE
			&& _handler_in_current_stack(pCtx, pRegistration->pHandler) == SG_FALSE
			)
		{
			// push the handler onto the current stack
			// this avoids re-entering its notification function (possibly infinitely)
			SG_ERR_CHECK(  _push_current_handler(pCtx, pRegistration->pHandler)  );

			// if we're keeping stats on this handler, update them
			if (pRegistration->pStats != NULL)
			{
				if (eChange == SG_LOG__OPERATION__COMPLETED)
				{
					pRegistration->pStats->uOperations_Total += 1u;
					if (pOperation->eResult == SG_ERR_OK)
					{
						pRegistration->pStats->uOperations_Successful += 1u;
					}
					else
					{
						pRegistration->pStats->uOperations_Failed += 1u;
					}
				}
			}

			// notify the handler
			SG_ERR_CHECK(  pRegistration->pHandler->fOperation(pCtx, pRegistration->pData, pOperation, eChange, &bCancel)  );

			// pop the handler back off the stack
			SG_ERR_CHECK(  _pop_current_handler(pCtx, &pPoppedHandler)  );
			SG_ASSERT(pPoppedHandler == pRegistration->pHandler);

			// if they requested that the operation be cancelled, record that
			if (bCancel == SG_TRUE)
			{
				pCtx->pLogContext->cStack.bCancelling = SG_TRUE;
			}
		}

		// move on to the next handler
		pRegistration = pRegistration->pNext;
	}

fail:
	return;
}

/**
 * Notifies all registered handlers of a change to the silenced setting.
 */
static void _notify_handlers__silenced(
	SG_context* pCtx,     //< [in] [out] Error and context information.
	SG_bool		bSilenced //< [in] The new state of the silenced setting
	)
{
	SG_log__handler_registration* pRegistration = gpFirstHandlerRegistration;

	// run through the set of registered handlers
	while (pRegistration != NULL)
	{
		const SG_log__handler* pPoppedHandler = NULL;

		// only notify the handler if:
		// 1) it has a silenced callback
		// 2) it's not already in the operation stack (this avoids re-entering its operation callback)
		if (
			   pRegistration->pHandler->fSilenced != NULL
			&& _handler_in_current_stack(pCtx, pRegistration->pHandler) == SG_FALSE
			)
		{
			// push the handler onto the current stack
			// this avoids re-entering its notification function (possibly infinitely)
			SG_ERR_CHECK(  _push_current_handler(pCtx, pRegistration->pHandler)  );

			// notify the handler
			SG_ERR_CHECK(  pRegistration->pHandler->fSilenced(pCtx, pRegistration->pData, bSilenced)  );

			// pop the handler back off the stack
			SG_ERR_CHECK(  _pop_current_handler(pCtx, &pPoppedHandler)  );
			SG_ASSERT(pPoppedHandler == pRegistration->pHandler);
		}

		// move on to the next handler
		pRegistration = pRegistration->pNext;
	}

fail:
	return;
}

/**
 * Checks if a registered handler wants to receive a given type of message.
 */
static SG_bool _handler_wants_message(
	const SG_log__handler_registration* pRegistration, //< The handler registration to check.
	SG_log__message_type                eType          //< The type of message to check for.
	)
{
	switch (eType)
	{
	case SG_LOG__MESSAGE__VERBOSE:
		return ((pRegistration->uFlags & SG_LOG__FLAG__HANDLE_MESSAGE__VERBOSE) != 0) ? SG_TRUE : SG_FALSE;
	case SG_LOG__MESSAGE__INFO:
		return ((pRegistration->uFlags & SG_LOG__FLAG__HANDLE_MESSAGE__INFO) != 0) ? SG_TRUE : SG_FALSE;
	case SG_LOG__MESSAGE__WARNING:
		return ((pRegistration->uFlags & SG_LOG__FLAG__HANDLE_MESSAGE__WARNING) != 0) ? SG_TRUE : SG_FALSE;
	case SG_LOG__MESSAGE__ERROR:
		return ((pRegistration->uFlags & SG_LOG__FLAG__HANDLE_MESSAGE__ERROR) != 0) ? SG_TRUE : SG_FALSE;
	default:
		return SG_FALSE;
	}
}

/**
 * Notifies all registered handlers of a reported message
 */
static void _notify_handlers__message(
	SG_context*              pCtx,            //< [in] [out] Error and context information.
	const SG_log__operation* pOperation,      //< [in] The operation to notify the handlers about.
	                                          //<      If NULL, then the current operation will be used.
	SG_log__message_type     eType,           //< [in] The type of message being reported.
	const SG_string*         pMessage,        //< [in] The message being reported.
	const SG_string*         pDetailedMessage //< [in] The detailed form of the message being reported.
	                                          //<      If NULL, there is no detailed form of the message.
	                                          //<      If non-NULL, then handlers with the flag
	                                          //<      SG_LOG__FLAG__DETAILED_MESSAGES set will receive
	                                          //<      pDetailedMessage instead of pMessage.
	)
{
	SG_log__handler_registration* pRegistration = gpFirstHandlerRegistration;

	// if they didn't specify an operation, look up the current one
	if (pOperation == NULL)
	{
		SG_log__operation* pMutableOperation = NULL;
		SG_ERR_CHECK(  _get_current_operation(pCtx, SG_TRUE, &pMutableOperation)  );
		pOperation = pMutableOperation;
	}

	// run through the set of registered handlers
	while (pRegistration != NULL)
	{
		SG_bool                bCancel        = SG_FALSE;
		const SG_log__handler* pPoppedHandler = NULL;

		// only give the message to this handler if:
		// 1) it has a message callback
		// 2) it wants to receive the message
		// 3) it's not already in the operation stack (this avoids re-entering its operation callback)
		if (
			   pRegistration->pHandler->fMessage != NULL
			&& _handler_wants_message(pRegistration, eType) == SG_TRUE
			&& _handler_in_current_stack(pCtx, pRegistration->pHandler) == SG_FALSE
			)
		{
			const SG_string* pHandlerMessage = pMessage;

			// push the handler onto the current stack
			// this avoids re-entering its notification function (possibly infinitely)
			SG_ERR_CHECK(  _push_current_handler(pCtx, pRegistration->pHandler)  );

			// if we're keeping stats on this handler, update them
			if (pRegistration->pStats != NULL)
			{
				pRegistration->pStats->uMessages_Total += 1u;
				pRegistration->pStats->uMessages_Type[eType] += 1u;
			}

			// if we have a detailed form of the message and the handler wants it,
			// then deliver that instead
			if (
				   pDetailedMessage != NULL
				&& (pRegistration->uFlags & SG_LOG__FLAG__DETAILED_MESSAGES) == SG_LOG__FLAG__DETAILED_MESSAGES
				)
			{
				pHandlerMessage = pDetailedMessage;
			}

			// notify the handler
			SG_ERR_CHECK(  pRegistration->pHandler->fMessage(pCtx, pRegistration->pData, pOperation, eType, pHandlerMessage, &bCancel)  );

			// pop the handler back off the stack
			SG_ERR_CHECK(  _pop_current_handler(pCtx, &pPoppedHandler)  );
			SG_ASSERT(pPoppedHandler == pRegistration->pHandler);

			// if they requested that the operation be cancelled, record that
			if (bCancel == SG_TRUE)
			{
				pCtx->pLogContext->cStack.bCancelling = SG_TRUE;
			}
		}

		// move on to the next handler
		pRegistration = pRegistration->pNext;
	}

fail:
	return;
}

/**
 * Internal function that all message reporting funnels through.
 */
static void _report_message(
	SG_context*          pCtx,            //< [in] [out] Error and context information.
	SG_log__message_type eType,           //< [in] The type of message being reported.
	const SG_string*     pMessage,        //< [in] The message being reported.
	const SG_string*     pDetailedMessage //< [in] The detailed form of the message being reported.
	                                      //<      If NULL, there is no detailed form of the message.
	                                      //<      If non-NULL, then handlers with the flag
	                                      //<      SG_LOG__FLAG__DETAILED_MESSAGES set will receive
	                                      //<      pDetailedMessage instead of pMessage.
	)
{
	// update the stack time
	SG_ERR_CHECK(  _update_stack_time(pCtx)  );

	// notify our handlers
	SG_ERR_CHECK(  _notify_handlers__message(pCtx, NULL, eType, pMessage, pDetailedMessage)  );

fail:
	return;
}


/*
**
** Public Functions
**
*/

void SG_log__push_operation__internal(
	SG_context* pCtx,
	const char* szDescription,
	SG_uint32   uFlags,
	const char* szFile,
	SG_uint32   uLine
	)
{
	SG_log__operation* pOperation = NULL;

	SG_ASSERT(guInitialized > 0u);

	// make sure there's enough room on the stack to push another operation
	if (pCtx->pLogContext->cStack.uCount >= MAX_DEPTH)
	{
		SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "Not enough room on the operation stack to push another operation"));
	}

	// find the next available operation
	pOperation = pCtx->pLogContext->cStack.aOperations + pCtx->pLogContext->cStack.uCount;

	// set the basic operation values
	pOperation->szFile         = szFile;
	pOperation->uLine          = uLine;
	pOperation->uFlags         = uFlags;
	pOperation->szDescription  = szDescription;
	pOperation->szStep         = NULL;
	pOperation->szStepUnit     = NULL;
	pOperation->uTotalSteps    = 0u;
	pOperation->uFinishedSteps = 0u;
	pOperation->uValueCount    = 0u;
	pOperation->uValueLatest   = 0u;
	pOperation->eResult        = SG_ERR_OK;

	// look up the operation's start time
	SG_ERR_CHECK(  _update_stack_time(pCtx)  );
	pOperation->iStartTime = pCtx->pLogContext->cStack.iLatestTime;

	// notify our handlers
	SG_ERR_CHECK(  _update_stack_time(pCtx)  );
	SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__CREATED)  );

	// set the new operation as the current one
	pCtx->pLogContext->cStack.uCount += 1u;
	SG_ERR_CHECK(  _update_stack_time(pCtx)  );

	// notify our handlers
	SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__PUSHED)  );

fail:
	return;
}

void SG_log__set_operation(
	SG_context* pCtx,
	const char* szDescription
	)
{
	SG_log__operation* pOperation = NULL;

	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pOperation)  );
	if (SG_strcmp__null(pOperation->szDescription, szDescription) != 0)
	{
		pOperation->szDescription = szDescription;
		SG_ERR_CHECK(  _update_stack_time(pCtx)  );
		SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__DESCRIBED)  );
	}

fail:
	return;
}

void SG_log__set_value__variant(
	SG_context*       pCtx,
	const char*       szName,
	const SG_variant* pValue,
	SG_uint32         uFlags
	)
{
	SG_log__operation* pOperation = NULL;
	SG_uint32          uIndex     = 0u;
	SG_bool            bChanged   = SG_FALSE;

	SG_ASSERT(guInitialized > 0u);

	// get the current operation
	SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pOperation)  );

	// get the index of the named value
	if (_get_value_index(pCtx, pOperation, szName, &uIndex) == SG_TRUE)
	{
		// value already exists in the operation, just update its information
		SG_bool bEqual = SG_FALSE;

		// check if the new value is the same as the old
		SG_ERR_CHECK(  SG_variant__equal(pCtx, pValue, &(pOperation->aValueValues[uIndex]), &bEqual)  );
		bChanged = (bEqual == SG_FALSE || uFlags != pOperation->aValueFlags[uIndex]);

		// update the values
		if (bChanged == SG_TRUE)
		{
			pOperation->aValueValues[uIndex] = *pValue;
			pOperation->aValueFlags[uIndex]  = uFlags;
			SG_ERR_CHECK(  _update_stack_time(pCtx)  );
		}
	}
	else
	{
		// value doesn't exist in the operation yet, we'll have to add it
		bChanged = SG_TRUE;

		// make sure there's enough room in the operation for another value
		if (pOperation->uValueCount >= MAX_VALUES)
		{
			SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "Not enough room in the operation to set another value."));
		}

		// get the next index
		uIndex = pOperation->uValueCount;

		// set the value's information
		pOperation->aValueNames[uIndex]  = szName;
		pOperation->aValueValues[uIndex] = *pValue;
		pOperation->aValueFlags[uIndex]  = uFlags;

		// update the number of values
		pOperation->uValueCount += 1u;
		SG_ERR_CHECK(  _update_stack_time(pCtx)  );
	}

	// set the latest modified value index
	pOperation->uValueLatest = uIndex;

	// notify our handlers
	if (bChanged == SG_TRUE)
	{
		SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__VALUE_SET)  );
	}

fail:
	return;
}

void SG_log__set_value__bool(
	SG_context* pCtx,
	const char* szName,
	SG_bool     bValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_BOOL;
	cVariant.v.val_bool = bValue;
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, szName, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value__int(
	SG_context* pCtx,
	const char* szName,
	SG_int64    iValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_INT64;
	cVariant.v.val_int64 = iValue;
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, szName, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value__double(
	SG_context* pCtx,
	const char* szName,
	double      dValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_DOUBLE;
	cVariant.v.val_double = dValue;
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, szName, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value__sz(
	SG_context* pCtx,
	const char* szName,
	const char* szValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_SZ;
	cVariant.v.val_sz = szValue;
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, szName, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value__string(
	SG_context*      pCtx,
	const char*      szName,
	const SG_string* pValue,
	SG_uint32        uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_SZ;
	cVariant.v.val_sz = SG_string__sz(pValue);
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, szName, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value__vhash(
	SG_context*     pCtx,
	const char*     szName,
	const SG_vhash* pValue,
	SG_uint32       uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	// unfortunately we need to cast away the const, here
	// the calling code should certainly expect their value to not be modified
	// but there's not really a good way to get the const through to the handlers
	// so until there is, we'll trust them not to modify the value
	// it probably wouldn't occur to most handler implementations to modify it anyway
	cVariant.type = SG_VARIANT_TYPE_VHASH;
	cVariant.v.val_vhash = (SG_vhash*)pValue;
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, szName, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value__varray(
	SG_context*      pCtx,
	const char*      szName,
	const SG_varray* pValue,
	SG_uint32        uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	// unfortunately we need to cast away the const, here
	// the calling code should certainly expect their value to not be modified
	// but there's not really a good way to get the const through to the handlers
	// so until there is, we'll trust them not to modify the value
	// it probably wouldn't occur to most handler implementations to modify it anyway
	cVariant.type = SG_VARIANT_TYPE_VARRAY;
	cVariant.v.val_varray = (SG_varray*)pValue;
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, szName, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value_index__variant(
	SG_context*       pCtx,
	const char*       szName,
	SG_uint32         uIndex,
	const SG_variant* pValue,
	SG_uint32         uFlags
	)
{
	SG_string* sName = NULL;

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sName, "%s[%u]", szName, uIndex)  );
	SG_ERR_CHECK(  SG_log__set_value__variant(pCtx, SG_string__sz(sName), pValue, uFlags)  );

fail:
	SG_STRING_NULLFREE(pCtx, sName);
	return;
}

void SG_log__set_value_index__bool(
	SG_context* pCtx,
	const char* szName,
	SG_uint32   uIndex,
	SG_bool     bValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_BOOL;
	cVariant.v.val_bool = bValue;
	SG_ERR_CHECK(  SG_log__set_value_index__variant(pCtx, szName, uIndex, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value_index__int(
	SG_context* pCtx,
	const char* szName,
	SG_uint32   uIndex,
	SG_int64    iValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_INT64;
	cVariant.v.val_int64 = iValue;
	SG_ERR_CHECK(  SG_log__set_value_index__variant(pCtx, szName, uIndex, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value_index__double(
	SG_context* pCtx,
	const char* szName,
	SG_uint32   uIndex,
	double      dValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_DOUBLE;
	cVariant.v.val_double = dValue;
	SG_ERR_CHECK(  SG_log__set_value_index__variant(pCtx, szName, uIndex, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value_index__sz(
	SG_context* pCtx,
	const char* szName,
	SG_uint32   uIndex,
	const char* szValue,
	SG_uint32   uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_SZ;
	cVariant.v.val_sz = szValue;
	SG_ERR_CHECK(  SG_log__set_value_index__variant(pCtx, szName, uIndex, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value_index__string(
	SG_context*      pCtx,
	const char*      szName,
	SG_uint32        uIndex,
	const SG_string* pValue,
	SG_uint32        uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	cVariant.type = SG_VARIANT_TYPE_SZ;
	cVariant.v.val_sz = SG_string__sz(pValue);
	SG_ERR_CHECK(  SG_log__set_value_index__variant(pCtx, szName, uIndex, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value_index__vhash(
	SG_context*     pCtx,
	const char*     szName,
	SG_uint32       uIndex,
	const SG_vhash* pValue,
	SG_uint32       uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	// unfortunately we need to cast away the const, here
	// the calling code should certainly expect their value to not be modified
	// but there's not really a good way to get the const through to the handlers
	// so until there is, we'll trust them not to modify the value
	// it probably wouldn't occur to most handler implementations to modify it anyway
	cVariant.type = SG_VARIANT_TYPE_VHASH;
	cVariant.v.val_vhash = (SG_vhash*)pValue;
	SG_ERR_CHECK(  SG_log__set_value_index__variant(pCtx, szName, uIndex, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_value_index__varray(
	SG_context*      pCtx,
	const char*      szName,
	SG_uint32        uIndex,
	const SG_varray* pValue,
	SG_uint32        uFlags
	)
{
	SG_variant cVariant;

	SG_ASSERT(guInitialized > 0u);

	// unfortunately we need to cast away the const, here
	// the calling code should certainly expect their value to not be modified
	// but there's not really a good way to get the const through to the handlers
	// so until there is, we'll trust them not to modify the value
	// it probably wouldn't occur to most handler implementations to modify it anyway
	cVariant.type = SG_VARIANT_TYPE_VARRAY;
	cVariant.v.val_varray = (SG_varray*)pValue;
	SG_ERR_CHECK(  SG_log__set_value_index__variant(pCtx, szName, uIndex, &cVariant, uFlags)  );

fail:
	return;
}

void SG_log__set_steps(
	SG_context* pCtx,
	SG_uint32   uSteps,
	const char* szUnit
	)
{
	SG_log__operation* pOperation = NULL;

	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pOperation)  );
	if (pOperation->uTotalSteps != uSteps)
	{
		pOperation->uTotalSteps = uSteps;
		pOperation->szStepUnit = szUnit;
		SG_ERR_CHECK(  _update_stack_time(pCtx)  );
		SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__STEPS_SET)  );
	}

fail:
	return;
}

void SG_log__set_step(
	SG_context* pCtx,
	const char* szDescription
	)
{
	SG_log__operation* pOperation = NULL;

	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pOperation)  );
	if (SG_strcmp__null(pOperation->szStep, szDescription) != 0)
	{
		pOperation->szStep = szDescription;
		SG_ERR_CHECK(  _update_stack_time(pCtx)  );
		SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__STEP_DESCRIBED)  );
	}

fail:
	return;
}

void SG_log__finish_steps(
	SG_context* pCtx,
	SG_uint32   uSteps
	)
{
	SG_log__operation* pOperation = NULL;

	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pOperation)  );
	if (uSteps > 0u)
	{
		pOperation->uFinishedSteps += uSteps;
		SG_ERR_CHECK(  _update_stack_time(pCtx)  );
		SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__STEPS_FINISHED)  );
	}

fail:
	return;
}

void SG_log__set_finished(
	SG_context* pCtx,
	SG_uint32   uSteps
	)
{
	SG_log__operation* pOperation = NULL;

	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pOperation)  );
	if (pOperation->uFinishedSteps != uSteps)
	{
		pOperation->uFinishedSteps = uSteps;
		SG_ERR_CHECK(  _update_stack_time(pCtx)  );
		SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__STEPS_FINISHED)  );
	}

fail:
	return;
}

void SG_log__pop_operation(
	SG_context* pCtx
	)
{
	SG_log__operation* pOperation    = NULL;
	SG_error           uCurrentError = SG_ERR_OK;
	SG_error           uError        = SG_ERR_OK;

	SG_ASSERT(guInitialized > 0u);

	// get the current error state from the context
	// we need to do this BEFORE doing any of our own operations that might set an error!
	uError = SG_context__get_err(pCtx, &uCurrentError);

	// now push the caller's error down the stack, so we can set/check our own errors
	// note that we're doing this before anything in this function that might goto fail
	// that way in fail we can unconditionally pop our state off the stack
	SG_context__push_level(pCtx);

	// get the current operation
	SG_ERR_CHECK(  _get_current_operation(pCtx, SG_FALSE, &pOperation)  );

	// set the operation's result to the caller's error state that we retrieved earlier
	if (SG_IS_OK(uError))
	{
		pOperation->eResult = uCurrentError;
		SG_ERR_CHECK(  _update_stack_time(pCtx)  );
	}

	// notify our handlers
	SG_ERR_CHECK(  _update_stack_time(pCtx)  );
	SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__COMPLETED)  );

	// remove the operation from the stack
	pCtx->pLogContext->cStack.uCount -= 1u;
	SG_ERR_CHECK(  _update_stack_time(pCtx)  );

	// if the stack is now empty and we were in the process of cancelling
	// then we've successfully cancelled everything
	if (pCtx->pLogContext->cStack.uCount == 0u && pCtx->pLogContext->cStack.bCancelling == SG_TRUE)
	{
		pCtx->pLogContext->cStack.bCancelling = SG_FALSE;
	}

	// notify our handlers
	SG_ERR_CHECK(  _notify_handlers__operation(pCtx, pOperation, SG_LOG__OPERATION__POPPED)  );

fail:
	// pop our error state off the stack, returning to the caller's
	SG_context__pop_level(pCtx);
	return;
}

void SG_log__cancel_requested(
	SG_context* pCtx,
	SG_bool*    pCancel
	)
{
	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pCancel);

	*pCancel = pCtx->pLogContext->cStack.bCancelling;

fail:
	return;
}

void SG_log__check_cancel(
	SG_context* pCtx
	)
{
	SG_bool bCancel = SG_FALSE;

	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  SG_log__cancel_requested(pCtx, &bCancel)  );

	if (bCancel == SG_TRUE)
	{
		SG_ERR_THROW(SG_ERR_CANCEL);
	}

fail:
	return;
}

void SG_log__report_message(
	SG_context*          pCtx,
	SG_log__message_type eType,
	const char*          szFormat,
	...
	)
{
	va_list pArgs;

	SG_ASSERT(guInitialized > 0u);

	va_start(pArgs, szFormat);
	SG_ERR_CHECK(  SG_log__report_message__v(pCtx, eType, szFormat, pArgs)  );
	va_end(pArgs);

fail:
	return;
}

void SG_log__report_message__v(
	SG_context*          pCtx,
	SG_log__message_type eType,
	const char*          szFormat,
	va_list              pArgs
	)
{
	SG_string* pMessage = NULL;

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(szFormat);

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pMessage)  );
	SG_ERR_CHECK(  SG_string__vsprintf(pCtx, pMessage, szFormat, pArgs)  );

	SG_ERR_CHECK(  _report_message(pCtx, eType, pMessage, NULL)  );

fail:
	SG_STRING_NULLFREE(pCtx, pMessage);
	return;
}

void SG_log__report_verbose(
	SG_context* pCtx,
	const char* szFormat,
	...
	)
{
	va_list pArgs;

	SG_ASSERT(guInitialized > 0u);

	va_start(pArgs, szFormat);
	SG_ERR_CHECK(  SG_log__report_message__v(pCtx, SG_LOG__MESSAGE__VERBOSE, szFormat, pArgs)  );
	va_end(pArgs);

fail:
	return;
}

void SG_log__report_info(
	SG_context* pCtx,
	const char* szFormat,
	...
	)
{
	va_list pArgs;

	SG_ASSERT(guInitialized > 0u);

	va_start(pArgs, szFormat);
	SG_ERR_CHECK(  SG_log__report_message__v(pCtx, SG_LOG__MESSAGE__INFO, szFormat, pArgs)  );
	va_end(pArgs);

fail:
	return;
}

void SG_log__report_warning(
	SG_context* pCtx,
	const char* szFormat,
	...
	)
{
	va_list pArgs;

	SG_ASSERT(guInitialized > 0u);

	va_start(pArgs, szFormat);
	SG_ERR_CHECK(  SG_log__report_message__v(pCtx, SG_LOG__MESSAGE__WARNING, szFormat, pArgs)  );
	va_end(pArgs);

fail:
	return;
}

void SG_log__report_error(
	SG_context* pCtx,
	const char* szFormat,
	...
	)
{
	va_list pArgs;

	SG_ASSERT(guInitialized > 0u);

	va_start(pArgs, szFormat);
	SG_ERR_CHECK(  SG_log__report_message__v(pCtx, SG_LOG__MESSAGE__ERROR, szFormat, pArgs)  );
	va_end(pArgs);

fail:
	return;
}

void SG_log__report_message__current_error(
	SG_context*          pCtx,
	SG_log__message_type eType
	)
{
	SG_string* pMessage;
	SG_string* pDetailedMessage;
	SG_error   uError;

	SG_ASSERT(guInitialized > 0u);

	// get a brief/release error string from the context
	uError = SG_context__err_to_string(pCtx, SG_FALSE, &pMessage);
	if (SG_IS_ERROR(uError))
	{
		return;
	}

	// get a detailed/debug error string from the context
	uError = SG_context__err_to_string(pCtx, SG_TRUE, &pDetailedMessage);
	if (SG_IS_ERROR(uError))
	{
		return;
	}

	// make sure that we at least have a brief error string
	if (pMessage == NULL && pDetailedMessage == NULL)
	{
		// didn't get either one
		return;
	}
	else if (pMessage == NULL)
	{
		// only got a detailed one, swap it to the brief one
		// because _report_message must have at least a normal/brief format
		pMessage = pDetailedMessage;
		pDetailedMessage = NULL;
	}

	// ignore any errors in _report_message, so we don't lose the current error
	SG_ERR_IGNORE(  _report_message(pCtx, eType, pMessage, pDetailedMessage)  );

	SG_STRING_NULLFREE(pCtx, pMessage);
	SG_STRING_NULLFREE(pCtx, pDetailedMessage);
}

void SG_log__operation__get_basic(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	const char**             ppDescription,
	const char**             ppStep,
	SG_uint32*               pFlags
	)
{
	SG_UNUSED(pCtx);

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pOperation);

	if (ppDescription != NULL)
	{
		*ppDescription = pOperation->szDescription;
	}
	if (ppStep != NULL)
	{
		*ppStep = pOperation->szStep;
	}
	if (pFlags != NULL)
	{
		*pFlags = pOperation->uFlags;
	}

fail:
	return;
}

void SG_log__operation__get_time(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	SG_int64*                pStart,
	SG_int64*                pLatest,
	SG_int64*                pElapsed
	)
{
	SG_UNUSED(pCtx);

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pOperation);

	if (pStart != NULL)
	{
		*pStart = pOperation->iStartTime;
	}
	if (pLatest != NULL)
	{
		*pLatest = pCtx->pLogContext->cStack.iLatestTime;
	}
	if (pElapsed != NULL)
	{
		*pElapsed = pCtx->pLogContext->cStack.iLatestTime - pOperation->iStartTime;
	}

fail:
	return;
}

void SG_log__operation__get_source(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	const char**             ppFile,
	SG_uint32*               pLine
	)
{
	SG_UNUSED(pCtx);

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pOperation);

	if (ppFile != NULL)
	{
		*ppFile = pOperation->szFile;
	}
	if (ppFile != NULL && pOperation->szFile != NULL)
	{
		*pLine = pOperation->uLine;
	}

fail:
	return;
}

void SG_log__operation__get_progress(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	const char**             ppszUnit,
	SG_uint32*               pFinished,
	SG_uint32*               pTotal,
	SG_uint32*               pPercent,
	SG_uint32                uDefault
	)
{
	SG_UNUSED(pCtx);

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pOperation);

	if (ppszUnit != NULL)
	{
		*ppszUnit = pOperation->szStepUnit;
	}
	if (pFinished != NULL)
	{
		*pFinished = pOperation->uFinishedSteps;
	}
	if (pTotal != NULL)
	{
		*pTotal = pOperation->uTotalSteps;
	}
	if (pPercent != NULL)
	{
		if (pOperation->uTotalSteps == 0u)
		{
			*pPercent = uDefault;
		}
		else
		{
			*pPercent = (SG_uint32)floor(((float)pOperation->uFinishedSteps / (float)pOperation->uTotalSteps) * 100.0f);
		}
	}

fail:
	return;
}

void SG_log__operation__get_value_count(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	SG_uint32*               pCount
	)
{
	SG_UNUSED(pCtx);

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pOperation);

	if (pCount != NULL)
	{
		*pCount = pOperation->uValueCount;
	}

fail:
	return;
}

void SG_log__operation__get_value__index(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	SG_uint32                uIndex,
	const char**             ppName,
	const SG_variant**       ppValue,
	SG_uint32*               pFlags
	)
{
	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pOperation);
	SG_ARGCHECK(uIndex < pOperation->uValueCount, uIndex);

	if (ppName != NULL)
	{
		*ppName = pOperation->aValueNames[uIndex];
	}
	if (ppValue != NULL)
	{
		*ppValue = &(pOperation->aValueValues[uIndex]);
	}
	if (pFlags != NULL)
	{
		*pFlags = pOperation->aValueFlags[uIndex];
	}

fail:
	return;
}

void SG_log__operation__get_value__name(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	const char*              szName,
	const SG_variant**       ppValue,
	SG_uint32*               pFlags
	)
{
	SG_uint32 uIndex = 0u;

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pOperation);

	// get the index of the requested value
	if (_get_value_index(pCtx, pOperation, szName, &uIndex) == SG_TRUE)
	{
		// found it
		if (ppValue != NULL)
		{
			*ppValue  = &(pOperation->aValueValues[uIndex]);
		}
		if (pFlags != NULL)
		{
			*pFlags = pOperation->aValueFlags[uIndex];
		}
	}
	else
	{
		// couldn't find it
		if (ppValue != NULL)
		{
			*ppValue = NULL;
		}
	}

fail:
	return;
}

void SG_log__operation__get_value__latest(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	const char**             ppName,
	const SG_variant**       ppValue,
	SG_uint32*               pFlags
	)
{
	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  SG_log__operation__get_value__index(pCtx, pOperation, pOperation->uValueLatest, ppName, ppValue, pFlags)  );

fail:
	return;
}

void SG_log__operation__get_result(
	SG_context*              pCtx,
	const SG_log__operation* pOperation,
	SG_error*                pResult
	)
{
	SG_ASSERT(guInitialized > 0u);
	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pOperation);

	if (pResult != NULL)
	{
		*pResult = pOperation->eResult;
	}

fail:
	return;
}

void SG_log__stack__get_threading(
	SG_context*    pCtx,
	SG_process_id* pProcess,
	SG_thread_id*  pThread
	)
{
	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  SG_context__get_process(pCtx, pProcess)  );
	SG_ERR_CHECK(  SG_context__get_thread(pCtx, pThread)  );

fail:
	return;
}

void SG_log__stack__get_count(
	SG_context* pCtx,
	SG_uint32*  pCount
	)
{
	SG_ASSERT(guInitialized > 0u);

	if (pCount != NULL)
	{
		*pCount = pCtx->pLogContext->cStack.uCount;
	}
}

void SG_log__stack__get_operations(
	SG_context* pCtx,
	SG_vector** ppvOps
	)
{
	SG_vector* pv = NULL;
	SG_uint32 i;
	SG_uint32 stackSize;

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK_RETURN(ppvOps);

	stackSize = pCtx->pLogContext->cStack.uCount;
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pv, stackSize)  );

	for (i = 0; i < stackSize; i++)
		SG_ERR_CHECK(  SG_vector__append(pCtx, pv, &pCtx->pLogContext->cStack.aOperations[i], NULL)  );

	SG_RETURN_AND_NULL(pv, ppvOps);

fail:
	SG_VECTOR_NULLFREE(pCtx, pv);
}

void SG_log__stack__get_operations__minimum_age(
	SG_context* pCtx,
    SG_int64 minimum_age,
	SG_vector** ppvOps
	)
{
	SG_vector* pv = NULL;
	SG_uint32 i;
	SG_uint32 stackSize;
    SG_uint32 count = 0;
    SG_int64 now = -1;

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK_RETURN(ppvOps);

	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &now)  );

	stackSize = pCtx->pLogContext->cStack.uCount;
    i = stackSize - 1;
    do
    {
        SG_int64 age = now - pCtx->pLogContext->cStack.aOperations[i].iStartTime;
        if (age > minimum_age)
        {
            count = i+1;
            break;
        }

        if (0 == i)
        {
            count = 0;
            break;
        }

        i--;
    } while (1);

    if (count)
    {
        SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pv, count)  );

        for (i = 0; i < count; i++)
        {
            SG_ERR_CHECK(  SG_vector__append(pCtx, pv, &pCtx->pLogContext->cStack.aOperations[i], NULL)  );
        }
    }

	SG_RETURN_AND_NULL(pv, ppvOps);

fail:
	SG_VECTOR_NULLFREE(pCtx, pv);
}

void SG_log__stack__get_time(
	SG_context* pCtx,
	SG_int64*   pLatest
	)
{
	SG_ASSERT(guInitialized > 0u);

	if (pLatest != NULL)
	{
		*pLatest = pCtx->pLogContext->cStack.iLatestTime;
	}
}

void SG_log__silence(
	SG_context* pCtx,
	SG_bool bSilence
	)
{
	SG_ASSERT(guInitialized > 0u);

	SG_ERR_CHECK(  _notify_handlers__silenced(pCtx, bSilence)  );

fail:
	return;
}

void SG_log__global_init(
	SG_context* pCtx
	)
{
	// WARNING: This function must be valid to call before SG_lib__global_initialize.

	if (guInitialized == 0u)
	{
		SG_ERR_CHECK(  SG_mutex__init(pCtx, &gcFirstHandlerRegistration_Mutex)  );
	}

	guInitialized += 1u;

fail:
	return;
}

void SG_log__global_cleanup()
{
	// WARNING: This function must be valid to call after SG_lib__global_cleanup.

	if (guInitialized == 1u)
	{
		SG_mutex__destroy(&gcFirstHandlerRegistration_Mutex);
	}

	guInitialized -= 1u;
}

void SG_log__register_handler(
	SG_context*            pCtx,
	const SG_log__handler* pHandler,
	void*                  pHandlerData,
	SG_log__stats*         pStats,
	SG_uint32              uFlags
	)
{
	// WARNING: This function must be valid to call before SG_lib__global_initialize.

	SG_log__handler_registration* pRegistration = NULL;

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pHandler);

	// make sure only one thread is messing with gpFirstHandlerRegistration at a time
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &gcFirstHandlerRegistration_Mutex)  );

	// unless they specified SG_LOG__FLAG__MULTIPLE
	// check if this handler/data pair is already registered
	if ((uFlags & SG_LOG__FLAG__MULTIPLE) == 0)
	{
		SG_log__handler_registration* pCurrent = gpFirstHandlerRegistration;

		while (pCurrent != NULL)
		{
			if (pCurrent->pHandler == pHandler && pCurrent->pData == pHandlerData)
			{
				goto fail; // early return
			}
			pCurrent = pCurrent->pNext;
		}
	}

	// initialize the handler's data
	if (pHandler->fInit != NULL)
	{
		SG_ERR_CHECK(  pHandler->fInit(pCtx, pHandlerData)  );
	}

	// allocate a registration and set its data
	SG_ERR_CHECK(  SG_alloc1(pCtx, pRegistration)  );
	pRegistration->pHandler = pHandler;
	pRegistration->pData    = pHandlerData;
	pRegistration->pStats   = pStats;
	pRegistration->uFlags   = uFlags;

	// wire the registration into the global linked list
	pRegistration->pNext = gpFirstHandlerRegistration;
	gpFirstHandlerRegistration = pRegistration;
	pRegistration = NULL;

	/* Fall through to common cleanup. */
fail:
	SG_NULLFREE(pCtx, pRegistration);
	_ERR_UNLOCK(pCtx, &gcFirstHandlerRegistration_Mutex);
}

void SG_log__unregister_handler(
	SG_context*            pCtx,
	const SG_log__handler* pHandler,
	void*                  pHandlerData
	)
{
	// WARNING: This function must be valid to call after SG_lib__global_cleanup.

	SG_log__handler_registration* pCurrent      = NULL;
	SG_log__handler_registration* pPrevious     = NULL;
	SG_log__handler_registration* pRegistration = NULL;

	SG_ASSERT(guInitialized > 0u);
	SG_NULLARGCHECK(pHandler);

	// make sure only one thread is messing with gpFirstHandlerRegistration at a time
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &gcFirstHandlerRegistration_Mutex)  );

	// Run through the registration list and find the specified one.
	// This is a little tricky, because the global list is singly linked.
	// So to remove entry N, we need to modify entry N-1 to point to entry N+1.
	// The tricky part is once we've iterated through to N, we've lost our pointer to N-1.
	// So we need to keep track of pointers to both N and N-1.
	pCurrent = gpFirstHandlerRegistration;
	while (pCurrent != NULL)
	{
		if (pCurrent->pHandler == pHandler && pCurrent->pData == pHandlerData)
		{
			break;
		}
		SG_ASSERT(pPrevious == NULL || pPrevious->pNext == pCurrent);
		pPrevious = pCurrent;
		pCurrent = pCurrent->pNext;
	}
	if (pCurrent == NULL)
	{
		// didn't find it
		goto fail; // early return
	}

	// remove the handler from the global list
	if (pPrevious == NULL)
	{
		// if the previous pointer is still NULL
		// then our current pointer must be the first one in the list
		SG_ASSERT(pCurrent == gpFirstHandlerRegistration);
		gpFirstHandlerRegistration = pCurrent->pNext;
	}
	else
	{
		pPrevious->pNext = pCurrent->pNext;
	}

	// move the pointer to one that we know we own
	pRegistration = pCurrent;
	pCurrent      = NULL;
	pPrevious     = NULL;

	// destroy the data
	// Note: It's important that this occurs after the handler is removed from the global list.
	//       Otherwise any log reporting functions it calls from its destroy handler
	//       might cause its operation/message handlers to be invoked.
	if (pRegistration->pHandler->fDestroy != NULL)
	{
		SG_ERR_CHECK(  pRegistration->pHandler->fDestroy(pCtx, pRegistration->pData)  );
	}

	/* Fall through to common cleanup */
fail:
	// free the entry
	SG_NULLFREE(pCtx, pRegistration);
	_ERR_UNLOCK(pCtx, &gcFirstHandlerRegistration_Mutex);
}

void SG_log__unregister_all_handlers(
	SG_context* pCtx
	)
{
	// WARNING: This function must be valid to call after SG_lib__global_cleanup.

	SG_ASSERT(guInitialized > 0u);

	// make sure only one thread is messing with gpFirstHandlerRegistration at a time
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &gcFirstHandlerRegistration_Mutex)  );

	// unregister every handler in the list
	while (gpFirstHandlerRegistration != NULL)
	{
		SG_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpFirstHandlerRegistration->pHandler, gpFirstHandlerRegistration->pData)  );
	}

	/* Fall through to common cleanup */
fail:
	_ERR_UNLOCK(pCtx, &gcFirstHandlerRegistration_Mutex);
}

SG_error SG_log__init_context(
	SG_context* pCtx
	)
{
	// WARNING: This function must be valid to call before SG_lib__global_initialize.
	// WARNING: This function must be valid to call before SG_log__global_init.

	SG_error         uError       = SG_ERR_OK;
	SG_log__context* pContextData = NULL;

	// if our data is already initialized, then we have nothing to do
	if (pCtx->pLogContext != NULL)
	{
		return SG_ERR_OK;
	}

	// allocate our context data
	uError = SG_alloc_err(1, sizeof(SG_log__context), &pContextData);
	if (SG_IS_ERROR(uError))
	{
		return uError;
	}

	// configure misc data
	pContextData->cCurrentHandlers.uCount = 0u;

	// configure the stack data
	pContextData->cStack.uCount      = 0u;
	pContextData->cStack.bCancelling = SG_FALSE;
	pContextData->cStack.iLatestTime = 0;
	SG_ERR_IGNORE(  SG_time__get_milliseconds_since_1970_utc(pCtx, &pContextData->cStack.iLatestTime)  );

	// return the data
	pCtx->pLogContext = pContextData;
	return uError;
}

void SG_log__destroy_context(
	SG_context* pCtx
	)
{
	// WARNING: This function must be valid to call after SG_lib__global_cleanup.
	// WARNING: This function must be valid to call after SG_log__global_cleanup.

	// if our data isn't initialized, then we have nothing to free
	if (pCtx->pLogContext == NULL)
	{
		return;
	}

	// free the data
	SG_free(pCtx, pCtx->pLogContext);
}
