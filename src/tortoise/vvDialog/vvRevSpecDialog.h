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

#ifndef VV_REV_SPEC_DIALOG_H
#define VV_REV_SPEC_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvRevSpec.h"
#include "vvValidatorMessageBoxReporter.h"
#include "vvRepoLocator.h"

/**
 * A dialog box that allows the user to choose/create a vvRevSpec from a repository.
 * Currently the dialog only supports choosing a single revision.
 */
class vvRevSpecDialog : public vvDialog
{
// construction
public:
	/**
	 * Destructor.
	 */
	~vvRevSpecDialog();

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*                  pParent,                //< [in] The new dialog's parent window, or NULL if it won't have one.
		const class vvRepoLocator& cRepo,                  //< [in] The repo to choose a revision from.
		class vvVerbs&             cVerbs,                 //< [in] The vvVerbs implementation to retrieve repo information from.
		class vvContext&           cContext,               //< [in] Context to use with pVerbs.
		vvRevSpec                  cRevSpec = vvRevSpec(), //< [in] Initial value to display in the dialog.
		                                                   //<      The dialog assumes ownership of the rev spec.
		const wxString&            sTitle = wxEmptyString, //< [in] Title to use for the dialog, or empty for the default.
		const wxString&            sLabel = wxEmptyString, //< [in] Label to use for the dialog's main control group, or empty for the default.
		bool					   bMerging = false		   //< [in] True means that this dialog is determining the revision spec for merge.  Merge causes some different behavior when selecting a branch.
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
	 * Gets the RevSpec created/chosen in the dialog.
	 * Caller does NOT own the returned spec.
	 */
	const vvRevSpec& GetRevSpec() const;

// private data
private:
	// data
	vvRevSpec mcSpec;

	// widgets
	class vvRevSpecControl* mwSpec;

	// misc
	vvRepoLocator m_cRepoLocator;
	vvValidatorMessageBoxReporter mcValidatorReporter; //< Reporter that will display validation errors.
	wxArrayString m_arBaselineHIDs;

	bool m_bMerging;
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvRevSpecDialog);
	DECLARE_EVENT_TABLE();
};


#endif
