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
#include "vvServeCommand.h"

#include "vvContext.h"
#include "vvServeDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvServeCommand.
 */
static const vvServeCommand gcServeCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "serve";
static const unsigned int guCommandFlags        = vvCommand::FLAG_NO_EXECUTE;
static const char*        gszCommandDescription = "Start a web server.";


/*
**
** Public Functions
**
*/

vvServeCommand::vvServeCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvServeCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// get the directory name from the first argument
	if (cArguments.Count() < 1u)
	{
		wxLogError("No argument was passed in to serve.");
		return false;
	}

	this->msPath = cArguments[0];
	
	// success
	return true;
}

bool vvServeCommand::Init()
{
	bGotRepo = false;
	wxString sFolder, sRepoName;
	if (DetermineRepo__from_workingFolder(this->msPath, false))
		bGotRepo = true;
	
	if (bGotRepo == false)
	{
		wxArrayString aRepoNames;
		this->GetContext()->Error_Reset();
		//We didn't get get a valid repo.  Check all repos, just to make sure that 
		//there are not any that need to be upgraded.
		
		this->GetVerbs()->Upgrade__listOldRepos(*this->GetContext(), aRepoNames);
		if (aRepoNames.Count() > 0)
			return PromptForUpgrade(NULL);
	}
	
	return true;
}

vvDialog* vvServeCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	wParent;
	this->mwDialog = new vvServeDialog();
	if (!this->mwDialog->Create(wParent, bGotRepo ? &m_cRepoLocator : NULL, *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}

