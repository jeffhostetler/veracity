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
#include "vvSgWxLogHandler.h"

#include "sg.h"
#include "vvContext.h"
#include "vvSgHelpers.h"


/*
**
** Types
**
*/

// forward declarations of internal functions
static SG_log__handler__message _Handler_Message;


/*
**
** Globals
**
*/

/**
 * Sglib log handler.
 */
static SG_log__handler gcWxLogHandlerHandler =
{
	NULL,
	NULL,
	_Handler_Message,
	NULL,
};


/*
**
** Internal Functions
**
*/

/**
 * Sglib log handler function for reported messages.
 * See SG_log__handler__message.
 */
void _Handler_Message(
	SG_context*              pCtx,
	void*                    pThis,
	const SG_log__operation* pOperation,
	SG_log__message_type     eType,
	const SG_string*         pMessage,
	SG_bool*                 pCancel
	)
{
	vvSgWxLogHandler* pHandler = static_cast<vvSgWxLogHandler*>(pThis);
	wxString          sMessage = vvSgHelpers::Convert_sgString_wxString(pMessage);

	SG_UNUSED(pCtx);
	SG_UNUSED(pOperation);
	SG_UNUSED(pHandler);

	switch (eType)
	{
	case SG_LOG__MESSAGE__VERBOSE: wxLogVerbose("%s", sMessage); break;
	case SG_LOG__MESSAGE__INFO:    wxLogMessage("%s", sMessage); break;
	case SG_LOG__MESSAGE__WARNING: wxLogWarning("%s", sMessage); break;
	case SG_LOG__MESSAGE__ERROR:   wxLogError("%s", sMessage);   break;
	default: break;
	}

	*pCancel = SG_FALSE;
}


/*
**
** Public Functions
**
*/

vvSgWxLogHandler::vvSgWxLogHandler()
	: vvSgGeneralLogHandler(gcWxLogHandlerHandler, static_cast<void*>(this))
{
}
