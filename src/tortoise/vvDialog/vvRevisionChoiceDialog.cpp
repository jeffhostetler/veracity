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
#include "vvRevisionChoiceDialog.h"
#include "vvHistoryListControl.h"


/*
**
** Public Functions
**
*/

bool vvRevisionChoiceDialog::Create(
	wxWindow*            pParent,
	const vvRepoLocator& cRepoLocator,
	vvVerbs::HistoryData data,
	const wxString&		sTitle,
	const wxString&		sDescription,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	m_data = data;
	m_pVerbs = &cVerbs;
	m_pContext = &cContext;
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}
	this->SetTitle(sTitle);
	sDescription;
	wxString sWorkingFolder = wxEmptyString;
	m_historyControl = new vvHistoryListControl(cVerbs, cContext, cRepoLocator, this, wxID_ANY);
	wxXmlResource::Get()->AttachUnknownControl("wRevisionChoiceControl", m_historyControl, pParent);

	wxTextCtrl* pDescription = XRCCTRL(*this, "wDescription", wxTextCtrl);
	pDescription->SetValue(sDescription);
	pDescription->SetBackgroundColour(this->GetBackgroundColour());

	// success
	return true;
}

bool vvRevisionChoiceDialog::TransferDataToWindow()
{ 
	vvHistoryColumnPreferences columnPrefs;
	m_historyControl->ApplyColumnPreferences();
	m_historyControl->LoadHistory(m_data);
	m_historyControl->SetFocus();
	return true;
}

bool vvRevisionChoiceDialog::TransferDataFromWindow()
{
	wxArrayString selections = m_historyControl->GetSelection();
	if (selections.Count() == 0)
	{
		wxMessageBox("Please select a revision", "Error", wxOK | wxOK_DEFAULT);
		
		return false;
	}
	m_sCurrentSelection = selections[0];
	return true;
}

wxString vvRevisionChoiceDialog::GetSelection()
{
	return m_sCurrentSelection;
}

IMPLEMENT_DYNAMIC_CLASS(vvRevisionChoiceDialog, vvDialog);
BEGIN_EVENT_TABLE(vvRevisionChoiceDialog, vvDialog)
END_EVENT_TABLE()
