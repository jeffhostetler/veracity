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

#ifndef VV_UPGRADE_CONFIRMATION_DIALOG_H
#define VV_UPGRADE_CONFIRMATION_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"


/**
 * A dialog box that displays details about a log message.
 */
class vvUpgradeConfirmationDialog : public vvDialog
{
// constants
public:
	static const wxString ssDefaultMessageType;

// construction
public:
	/**
	 * Default constructor.
	 */
	vvUpgradeConfirmationDialog();

	/**
	 * Create constructor.
	 */
	vvUpgradeConfirmationDialog(
		wxWindow*         pParent,                          //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxArrayString aAllReposToUpgrade
		);

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*         pParent,                          //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxArrayString aAllReposToUpgrade
		);

	/**
	 * Transfers our internal data to our widgets.
	 */
	virtual bool TransferDataToWindow();

// private data
private:
	// widgets
	//wxTextCtrl*     mwMessage;

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvUpgradeConfirmationDialog);
	DECLARE_EVENT_TABLE();
};


#endif
