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
#include "vvCreateCommand.h"

#include "vvContext.h"
#include "vvCreateDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvCreateCommand.
 */
static const vvCreateCommand gcCreateCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "create";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Create a working copy from a new or existing repo.";


/*
**
** Public Functions
**
*/

vvCreateCommand::vvCreateCommand(unsigned int extraFlags)
	: vvCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvCreateCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No directory specified to create a working copy in.");
		return false;
	}
	wxString sDirectory = cArguments[0];

	// make sure this is actually an existing directory
	if (!wxDirExists(sDirectory))
	{
		wxLogError("Specified directory doesn't exist: %s", sDirectory);
		return false;
	}

	// make sure the directory is empty
	wxDir cDirectory(sDirectory);
	if (!cDirectory.IsOpened())
	{
		wxLogError("Cannot open directory for traversal: %s", sDirectory);
		return false;
	}
	if (cDirectory.HasFiles() || cDirectory.HasSubDirs())
	{
		wxLogError("Unable to create a working copy in a folder that isn't empty: %s", sDirectory);
		return false;
	}

	// success
	this->msDirectory = sDirectory;
	return true;
}

vvDialog* vvCreateCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	this->mwDialog = new vvCreateDialog();
	if (!this->mwDialog->Create(wParent, this->msDirectory, *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

bool vvCreateCommand::CanHandleError(wxUint64 sgErrorCode) const
{
	if (sgErrorCode == SG_ERR_BRANCH_NEEDS_MERGE)
	{
		return true;
	}
	else
		return vvCommand::CanHandleError(sgErrorCode);
}

bool vvCreateCommand::TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode)
{
	if (sgErrorCode == SG_ERR_BRANCH_NEEDS_MERGE)
	{
		wxString   sRepo        = this->mwDialog->GetRepo();

		sRepo.Trim(true);
		sRepo.Trim(false);
		this->mwDialog->SetRepo(sRepo);
		return this->mwDialog->DisambiguateBranch();
	}
	else
		return vvCommand::TryToHandleError(pContext, parent, sgErrorCode);
}

bool vvCreateCommand::Execute()
{
	vvVerbs*   pVerbs       = this->GetVerbs();
	vvContext* pContext     = this->GetContext();
	wxString   sRepo        = this->mwDialog->GetRepo();
	wxString   sCloneSource = this->mwDialog->GetCloneSource();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	// trim the input strings
	sRepo.Trim(true);
	sRepo.Trim(false);
	sCloneSource.Trim(true);
	sCloneSource.Trim(false);

	// start an operation
	pContext->Log_PushOperation("Creating working copy", vvContext::LOG_FLAG_NONE);
	pContext->Log_SetSteps(2u);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);

	// create/clone the new repository
	pContext->Log_SetStep("Creating repository");
	switch (this->mwDialog->GetCloneAction())
	{
	case vvCreateDialog::CLONE_EMPTY:
		{
			wxString sSharedUsersRepo = this->mwDialog->GetSharedUsersRepo();
			sSharedUsersRepo.Trim(true);
			sSharedUsersRepo.Trim(false);

			if (!pVerbs->New(*pContext, sRepo, this->msDirectory, sSharedUsersRepo))
			{
				pContext->Error_ResetReport("Unable to initialize new repository.");
				return false;
			}
		}
		break;
	case vvCreateDialog::CLONE_LOCAL:
		{
			if (!pVerbs->Clone(*pContext, sCloneSource, sRepo, this->msUserName, this->msPassword))
			{
				//pContext->Error_ResetReport("Unable to clone repository: %s", sCloneSource);
				return false;
			}
		}
		break;
	case vvCreateDialog::CLONE_REMOTE:
		{
			this->SetRemoteRepo(sCloneSource);
			if (!pVerbs->Clone(*pContext, sCloneSource, sRepo, this->msUserName, this->msPassword))
			{
				//pContext->Error_ResetReport("Unable to clone repository: %s", sCloneSource);
				return false;
			}
			if (msUserName != wxEmptyString)
			{
				//They successfully cloned, and had to input a username.  Set the whoami
				pVerbs->SetCurrentUser(*pContext, sRepo, msUserName, false);
			}
		}
		break;
	}
	pContext->Log_FinishStep();

	// checkout a working copy from the repository
	pContext->Log_SetStep("Checking out repository");
	if (this->mwDialog->GetCloneAction() != vvCreateDialog::CLONE_EMPTY)
	{
		wxString sCheckoutTarget = this->mwDialog->GetCheckoutTarget();

		sCheckoutTarget.Trim(true);
		sCheckoutTarget.Trim(false);

		switch (this->mwDialog->GetCheckoutType())
		{
		case vvCreateDialog::CHECKOUT_BRANCH:
			if (!pVerbs->Checkout(*pContext, sRepo, this->msDirectory, vvVerbs::CHECKOUT_BRANCH, sCheckoutTarget))
			{
				if (pContext->Error_Check() && !CanHandleError(pContext->Get_Error_Code()))
				{
					pContext->Error_ResetReport("Unable to checkout branch: %s", sCheckoutTarget);
					return false;
				}
				else 
					return false;
			}
			break;
		case vvCreateDialog::CHECKOUT_CHANGESET:
			if (!pVerbs->Checkout(*pContext, sRepo, this->msDirectory, vvVerbs::CHECKOUT_CHANGESET, sCheckoutTarget))
			{
				pContext->Error_ResetReport("Unable to checkout changeset: %s", sCheckoutTarget);
				return false;
			}
			break;
		case vvCreateDialog::CHECKOUT_TAG:
			if (!pVerbs->Checkout(*pContext, sRepo, this->msDirectory, vvVerbs::CHECKOUT_TAG, sCheckoutTarget))
			{
				pContext->Error_ResetReport("Unable to checkout tag: %s", sCheckoutTarget);
				return false;
			}
			break;
		}

		wxString sBranchToAttachTo = mwDialog->GetBranchAttachment();
		if (!sBranchToAttachTo.IsEmpty())
		{
			if (!pVerbs->PerformBranchAttachmentAction(*pContext, this->msDirectory, vvVerbs::BRANCH_ACTION__ATTACH, sBranchToAttachTo))
			{
				pContext->Log_ReportError_CurrentError();
			}
		}
	}
	pContext->Log_FinishStep();

	return true;
}
