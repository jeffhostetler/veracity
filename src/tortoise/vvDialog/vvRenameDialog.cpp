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
#include "vvRenameDialog.h"

#include "vvVerbs.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvDialogHeaderControl.h"

/*
**
** Public Functions
**
*/

bool vvRenameDialog::Create(
	wxWindow*            pParent,
	const wxString&      sInitialValue,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, sInitialValue, cVerbs, cContext));
	
	// store the given initialization data
	this->msNewName      = sInitialValue;

	wxStaticText * staticLabel                = XRCCTRL(*this, "label1",                 wxStaticText);
	this->mwTextEntry						  = XRCCTRL(*this, "text1",                 wxTextCtrl);

	wxString currentLabel = staticLabel->GetLabelText();
	
	
	wxFileName fileName = wxFileName::FileName(sInitialValue);
	wxString justTheFileName = fileName.GetFullName();
	
	
	currentLabel.Printf(currentLabel, justTheFileName);
	staticLabel->SetLabelText(currentLabel);
	this->mwTextEntry->AppendText(justTheFileName);
	//this->Fit();
	staticLabel->Wrap(450);
	this->Fit();
	// success
	return true;
}

bool vvRenameDialog::TransferDataToWindow()
{ 
	// success
	return true;
}

bool vvRenameDialog::TransferDataFromWindow()
{
	// success
	this->msNewName = this->mwTextEntry->GetValue();
	return true;
}

wxString vvRenameDialog::GetNewName()
{
	return this->msNewName;
}

IMPLEMENT_DYNAMIC_CLASS(vvRenameDialog, vvDialog);
BEGIN_EVENT_TABLE(vvRenameDialog, vvDialog)
END_EVENT_TABLE()
