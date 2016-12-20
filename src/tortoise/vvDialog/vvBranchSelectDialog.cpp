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
#include "vvBranchSelectDialog.h"

#include "vvVerbs.h"

/*
**
** Public Functions
**
*/

bool vvBranchSelectDialog::Create(
	wxWindow*            pParent,
	vvRepoLocator		 cRepoLocator,
	const wxString&      sInitialBranchSelection,
	bool				 bShowAll,
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
	this->m_cRepoLocator = cRepoLocator;
	this->msSelectedBranch      = sInitialBranchSelection;
	this->m_bShowAll			= bShowAll;

	this->mwBranchList						  = XRCCTRL(*this, "clBranchList",                 wxListBox);
	this->m_wShowAllCheckBox				  = XRCCTRL(*this, "cbShowAll",					   wxCheckBox);
	
	// success
	return true;
}

bool vvBranchSelectDialog::TransferDataToWindow()
{ 
	m_wShowAllCheckBox->SetValue(m_bShowAll);
	wxCommandEvent bogus;
	RefreshBranches(bogus);
	//Fit();
	return true;
}

void vvBranchSelectDialog::RefreshBranches(wxCommandEvent& ev)
{
	ev;
	// success
	{
		m_cBranches.clear();
		wxBusyCursor cBusyCursor;

		if (!GetVerbs()->GetBranches(*this->GetContext(), this->m_cRepoLocator.GetRepoName(), m_wShowAllCheckBox->GetValue(), m_cBranches))
		{
			wxLogError("Error retrieving branch list from repository.");
			return;
		}
	}

	// get the index of the branch currently entered in the control
	int iIndex = wxNOT_FOUND;
	int currentEntryNumber = 0;
	for (vvVerbs::BranchList::const_iterator it = m_cBranches.begin(); it != m_cBranches.end(); ++it)
	{
		if (it->sBranchName.CompareTo(this->msSelectedBranch) == 0)
		{
			iIndex = currentEntryNumber;
		}
		currentEntryNumber++;
	}

	wxArrayString aBranchNames = GetVerbs()->GetBranchDisplayNames(m_cBranches);

	{
		this->mwBranchList->Set(aBranchNames);
		if (iIndex != wxNOT_FOUND)
		{
			this->mwBranchList->SetSelection(iIndex);
		}
	}
	return;
}

bool vvBranchSelectDialog::TransferDataFromWindow()
{
	// success
	int selectedIndex = this->mwBranchList->GetSelection();
	if (selectedIndex < 0)
		return false;
	msSelectedBranch = m_cBranches[this->mwBranchList->GetSelection()].sBranchName;
	return true;
}

wxString vvBranchSelectDialog::GetSelectedBranch()
{
	return this->msSelectedBranch;
}

void vvBranchSelectDialog::OnDoubleClick(wxCommandEvent& WXUNUSED(ev))
{
	this->AcceptAndClose();
}

IMPLEMENT_DYNAMIC_CLASS(vvBranchSelectDialog, vvDialog);
BEGIN_EVENT_TABLE(vvBranchSelectDialog, vvDialog)
	EVT_CHECKBOX(wxID_ANY, vvBranchSelectDialog::RefreshBranches)
	EVT_LISTBOX_DCLICK(wxID_ANY, vvBranchSelectDialog::OnDoubleClick)
END_EVENT_TABLE()
