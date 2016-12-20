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
#include "vvUpgradeCommand.h"

#include "vvContext.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/**
 * Global instance of vvUpgradeCommand.
 */
static const vvUpgradeCommand gcRevertCommand(vvCommand::FLAG_GLOBAL_INSTANCE);

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "upgrade";
static const unsigned int guCommandFlags        = vvCommand::FLAG_NO_DIALOG;
static const char*        gszCommandDescription = "Upgrade repositories to the new version";


/*
**
** Public Functions
**
*/

vvUpgradeCommand::vvUpgradeCommand(unsigned int extraFlags)
	: vvCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
{
}

bool vvUpgradeCommand::ParseArguments(
	const wxArrayString& cArguments
	)
{
	cArguments;
	//this command should not be called from the command line
	return false;
}

bool vvUpgradeCommand::Init()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);

	return true;
}

bool vvUpgradeCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pContext = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pContext != NULL);
	
	if (!pVerbs->Upgrade(*pContext))
	{
		pContext->Log_ReportError_CurrentError();
	}

	return !pContext->Error_Check();
}
