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
#include "vvUpgradeConfirmationDialog.h"

/*
**
** Public Functions
**
*/

vvUpgradeConfirmationDialog::vvUpgradeConfirmationDialog()
{
}

vvUpgradeConfirmationDialog::vvUpgradeConfirmationDialog(
	wxWindow*         pParent,
	const wxArrayString aAllReposToUpgrade
	)
{
	this->Create(pParent, aAllReposToUpgrade);
}

bool vvUpgradeConfirmationDialog::Create(
	wxWindow*         pParent,
	const wxArrayString aAllReposToUpgrade
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, NULL, NULL, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// get our widgets
	wxListBox * wListBox = XRCCTRL(*this, "wListBox",  wxListBox);
	
	wListBox->Append(aAllReposToUpgrade);
	//this->mwMessage->SetBackgroundColour(this->GetBackgroundColour());

	// success
	return true;
}

bool vvUpgradeConfirmationDialog::TransferDataToWindow()
{
	return true;
}


IMPLEMENT_DYNAMIC_CLASS(vvUpgradeConfirmationDialog, vvDialog);

BEGIN_EVENT_TABLE(vvUpgradeConfirmationDialog, vvDialog)
END_EVENT_TABLE()
