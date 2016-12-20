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
#include "vvInOutCommand.h"

#include "vvContext.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

/*
 * Command specification.
 * These need to be char* instead of wxString, because they're used by a static constructor.
 * And we can't be sure the wxStrings will be constructed before the static instance that uses them.
 */
static const char*        gszCommandName        = "inout";
static const unsigned int guCommandFlags        = 0;
static const char*        gszCommandDescription = "This command is only used internally.";


/*
**
** Public Functions
**
*/

vvInOutCommand::vvInOutCommand(unsigned int extraFlags)
	: vvRepoCommand(gszCommandName, guCommandFlags | extraFlags, gszCommandDescription)
{
}

vvInOutCommand::vvInOutCommand(bool bIncoming, wxString sLocalRepo, wxString sOtherRepo)
	: vvRepoCommand(gszCommandName, guCommandFlags, gszCommandDescription)
{
	mbIncoming = bIncoming;
	msLocalRepo = sLocalRepo;
	msOtherRepo = sOtherRepo;
	SetRemoteRepo(msOtherRepo);
}

bool vvInOutCommand::Execute()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext* pCtx = this->GetContext();

	wxASSERT(pVerbs != NULL);
	wxASSERT(pCtx != NULL);

	DetermineRepo__from_repoName(msLocalRepo);
	pCtx->Log_PushOperation(mbIncoming ? "Loading incoming revisions" : "Loading outgoing revisions", vvContext::LOG_FLAG_NONE);
	wxScopeGuard cPopOperation = wxMakeGuard(&vvContext::Log_PopOperation_Static, pCtx);
	wxUnusedVar(cPopOperation);
	if (this->mbIncoming)
	{
		pVerbs->Incoming(*pCtx, this->msOtherRepo, this->msLocalRepo, this->msUserName, this->msPassword, mResults);
	}
	else
	{
		pVerbs->Outgoing(*pCtx, this->msOtherRepo, this->msLocalRepo, this->msUserName, this->msPassword, mResults);
	}
	if (pCtx->Error_Check() && !CanHandleError(pCtx->Get_Error_Code()))
	{
		pCtx->Log_ReportError_CurrentError();
	}
	pCtx->Log_FinishStep();
	return !pCtx->Error_Check();
}

vvVerbs::HistoryData vvInOutCommand::GetResults()
{
	return mResults;
}

