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
#include "vvHistoryCommand.h"

#include "vvContext.h"
#include "vvHistoryDialog.h"
#include "vvRepoBrowserDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvHistoryCommand.
 */
static const vvHistoryCommand gcHistoryCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "history";
static const unsigned int guCommandFlags        = vvCommand::FLAG_NO_EXECUTE;
static const char*        gszCommandDescription = "History a repository object.";


/*
**
** Public Functions
**
*/

vvHistoryCommand::vvHistoryCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvHistoryCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to history.");
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

vvDialog* vvHistoryCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;
	this->mwDialog = new vvHistoryDialog();
	vvHistoryFilterOptions hfo;
	
	wxString sRepoPath;
	wxString sBranch = wxEmptyString;
	wxArrayString baselineHIDs;
	
	if (!DetermineRepo__from_workingFolder(this->mcPaths, true, true))
		return false;

	//All history arguments must be repo paths.
	if (!pVerbs->GetRepoPath(*pContext, this->mcPaths[0], this->GetRepoLocator().GetWorkingFolder(), sRepoPath))
	{
		pContext->Log_ReportError_CurrentError();
		return NULL;
	}
	if(!pVerbs->GetBranch(*pContext, this->GetRepoLocator().GetWorkingFolder(), sBranch))
	{
		pContext->Log_ReportError_CurrentError();
		return NULL;
	}
	if (!pVerbs->GetHistoryFilterDefaults(*pContext, this->GetRepoLocator().GetRepoName(), hfo))
	{
		pContext->Log_ReportError_CurrentError();
		return NULL;
	}
	hfo.sFileOrFolderPath = sRepoPath;
	hfo.sBranch = sBranch;
	if (!this->mwDialog->Create(wParent, hfo, this->GetRepoLocator(), *pVerbs, *pContext, this->GetBaselineHIDs()[0]))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

bool vvHistoryCommand::Execute()
{
	return true;
}
