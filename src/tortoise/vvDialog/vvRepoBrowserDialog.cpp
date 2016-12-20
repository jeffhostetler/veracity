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
#include "vvRepoBrowserDialog.h"
#include "vvRepoBrowserControl.h"
#include "vvWxHelpers.h"


/*
**
** Public Functions
**
*/

bool vvRepoBrowserDialog::Create(
	wxWindow*            pParent,
	const wxString&      sRepoName,
	const wxString&		sChangesetHID,
	const wxString&      sInitialSelection,
	bool                 bChooser,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	sRepoName;
	m_sInitialSelection = sInitialSelection;
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}
	m_browserControl = new vvRepoBrowserControl(cVerbs, cContext, sRepoName, sChangesetHID, this, wxID_ANY);
	wxXmlResource::Get()->AttachUnknownControl("wRepoBrowserControl", m_browserControl, pParent);

	// configure ourselves to be a chooser or a browser
	if (bChooser)
	{
		this->Bind(EVT_REPO_BROWSER_ITEM_ACTIVATED, &vvRepoBrowserDialog::OnItemActivated, this, wxID_ANY);
	}
	else
	{
		this->SetTitle("Browse Revision");
		XRCCTRL(*this, "wxID_OK", wxButton)->SetLabelText("Close");
		XRCCTRL(*this, "wxID_CANCEL", wxButton)->Hide();
		this->Layout();
	}

	this->Fit();
	// success
	return true;
}

bool vvRepoBrowserDialog::TransferDataToWindow()
{ 
	m_browserControl->SetSelection(m_sInitialSelection);
	return true;
}

bool vvRepoBrowserDialog::TransferDataFromWindow()
{
	m_sCurrentSelection = m_browserControl->GetSelection();
	return true;
}

wxString vvRepoBrowserDialog::GetSelection()
{
	return m_sCurrentSelection;
}

void vvRepoBrowserDialog::OnItemActivated(
	wxCommandEvent& e
	)
{
	this->m_sCurrentSelection = e.GetString();
	this->EndModal(wxID_OK);
}

IMPLEMENT_DYNAMIC_CLASS(vvRepoBrowserDialog, vvDialog);
BEGIN_EVENT_TABLE(vvRepoBrowserDialog, vvDialog)
END_EVENT_TABLE()
