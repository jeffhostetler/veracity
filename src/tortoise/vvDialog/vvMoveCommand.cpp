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
#include "vvMoveCommand.h"

#include "vvContext.h"
#include "vvMoveDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvMoveCommand.
 */
static const vvMoveCommand gcMoveCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "move";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Move a repository object.";


/*
**
** Public Functions
**
*/

vvMoveCommand::vvMoveCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvMoveCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to move.");
		return false;
	}

	// store the given paths
	for (wxArrayString::const_iterator it = cArguments.begin(); it != cArguments.end(); ++it)
	{
		this->mcPaths.Add(*it);
	}
	// success
	return true;
}

vvDialog* vvMoveCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;
	if (!DetermineRepo__from_workingFolder(this->mcPaths))
		return false;

	this->mwDialog = new vvMoveDialog();
	if (!this->mwDialog->Create(wParent, this->mcPaths[0], this->mcPaths.GetCount(), *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

bool vvMoveCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	pContext->Log_PushOperation("Moving object", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);

	wxString sNewPath = this->mwDialog->GetNewPath();
	if (sNewPath.IsEmpty())
	{
		pContext->Log_ReportError("The new path is empty.");
	}
	else if (!pVerbs->Move(*pContext, this->mcPaths, sNewPath))
	{
		pContext->Log_ReportError_CurrentError();
	}

	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
