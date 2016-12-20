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

#ifndef VV_SETTINGS_DIALOG_H
#define VV_SETTINGS_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include "vvRepoLocator.h"

/**
 * A dialog box used to manipulate Tortoise-specific config settings.
 */
class vvSettingsDialog : public vvDialog
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvSettingsDialog();

	/**
	 * Create constructor.
	 */
	vvSettingsDialog(
		wxWindow*        wParent,                 //< [in] The new dialog's parent window, or NULL if it won't have one.
		class vvVerbs&   cVerbs,                  //< [in] The vvVerbs implementation to use.
		class vvContext& cContext,                //< [in] Context to use with pVerbs.
		const vvRepoLocator& cRepoLocator		  //< [in] Include repo-specific settings from this repo.
		);

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*        wParent,                 //< [in] The new dialog's parent window, or NULL if it won't have one.
		class vvVerbs&   cVerbs,                  //< [in] The vvVerbs implementation to use.
		class vvContext& cContext,                //< [in] Context to use with pVerbs.
		const vvRepoLocator& cRepoLocator		  //< [in] Include repo-specific settings from this repo.
		);

// implementation
public:
	/**
	 * Transfers our internal data to our widgets.
	 */
	virtual bool TransferDataToWindow();

	/**
	 * Stores data from our widgets to our internal data.
	 */
	virtual bool TransferDataFromWindow();

// event handlers
protected:
	void OnForgetPasswords(wxCommandEvent& e);

// helpers
private:
	/**
	 * Populates the user list combo box.
	 */
	bool PopulateUsers();

// private data
private:
	// data
	vvRepoLocator                      mcRepoLocator;              //< The repo we're displaying repo-specific settings from.
	                                                   //< Empty if we're not displaying repo-specific settings.
	vvValidatorMessageBoxReporter mcValidatorReporter; //< Reports errors from validators to a message box.

	// widgets
	wxComboBox* mwCurrentUser;
	wxButton*   mwForgetPasswords;
	wxComboBox* mwDiffTool;
	wxComboBox* mwMergeTool;
	wxCheckBox* mwHideOutsideWC;

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvSettingsDialog);
	DECLARE_EVENT_TABLE();
};


#endif
