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
#include "vvResolveCommand.h"

#include "vvContext.h"
#include "vvResolveDialog.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvResolveCommand.
 */
static const vvResolveCommand gcResolveCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "resolve";
static const unsigned int guCommandFlags        = vvCommand::FLAG_NO_EXECUTE;
static const char*        gszCommandDescription = "View/resolve a conflict on an item.";


/*
**
** Public Functions
**
*/

vvResolveCommand::vvResolveCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
	, mwDialog(NULL)
{
}

bool vvResolveCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	// make sure we got a path
	if (cArguments.Count() < 1u)
	{
		wxLogError("No item specified to resolve.");
		return false;
	}

	// store the given data
	this->msPath = cArguments[0];

	// success
	return true;
}

bool vvResolveCommand::Init()
{
	// TODO: check that the item has conflicts
	if (!DetermineRepo__from_workingFolder(this->msPath))
		return false;
	return true;
}

vvDialog* vvResolveCommand::CreateDialog(
	wxWindow* wParent
	)
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	this->mwDialog = new vvResolveDialog();
	if (!this->mwDialog->Create(wParent, this->msPath, vvVerbs::RESOLVE_STATE_COUNT, *pVerbs, *pContext))
	{
		delete this->mwDialog;
		return NULL;
	}
	return this->mwDialog;
}
