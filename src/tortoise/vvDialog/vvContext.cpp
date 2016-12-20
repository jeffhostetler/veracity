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

#include "precompiled.h"
#include "vvContext.h"

#include "sg.h"
#include "vvFlags.h"
#include "vvSgHelpers.h"


/*
**
** Macros
**
*/

/**
 * Analagous to SG_ERR_IGNORE.
 */
#define IGNORE_ERROR(Expression) \
	this->Error_Push();          \
	Expression;                  \
	this->Error_Pop();


/*
**
** Internal Functions
**
*/

SG_log__message_type _TranslateMessageType(
	vvContext::Log_MessageType eType
	)
{
	wxCOMPILE_TIME_ASSERT(vvContext::LOG_MESSAGE_COUNT == SG_LOG__MESSAGE__COUNT, LogMessageCountsDontMatch);
	switch (eType)
	{
	case vvContext::LOG_MESSAGE_VERBOSE: return SG_LOG__MESSAGE__VERBOSE;
	case vvContext::LOG_MESSAGE_INFO:    return SG_LOG__MESSAGE__INFO;
	case vvContext::LOG_MESSAGE_WARNING: return SG_LOG__MESSAGE__WARNING;
	case vvContext::LOG_MESSAGE_ERROR:   return SG_LOG__MESSAGE__ERROR;
	default:
		wxLogError("Unknown message type: %d", eType);
		return SG_LOG__MESSAGE__COUNT;
	}
}

unsigned int _TranslateLogFlags(
	unsigned int uFlags
	)
{
	vvFlags cInput(uFlags);
	vvFlags cOutput(0u);

	wxCOMPILE_TIME_ASSERT(vvContext::LOG_FLAG_LAST == SG_LOG__FLAG__LAST, LogFlagCountsDontMatch);
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG_VERBOSE))
	{
		cOutput.AddFlags(SG_LOG__FLAG__VERBOSE);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG_CAN_CANCEL))
	{
		cOutput.AddFlags(SG_LOG__FLAG__CAN_CANCEL);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG_INPUT))
	{
		cOutput.AddFlags(SG_LOG__FLAG__INPUT);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG_INTERMEDIATE))
	{
		cOutput.AddFlags(SG_LOG__FLAG__INTERMEDIATE);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG_OUTPUT))
	{
		cOutput.AddFlags(SG_LOG__FLAG__OUTPUT);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG_MULTIPLE))
	{
		cOutput.AddFlags(SG_LOG__FLAG__MULTIPLE);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG__HANDLE_OPERATION__VERBOSE))
	{
		cOutput.AddFlags(SG_LOG__FLAG__HANDLE_OPERATION__VERBOSE);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG__HANDLE_OPERATION__NORMAL))
	{
		cOutput.AddFlags(SG_LOG__FLAG__HANDLE_OPERATION__NORMAL);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG__HANDLE_MESSAGE__ERROR))
	{
		cOutput.AddFlags(SG_LOG__FLAG__HANDLE_MESSAGE__ERROR);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG__HANDLE_MESSAGE__WARNING))
	{
		cOutput.AddFlags(SG_LOG__FLAG__HANDLE_MESSAGE__WARNING);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG__HANDLE_MESSAGE__INFO))
	{
		cOutput.AddFlags(SG_LOG__FLAG__HANDLE_MESSAGE__INFO);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG__HANDLE_MESSAGE__VERBOSE))
	{
		cOutput.AddFlags(SG_LOG__FLAG__HANDLE_MESSAGE__VERBOSE);
	}
	if (cInput.HasAnyFlag(vvContext::LOG_FLAG__DETAILED_MESSAGES))
	{
		cOutput.AddFlags(SG_LOG__FLAG__DETAILED_MESSAGES);
	}
	return cOutput.GetValue();
}


/*
**
** Public Functions
**
*/

void vvContext::Log_PopOperation_Static(
	vvContext* pContext
	)
{
	wxASSERT(pContext != NULL);
	pContext->Log_PopOperation();
}

void vvContext::Log_FinishStep_Static(
	vvContext* pContext
	)
{
	pContext->Log_FinishStep();
}

void vvContext::Log_FinishSteps_Static(
	vvContext*   pContext,
	unsigned int uSteps
	)
{
	pContext->Log_FinishSteps(uSteps);
}

vvContext::vvContext()
	: mpContext(NULL)
	, mbOwned(true)
	, muStackSize(0u)
{
	if (!SG_IS_OK(SG_context__alloc(&this->mpContext)))
	{
		wxLogError("Unable to initialize sglib context.");
	}
}

vvContext::vvContext(
	sgContext* pContext
	)
	: mpContext(pContext)
	, mbOwned(false)
	, muStackSize(0u)
{
}

vvContext::~vvContext()
{
	if (this->mbOwned)
	{
		SG_CONTEXT_NULLFREE(this->mpContext);
	}
}

vvContext::operator vvContext::sgContext*() const
{
	return this->mpContext;
}

vvContext::sgContext* vvContext::operator->() const
{
	return this->mpContext;
}

void vvContext::Error_Push()
{
	SG_context__push_level(*this);
}

void vvContext::Error_Pop()
{
	SG_context__pop_level(*this);
}

bool vvContext::Error_Check() const
{
	return (SG_CONTEXT__HAS_ERR(this->mpContext) == SG_TRUE);
}

wxUint64 vvContext::Get_Error_Code() const
{
	wxUint64 uError;

	SG_context__get_err(this->mpContext, (SG_error*)&uError);

	return uError;
}

bool vvContext::Error_Equals(
	wxUint64 uError
	)
{
	if (this->Error_Check())
	{
		return vvSgHelpers::Convert_sgBool_cppBool(SG_context__err_equals(this->mpContext, (SG_error)uError));
	}
	else
	{
		return false;
	}
}

void vvContext::Error_Reset()
{
	SG_context__err_reset(this->mpContext);
	m_lastLoggedErrorCode = 0;
}

wxString vvContext::Error_GetMessage()
{
	SG_string* sMessage = NULL;
	wxString   sResult  = wxEmptyString;

	if (this->Error_Check())
	{
#if defined(DEBUG)
		SG_context__err_to_string(this->mpContext, SG_TRUE, &sMessage);
#else
		SG_context__err_to_string(this->mpContext, SG_FALSE, &sMessage);
#endif
		sResult = vvSgHelpers::Convert_sgString_wxString(sMessage);
		SG_STRING_NULLFREE(this->mpContext, sMessage);
	}

	return sResult;
}

void vvContext::Log_PushOperation_Internal(
	const wxString& sDescription,
	unsigned int    uFlags,
	const wxString& sFile,
	unsigned int    uLine
	)
{
	IGNORE_ERROR(  SG_log__push_operation__internal(*this, this->mcStrings.Add(sDescription), _TranslateLogFlags(uFlags), this->mcStrings.Add(sFile), uLine)  );
	this->muStackSize += 1u;
}

void vvContext::Log_SetOperation(
	const wxString& sDescription
	)
{
	IGNORE_ERROR(  SG_log__set_operation(*this, this->mcStrings.Add(sDescription))  );
}

void vvContext::Log_SetValue(
	const wxString& sName,
	bool            bValue,
	unsigned int    uFlags
	)
{
	IGNORE_ERROR(  SG_log__set_value__bool(*this, this->mcStrings.Add(sName), bValue ? SG_TRUE : SG_FALSE, _TranslateLogFlags(uFlags))  );
}

void vvContext::Log_SetValue(
	const wxString& sName,
	wxInt64         iValue,
	unsigned int    uFlags
	)
{
	IGNORE_ERROR(  SG_log__set_value__int(*this, this->mcStrings.Add(sName), iValue, _TranslateLogFlags(uFlags))  );
}

void vvContext::Log_SetValue(
	const wxString& sName,
	wxUint64        uValue,
	unsigned int    uFlags
	)
{
	this->Log_SetValue(sName, static_cast<wxInt64>(uValue), uFlags);
}

void vvContext::Log_SetValue(
	const wxString& sName,
	int             iValue,
	unsigned int    uFlags
	)
{
	this->Log_SetValue(sName, static_cast<wxInt64>(iValue), uFlags);
}

void vvContext::Log_SetValue(
	const wxString& sName,
	unsigned int    uValue,
	unsigned int    uFlags
	)
{
	this->Log_SetValue(sName, static_cast<int>(uValue), uFlags);
}

void vvContext::Log_SetValue(
	const wxString& sName,
	double          dValue,
	unsigned int    uFlags
	)
{
	IGNORE_ERROR(  SG_log__set_value__double(*this, this->mcStrings.Add(sName), dValue, _TranslateLogFlags(uFlags))  );
}

void vvContext::Log_SetValue(
	const wxString& sName,
	const wxString& sValue,
	unsigned int    uFlags
	)
{
	IGNORE_ERROR(  SG_log__set_value__sz(*this, this->mcStrings.Add(sName), this->mcStrings.Add(sValue), _TranslateLogFlags(uFlags))  );
}

void vvContext::Log_SetValue(
	const wxString& sName,
	char*           szValue,
	unsigned int    uFlags
	)
{
	this->Log_SetValue(sName, wxString(szValue), uFlags);
}

void vvContext::Log_SetSteps(
	unsigned int uSteps
	)
{
	IGNORE_ERROR(  SG_log__set_steps(*this, uSteps, NULL)  );
}

void vvContext::Log_SetStep(
	const wxString& sDescription
	)
{
	IGNORE_ERROR(  SG_log__set_step(*this, this->mcStrings.Add(sDescription))  );
}

void vvContext::Log_FinishSteps(
	unsigned int uSteps
	)
{
	IGNORE_ERROR(  SG_log__finish_steps(*this, uSteps)  );
}

void vvContext::Log_FinishStep()
{
	IGNORE_ERROR(  SG_log__finish_step(*this)  );
}

void vvContext::Log_SetFinished(
	unsigned int uSteps
	)
{
	IGNORE_ERROR(  SG_log__set_finished(*this, uSteps)  );
}

void vvContext::Log_PopOperation()
{
	SG_log__pop_operation(*this);
	this->muStackSize -= 1u;
	if (this->muStackSize == 0u)
	{
		this->mcStrings.Clear();
	}
}

bool vvContext::Log_CancelRequested()
{
	SG_bool bCancel = SG_FALSE;
	IGNORE_ERROR(  SG_log__cancel_requested(*this, &bCancel)  );
	return vvSgHelpers::Convert_sgBool_cppBool(bCancel);
}

bool vvContext::Log_CheckCancel()
{
	IGNORE_ERROR(  SG_log__check_cancel(*this)  );
	return this->Error_Check();
}

bool vvContext::_Have_we_logged_current_error()
{
	wxUint64 currentErrorCode;
	SG_context__get_err(*this, &currentErrorCode);
	if (currentErrorCode == m_lastLoggedErrorCode)
		return true;
	else
		m_lastLoggedErrorCode = currentErrorCode;
	return false;

}

void vvContext::Log_ReportMessage_CurrentError(
	Log_MessageType eType
	)
{
	if (!_Have_we_logged_current_error())
		SG_log__report_message__current_error(*this, _TranslateMessageType(eType));
}

void vvContext::Log_ReportVerbose_CurrentError()
{
	if (!_Have_we_logged_current_error())
		SG_log__report_verbose__current_error(*this);
}

void vvContext::Log_ReportInfo_CurrentError()
{
	if (!_Have_we_logged_current_error())
		SG_log__report_info__current_error(*this);
}

void vvContext::Log_ReportWarning_CurrentError()
{
	if (!_Have_we_logged_current_error())
		SG_log__report_warning__current_error(*this);
}

void vvContext::Log_ReportError_CurrentError()
{
	if (!_Have_we_logged_current_error() && this->Error_Check())
		SG_log__report_error__current_error(*this);
}

vvSglibStringPool& vvContext::GetStringPool()
{
	return this->mcStrings;
}

void vvContext::Error_ResetReportT(
	const wxChar* szFormat,
	...
	)
{
	va_list pArgs;
	va_start(pArgs, szFormat);
	this->Error_ResetReportS(wxString::FormatV(szFormat, pArgs));
	va_end(pArgs);
}

void vvContext::Error_ResetReportA(
	const char* szFormat,
	...
	)
{
	va_list pArgs;
	va_start(pArgs, szFormat);
	this->Error_ResetReportS(wxString::FormatV(szFormat, pArgs));
	va_end(pArgs);
}

void vvContext::Error_ResetReportS(
	const wxString& sMessage
	)
{
	bool bError = this->Error_Check();
	if (bError)
	{
		if (!sMessage.IsEmpty())
		{
			this->Log_ReportError(sMessage);
		}
		this->Log_ReportError_CurrentError();
	}
	this->Error_Reset();
}

void vvContext::Log_ReportMessageT(
	Log_MessageType eType,
	const wxChar*   szFormat,
	...
	)
{
	wxASSERT(eType >= 0 && eType < vvContext::LOG_MESSAGE_COUNT);
	va_list pArgs;
	va_start(pArgs, szFormat);
	this->Log_ReportMessageS(eType, wxString::FormatV(szFormat, pArgs));
	va_end(pArgs);
}

void vvContext::Log_ReportMessageA(
	Log_MessageType eType,
	const char* szFormat,
	...
	)
{
	wxASSERT(eType >= 0 && eType < vvContext::LOG_MESSAGE_COUNT);
	va_list pArgs;
	va_start(pArgs, szFormat);
	this->Log_ReportMessageS(eType, wxString::FormatV(szFormat, pArgs));
	va_end(pArgs);
}

void vvContext::Log_ReportMessageS(
	Log_MessageType eType,
	const wxString& sMessage
	)
{
	IGNORE_ERROR(  SG_log__report_message(*this, _TranslateMessageType(eType), "%s", vvSgHelpers::Convert_wxString_sglibString(sMessage).data())  );
}

template <vvContext::Log_MessageType eType>
void vvContext::Log_ReportMessageTypeT(
	const wxChar* szFormat,
	...
	)
{
	wxCOMPILE_TIME_ASSERT(eType >= 0 && eType < vvContext::LOG_MESSAGE_COUNT, UnknownMessageType);
	va_list pArgs;
	va_start(pArgs, szFormat);
	this->Log_ReportMessageTypeS<eType>(wxString::FormatV(szFormat, pArgs));
	va_end(pArgs);
}

template <vvContext::Log_MessageType eType>
void vvContext::Log_ReportMessageTypeA(
	const char* szFormat,
	...
	)
{
	wxCOMPILE_TIME_ASSERT(eType >= 0 && eType < vvContext::LOG_MESSAGE_COUNT, UnknownMessageType);
	va_list pArgs;
	va_start(pArgs, szFormat);
	this->Log_ReportMessageTypeS<eType>(wxString::FormatV(szFormat, pArgs));
	va_end(pArgs);
}

template <vvContext::Log_MessageType eType>
void vvContext::Log_ReportMessageTypeS(
	const wxString& sMessage
	)
{
	wxCOMPILE_TIME_ASSERT(eType >= 0 && eType < vvContext::LOG_MESSAGE_COUNT, UnknownMessageType);
	this->Log_ReportMessageS(eType, sMessage);
}

template void vvContext::Log_ReportMessageTypeT<vvContext::LOG_MESSAGE_VERBOSE>(const wxChar* szFormat, ...);
template void vvContext::Log_ReportMessageTypeT<vvContext::LOG_MESSAGE_INFO>   (const wxChar* szFormat, ...);
template void vvContext::Log_ReportMessageTypeT<vvContext::LOG_MESSAGE_WARNING>(const wxChar* szFormat, ...);
template void vvContext::Log_ReportMessageTypeT<vvContext::LOG_MESSAGE_ERROR>  (const wxChar* szFormat, ...);
template void vvContext::Log_ReportMessageTypeA<vvContext::LOG_MESSAGE_VERBOSE>(const char* szFormat, ...);
template void vvContext::Log_ReportMessageTypeA<vvContext::LOG_MESSAGE_INFO>   (const char* szFormat, ...);
template void vvContext::Log_ReportMessageTypeA<vvContext::LOG_MESSAGE_WARNING>(const char* szFormat, ...);
template void vvContext::Log_ReportMessageTypeA<vvContext::LOG_MESSAGE_ERROR>  (const char* szFormat, ...);
