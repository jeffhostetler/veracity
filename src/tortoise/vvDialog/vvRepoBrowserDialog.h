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

#ifndef VV_REPOBROWSER_DIALOG_H
#define VV_REPOBROWSER_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvRepoBrowserControl.h"

/**
 * This dialog is responsible for popping up
 * a wxTextEntryDialog.
 */
class vvRepoBrowserDialog : public vvDialog
{
// functionality
public:
	vvRepoBrowserDialog::vvRepoBrowserDialog()
	{
		m_sInitialSelection = wxEmptyString;
		m_sCurrentSelection = wxEmptyString;
	}
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            pParent,                         //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxString&      sRepoName,							  //< [in] The name of the repo to query against.
		const wxString&		sChangesetHID,						  //< [in] The changeset to browse.
		const wxString&      sInitialSelection,					  //< [in] The initial selection
		bool                 bChooser,                        //< [in] True: File-picker style - requires selection of a specific item, and allows cancelation.
		                                                      //<      False: Browser style - only allows browsing, no items are chosen and no cancelation possible.
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

	wxString vvRepoBrowserDialog::GetSelection();

// event handlers
protected:
	void OnItemActivated(wxCommandEvent& e);

// private data
private:
	// internal data
	vvRepoBrowserControl * m_browserControl;
	wxString m_sInitialSelection;
	wxString m_sCurrentSelection;
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvRepoBrowserDialog);
	DECLARE_EVENT_TABLE();
};


#endif
