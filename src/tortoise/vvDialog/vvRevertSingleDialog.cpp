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
#include "vvRevertSingleDialog.h"

#include "vvVerbs.h"
#include "vvContext.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"


/*
**
** Globals
**
*/
static const wxString gsExplanation = "Are you sure that you want to revert all changes to %s?";

/*
**
** Public Functions
**
*/

bool vvRevertSingleDialog::Create(
	wxWindow*            pParent,
	const wxString&      sPathToRevert,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName(), false))
	{
		return false;
	}

	msPath = sPathToRevert;

	//Get the parent path of the object that they are moving.	
	wxFileName fileName = wxFileName::FileName(sPathToRevert);
	fileName.MakeAbsolute();
	wxString parentDir = fileName.GetPath();
	
	this->mwExplanation = new vvSelectableStaticText(this, wxID_ANY, wxString::Format(gsExplanation, sPathToRevert), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_NO_VSCROLL);
	wxXmlResource::Get()->AttachUnknownControl("wRevertSingle_Explanation", this->mwExplanation);
	//this->mwExplanation->SetBackgroundColour(this->GetThemeBackgroundColour());

	//this->Fit();
	// success
	return true;
}

bool vvRevertSingleDialog::TransferDataToWindow()
{ 
	bool bSaveBackups = true;
	if (!this->GetVerbs()->GetSetting_Revert__Save_Backups(*this->GetContext(), &bSaveBackups))
		this->GetContext()->Error_ResetReport("Unable to populate state data.");
	XRCCTRL(*this, "wSaveBackups", wxCheckBox)->SetValue(bSaveBackups);
	this->Fit();
	return true;
}

bool vvRevertSingleDialog::TransferDataFromWindow()
{
	// success
	this->mbSaveBackups = XRCCTRL(*this, "wSaveBackups", wxCheckBox)->GetValue();
	return true;
}

bool vvRevertSingleDialog::GetSaveBackups()
{
	return mbSaveBackups;
}

IMPLEMENT_DYNAMIC_CLASS(vvRevertSingleDialog, vvDialog);
BEGIN_EVENT_TABLE(vvRevertSingleDialog, vvDialog)
END_EVENT_TABLE()
