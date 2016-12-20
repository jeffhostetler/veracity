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
#include "vvMoveDialog.h"

#include "vvVerbs.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvDialogHeaderControl.h"

/*
**
** Globals
**
*/
static const wxString gsSingleItem = "Please select the new parent for %s:";
static const wxString gsMultipleItems = "Please select the new parent for the %d items to move:";


/*
**
** Public Functions
**
*/

bool vvMoveDialog::Create(
	wxWindow*            pParent,
	const wxString&      sInitialValue,
	unsigned int		 nCountItemsToMove,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	
	// store the given initialization data
	this->msNewPath      = sInitialValue;

	wxStaticText * staticLabel                = XRCCTRL(*this, "label1",                 wxStaticText);
	this->mwDirControl						  = XRCCTRL(*this, "dir1",                 wxGenericDirCtrl);

	wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, sInitialValue, cVerbs, cContext));

	// This should hide the .sgdrawer directory, 
	//if we go back to marking them as hidden.
	this->mwDirControl->ShowHidden(false);

	//Get the parent path of the object that they are moving.	
	wxFileName fileName = wxFileName::FileName(sInitialValue);
	fileName.MakeAbsolute();
	wxString parentDir = fileName.GetPath();
	
	this->mwDirControl->SetPath(parentDir);
	wxString currentLabel = staticLabel->GetLabelText();
	if (nCountItemsToMove == 1)
	{
		currentLabel.Printf(gsSingleItem, fileName.GetFullName());
	}
	else
	{
		currentLabel.Printf(gsMultipleItems, nCountItemsToMove);
	}
	staticLabel->SetLabelText(currentLabel);
	//this->Fit();
	staticLabel->Wrap(450);
	this->Fit();
	// success
	return true;
}

bool vvMoveDialog::TransferDataToWindow()
{ 
	// success
	return true;
}

bool vvMoveDialog::TransferDataFromWindow()
{
	// success
	this->msNewPath = this->mwDirControl->GetPath();
	return true;
}

wxString vvMoveDialog::GetNewPath()
{
	return this->msNewPath;
}

IMPLEMENT_DYNAMIC_CLASS(vvMoveDialog, vvDialog);
BEGIN_EVENT_TABLE(vvMoveDialog, vvDialog)
END_EVENT_TABLE()
