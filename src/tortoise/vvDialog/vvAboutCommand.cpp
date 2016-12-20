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
#include "vvAboutCommand.h"

#include "vvContext.h"
#include "vvAboutDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvAboutCommand.
 */
static const vvAboutCommand gcAboutCommand(vvCommand::FLAG_GLOBAL_INSTANCE | vvCommand::FLAG_NO_EXECUTE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "about";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Display information about Veracity.";


/*
**
** Public Functions
**
*/

vvAboutCommand::vvAboutCommand(unsigned int extraFlags)
	: vvCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvAboutCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	cArguments;
	// success
	return true;
}

vvDialog* vvAboutCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;

	
	this->mwDialog = new vvAboutDialog();
	if (!this->mwDialog->Create(wParent, *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

bool vvAboutCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	pContext->Log_PushOperation("Running About Dialog", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);

	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
