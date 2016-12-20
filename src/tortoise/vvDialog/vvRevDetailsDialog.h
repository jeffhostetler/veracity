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

#ifndef VV_REVDETAILS_DIALOG_H
#define VV_REVDETAILS_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include <wx/dirctrl.h>
#include <wx/combo.h>
#include "vv_wxListViewComboPopup.h"
#include "vvTextLinkControl.h"
#include "vvHistoryListControl.h"
/**
 * This dialog is responsible for popping up
 * a wxTextEntryDialog.
 */
class vvRevDetailsDialog : public vvDialog
{
// events
protected:
// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            pParent,                         //< [in] The new dialog's parent window, or NULL if it won't have one.
		const vvRepoLocator& cRepoLocator,					  //< [in] The repository to use.
		const wxString&      sRevID,						//< [in] The revision to fetch details.
		class vvVerbs&       cVerbs,                          //< [in] The vvVerbs implementation to retrieve local repo and config info from.
		class vvContext&     cContext                         //< [in] Context to use with pVerbs.
		);

	/**
	 * Transfers our internal data to our widgets.
	 */
	virtual bool TransferDataToWindow();

	/**
	 * Stores data from our widgets to our internal data.
	 */
	virtual bool TransferDataFromWindow();

	void SetRevision(vvVerbs::HistoryEntry he);

private:
	void SetReadOnly();
	void DisableNavigation();
	void RevLinkClicked(wxCommandEvent& evt);
	void LoadRevision(wxString sRevHID);
	void OnTagsContext(wxContextMenuEvent& event);
	void OnStampsContext(wxContextMenuEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void AddTag(wxCommandEvent& evt);
	void AddStamp(wxCommandEvent& evt);
	void LaunchDiff(wxString rev1, wxString rev2);
// private data
private:
	const vvRepoLocator* m_pRepoLocator;
	wxString msRevID;
	wxArrayString aRevisions;
	int nCurrentRevision;
	vvTextLinkControl * m_pForwardLink;
	vvTextLinkControl * m_pBackLink;
	wxWindowID m_nParentRevLinkID_1;
	wxWindowID m_nParentRevLinkID_2;
	wxWindowID m_nParentRevLinkID_3;
	wxBitmap m_DiffBitmap;
	vvVerbs::HistoryEntry m_historyEntry;
	wxArrayInt m_parentButtonIds;
	wxArrayInt m_childButtonIds;
	wxArrayString m_parentCSIDs;
	wxArrayString m_childCSIDs;
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvRevDetailsDialog);
	DECLARE_EVENT_TABLE();
};


#endif
