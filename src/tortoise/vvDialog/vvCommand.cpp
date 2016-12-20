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
#include "vvCommand.h"
#include "vvLoginDialog.h"
#include "vvUpgradeCommand.h"
#include "vvProgressExecutor.h"
#include "vvUpgradeConfirmationDialog.h"

#include "sg.h"


/*
**
** Globals
**
*/

vvCommand* vvCommand::spFirst = NULL;


/*
**
** Public Functions
**
*/

vvCommand* vvCommand::GetFirst()
{
	return vvCommand::spFirst;
}

vvCommand* vvCommand::FindByName(
	const wxString& sName
	)
{
	vvCommand* pCurrent = vvCommand::GetFirst();
	while (pCurrent != NULL)
	{
		if (pCurrent->GetName().compare(sName) == 0)
		{
			return pCurrent;
		}
		pCurrent = pCurrent->GetNext();
	}
	return NULL;
}

vvCommand::vvCommand(
	const wxString& sName,
	unsigned int    uFlags,
	const wxString& sUsageDescription
	)
	: vvHasFlags(uFlags)
	, msName(sName)
	, msUsageDescription(sUsageDescription)
	, mpVerbs(NULL)
	, mpContext(NULL)
	, mbAlreadyLoadedPassword(false)
{
	if (HasAnyFlag(vvCommand::FLAG_GLOBAL_INSTANCE))
	{
		mpNext = vvCommand::spFirst; // we'll point to the current first instance
		// set ourselves as the new first instance in the global list
		vvCommand::spFirst = this;
	}
}

const wxString& vvCommand::GetName() const
{
	return this->msName;
}

const wxString& vvCommand::GetUsageDescription() const
{
	return this->msUsageDescription;
}

vvCommand* vvCommand::GetNext() const
{
	return this->mpNext;
}

void vvCommand::SetVerbs(
	class vvVerbs* pVerbs
	)
{
	this->mpVerbs = pVerbs;
}

class vvVerbs* vvCommand::GetVerbs() const
{
	return this->mpVerbs;
}

void vvCommand::SetContext(
	class vvContext* pContext
	)
{
	this->mpContext = pContext;
}

class vvContext* vvCommand::GetContext() const
{
	return this->mpContext;
}

bool vvCommand::ParseArguments(
	const wxArrayString& WXUNUSED(cArguments)
	)
{
	return true;
}

bool vvCommand::Init()
{
	return true;
}

class vvDialog* vvCommand::CreateDialog(
	wxWindow* WXUNUSED(wParent)
	)
{
	return NULL;
}

bool vvCommand::Execute()
{
	return true;
}

bool vvCommand::GetNeedToSavePassword() const
{
	return mbRememberPassword;
}

bool vvCommand::CanHandleError(wxUint64 sgErrorCode) const
{
	//This has to be here, so that the Create command can prompt.
	return sgErrorCode == SG_ERR_AUTHORIZATION_REQUIRED;
}

bool vvCommand::TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode)
{
	//This has to be here, so that the Create command can prompt.
	if (sgErrorCode == SG_ERR_AUTHORIZATION_REQUIRED)
	{
		return PromptForLoginCredentials(parent, vvNULL);
	}
	return false;
}

bool vvCommand::LoadRememberedPassword(vvNullable<vvRepoLocator> cRepoLocator)
{
	wxString remoteRepo = GetRemoteRepo();
	//We can't load the password without a username, we can't get a username without
	//a repo.
	if (cRepoLocator.IsNull())
		return false;

	this->GetVerbs()->GetCurrentUser(*this->GetContext(), cRepoLocator.GetValue().GetRepoName(), msUserName);

	if (this->GetVerbs()->GetSavedPassword(*this->GetContext(), this->GetRemoteRepo(), msUserName, msPassword))
	{
		mbAlreadyLoadedPassword = true;
		return true;
	}
	return false;
}

bool vvCommand::SaveRememberedPassword()
{
	wxString remoteRepo = GetRemoteRepo();
	//We can't save the password without these things.
	if (this->GetRemoteRepo().IsEmpty() || msUserName.IsEmpty() || msPassword.IsEmpty())
		return false;
	
	if (this->GetVerbs()->SetSavedPassword(*this->GetContext(), this->GetRemoteRepo(), msUserName, msPassword))
	{
		return true;
	}
	return false;
}

bool vvCommand::PromptForUpgrade(wxWindow * parent)
{
	wxArrayString aOldRepos;
	if ( !this->GetVerbs()->Upgrade__listOldRepos(*this->GetContext(), aOldRepos) )
	{
		this->GetContext()->Log_ReportError_CurrentError();
		return false;
	}
	vvUpgradeConfirmationDialog dlg(parent, aOldRepos);
	if (dlg.ShowModal() == wxID_OK)
	{
		//The dialog wants us to upgrade all repos.
		vvUpgradeCommand command = vvUpgradeCommand();		
		command.SetVerbs(this->GetVerbs());
		
		vvProgressExecutor exec = vvProgressExecutor(command);
		return exec.Execute(parent);
	}
	//Abandon this command, and exit.
	return false;
}

bool vvCommand::PromptForLoginCredentials(wxWindow * parent, vvNullable<vvRepoLocator> cRepoLocator)
{
	vvLoginDialog ld;
	ld.Create(parent, cRepoLocator, msUserName, this->GetRemoteRepo(), *this->GetVerbs(), *this->GetContext());
	if (ld.ShowModal() == wxID_OK)
	{
		msUserName = ld.GetUserName();
		msPassword = ld.GetPassword();
		mbRememberPassword = ld.GetRememberPassword();
		return true;
	}
	return false;
}
