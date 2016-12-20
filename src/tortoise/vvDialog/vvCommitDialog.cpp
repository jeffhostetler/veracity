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

#include "precompiled.h"
#include "vvCommitDialog.h"

#include "vvContext.h"
#include "vvCurrentUserControl.h"
#include "vvDialogHeaderControl.h"
#include "vvStampsControl.h"
#include "vvStatusControl.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorValueCompareConstraint.h"
#include "vvVerbs.h"
#include "vvRevisionLinkControl.h"
#include "vvWxHelpers.h"
#include "wx/persist.h"
#include "vvSelectableStaticText.h"
/*
**
** Globals
**
*/
wxString gsWISearchDiffBitmapName = "WISearch__search";

// style for the status control
const unsigned int guStatusControlStyle = vvStatusControl::STYLE_SELECT;

// style for the stamps control
const unsigned int guStampsControlStyle = vvStampsControl::STYLE_CHECKABLE;

// style for the stamps control
const unsigned int guStampsExplanationControlStyle = wxTE_MULTILINE | wxTE_NO_VSCROLL;

/**
 * Suffixes for tab names.
 */
static const wxString gsTabSuffix_Clean  = "";
static const wxString gsTabSuffix_Dirty  = " (*)";
static const wxString gsTabSuffix_Number = " (%d)";
static const wxString gsTabSuffix_Subset = " (%d/%d)";

/*
 * Hints for various text box controls.
 */
static const wxString gsHint_Comment    = "Enter a comment to associate with the new changeset.";
static const wxString gsHint_Stamps_New = "New stamp name";
static const wxString gsStamps_Explanation = "Adding a stamp here only adds it to the list at the left.  It will not actually be added to the repository until you check it and commit the changeset.";
/**
 * String displayed at the top of the validation error message box.
 */
static const wxString gsValidationErrorMessage = "There was an error in the information specified.  Please correct the following problems and try again.";

/*
 * Strings displayed as field labels when validation errors occur.
 */
static const wxString gsValidationField_Commit_Comment = "Comment";
static const wxString gsValidationField_Files          = "Files";
static const wxString gsValidationField_User           = "User";


/*
**
** Internal Functions
**
*/

/**
 * Checks if a path list contains a single entry that indicates the repo root.
 * Returns true if this is the case, or false otherwise.
 */
bool _ListContainsOnlyRepoRoot(
	const wxArrayString& cPaths //< [in] The list of paths to check.
	)
{
	// if we don't have exactly one entry, no match
	if (cPaths.size() != 1u)
	{
		return false;
	}

	// if the one entry exactly equals the repo root, match
	const wxString& cPath = cPaths.front();
	if (cPath == vvVerbs::sszRepoRoot)
	{
		return true;
	}

	// if the entry doesn't have one more character than the repo root, no match
	size_t uLength = strlen(vvVerbs::sszRepoRoot);
	if (cPath.Length() != uLength + 1u)
	{
		return false;
	}

	// if the entry's extra character isn't a patch separator, no match
	if (wxFileName::GetPathSeparators().Index(cPath[uLength]) == 0)
	{
		return false;
	}

	// must be one entry with a trailing path separator, match
	return true;
}


/*
**
** Public Functions
**
*/

vvCommitDialog::vvCommitDialog()
	: mbAllFiles(true)
	, mwUser(NULL)
	, mwComment(NULL)
	, mwNotebook(NULL)
	, mwFiles(NULL)
	, mwStamps(NULL)
	, mwStamps_New(NULL)
	, mwStamps_Add(NULL)
	, mwBranchControl(NULL)
{
}

vvCommitDialog::~vvCommitDialog()
{
	if (this->mwFiles != NULL)
	{
		delete this->mwFiles;
	}
}

bool vvCommitDialog::Create(
	wxWindow*            wParent,
	const wxString&      sDirectory,
	vvVerbs&             cVerbs,
	vvContext&           cContext,
	const wxArrayString* pFiles,
	const wxArrayString* pStamps,
	const wxString&      sComment
	)
{
	// create the dialog window
	if (!vvDialog::Create(wParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// look up the repo associated with the given working copy
	if (!cVerbs.FindWorkingFolder(cContext, sDirectory, NULL, &this->msRepo, &mcBaselineHIDs))
	{
		cContext.Error_ResetReport("Cannot find the working copy for path: %s", sDirectory);
		return false;
	}

	// store the given initialization data
	this->msDirectory = sDirectory;
	this->msComment   = sComment;
	if (pStamps != NULL)
	{
		this->mcStamps.assign(pStamps->begin(), pStamps->end());
	}
	if (pFiles == NULL || _ListContainsOnlyRepoRoot(*pFiles))
	{
		this->mbAllFiles = true;
	}
	else
	{
		this->mbAllFiles = false;
		this->mcFiles.assign(pFiles->begin(), pFiles->end());
	}

	// get all of our widgets
	this->mwComment       = XRCCTRL(*this, "wComment",    wxTextCtrl);
	this->mwNotebook      = XRCCTRL(*this, "wNotebook",   wxNotebook);
	this->mwStamps_New    = XRCCTRL(*this, "wStamps_New", wxTextCtrl);
	this->mwStamps_Add    = XRCCTRL(*this, "wStamps_Add", wxButton);
	this->mwWI_Repos      = XRCCTRL(*this, "wWI_Repos", wxComboBox);
	this->mwWI_Search     = XRCCTRL(*this, "wWorkItemSearch", wxTextCtrl);
	XRCCTRL(*this, "wWorkItemSearchButton", wxBitmapButton)->SetBitmap(wxArtProvider::GetBitmap(gsWISearchDiffBitmapName));
	// create our header control
	vvDialogHeaderControl* wHeader = new vvDialogHeaderControl(this, wxID_ANY, this->msDirectory, cVerbs, cContext);

	vvRepoLocator cRepo = vvRepoLocator::FromRepoNameAndWorkingFolderPath(this->msRepo, this->msDirectory);
	
	
	// put the baseline hids in the header.
	for (wxArrayString::const_iterator it = mcBaselineHIDs.begin(); it != mcBaselineHIDs.end(); it++)
	{
		vvRevisionLinkControl * pRevLink = new vvRevisionLinkControl(0, *it, wHeader, wxID_ANY, cRepo, *this->GetVerbs(), *this->GetContext(), false, true);
		if (it == mcBaselineHIDs.begin())
			wHeader->AddControlValue("Parent:", pRevLink, true);
		else
			wHeader->AddControlValue("Merging:", pRevLink, true);
	}
	
	// create our user control for the header
	this->mwUser = new vvCurrentUserControl(wHeader, wxID_ANY, &cRepo, &cVerbs, &cContext);
	wHeader->AddControlValue("User:", this->mwUser, true);

	// attach the header
	wxXmlResource::Get()->AttachUnknownControl("wHeader", wHeader);

	// create our status control
	this->mwFiles = new vvStatusControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, guStatusControlStyle, vvStatusControl::MODE_COLLAPSED, vvStatusControl::ITEM_DEFAULT, vvStatusControl::STATUS_DEFAULT, vvStatusControl::STATUS_FOUND | vvStatusControl::STATUS_LOCKED | vvStatusControl::STATUS_LOCK_CONFLICT);
	wxXmlResource::Get()->AttachUnknownControl("wFiles", this->mwFiles);

	// create our stamps control
	this->mwStamps = new vvStampsControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, guStampsControlStyle);
	wxXmlResource::Get()->AttachUnknownControl("wStamps", this->mwStamps);

	// create the stamps explanation control
	this->mwStamps_Explanation = new vvSelectableStaticText(this, wxID_ANY, gsStamps_Explanation, wxDefaultPosition, wxDefaultSize, guStampsExplanationControlStyle);
	wxXmlResource::Get()->AttachUnknownControl("wStamps_Explanation", this->mwStamps_Explanation);
	this->mwStamps_Explanation->SetBackgroundColour(mwNotebook->GetThemeBackgroundColour());

	// configure controls
	this->mwComment->SetHint(gsHint_Comment);
	this->mwStamps_New->SetHint(gsHint_Stamps_New);

	// create the branch control
	this->mwBranchControl = new vvBranchControl(this, msRepo, &mcBaselineHIDs, &cVerbs, &cContext);
	this->mwBranchControl->MSWSetTransparentBackground(true);
	wxXmlResource::Get()->AttachUnknownControl("wBranchControl", this->mwBranchControl);
	this->mwBranchControl->SetBackgroundColour(mwNotebook->GetThemeBackgroundColour());

	// create the work items control
	this->mwWorkItemsControl = new vvWorkItemsControl(this, wxID_ANY, &cVerbs, &cContext, wxDefaultPosition, wxDefaultSize, vvWorkItemsControl::STYLE_CHECKABLE);
	wxXmlResource::Get()->AttachUnknownControl("vvWorkItemsControl", this->mwWorkItemsControl);
	//Use the wxPersistenceManager to remember column widths, and the previously selected repo.
	wxPersistenceManager::Get().RegisterAndRestore(this->mwWorkItemsControl);

	// get the names of the various tabs
	wxASSERT(this->mwNotebook->GetPageCount() == TAB_COUNT);
	mcTabNames.resize(TAB_COUNT);
	mcTabNames[TAB_FILES]      = this->mwNotebook->GetPageText(TAB_FILES);
	mcTabNames[TAB_STAMPS]     = this->mwNotebook->GetPageText(TAB_STAMPS);
	mcTabNames[TAB_BRANCH]     = this->mwNotebook->GetPageText(TAB_BRANCH);
	mcTabNames[TAB_WORK_ITEMS] = this->mwNotebook->GetPageText(TAB_WORK_ITEMS);

	// setup validators
	this->mcValidatorReporter.SetHelpMessage(gsValidationErrorMessage);
	this->mwComment->SetValidator(vvValidator(gsValidationField_Commit_Comment)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);
	this->mwFiles->SetValidator(vvValidator(gsValidationField_Files)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);
	this->mwUser->SetValidator(vvValidator(gsValidationField_User)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);

	// HACK: update the splitter's sash position to force it to re-layout the notebook tabs
	//       otherwise for some reason the status control isn't expanded to fill its tab
	wxSplitterWindow* wSplitter = XRCCTRL(*this, "wSplitter", wxSplitterWindow);
	int iOldPosition = wSplitter->GetSashPosition();
	wSplitter->SetSashPosition(iOldPosition + 1);
	wSplitter->SetSashPosition(iOldPosition);

	// focus on the comment control
	this->mwComment->SetFocus();

	// success
	return true;
}

bool vvCommitDialog::TransferDataToWindow()
{
	// ----- COMMENT -----

	// set the comment
	this->mwComment->SetValue(this->msComment);

	// retrieve status data and populate the file list
	m_cRepoLocator = vvRepoLocator::FromRepoNameAndWorkingFolderPath(this->msRepo, this->msDirectory);
	if (!this->mwFiles->FillData(*this->GetVerbs(), *this->GetContext(), m_cRepoLocator))
	{
		this->GetContext()->Error_ResetReport("Unable to populate status data.");
	}

	// ----- FILES -----

	// expand all of the files
	this->mwFiles->ExpandAll();

	// select the appropriate files
	if (this->mbAllFiles)
	{
		this->mwFiles->SetItemSelectionAll(true);
	}
	else if (this->mwFiles->SetItemSelectionByPath(this->mcFiles, true) == 0u)
	{
		this->mwFiles->SetItemSelectionAll(true);
	}

	// ----- STAMPS -----

	// populate the stamps control
	this->mwStamps->AddItems(*this->GetVerbs(), *this->GetContext(), this->msRepo);

	// pre-check any items in our list
	for (wxArrayString::const_iterator it = this->mcStamps.begin(); it != this->mcStamps.end(); ++it)
	{
		wxDataViewItem cItem = this->mwStamps->GetItemByName(*it);
		if (cItem.IsOk())
		{
			this->mwStamps->CheckItem(cItem);
		}
	}

	// ----- BRANCH -----
	wxString sOrigBranch;
	if (!this->GetVerbs()->GetBranch(*this->GetContext(), m_cRepoLocator.GetWorkingFolder(), sOrigBranch))
	{
		this->GetContext()->Error_ResetReport("Unable to retrieve current branch.");
	}
	this->mwBranchControl->TransferDataToWindow(sOrigBranch);


	wxArrayString aRepos;
	if (!this->GetVerbs()->GetLocalRepos(*this->GetContext(), aRepos))
	{
		this->GetContext()->Error_ResetReport("Unable to retrieve the list of local repositories.");
	}
	this->mwWI_Repos->Append(aRepos);
	//Try to select the last one they used.
	wxPersistenceManager::Get().RegisterAndRestore(this->mwWI_Repos);
	// ----- END -----

	// update our display to reflect any widget changes
	this->UpdateDisplay();

	// success
	return true;
}

bool vvCommitDialog::TransferDataFromWindow()
{
	// get the comment
	this->msComment = this->mwComment->GetValue();

	// get the selected files
	vvStatusControl::stlItemDataList cStatusItems;
	this->mwFiles->GetItemData(&cStatusItems, vvStatusControl::COLUMN_FILENAME);
	this->mcFiles.clear();
	this->mbAllFiles = true;
	for (vvStatusControl::stlItemDataList::const_iterator it = cStatusItems.begin(); it != cStatusItems.end(); ++it)
	{
		if (it->bSelected)
		{
			this->mcFiles.Add(it->sRepoPath);
		}
		else if (it->eStatus != vvStatusControl::STATUS_FOUND && it->eStatus != vvStatusControl::STATUS_LOCKED) //Found files can't be selected.
		{
			this->mbAllFiles = false;
		}
	}

	// get the checked stamps
	this->mcStamps.clear();
	vvStampsControl::stlItemDataList cStampItems;
	this->mwStamps->GetItemData(&cStampItems, true);
	for (vvStampsControl::stlItemDataList::const_iterator it = cStampItems.begin(); it != cStampItems.end(); ++it)
	{
		// we asked for only checked items, so they'd better all be checked
		wxASSERT(it->bChecked);
		this->mcStamps.Add(it->sName);
	}
	
	wxString sRepo = this->mwWI_Repos->GetValue();
	vvWorkItemsControl::stlItemDataList items;
	this->mwWorkItemsControl->GetItemData(&items, true);
	masAssociatedBugs.Clear();

	for (vvWorkItemsControl::stlItemDataList::const_iterator it = items.begin(); it != items.end(); it++)
	{
		masAssociatedBugs.push_back(sRepo + ":" + it->sID);
	}


	//If they are creating a new branch, prompt to make sure 
	//they want to create it.
	return this->mwBranchControl->TransferDataFromWindow();
}

wxString vvCommitDialog::GetComment() const
{
	return this->msComment;
}

const wxArrayString& vvCommitDialog::GetSelectedStamps() const
{
	return this->mcStamps;
}

wxArrayString vvCommitDialog::GetWorkItemAssociations() const
{
	return this->masAssociatedBugs;
}

const wxArrayString& vvCommitDialog::GetSelectedFiles(
	bool* pAll
	) const
{
	if (pAll != NULL)
	{
		*pAll = this->mbAllFiles;
	}
	return this->mcFiles;
}

wxString vvCommitDialog::GetBranchName() const
{
	return this->mwBranchControl->GetBranchName();
}

vvVerbs::BranchAttachmentAction vvCommitDialog::GetBranchAction() const
{
	return this->mwBranchControl->GetBranchAction();
}

wxString vvCommitDialog::GetOriginalBranchName() const
{
	return this->mwBranchControl->GetOriginalBranchName();
}

vvVerbs::BranchAttachmentAction vvCommitDialog::GetBranchRestoreAction() const
{
	return this->mwBranchControl->GetBranchRestoreAction();
}

void vvCommitDialog::UpdateDisplay()
{
	// update the Files tab's suffix
	unsigned int uFileCount         = this->mwFiles->GetItemData(NULL, 0u);
	unsigned int uSelectedFileCount = this->mwFiles->GetItemData(NULL, 0u, true);
	this->SetTabSuffix(TAB_FILES, wxString::Format(gsTabSuffix_Subset, uSelectedFileCount, uFileCount));

	// update the Stamps tab's suffix
	unsigned int uStampCount = this->mwStamps->GetItemData(NULL, true);
	this->SetTabSuffix(TAB_STAMPS, wxString::Format(gsTabSuffix_Number, uStampCount));

	// Update the Branch tab's suffix to show a star if it's changed
	this->SetTabSuffix(TAB_BRANCH, mwBranchControl->IsDirty() ? gsTabSuffix_Dirty : gsTabSuffix_Clean);

	// update the Work Item tab's suffix
	unsigned int uWorkItemCount = this->mwWorkItemsControl->GetItemData(NULL, true);
	this->SetTabSuffix(TAB_WORK_ITEMS, wxString::Format(gsTabSuffix_Number, uWorkItemCount));
}

void vvCommitDialog::OnUpdateEvent(
	wxDataViewEvent& WXUNUSED(cEvent)
	)
{
	this->UpdateDisplay();
}

void vvCommitDialog::OnUpdateEvent(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	this->UpdateDisplay();
}

void vvCommitDialog::OnUpdateEvent(
	wxUpdateUIEvent& WXUNUSED(cEvent)
	)
{
	//This is only fired by the BranchControl, so only update its tab.
	this->SetTabSuffix(TAB_BRANCH, mwBranchControl->IsDirty() ? gsTabSuffix_Dirty : gsTabSuffix_Clean);
}

void vvCommitDialog::OnAddStamp(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	// if they didn't type in a stamp name, don't bother
	wxString sStamp = this->mwStamps_New->GetValue();
	if (sStamp.IsEmpty())
	{
		return;
	}

	// check if the item is in the list already
	wxDataViewItem cItem = this->mwStamps->GetItemByName(sStamp);
	if (cItem.IsOk())
	{
		// in the list already, check it
		this->mwStamps->CheckItem(cItem);
	}
	else
	{
		// not in the list yet, add it
		cItem = this->mwStamps->AddItem(sStamp, 0u, true);
	}

	// select and scroll to the item
	this->mwStamps->EnsureVisible(cItem);
	this->mwStamps->Select(cItem);

	// clear the new stamp text control
	this->mwStamps_New->SetValue(wxEmptyString);

	// update the display accordingly
	this->UpdateDisplay();
}

void vvCommitDialog::OnWorkItemSearch(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	wxBusyCursor cur;
	this->mwWorkItemsControl->AddItems(*this->GetVerbs(), *this->GetContext(), this->mwWI_Repos->GetValue(), this->mwWI_Search->GetValue(), NULL);
}

void vvCommitDialog::SetTabSuffix(
	NotebookTab     eTab,
	const wxString& sSuffix
	)
{
	this->mwNotebook->SetPageText(eTab, this->mcTabNames[eTab] + sSuffix);
}

IMPLEMENT_DYNAMIC_CLASS(vvCommitDialog, vvDialog);

BEGIN_EVENT_TABLE(vvCommitDialog, vvDialog)
	EVT_DATAVIEW_ITEM_VALUE_CHANGED(XRCID("wFiles"),                OnUpdateEvent)
	EVT_DATAVIEW_ITEM_VALUE_CHANGED(XRCID("wStamps"),               OnUpdateEvent)
	EVT_DATAVIEW_ITEM_VALUE_CHANGED(XRCID("vvWorkItemsControl"),    OnUpdateEvent)
	EVT_BUTTON(                     XRCID("wStamps_Add"),           OnAddStamp)
	EVT_TEXT_ENTER(                 XRCID("wStamps_New"),           OnAddStamp)
	EVT_UPDATE_UI(                  XRCID("wBranchControl"),        OnUpdateEvent)
	EVT_TEXT_ENTER(                 XRCID("wWorkItemSearch"),       OnWorkItemSearch)
	EVT_BUTTON(		                XRCID("wWorkItemSearchButton"), OnWorkItemSearch)
END_EVENT_TABLE()
