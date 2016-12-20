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
#include "vvRevSpecControl.h"

#include "vvContext.h"
#include "vvRepoLocator.h"
#include "vvRevisionSelectionDialog.h"
#include "vvRevSpec.h"
#include "vvSgHelpers.h"
#include "vvVerbs.h"
#include "vvWxHelpers.h"
#include "vvBranchSelectDialog.h"
#include "vvRevisionChoiceDialog.h"
#include "sg.h"
/*
**
** Globals
**
*/

/*
 * Hints for various text box controls.
 */
static const wxString gsHint_Branch_Value   = "Name of the branch to select the head revision from";
static const wxString gsHint_Tag_Value      = "Name of the tag to select the target revision from";
static const wxString gsHint_Revision_Value = "ID of the revision to select";

/*
 * Strings used by the Select Branch dialog.
 */
static const wxString gsSelectBranch_Caption = "Select Branch";
static const wxString gsSelectBranch_Message = "Select the branch to use the head revision from.";

/*
 * Strings used by the Select Tag dialog.
 */
static const wxString gsSelectTag_Caption = "Select Tag";
static const wxString gsSelectTag_Message = "Select the tag to use the target revision from.";

/*
 * Strings used by the Select Revision dialog.
 */
static const wxString gsSelectRevision_Caption = "Select Revision";
static const wxString gsSelectRevision_Message = "Select the revision to use.";


/*
**
** Public Functions
**
*/

vvRevSpecControl::vvRevSpecControl()
	: mpVerbs(NULL)
	, mpContext(NULL)
{
}

vvRevSpecControl::vvRevSpecControl(
	wxWindow*        pParent,
	const wxString&  sRepo,
	vvVerbs*         pVerbs,
	vvContext*       pContext,
	const vvRevSpec& cRevSpec
	)
	: mpVerbs(NULL)
	, mpContext(NULL)
{
	this->Create(pParent, sRepo, pVerbs, pContext, cRevSpec);
}

bool vvRevSpecControl::Create(
	wxWindow*        pParent,
	const wxString&  sRepo,
	vvVerbs*         pVerbs,
	vvContext*       pContext,
	const vvRevSpec& cRevSpec
	)
{
	wxASSERT(cRevSpec.IsNull() || pContext != NULL);

	// create our base class by loading it from resources
	if (!wxXmlResource::Get()->LoadPanel(this, pParent, this->GetClassInfo()->GetClassName()))
	{
		wxLogError("Couldn't load control layout from resources.");
		return false;
	}

	// retrieve pointers to all our widgets
	this->mwBranch_Select   = XRCCTRL(*this, "wBranch_Select",   wxRadioButton);
	this->mwBranch_Value    = XRCCTRL(*this, "wBranch_Value",    wxTextCtrl);
	this->mwBranch_Browse   = XRCCTRL(*this, "wBranch_Browse",   wxButton);
	this->mwTag_Select      = XRCCTRL(*this, "wTag_Select",      wxRadioButton);
	this->mwTag_Value       = XRCCTRL(*this, "wTag_Value",       wxTextCtrl);
	this->mwTag_Browse      = XRCCTRL(*this, "wTag_Browse",      wxButton);
	this->mwRevision_Select = XRCCTRL(*this, "wRevision_Select", wxRadioButton);
	this->mwRevision_Value  = XRCCTRL(*this, "wRevision_Value",  wxTextCtrl);
	this->mwRevision_Browse = XRCCTRL(*this, "wRevision_Browse", wxButton);

	// make sure one of the radio buttons is selected
	if (
		   !this->mwBranch_Select->GetValue()
		&& !this->mwTag_Select->GetValue()
		&& !this->mwRevision_Select->GetValue()
		)
	{
		this->mwBranch_Select->SetValue(true);
	}

	// set text field hints
	// can't currently do this in the XRC file
	this->mwBranch_Value  ->SetHint(gsHint_Branch_Value);
	this->mwTag_Value     ->SetHint(gsHint_Tag_Value);
	this->mwRevision_Value->SetHint(gsHint_Revision_Value);

	// set our data
	// these will cause UpdateDisplay calls
	this->SetRepo(sRepo, pVerbs, pContext);
	this->SetRevSpec(cRevSpec, *pContext);

	// success
	return true;
}

void vvRevSpecControl::SetRepo(
	const wxString& sRepo,
	vvVerbs*        pVerbs,
	vvContext*      pContext
	)
{
	if (sRepo.IsEmpty() || pVerbs == NULL || pContext == NULL)
	{
		this->msRepo    = wxEmptyString;
		this->mpVerbs   = NULL;
		this->mpContext = NULL;
	}
	else
	{
		this->msRepo    = sRepo;
		this->mpVerbs   = pVerbs;
		this->mpContext = pContext;
	}

	this->UpdateDisplay();
}

bool vvRevSpecControl::SetRevSpec(
	const vvRevSpec& cRevSpec,
	vvContext&       cContext
	)
{
	// if the spec is NULL, there's nothing to set
	if (cRevSpec.IsNull())
	{
		return false;
	}

	// check the counts of each type in the spec
	unsigned int uBranches = 0u;
	if (!cRevSpec.GetBranchCount(cContext, uBranches))
	{
		return false;
	}
	unsigned int uTags = 0u;
	if (!cRevSpec.GetTagCount(cContext, uTags))
	{
		return false;
	}
	unsigned int uRevisions = 0u;
	if (!cRevSpec.GetRevisionCount(cContext, uRevisions))
	{
		return false;
	}

	// look for a type with exactly one
	// and use it to set the current value
	wxArrayString cValues;
	if (uBranches == 1u)
	{
		if (!cRevSpec.GetBranches(cContext, cValues))
		{
			return false;
		}
		this->SetValue(TYPE_BRANCH, cValues[0]);
	}
	else if (uTags == 1u)
	{
		if (!cRevSpec.GetTags(cContext, cValues))
		{
			return false;
		}
		this->SetValue(TYPE_TAG, cValues[0]);
	}
	else if (uRevisions == 1u)
	{
		if (!cRevSpec.GetRevisions(cContext, cValues))
		{
			return false;
		}
		this->SetValue(TYPE_REVISION, cValues[0]);
	}
	else
	{
		return false;
	}

	return true;
}

void vvRevSpecControl::SetValue(
	Type            eType,
	const wxString& sValue
	)
{
	wxASSERT(eType >= 0 && eType < TYPE_COUNT);

	if (eType == TYPE_BRANCH)
	{
		this->mwBranch_Select->SetValue(true);
		this->mwBranch_Value->SetValue(sValue);
	}
	else
	{
		this->mwBranch_Select->SetValue(false);
	}

	if (eType == TYPE_TAG)
	{
		this->mwTag_Select->SetValue(true);
		this->mwTag_Value->SetValue(sValue);
	}
	else
	{
		this->mwTag_Select->SetValue(false);
	}

	if (eType == TYPE_REVISION)
	{
		this->mwRevision_Select->SetValue(true);
		this->mwRevision_Value->SetValue(sValue);
	}
	else
	{
		this->mwRevision_Select->SetValue(false);
	}

	this->UpdateDisplay();
}

vvRevSpec vvRevSpecControl::GetRevSpec(
	vvContext& cContext
	) const
{
	vvRevSpec cSpec(cContext);
	wxString  sValue = this->GetValue();

	if (!sValue.IsEmpty())
	{
		switch (this->GetValueType())
		{
		case TYPE_BRANCH:
			cSpec.AddBranch(cContext, sValue);
			break;
		case TYPE_TAG:
			cSpec.AddTag(cContext, sValue);
			break;
		case TYPE_REVISION:
			cSpec.AddRevision(cContext, sValue);
			break;
		default:
			wxFAIL_MSG("Unknown TYPE value.");
			break;
		}
	}

	return cSpec;
}

vvRevSpecControl::Type vvRevSpecControl::GetValueType() const
{
	if (this->mwBranch_Select->GetValue())
	{
		return TYPE_BRANCH;
	}
	else if (this->mwTag_Select->GetValue())
	{
		return TYPE_TAG;
	}
	else if (this->mwRevision_Select->GetValue())
	{
		return TYPE_REVISION;
	}
	else
	{
		wxFAIL_MSG("Unknown TYPE value.");
		return TYPE_COUNT;
	}
}

wxString vvRevSpecControl::GetValue() const
{
	switch (this->GetValueType())
	{
	case TYPE_BRANCH:
		return this->mwBranch_Value->GetValue();
	case TYPE_TAG:
		return this->mwTag_Value->GetValue();
	case TYPE_REVISION:
		return this->mwRevision_Value->GetValue();
	default:
		wxFAIL_MSG("Unknown TYPE value.");
		return wxEmptyString;
	}
}

void vvRevSpecControl::UpdateDisplay()
{
	Type eType = this->GetValueType();

	// enable the correct browse/edit controls for the currently selected type
	this->mwBranch_Value   ->Enable(eType == TYPE_BRANCH);
	this->mwBranch_Browse  ->Enable(eType == TYPE_BRANCH);
	this->mwTag_Value      ->Enable(eType == TYPE_TAG);
	this->mwTag_Browse     ->Enable(eType == TYPE_TAG);
	this->mwRevision_Value ->Enable(eType == TYPE_REVISION);
	this->mwRevision_Browse->Enable(eType == TYPE_REVISION);

	// show/hide the browse buttons depending on whether or not we have a repo to browse
	vvXRCSIZERITEM(*this, "cBranch_Browse")  ->Show(!this->msRepo.IsEmpty());
	vvXRCSIZERITEM(*this, "cTag_Browse")     ->Show(!this->msRepo.IsEmpty());
	vvXRCSIZERITEM(*this, "cRevision_Browse")->Show(!this->msRepo.IsEmpty());

	// relayout to account for visibility changes
	this->Layout();
}

void vvRevSpecControl::OnControlValueChange(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	this->UpdateDisplay();
}

void vvRevSpecControl::OnBrowseBranches(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	// the button should have been hidden if we didn't have this info
	wxASSERT(!this->msRepo.IsEmpty());
	wxASSERT(this->mpVerbs != NULL);
	wxASSERT(this->mpContext != NULL);

	vvBranchSelectDialog vvbsd;
	vvbsd.Create(vvFindTopLevelParent(this), vvRepoLocator::FromRepoName(this->msRepo), this->mwBranch_Value->GetValue(), false, *mpVerbs, *mpContext);
	if (vvbsd.ShowModal() == wxID_OK)
	{
		// set the value to the branch they chose
		this->mwBranch_Value->SetValue(vvbsd.GetSelectedBranch());
	}
}

void vvRevSpecControl::OnBrowseTags(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	// the button should have been hidden if we didn't have this info
	wxASSERT(!this->msRepo.IsEmpty());
	wxASSERT(this->mpVerbs != NULL);
	wxASSERT(this->mpContext != NULL);

	// get the list of tags in the repo
	wxArrayString cTags;
	{
		wxBusyCursor cBusyCursor;

		if (!this->mpVerbs->GetTags(*this->mpContext, this->msRepo, cTags))
		{
			wxLogError("Error retrieving tag list from repository.");
			return;
		}
		cTags.Sort(vvSortStrings_Insensitive);
	}

	// get the index of the tag currently entered in the control
	int iIndex = cTags.Index(this->mwTag_Value->GetValue(), false);

	// create a dialog to allow the user to choose one of them
	wxSingleChoiceDialog cDialog;
	{
		wxBusyCursor cBusyCursor;

		if (!cDialog.Create(vvFindTopLevelParent(this), gsSelectTag_Message, gsSelectTag_Caption, cTags))
		{
			wxLogError("Error creating tag selection dialog.");
			return;
		}
		if (iIndex != wxNOT_FOUND)
		{
			cDialog.SetSelection(iIndex);
		}
	}

	// show the dialog
	if (cDialog.ShowModal() != wxID_OK)
	{
		return;
	}

	// set the value to the branch they chose
	this->mwTag_Value->SetValue(cDialog.GetStringSelection());
}

vvRevSpecControl::BranchAmbiguationState vvRevSpecControl::DisambiguateBranchIfNecessary(bool bMerging, wxArrayString arBaselineHIDs, vvRevSpec& cReturnSpec)
{
	//Now check to see if the branch actually has multiple heads.
	wxArrayString arBranches;

	vvRevSpec cSpec = GetRevSpec(*mpContext);
	cSpec.GetBranches(*mpContext, arBranches);
	if (arBranches.Count() != 0)
	{
		wxArrayString arBranchHeads;
		wxString sSelectedBranch = arBranches[0];
		mpVerbs->GetBranchHeads(*mpContext, msRepo, sSelectedBranch, arBranchHeads);
		if (bMerging && arBranchHeads.Index(arBaselineHIDs[0]) != wxNOT_FOUND)
			arBranchHeads.Remove(arBaselineHIDs[0]);
		if (arBranchHeads.Count() > 1)
		{
			//We need to prompt them to pick a single head.
			vvVerbs::HistoryData data;
			for (unsigned int i = 0; i < arBranchHeads.Count(); i++)
			{
				vvVerbs::HistoryEntry e;
				vvVerbs::RevisionList children;
				if (!mpVerbs->Get_Revision_Details(*mpContext, msRepo, arBranchHeads[i], e, children))
				{
					if (mpContext->Error_Equals(SG_ERR_NOT_FOUND))
					{
						mpContext->Error_Reset();
						vvVerbs::CommentEntry ce;
						e.nRevno = 0;
						e.sChangesetHID = arBranchHeads[i];
						e.bIsMissing = true;
						ce.sCommentText = "This changeset is missing from the local repository";
						e.cComments.push_back(ce);
					}
					else
					{
						mpContext->Log_ReportError_CurrentError();
						mpContext->Error_Reset();
					}
				}
				data.push_back(e);
			}

			int nCountAvailableHeads = 0;

			for (vvVerbs::HistoryData::const_iterator it = data.begin(); it != data.end(); it++)
			{
				if (!it->bIsMissing)
					nCountAvailableHeads++;
			}

			if (nCountAvailableHeads == 0)
			{
				mpContext->Log_ReportError("None of the branch heads for \"" + sSelectedBranch + "\" are in the local instance of the repo.  You will need to select a specific revision.");
				return BRANCH__STILL_AMBIGUOUS;
			}
			else if (nCountAvailableHeads == 1)
			{
				//The regular update will disambiguate the branch for us.
				return BRANCH__NO_LONGER_AMBIGUOUS;
			}
			else if (nCountAvailableHeads >= 2)
			{
				vvRevisionChoiceDialog * vrcd = new vvRevisionChoiceDialog();
				vrcd->Create(this, vvRepoLocator::FromRepoName(msRepo), data, "Select a branch head", "The \"" + sSelectedBranch + "\" branch has multiple heads.  Please select one head.", *mpVerbs, *mpContext);
				if (vrcd->ShowModal() == wxID_OK)
				{
					vrcd->Destroy();
					cReturnSpec.AddRevision(*mpContext, vrcd->GetSelection());
					cReturnSpec.SetUpdateBranch(sSelectedBranch);
					return BRANCH__NO_LONGER_AMBIGUOUS;
				}
				else
				{
					vrcd->Destroy();
					return BRANCH__STILL_AMBIGUOUS;
				}
			}
		}
		return BRANCH__WAS_NOT_AMBIGUOUS;
	}
	return BRANCH__WAS_NOT_AMBIGUOUS;
}
void vvRevSpecControl::OnBrowseRevisions(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	// the button should have been hidden if we didn't have this info
	wxASSERT(!this->msRepo.IsEmpty());
	wxASSERT(this->mpVerbs != NULL);
	wxASSERT(this->mpContext != NULL);

	// create a dialog to allow the user to choose a revision
	vvRevisionSelectionDialog cDialog;
	if (!cDialog.Create(vvFindTopLevelParent(this), this->msRepo, *this->mpVerbs, *this->mpContext, gsSelectRevision_Caption, gsSelectRevision_Message))
	{
		wxLogError("Error creating revision selection dialog.");
		return;
	}

	// pre-select the currently entered revision
	// TODO: once the dialog has that capability
	//cDialog.SetRevision(this->mwRevision_Value->GetValue());

	// show the dialog
	if (cDialog.ShowModal() != wxID_OK)
	{
		return;
	}

	// set the value to the revision they chose
	this->mwRevision_Value->SetValue(cDialog.GetRevision());
}

IMPLEMENT_DYNAMIC_CLASS(vvRevSpecControl, wxPanel);

BEGIN_EVENT_TABLE(vvRevSpecControl, wxPanel)
	// when a radio button is selected, update the display to match it
	EVT_RADIOBUTTON(XRCID("wBranch_Select"),   OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wTag_Select"),      OnControlValueChange)
	EVT_RADIOBUTTON(XRCID("wRevision_Select"), OnControlValueChange)

	// when a browse button is clicked, invoke the appropriate dialog
	EVT_BUTTON(XRCID("wBranch_Browse"),   OnBrowseBranches)
	EVT_BUTTON(XRCID("wTag_Browse"),      OnBrowseTags)
	EVT_BUTTON(XRCID("wRevision_Browse"), OnBrowseRevisions)
END_EVENT_TABLE()
