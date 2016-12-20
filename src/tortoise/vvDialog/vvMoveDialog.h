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

#ifndef VV_MOVE_DIALOG_H
#define VV_MOVE_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include <wx/dirctrl.h>

/**
 * This dialog is responsible for popping up
 * a wxTextEntryDialog.
 */
class vvMoveDialog : public vvDialog
{
// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            pParent,                         //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxString&      sInitialValue,                   //< [in] One of the items to move.  If it's the only item, its name will be displayed.  Otherwise, it will be used to set the initial selection in the dialog.
		unsigned int		 nCountItemsToMove,				  //< [in] The number of items that will be moved.
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

	/**
	 * Get the new name from the dialog.
	 **/
	wxString GetNewPath();

// private data
private:
	// internal data
	wxString     msNewPath;        //< The new name 
	wxGenericDirCtrl * mwDirControl; 
	
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvMoveDialog);
	DECLARE_EVENT_TABLE();
};


#endif
