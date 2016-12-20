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
#include "vvSyncTargetsControl.h"

#include "vvVerbs.h"
#include "vvContext.h"
#include "wx/combo.h"


/*
**
** Globals
**
*/

static const char* gszColumn_URL    = "URL or repo name";
static const char* gszColumn_Alias   = "Alias";


vvSyncTargetsControl::vvSyncTargetsControl()
{
	mwListViewComboPopup = NULL;
}

bool vvSyncTargetsControl::Populate(
	vvContext&           cContext,
	vvVerbs&             cVerbs,
	const wxString&      sRepoDescriptor
	)
{
	wxArrayString cRemoteRepos;
	wxArrayString cRemoteRepoAliases;
	if (!cVerbs.GetRemoteRepos(cContext, sRepoDescriptor, cRemoteRepos, cRemoteRepoAliases))
	{
		cContext.Error_ResetReport("Could not load remote repositories for: %s", sRepoDescriptor);
		return false;
	}

	this->mwListViewComboPopup->InsertColumn(0, gszColumn_URL);
	this->mwListViewComboPopup->InsertColumn(1, gszColumn_Alias, wxLIST_FORMAT_RIGHT);
	for (unsigned int uIndex = 0u; uIndex < cRemoteRepos.size(); ++uIndex)
	{
		wxListItem item;

		this->mwListViewComboPopup->InsertItem(uIndex, "test");
		//this->mwListViewComboPopup->SetItem(this->mwListViewComboPopup->GetItemCount(), 0, cRemoteRepos[uIndex] );
		this->mwListViewComboPopup->SetItem(uIndex, 0, cRemoteRepos[uIndex] );
		this->mwListViewComboPopup->SetItem(uIndex, 1, cRemoteRepoAliases[uIndex] );
		if (uIndex == 0)
			this->SetValue(cRemoteRepos[0]);
	}

	//Set the column widths.
	int comboWidth = (int)this->GetSize().GetWidth();
	int firstColumnWidth =  (int)((double)comboWidth * 0.75);
	int secondColumnWidth =  (int)((double)comboWidth * 0.20);
	this->mwListViewComboPopup->SetColumnWidth(0, firstColumnWidth);
	this->mwListViewComboPopup->SetColumnWidth(1, secondColumnWidth);
	this->SetInsertionPoint(0);

	// done
	return true;
}
/*
**
** Public Functions
**
*/

vvSyncTargetsControl::vvSyncTargetsControl(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	)
{
	mwListViewComboPopup = NULL;
	this->Create(pParent, cId, cPosition, cSize, iStyle, cValidator);
}

bool vvSyncTargetsControl::Create(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	)
{
	mwListViewComboPopup = NULL;
	vvFlags cStyle = iStyle;
	// create our parent
	if (!wxComboCtrl::Create(pParent, cId, wxEmptyString, cPosition, cSize, iStyle, cValidator))
	{
		return false;
	}

	this->mwListViewComboPopup = new vv_wxListViewComboPopup();
	wxComboCtrl::UseAltPopupWindow();
	wxComboCtrl::EnablePopupAnimation(false);
    wxComboCtrl::SetPopupMinWidth(200);
	wxComboCtrl::SetPopupControl(this->mwListViewComboPopup);

	// done
	return true;
}

/**

Event handlers.

**/

void vvSyncTargetsControl::OnComboBoxDropdown(wxCommandEvent& cEvent)
{
	cEvent;
	this->mwListViewComboPopup->SetFocus();
}

void vvSyncTargetsControl::OnComboBoxResized(
	wxSizeEvent& cEvent
	)
{
	cEvent;
	if (this->IsCreated())
	{
		int comboWidth = (int)this->GetSize().GetWidth();
		int firstColumnWidth =  (int)((double)comboWidth * 0.75);
		int secondColumnWidth =  (int)((double)comboWidth * 0.20 );
		if (this->mwListViewComboPopup)
		{
			this->mwListViewComboPopup->SetColumnWidth(0, firstColumnWidth);
			this->mwListViewComboPopup->SetColumnWidth(1, secondColumnWidth);
		}
		cEvent.Skip();
	}
}


IMPLEMENT_DYNAMIC_CLASS(vvSyncTargetsControl, wxComboCtrl);

BEGIN_EVENT_TABLE(vvSyncTargetsControl, wxComboCtrl)
	EVT_COMBOBOX_DROPDOWN(wxID_ANY, OnComboBoxDropdown)
	EVT_SIZE(OnComboBoxResized)
END_EVENT_TABLE()
