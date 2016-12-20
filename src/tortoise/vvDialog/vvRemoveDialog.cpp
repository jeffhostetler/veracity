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
#include "vvRemoveDialog.h"

#include "vvVerbs.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"


/*
**
** Globals
**
*/
static const wxString gsSingleItem = "Please select the new parent for %s:";
static const wxString gsMultipleItems = "Please select the new parent for the %d items to remove:";


/*
**
** Public Functions
**
*/

bool vvRemoveDialog::Create(
	wxWindow*            pParent,
	const wxArrayString&      cPathsToRemove,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	mcPaths = cPathsToRemove;
	mwListBox             = XRCCTRL(*this, "listBox",                 wxListBox);
	
	// This should hide the .sgdrawer directory, 
	//if we go back to marking them as hidden.

	//Get the parent path of the object that they are moving.	
	wxFileName fileName = wxFileName::FileName(cPathsToRemove[0]);
	fileName.MakeAbsolute();
	wxString parentDir = fileName.GetPath();

	this->Fit();
	// success
	return true;
}

bool vvRemoveDialog::TransferDataToWindow()
{ 
	for (wxArrayString::const_iterator it = mcPaths.begin(); it != mcPaths.end(); ++it)
	{
		mwListBox->AppendString(*it);
	}
	return true;
}

bool vvRemoveDialog::TransferDataFromWindow()
{
	// success
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(vvRemoveDialog, vvDialog);
BEGIN_EVENT_TABLE(vvRemoveDialog, vvDialog)
END_EVENT_TABLE()
