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
#include "vvRevertCommand.h"

#include "vvContext.h"
#include "vvRemoveDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvRevertCommand.
 */
static const vvRevertCommand gcRevertCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "revert";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Revert a change inside a working copy.";


/*
**
** Public Functions
**
*/

vvRevertCommand::vvRevertCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvRevertCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to revert.");
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

bool vvRevertCommand::Init()
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
	m_bSaveBackups = true;
	if (!pVerbs->GetSetting_Revert__Save_Backups(*pContext, &m_bSaveBackups))
	{
		pContext->Error_ResetReport("Could not get setting for revert");
		return false;
	}

	return true;
}

vvDialog* vvRevertCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;
	this->mwDialog = new vvRevertDialog();
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator(), this->mcRepoPaths, m_bSaveBackups, *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

void vvRevertCommand::SetExecuteData(vvRepoLocator cRepoLocator, bool bAll, wxArrayString * pathsToRevert, bool bSaveBackups)
{
	SetRepoLocator(cRepoLocator);
	m_bAll = bAll;
	if (pathsToRevert != NULL)
		m_dialogResult_cSelectedFiles = *pathsToRevert;
	m_dialogResult_bSaveBackups = bSaveBackups;
}

bool vvRevertCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	if (mwDialog != NULL)
	{
		bool bAll = false;
		wxArrayString selectedFiles = mwDialog->GetSelectedPaths(&bAll);
		SetExecuteData(this->GetRepoLocator(), bAll, &selectedFiles, mwDialog->GetSaveBackups());
	}

	pContext->Log_PushOperation("Reverting object", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);

	if (!pVerbs->Revert(*pContext, this->GetRepoLocator().GetWorkingFolder(), m_bAll ? NULL : &m_dialogResult_cSelectedFiles, !m_dialogResult_bSaveBackups))
	{
		pContext->Log_ReportError_CurrentError();
	}

	if (!pContext->Error_Check() && m_dialogResult_cSelectedFiles.Count() > 0)
	{
		wxArrayString cDiskPaths;
		if (!pVerbs->GetAbsolutePaths(*pContext, this->GetRepoLocator().GetWorkingFolder(), m_dialogResult_cSelectedFiles, cDiskPaths, false))
		{
			if (pContext->Error_Check())
				pContext->Log_ReportVerbose_CurrentError();
			pContext->Error_Reset(); //Ignore any errors in refreshing the wc.
		}
		pVerbs->RefreshWC(*pContext, this->GetRepoLocator().GetWorkingFolder(), &cDiskPaths);
	}
	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
