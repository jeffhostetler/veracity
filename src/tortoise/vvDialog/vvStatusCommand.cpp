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
#include "vvStatusCommand.h"

#include "vvContext.h"
#include "vvStatusDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvStatusCommand.
 */
static const vvStatusCommand			gcStatusCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "status";
static const unsigned int guCommandFlags        = vvCommand::FLAG_NO_EXECUTE;
static const char*        gszCommandDescription = "Display the status of a working copy.";


/*
**
** Public Functions
**
*/

vvStatusCommand::vvStatusCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvStatusCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// make sure we got at least one path
	if (cArguments.Count() < 1u)
	{
		wxLogError("No path(s) specified to display the status of.");
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

bool vvStatusCommand::Init()
{
	if (!DetermineRepo__from_workingFolder(this->mcPaths))
		return false;
	return true;
}

vvDialog* vvStatusCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	this->mwDialog = new vvStatusDialog();
	if (!this->mwDialog->Create(wParent, this->GetRepoLocator(), *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}
