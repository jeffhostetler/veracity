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
#include "vvSyncDialog.h"

#include "vvVerbs.h"
#include "vvContext.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvProgressExecutor.h"
#include "vvDialogHeaderControl.h"
#include "vvHistoryListControl.h"
#include "vvInOutCommand.h"
#include "vvWxHelpers.h"
#include "wx/persist.h"

/*
**
** Globals
**
*/

static const wxString gsValidationField_Repo                     = "Other Repository";
static const wxString gsValidationField_Revision_Branch_Value    = "Branches";
static const wxString gsValidationField_Revision_Tag_Value       = "Tags";
static const wxString gsValidationField_Revision_Changeset_Value = "Changesets";

static const wxString gsHint_Repos                 = "Local repository name, or remote repository URL";
static const wxString gsHint_Dags			       = "All areas will be transferred";
static const wxString gsHint_Revision_Branch_Value    = "Name of the branches to transfer";
static const wxString gsHint_Revision_Tag_Value       = "Name of the tags to transfer";
static const wxString gsHint_Revision_Changeset_Value = "ID of the changesets to transfer";

/*
**
** Public Functions
**
*/

bool vvSyncDialog::Create(
	wxWindow*            pParent,
	const wxString&      sInitialValue,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}
	// create our header control
	wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, sInitialValue, cVerbs, cContext));

	this->mwComboBox						  = XRCCTRL(*this, "wxOperation",                 wxComboBox);
	this->mwOKButton						  = XRCCTRL(*this, "wxID_OK",                 wxButton);
	this->mwDagBrowseButton					  = XRCCTRL(*this, "wDagBrowse",                 wxButton);
	this->mwDags							  = XRCCTRL(*this, "wDags",                 wxTextCtrl);
	this->mwCheckBoxUpdate					  = XRCCTRL(*this, "wCheckBox_Update",                 wxCheckBox);
	this->msPathProvided = sInitialValue;
	
	this->mwComboBoxRepos = new vvSyncTargetsControl(this, wxID_ANY);

	wxXmlResource::Get()->AttachUnknownControl("wxRepos", mwComboBoxRepos);


	bSyncingVersionControl = true;

	this->mwComboBoxRepos			->SetHint(gsHint_Repos);
	this->mwDags					->SetHint(gsHint_Dags);
	
	this->mwComboBoxRepos->SetValidator(vvValidator(gsValidationField_Repo)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);

	wxPersistenceManager::Get().RegisterAndRestore(mwCheckBoxUpdate);

	wxString psRepoDescriptorName;
	cVerbs.FindWorkingFolder(cContext, msPathProvided, NULL, &psRepoDescriptorName);
	m_sRepoName = wxString(psRepoDescriptorName);
	m_cRepoLocator = vvRepoLocator::FromRepoNameAndWorkingFolderPath(psRepoDescriptorName, msPathProvided);
	m_revisionsControl = new vvHistoryListControl(cVerbs, cContext, m_cRepoLocator, this, wxID_ANY);
	wxXmlResource::Get()->AttachUnknownControl("wRevisions", m_revisionsControl, this);

	//XRCCTRL(*this, "wHeader",                 vvDialogHeaderControl)->SetBackgroundColour("blue");
	//m_revisionsControl->Show(false);
	m_revisionsControl->SetMinSize(wxSize(-1,200));
	//vvXRCSIZERITEM(*this, "cRevision")->Show(false);
	this->SetMinSize(wxSize(400,265));
	FitHeight();
	return true;
}

bool vvSyncDialog::TransferDataToWindow()
{ 
	vvContext&     pCtx = *this->GetContext();
	vvVerbs*   pVerbs   = this->GetVerbs();
	
	this->mwComboBoxRepos->Populate(pCtx, *pVerbs, m_sRepoName);

	this->m_revisionsControl->ApplyColumnPreferences();
	UpdateDisplay();
	
	pVerbs->ListAreas(pCtx, m_sRepoName, mcAreas_all);
	return true;
}

bool vvSyncDialog::TransferDataFromWindow()
{
	// success
	this->msOtherRepoSpec = this->mwComboBoxRepos->GetValue();
	this->mSelectedOperation = gui_GetSyncAction();
	if (!bSyncingVersionControl)
	{
		mRevisionMode = REV_MODE_ALL;
	}
	else
	{
		if (XRCCTRL(*this, "wRevision_Select_One", wxRadioButton)	->GetValue())
		{
			mcChangesets_selected = m_revisionsControl->GetSelection();
		}
		mRevisionMode = REV_MODE_CHANGESETS;
		/*if (XRCCTRL(*this, "wRevision_Branch_Select", wxRadioButton)	->GetValue())
			mRevisionMode = REV_MODE_BRANCHES;
		else if (XRCCTRL(*this, "wRevision_Tag_Select", wxRadioButton)	->GetValue())
			mRevisionMode = REV_MODE_TAGS;
		else if (XRCCTRL(*this, "wRevision_Changeset_Select", wxRadioButton)	->GetValue())
			mRevisionMode = REV_MODE_CHANGESETS;
			*/
	}
	return true;
}

vvSyncDialog::SyncAction vvSyncDialog::GetSyncAction()
{
	return mSelectedOperation;
}

wxString vvSyncDialog::GetOtherRepo()
{
	return msOtherRepoSpec;
}

wxString vvSyncDialog::GetMyRepoName()
{
	return m_sRepoName;
}

bool vvSyncDialog::GetUpdateAfterPull()
{
	return GetSyncAction() == SYNC_PULL && this->mwCheckBoxUpdate->GetValue();
}

vvSyncDialog::SyncAction vvSyncDialog::gui_GetSyncAction()
{
	return (SyncAction)this->mwComboBox->GetCurrentSelection();
}

//////////////////////////////Events//////////////////
void vvSyncDialog::OnComboBoxSelectionChanged(
	wxCommandEvent& cEvent
	)
{
	if (cEvent.GetEventObject() == this->mwComboBox)
		this->mwOKButton->SetLabel(this->mwComboBox->GetValue());
	ShowOrHideUpdateControl();
	//If they've changed the text of the other repo,
	//we need to flip the revision selection to "sync all revisions"
	if (mwComboBoxRepos->GetValue() != m_sPreviousRepoValue)
	{
		XRCCTRL(*this, "wRevision_All_Select", wxRadioButton)->SetValue(true);
		m_sPreviousRepoValue = mwComboBoxRepos->GetValue();
	}
	UpdateDisplay();
}

void vvSyncDialog::ShowOrHideUpdateControl()
{
	if (gui_GetSyncAction() != SYNC_PULL || bSyncingVersionControl == false)
	{
		//They aren't doing a pull.  Always hide
		vvXRCSIZERITEM(*this, "cUpdate")->Show(false);
	}
	else
	{
		vvXRCSIZERITEM(*this, "cUpdate")->Show(true);
	}
	if (gui_GetSyncAction() == SYNC_PULL_THEN_PUSH || bSyncingVersionControl == false)
	{
		vvXRCSIZERITEM(*this, "cRevision")->Show(false);
	}
	else
	{
		vvXRCSIZERITEM(*this, "cRevision")->Show(true);
	}

	this->FitHeight();
}

void vvSyncDialog::LoadRevisionsIfNecessary()
{
	if (m_revisionsControl->IsEnabled())
	{
		wxBusyCursor cur;
		wxRadioButton* wButton = XRCCTRL(*this, "wRevision_Select_One", wxRadioButton);
		static wxString sButtonLabel = wButton->GetLabelText();
		wButton->SetLabelText(sButtonLabel + " (Loading...)");
		m_bDoPull = gui_GetSyncAction() == SYNC_PULL;
		//Reset the list to empty.
		vvVerbs::HistoryData emptyList;
		m_revisionsControl->LoadHistory(emptyList);
		vvContext NewContext;
		vvVerbs::HistoryData cHistoryResults;
		vvInOutCommand command = vvInOutCommand(gui_GetSyncAction() == SYNC_PULL, m_sRepoName, this->mwComboBoxRepos->GetValue());
		command.SetVerbs(this->GetVerbs());
		command.SetContext(this->GetContext());
		vvProgressExecutor executor(command);
		bool bIsRemote = false;
		this->GetVerbs()->CheckOtherRepo_IsItRemote(*this->GetContext(), this->mwComboBoxRepos->GetValue(), bIsRemote);
		if (bIsRemote)
			executor.SetTimeout(500u);
		//else
		//	executor.SetTimeout(10000u);
		executor.Execute(vvFindTopLevelParent(this));
		cHistoryResults = command.GetResults();
		m_revisionsControl->LoadHistory(cHistoryResults, gui_GetSyncAction() == SYNC_PULL); //Incoming changes need to pass the RevisionsAreRemote flag.
		wButton->SetLabelText(wxString::Format("%s (%u %s)", sButtonLabel, (unsigned int)cHistoryResults.size(), m_bDoPull ? "incoming" : "outgoing"));
	}
}

void vvSyncDialog::UpdateDisplay()
{
	//If they've changed the text of the other repo,
	//we need to flip the revision selection to "sync all revisions"
	if (mwComboBoxRepos->GetValue() != m_sPreviousRepoValue)
	{
		XRCCTRL(*this, "wRevision_All_Select", wxRadioButton)->SetValue(true);
		m_sPreviousRepoValue = mwComboBoxRepos->GetValue();
	}

	if (XRCCTRL(*this, "wRevision_Select_One", wxRadioButton)->GetValue())
	{
		
		m_revisionsControl->Enable(true);
		m_revisionsControl->SetListBackgroundColour(m_cCachedBackgroundColor);
	}
	else
	{
		//Save the original background color, to restore it when we enable.
		m_cCachedBackgroundColor = m_revisionsControl->GetListBackgroundColour();
		m_revisionsControl->SetListBackgroundColour(this->GetBackgroundColour());
		//Reset the list to empty.
		vvVerbs::HistoryData emptyList;
		m_revisionsControl->LoadHistory(emptyList);
		m_revisionsControl->Enable(false);
	}
	LoadRevisionsIfNecessary();
	/*
	this->mwBranches   ->Enable(XRCCTRL(*this, "wRevision_Branch_Select", wxRadioButton)	->GetValue());
	this->mwTags      ->Enable(XRCCTRL(*this, "wRevision_Tag_Select", wxRadioButton)     ->GetValue());
	this->mwChangesets->Enable(XRCCTRL(*this, "wRevision_Changeset_Select", wxRadioButton)		->GetValue());
	this->mwButtonBrowseBranches->Enable(XRCCTRL(*this, "wRevision_Branch_Select", wxRadioButton)	->GetValue());
	this->mwButtonBrowseTags	->Enable(XRCCTRL(*this, "wRevision_Tag_Select", wxRadioButton)     ->GetValue());
	this->mwButtonBrowseChangesets->Enable(XRCCTRL(*this, "wRevision_Changeset_Select", wxRadioButton)		->GetValue());
	*/
}
void vvSyncDialog::OnControlValueChange(
	wxCommandEvent& cEvent
	)
{
	cEvent;
	UpdateDisplay();
}

void vvSyncDialog::OnTextInDagsBox(
	wxCommandEvent& cEvent
	)
{
	cEvent;
	if (!mwDags->GetValue().IsEmpty() && mcAreas_selected.Count() == 0)
		mwDags->Clear();
	else if (mcAreas_selected.Count() != 0)
	{
		//There might be text from our dag selection.
		PopulateDagsBox();
	}
}
void vvSyncDialog::PopulateDagsBox()
{
	wxString value;
	if (mcAreas_selected.Count() == 0)
		return;
	for (size_t index = 0; index < mcAreas_selected.Count(); index++)
	{
		if (!value.IsEmpty())
			value += ", ";
		value += mcAreas_selected[index];
	}
	if (!value.IsEmpty() && value != mwDags->GetValue())
		mwDags->SetValue(value);
}
void vvSyncDialog::OnBrowseButtonPressed(
	wxCommandEvent& cEvent
	)
{
	if (cEvent.GetEventObject() == this->mwDagBrowseButton)
	{
		{
			
			// create a dialog to allow the user to choose one of them
			wxMultiChoiceDialog cDialog;
			{
				if (!cDialog.Create(this, "Select the areas that you wish to sync.", "Select Areas", mcAreas_all))
				{
					wxLogError("Error creating area selection dialog.");
					return;
				}
				wxArrayInt selectionsIn;
				for (size_t index = 0; index < mcAreas_all.Count(); index++)
				{
					selectionsIn.Add(index);
				}
				cDialog.SetSelections(selectionsIn);
				mcAreas_selected.Empty();

				// show the dialog
				if (cDialog.ShowModal() != wxID_OK)
				{
					return;
				}

				bSyncingVersionControl = false;
				wxArrayInt selections = cDialog.GetSelections();
				wxString versionControlName = this->GetVerbs()->GetVersionControlAreaName();
				for (wxArrayInt::const_iterator it = selections.begin(); it != selections.end(); ++it)
				{
					wxString selectedArea = mcAreas_all[*it];
					if (selectedArea.CompareTo(versionControlName) == 0)
					{
						bSyncingVersionControl = true;
						break;
					}
				}
				
				ShowOrHideUpdateControl();
				this->FitHeight();
				// set the value to the branch they chose
				this->mwDags->Clear();
				if (selections.Count() == mcAreas_all.Count())
				{
					mcAreas_selected.Clear();
					return;
				}
				mcAreas_selected.Empty();
				for (size_t index = 0; index < selections.Count(); index++)
				{
					mcAreas_selected.Add(mcAreas_all[cDialog.GetSelections()[index]]);
				}
				PopulateDagsBox();
			}
		}
	}
	//this->SetMinSize(wxSize(this->GetSize().GetWidth(), this->GetSizer()->GetMinSize().GetHeight()));
	this->FitHeight();
}

void vvSyncDialog::FitHeight()
{
	wxSize minSize = this->GetMinSize();
	Layout();
	if (gui_GetSyncAction() == SYNC_PULL_THEN_PUSH)
	{
		minSize.SetWidth(-1);
		this->SetSize(minSize);
	}
	else
	{
		wxSize size = this->GetSize();
		size.SetHeight(-1);
		this->SetMinSize(size);
		this->Fit();
		this->SetMinSize(minSize);
	}
	this->Refresh();
	
	
}

wxArrayString vvSyncDialog::GetAreas()
{
	return mcAreas_selected;	
}

vvSyncDialog::RevisionMode vvSyncDialog::GetRevisionMode()
{
	return mRevisionMode;
}

wxArrayString vvSyncDialog::GetChangesets()
{
	if (GetRevisionMode() == REV_MODE_CHANGESETS)
		return mcChangesets_selected;
	else
	{
		wxArrayString emptyArray;
		return emptyArray;
	}
}

IMPLEMENT_DYNAMIC_CLASS(vvSyncDialog, vvDialog);
BEGIN_EVENT_TABLE(vvSyncDialog, vvDialog)
	EVT_COMBOBOX(wxID_ANY, OnComboBoxSelectionChanged)
	EVT_COMBOBOX_CLOSEUP(wxID_ANY, OnComboBoxSelectionChanged)
	EVT_BUTTON(XRCID("wDagBrowse"),         vvSyncDialog::OnBrowseButtonPressed)

	EVT_TEXT(XRCID("wxRepos"),				vvSyncDialog::OnControlValueChange)
	EVT_TEXT(XRCID("wDags"),				vvSyncDialog::OnTextInDagsBox)
	EVT_RADIOBUTTON(XRCID("wRevision_All_Select"),      vvSyncDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wRevision_Select_One"),    vvSyncDialog::OnControlValueChange)
END_EVENT_TABLE()
