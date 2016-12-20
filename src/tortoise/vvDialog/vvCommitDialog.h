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

#ifndef VV_COMMIT_DIALOG_H
#define VV_COMMIT_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include "vvRepoLocator.h"
#include "vvBranchControl.h"
#include "vvWorkItemsControl.h"

/**
 * A dialog box used to gather info to commit a working copy.
 */
class vvCommitDialog : public vvDialog
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvCommitDialog();

	/**
	 * Destructor.
	 */
	~vvCommitDialog();

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            wParent,                    //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxString&      sDirectory,                 //< [in] The directory of the working copy to commit from.
		class vvVerbs&       cVerbs,                     //< [in] The vvVerbs implementation to commit with.
		class vvContext&     cContext,                   //< [in] Context to use with pVerbs.
		const wxArrayString* pFiles      = NULL,         //< [in] Repo paths of dirty files to pre-select.
		                                                 //<      If empty, all dirty files will be pre-selected.
		                                                 //<      Passing NULL is equivalent to passing an empty list.
		const wxArrayString* pStamps     = NULL,         //< [in] List of stamps to pre-select.
		                                                 //<      Passing NULL is equivalent to passing an empty list.
		const wxString&      sComment    = wxEmptyString //< [in] The comment to pre-fill into the dialog.
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

	/**
	 * Gets the comment entered in the dialog.
	 */
	wxString GetComment() const;

	/**
	 * Gets the set of stamps that are checked in the dialog.
	 */
	const wxArrayString& GetSelectedStamps() const;

	/**
	 * Gets whether or not the Commit Submodules option is selected.
	 */
	bool GetCommitSubmodules() const;

	/**
	 * Gets the repo paths of files that are checked in the dialog.
	 */
	const wxArrayString& GetSelectedFiles(
		bool* pAll = NULL //< [out] Set to true if all available files are checked.
		                  //<       Set to false if some available files were left unchecked.
		) const;

	
	wxString GetBranchName() const;
	vvVerbs::BranchAttachmentAction vvCommitDialog::GetBranchAction() const;
	
	wxString GetOriginalBranchName() const;
	vvVerbs::BranchAttachmentAction GetBranchRestoreAction() const;

	wxArrayString GetWorkItemAssociations() const;
		
	// internal functionality
protected:
	/**
	 * Update the visible/enabled state of all controls, based on the current values.
	 */
	void UpdateDisplay();
	bool IsBranchTabDirty();

// event handlers
protected:
	void OnUpdateEvent(wxDataViewEvent& cEvent);
	void OnUpdateEvent(wxCommandEvent& cEvent);
	void OnUpdateEvent(wxUpdateUIEvent& cEvent);
	void OnAddStamp(wxCommandEvent& cEvent);
	void OnWorkItemSearch(wxCommandEvent& cEvent);

// private types
private:
	/**
	 * The set of tabs displayed in our notebook.
	 * Must occur in the same order as the tabs are declared in the XRC file!
	 */
	enum NotebookTab
	{
		TAB_FILES,
		TAB_STAMPS,
		TAB_BRANCH,
		TAB_WORK_ITEMS,
		TAB_COUNT,
	};

// private functionality
private:
	/**
	 * Sets the suffix to be displayed on a given tab.
	 */
	void SetTabSuffix(
		NotebookTab     eTab,   //< [in] The tab to set the suffix on.
		const wxString& sSuffix //< [in] The suffix to set.
		);

// private data
private:
	// internal data
	wxString      msDirectory;    //< The directory of the working copy to commit.
	wxString      msRepo;         //< The name of the repo to commit to.
	wxString      msComment;      //< The comment.
	wxArrayString mcStamps;       //< List of checked stamps.
	vvRepoLocator m_cRepoLocator; //< The repo locator.
	wxArrayString mcFiles;        //< Repo paths of selected dirty files.
	bool          mbAllFiles;     //< True if all possible files are selected, false if any are not.
	wxArrayString mcBaselineHIDs; //< The baseline HIDs of the working copy. Used by the branch control.

	// tab names
	wxArrayString mcTabNames; //< Base names of all the tabs in our notebook.

	// widgets
	class vvCurrentUserControl* mwUser;          //< Control displaying the current user.
	wxTextCtrl*                 mwComment;       //< Comment to apply to the new changeset.
	wxNotebook*                 mwNotebook;      //< Notebook containing the various tabs.
	class vvStatusControl*      mwFiles;         //< Allows the user to choose which files to commit.
	class vvStampsControl*      mwStamps;        //< List of stamps to apply to the new changeset.
	wxTextCtrl*                 mwStamps_New;    //< Text box for entering the name of a new stamp.
	wxButton*                   mwStamps_Add;    //< Button to add a new stamp to the list.
	vvBranchControl*            mwBranchControl; //< The control tha tlets users choose the branch to attach to.
	vvWorkItemsControl*         mwWorkItemsControl; //< The control tha tlets users choose the work items to associate with.
	wxComboBox*					mwWI_Repos;			//The combo box that will have the list of repositories.
	wxTextCtrl*					mwWI_Search;		//The text box for the work item search.
	wxArrayString				masAssociatedBugs;   //The bugs to associate with.
	class vvSelectableStaticText *	mwStamps_Explanation;

	// misc
	vvValidatorMessageBoxReporter mcValidatorReporter; //< Reporter that will display validation errors.

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvCommitDialog);
	DECLARE_EVENT_TABLE();
};


#endif
