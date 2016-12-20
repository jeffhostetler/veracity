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

#ifndef VV_RESOLVE_DIALOG_H
#define VV_RESOLVE_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvVerbs.h"

/**
 * A dialog box used to display and manipulate conflicts on an item.
 */
class vvResolveDialog : public vvDialog
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvResolveDialog();

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*                   wParent,       //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxString&             sAbsolutePath, //< [in] Absolute path of the item to display/resolve a conflict from.
		vvVerbs::ResolveChoiceState eState,        //< [in] The type of conflict from the item to display/resolve.
		                                           //<      RESOLVE_STATE_COUNT may be specified if the item only has one conflict.
		class vvVerbs&              cVerbs,        //< [in] The vvVerbs implementation to use.
		class vvContext&            cContext       //< [in] Context to use with pVerbs.
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
	 * Refreshes our list of values.
	 * After the refresh, the previously selected values are selected.
	 * Returns true if the refresh/update were successful or false otherwise.
	 */
	bool RefreshData();

	/**
	 * Refreshes our list of values.
	 * After the refresh, the value with the specified label is selected.
	 * Returns true if the refresh/update were successful or false otherwise.
	 */
	bool RefreshData(
		const wxString& sLabel
		);

	/**
	 * Refreshes our list of values.
	 * After the refresh, the values with the specified labels are selected.
	 * Returns true if the refresh/update were successful or false otherwise.
	 */
	bool RefreshData(
		const wxArrayString& sLabels
		);

	/**
	 * Gets resolve data about the currently selected value.
	 * Returns NULL if there is not exactly one value selected.
	 */
	vvVerbs::ResolveValue* GetSelectedValue();

	/**
	 * Gets the set of selected values.
	 * Returns the number of selected values.
	 */
	size_t GetSelectedValues(
		wxArrayPtrVoid* pValues = NULL //< [out] Filled with the currently selected values.
		                               //<       Values are of type vvVerbs::ResolveValue*.
		                               //<       May pass NULL if only the count is required.
		);

	/**
	 * Gets the labels of all currently selected values.
	 * Returns the number of selected values.
	 */
	size_t GetSelectedValueLabels(
		wxArrayString* pLabels = NULL //< [out] Filled with the labels of the currently selected values.
		                              //<       May pass NULL if only the count is required.
		);

	/**
	 * Updates the dialog to reflect UI changes (like selection).
	 */
	void UpdateDisplay();

	/**
	 * Runs a diff on the given values using the currently selected difftool.
	 */
	void DiffValues(
		const wxString& sLeft,
		const wxString& sRight
		);

// event handlers
protected:
	void OnUpdate(wxListEvent& e);
	void OnRelated(wxHyperlinkEvent& e);
	void OnDiff_Selected(wxCommandEvent& e);
	void OnDiff_Menu(wxCommandEvent& e);
	void OnDiff_Pair(wxCommandEvent& e);
	void OnDiff_Complete(wxThreadEvent& e);
	void OnMerge(wxCommandEvent& e);
	void OnMerge_Complete(wxThreadEvent& e);
	void OnAccept(wxCommandEvent& e);
	void OnCancel(wxCommandEvent& e);

// private functionality
private:
	/**
	 * Retrieves new resolve data to be displayed.
	 */
	bool GetResolveData();

	/**
	 * Stores a thread as the current one and runs it.
	 */
	bool RunThread(
		wxThread* pThread //< [in] The thread to store, create, and run.
		                  //<      Function assumes ownership of the pointer.
		);

// private data
private:
	// setup data
	wxString                     msFolder; //< Root of the working folder we're resolving in.
	wxString                     msGid;    //< GID of the item we're resolving on.
	vvVerbs::ResolveChoiceState  meState;  //< Type of the conflict we're resolving on the item.

	// display data
	vvVerbs::ResolveItem         mcItem;   //< Resolve data for the item we're resolving on.
	vvVerbs::ResolveChoice*      mpChoice; //< Resolve data for the choice we're resolving on.  Points into mcItem.
	wxArrayString                mcValues; //< Labels of the selected values.

	// helper data
	wxThread*                    mpThread; //< Thread that is currently running a diff or merge.
	                                       //< It'd be nice to be able to do both at once, but they
	                                       //< both lock down the pendingtree while in progress, so
	                                       //< that won't be possible until WC is done.

	// widgets
	class vvDialogHeaderControl* mwHeader;
	wxListCtrl*                  mwValues;
	wxButton*                    mwDiff;
	wxMenu                       mwDiffMenu;
	wxButton*                    mwMerge;
	wxButton*                    mwAccept;

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvResolveDialog);
	DECLARE_EVENT_TABLE();
};

#endif
