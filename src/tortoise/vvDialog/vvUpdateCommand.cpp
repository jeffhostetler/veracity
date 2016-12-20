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
#include "vvUpdateCommand.h"

#include "vvContext.h"
#include "vvRevSpecDialog.h"
#include "vvVerbs.h"

/*
**
** Globals
**
*/

/**
 * Global instance of vvUpdateCommand.
 */
static const vvUpdateCommand			gcUpdateCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "update";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Update a repository object.";


/*
**
** Public Functions
**
*/

vvUpdateCommand::vvUpdateCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
	m_nbDetach.SetNull();
}

bool vvUpdateCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to update.");
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

vvDialog* vvUpdateCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;
	this->mwDialog = new vvRevSpecDialog();
	if (!DetermineRepo__from_workingFolder(mcPaths))
		return false;

	pVerbs->GetBranch(*pContext, this->GetRepoLocator().GetWorkingFolder(), m_sBranch);

	vvRevSpec initialSelection(*pContext);
	if (!m_sBranch.IsEmpty())
		initialSelection.AddBranch(*pContext, m_sBranch);
	//else
	//	initialSelection.AddRevision(*pContext, "");
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator(), *pVerbs, *pContext, initialSelection, "Update Working Copy"))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}
void vvUpdateCommand::SetExecuteData(vvRepoLocator cRepoLocator, vvRevSpec cRevSpec)
{
	SetRepoLocator(cRepoLocator);
	m_cRevSpec				 = cRevSpec;
}

bool vvUpdateCommand::CanHandleError(wxUint64 sgErrorCode) const
{
	if (sgErrorCode == SG_ERR_MUST_ATTACH_OR_DETACH)
	{
		return true;
	}
	else
		return vvCommand::CanHandleError(sgErrorCode);
}

bool vvUpdateCommand::TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode)
{
	if (sgErrorCode == SG_ERR_MUST_ATTACH_OR_DETACH)
	{
		if (m_sBranch.IsEmpty())
			this->GetVerbs()->GetBranch(*pContext, this->GetRepoLocator().GetWorkingFolder(), m_sBranch);

		wxString sMessage = wxString::Format("You are currently attached to branch \"%s\", but you \nare updating to a revision that isn't a branch head. \nWhat would you like to do?", m_sBranch);
		wxMessageDialog md(parent, sMessage, "Detach Working Copy?", wxYES_NO | wxCANCEL | wxCENTRE);

		md.SetYesNoLabels("Detach", "Stay Attached");
		int result = md.ShowModal();
		if (result == wxID_YES)
		{
			m_nbDetach = true;
			return true;
		}
		else if (result == wxID_NO)
		{
			m_nbDetach = false;
			return true;
		}
	}
	else
		return vvCommand::TryToHandleError(pContext, parent, sgErrorCode);
	return false;
}

bool vvUpdateCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	pContext->Log_PushOperation("Updating", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);

	if (this->mwDialog != NULL)
	{
		this->SetExecuteData(this->GetRepoLocator(), this->mwDialog->GetRevSpec());
	}

	if (!pVerbs->Update(*pContext, this->GetRepoLocator().GetWorkingFolder(), m_cRevSpec.GetUpdateBranch(), &m_cRevSpec, m_nbDetach))
	{
		if (!CanHandleError(pContext->Get_Error_Code()))
			pContext->Log_ReportError_CurrentError();
	}

	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
