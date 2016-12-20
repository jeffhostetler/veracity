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
#include "vvCreateDialog.h"

#include "vvDialogHeaderControl.h"
#include "vvVerbs.h"
#include "vvContext.h"
#include "vvRevSpecControl.h"
#include "vvSyncTargetsControl.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvWxHelpers.h"

/*
**
** Types
**
*/

/**
 * IDs of controls in the dialog.
 */
enum Control
{
	CONTROL_REPO, //< The repo drop-down.
};


/*
**
** Globals
**
*/

/**
 * String displayed in the Repo list to indicate the creation of a new repo.
 * Must match the string used in the XRC file.
 */
static const wxString gsNewRepoString = "<New Repository>";

/*
 * Hints for various text box controls.
 */
static const wxString gsHint_New_Name         = "Local name for the new repository";
static const wxString gsHint_New_Remote_Value = "URL of the remote repository to clone";

/**
 * String displayed at the top of the validation error message box.
 */
static const wxString gsValidationErrorMessage = "There was an error in the information specified.  Please correct the following problems and try again.";

/*
 * Strings displayed as field labels when validation errors occur.
 */
static const wxString gsValidationField_Repo             = "Repository";
static const wxString gsValidationField_New_Name         = "New Repository Name";
static const wxString gsValidationField_New_Remote_Value = "Remote Repository";
static const wxString gsValidationField_New_Local_Value  = "Local Repository";
static const wxString gsValidationField_Revision         = "Checkout Revision";
static const wxString gsValidationField_ShareUsersChoice = "Shared Users Repository";

/*
**
** Public Functions
**
*/

bool vvCreateDialog::Create(
	wxWindow*       pParent,
	const wxString& sFolder,
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const wxString& sRepoName,
	CloneAction     eCloneAction,
	const wxString& sCloneSource,
	CheckoutType    eCheckoutType,
	const wxString& sCheckoutTarget
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// store the given initialization data
	this->msRepoName       = sRepoName;
	this->meCloneAction    = eCloneAction;
	this->msCloneSource    = sCloneSource;
	this->meCheckoutType   = eCheckoutType;
	this->msCheckoutTarget = sCheckoutTarget;

	// retrieve pointers to all our widgets
	this->mwNew_Name          = XRCCTRL(*this, "wNew_Name",          wxTextCtrl);
	this->mwNew_Remote_Select = XRCCTRL(*this, "wNew_Remote_Select", wxRadioButton);
	this->mwNew_Local_Select  = XRCCTRL(*this, "wNew_Local_Select",  wxRadioButton);
	this->mwNew_Local_Value   = XRCCTRL(*this, "wNew_Local_Value",   wxChoice);
	this->mwNew_Empty_Select  = XRCCTRL(*this, "wNew_Empty_Select",  wxRadioButton);
	this->mwNew_Empty_Value   = XRCCTRL(*this, "wNew_Empty_Value",   wxStaticText);

	this->mwShareUsersChoice   = XRCCTRL(*this, "wShareUsersChoice",   wxChoice);
	this->mwShareUsers		   = XRCCTRL(*this, "wShareUsers",   wxCheckBox);

	// create our header control
	vvDialogHeaderControl* wHeader = new vvDialogHeaderControl(this);
	this->mwRepo = new wxChoice(wHeader, CONTROL_REPO);
	wHeader->AddFolderValue(sFolder, cVerbs, cContext);
	wHeader->AddControlValue(vvDialogHeaderControl::sszLabel_Repo, this->mwRepo, false);
	wxXmlResource::Get()->AttachUnknownControl("wHeader", wHeader);

	// create the repo dropdown
	this->mwNew_Remote_Value = new vvSyncTargetsControl(this, wxID_ANY);
	wxXmlResource::Get()->AttachUnknownControl("wNew_Remote_Value", mwNew_Remote_Value);

	// create the revision control
	this->mwRevision = new vvRevSpecControl(this, sRepoName, &cVerbs, &cContext);
	wxXmlResource::Get()->AttachUnknownControl("wRevision", this->mwRevision);

	// set the height of the blank static text boxes that are replacing actual text controls
	// we want the height to be the same as a text control
	// that way all of the associated radio buttons will be evenly spaced
	this->mwNew_Empty_Value->SetMinSize(this->mwNew_Remote_Value->GetSize());

	// setup validators
	this->mcValidatorReporter.SetHelpMessage(gsValidationErrorMessage);
	this->mwRepo->SetValidator(vvValidator(gsValidationField_Repo)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);
	this->mwNew_Name->SetValidator(vvValidator(gsValidationField_New_Name)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
		.AddConstraint(vvValidatorRepoExistsConstraint(*this->GetVerbs(), *this->GetContext(), false))
		);
	this->mwNew_Remote_Value->SetValidator(vvValidator(gsValidationField_New_Remote_Value)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);
	this->mwNew_Local_Value->SetValidator(vvValidator(gsValidationField_New_Local_Value)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);
	this->mwRevision->SetValidator(vvValidator(gsValidationField_Revision)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false, this->GetContext()))
	);

	this->mwShareUsersChoice->SetValidator(vvValidator(gsValidationField_ShareUsersChoice)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false))
	);

	// set text field hints
	// can't currently do this in the XRC file
	this->mwNew_Name        ->SetHint(gsHint_New_Name);
	this->mwNew_Remote_Value->SetHint(gsHint_New_Remote_Value);

	// focus on the repo select list
	this->mwRepo->SetFocus();

	// success
	return true;
}

bool vvCreateDialog::TransferDataToWindow()
{
	// populate the lists of local repos
	this->mwRepo->Clear();
	this->mwNew_Local_Value->Clear();
	this->mwShareUsersChoice->Clear();
	if (this->GetVerbs() != NULL)
	{
		wxArrayString cLocalRepos;
		this->GetVerbs()->GetLocalRepos(*this->GetContext(), cLocalRepos);
		cLocalRepos.Sort(vvSortStrings_Insensitive);
		for (wxArrayString::const_iterator itRepo = cLocalRepos.begin(); itRepo != cLocalRepos.end(); ++itRepo)
		{
			// add the repo to each repo list
			this->mwRepo->Append(*itRepo);
			this->mwNew_Local_Value->Append(*itRepo);
			this->mwShareUsersChoice->Append(*itRepo);
		}
	}
	this->mwRepo->Append(gsNewRepoString);

	// populate the list of recently used remote URLs
	if (this->GetVerbs() != NULL)
	{
		this->mwNew_Remote_Value->Populate(*this->GetContext(), *this->GetVerbs(), wxEmptyString);
	}

	// check which repo to pre-select
	if (this->meCloneAction != CLONE_NOTHING || this->mwRepo->GetCount() == 1u)
	{
		this->mwRepo->SetSelection(0);
		this->mwNew_Name->SetValue(this->msRepoName);
	}
	else if (this->msRepoName.IsEmpty())
	{
		this->mwRepo->SetSelection(wxNOT_FOUND);
	}
	else
	{
		if (!this->mwRepo->SetStringSelection(this->msRepoName))
		{
			wxLogWarning("Couldn't pre-select repository '%s' because it's not in the local repository list.", this->msRepoName);
		}
	}

	// pre-fill in the appropriate clone data
	switch (this->meCloneAction)
	{
	case CLONE_EMPTY:
		this->mwNew_Empty_Select->SetValue(true);
		break;
	case CLONE_LOCAL:
		this->mwNew_Local_Select->SetValue(true);
		if (this->msCloneSource.IsEmpty())
		{
			this->mwNew_Local_Value->SetSelection(wxNOT_FOUND);
		}
		else
		{
			if (!this->mwNew_Local_Value->SetStringSelection(this->msCloneSource))
			{
				wxLogWarning("Couldn't pre-select local clone source '%s' because it's not in the local repository list.", this->msCloneSource);
			}
		}
		break;
	case CLONE_REMOTE:
		this->mwNew_Remote_Select->SetValue(true);
		this->mwNew_Remote_Value->SetValue(this->msCloneSource);
		break;
	}

	// pre-fill in the appropriate checkout revision
	if (this->GetContext() != NULL)
	{
		vvRevSpecControl::Type eType = vvRevSpecControl::TYPE_COUNT;
		switch (this->meCheckoutType)
		{
		case CHECKOUT_BRANCH:
			eType = vvRevSpecControl::TYPE_BRANCH;
			break;
		case CHECKOUT_CHANGESET:
			eType = vvRevSpecControl::TYPE_REVISION;
			break;
		case CHECKOUT_TAG:
			eType = vvRevSpecControl::TYPE_TAG;
			break;
		}
		this->mwRevision->SetValue(eType, this->msCheckoutTarget);
	}

	// update our display to reflect any widget changes
	this->UpdateDisplay();

	// success
	return true;
}

bool vvCreateDialog::TransferDataFromWindow()
{
	// check if the user wants to create a new repo
	if (this->mwRepo->GetStringSelection() != gsNewRepoString)
	{
		this->msRepoName    = this->mwRepo->GetStringSelection();
		this->meCloneAction = CLONE_NOTHING;
		this->msCloneSource = wxEmptyString;
	}
	else
	{
		this->msRepoName = this->mwNew_Name->GetValue();
		if (this->mwNew_Empty_Select->GetValue())
		{
			this->meCloneAction = CLONE_EMPTY;
			this->msCloneSource = wxEmptyString;
			if (mwShareUsers->GetValue())
			{
				this->msShareUserRepo = this->mwShareUsersChoice->GetStringSelection();
			}
		}
		else if (this->mwNew_Local_Select->GetValue())
		{
			this->meCloneAction = CLONE_LOCAL;
			this->msCloneSource = this->mwNew_Local_Value->GetStringSelection();
		}
		else if (this->mwNew_Remote_Select->GetValue())
		{
			this->meCloneAction = CLONE_REMOTE;
			this->msCloneSource = this->mwNew_Remote_Value->GetValue();
		}
		else
		{
			wxFAIL_MSG("vvCreateDialog somehow ended up with no clone action selected.");
			return false;
		}

	}
	
	this->msCheckoutTarget = this->mwRevision->GetValue();
	// check which revision the user wants to checkout
	switch (this->mwRevision->GetValueType())
	{
	case vvRevSpecControl::TYPE_BRANCH:
		{
			//Assume that this will be a regular branch checkout
			this->meCheckoutType = CHECKOUT_BRANCH;

			//We can't help to disambiguate remote clones.
			//And empty repos have no ambiguous branches.
			if (this->meCloneAction != CLONE_REMOTE &&
				this->meCloneAction != CLONE_EMPTY)
			{
				vvContext * pContext = this->GetContext();
				vvRevSpec cSpec_PossiblyAmbiguous(*pContext);
				vvRevSpec cSpec_Disambiguated(*pContext);
				cSpec_PossiblyAmbiguous.AddBranch(*pContext, this->mwRevision->GetValue());
				vvRevSpecControl::BranchAmbiguationState branchAmbiguationState = this->mwRevision->DisambiguateBranchIfNecessary(false, wxArrayString(), cSpec_Disambiguated);
				if (branchAmbiguationState == vvRevSpecControl::BRANCH__NO_LONGER_AMBIGUOUS)
				{
					this->meCheckoutType = CHECKOUT_CHANGESET;
					this->msBranchAttachment = this->msCheckoutTarget;
					wxArrayString arRevisions;
					cSpec_Disambiguated.GetRevisions(*pContext, arRevisions);
					this->msCheckoutTarget = arRevisions[0];
				}
				else if (branchAmbiguationState == vvRevSpecControl::BRANCH__STILL_AMBIGUOUS)
				{
					return false;
				}
			}
			
			break;
		}
	case vvRevSpecControl::TYPE_REVISION:
		this->meCheckoutType = CHECKOUT_CHANGESET;
		break;
	case vvRevSpecControl::TYPE_TAG:
		this->meCheckoutType = CHECKOUT_TAG;
		break;
	default:
		wxFAIL_MSG("vvCreateDialog somehow ended up with no checkout type selected.");
		return false;
	}

	// success
	return true;
}

bool vvCreateDialog::DisambiguateBranch()
{
	vvContext * pContext = this->GetContext();
	vvRevSpec cSpec_PossiblyAmbiguous(*pContext);
	vvRevSpec cSpec_Disambiguated(*pContext);
	cSpec_PossiblyAmbiguous.AddBranch(*pContext, this->mwRevision->GetValue());
	vvRevSpecControl::BranchAmbiguationState branchAmbiguationState = this->mwRevision->DisambiguateBranchIfNecessary(false, wxArrayString(), cSpec_Disambiguated);
	if (branchAmbiguationState == vvRevSpecControl::BRANCH__NO_LONGER_AMBIGUOUS)
	{
		this->meCloneAction = CLONE_NOTHING;
		this->meCheckoutType = CHECKOUT_CHANGESET;
		this->msBranchAttachment = this->msCheckoutTarget;
		wxArrayString arRevisions;
		cSpec_Disambiguated.GetRevisions(*pContext, arRevisions);
		this->msCheckoutTarget = arRevisions[0];
	}
	else if (branchAmbiguationState == vvRevSpecControl::BRANCH__STILL_AMBIGUOUS)
	{
		return false;
	}
	return true;
}

const wxString& vvCreateDialog::GetRepo() const
{
	return this->msRepoName;
}

void vvCreateDialog::SetRepo(
	const wxString& sRepo)
{
	return this->mwRevision->SetRepo(sRepo, this->GetVerbs(), this->GetContext());
}

vvCreateDialog::CloneAction vvCreateDialog::GetCloneAction() const
{
	return this->meCloneAction;
}

const wxString& vvCreateDialog::GetCloneSource() const
{
	return this->msCloneSource;
}

const wxString& vvCreateDialog::GetSharedUsersRepo() const
{
	return this->msShareUserRepo;
}

vvCreateDialog::CheckoutType vvCreateDialog::GetCheckoutType() const
{
	return this->meCheckoutType;
}

const wxString& vvCreateDialog::GetCheckoutTarget() const
{
	return this->msCheckoutTarget;
}

const wxString& vvCreateDialog::GetBranchAttachment() const
{
	return this->msBranchAttachment;
}
void vvCreateDialog::UpdateDisplay()
{
	// check a few things up front
	bool bRepoSelected    = this->mwRepo->GetSelection() != wxNOT_FOUND;
	bool bNewSelected     = this->mwRepo->GetStringSelection() == gsNewRepoString;
	bool bEmptySelected   = this->mwNew_Empty_Select->GetValue();
	bool bUseExistingRepo = bRepoSelected && !(bNewSelected && bEmptySelected);

	// if they want to use an existing repo, check if the one they've entered/chosen validates
	// if so they should be able to browse it for things
	wxString sBrowseRepo = wxEmptyString;
	if (bUseExistingRepo)
	{
		if (bRepoSelected && !bNewSelected)
		{
			// they're making a working copy from a repo we've already got
			sBrowseRepo = this->mwRepo->GetStringSelection();
		}
		else if (
			   this->mwNew_Local_Select->GetValue()
			&& this->mwNew_Local_Value->GetValidator() != NULL
			&& this->mwNew_Local_Value->GetValidator()->Validate(this)
			)
		{
			// they're making a working copy from a cloned local repo
			sBrowseRepo = this->mwNew_Local_Value->GetStringSelection();
		}
	}

	// show the New Repo block if they've selected to create a new repo
	vvXRCSIZERITEM(*this, "cNew")->Show(bNewSelected);

	// if the local list of repos to clone is empty, disable and hide it
	if (this->mwNew_Local_Value->GetCount() == 0u)
	{
		this->mwNew_Local_Select->SetValue(false);
		this->mwNew_Local_Select->Enable(false);
		vvXRCSIZERITEM(*this, "cNew_Local_Select")->Show(false);
		vvXRCSIZERITEM(*this, "cNew_Local_Value") ->Show(false);
	}

	// enable the text controls according to their corresponding to radio buttons
	this->mwNew_Remote_Value->Enable(this->mwNew_Remote_Select->GetValue());
	this->mwNew_Local_Value ->Enable(this->mwNew_Local_Select ->GetValue());

	bool bShowSharedContainer = bEmptySelected 
											&& this->mwShareUsersChoice->GetCount() > 0
											&& mwNew_Empty_Select->IsEnabled() 
											&& mwNew_Empty_Select->IsShown();
	vvXRCSIZERITEM(*this, "cSharedUsers")->Show(bShowSharedContainer);
	if (bShowSharedContainer)
	{
		this->mwShareUsersChoice ->Enable(this->mwShareUsers ->GetValue());
	}

	// show the Checkout Revision block if an existing repo is selected
	vvXRCSIZERITEM(*this, "cRevision")->Show(bUseExistingRepo);
	this->mwRevision->Enable(bUseExistingRepo);

	// set the browsable repo to the rev spec control
	// if it's blank (repo not browsable), this will disable browsing
	this->mwRevision->SetRepo(sBrowseRepo, this->GetVerbs(), this->GetContext());

	// our minimum height might have changed if we changed which blocks are visible
	this->SetMinSize(wxSize(this->GetSize().GetWidth(), this->GetSizer()->GetMinSize().GetHeight()));

	// relayout our controls and fit the window to their new size
	this->Layout();
	this->Fit();
}

void vvCreateDialog::OnControlValueChange(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	this->UpdateDisplay();
}

IMPLEMENT_DYNAMIC_CLASS(vvCreateDialog, vvDialog);

BEGIN_EVENT_TABLE(vvCreateDialog, vvDialog)
	// when a value changes, we need to update the display to match
	EVT_CHOICE(     CONTROL_REPO,                        vvCreateDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wNew_Remote_Select"),         vvCreateDialog::OnControlValueChange)
	EVT_TEXT(       XRCID("wNew_Remote_Value"),          vvCreateDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wNew_Local_Select"),          vvCreateDialog::OnControlValueChange)
	EVT_CHOICE(     XRCID("wNew_Local_Value"),           vvCreateDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wNew_Empty_Select"),          vvCreateDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wRevision_Head_Select"),      vvCreateDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wRevision_Branch_Select"),    vvCreateDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wRevision_Tag_Select"),       vvCreateDialog::OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wRevision_Changeset_Select"), vvCreateDialog::OnControlValueChange)
	EVT_CHECKBOX(	XRCID("wShareUsers"),				 vvCreateDialog::OnControlValueChange)
END_EVENT_TABLE()
