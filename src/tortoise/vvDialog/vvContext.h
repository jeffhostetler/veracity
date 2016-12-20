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

#ifndef VV_CONTEXT_H
#define VV_CONTEXT_H

#include "precompiled.h"
#include "vvSglibStringPool.h"


/**
 * Wrapper around SG_context.
 */
class vvContext
{
// static functionality
public:
	/**
	 * Static wrapper around Log_PopOperation.
	 * Intended for use with wxMakeGuard.
	 */
	static void Log_PopOperation_Static(
		vvContext* pContext //< Instance to call Log_PopOperation on.  Cannot be NULL.
		);

	/**
	 * Static wrapper around Log_FinishStep.
	 * Intended for use with wxMakeGuard.
	 */
	static void Log_FinishStep_Static(
		vvContext* pContext //< Instance to call Log_FinishStep on.  Cannot be NULL.
		);

	/**
	 * Static wrapper around Log_FinishSteps.
	 * Intended for use with wxMakeGuard.
	 */
	static void Log_FinishSteps_Static(
		vvContext*   pContext, //< Instance to call Log_FinishSteps on.  Cannot be NULL.
		unsigned int uSteps
		);

// types
public:
	/**
	 * Forward declaration of SG_context.
	 */
	typedef struct _sg_context sgContext;


	/**
	 * Wrapper around SG_log__message_type.
	 */
	enum Log_MessageType
	{
		LOG_MESSAGE_VERBOSE, //< Information that the average user generally doesn't care about.
		LOG_MESSAGE_INFO,    //< Information that an average user usually cares about.
		LOG_MESSAGE_WARNING, //< Information about a possible error condition that was ignored or recovered from.
		LOG_MESSAGE_ERROR,   //< Information about an irrecoverable error condition that caused a failure.
		LOG_MESSAGE_COUNT,   //< Number of elements in this enum, for iteration.
	};

	/**
	 * Wrapper around SG_log__flag
	 */
	enum Log_Flag
	{
		// flags for anything
		LOG_FLAG_NONE                       =      0, //< The empty/none flag value.

		// flags for both operations and values
		LOG_FLAG_VERBOSE                    = 1 << 0, //< The operation/value contains data that an average user usually doesn't care about.

		// flags for operations
		LOG_FLAG_CAN_CANCEL                 = 1 << 1, //< The operation checks for and honors cancel requests.

		// flags for values
		LOG_FLAG_INPUT                      = 1 << 2, //< Value is an input parameter to the operation.
		LOG_FLAG_INTERMEDIATE               = 1 << 3, //< Value is calculated and used during the operation.
		LOG_FLAG_OUTPUT                     = 1 << 4, //< Value is a result from the operation.

		// flags for handlers
		LOG_FLAG_MULTIPLE                   = 1 <<  5, //< Allow the same handler/data pair to be registered several times.
		LOG_FLAG__HANDLE_OPERATION__VERBOSE = 1 <<  6, //< This handler wants to receive operations flagged as verbose.
		LOG_FLAG__HANDLE_OPERATION__NORMAL  = 1 <<  7, //< This handler wants to receive operations not flagged as verbose.
		LOG_FLAG__HANDLE_MESSAGE__VERBOSE   = 1 <<  8, //< This handler wants to receive verbose messages.
		LOG_FLAG__HANDLE_MESSAGE__INFO      = 1 <<  9, //< This handler wants to receive info messages.
		LOG_FLAG__HANDLE_MESSAGE__WARNING   = 1 << 10, //< This handler wants to receive warning messages.
		LOG_FLAG__HANDLE_MESSAGE__ERROR     = 1 << 11, //< This handler wants to receive error messages.
		LOG_FLAG__DETAILED_MESSAGES         = 1 << 12, //< This handler wants to receive the detailed form of messages when available.

		// last, for iteration purposes
		LOG_FLAG_LAST                       = 1 << 13
	};

// construction
public:
	/**
	 * Default constructor.
	 * Allocates the underlying context.
	 */
	vvContext();

	/**
	 * Wrapper constructor.
	 * Wraps an existing context.
	 */
	vvContext(
		sgContext* pContext //< [in] The context to wrap.
		);

	/**
	 * Destructor.
	 * Frees the underlying context.
	 */
	~vvContext();

// functionality - compatibility
public:
	/**
	 * Implicit cast to the underlying context type.
	 */
	operator sgContext*() const;

	/**
	 * Implicit defeference to the underlying context type.
	 */
	sgContext* operator->() const;

// functionality - error checking
public:
	/**
	 * Pushes the current error level.
	 * Wrapper around SG_context__push_level.
	 */
	void Error_Push();

	/**
	 * Pops the current error level.
	 * Wrapper around SG_context__pop_level.
	 */
	void Error_Pop();

	/**
	 * Checks whether or not the context has an error.
	 * Returns true if the context has an error, or false if it doesn't.
	 * Wrapper around SG_CONTEXT__HAS_ERR.
	 */
	bool Error_Check() const;

	/**
	 *
	 * Returns the current error code.
	 *
	 */
	wxUint64 vvContext::Get_Error_Code() const;

	/**
	 * Checks if the current error equals a given error.
	 * Returns true if the current error is the given error.
	 * Returns false if the context has no error, or the current error is not the given one.
	 * Wrapper around SG_context__err_equals.
	 */
	bool Error_Equals(
		wxUint64 uError //< [in] The SG_error to compare against.
		);

	/**
	 * Resets the error state of the context.
	 * Wrapper around SG_context__err_reset.
	 */
	void Error_Reset();

	/**
	 * Wrapper around Error_Reset that reports any current error to
	 * Log_ReportError_CurrentError before resetting the error state.
	 * Also accepts an optional error message to print out along with
	 * the current error, to provide additional context.
	 */
	WX_DEFINE_VARARG_FUNC(void, Error_ResetReport, 1, (wxFormatString), Error_ResetReportT, Error_ResetReportA)

	/**
	 * Retrieves a message for the current error.
	 * Returns wxEmptyString if there is no current error.
	 * The error message is detailed only in debug builds.
	 */
	wxString Error_GetMessage();

// functionality - logging
public:
	/**
	 * Wrapper around SG_log__push_operation__internal.
	 */
	void Log_PushOperation_Internal(
		const wxString& sDescription,
		unsigned int    uFlags,
		const wxString& sFile,
		unsigned int    uLine
		);

	/**
	 * Wrapper around PushOperation_Internal that automatically captures file/line information.
	 */
	#define Log_PushOperation(sDescription, uFlags) Log_PushOperation_Internal(sDescription, uFlags, __FILE__, __LINE__)

	/**
	 * Wrapper around SG_log__set_operation.
	 */
	void Log_SetOperation(
		const wxString& sDescription
		);

	/*
	 * Wrappers around SG_log__set_value__*.
	 */
	void Log_SetValue(
		const wxString& sName,
		bool            bValue,
		unsigned int    uFlags
		);
	void Log_SetValue(
		const wxString& sName,
		wxInt64         iValue,
		unsigned int    uFlags
		);
	void Log_SetValue(
		const wxString& sName,
		wxUint64        uValue,
		unsigned int    uFlags
		);
	void Log_SetValue(
		const wxString& sName,
		int             iValue,
		unsigned int    uFlags
		);
	void Log_SetValue(
		const wxString& sName,
		unsigned int    uValue,
		unsigned int    uFlags
		);
	void Log_SetValue(
		const wxString& sName,
		double          dValue,
		unsigned int    uFlags
		);
	void Log_SetValue(
		const wxString& sName,
		const wxString& sValue,
		unsigned int    uFlags
		);
	void Log_SetValue(
		const wxString& sName,
		char*           szValue,
		unsigned int    uFlags
		);

	/**
	 * Wrapper around SG_log__set_steps.
	 */
	void Log_SetSteps(
		unsigned int uSteps
		);

	/**
	 * Wrapper around SG_log__set_step.
	 */
	void Log_SetStep(
		const wxString& sDescription
		);

	/**
	 * Wrapper around SG_log__finish_steps.
	 */
	void Log_FinishSteps(
		unsigned int uSteps
		);

	/**
	 * Wrapper around SG_log__finish_step.
	 */
	void Log_FinishStep();

	/**
	 * Wrapper around SG_log__set_finished.
	 */
	void Log_SetFinished(
		unsigned int uSteps
		);

	/**
	 * Wrapper around SG_log__pop_operation.
	 */
	void Log_PopOperation();

	/**
	 * Wrapper around SG_log__cancel_requested.
	 */
	bool Log_CancelRequested();

	/**
	 * Wrapper around SG_log__check_cancel.
	 * Returns true if cancelation was requested, or false if it wasn't.
	 */
	bool Log_CheckCancel();

	/**
	 * Wrapper around SG_log__report_message.
	 */
	WX_DEFINE_VARARG_FUNC(void, Log_ReportMessage, 2, (Log_MessageType, wxFormatString), Log_ReportMessageT, Log_ReportMessageA)

	/**
	 * Wrapper for Log_ReportMessage(MESSAGE_VERBOSE, ...).
	 */
	WX_DEFINE_VARARG_FUNC(void, Log_ReportVerbose, 1, (wxFormatString), Log_ReportMessageTypeT<LOG_MESSAGE_VERBOSE>, Log_ReportMessageTypeA<MESSAGE_VERBOSE>)

	/**
	 * Wrapper for Log_ReportMessage(MESSAGE_INFO, ...).
	 */
	WX_DEFINE_VARARG_FUNC(void, Log_ReportInfo, 1, (wxFormatString), Log_ReportMessageTypeT<LOG_MESSAGE_INFO>, Log_ReportMessageTypeA<MESSAGE_INFO>)

	/**
	 * Wrapper for Log_ReportMessage(MESSAGE_WARNING, ...).
	 */
	WX_DEFINE_VARARG_FUNC(void, Log_ReportWarning, 1, (wxFormatString), Log_ReportMessageTypeT<LOG_MESSAGE_WARNING>, Log_ReportMessageTypeA<MESSAGE_WARNING>)

	/**
	 * Wrapper for Log_ReportMessage(MESSAGE_ERROR, ...).
	 */
	WX_DEFINE_VARARG_FUNC(void, Log_ReportError, 1, (wxFormatString), Log_ReportMessageTypeT<LOG_MESSAGE_ERROR>, Log_ReportMessageTypeA<MESSAGE_ERROR>)

	/**
	 * Wrapper around SG_log__report_message__current_error.
	 */
	void Log_ReportMessage_CurrentError(
		Log_MessageType eType
		);

	/**
	 * Wrapper around SG_log__report_verbose__current_error.
	 */
	void Log_ReportVerbose_CurrentError();

	/**
	 * Wrapper around SG_log__report_info__current_error.
	 */
	void Log_ReportInfo_CurrentError();

	/**
	 * Wrapper around SG_log__report_warning__current_error.
	 */
	void Log_ReportWarning_CurrentError();

	/**
	 * Wrapper around SG_log__report_error__current_error.
	 */
	void Log_ReportError_CurrentError();

// functionality - misc.
public:
	/**
	 * Gets a pool of sglib-encoded strings cached in the context.
	 * This is meant to allow callers to add new strings to the pool (using
	 * any overloads available on the pool), so that they can have a convenient
	 * place to cache a string that will be passed to sglib and needs to remain
	 * valid for longer than the source string will exist.
	 * Warning: Do NOT remove strings from the returned pool.
	 */
	vvSglibStringPool& GetStringPool();

// private functionality
private:
	// implementations of the variadic wrappers
	// T (as in TChar) variants are the native character size implementation.
	// A (as in ASCII) variants are the 8-bit character size implementation.
	// S (as in String) variants are usually what the T and A versions are implemented in terms of.
	void Error_ResetReportT(const wxChar* szFormat, ...);
	void Error_ResetReportA(const char*   szFormat, ...);
	void Error_ResetReportS(const wxString& sMessage);
	void Log_ReportMessageT(Log_MessageType eType, const wxChar* szFormat, ...);
	void Log_ReportMessageA(Log_MessageType eType, const char*   szFormat, ...);
	void Log_ReportMessageS(Log_MessageType eType, const wxString& sMessage);
	template <Log_MessageType eType> void Log_ReportMessageTypeT(const wxChar* szFormat, ...);
	template <Log_MessageType eType> void Log_ReportMessageTypeA(const char*   szFormat, ...);
	template <Log_MessageType eType> void Log_ReportMessageTypeS(const wxString& sMessage);

	/*
	 * Check to see if the current error has already been logged.
	 */
	bool _Have_we_logged_current_error();

// private data
private:
	sgContext*        mpContext;             //< The SG_context* that we're wrapping.
	bool              mbOwned;               //< Whether or not we own mpContext.
	vvSglibStringPool mcStrings;             //< Local memory for strings that are being logged.
	unsigned int      muStackSize;           //< Size of the operation stack being logged.
	wxUint64          m_lastLoggedErrorCode; //The error code for the last error that we logged.  This is used to prevent duplicate logging.
};


#endif
