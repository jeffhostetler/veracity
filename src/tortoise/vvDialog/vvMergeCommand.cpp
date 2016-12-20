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
#include "vvMergeCommand.h"

#include "vvContext.h"
#include "vvCommand.h"
#include "vvRevSpecDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvMergeCommand.
 */
static const vvMergeCommand gcMergeCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "merge";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Merge another revision into a working copy.";


/*
**
** Public Functions
**
*/

vvMergeCommand::vvMergeCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvMergeCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No directory specified to merge a revision into.");
		return false;
	}
	wxString sPath = cArguments[0];

	// make sure this is actually an existing directory
	if (!wxFileExists(sPath) && !wxDirExists(sPath))
	{
		wxLogError("Specified directory doesn't exist: %s", sPath);
		return false;
	}

	// success
	this->msInitialSelection = sPath;
	return true;
}

bool vvMergeCommand::Init()
{
	return true;
}

vvDialog* vvMergeCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();
	vvRevSpec  cRevSpec(*pContext);
	wxString   sBranch  = wxEmptyString;

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	if (!DetermineRepo__from_workingFolder(this->msInitialSelection))
		return false;

	// if our working copy is attached to a branch
	// use that as the dialog's initial value
	if (!pVerbs->GetBranch(*pContext, this->GetRepoLocator().GetWorkingFolder(), sBranch))
	{
		pContext->Error_ResetReport("Unable to get branch attached to working copy.");
		return NULL;
	}
	if (!sBranch.IsEmpty())
	{
		if (!cRevSpec.AddBranch(*pContext, sBranch))
		{
			pContext->Error_ResetReport("Unable to add branch to rev spec.");
			return NULL;
		}
	}

	m_sBranch = sBranch;
	// create the dialog
	this->mwDialog = new vvRevSpecDialog();
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator(), *pVerbs, *pContext, cRevSpec, "Merge Working Copy", wxEmptyString, true))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

void vvMergeCommand::SetExecuteData(vvContext & pCtx, vvRepoLocator cRepoLocator, vvRevSpec revSpec)
{
	this->SetRepoLocator(cRepoLocator);
	mcRevSpec = vvRevSpec(pCtx, revSpec);
}

bool vvMergeCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	pContext->Log_PushOperation("Merging", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);

	if (mwDialog != NULL)
		SetExecuteData(*pContext, this->GetRepoLocator(), mwDialog->GetRevSpec());

	//Now check to see if the branch actually has multiple heads.
	bool bAttemptMerge = true;
	wxArrayString arBranches;
	this->mcRevSpec.GetBranches(*pContext, arBranches);
	//if they are trying to merge their currently-attached branch.
	if (arBranches.Count() != 0 && m_sBranch == arBranches[0] )
	{
		wxArrayString arBranchHeads;
		wxString sSelectedBranch = arBranches[0];
		pVerbs->GetBranchHeads(*pContext, this->GetRepoLocator().GetRepoName(), sSelectedBranch, arBranchHeads);
		if (arBranchHeads.Count() == 1)
		{
			//The branch does not need to be merged.
			pContext->Log_ReportWarning("The \"%s\" branch does not need to be merged", sSelectedBranch);
			bAttemptMerge = false;
		}
	}

	if (bAttemptMerge && !pVerbs->Merge(*pContext, this->GetRepoLocator().GetWorkingFolder(), this->mcRevSpec, false))
	{
		if (pContext->Error_Equals(SG_ERR_DAGLCA_LEAF_IS_ANCESTOR))
		{
			pContext->Error_Reset();
			wxMessageDialog * msg = new wxMessageDialog(mwDialog, wxString::Format("No changesets need to be merged.  Would you like to update the '%s' branch head to the target changeset?", m_sBranch), "No Merge Needed", wxYES_NO | wxYES_DEFAULT);
			if (msg->ShowModal() == wxID_YES)
			{
				if (!pVerbs->Merge(*pContext, this->GetRepoLocator().GetWorkingFolder(), this->mcRevSpec, true))
					pContext->Log_ReportError_CurrentError();
			}
		}
		else
			pContext->Log_ReportError_CurrentError();
	}

	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
