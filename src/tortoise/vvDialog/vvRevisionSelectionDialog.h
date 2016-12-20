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

#ifndef VV_REVISION_SELECTION_DIALOG_H
#define VV_REVISION_SELECTION_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvHistoryObjects.h"

/**
 * A dialog that allows the user to choose a single revision/changeset from
 * the history of a given repo.
 */
class vvRevisionSelectionDialog : public vvDialog
{
// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*                           pParent,                  //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxString&                     sRepo,                    //< [in] The repo to select a revision from.
		class vvVerbs&                      cVerbs,                   //< [in] The vvVerbs implementation to retrieve history from.
		class vvContext&                    cContext,                 //< [in] Context to use with pVerbs.
		const wxString&                     sTitle = wxEmptyString,   //< [in] Title to use for the dialog.
		                                                              //<      If empty, a default title is used.
		const wxString&                     sMessage = wxEmptyString, //< [in] Message to display at the top of the dialog.
		                                                              //<      If empty, no message is displayed.
		const class vvHistoryFilterOptions* pFilter = NULL            //< [in] The default history filter settings to use.
		                                                              //<      If NULL, default filter settings are applied.
		);

	/**
	 * Transfers our internal data to our widgets.
	 */
	virtual bool TransferDataToWindow();

	/**
	 * Stores data from our widgets to our internal data.
	 */
	virtual bool TransferDataFromWindow();

	/**
	 * Gets the revision selected in the dialog.
	 */
	const wxString& GetRevision() const;

// event handlers
protected:
	bool OnStatus(wxCommandEvent& e);

// private data
private:
	// internal data
	wxString                     msRevision; //< The revision selected in the dialog.
	class vvHistoryFilterOptions mcFilter;   //< The filter settings applied in the dialog.

	// widgets
	class vvHistoryTabbedControl* mwHistory;
	wxStaticText*                 mwStatus;

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvRevisionSelectionDialog);
	DECLARE_EVENT_TABLE();
};


#endif
