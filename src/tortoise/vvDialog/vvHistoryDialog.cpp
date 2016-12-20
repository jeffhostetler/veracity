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
#include "vvHistoryDialog.h"
#include "vvHistoryOptionsControl.h"
#include "vvHistoryObjects.h"

#include "vvVerbs.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvHistoryListControl.h"
#include "vvDialogHeaderControl.h"

/*
**
** Globals
**
*/
static const wxString gsSingleItem = "Please select the new parent for %s:";
static const wxString gsMultipleItems = "Please select the new parent for the %d items to history:";


/*
**
** Public Functions
**
*/

bool vvHistoryDialog::Create(
	wxWindow*            pParent,
	vvHistoryFilterOptions& filterOptions,
	const vvRepoLocator& cRepoLocator,
	vvVerbs&             cVerbs,
	vvContext&           cContext,
	wxString& sChangesetHID
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}
	// create our header control
	wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, cRepoLocator.IsWorkingFolder() ? cRepoLocator.GetWorkingFolder() : cRepoLocator.GetRepoName(), cVerbs, cContext));

	m_tabbedControl = new vvHistoryTabbedControl(cVerbs, cContext, cRepoLocator, this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE, wxDefaultValidator, sChangesetHID);
	wxXmlResource::Get()->AttachUnknownControl("wHistoryControl", m_tabbedControl, pParent);
	
	m_statusLabel = XRCCTRL(*this, "wxStatusLabel",                 wxStaticText);

	m_filterOptions = filterOptions;
	//this->Fit();
	// success
	return true;
}

bool vvHistoryDialog::TransferDataToWindow()
{ 
	m_tabbedControl->ApplyDefaultOptions(m_filterOptions);
	m_tabbedControl->LoadData(m_filterOptions);
	return true;
}

bool vvHistoryDialog::TransferDataFromWindow()
{
	return true;
}
const wxEventType newEVT_HISTORY_STATUS = wxNewEventType();

bool vvHistoryDialog::OnStatus(wxCommandEvent& ev)
{
	m_statusLabel->SetLabel(ev.GetString() + (ev.GetInt() != 0 ? " (truncated)" : ""));
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(vvHistoryDialog, vvDialog);
BEGIN_EVENT_TABLE(vvHistoryDialog, vvDialog)
	EVT_HISTORY_STATUS(wxID_ANY, vvHistoryDialog::OnStatus)
END_EVENT_TABLE()
