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
#include "vvDiffCommand.h"

#include "vvContext.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvDiffCommand.
 */
static const vvDiffCommand gcRevertCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "diff";
static const unsigned int guCommandFlags        = vvCommand::FLAG_NO_DIALOG | vvCommand::FLAG_NO_PROGRESS;
static const char*        gszCommandDescription = "Display differences between working copy and baseline.";


/*
**
** Public Functions
**
*/

vvDiffCommand::vvDiffCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
{
}

bool vvDiffCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to revert.");
		return false;
	}
	if (cArguments.Count() > 1u)
	{
		wxLogError("Only one file can be diffed at a time.");
		return false;
	}
	
	// store the given paths
	this->msDiffObject = cArguments[0];

	// success
	return true;
}

bool vvDiffCommand::Init()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	if (!DetermineRepo__from_workingFolder(msDiffObject))
		return false;

	// convert our absolute paths to repo paths
	if (!pVerbs->GetRepoPath(*pContext, msDiffObject, this->GetRepoLocator().GetWorkingFolder(), msRepoPath))
	{
		pContext->Error_ResetReport("Failed to translate the given paths into repo paths relative to working copy: %s", this->GetRepoLocator().GetWorkingFolder());
		return false;
	}

	return true;
}

void vvDiffCommand::SetExecuteData(vvRepoLocator cRepoLocator, wxString& sRepoPath, wxString pRev1, wxString pRev2)
{
	this->SetRepoLocator(cRepoLocator);
	msRepoPath = sRepoPath;
	msRev1 = pRev1;
	msRev2 = pRev2;
}

bool vvDiffCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wxArrayString paths;
	paths.Add(msRepoPath);
	
	wxString sGid;

	if (pVerbs->GetGid(*pContext, msRepoPath, sGid))
	{
		msRepoPath = "@" + sGid;
	}
	else
		pContext->Error_Reset();

	pContext->Log_PushOperation("Diffing", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pContext);
	wxUnusedVar(cPopOperation);

	if (!pVerbs->Diff(*pContext, this->GetRepoLocator(), &paths, false, msRev1.IsEmpty() ? NULL : &msRev1, msRev2.IsEmpty() ? NULL : &msRev2, NULL))
	{
		pContext->Log_ReportError_CurrentError();
	}

	pContext->Log_FinishStep();
	return !pContext->Error_Check();
}
