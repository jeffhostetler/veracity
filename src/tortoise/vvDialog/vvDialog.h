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

#ifndef VV_DIALOG_H
#define VV_DIALOG_H

#include "precompiled.h"
#include "vvValidator.h"


/**
 * Base class for all top-level dialogs spawned by this program.
 */
class vvDialog : public wxDialog
{
// construction
public:
	vvDialog();

// functionality
public:
	/**
	 * Called by the main application to create the dialog.
	 */
	virtual bool Create(
		wxWindow*        pParent,                  //< [in] The new dialog's parent window, or NULL if it won't have one.
		class vvVerbs*   pVerbs    = NULL,         //< [in] The vvVerbs implementation to use for interfacing with sglib, or NULL if none is necessary.
		class vvContext* pContext  = NULL,         //< [in] The context to use with pVerbs, or NULL if none is necessary.
		const wxString&  sResource = wxEmptyString,//< [in] The name of the top-level XRC dialog resource to use for the dialog.
		                                           //<      If wxEmptyString, then no dialog resource will be loaded/used.
		bool bRestoreSizeAndPosition = true		   //< [in] If true, the dialog will not save its size and position.
		);

	/**
	 * Sets the action to execute when the dialog is closed successfully (with wxID_OK).
	 * If no action is set, it is assumed that the dialog's owner/caller will do something with the dialog's data.
	 */
	void SetExecutor(
		class vvProgressExecutor* pExecutor //< [in] The action to execute when OK is clicked.
		);

	/**
	 * Called to run validation on this window and possibly its children.
	 */
	virtual bool Validate();

	/**
	 * Gets the vvVerbs implementation that was passed to Create.
	 */
	class vvVerbs* GetVerbs() const;

	/**
	 * Gets the vvContext that was passed to Create.
	 */
	class vvContext* GetContext() const;

// event handlers
protected:
	void OnOk(wxCommandEvent& cEvent);

// private types
private:
	typedef std::list<vvValidatorReporter*> stlReporterList;
	typedef void (vvValidatorReporter::*NotifyFunction)(wxWindow* wWindow);

// private static functionality
private:
	/**
	 * Sets the wxWS_EX_VALIDATE_RECURSIVELY on a window and all of its children.
	 */
	static void SetValidateRecursiveFlag(
		wxWindow* wWindow //< [in] Window to set the flag on.
		);

	/**
	 * Calls a NotifyFunction on the reporters of any validator assigned to the given window.
	 * If the window uses wxWS_EX_VALIDATE_RECURSIVELY,
	 * then this function is also called on all of the window's children.
	 */
	static void NotifyReporters(
		wxWindow*        wWindow,    //< [in] Window to notify reporters in.
		stlReporterList& cReporters, //< [in] List of reporters that have already been notified.
		NotifyFunction   fNotify,    //< [in] The function to call on the reporters.
		wxWindow*        wParent     //< [in] The parent window to pass to the notification function.
		);

	/**
	 * Validates a given window against any validator it has assigned to it.
	 * If the window uses wxWS_EX_VALIDATE_RECURSIVELY,
	 * then this function is also called on all of the window's children.
	 */
	static bool ValidateWindow(
		wxWindow* wWindow, //< [in] The window to validate against its validator.
		wxWindow* wParent  //< [in] The parent window to pass to the validator.
		);

// private data
private:
	class vvVerbs*            mpVerbs;    //< The vvVerbs implementation that the dialog is using.
	class vvContext*          mpContext;  //< The vvContext that the dialog is using with mpVerbs.
	class vvProgressExecutor* mpExecutor; //< The action to execute when we're closed with wxID_OK.

// macro declarations
private:
	DECLARE_CLASS(vvDialog);
	DECLARE_EVENT_TABLE();
};


#endif
