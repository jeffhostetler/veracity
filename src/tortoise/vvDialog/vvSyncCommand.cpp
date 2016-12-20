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
#include "vvSyncCommand.h"

#include "vvContext.h"
#include "vvSyncDialog.h"
#include "vvVerbs.h"
#include "vvLoginDialog.h"
#include "vvRepoLocator.h"
#include "vvMergeCommand.h"
#include "vvProgressExecutor.h"
#include "sg.h"

/*
**
** Globals
**
*/

/**
 * Global instance of vvSyncCommand.
 */
static const vvSyncCommand			gcSyncCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "sync";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Synchronize changes between repository instances.";


/*
**
** Public Functions
**
*/

vvSyncCommand::vvSyncCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
	, m_bPushCloneIsOK(false)
	, m_bMerged(false)
{
}

bool vvSyncCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to sync.");
		return false;
	}
	wxString sItem = cArguments[0];

	// make sure this is actually an existing directory
	if ( ! (wxFileExists(sItem) || wxDirExists(sItem)) )
	{
		wxLogError("Specified item doesn't exist: %s", sItem);
		return false;
	}

	// success
	this->msDirectory = sItem;
	return true;
}

bool vvSyncCommand::Init()
{
	if (!DetermineRepo__from_workingFolder(this->msDirectory))
		return false;
	return true;
}

vvDialog* vvSyncCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;
	this->mwDialog = new vvSyncDialog();
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator().GetWorkingFolder(), *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

bool vvSyncCommand::CanHandleError(wxUint64 sgErrorCode) const
{
	if ((m_selectedAction == vvSyncDialog::SYNC_PUSH && sgErrorCode == SG_ERR_NOTAREPOSITORY))
	{
		return true;
	}
	else if (sgErrorCode == SG_ERR_BRANCH_NEEDS_MERGE)
	{
		return true;
	}
	else
		return vvRepoCommand::CanHandleError(sgErrorCode);
}

bool vvSyncCommand::TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode)
{
	if (sgErrorCode == SG_ERR_NOTAREPOSITORY)
	{
		wxString sRepoName = m_sOtherRepo.AfterLast('/');
		wxMessageDialog wmd(parent, "There is not a repository named \"" + sRepoName + "\" on the remote server.  Would you like to clone this repository onto the remote server?", "Confirm Remote Clone", wxYES_NO | wxNO_DEFAULT | wxCENTRE);
		if (wmd.ShowModal() == wxID_YES)
		{
			m_bPushCloneIsOK = true;
			return true;
		}
	}
	else if (sgErrorCode == SG_ERR_BRANCH_NEEDS_MERGE)
	{
		vvVerbs*   pVerbs   = this->GetVerbs();
		wxString sBranch;
		if (pVerbs->GetBranch(*pContext, this->GetRepoLocator().GetWorkingFolder(), sBranch))
		{
			wxMessageDialog wmd(parent, "The branch \"" + sBranch + "\" has multiple heads.  Would you like to merge it? Cancelling will leave your working copy unchanged. ", "Attempt Merge?", wxYES_NO | wxYES_DEFAULT | wxCENTRE);
			wmd.SetYesNoLabels("Merge", "Cancel");
			if (wmd.ShowModal() == wxID_YES)
			{
				vvMergeCommand command = vvMergeCommand();		
				command.SetVerbs(pVerbs);
				vvRevSpec cRevSpec(*pContext);
				cRevSpec.AddBranch(*pContext, sBranch);
				command.SetExecuteData(*pContext, m_cRepoLocator, cRevSpec);
				vvProgressExecutor exec = vvProgressExecutor(command);
				exec.Execute(parent);
			}
			m_bMerged = true;
			return true;
		}
	}
	else
		return vvRepoCommand::TryToHandleError(pContext, parent, sgErrorCode);
	return false;
}

bool vvSyncCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	if (m_bMerged == true)
		return true;

	m_selectedAction = this->mwDialog->GetSyncAction();
	m_sOtherRepo = this->mwDialog->GetOtherRepo();
	this->SetRemoteRepo(m_sOtherRepo);
	//There's no need to start an operation here, since one will be started inside sglib.
	if (m_selectedAction == vvSyncDialog::SYNC_PULL || m_selectedAction == vvSyncDialog::SYNC_PULL_THEN_PUSH)
	{
		if (!pVerbs->Pull(*pContext, this->mwDialog->GetOtherRepo(), this->mwDialog->GetMyRepoName(), 
			this->mwDialog->GetAreas(), wxArrayString(), this->mwDialog->GetChangesets(), wxArrayString(), this->msUserName, this->msPassword))
		{
			if (!CanHandleError(pContext->Get_Error_Code()))
				pContext->Log_ReportError_CurrentError();
			return false; //Don't try to push.
		}
		else
			pVerbs->RefreshWC(*pContext, this->GetRepoLocator().GetWorkingFolder());
	}
	
	if (!pContext->Error_Check() && (m_selectedAction == vvSyncDialog::SYNC_PUSH || m_selectedAction == vvSyncDialog::SYNC_PULL_THEN_PUSH))
	{
		if (!m_bPushCloneIsOK && !pVerbs->Push(*pContext, this->mwDialog->GetOtherRepo(), this->mwDialog->GetMyRepoName(), 
			this->mwDialog->GetAreas(), wxArrayString(), this->mwDialog->GetChangesets(), wxArrayString(), this->msUserName, this->msPassword))
		{
			if (!CanHandleError(pContext->Get_Error_Code()))
				pContext->Log_ReportError_CurrentError();
			else
			{
				//This is a hack.  I admit to that.
				//It is possible that Push might have failed before starting an operation.
				//Starting and closing this dummy operation will 
				//trick the progress dialog into enabling the close button. --jeremy
				pContext->Log_PushOperation("Pushing", vvContext::LOG_FLAG_NONE);
				pContext->Log_PopOperation();
			}
		}
		else if (m_bPushCloneIsOK && !pVerbs->Clone(*pContext, this->mwDialog->GetMyRepoName(), this->mwDialog->GetOtherRepo(),  
			this->msUserName, this->msPassword))
		{
			if (!CanHandleError(pContext->Get_Error_Code()))
				pContext->Log_ReportError_CurrentError();
		}

	}

	if (!pContext->Error_Check() && m_selectedAction == vvSyncDialog::SYNC_PULL && this->mwDialog->GetUpdateAfterPull())
	{
		if (!pVerbs->Update(*pContext, this->msDirectory, wxEmptyString, NULL, false))
		{
			if (!CanHandleError(pContext->Get_Error_Code()))
				pContext->Log_ReportError_CurrentError();
		}
	}
	return !pContext->Error_Check();
}
