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
#include "vvSettingsCommand.h"

#include "vvSettingsDialog.h"
#include "vvContext.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvSettingsCommand.
 */
static const vvSettingsCommand gcSettingsCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "settings";
static const unsigned int guCommandFlags        = vvCommand::FLAG_NO_EXECUTE;
static const char*        gszCommandDescription = "View and manipulate Tortoise configuration settings.";


/*
**
** Public Functions
**
*/

vvSettingsCommand::vvSettingsCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvSettingsCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// make sure we got at least one path
	if (cArguments.Count() > 0u)
	{
		this->msPath = cArguments[0u];
	}

	// success
	return true;
}

bool vvSettingsCommand::Init()
{
	bool returnFromDetermineRepo = true;
	if (!this->msPath.IsEmpty())
	{
		returnFromDetermineRepo = DetermineRepo__from_workingFolder(this->msPath, false);
		msRepo = this->GetRepoLocator().GetRepoName();
	}
	//If there is a repo, but the return value was still false, they canceled the upgrade prompt.
	if (!msRepo.IsEmpty() && !returnFromDetermineRepo)
		return false;
	return true;
}

vvDialog* vvSettingsCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	this->mwDialog = new vvSettingsDialog();
	if (!this->mwDialog->Create(wParent, *pVerbs, *pContext, this->GetRepoLocator()))
	{
		pContext->Log_ReportError_CurrentError();
		pContext->Error_Reset();
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}
