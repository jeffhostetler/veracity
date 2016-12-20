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
 * @file u0066_log.c
 *
 * @details Tests sg_log.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"

/*
**
** Types
**
*/

// forward declarations of our handler implementation functions
static SG_log__handler__init      _handler__init;
static SG_log__handler__operation _handler__operation;
static SG_log__handler__message   _handler__message;
static SG_log__handler__destroy   _handler__destroy;

/**
 * An enumeration of possible calls that can be made to our handler.
 */
typedef enum
{
	EXPECT_INIT,
	EXPECT_OPERATION,
	EXPECT_MESSAGE,
	EXPECT_DESTROY,
}
u0066__expectation__type;

/**
 * Details about a single expected operation.
 */
typedef struct
{
	const char* szDescription;
	const char* szStep;
	SG_uint32   uFlags;
	const char* szFile;
	SG_uint32   uLine;
	SG_uint32   uFinished;
	SG_uint32   uTotal;
	SG_uint32   uPercent;
	SG_uint32   uValueCount;
	// TODO: add checking the actual value names/values/flags
	SG_error    uResult;
}
u0066__expectation__operation;

/**
 * Prototype of a function to call in response to our handler being called.
 */
typedef void u0066__reaction(
	SG_context* pCtx //< [in] [out] Error and context info.
	);

/**
 * Expected data about a single call to our handler.
 */
typedef struct
{
	u0066__expectation__type      eType;             //< type of call expected
	SG_uint32                     uStackCount;       //< expected stack size during the call
	u0066__expectation__operation cCurrentOperation; //< expected values for the current operation during the call (only during operation and message calls)
	SG_log__operation_change      eOperationChange;  //< expected type of operation change (only during operation calls)
	SG_log__message_type          eMessageType;      //< expected type of message (only during message calls)
	const char*                   szMessageString;   //< expected message string (only during message calls)
	u0066__reaction*              fReaction;         //< function to call in reaction to the handler being called (or NULL to not call one)
	SG_bool                       bCancel;           //< whether or not to return a cancellation from the call
}
u0066__expectation__data;

/**
 * Maximum number of expectations we'll allocate room for in a list.
 */
#define MAX_EXPECTATIONS 32

/**
 * Maximum size of expected operation stack.
 */
#define MAX_OPERATIONS 32

/**
 * Index to set to the list when there is no index associated with the current test.
 */
#define UNKNOWN_TEST_INDEX ((SG_uint32)-1)

/**
 * List of data about expected calls to our handler.
 * Passed as our handler's data.
 */
typedef struct
{
	SG_uint32 uTestIndex; //< Index of the test case currently being executed (used in failure output, UNKNOWN_TEST_INDEX if not running an indexed test)

	SG_process_id cProcess; //< ID of the process that the list is being expected in
	SG_thread_id  cThread;  //< ID of the thread that the list is being expected in

	u0066__expectation__data aExpectations[MAX_EXPECTATIONS]; //< list of expectations in their expected order
	SG_uint32                uExpectationCount;               //< number of valid expectations in aExpectations
	SG_uint32                uNextExpectation;                //< the index of the one to match against the next call

	u0066__expectation__operation aStack[MAX_OPERATIONS]; //< current expected stack of operations
	SG_uint32                     uStackCount;            //< number of valid operations in the stack
}
u0066__expectation__list;


/*
**
** Globals
**
*/

/**
 * Global instance of our handler.
 */
static const SG_log__handler gcTestHandler =
{
	_handler__init,
	_handler__operation,
	_handler__message,
	NULL,
	_handler__destroy,
};
static const SG_log__handler* gpTestHandler = &gcTestHandler;


/*
**
** Internal Functions
**
*/

/**
 * Initializes a list of expectations.
 */
static void _list__init(
	SG_context*               pCtx,      //< [in] [out] Error and context info.
	u0066__expectation__list* pList,     //< [in] [out] The list to initialize.
	SG_uint32                 uTestIndex //< [in] Index of the test being initialized for (or UNKNOWN_TEST_INDEX)
	)
{
	SG_NULLARGCHECK(pList);

	pList->uTestIndex = uTestIndex;
	pList->uExpectationCount = 0u;
	pList->uNextExpectation  = 0u;
	pList->uStackCount   = 0u;
	//zero out the stack.
	memset(pList->aStack, 0, MAX_OPERATIONS * sizeof(u0066__expectation__operation)  );
	memset(pList->aExpectations, 0, MAX_OPERATIONS * sizeof(u0066__expectation__data)  );
	VERIFY_ERR_CHECK(  SG_thread__get_current_process(pCtx, &pList->cProcess)  );
	VERIFY_ERR_CHECK(  SG_thread__get_current_thread(pCtx, &pList->cThread)  );

fail:
	return;
}

/**
 * Gets the most recent expectation from the list.
 */
static void _list__get_expectation(
	SG_context*                pCtx,  //< [in] [out] Error and context info.
	u0066__expectation__list*  pList, //< [in] [out] The list to get an expectation from.
	u0066__expectation__data** ppData //< [out] The list's current expectation.
	)
{
	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pList);
	SG_ARGCHECK(pList->uExpectationCount > 0u, pList->uExpectationCount);

	*ppData = pList->aExpectations + pList->uExpectationCount - 1u;

fail:
	return;
}

/**
 * Pushes a new operation onto a list's operation stack.
 */
static void _list__push_operation(
	SG_context*               pCtx,          //< [in] [out] Error and context info.
	u0066__expectation__list* pList,         //< [in] [out] The list to push an operation onto.
	const char*               szDescription, //< [in] Description of the new expected operation.
	SG_uint32                 uFlags,        //< [in] Flags for the new expected operation.
	const char*               szFile,        //< [in] File for the new expected operation.
	SG_uint32                 uLine          //< [in] Line for the new expected operation.
	)
{
	u0066__expectation__operation* pOperation = NULL;

	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pList);

	VERIFY_COND("No space for additional operations", pList->uStackCount < MAX_OPERATIONS);

	pOperation = pList->aStack + pList->uStackCount;

	pOperation->szDescription = szDescription;
	pOperation->szStep        = NULL;
	pOperation->uFlags        = uFlags;
	pOperation->szFile        = szFile;
	pOperation->uLine         = uLine;
	pOperation->uFinished     = 0u;
	pOperation->uTotal        = 0u;
	pOperation->uPercent      = 0u;
	pOperation->uValueCount   = 0u;
	pOperation->uResult       = SG_ERR_OK;

	pList->uStackCount += 1u;

fail:
	return;
}

static void _list__get_operation(
	SG_context*                     pCtx,       //< [in] [out] Error and context info.
	u0066__expectation__list*       pList,      //< [in] [out] The list to get an operation from.
	u0066__expectation__operation** ppOperation //< [out] The list's current operation.
	)
{
	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pList);
	SG_ARGCHECK(pList->uStackCount > 0u, pList->uStackCount);

	*ppOperation = pList->aStack + pList->uStackCount - 1u;

fail:
	return;
}

/**
 * Pops an operation off a list's operation stack.
 */
static void _list__pop_operation(
	SG_context*               pCtx, //< [in] [out] Error and context info.
	u0066__expectation__list* pList //< [in] [out] The list to pop an operation from.
	)
{
	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pList);

	VERIFY_COND("Popping an empty stack.", pList->uStackCount > 0u);

	if (pList->uStackCount > 0u)
	{
		pList->uStackCount -= 1u;
	}

fail:
	return;
}

/**
 * Adds an expectation to a list.
 * The operation for the new expectation is copied from the list's current operation.
 * Caller should initialize call-specific data in the returned expectation.
 */
static void _list__add_expectation(
	SG_context*                pCtx,  //< [in] [out] Error and context info.
	u0066__expectation__list*  pList, //< [in] [out] The list to add an expectation to.
	u0066__expectation__data** ppData //< [out] The new expectation.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pList);

	VERIFY_COND("No space for additional expectations", pList->uExpectationCount < MAX_EXPECTATIONS);

	pData = pList->aExpectations + pList->uExpectationCount;

	pData->uStackCount = pList->uStackCount;
	pData->bCancel     = SG_FALSE;
	pData->fReaction   = NULL;
	if (pList->uStackCount > 0u)
	{
		pData->cCurrentOperation = pList->aStack[pList->uStackCount - 1u];
	}

	pList->uExpectationCount += 1u;
	if (ppData != NULL)
	{
		*ppData = pData;
	}

fail:
	return;
}

/**
 * Adds a new operation call expectation to a list.
 */
static void _list__add_expectation__operation(
	SG_context*                pCtx,    //< [in] [out] Error and context info.
	u0066__expectation__list*  pList,   //< [in] [out] The list to add an expectation to.
	SG_log__operation_change   eChange, //< [in] The operation change to initialize the new expectation with.
	u0066__expectation__data** ppData   //< [out] The new expectation.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__add_expectation(pCtx, pList, &pData)  );
	pData->eType = EXPECT_OPERATION;
	pData->eOperationChange = eChange;

	if (ppData != NULL)
	{
		*ppData = pData;
	}

fail:
	return;
}

/**
 * Adds a new message call expectation to a list.
 */
static void _list__add_expectation__message(
	SG_context*                pCtx,      //< [in] [out] Error and context info.
	u0066__expectation__list*  pList,     //< [in] [out] The list to add an expectation to.
	SG_log__message_type       eType,     //< [in] The message type to initialize the new expectation with.
	const char*                szMessage, //< [in] The message string to initialize the new expectation with.
	u0066__expectation__data** ppData     //< [out] The new expectation.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__add_expectation(pCtx, pList, &pData)  );
	pData->eType = EXPECT_MESSAGE;
	pData->eMessageType    = eType;
	pData->szMessageString = szMessage;

	if (ppData != NULL)
	{
		*ppData = pData;
	}

fail:
	return;
}

/**
 * Adds expectations to the list consistent with registering a handler.
 */
static void _expect__register_handler(
	SG_context*               pCtx, //< [in] [out] Error and context info.
	u0066__expectation__list* pList //< [in] [out] The list to add expectations to.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__add_expectation(pCtx, pList, &pData)  );
	pData->eType = EXPECT_INIT;

fail:
	return;
}

/**
 * Adds expectations to the list consistent with pushing an operation.
 */
static void _expect__push_operation(
	SG_context*               pCtx,          //< [in] [out] Error and context info.
	u0066__expectation__list* pList,         //< [in] [out] The list to add expectations to.
	const char*               szDescription, //< [in] Description to expect on the operation.
	SG_uint32                 uFlags,        //< [in] Flags to expect on the operation.
	const char*               szFile,        //< [in] Code file to expect on the operation.
	SG_uint32                 uLine          //< [in] Code line to expect on the operation.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	// This is a little tricky, because of how CREATED and PUSHED work.
	// When we receive CREATED, we'll receive the new operation, but it will NOT be on the stack yet.
	// When we receive PUSHED, we'll receive the new operation, and it WILL be on the stack.
	// Since _list__add_expectation always copies the operation on top of the list's stack,
	// we'll push our expected operation onto the list's stack first, so both expectations copy its data.
	// However, the CREATED expectation will include the new operation in the stack size, which we don't want.
	// So we need to decrement its expected stack count after it's been created.
	// Then we can add the PUSHED expectation normally.

	VERIFY_ERR_CHECK(  _list__push_operation(pCtx, pList, szDescription, uFlags, szFile, uLine)  );
	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__CREATED, &pData)  );
	pData->uStackCount -= 1u;
	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__PUSHED, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with describing an operation.
 */
static void _expect__set_operation(
	SG_context*               pCtx,         //< [in] [out] Error and context info.
	u0066__expectation__list* pList,        //< [in] [out] The list to add expectations to.
	const char*               szDescription //< [in] Description to expect.
	)
{
	u0066__expectation__operation* pOperation = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_operation(pCtx, pList, &pOperation)  );
	pOperation->szDescription = szDescription;

	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__DESCRIBED, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with setting a value on an operation.
 */
static void _expect__set_value(
	SG_context*               pCtx, //< [in] [out] Error and context info.
	u0066__expectation__list* pList //< [in] [out] The list to add expectations to.
	// TODO: take the expected name/value/flags and store it
	)
{
	u0066__expectation__operation* pOperation = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_operation(pCtx, pList, &pOperation)  );
	pOperation->uValueCount += 1u;

	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__VALUE_SET, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with setting a total step count on an operation.
 */
static void _expect__set_steps(
	SG_context*               pCtx,  //< [in] [out] Error and context info.
	u0066__expectation__list* pList, //< [in] [out] The list to add expectations to.
	SG_uint32                 uSteps //< [in] The number of steps to expect.
	)
{
	u0066__expectation__operation* pOperation = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_operation(pCtx, pList, &pOperation)  );
	pOperation->uTotal = uSteps;

	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__STEPS_SET, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with describing the current step of an operation.
 */
static void _expect__set_step(
	SG_context*               pCtx,         //< [in] [out] Error and context info.
	u0066__expectation__list* pList,        //< [in] [out] The list to add expectations to.
	const char*               szDescription //< [in] Step description to expect.
	)
{
	u0066__expectation__operation* pOperation = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_operation(pCtx, pList, &pOperation)  );
	pOperation->szStep = szDescription;

	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__STEP_DESCRIBED, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with finishing steps on an operation.
 */
static void _expect__finish_steps(
	SG_context*               pCtx,  //< [in] [out] Error and context info.
	u0066__expectation__list* pList, //< [in] [out] The list to add expectations to.
	SG_uint32                 uSteps //< [in] The number of finished steps to expect.
	)
{
	u0066__expectation__operation* pOperation = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_operation(pCtx, pList, &pOperation)  );
	pOperation->uFinished += uSteps;

	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__STEPS_FINISHED, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with setting the finished steps on an operation.
 */
static void _expect__set_finished(
	SG_context*               pCtx,  //< [in] [out] Error and context info.
	u0066__expectation__list* pList, //< [in] [out] The list to add expectations to.
	SG_uint32                 uSteps //< [in] The number of finished steps to expect.
	)
{
	u0066__expectation__operation* pOperation = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_operation(pCtx, pList, &pOperation)  );
	pOperation->uFinished = uSteps;

	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__STEPS_FINISHED, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with popping an operation.
 */
static void _expect__pop_operation(
	SG_context*               pCtx, //< [in] [out] Error and context info.
	u0066__expectation__list* pList //< [in] [out] The list to add expectations to.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	// This is a little tricky, because of how COMPLETED and POPPED work.
	// When we receive COMPLETED, we'll receive the finished operation, and it WILL still be on the stack.
	// When we receive POPPED, we'll still receive the finished operation, and it will NOT be on the stack any longer.
	// Since _list__add_expectation always copies the operation on top of the list's stack,
	// we'll pop our expected operation from the list's stack last, so both expectations copy its data.
	// The COMPLETED expectation can be added normally.
	// However, the POPPED expectation will still include the operation in the stack size, which we don't want.
	// So we need to decrement its expected stack count after it's been created.

	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__COMPLETED, NULL)  );
	VERIFY_ERR_CHECK(  _list__add_expectation__operation(pCtx, pList, SG_LOG__OPERATION__POPPED, &pData)  );
	pData->uStackCount -= 1u;
	VERIFY_ERR_CHECK(  _list__pop_operation(pCtx, pList)  );

fail:
	return;
}

/**
 * Modifies the most recent expectation in the list to call a reaction function.
 */
static void _expect__reaction(
	SG_context*               pCtx,     //< [in] [out] Error and context info.
	u0066__expectation__list* pList,    //< [in] [out] The list to add expectations to.
	u0066__reaction*          fReaction //< [in] The function to call in response to the expectation being met.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_expectation(pCtx, pList, &pData)  );
	pData->fReaction = fReaction;

fail:
	return;
}

/**
 * Modifies the most recent expectation in the list to return a cancel value.
 */
static void _expect__cancel(
	SG_context*               pCtx, //< [in] [out] Error and context info.
	u0066__expectation__list* pList //< [in] [out] The list to add expectations to.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__get_expectation(pCtx, pList, &pData)  );
	pData->bCancel = SG_TRUE;

fail:
	return;
}

/**
 * Adds expectations to the list consistent with sending a message.
 */
static void _expect__report_message(
	SG_context*               pCtx,     //< [in] [out] Error and context info.
	u0066__expectation__list* pList,    //< [in] [out] The list to add expectations to.
	SG_log__message_type      eType,    //< [in] The type of message to expect.
	const char*               szMessage //< [in] The text of the message to expect.
	)
{
	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__add_expectation__message(pCtx, pList, eType, szMessage, NULL)  );

fail:
	return;
}

/**
 * Adds expectations to the list consistent with unregistering a handler.
 */
static void _expect__unregister_handler(
	SG_context*               pCtx, //< [in] [out] Error and context info.
	u0066__expectation__list* pList //< [in] [out] The list to add expectations to.
	)
{
	u0066__expectation__data* pData = NULL;

	SG_NULLARGCHECK(pList);

	VERIFY_ERR_CHECK(  _list__add_expectation(pCtx, pList, &pData)  );
	pData->eType = EXPECT_DESTROY;

fail:
	return;
}

/**
 * A u0066__reaction function that logs an operation.
 *
 * This is meant to perform a typical operation using all
 * the capabilities of the log reporting interface.
 *
 * Our goal is to use this as a reaction function from within
 * our test handler, which simulates a log handler itself
 * calling the log reporting API to report operations,
 * progress, and messages.  The log system should be able to
 * ensure that this doesn't result in infinite recursion by
 * not notifying a handler of events that occur while already
 * within one of its functions.
 *
 * This reaction function should be able to be specified
 * with any other expectation  and not result in our test
 * handler actually receiving any additional notifications.
 */
static void _reaction__log_operation(
	SG_context* pCtx
	)
{
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Reaction operation", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Value", "Whatever", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 4u, NULL)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1: Verbose")  );
	SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Verbose message")  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Step 2: Info")  );
	SG_ERR_CHECK(  SG_log__report_info(pCtx, "Info message")  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Step 3: Warning")  );
	SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Warning message")  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Step 4: Error")  );
	SG_ERR_CHECK(  SG_log__report_error(pCtx, "Error message")  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );

fail:
	return;
}

/**
 * Verifies that an expectation list is in a valid state for an expected call.
 * Increments the index of the next expectation.
 * Returns data about the current expectation for verification by caller.
 */
static void _verify__list(
	SG_context*                pCtx,  //< [in] [out] Error and context info.
	u0066__expectation__list*  pList, //< [in] The list to verify.
	u0066__expectation__data** ppData //< [out] The expectation that should match the current call.
	)
{
	SG_process_id             cProcess;
	SG_thread_id              cThread;
	SG_bool                   bThreadsEqual;

	SG_NULLARGCHECK(pList);

	VERIFY_COND_FAIL("Unexpected call", pList->uNextExpectation < pList->uExpectationCount);

	VERIFY_ERR_CHECK(  SG_log__stack__get_threading(pCtx, &cProcess, &cThread)  );
	VERIFYP_COND("Process ID doesn't match expectation", cProcess == pList->cProcess, ("TestIndex(%d)", pList->uTestIndex));
	VERIFY_ERR_CHECK(  SG_thread__threads_equal(pCtx, cThread, pList->cThread, &bThreadsEqual)  );
	VERIFYP_COND("Thread ID doesn't match expectation", bThreadsEqual == SG_TRUE, ("TestIndex(%d)", pList->uTestIndex));

	*ppData = pList->aExpectations + pList->uNextExpectation;
	pList->uNextExpectation += 1u;

fail:
	return;
}

/**
 * Verifies that the current state of the operation stack matches an expectation.
 */
static void _verify__stack(
	SG_context*                     pCtx,  //< [in] [out] Error and context info.
	const u0066__expectation__list* pList, //< [in] The list currently being verified.
	const u0066__expectation__data* pData  //< [in] Data containing expected state of the operation stack.
	)
{
	SG_uint32 uStackCount;

	SG_NULLARGCHECK(pData);

	VERIFY_ERR_CHECK(  SG_log__stack__get_count(pCtx, &uStackCount)  );
	VERIFYP_COND("Stack is the wrong size", uStackCount == pData->uStackCount, ("TestIndex(%d) Size(%d) Expected(%d)", pList->uTestIndex, uStackCount, pData->uStackCount));

fail:
	return;
}

/**
 * Verifies that an operation matches an expectation.
 */
static void _verify__operation(
	SG_context*                          pCtx,     //< [in] [out] Error and context info.
	const u0066__expectation__list*      pList,    //< [in] The list currently being verified.
	const SG_log__operation*             pActual,  //< [in] The operation to verify.
	const u0066__expectation__operation* pExpected //< [in] The expected data for the operation.
	)
{
	const char* szDescription;
	const char* szStep;
	SG_uint32   uFlags;
	SG_int64    iStart;
	SG_int64    iLatest;
	SG_int64    iElapsed;
	SG_int64    iStackLatest;
	const char* szFile;
	SG_uint32   uLine;
	SG_uint32   uFinished;
	SG_uint32   uTotal;
	SG_uint32   uPercent;
	SG_uint32   uValueCount;
	SG_error    uResult;
	SG_int_to_string_buffer sz1;
	SG_int_to_string_buffer sz2;
	SG_int_to_string_buffer sz3;
	SG_int_to_string_buffer sz4;

	VERIFYP_COND("Cannot verify NULL operations", pActual != NULL && pExpected != NULL, ("TestIndex(%d)", pList->uTestIndex));

	VERIFY_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pActual, &szDescription, &szStep, &uFlags)  );
	VERIFYP_COND("Operation description doesn't match expectation", SG_strcmp__null(szDescription, pExpected->szDescription) == 0, ("TestIndex(%d) Description(%s) Expected(%s)", pList->uTestIndex, szDescription, pExpected->szDescription));
	VERIFYP_COND("Step description doesn't match expectation", SG_strcmp__null(szStep, pExpected->szStep) == 0, ("TestIndex(%d) Step(%s) Expected(%s)", pList->uTestIndex, szStep, pExpected->szStep));
	VERIFYP_COND("Flags don't match expectation", uFlags == pExpected->uFlags, ("TestIndex(%d) Step(%d) Expected(%d)", pList->uTestIndex, uFlags, pExpected->uFlags));

	VERIFY_ERR_CHECK(  SG_log__operation__get_time(pCtx, pActual, &iStart, &iLatest, &iElapsed)  );
	VERIFY_ERR_CHECK(  SG_log__stack__get_time(pCtx, &iStackLatest)  );
	VERIFYP_COND("Start time is zero", iStart != 0, ("TestIndex(%d)", pList->uTestIndex));
	VERIFYP_COND("Latest time is zero", iLatest != 0, ("TestIndex(%d)", pList->uTestIndex));
	VERIFYP_COND("Latest time is before start time", iLatest >= iStart, ("TestIndex(%d) Start(%s) Latest(%s)", pList->uTestIndex, SG_int64_to_sz(iStart, sz1), SG_int64_to_sz(iLatest, sz2)));
	VERIFYP_COND("Elapsed time doesn't match expectation", iElapsed == (iLatest - iStart), ("TestIndex(%d) Elapsed(%s) Start(%s) Latest(%s) Expected(%s)", pList->uTestIndex, SG_int64_to_sz(iElapsed, sz1), SG_int64_to_sz(iStart, sz2), SG_int64_to_sz(iLatest, sz3), SG_int64_to_sz(iLatest-iStart, sz4)));
	VERIFYP_COND("Operation and stack have different latest times", iLatest == iStackLatest, ("TestIndex(%d) Operation(%s) Stack(%s)", pList->uTestIndex, SG_int64_to_sz(iLatest, sz1), SG_int64_to_sz(iStackLatest, sz2)));

	VERIFY_ERR_CHECK(  SG_log__operation__get_source(pCtx, pActual, &szFile, &uLine));
	VERIFYP_COND("Code file doesn't match expectation", SG_strcmp__null(szFile, pExpected->szFile) == 0, ("TestIndex(%d) File(%s) Expected(%s)", pList->uTestIndex, szFile, pExpected->szFile));
	if (szFile != NULL)
	{
		// get_source doesn't give us a line number if it gave us a NULL file
		// so only check the line number if we got a valid file
		VERIFYP_COND("Code line doesn't match expectation", uLine == pExpected->uLine, ("TestIndex(%d) Line(%d) Expected(%d)", pList->uTestIndex, uLine, pExpected->uLine));
	}

	VERIFY_ERR_CHECK(  SG_log__operation__get_progress(pCtx, pActual, NULL, &uFinished, &uTotal, &uPercent, 0u)  );
	VERIFYP_COND("Finished step count doesn't match expectation", uFinished == pExpected->uFinished, ("TestIndex(%d) Finished(%d) Expected(%d)", pList->uTestIndex, uFinished, pExpected->uFinished));
	VERIFYP_COND("Total step count doesn't match expectation", uTotal == pExpected->uTotal, ("TestIndex(%d) Total(%d) Expected(%d)", pList->uTestIndex, uTotal, pExpected->uTotal));
	VERIFYP_COND("Completion percentage doesn't match expectation", uPercent == pExpected->uPercent, ("TestIndex(%d) Percent(%d) Expected(%d)", pList->uTestIndex, uPercent, pExpected->uPercent));

	VERIFY_ERR_CHECK(  SG_log__operation__get_value_count(pCtx, pActual, &uValueCount)  );
	VERIFYP_COND("Value count doesn't match expectation", uValueCount == pExpected->uValueCount, ("TestIndex(%d) Count(%d) Expected(%d)", pList->uTestIndex, uValueCount, pExpected->uValueCount));
	// TODO: add checking of actual names/values/flags

	VERIFY_ERR_CHECK(  SG_log__operation__get_result(pCtx, pActual, &uResult)  );
	VERIFYP_COND("Result doesn't match expectation", uResult == pExpected->uResult, ("TestIndex(%d) Result(%s) Expected(%s)", pList->uTestIndex, SG_uint64_to_sz(uResult, sz1), SG_uint64_to_sz(pExpected->uResult, sz2)));

fail:
	return;
}

/**
 * Verifies whether or not cancellation is currently being requested in the log operation stack.
 */
static void _verify__cancel(
	SG_context*                     pCtx,   //< [in] [out] Error and context info.
	const u0066__expectation__list* pList,  //< [in] The list currently being verified.
	SG_bool                         bCancel //< [in] The cancel value to check for.
	)
{
	SG_bool bActual;

	VERIFY_ERR_CHECK(  SG_log__cancel_requested(pCtx, &bActual)  );
	if (bCancel == SG_FALSE)
	{
		VERIFYP_COND("Unexpected cancel request", bActual == SG_FALSE, ("TestIndex(%d)", pList->uTestIndex));
	}
	else
	{
		VERIFYP_COND("Expected cancel request", bActual == SG_TRUE, ("TestIndex(%d)", pList->uTestIndex));
	}

fail:
	return;
}

/**
 * Init handler.
 * Verifies that the call matches the next expectation in the list.
 */
static void _handler__init(
	SG_context* pCtx, //< [in] [out] Error and context info.
	void*       pThis //< [in] u0066__expectation__list to verify against.
	)
{
	u0066__expectation__list* pList  = (u0066__expectation__list*)pThis;
	u0066__expectation__data* pData  = NULL;

	VERIFY_ERR_CHECK(  _verify__list(pCtx, pList, &pData)  );
	VERIFY_ERR_CHECK(  _verify__stack(pCtx, pList, pData)  );

	VERIFYP_COND("Call type doesn't match expectation", pData->eType == EXPECT_INIT, ("TestIndex(%d) Type(%d) Expected(%d)", pList->uTestIndex, EXPECT_INIT, pData->eType));

	if (pData->fReaction != NULL)
	{
		pData->fReaction(pCtx);
	}

fail:
	return;
}

/**
 * Operation handler.
 * Verifies that the call matches the next expectation in the list.
 */
static void _handler__operation(
	SG_context*              pCtx,       //< [in] [out] Error and context info.
	void*                    pThis,      //< [in] u0066__expectation__list to verify against.
	const SG_log__operation* pOperation, //< [in] The operation that was changed.
	SG_log__operation_change eChange,    //< [in] The change that occurred to the operation.
	SG_bool*                 pCancel     //< [in] Whether or not we want to cancel the stack.
	)
{
	u0066__expectation__list* pList  = (u0066__expectation__list*)pThis;
	u0066__expectation__data* pData  = NULL;

	VERIFY_ERR_CHECK(  _verify__list(pCtx, pList, &pData)  );
	VERIFY_ERR_CHECK(  _verify__stack(pCtx, pList, pData)  );

	VERIFYP_COND("Call type doesn't match expectation", pData->eType == EXPECT_OPERATION, ("TestIndex(%d) Type(%d) Expected(%d)", pList->uTestIndex, EXPECT_OPERATION, pData->eType));
	VERIFYP_COND("Operation change type doesn't match expectation", pData->eOperationChange == eChange, ("TestIndex(%d) Change(%d) Expected(%d)", pList->uTestIndex, eChange, pData->eOperationChange));
	VERIFYP_COND("NULL operation doesn't match expectation", pOperation != NULL, ("TestIndex(%d)", pList->uTestIndex));
	if (pOperation != NULL)
	{
		VERIFY_ERR_CHECK(  _verify__operation(pCtx, pList, pOperation, &pData->cCurrentOperation)  );
	}

	if (pData->fReaction != NULL)
	{
		pData->fReaction(pCtx);
	}

	*pCancel = pData->bCancel;

fail:
	return;
}

/**
 * Message handler.
 * Verifies that the call matches the next expectation in the list.
 */
static void _handler__message(
	SG_context*              pCtx,       //< [in] [out] Error and context info.
	void*                    pThis,      //< [in] u0066__expectation__list to verify against.
	const SG_log__operation* pOperation, //< [in] The current operation on the stack.
	SG_log__message_type     eType,      //< [in] The type of message received.
	const SG_string*         pMessage,   //< [in] The actual message string received.
	SG_bool*                 pCancel     //< [in] Whether or not we want to cancel the stack.
	)
{
	u0066__expectation__list* pList  = (u0066__expectation__list*)pThis;
	u0066__expectation__data* pData  = NULL;

	VERIFY_ERR_CHECK(  _verify__list(pCtx, pList, &pData)  );
	VERIFY_ERR_CHECK(  _verify__stack(pCtx, pList, pData)  );

	VERIFYP_COND("Call type doesn't match expectation", pData->eType == EXPECT_MESSAGE, ("TestIndex(%d) Type(%d) Expected(%d)", pList->uTestIndex, EXPECT_MESSAGE, pData->eType));
	VERIFYP_COND("Message type doesn't match expectation", pData->eMessageType == eType, ("TestIndex(%d) Type(%d) Expected(%d)", pList->uTestIndex, eType, pData->eMessageType));
	//This next bit will segfault if we get an operation instead of a message.
	if (pData->eType == EXPECT_MESSAGE)
		VERIFYP_COND("Message string doesn't match expectation", strcmp(pData->szMessageString, SG_string__sz(pMessage)) == 0, ("TestIndex(%d) Message(%s) Expected(%s)", pList->uTestIndex, SG_string__sz(pMessage), pData->szMessageString));

	if (pOperation == NULL)
	{
		VERIFYP_COND("NULL operation doesn't match expected non-empty stack", pData->uStackCount == 0u, ("TestIndex(%d)", pList->uTestIndex));
	}
	else
	{
		VERIFYP_COND("Non-NULL operation doesn't match expected empty stack", pData->uStackCount > 0u, ("TestIndex(%d)", pList->uTestIndex));
		if (pData->uStackCount > 0u /*&& pData->cCurrentOperation.szDescription != NULL */)
		{
			VERIFY_ERR_CHECK(  _verify__operation(pCtx, pList, pOperation, &pData->cCurrentOperation)  );
		}
	}

	if (pData->fReaction != NULL)
	{
		pData->fReaction(pCtx);
	}

	*pCancel = pData->bCancel;

fail:
	return;
}

/**
 * Destroy handler.
 * Verifies that the call matches the next expectation in the list.
 */
static void _handler__destroy(
	SG_context* pCtx, //< [in] [out] Error and context info.
	void*       pThis //< [in] u0066__expectation__list to verify against.
	)
{
	u0066__expectation__list* pList  = (u0066__expectation__list*)pThis;
	u0066__expectation__data* pData  = NULL;

	VERIFY_ERR_CHECK(  _verify__list(pCtx, pList, &pData)  );
	VERIFY_ERR_CHECK(  _verify__stack(pCtx, pList, pData)  );

	VERIFYP_COND("Call type doesn't match expectation", pData->eType == EXPECT_DESTROY, ("TestIndex(%d) Type(%d) Expected(%d)", pList->uTestIndex, EXPECT_DESTROY, pData->eType));

	if (pData->fReaction != NULL)
	{
		pData->fReaction(pCtx);
	}

fail:
	return;
}


/*
**
** Tests
**
*/

/*
**
** register_handler
**
*/

void u0066__register_handler(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

	// send a message that we're not expecting
	// if we're actually no longer registered, we won't receive it
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "Unexpected")  );

	// unregistering while we're not registered should silently have no effect
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

void u0066__register_handler__badargs(SG_context* pCtx)
{
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_log__register_handler(pCtx, NULL, NULL, NULL, SG_LOG__FLAG__NONE), SG_ERR_INVALIDARG  );
}

void u0066__register_handler__multiple(SG_context* pCtx)
{
	u0066__expectation__list cList;

	// setup to only be registered once
	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	// register the handler for real
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );

	// these calls should not register the handler again
	// therefore it should not receive additional init calls
	// and we're only expecting one init call
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );

	// unregister the handler for real
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

	// send a message that we're not expecting
	// if we're actually no longer registered, we won't receive it
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "Unexpected")  );

	// setup to actually be registered multiple times
	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Test")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	// register multiple times
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__MULTIPLE | SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__MULTIPLE | SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__MULTIPLE | SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__MULTIPLE | SG_LOG__FLAG__HANDLER_TYPE__ALL)  );

	// unregister all but one of the times
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

	// send the message we're expecting
	// we should only receive it once (as expected), because we should only be registered once at this point
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "Test")  );

	// unregister the last time
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

	// send a message that we're not expecting
	// if we're actually no longer registered, we won't receive it
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "Unexpected")  );

	// unregistering while we're not registered should silently have no effect
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

void u0066__register_handler__recursive(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


/*
**
** unregister_handler
**
*/

// other cases covered by register_handler cases

void u0066__unregister_handler__badargs(SG_context* pCtx)
{
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_log__unregister_handler(pCtx, NULL, NULL), SG_ERR_INVALIDARG  );
}

void u0066__unregister_handler__recursive(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


/*
**
** unregister_all_handlers
**
*/

void u0066__unregister_all_handlers(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__MULTIPLE | SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__MULTIPLE | SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__MULTIPLE | SG_LOG__FLAG__HANDLER_TYPE__ALL)  );

	// this should unregister all three of our registrations
	// which should result in us receiving three unregister calls
	VERIFY_ERR_CHECK(  SG_log__unregister_all_handlers(pCtx)  );

fail:
	return;
}


/*
**
** push_operation
**
*/

typedef struct
{
	SG_bool     bExpect;
	const char* szDescription;
	SG_uint32   uFlags;
	const char* szFile;
	SG_uint32   uLine;
}
u0066__push_operation__case;

static const u0066__push_operation__case u0066__push_operation__cases[] =
{
	{ SG_TRUE,  "Operation 1", SG_LOG__FLAG__NONE,    "File 1", 123u },
	{ SG_TRUE,  "Operation 1", SG_LOG__FLAG__VERBOSE, "File 1", 123u },
	{ SG_TRUE,  "Operation 1", SG_LOG__FLAG__NONE,    NULL,       0u },
	{ SG_TRUE,  "Operation 1", SG_LOG__FLAG__VERBOSE, NULL,       0u },
	{ SG_TRUE,  NULL,          SG_LOG__FLAG__NONE,    "File 1", 123u },
	{ SG_TRUE,  NULL,          SG_LOG__FLAG__VERBOSE, "File 1", 123u },
	{ SG_TRUE,  NULL,          SG_LOG__FLAG__NONE,    NULL,       0u },
	{ SG_TRUE,  NULL,          SG_LOG__FLAG__VERBOSE, NULL,       0u },
};

void u0066__push_operation__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__push_operation__cases); ++uIndex)
	{
		const u0066__push_operation__case* pCase = u0066__push_operation__cases + uIndex;

		VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, uIndex)  );
		VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
		if (pCase->bExpect == SG_TRUE)
		{
			VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, pCase->szDescription, pCase->uFlags, pCase->szFile, pCase->uLine)  );
			VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
			VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
		}
		VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

		VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
		VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, pCase->szDescription, pCase->uFlags, pCase->szFile, pCase->uLine)  );
		VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}


/*
**
** set_operation
**
*/

typedef struct
{
	SG_bool     bExpect;
	const char* szDescription;
}
u0066__set_operation__case;

static u0066__set_operation__case u0066__set_operation__cases[] =
{
	{ SG_TRUE,  "Operation 1 Changed"},
	{ SG_TRUE,  NULL},
	{ SG_FALSE, "Operation 1"}, // Note: This is unexpected because it matches the operation's initial description (hard-coded in the test)
};

void u0066__set_operation__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__set_operation__cases); ++uIndex)
	{
		const u0066__set_operation__case* pCase = u0066__set_operation__cases + uIndex;

		VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, uIndex)  );
		VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		if (pCase->bExpect == SG_TRUE)
		{
			VERIFY_ERR_CHECK(  _expect__set_operation(pCtx, &cList, pCase->szDescription)  );
			VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
		}
		VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

		VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
		VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, pCase->szDescription)  );
		VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}

void u0066__set_operation__repeat(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__set_operation(pCtx, &cList, "Operation 1 Changed")  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, "Operation 1 Changed")  );
	VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, "Operation 1 Changed")  );
	VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, "Operation 1 Changed")  );
	VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, "Operation 1 Changed")  );
	VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, "Operation 1 Changed")  );
	VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, "Operation 1 Changed")  );
	VERIFY_ERR_CHECK(  SG_log__set_operation(pCtx, "Operation 1 Changed")  );
	// we should only be notified for the first call, because after that the value doesn't actually change
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


/*
**
** set_value
**
*/

// TODO: add cases for various value types, once the values are actually verified

typedef struct
{
	SG_bool   bExpect;
	SG_int32  iValue;
	SG_uint32 uFlags;
}
u0066__set_value__case;

static const u0066__set_value__case u0066__set_value__cases[] =
{
	{ SG_TRUE, 1234, SG_LOG__FLAG__NONE },
	{ SG_TRUE,    0, SG_LOG__FLAG__NONE },
	{ SG_TRUE, 1234, SG_LOG__FLAG__VERBOSE },
	{ SG_TRUE,    0, SG_LOG__FLAG__VERBOSE },
	{ SG_TRUE, 1234, SG_LOG__FLAG__INPUT },
	{ SG_TRUE,    0, SG_LOG__FLAG__INPUT },
	{ SG_TRUE, 1234, SG_LOG__FLAG__INTERMEDIATE },
	{ SG_TRUE,    0, SG_LOG__FLAG__INTERMEDIATE },
	{ SG_TRUE, 1234, SG_LOG__FLAG__OUTPUT },
	{ SG_TRUE,    0, SG_LOG__FLAG__OUTPUT },
};

void u0066__set_value__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__set_value__cases); ++uIndex)
	{
		const u0066__set_value__case* pCase = u0066__set_value__cases + uIndex;

		VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, uIndex)  );
		VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		if (pCase->bExpect == SG_TRUE)
		{
			VERIFY_ERR_CHECK(  _expect__set_value(pCtx, &cList)  );
			VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
		}
		VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

		VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
		VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		VERIFY_ERR_CHECK(  SG_log__set_value__int(pCtx, "Value 1", pCase->iValue, pCase->uFlags)  );
		VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}


/*
**
** set_steps
**
*/

typedef struct
{
	SG_bool   bExpect;
	SG_uint32 uSteps;
}
u0066__set_steps__case;

static u0066__set_steps__case u0066__set_steps__cases[] =
{
	{SG_TRUE,  1234u},
	{SG_FALSE,    0u},
};

void u0066__set_steps__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__set_steps__cases); ++uIndex)
	{
		const u0066__set_steps__case* pCase = u0066__set_steps__cases + uIndex;

		VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, uIndex)  );
		VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		if (pCase->bExpect == SG_TRUE)
		{
			VERIFY_ERR_CHECK(  _expect__set_steps(pCtx, &cList, pCase->uSteps)  );
			VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
		}
		VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

		VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
		VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, pCase->uSteps, NULL)  );
		VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}

void u0066__set_steps__repeat(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__set_steps(pCtx, &cList, 1234u)  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, 1234u, NULL)  );
	VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, 1234u, NULL)  );
	VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, 1234u, NULL)  );
	VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, 1234u, NULL)  );
	VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, 1234u, NULL)  );
	VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, 1234u, NULL)  );
	VERIFY_ERR_CHECK(  SG_log__set_steps(pCtx, 1234u, NULL)  );
	// we should only be notified for the first call, because after that the value doesn't actually change
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

//In this test, we report more messages than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_messages__errors_only(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLE_MESSAGE__ERROR)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


//In this test, we report more messages than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_messages__warnings_only(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLE_MESSAGE__WARNING)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


//In this test, we report more messages than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_messages__info_only(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLE_MESSAGE__INFO)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

//In this test, we report more messages than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_messages__verbose_only(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLE_MESSAGE__VERBOSE)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

//In this test, we report more messages than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_messages__quiet(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__QUIET)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

//In this test, we report more messages than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_messages__normal(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__NORMAL)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


//In this test, we report more messages than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_messages__all(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

//In this test, we report more operations than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_operations__no_verbose(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	//These messages will get reported, even though they are in a verbose operation.
	//Increase the expected stack count.
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	//VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", -1, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	//VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	//VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__NORMAL)  );
	//This operation should not make it through the logger.
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__VERBOSE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );

	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );

	

	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

//In this test, we report more operations than we expect, knowing that the logger will filter out the undesired
//ones.
void u0066__filter_operations__all(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__VERBOSE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	//This operation should not make it through the logger.
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__VERBOSE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );

	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

/*
**
** set_step
**
*/

typedef struct
{
	SG_bool     bExpect;
	const char* szDescription;
}
u0066__set_step__case;

static const u0066__set_step__case u0066__set_step__cases[] =
{
	{ SG_TRUE,  "Step 1" },
	{ SG_FALSE, NULL},
};

void u0066__set_step__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__set_step__cases); ++uIndex)
	{
		const u0066__set_step__case* pCase = u0066__set_step__cases + uIndex;

		VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, uIndex)  );
		VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		if (pCase->bExpect == SG_TRUE)
		{
			VERIFY_ERR_CHECK(  _expect__set_step(pCtx, &cList, pCase->szDescription)  );
			VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
		}
		VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

		VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
		VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, pCase->szDescription)  );
		VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}

void u0066__set_step__repeat(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__set_step(pCtx, &cList, "Step 1")  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1")  );
	VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1")  );
	VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1")  );
	VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1")  );
	VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1")  );
	VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1")  );
	VERIFY_ERR_CHECK(  SG_log__set_step(pCtx, "Step 1")  );
	// we should only be notified for the first call, because after that the value doesn't actually change
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


/*
**
** finish_steps
**
*/

typedef struct
{
	SG_bool   bExpect;
	SG_uint32 uSteps;
}
u0066__finish_steps__case;

static u0066__finish_steps__case u0066__finish_steps__cases[] =
{
	{ SG_TRUE,  1234u},
	{ SG_FALSE,    0u},
};

void u0066__finish_steps__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__finish_steps__cases); ++uIndex)
	{
		const u0066__finish_steps__case* pCase = u0066__finish_steps__cases + uIndex;

		VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, uIndex)  );
		VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		if (pCase->bExpect == SG_TRUE)
		{
			VERIFY_ERR_CHECK(  _expect__finish_steps(pCtx, &cList, pCase->uSteps)  );
			VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
		}
		VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

		VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
		VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, pCase->uSteps)  );
		VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}

void u0066__finish_steps__repeat(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, 0u)  );
	VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, 0u)  );
	VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, 0u)  );
	VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, 0u)  );
	VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, 0u)  );
	VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, 0u)  );
	VERIFY_ERR_CHECK(  SG_log__finish_steps(pCtx, 0u)  );
	// we should never be notified, because we're never actually causing the number of finished steps to change
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


/*
**
** set_finished
**
*/

typedef struct
{
	SG_bool   bExpect;
	SG_uint32 uSteps;
}
u0066__set_finished__case;

static u0066__set_finished__case u0066__set_finished__cases[] =
{
	{ SG_TRUE,  1234u},
	{ SG_FALSE,    0u},
};

void u0066__set_finished__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__set_finished__cases); ++uIndex)
	{
		const u0066__set_finished__case* pCase = u0066__set_finished__cases + uIndex;

		VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, uIndex)  );
		VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		if (pCase->bExpect == SG_TRUE)
		{
			VERIFY_ERR_CHECK(  _expect__set_finished(pCtx, &cList, pCase->uSteps)  );
			VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
		}
		VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
		VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

		VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
		VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
		VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, pCase->uSteps)  );
		VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}

void u0066__set_finished__repeat(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__set_finished(pCtx, &cList, 1234u)  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	// we should only be notified for the first call, because after that the value doesn't actually change
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


/*
**
** pop_operation
**
*/

// covered by push_operation cases


/*
**
** cancel_requested
**
*/

void u0066__cancel_requested__false(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__set_finished(pCtx, &cList, 1234u)  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_FALSE)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_FALSE)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_FALSE)  );
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_FALSE)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

void u0066__cancel_requested__true(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__set_finished(pCtx, &cList, 1234u)  );
	VERIFY_ERR_CHECK(  _expect__cancel(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_FALSE)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_FALSE)  );
	VERIFY_ERR_CHECK(  SG_log__set_finished(pCtx, 1234u)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_TRUE)  );
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  _verify__cancel(pCtx, &cList, SG_FALSE)  ); // should be reset after last operation is popped
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}


/*
**
** report_message
**
*/

void u0066__report_message(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, &cList, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__reaction(pCtx, &cList, _reaction__log_operation)  ); // see _reaction__log_operation for explanation
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, "Operation 1", SG_LOG__FLAG__NONE, "File 1", 123u)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

void u0066__report_message__badargs(SG_context* pCtx)
{
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_log__report_verbose(pCtx, NULL), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_log__report_info(pCtx, NULL), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_log__report_warning(pCtx, NULL), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_log__report_error(pCtx, NULL), SG_ERR_INVALIDARG  );
}

void u0066__report_message__no_stack(SG_context* pCtx)
{
	u0066__expectation__list cList;

	VERIFY_ERR_CHECK(  _list__init(pCtx, &cList, UNKNOWN_TEST_INDEX)  );
	VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__VERBOSE, "Verbose 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__INFO, "Info 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__WARNING, "Warning 1")  );
	VERIFY_ERR_CHECK(  _expect__report_message(pCtx, &cList, SG_LOG__MESSAGE__ERROR, "Error 1")  );
	VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s %d", "Verbose", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_info(pCtx, "%s %d", "Info", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_warning(pCtx, "%s %d", "Warning", 1)  );
	VERIFY_ERR_CHECK(  SG_log__report_error(pCtx, "%s %d", "Error", 1)  );
	VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );

fail:
	return;
}

/**
 * A message handler that simply keeps a copy of the most recent message in its data.
 * The handler doesn't manage its own data memory, it must be given an allocated string.
 */
void _handler__single_message_recorder__message(
	SG_context*              pCtx,
	void*                    pThis,
	const SG_log__operation* pOperation,
	SG_log__message_type     eType,
	const SG_string*         pMessage,
	SG_bool*                 pCancel
	)
{
	SG_string* pData = (SG_string*)pThis;

	SG_UNUSED(pOperation);
	SG_UNUSED(eType);
	SG_UNUSED(pCancel);

	SG_ERR_CHECK(  SG_string__set__string(pCtx, pData, pMessage)  );

fail:
	return;
}

/**
 * This test ensures that a handler using SG_LOG__FLAG__DETAILED_MESSAGES receives
 * a different error message than one that doesn't.
 */
void u0066__report_message__current_error__detailed(SG_context* pCtx)
{
	SG_log__handler cHandler         = { NULL, NULL, _handler__single_message_recorder__message, NULL, NULL };
	SG_string*      pBriefMessage    = NULL;
	SG_string*      pDetailedMessage = NULL;

	SG_STRING__ALLOC(pCtx, &pBriefMessage);
	SG_STRING__ALLOC(pCtx, &pDetailedMessage);

	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, &cHandler, (void*)pBriefMessage,    NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
	VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, &cHandler, (void*)pDetailedMessage, NULL, SG_LOG__FLAG__HANDLER_TYPE__ALL | SG_LOG__FLAG__DETAILED_MESSAGES)  );

	SG_context__err(pCtx, SG_ERR_INVALIDARG, __FILE__, __LINE__, "This is a test error.");
	SG_log__report_error__current_error(pCtx);
	SG_context__err_reset(pCtx);

	VERIFYP_COND("No brief message received.", SG_string__length_in_bytes(pBriefMessage) > 0u, ("BriefMessage(%s)", SG_string__sz(pBriefMessage)));
	VERIFYP_COND("No detailed message received.", SG_string__length_in_bytes(pDetailedMessage) > 0u, ("DetailedMessage(%s)", SG_string__sz(pDetailedMessage)));
	VERIFYP_COND("Brief and detailed messages are equal, but shouldn't be.", strcmp(SG_string__sz(pBriefMessage), SG_string__sz(pDetailedMessage)) != 0, ("BriefMessage(%s) DetailedMessage(%s)", SG_string__sz(pBriefMessage), SG_string__sz(pDetailedMessage)));

	VERIFY_ERR_CHECK(  SG_log__unregister_all_handlers(pCtx)  );

fail:
	SG_STRING_NULLFREE(pCtx, pBriefMessage);
	SG_STRING_NULLFREE(pCtx, pDetailedMessage);
};

// TODO: add a case for reporting a NULL message


/*
**
** A test that runs arbitrary sequences of progress reporting function calls.
**
*/

/*
 The idea with this block of code was to create a data-driven way to specify
 the sequence of reporting functions to call.  That way you could just specify
 a list of function sequences and turn this test loose to run each sequence
 using every possible combination of arguments (also an arbitrary list per function).
 This could replace many of the tests written so far with simple data
 declarations in a list of function sequences.  Unfortunately the code to actually
 run the functions turned out to be more effort than I have time for.
 However, I'm going to leave what I had so far here and commented out in case
 it's helpful to take it up again later.

typedef void u0066__report__function__expect(SG_context* pCtx, u0066__expectation__list* pList, SG_uint32 uArgset);
typedef void u0066__report__function__invoke(SG_context* pCtx, SG_uint32 uArgset);

typedef struct
{
	SG_uint32                        uArgsets;
	u0066__report__function__expect* fExpect;
	u0066__report__function__invoke* fInvoke;
}
u0066__report__function;

// ----- push_operation -----

typedef struct
{
	const char* szDescription;
	SG_uint32   uFlags;
	const char* szFile;
	SG_uint32   uLine;
}
u0066__report__function__push_operation__argset;

static const u0066__report__function__push_operation__argset u0066__report__function__push_operation__argsets[] =
{
	{ "Operation 1", SG_LOG__FLAG__NONE,    "File 1", 123u },
	{ "Operation 1", SG_LOG__FLAG__VERBOSE, "File 1", 123u },
	{ "Operation 1", SG_LOG__FLAG__NONE,    NULL,       0u },
	{ "Operation 1", SG_LOG__FLAG__VERBOSE, NULL,       0u },
	{ NULL,          SG_LOG__FLAG__NONE,    "File 1", 123u },
	{ NULL,          SG_LOG__FLAG__VERBOSE, "File 1", 123u },
	{ NULL,          SG_LOG__FLAG__NONE,    NULL,       0u },
	{ NULL,          SG_LOG__FLAG__VERBOSE, NULL,       0u },
};

void u0066__report__function__push_operation__expect(SG_context* pCtx, u0066__expectation__list* pList, SG_uint32 uArgset)
{
	const u0066__report__function__push_operation__argset* pArgset = NULL;

	SG_ASSERT(uArgset < SG_NrElements(u0066__report__function__push_operation__argsets));

	pArgset = u0066__report__function__push_operation__argsets + uArgset;
	VERIFY_ERR_CHECK(  _expect__push_operation(pCtx, pList, pArgset->szDescription, pArgset->uFlags, pArgset->szFile, pArgset->uLine)  );

fail:
	return;
}

void u0066__report__function__push_operation__invoke(SG_context* pCtx, SG_uint32 uArgset)
{
	const u0066__report__function__push_operation__argset* pArgset = NULL;

	SG_ASSERT(uArgset < SG_NrElements(u0066__report__function__push_operation__argsets));

	pArgset = u0066__report__function__push_operation__argsets + uArgset;
	VERIFY_ERR_CHECK(  SG_log__push_operation__internal(pCtx, pArgset->szDescription, pArgset->uFlags, pArgset->szFile, pArgset->uLine)  );

fail:
	return;
}

static const u0066__report__function _do__push_operation =
{
	SG_NrElements(u0066__report__function__push_operation__argsets),
	u0066__report__function__push_operation__expect,
	u0066__report__function__push_operation__invoke,
};

// ----- pop_operation -----

void u0066__report__function__pop_operation__expect(SG_context* pCtx, u0066__expectation__list* pList, SG_uint32 uArgset)
{
	const u0066__report__function__push_operation__argset* pArgset = NULL;

	SG_ASSERT(uArgset == 0u);

	pArgset = u0066__report__function__push_operation__argsets + uArgset;
	VERIFY_ERR_CHECK(  _expect__pop_operation(pCtx, pList)  );

fail:
	return;
}

void u0066__report__function__pop_operation__invoke(SG_context* pCtx, SG_uint32 uArgset)
{
	const u0066__report__function__push_operation__argset* pArgset = NULL;

	SG_ASSERT(uArgset == 0u);

	pArgset = u0066__report__function__push_operation__argsets + uArgset;
	VERIFY_ERR_CHECK(  SG_log__pop_operation(pCtx)  );

fail:
	return;
}

static const u0066__report__function _do__pop_operation =
{
	1u,
	u0066__report__function__pop_operation__expect,
	u0066__report__function__pop_operation__invoke,
};

// ----- test cases -----

typedef struct
{
	const u0066__report__function* aFunctions[32];
}
u0066__report__case;

static const u0066__report__case u0066__report__cases[] =
{
	{ { &_do__push_operation, &_do__pop_operation, NULL, }, },
};

// Note: Given a list of available functions, it should be possible to generate u0066__report__cases
//       programmatically by generating every possible ordering of the known functions.
//       One complication would be including permutations where each function can be
//       called multiple times (or zero times).

// ----- test function -----

void u0066__report__run_test_cases(SG_context* pCtx)
{
	u0066__expectation__list cList;
	SG_uint32                uIndex;

	for (uIndex = 0u; uIndex < SG_NrElements(u0066__report__cases); ++uIndex)
	{
		const u0066__report__case*     pCase     = u0066__report__cases + uIndex;
		const u0066__report__function* fFunction = NULL;

		// TODO: loop over each possible combination of argsets in the current function sequence
		//       i.e. for each function in the sequence, loop over its list of argsets
		//            in the innermost loop (over the last function's argsets), run the below code

			VERIFY_ERR_CHECK(  _list__init(pCtx, &cList)  );
			VERIFY_ERR_CHECK(  _expect__register_handler(pCtx, &cList)  );
			fFunction = pCase->aFunctions[0];
			while (fFunction != NULL)
			{
				fFunction->fExpect(pCtx, &cList, uCurrentArgsetForThisFunction);
				fFunction += 1u;
			}
			VERIFY_ERR_CHECK(  _expect__unregister_handler(pCtx, &cList)  );

			VERIFY_ERR_CHECK(  SG_log__register_handler(pCtx, gpTestHandler, (void*)&cList, SG_LOG__FLAG__NONE)  );
			fFunction = pCase->aFunctions[0];
			while (fFunction != NULL)
			{
				fFunction->fInvoke(pCtx, uCurrentArgsetForThisFunction);
				fFunction += 1u;
			}
			VERIFY_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpTestHandler, (void*)&cList)  );
	}

fail:
	return;
}
//*/


/*
**
** MAIN
**
*/

TEST_MAIN(u0066_log)
{
	TEMPLATE_MAIN_START;
	
	BEGIN_TEST(  u0066__register_handler(pCtx)  );
	BEGIN_TEST(  u0066__register_handler__badargs(pCtx)  );
	BEGIN_TEST(  u0066__register_handler__multiple(pCtx)  );
	BEGIN_TEST(  u0066__register_handler__recursive(pCtx)  );
	BEGIN_TEST(  u0066__unregister_handler__badargs(pCtx)  );
	BEGIN_TEST(  u0066__unregister_handler__recursive(pCtx)  );
	BEGIN_TEST(  u0066__unregister_all_handlers(pCtx)  );
	BEGIN_TEST(  u0066__push_operation__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0066__set_operation__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0066__set_operation__repeat(pCtx)  );
	BEGIN_TEST(  u0066__set_value__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0066__set_steps__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0066__set_steps__repeat(pCtx)  );
	BEGIN_TEST(  u0066__set_step__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0066__set_step__repeat(pCtx)  );
	BEGIN_TEST(  u0066__finish_steps__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0066__finish_steps__repeat(pCtx)  );
	BEGIN_TEST(  u0066__set_finished__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0066__set_finished__repeat(pCtx)  );
	BEGIN_TEST(  u0066__cancel_requested__false(pCtx)  );
	BEGIN_TEST(  u0066__cancel_requested__true(pCtx)  );
	BEGIN_TEST(  u0066__report_message(pCtx)  );
	BEGIN_TEST(  u0066__report_message__badargs(pCtx)  );
	BEGIN_TEST(  u0066__report_message__no_stack(pCtx)  );
	BEGIN_TEST(  u0066__report_message__current_error__detailed(pCtx)  );
	BEGIN_TEST(  u0066__filter_messages__errors_only(pCtx)  );
	BEGIN_TEST(  u0066__filter_messages__warnings_only(pCtx)  );
	BEGIN_TEST(  u0066__filter_messages__info_only(pCtx)  );
	BEGIN_TEST(  u0066__filter_messages__verbose_only(pCtx)  );
	BEGIN_TEST(  u0066__filter_messages__quiet(pCtx)  );
	BEGIN_TEST(  u0066__filter_messages__normal(pCtx)  );
	BEGIN_TEST(  u0066__filter_messages__all(pCtx)  );
	
	BEGIN_TEST(  u0066__filter_operations__no_verbose(pCtx)  );
	BEGIN_TEST(  u0066__filter_operations__all(pCtx)  );
	TEMPLATE_MAIN_END;
}
