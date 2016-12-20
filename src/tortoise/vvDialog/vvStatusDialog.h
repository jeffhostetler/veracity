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

#ifndef VV_STATUS_DIALOG_H
#define VV_STATUS_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvRepoLocator.h"

/**
 * A dialog box used to display the status of a working copy.
 */
class vvStatusDialog : public vvDialog
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvStatusDialog();

	/**
	 * Destructor.
	 */
	~vvStatusDialog();

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            pParent,                    //< [in] The new dialog's parent window, or NULL if it won't have one.
		const vvRepoLocator& cRepoLocator,               //< [in] The working copy or repo to display status from.
		                                                 //<      If this is a repo name, then sRevision and sRevision2 must both be non-empty.
		class vvVerbs&       cVerbs,                     //< [in] The vvVerbs implementation to retrieve status data from.
		class vvContext&     cContext,                   //< [in] Context to use with pVerbs.
		const wxString&      sRevision  = wxEmptyString, //< [in] The first of two revisions to compare.
		                                                 //<      If empty, cRepoLocator must indicate a working copy, which is compared to its baseline.
		const wxString&      sRevision2 = wxEmptyString  //< [in] The second of two revisions to compare.
		                                                 //<      If non-empty, then sRevision must also be non-empty.
		                                                 //<      If empty but sRevision is specified, it is compared against the working copy indicated by cRepoLocator.
		);

// implementation
public:
	/**
	 * Transfers our internal data to our widgets.
	 */
	virtual bool TransferDataToWindow();

// internal functionality
protected:
	/**
	 * Update the visible/enabled state of all controls, based on the current values.
	 */
	void UpdateDisplay();

// event handlers
protected:
	void OnUpdateEvent(wxCommandEvent& e);

// private data
private:
	// internal data
	vvRepoLocator mcRepoLocator; //< The working copy or repo containing the displayed status.
	wxString      msRevision;    //< The first of two revisions potentially being compared.
	wxString      msRevision2;   //< The second of two revisions potentially being compared.

	// widgets
	class vvStatusControl* mwStatus;
	wxCheckBox*            mwIgnored;

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvStatusDialog);
	DECLARE_EVENT_TABLE();
};


#endif
