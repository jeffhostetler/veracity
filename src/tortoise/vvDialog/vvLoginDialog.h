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

#ifndef VV_LOGIN_DIALOG_H
#define VV_LOGIN_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include "vvNullable.h"
#include "vvRepoLocator.h"


/**
 * This dialog is responsible for popping up
 * a wxTextEntryDialog.
 */
class vvLoginDialog : public vvDialog
{
// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            pParent,                         //< [in] The new dialog's parent window, or NULL if it won't have one.
		vvNullable<vvRepoLocator> cRepoLocator,				  //< [in] The repo to check for a whoami value.  Pass vvNULL to skip the check.
		const wxString&      sInitial_UserName,                   //< [in] The value to put in the user name field.
		const wxString&		 sRemoteRepo,
		vvVerbs&             cVerbs,
		vvContext&           cContext
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
	 * Get the new name from the dialog.
	 **/
	wxString GetUserName();

	/**
	 * Get the password name from the dialog.
	 **/
	wxString GetPassword();

	/**
	 * Get the remember password setting
	 **/
	bool GetRememberPassword();
	void FitHeight();

	
// private data
private:
	// internal data
	vvNullable<vvRepoLocator> mcRepoLocator;
	wxString     msUserName;        
	wxString     msPassword;        
	bool		 mbRememberPassword;
	wxComboBox * mwComboBox_User; 
	wxTextCtrl * mwTextEntry_Password; 
	wxCheckBox * mwCheckBox_Remember;
	
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvLoginDialog);
	DECLARE_EVENT_TABLE();
};


#endif
