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
#include "vvRevisionSelectionDialog.h"

#include "vvContext.h"
#include "vvDialogHeaderControl.h"
#include "vvHistoryTabbedControl.h"
#include "vvSelectableStaticText.h"
#include "vvVerbs.h"
#include "vvWxHelpers.h"


/*
**
** Globals
**
*/

/*
 * Format strings used for the status text.
 */
static const char* gszStatusFormat_Complete  = "%s";
static const char* gszStatusFormat_Truncated = "%s (truncated)";


/*
**
** Public Functions
**
*/

bool vvRevisionSelectionDialog::Create(
	wxWindow*                     pParent,
	const wxString&               sRepo,
	vvVerbs&                      cVerbs,
	vvContext&                    cContext,
	const wxString&               sTitle,
	const wxString&               sMessage,
	const vvHistoryFilterOptions* pFilter
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// if they passed a new title, set that
	if (!sTitle.IsEmpty())
	{
		this->SetTitle(sTitle);
	}

	// if they didn't specify a filter, load the default
	if (pFilter == NULL)
	{
		if (!cVerbs.GetHistoryFilterDefaults(cContext, sRepo, this->mcFilter))
		{
			cContext.Error_ResetReport(wxEmptyString);
			return false;
		}
	}
	else
	{
		this->mcFilter = *pFilter;
	}

	// get our widgets
	this->mwStatus = XRCCTRL(*this, "wStatus", wxStaticText);

	// create our message control
	wxXmlResource::Get()->AttachUnknownControl("wMessage", new vvSelectableStaticText(this, wxID_ANY, sMessage));
	if (sMessage.IsEmpty())
	{
		vvXRCSIZERITEM(*this, "cMessage")->Show(false);
	}

	// create our history control
	this->mwHistory = new vvHistoryTabbedControl(cVerbs, cContext, vvRepoLocator::FromRepoName(sRepo), this, wxID_ANY);
	wxXmlResource::Get()->AttachUnknownControl("wHistory", this->mwHistory);

	// success
	return true;
}

bool vvRevisionSelectionDialog::TransferDataToWindow()
{ 
	this->mwHistory->ApplyDefaultOptions(this->mcFilter);
	this->mwHistory->LoadData(this->mcFilter);
	return true;
}

bool vvRevisionSelectionDialog::TransferDataFromWindow()
{
	wxArrayString cSelections = this->mwHistory->GetSelection();
	wxASSERT(cSelections.size() <= 1u);
	this->msRevision = cSelections.size() == 0u ? wxEmptyString : cSelections[0];
	return true;
}

const wxString& vvRevisionSelectionDialog::GetRevision() const
{
	return this->msRevision;
}

bool vvRevisionSelectionDialog::OnStatus(
	wxCommandEvent& e
	)
{
	this->mwStatus->SetLabel(wxString::Format(e.GetInt() == 0 ? gszStatusFormat_Complete : gszStatusFormat_Truncated, e.GetString()));
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(vvRevisionSelectionDialog, vvDialog);

//const wxEventType newEVT_HISTORY_STATUS = wxNewEventType();
BEGIN_EVENT_TABLE(vvRevisionSelectionDialog, vvDialog)
	EVT_HISTORY_STATUS(wxID_ANY, OnStatus)
END_EVENT_TABLE()
