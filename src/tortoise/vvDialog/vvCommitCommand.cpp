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
#include "vvCommitCommand.h"

#include "vvContext.h"
#include "vvCommitDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvCommitCommand.
 */
static const vvCommitCommand gcCommitCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "commit";
static const unsigned int guCommandFlags        = 0u;
static const char*        gszCommandDescription = "Commit a working copy to its repository.";


/*
**
** Public Functions
**
*/

vvCommitCommand::vvCommitCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvCommitCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// make sure we got at least one path
	if (cArguments.Count() < 1u)
	{
		wxLogError("No path(s) specified to commit from.");
		return false;
	}

	// store the given paths
	for (wxArrayString::const_iterator it = cArguments.begin(); it != cArguments.end(); ++it)
	{
		this->mcAbsolutePaths.Add(*it);
	}

	// success
	return true;
}

bool vvCommitCommand::Init()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	if (!DetermineRepo__from_workingFolder(this->mcAbsolutePaths))
		return false;

	// convert our absolute paths to repo paths
	if (!pVerbs->GetRepoPaths(*pContext, this->mcAbsolutePaths, this->GetRepoLocator().GetWorkingFolder(), this->mcRepoPaths))
	{
		pContext->Error_ResetReport("Failed to translate the given paths into repo paths relative to working copy: %s", this->GetRepoLocator().GetWorkingFolder());
		return false;
	}
	wxASSERT(this->mcAbsolutePaths.size() == this->mcRepoPaths.size());

	return true;
}

vvDialog* vvCommitCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	this->mwDialog = new vvCommitDialog();
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator().GetWorkingFolder(), *pVerbs, *pContext, &this->mcRepoPaths))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

bool vvCommitCommand::Execute()
{
	vvVerbs*             pVerbs   = this->GetVerbs();
	vvContext*           pContext = this->GetContext();
	bool                 bSuccess = true;
	bool                 bBranch  = false;
	bool                 bAll     = false;
	const wxArrayString& cFiles   = this->mwDialog->GetSelectedFiles(&bAll);
	const wxArrayString& asWorkItems = this->mwDialog->GetWorkItemAssociations();

	//There's no need to start an operation here, since one will be started inside sglib.

	//Do the branch action that the user requested
	bSuccess = bBranch = bSuccess && pVerbs->PerformBranchAttachmentAction(*pContext, this->GetRepoLocator().GetWorkingFolder(), this->mwDialog->GetBranchAction(), this->mwDialog->GetBranchName());

	// run the commit
	bSuccess = bSuccess && pVerbs->Commit(*pContext, this->GetRepoLocator().GetWorkingFolder(), this->mwDialog->GetComment(), !bAll ? &cFiles : NULL, &this->mwDialog->GetSelectedStamps(),  asWorkItems.Count() == 0 ? NULL : &asWorkItems, NULL, this->mwDialog->GetBranchName().IsEmpty() ? vvVerbs::COMMIT_DETACHED : 0);

	// check if anything went wrong
	if (!bSuccess)
	{
		// if the context has an error, report it
		if (pContext->Error_Check())
		{
			pContext->Log_ReportError_CurrentError();
		}

		// if we did a branch action, undo it
		if (bBranch)
		{
			pContext->Error_Push();
			if (!pVerbs->PerformBranchAttachmentAction(*pContext, this->GetRepoLocator().GetWorkingFolder(), this->mwDialog->GetBranchRestoreAction(), this->mwDialog->GetOriginalBranchName()))
			{
				pContext->Log_ReportError_CurrentError();
			}
			pContext->Error_Pop();
		}
	}

	if (!pContext->Error_Check())
	{
		//Tell veracity_cache to refresh all of the paths that were committed.
		wxArrayString cAbsolutePaths;
		for (wxArrayString::const_iterator it = cFiles.begin(); it != cFiles.end(); ++it)
		{
			wxString sAbsolutePath;
			if (!it->StartsWith("@/"))
			{
				//It's a path like @b/, @c/, or @g
				//Skip it.
				continue;
			}
			pVerbs->GetAbsolutePath(*pContext, this->GetRepoLocator().GetWorkingFolder(), *it, sAbsolutePath, false);
			if (!pContext->Error_Check())
			{
				cAbsolutePaths.push_back(sAbsolutePath);
			}
			else
				pContext->Error_Reset();
		}
		pVerbs->RefreshWC(*pContext, this->GetRepoLocator().GetWorkingFolder(), &cAbsolutePaths);
	}

	// done
	return bSuccess;
}
