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
#include "vvDialogHeaderControl.h"
#include "vvRevertDialog.h"
#include "vvStatusControl.h"

#include "vvVerbs.h"
#include "vvContext.h"

/*
**
** Globals
**
*/

static const wxString gsValidationErrorMessage = "There was an error in the information specified.  Please correct the following problems and try again.";

static const wxString gsValidationField_Files          = "Files";

// style for the status control
const unsigned int guStatusControlStyle = vvStatusControl::STYLE_SELECT;

/*
**
** Public Functions
**
*/

bool vvRevertDialog::Create(
	wxWindow*            pParent,
	const vvRepoLocator& cRepoLocator,
	const wxArrayString&      cPathsToRemove,
	bool bSaveBackups,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}
	m_cRepoLocator = cRepoLocator;

	mcPaths = wxArrayString(cPathsToRemove);

	// create our header control
	wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, cRepoLocator.IsWorkingFolder() ? cRepoLocator.GetWorkingFolder() : cRepoLocator.GetRepoName(), cVerbs, cContext));

	// create our status control
	this->mwFiles = new vvStatusControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, guStatusControlStyle);
	//We keep vvStatusControl::STATUS_LOST, since you can revert lost files.
	this->mwFiles->SetDisplay(vvStatusControl::MODE_COLLAPSED, vvStatusControl::ITEM_DEFAULT, vvStatusControl::STATUS_DEFAULT & ~vvStatusControl::STATUS_FOUND & ~vvStatusControl::STATUS_LOCKED & ~vvStatusControl::STATUS_LOCK_CONFLICT);
	wxXmlResource::Get()->AttachUnknownControl("wStatus", this->mwFiles);

	XRCCTRL(*this, "wSaveBackups", wxCheckBox)->SetValue(bSaveBackups);

	this->mcValidatorReporter.SetHelpMessage(gsValidationErrorMessage);
	this->mwFiles->SetValidator(vvValidator(gsValidationField_Files)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false)) 
		);

	this->Fit();
	// success
	return true;
}

bool vvRevertDialog::TransferDataToWindow()
{ 
	// retrieve status data and populate widgets
	if (!this->mwFiles->FillData(*this->GetVerbs(), *this->GetContext(), m_cRepoLocator))
	{
		this->GetContext()->Error_ResetReport("Unable to populate state data.");
		return false;
	}
	
	if (this->mwFiles->SetItemSelectionByPath(this->mcPaths, true) == 0u)
	{
		this->mwFiles->SetItemSelectionAll(true);
	}
	this->mwFiles->ExpandAll();
	return true;
}

wxArrayString vvRevertDialog::GetSelectedPaths(bool*bAll)
{
	if (bAll != NULL)
		*bAll = m_bAll;
	return mcSelectedFiles;
}

bool vvRevertDialog::GetSaveBackups()
{
	return mbSaveBackups;
}

bool vvRevertDialog::TransferDataFromWindow()
{
	// success
	vvStatusControl::stlItemDataList cStatusItems;
	this->mwFiles->GetItemData(&cStatusItems, vvStatusControl::COLUMN_FILENAME);
	this->mcSelectedFiles.clear();
	//Assume that they have selected all,
	this->m_bAll = true;
	for (vvStatusControl::stlItemDataList::const_iterator it = cStatusItems.begin(); it != cStatusItems.end(); ++it)
	{
		if (it->bSelected)
		{
			this->mcSelectedFiles.Add(it->sRepoPath);
		}
		else
		{
			this->m_bAll = false;
		}
	}
	this->mbSaveBackups = XRCCTRL(*this, "wSaveBackups", wxCheckBox)->GetValue();
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(vvRevertDialog, vvDialog);
BEGIN_EVENT_TABLE(vvRevertDialog, vvDialog)
END_EVENT_TABLE()
