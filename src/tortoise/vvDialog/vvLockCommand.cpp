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
#include "vvLockCommand.h"

#include "vvContext.h"
#include "vvLockDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvLockCommand.
 */
static const vvLockCommand gcLockCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "lock";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Lock a file.";


/*
**
** Public Functions
**
*/

vvLockCommand::vvLockCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
	, mbUnlockMode(false)
{
}

bool vvLockCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to lock.");
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

bool vvLockCommand::Init()
{
	if (!DetermineRepo__from_workingFolder(this->mcPaths))
		return false;

	for (wxArrayString::const_iterator it = mcPaths.begin(); it != mcPaths.end(); ++it)
	{
		if (!wxFile::Exists(*it))
		{
			this->GetContext()->Log_ReportError("Only files can be locked");
			return false;
		}
	}

	wxString sCurrentUser;
	if (!this->GetVerbs()->GetCurrentUser(*this->GetContext(), this->GetRepoLocator().GetRepoName(), sCurrentUser))
	{
		this->GetContext()->Error_ResetReport("No current user is not set.");
	}
	
	//Cross reference the gids with the locked items
	int lockedFileCount = 0;
	int unlockedFileCount = 0;
	for (wxArrayString::const_iterator it = mcPaths.begin(); it != mcPaths.end(); ++it)
	{
		vvVerbs::LockList ll;
		bool bLocked = false;
		
		if (!this->GetVerbs()->CheckFileForLock(*this->GetContext(), this->GetRepoLocator(), *it, ll))
		{
			this->GetContext()->Error_ResetReport("Error checking for an existing lock on " + *it);
			return false;
		}
		
		for (vvVerbs::LockList::const_iterator itLock = ll.begin(); 
			itLock != ll.end(); 
			++itLock)
		{
			if (itLock->sUsername != sCurrentUser)
			{
				wxMessageBox("The file " + *it + " is locked by: " + itLock->sUsername + ".  If you wish to forcibly remove the lock, use the command line client's unlock command with the --force parameter.", "File Already Locked");
				return false;
			}
			lockedFileCount++;
			bLocked = true;
		}
		if (bLocked == false)
			unlockedFileCount++;
	}
	if (lockedFileCount > 0 && unlockedFileCount > 0)
	{
		wxMessageBox("Some of the selected files are locked, and some are not.  Please select a set of files that are all locked, or all unlocked.", "Some Files are Already Locked");
		return false;
	}

	if (lockedFileCount > 0)
		mbUnlockMode = true;
	return true;
}

vvDialog* vvLockCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;

	this->mwDialog = new vvLockDialog();
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator(), mbUnlockMode, this->mcPaths, this->mcPaths.GetCount(), *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

bool vvLockCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	this->SetRemoteRepo(this->mwDialog->GetLockServer());
	wxString operationText = (mbUnlockMode ? "Unlocking file" : "Locking file");
	if (mcPaths.Count() > 1)
		operationText += "s";
	pContext->Log_PushOperation(operationText, vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);
	if (mbUnlockMode)
	{
		if (!pVerbs->Unlock(*pContext, GetRepoLocator(), this->mwDialog->GetLockServer(), false, this->msUserName, this->msPassword, mcPaths))
		{
			if (pContext->Error_Check() && !CanHandleError(pContext->Get_Error_Code()))
			{
				pContext->Log_ReportError_CurrentError();
			}
		}
	}
	else
	{
		if (!pVerbs->Lock(*pContext, GetRepoLocator(), this->mwDialog->GetLockServer(), this->msUserName, this->msPassword, mcPaths))
		{
			if (pContext->Error_Check() && !CanHandleError(pContext->Get_Error_Code()))
			{
				pContext->Log_ReportError_CurrentError();
			}
		}
	}

	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
