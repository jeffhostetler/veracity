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
#include "vvLockDialog.h"

#include "vvVerbs.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvDialogHeaderControl.h"
#include "vvCurrentUserControl.h"

/*
**
** Globals
**
*/
static const wxString gsLockSingleItem = "Are you sure that you want to lock the following file?";
static const wxString gsLockMultipleItems = "Are you sure that you want to lock the following files?";
static const wxString gsUnlockSingleItem = "Are you sure that you want to unlock the following file?";
static const wxString gsUnlockMultipleItems = "Are you sure that you want to unlock the following files?";

static const wxString gszUnlockModeTitle = "Unlock files";
static const wxString gszLockModeTitle = "Lock files";

/**
 * String displayed at the top of the validation error message box.
 */
static const wxString gsValidationErrorMessage = "There was an error in the information specified.  Please correct the following problems and try again.";


static const wxString gsValidationField_User           = "User";

static const wxString gsValidationField_Server         = "Server";

/*
**
** Public Functions
**
*/

bool vvLockDialog::Create(
	wxWindow*            pParent,
	vvRepoLocator repoLocator,
	bool bUnlockMode,
	const wxArrayString&      sLockPaths,
	unsigned int		 nCountItemsToLock,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	this->mcRepoLocator = repoLocator;
	
	wxStaticText * staticLabel                = XRCCTRL(*this, "label1",                 wxStaticText);
	this->mwListBox							  = XRCCTRL(*this, "listbox1",                 wxListBox);
	this->mwSyncTarget = new vvSyncTargetsControl(this, wxID_ANY);
	vvDialogHeaderControl* wHeader = new vvDialogHeaderControl(this, wxID_ANY, repoLocator.GetWorkingFolder(), cVerbs, cContext);
	
	// create our user control for the header
	this->mwUser = new vvCurrentUserControl(wHeader, wxID_ANY, &mcRepoLocator, &cVerbs, &cContext);
	wHeader->AddControlValue("User:", this->mwUser, true);
	
	wxXmlResource::Get()->AttachUnknownControl("wHeader", wHeader);

	wxXmlResource::Get()->AttachUnknownControl("wSyncTarget", this->mwSyncTarget);

	this->mwListBox->Append(sLockPaths);
	

	wxString currentLabel = staticLabel->GetLabelText();
	if (nCountItemsToLock == 1)
	{
		currentLabel = bUnlockMode ? gsUnlockSingleItem : gsLockSingleItem;
	}
	else
	{
		currentLabel = bUnlockMode ? gsUnlockMultipleItems: gsLockMultipleItems;
	}

	if (bUnlockMode)
		this->SetTitle(gszUnlockModeTitle);
	else
		this->SetTitle(gszLockModeTitle);
	staticLabel->SetLabelText(currentLabel);
	if (bUnlockMode)
		XRCCTRL(*this, "wxID_OK",                 wxButton)->SetLabel("Unlock");
	
	this->mcValidatorReporter.SetHelpMessage(gsValidationErrorMessage);
	this->mwUser->SetValidator(vvValidator(gsValidationField_User)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);

	this->mwSyncTarget->SetValidator(vvValidator(gsValidationField_Server)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);
	this->Fit();
	// success
	return true;
}

bool vvLockDialog::TransferDataToWindow()
{ 
	this->mwSyncTarget->Populate(*this->GetContext(), *this->GetVerbs(), mcRepoLocator.GetRepoName());
	// success
	return true;
}

bool vvLockDialog::TransferDataFromWindow()
{
	this->msSyncTarget = this->mwSyncTarget->GetValue();
	// success
	return true;
}

wxString vvLockDialog::GetLockServer() const
{
	return msSyncTarget;
}

IMPLEMENT_DYNAMIC_CLASS(vvLockDialog, vvDialog);
BEGIN_EVENT_TABLE(vvLockDialog, vvDialog)
END_EVENT_TABLE()
