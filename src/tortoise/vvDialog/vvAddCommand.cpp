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
#include "vvAddCommand.h"

#include "vvContext.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvAddCommand.
 */
static const vvAddCommand gcAddCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "add";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "Add a repository object.";


/*
**
** Public Functions
**
*/

vvAddCommand::vvAddCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription),
	mwDialog(NULL)
{
}

bool vvAddCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to Add.");
		return false;
	}

	// store the given paths
	for (wxArrayString::const_iterator it = cArguments.begin(); it != cArguments.end(); ++it)
	{
		this->mcDiskPaths.Add(*it);
	}
	// success
	return true;
}

bool vvAddCommand::Init()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	if (!DetermineRepo__from_workingFolder(this->mcDiskPaths))
		return false;

	msBaseFolder = this->GetRepoLocator().GetWorkingFolder();

	// convert our absolute paths to repo paths
	if (!pVerbs->GetRepoPaths(*pContext, this->mcDiskPaths, this->GetRepoLocator().GetWorkingFolder(), this->mcRepoPaths))
	{
		pContext->Error_ResetReport("Failed to translate the given paths into repo paths relative to working copy: %s", msBaseFolder);
		return false;
	}
	wxASSERT(this->mcDiskPaths.size() == this->mcRepoPaths.size());

	return true;
}

vvDialog* vvAddCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;
	this->mwDialog = new vvAddDialog();
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator(), this->mcRepoPaths, *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

void vvAddCommand::SetExecuteData(const wxString& sBaseFolder, const wxArrayString& repoPathsToAdd)
{
	wxArrayString cDiskPaths;
	//Convert the repo paths to disk paths.
	this->GetVerbs()->GetAbsolutePaths(*this->GetContext(), sBaseFolder, repoPathsToAdd, cDiskPaths);
	mcDiskPaths = cDiskPaths;
}

bool vvAddCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	
	bool bAll = false;
	if (mwDialog != NULL)
	{
		SetExecuteData(msBaseFolder, mwDialog->GetSelectedPaths(&bAll));
	}

	if (this->mcDiskPaths.Count() > 1)
		pContext->Log_PushOperation("Adding objects", vvContext::LOG_FLAG_NONE);
	else
		pContext->Log_PushOperation("Adding object", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);
	if (bAll)
	{
		wxArrayString arrOfOnePath;
		//If all are selected, it's more efficient to let a recursive scan find them
		//than to add them individually.
		arrOfOnePath.Add(msBaseFolder);
		pVerbs->Add(*pContext, arrOfOnePath);
	}
	else
		pVerbs->Add(*pContext, this->mcDiskPaths);

	if (!pContext->Error_Check())
	{
		pVerbs->RefreshWC(*pContext, this->GetRepoLocator().GetWorkingFolder(), &mcDiskPaths);
	}
	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
