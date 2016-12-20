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
#include "vvSgLogHandler.h"

#include "sg.h"
#include "vvApplication.h"
#include "vvContext.h"
#include "vvSgHelpers.h"
#include "vvVerbs.h"


/*
**
** Internal Functions
**
*/

/**
 * SG_log_text__writer implementation used by vvSgTextLogHandler.
 */
void _TextWriter(
	SG_context*      pCtx,        //< [in] [out] Error and context information.
	void*            pWriterData, //< [in] Data specific to the writer.
	const SG_string* pText,       //< [in] The text to write.
	                              //<      If NULL, then the writer is being initialized or destroyed (check bError to determine which).
	SG_bool          bError       //< [in] Whether or not the text should be considered an error.
	                              //<      This is for things like choosing STDERR vs STDOUT.
	                              //<      If szText is NULL, then this will be true for initialization or false for destruction.
	)
{
	vvContext cContext(pCtx);

	if (pText == NULL)
	{
		return;
	}

	vvSgTextLogHandler* pThis = static_cast<vvSgTextLogHandler*>(pWriterData);
	pThis->DoWrite(cContext, vvSgHelpers::Convert_sgString_wxString(pText), vvSgHelpers::Convert_sgBool_cppBool(bError));
}


/*
**
** Public Functions
**
*/

// ----- vvSgLogHandler -----

vvSgLogHandler::vvSgLogHandler()
	: mnbRegistered(vvNULL)
	, mcFlags(SG_LOG__FLAG__HANDLER_TYPE__ALL)
	, mpStats(NULL)
{
}

vvSgLogHandler::~vvSgLogHandler()
{
	wxASSERT(this->mnbRegistered.IsNull());

	if (this->mpStats != NULL)
	{
		delete this->mpStats;
	}
}

void vvSgLogHandler::SetLevel(
	Level eLevel
	)
{
	switch (eLevel)
	{
	case LEVEL_ALL:
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_VERBOSE, true);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_INFO, true);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_WARNING, true);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_ERROR, true);
		this->SetReceiveOperation(true, true);
		this->SetReceiveOperation(false, true);
		break;
	case LEVEL_NORMAL:
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_VERBOSE, false);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_INFO, true);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_WARNING, true);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_ERROR, true);
		this->SetReceiveOperation(true, false);
		this->SetReceiveOperation(false, true);
		break;
	case LEVEL_QUIET:
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_VERBOSE, false);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_INFO, false);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_WARNING, true);
		this->SetReceiveMessage(vvContext::LOG_MESSAGE_ERROR, true);
		this->SetReceiveOperation(true, false);
		this->SetReceiveOperation(false, false);
		break;
	default:
		wxFAIL_MSG("Unknown log level.");
		break;
	}
}

void vvSgLogHandler::SetLevelFromConfig(
	vvVerbs&   cVerbs,
	vvContext& cContext
	)
{
	wxString sLogLevel;

	cVerbs.GetConfigValue(cContext, 0, SG_LOCALSETTING__LOG_LEVEL, wxEmptyString, sLogLevel, wxEmptyString);

	if (!sLogLevel.IsEmpty())
	{
		if (sLogLevel.CmpNoCase("quiet") == 0)
		{
			this->SetLevel(LEVEL_QUIET);
		}
		else if (sLogLevel.CmpNoCase("normal") == 0)
		{
			this->SetLevel(LEVEL_NORMAL);
		}
		else if (sLogLevel.CmpNoCase("verbose") == 0)
		{
			this->SetLevel(LEVEL_ALL);
		}
	}
}

void vvSgLogHandler::SetReceiveMessage(
	vvContext::Log_MessageType eType,
	bool                       bReceive
	)
{
	unsigned int uFlag = 0u;
	switch (eType)
	{
	case vvContext::LOG_MESSAGE_ERROR:
		uFlag = SG_LOG__FLAG__HANDLE_MESSAGE__ERROR;
		break;
	case vvContext::LOG_MESSAGE_WARNING:
		uFlag = SG_LOG__FLAG__HANDLE_MESSAGE__WARNING;
		break;
	case vvContext::LOG_MESSAGE_INFO:
		uFlag = SG_LOG__FLAG__HANDLE_MESSAGE__INFO;
		break;
	case vvContext::LOG_MESSAGE_VERBOSE:
		uFlag = SG_LOG__FLAG__HANDLE_MESSAGE__VERBOSE;
		break;
	}
	this->UpdateFlags(uFlag, bReceive);
}

void vvSgLogHandler::SetReceiveDetailedMessages(
	vvNullable<bool> nbDetailed
	)
{
	if (nbDetailed.IsNull())
	{
#if defined(DEBUG)
		nbDetailed = true;
#else
		nbDetailed = false;
#endif
	}
	this->UpdateFlags(SG_LOG__FLAG__DETAILED_MESSAGES, nbDetailed.GetValue());
}

void vvSgLogHandler::SetReceiveOperation(
	bool bVerbose,
	bool bReceive
	)
{
	unsigned int uFlag = 0u;
	if (bVerbose)
	{
		uFlag = SG_LOG__FLAG__HANDLE_OPERATION__VERBOSE;
	}
	else
	{
		uFlag = SG_LOG__FLAG__HANDLE_OPERATION__NORMAL;
	}
	this->UpdateFlags(uFlag, bReceive);
}

void vvSgLogHandler::EnableStatTracking(
	bool bStats
	)
{
	wxASSERT_MSG(this->mnbRegistered.IsNull(), "Stat tracking must be enabled/disabled before registration.");

	// check if we're turning tracking on or off
	if (bStats)
	{
		// if we don't have a stats structure yet, create one
		if (this->mpStats == NULL)
		{
			this->mpStats = new sgLogStats;
		}

		// zero out the stats data
		memset(this->mpStats, 0, sizeof(sgLogStats));
	}
	else
	{
		// if we already have a stats structure, delete it
		if (this->mpStats != NULL)
		{
			delete this->mpStats;
			this->mpStats = NULL;
		}
	}
}

unsigned int vvSgLogHandler::GetMessageCount(
	vvNullable<vvContext::Log_MessageType> neType
	) const
{
	wxASSERT_MSG(this->mpStats != NULL, "Stat retrieval is only possible if stat tracking is enabled.");

	if (neType.IsNull())
	{
		return this->mpStats->uMessages_Total;
	}
	else
	{
		switch (neType.GetValue())
		{
		case vvContext::LOG_MESSAGE_VERBOSE:
			return this->mpStats->uMessages_Type[SG_LOG__MESSAGE__VERBOSE];
		case vvContext::LOG_MESSAGE_INFO:
			return this->mpStats->uMessages_Type[SG_LOG__MESSAGE__INFO];
		case vvContext::LOG_MESSAGE_WARNING:
			return this->mpStats->uMessages_Type[SG_LOG__MESSAGE__WARNING];
		case vvContext::LOG_MESSAGE_ERROR:
			return this->mpStats->uMessages_Type[SG_LOG__MESSAGE__ERROR];
		default:
			wxFAIL_MSG("Unknown log message type.");
			return 0u;
		}
	}
}

unsigned int vvSgLogHandler::GetOperationCount(
	vvNullable<bool> nbSuccessful
	) const
{
	wxASSERT_MSG(this->mpStats != NULL, "Stat retrieval is only possible if stat tracking is enabled.");

	if (nbSuccessful.IsNull())
	{
		return this->mpStats->uOperations_Total;
	}
	else if (nbSuccessful.GetValue())
	{
		return this->mpStats->uOperations_Successful;
	}
	else
	{
		return this->mpStats->uOperations_Failed;
	}
}

bool vvSgLogHandler::RegisterSgLogHandler(
	vvContext& cContext,
	bool       bExclusive
	)
{
	if (this->mnbRegistered.IsValid())
	{
		return true;
	}

	SG_log__global_init(cContext);
	if (cContext.Error_Check())
	{
		return false;
	}

	if (!this->DoRegister(cContext, this->mcFlags.GetValue(), this->mpStats))
	{
		return false;
	}

	if (bExclusive && !wxGetApp().EnableGlobalSglibLogging(cContext, false))
	{
		return false;
	}

	this->mnbRegistered = bExclusive;
	return true;
}

bool vvSgLogHandler::UnregisterSgLogHandler(
	vvContext& cContext
	)
{
	if (this->mnbRegistered.IsNull())
	{
		return true;
	}

	if (!this->DoUnregister(cContext))
	{
		return false;
	}

	if (this->mnbRegistered.GetValue() && !wxGetApp().EnableGlobalSglibLogging(cContext, true))
	{
		return false;
	}

	this->mnbRegistered.SetNull();
	return true;
}

void vvSgLogHandler::UpdateFlags(
	unsigned int uFlags,
	bool         bAdd
	)
{
	if (bAdd)
	{
		this->mcFlags.AddFlags(uFlags);
	}
	else
	{
		this->mcFlags.RemoveFlags(uFlags);
	}
}

// ----- vvSgGeneralLogHandler -----

vvSgGeneralLogHandler::vvSgGeneralLogHandler(
	sgLogHandler& cHandler,
	void*         pUserData
	)
	: mpHandler(&cHandler)
	, mpUserData(pUserData)
{
}

bool vvSgGeneralLogHandler::DoRegister(
	vvContext&   cContext,
	unsigned int uFlags,
	sgLogStats*  pStats
	)
{
	SG_log__register_handler(cContext, this->mpHandler, this->mpUserData, pStats, uFlags);
	return !cContext.Error_Check();
}

bool vvSgGeneralLogHandler::DoUnregister(
	vvContext& cContext
	)
{
	SG_log__unregister_handler(cContext, this->mpHandler, this->mpUserData);
	return !cContext.Error_Check();
}

// ----- vvSgTextLogHandler -----

vvSgTextLogHandler::vvSgTextLogHandler(
	vvContext& cContext
	)
{
	this->mpHandler = new SG_log_text__data;

	SG_log_text__set_defaults(cContext, this->mpHandler);
	if (cContext.Error_Check())
	{
		delete this->mpHandler;
		this->mpHandler = NULL;
		return;
	}

	this->mpHandler->fWriter     = _TextWriter;
	this->mpHandler->pWriterData = this;
}

vvSgTextLogHandler::~vvSgTextLogHandler()
{
	if (this->mpHandler != NULL)
	{
		delete this->mpHandler;
	}
}

bool vvSgTextLogHandler::IsOk() const
{
	return (this->mpHandler != NULL);
}

#define IMPLEMENT_PROPERTY__BOOL(FunctionName, PropertyName)                         \
	void vvSgTextLogHandler::FunctionName(                                           \
		bool bValue                                                                  \
		)                                                                            \
	{                                                                                \
		this->mpHandler->PropertyName = vvSgHelpers::Convert_cppBool_sgBool(bValue); \
	}

#define IMPLEMENT_PROPERTY__STRING(FunctionName, PropertyName)       \
void vvSgTextLogHandler::FunctionName(                               \
	const wxString& sValue                                           \
	)                                                                \
{                                                                    \
	if (sValue.IsEmpty())                                            \
	{                                                                \
		this->mpHandler->PropertyName = NULL;                        \
	}                                                                \
	else                                                             \
	{                                                                \
		this->mpHandler->PropertyName = this->mcStrings.Add(sValue); \
	}                                                                \
}

IMPLEMENT_PROPERTY__BOOL(SetLogVerboseOperations, bLogVerboseOperations);
IMPLEMENT_PROPERTY__BOOL(SetLogVerboseValues,     bLogVerboseValues);

IMPLEMENT_PROPERTY__STRING(SetRegisterMessage,     szRegisterMessage);
IMPLEMENT_PROPERTY__STRING(SetUnregisterMessage,   szUnregisterMessage);
IMPLEMENT_PROPERTY__STRING(SetDateTimeFormat,      szDateTimeFormat);
IMPLEMENT_PROPERTY__STRING(SetProcessThreadFormat, szProcessThreadFormat);
IMPLEMENT_PROPERTY__STRING(SetIndent,              szIndent);
IMPLEMENT_PROPERTY__STRING(SetElapsedTimeFormat,   szElapsedTimeFormat);
IMPLEMENT_PROPERTY__STRING(SetCompletionFormat,    szCompletionFormat);
IMPLEMENT_PROPERTY__STRING(SetOperationFormat,     szOperationFormat);
IMPLEMENT_PROPERTY__STRING(SetValueFormat,         szValueFormat);
IMPLEMENT_PROPERTY__STRING(SetStepFormat,          szStepFormat);
IMPLEMENT_PROPERTY__STRING(SetStepMessage,         szStepMessage);
IMPLEMENT_PROPERTY__STRING(SetResultFormat,        szResultFormat);
IMPLEMENT_PROPERTY__STRING(SetVerboseFormat,       szVerboseFormat);
IMPLEMENT_PROPERTY__STRING(SetInfoFormat,          szInfoFormat);
IMPLEMENT_PROPERTY__STRING(SetWarningFormat,       szWarningFormat);
IMPLEMENT_PROPERTY__STRING(SetErrorFormat,         szErrorFormat);

#undef IMPLEMENT_PROPERTY__BOOL
#undef IMPLEMENT_PROPERTY__STRING

void vvSgTextLogHandler::SetLevel(
	Level eLevel
	)
{
	// pass the level along to the base class
	vvSgLogHandler::SetLevel(eLevel);

	switch (eLevel)
	{
	case LEVEL_ALL:
		this->SetLogVerboseOperations(true);
		this->SetLogVerboseValues(true);
		this->SetRegisterMessage("---- vvDialog started logging ----");
		this->SetUnregisterMessage("---- vvDialog stopped logging ----");
		break;
	case LEVEL_NORMAL:
		this->SetLogVerboseOperations(false);
		this->SetLogVerboseValues(false);
		this->SetRegisterMessage(wxEmptyString);
		this->SetUnregisterMessage(wxEmptyString);
		break;
	case LEVEL_QUIET:
		this->SetLogVerboseOperations(false);
		this->SetLogVerboseValues(false);
		this->SetRegisterMessage(wxEmptyString);
		this->SetUnregisterMessage(wxEmptyString);
		break;
	default:
		wxFAIL_MSG("Unknown log level.");
		break;
	}
}

bool vvSgTextLogHandler::DoRegister(
	vvContext&   cContext,
	unsigned int uFlags,
	sgLogStats*  pStats
	)
{
	SG_log_text__register(cContext, this->mpHandler, pStats, uFlags);
	return !cContext.Error_Check();
}

bool vvSgTextLogHandler::DoUnregister(
	vvContext& cContext
	)
{
	SG_log_text__unregister(cContext, this->mpHandler);
	return !cContext.Error_Check();
}
