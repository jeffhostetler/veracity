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

#ifndef VV_DIALOG_HEADER_CONTROL_H
#define VV_DIALOG_HEADER_CONTROL_H

#include "precompiled.h"


/**
 * A header for most dialogs that displays simple context information and a logo.
 * The default information to display is the working copy, repo, and branch.
 */
class vvDialogHeaderControl : public wxPanel
{
// constants
public:
	/**
	 * The name of the bitmap that is displayed by default.
	 */
	static const wxArtID scDefaultBitmapName;

	/*
	 * Labels for the specialized values.
	 */
	static const char* sszLabel_Folder; //< Label for folder values.
	static const char* sszLabel_Repo;   //< Label for repo values.
	static const char* sszLabel_Branch; //< Label for branch values.

// construction
public:
	/**
	 * Default constructor.
	 */
	vvDialogHeaderControl();

	/**
	 * Create constructor.
	 */
	vvDialogHeaderControl(
		wxWindow*          pParent,                          //< [in] The control's parent window.
		wxWindowID         cId        = wxID_ANY,            //< [in] The control's ID.
		const wxArtID&     cBitmap    = scDefaultBitmapName, //< [in] The name of the bitmap to display.
		const wxPoint&     cPosition  = wxDefaultPosition,   //< [in] The control's position.
		const wxSize&      cSize      = wxDefaultSize,       //< [in] The control's size.
		long               iStyle     = 0                    //< [in] The control's style.
		);

	/**
	 * Create constructor.
	 */
	vvDialogHeaderControl(
		wxWindow*          pParent,                          //< [in] The control's parent window.
		wxWindowID         cId,                              //< [in] The control's ID.
		const wxString&    sFolder,                          //< [in] The working copy to display information about.
		class vvVerbs&     cVerbs,                           //< [in] The vvVerbs implementation to use to get information about sFolder.
		class vvContext&   cContext,                         //< [in] The vvContext to use with cVerbs.
		const wxArtID&     cBitmap    = scDefaultBitmapName, //< [in] The name of the bitmap to display.
		const wxPoint&     cPosition  = wxDefaultPosition,   //< [in] The control's position.
		const wxSize&      cSize      = wxDefaultSize,       //< [in] The control's size.
		long               iStyle     = 0                    //< [in] The control's style.
		);

// functionality
public:
	/**
	 * Creates the control without any values.
	 */
	bool Create(
		wxWindow*          pParent,                          //< [in] The control's parent window.
		wxWindowID         cId        = wxID_ANY,            //< [in] The control's ID.
		const wxArtID&     cBitmap    = scDefaultBitmapName, //< [in] The name of the bitmap to display.
		const wxPoint&     cPosition  = wxDefaultPosition,   //< [in] The control's position.
		const wxSize&      cSize      = wxDefaultSize,       //< [in] The control's size.
		long               iStyle     = 0                    //< [in] The control's style.
		);

	/**
	 * Creates the control with the default values.
	 */
	bool Create(
		wxWindow*          pParent,                          //< [in] The control's parent window.
		wxWindowID         cId,                              //< [in] The control's ID.
		const wxString&    sFolder,                          //< [in] The working copy to display information about.
		class vvVerbs&     cVerbs,                           //< [in] The vvVerbs implementation to use to get information about sFolder.
		class vvContext&   cContext,                         //< [in] The vvContext to use with cVerbs.
		const wxArtID&     cBitmap    = scDefaultBitmapName, //< [in] The name of the bitmap to display.
		const wxPoint&     cPosition  = wxDefaultPosition,   //< [in] The control's position.
		const wxSize&      cSize      = wxDefaultSize,       //< [in] The control's size.
		long               iStyle     = 0                    //< [in] The control's style.
		);

	/**
	 * Adds a value displayed with an arbitrary control.
	 * Returns true if the value was added successfully, or false if it wasn't.
	 *
	 * Note: The given control must be a child of this header control.
	 */
	bool AddControlValue(
		const wxString& sLabel,   //< [in] The label to use for the given control.
		wxWindow*       wControl, //< [in] The control to add to the header.
		bool            bExpand   //< [in] Whether or not the control should expand width-wise.
		);

	/**
	 * Specialization of AddControlValue that displays a plain text string value.
	 * Returns true if the value was added successfully, or false if it wasn't.
	 */
	bool AddTextValue(
		const wxString& sLabel, //< [in] The label to use for the given text value.
		const wxString& sValue  //< [in] The text value to display.
		);

	/**
	 * Changes a text value previously added with AddTextValue.
	 * Pass the same label, which is used to find the control to update.
	 */
	bool SetTextValue(
		const wxString& sLabel, //< [in] The label of the value to set, as passed earlier to AddTextValue.
		const wxString& sValue  //< [in] The text value to change the display to.
		);

	/**
	 * Specialization of AddTextValue that displays a working copy.
	 * Returns true if the value was added successfully, or false if it wasn't.
	 */
	bool AddFolderValue(
		const wxString&  sFolder,   //< [in] The working copy to display.
		                            //<      This can actually be any path within a working copy.
		class vvVerbs&   cVerbs,    //< [in] The vvVerbs implementation to use to lookup sFolder's working copy.
		class vvContext& cContext   //< [in] The vvContext to use with pVerbs.
		);

	/**
	 * Specialization of AddTextValue that displays a repo name.
	 * Returns true if the value was added successfully, or false if it wasn't.
	 */
	bool AddRepoValue(
		const wxString&  sFolder,   //< [in] The working copy to display the associated repo of.
		                            //<      This can actually be any path within a working copy.
		class vvVerbs&   cVerbs,    //< [in] The vvVerbs implementation to use to lookup sFolder's working copy.
		class vvContext& cContext   //< [in] The vvContext to use with pVerbs.
		);

	/**
	 * Combination of AddFolderValue and AddRepoValue that avoids duplicated work.
	 * Returns true if the values were added successfully, or false if they weren't.
	 */
	bool AddFolderAndRepoValues(
		const wxString&  sFolder,   //< [in] The working copy to display.
		                            //<      This can actually be any path within a working copy.
		class vvVerbs&   cVerbs,    //< [in] The vvVerbs implementation to use to lookup sFolder's working copy and repo.
		class vvContext& cContext   //< [in] The vvContext to use with pVerbs.
		);

	/**
	 * Specialization of AddTextValue that displays a branch name.
	 * Returns true if the value was added successfully, or false if it wasn't.
	 */
	bool AddBranchValue(
		const wxString&  sFolder,   //< [in] The working copy to display the tied branch of.
		                            //<      This can actually be any path within a working copy.
		class vvVerbs&   cVerbs,    //< [in] The vvVerbs implementation to use to lookup sFolder's working copy.
		class vvContext& cContext   //< [in] The vvContext to use with pVerbs.
		);

	/**
	 * A combination of AddFolderValue, AddRepoValue, and AddBranchValue.
	 * Returns true if the values were added successfully, or false if they weren't.
	 */
	bool AddDefaultValues(
		const wxString&  sFolder,   //< [in] The working copy to display information about.
		                            //<      This can actually be any path within a working copy.
		class vvVerbs&   cVerbs,    //< [in] The vvVerbs implementation to use to lookup sFolder's working copy.
		class vvContext& cContext   //< [in] The vvContext to use with pVerbs.
		);

// private data
private:
	wxFlexGridSizer* mpSizer;  //< The sizer to add values to.
	wxStaticBitmap*  mwBitmap; //< Displays the bitmap.

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvDialogHeaderControl);
	DECLARE_EVENT_TABLE();
};


#endif
