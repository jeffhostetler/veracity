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
#include "vvLoginDialog.h"

#include "vvVerbs.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvDialogHeaderControl.h"

/*
**
** Public Functions
**
*/

bool vvLoginDialog::Create(
	wxWindow*            pParent,
	vvNullable<vvRepoLocator> cRepoLocator,
	const wxString&      sInitialValue_User,
	const wxString&		 sRemoteRepo,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	vvDialogHeaderControl * header = new vvDialogHeaderControl(this, wxID_ANY, cRepoLocator.IsNull() ? wxEmptyString : cRepoLocator.GetValue().GetRepoName(), cVerbs, cContext);
	wxXmlResource::Get()->AttachUnknownControl("wHeader", header);
	header->AddTextValue("Remote Repository:", sRemoteRepo);
	// store the given initialization data
	this->msUserName      = sInitialValue_User;
	this->mcRepoLocator = cRepoLocator;

	this->mwComboBox_User					  = XRCCTRL(*this, "textUsername",                 wxComboBox);
	this->mwTextEntry_Password					  = XRCCTRL(*this, "textPassword",                 wxTextCtrl);
	this->mwCheckBox_Remember					  = XRCCTRL(*this, "checkRememberPassword",                 wxCheckBox);

	FitHeight();
	// success
	return true;
}

void vvLoginDialog::FitHeight()
{
	wxSize minSize = this->GetMinSize();
	Layout();
	wxSize size = this->GetSize();
	size.SetHeight(-1);
	this->SetMinSize(size);
	this->Fit();
	this->SetMinSize(minSize);
	this->Refresh();	
}

bool vvLoginDialog::TransferDataToWindow()
{ 
	if (msUserName.IsEmpty() && !mcRepoLocator.IsNull())
	{
		this->GetVerbs()->GetCurrentUser(*this->GetContext(), mcRepoLocator.GetValue().GetRepoName(), msUserName);
		wxArrayString aUsers;
		this->GetVerbs()->GetUsers(*this->GetContext(), mcRepoLocator.GetValue().GetRepoName(), aUsers); 
		this->mwComboBox_User->Append(aUsers);
	}
	
	this->mwComboBox_User->SetValue(msUserName);
	if (msUserName.IsEmpty())
		this->mwComboBox_User->SetFocus();
	else
		this->mwTextEntry_Password->SetFocus();

	return true;
}

bool vvLoginDialog::TransferDataFromWindow()
{
	// success
	this->msUserName = this->mwComboBox_User->GetValue();
	this->msPassword = this->mwTextEntry_Password->GetValue();
	this->mbRememberPassword = this->mwCheckBox_Remember->GetValue();
	return true;
}

wxString vvLoginDialog::GetUserName()
{
	return this->msUserName;
}

wxString vvLoginDialog::GetPassword()
{
	return this->msPassword;
}

bool vvLoginDialog::GetRememberPassword()
{
	return this->mbRememberPassword;
}

IMPLEMENT_DYNAMIC_CLASS(vvLoginDialog, vvDialog);
BEGIN_EVENT_TABLE(vvLoginDialog, vvDialog)
END_EVENT_TABLE()
