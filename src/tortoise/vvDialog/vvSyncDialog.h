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

#ifndef VV_SYNC_DIALOG_H
#define VV_SYNC_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include <wx/dirctrl.h>
#include <wx/combo.h>
#include "vv_wxListViewComboPopup.h"
#include "vvHistoryListControl.h"
#include "vvSyncTargetsControl.h"

/**
 * This dialog is responsible for popping up
 * a wxTextEntryDialog.
 */
class vvSyncDialog : public vvDialog
{
// events
protected:
	void vvSyncDialog::OnComboBoxSelectionChanged(wxCommandEvent& cEvent);
	void vvSyncDialog::OnBrowseButtonPressed(wxCommandEvent& cEvent);
	void vvSyncDialog::OnTextInDagsBox(wxCommandEvent& cEvent);
// functionality
public:
	/**
	 * Actions the user might select to .
	 */
	enum SyncAction
	{
		SYNC_PULL,			//< User chose not to create a new repo.  Clone source is not valid.
		SYNC_PUSH,			//< User chose to create a new blank repo.  Clone source is not valid.
		SYNC_PULL_THEN_PUSH //< User chose to clone a local repo.  The clone source will be the name.
	};

	enum RevisionMode
	{
		REV_MODE_ALL,			//< transfer all revisions
		REV_MODE_BRANCHES,		//< transfer revisions by named branch.
		REV_MODE_TAGS,		//< transfer revisions by tags.
		REV_MODE_CHANGESETS,		//< transfer revisions by changeset HIDs.
		REV_MODE_UNAVAILABLE	//< The user is not syncing version control, so we can't pick and choose revisions.
	};
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            pParent,                         //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxString&      sInitialValue,                   //< [in] One of the items to move.  If it's the only item, its name will be displayed.  Otherwise, it will be used to set the initial selection in the dialog.
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

	wxString GetOtherRepo();

	wxString GetMyRepoName();

	SyncAction GetSyncAction();

	RevisionMode GetRevisionMode();

	wxArrayString GetAreas();

	wxArrayString GetChangesets();

	bool GetUpdateAfterPull();

	void LoadRevisionsIfNecessary();

	void ShowOrHideUpdateControl();

	void UpdateDisplay();
	void OnControlValueChange(wxCommandEvent& cEvent);

// private data
private:

	/**
	 * Get the new name from the dialog.
	 **/
	SyncAction gui_GetSyncAction();
	
	void vvSyncDialog::FitHeight();

	void PopulateDagsBox();

	// internal data
	vvRepoLocator m_cRepoLocator;
	wxString     msNewPath;        //< The new name 
	wxComboBox * mwComboBox;
	wxButton * mwOKButton;
	wxCheckBox * mwCheckBoxUpdate;
	wxButton * mwDagBrowseButton;
	wxString msPathProvided;
	vvSyncTargetsControl * mwComboBoxRepos;
	wxTextCtrl * mwDags;
	vvHistoryListControl * m_revisionsControl;
	
	wxString msMyRepoName;
	wxString msOtherRepoSpec;
	SyncAction mSelectedOperation;
	bool bSyncingVersionControl;
	wxColor m_cCachedBackgroundColor;

	wxArrayString mcAreas_all;
	wxArrayString mcAreas_selected;
	wxArrayString mcChangesets_selected;
	RevisionMode mRevisionMode;
	wxString m_sRepoName;
	vvValidatorMessageBoxReporter mcValidatorReporter; //< Reporter that will display validation errors.
	bool m_bDoPull;
	wxString m_sPreviousRepoValue;
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvSyncDialog);
	DECLARE_EVENT_TABLE();
};


#endif
