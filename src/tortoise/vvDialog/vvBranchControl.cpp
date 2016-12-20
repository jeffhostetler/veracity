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
#include "vvBranchControl.h"

#include "vvContext.h"
#include "vvRepoLocator.h"
#include "vvRevisionChoiceDialog.h"
#include "vvRevSpec.h"
#include "vvVerbs.h"
#include "vvWxHelpers.h"
#include "vvBranchSelectDialog.h"

/*
**
** Globals
**
*/


/*
**
** Public Functions
**
*/

vvBranchControl::vvBranchControl()
	: mpVerbs(NULL)
	, mpContext(NULL)
{
}

vvBranchControl::vvBranchControl(
	wxWindow*        pParent,
	const wxString&  sRepo,
	wxArrayString*	 pWFParents,
	vvVerbs*         pVerbs,
	vvContext*       pContext
	)
	: mpVerbs(NULL)
	, mpContext(NULL)
{
	this->Create(pParent, sRepo, pWFParents, pVerbs, pContext);
}

bool vvBranchControl::Create(
	wxWindow*        pParent,
	const wxString&  sRepo,
	wxArrayString*	 pWFParents,
	vvVerbs*         pVerbs,
	vvContext*       pContext
	)
{
	mpContext = pContext;
	mpVerbs = pVerbs;
	this->mpWFParents = pWFParents;
	this->msRepo = sRepo;
	// create our base class by loading it from resources
	if (!wxXmlResource::Get()->LoadPanel(this, pParent, this->GetClassInfo()->GetClassName()))
	{
		wxLogError("Couldn't load control layout from resources.");
		return false;
	}

	this->mwAttachCheckBox = XRCCTRL(*this, "wAttachCheckBox", wxCheckBox);
	this->mwBranchText = XRCCTRL(*this, "wBranchText", wxTextCtrl);
	this->mwHeadCountWarning = XRCCTRL(*this, "wHeadCountWarning", wxTextCtrl);
	this->mwBrowseButton = XRCCTRL(*this, "wBranch_Browse", wxButton);
	mwHeadCountWarning->SetForegroundColour(wxColor("red"));

	// retrieve pointers to all our widgets
	
	// success
	return true;
}
void vvBranchControl::UpdateDisplay()
{
	bool currentlyAttached = mwAttachCheckBox->GetValue();
	mwBranchText->Enable(currentlyAttached);
	mwBrowseButton->Enable(currentlyAttached);
	wxString currentValue = mwBranchText->GetValue();
	if (!currentlyAttached)
		this->mwHeadCountWarning->SetValue("");
	if (!currentlyAttached &&  currentValue != wxEmptyString)
		mwBranchText->SetValue(wxEmptyString);
}

void vvBranchControl::TransferDataToWindow(wxString& sOrigBranch)
{
	this->msOrigBranch = sOrigBranch;
	mwBranchText->Clear();

	if (msOrigBranch != wxEmptyString)
		mwBranchText->SetValue(msOrigBranch);	
	mwAttachCheckBox->SetValue(msOrigBranch != wxEmptyString);
	//We have to do this here, instead of Create()
	XRCCTRL(*this, "wExplanationText", wxTextCtrl)->SetBackgroundColour(this->GetBackgroundColour());
	mwHeadCountWarning->SetBackgroundColour(this->GetBackgroundColour());
	wxCommandEvent blah;
	OnBranchChange(blah);
	this->UpdateDisplay();
	OnControlValueChange(blah);
}

bool vvBranchControl::IsDirty()
{
	return GetActionChoice() != vvVerbs::BRANCH_ACTION__NO_CHANGE;
}

vvVerbs::BranchAttachmentAction vvBranchControl::GetActionChoice()
{

	bool bBranchDirty = false;
	bool currentlyAttached = mwAttachCheckBox->GetValue();
	wxString selectedBranch = mwBranchText->GetValue();
	//can't attach to an empty branch
	if (currentlyAttached && selectedBranch.IsEmpty())
		return vvVerbs::BRANCH_ACTION__ERROR;
	//used to be detached and still are
	if (msOrigBranch.IsEmpty() && !currentlyAttached)
		return vvVerbs::BRANCH_ACTION__NO_CHANGE;

	if (msOrigBranch.IsEmpty() && currentlyAttached)
		bBranchDirty = true; //They used to be detached, but now they are attached
	else if (!msOrigBranch.IsEmpty() && !currentlyAttached)
		bBranchDirty = true; //They used to be attached, but now they are detached
	else if (!msOrigBranch.IsEmpty() && msOrigBranch != selectedBranch)
		bBranchDirty = true; //They chose a different branch
	bool bExists = false;
	bool bOpen = false;
	mpVerbs->BranchExists(*mpContext, msRepo, selectedBranch, bExists, bOpen);
	if (bExists && bOpen && !bBranchDirty)
		return vvVerbs::BRANCH_ACTION__NO_CHANGE;
	//They have a change to something.
	if (!currentlyAttached)
		//Detach from branches
		return vvVerbs::BRANCH_ACTION__DETACH;
	
	
	if (bExists == false)
		//They want to create a new branch
		return vvVerbs::BRANCH_ACTION__CREATE;
	else if (bOpen == false)
		//They want to reopen a closed branch.
		return vvVerbs::BRANCH_ACTION__REOPEN;
	else
		//They want to attach to one of the existing branches.
		return vvVerbs::BRANCH_ACTION__ATTACH;
}

bool vvBranchControl::TransferDataFromWindow()
{
	this->msBranch = this->mwBranchText->GetValue();
	this->meAction = GetActionChoice();
	if (meAction == vvVerbs::BRANCH_ACTION__CREATE)
	{
		if (wxMessageBox("Are you sure that you want to create a new brach named \"" + msBranch + "\"?", "Create Branch Confirmation", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
			return false;
	}
	if (meAction == vvVerbs::BRANCH_ACTION__REOPEN)
	{
		if (wxMessageBox("Are you sure that you want to reopen a the \"" + msBranch + "\" branch?", "Reopen Branch Confirmation", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
			return false;
	}
	if (meAction == vvVerbs::BRANCH_ACTION__ERROR)
	{
		wxMessageBox("Can't attach to a branch with an empty name.", "Branch Attachment Error");
		return false;
	}

	if (msOrigBranch.IsEmpty())
	{
		meRestoreAction = vvVerbs::BRANCH_ACTION__DETACH;
	}
	else if (meAction == vvVerbs::BRANCH_ACTION__ATTACH || meAction == vvVerbs::BRANCH_ACTION__DETACH || meAction == vvVerbs::BRANCH_ACTION__CREATE)
	{
		//Restore the attachment to the original branch.
		meRestoreAction = vvVerbs::BRANCH_ACTION__ATTACH;
	}
	else if (meAction == vvVerbs::BRANCH_ACTION__REOPEN)
	{
		//Close the branch that we reopened.
		meRestoreAction = vvVerbs::BRANCH_ACTION__CLOSE;
	}
	return true;
}

wxString vvBranchControl::GetOriginalBranchName() const
{
	return msOrigBranch;
}
vvVerbs::BranchAttachmentAction vvBranchControl::GetBranchRestoreAction() const
{
	return meRestoreAction;
}

wxString vvBranchControl::GetBranchName() const
{
	return msBranch;
}
vvVerbs::BranchAttachmentAction vvBranchControl::GetBranchAction() const
{
	return meAction;
}

void vvBranchControl::OnControlValueChange(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	this->UpdateDisplay();
	if (this->mwAttachCheckBox->GetValue())
	{
		this->mwHeadCountWarning->SetLabel("");
	}
	else
	{
		this->mwHeadCountWarning->SetLabel("WARNING: This commit will be anonymous and not attached to any branch.");
	}
}

void vvBranchControl::OnBrowseBranch(
	wxCommandEvent& WXUNUSED(cEvent))
{
	vvBranchSelectDialog vvbsd;
	vvbsd.Create(this, vvRepoLocator::FromRepoName(this->msRepo), this->mwBranchText->GetValue(), false, *mpVerbs, *mpContext);
	if (vvbsd.ShowModal() == wxID_OK)
	{
		// set the value to the branch they chose
		this->mwBranchText->SetValue(vvbsd.GetSelectedBranch());
	}
}

void vvBranchControl::OnBranchChange(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	wxString sBranch = this->mwBranchText->GetValue();
	if (mpWFParents != NULL && this->mwAttachCheckBox->GetValue() && !sBranch.IsEmpty())
	{
		vvVerbs::BranchAttachmentAction actionChoice = this->GetActionChoice();
		if (actionChoice == vvVerbs::BRANCH_ACTION__NO_CHANGE ||
			actionChoice == vvVerbs::BRANCH_ACTION__ATTACH)
		{
			//They are attempting to attach to a branch.  Warn them if they will be increasing the head count.
			wxArrayString sBranchHeads;
			if (this->mpVerbs->GetBranchHeads(*mpContext, msRepo, sBranch, sBranchHeads))
			{
				//loop over the parent HIDs.  If any of them are in the heads array, then
				//the next commit will move the head.  If none of them match, then the 
				//next commit will increase the head count.
				bool bFound = false;
				if (sBranchHeads.Count() == 0)
					bFound = true;
				else
				{
					for (wxArrayString::const_iterator it = mpWFParents->begin(); it != mpWFParents->end(); ++it)
					{
						if (bFound)
							break;
						for (wxArrayString::const_iterator it2 = sBranchHeads.begin(); it2 != sBranchHeads.end(); ++it2)
						{
							//Here, we're checking to see if we're connected to any of the branch heads.
							bool bAreConnected = false;
							if (!this->mpVerbs->CheckForDagConnection(*mpContext, msRepo, *it2, *it, &bAreConnected) )
							{
								mpContext->Error_ResetReport("Error while checking for dag connection.");
								return;
							}
							if (bAreConnected)
							{
								bFound = true;
								break;
							}
						}
					}
				}
				if (!bFound)
				{
					mwHeadCountWarning->SetLabel("WARNING:  Committing from this baseline will create a new head for the \"" + sBranch + "\" branch.");
					this->GetSizer()->Layout();
				}
				else
					mwHeadCountWarning->SetLabelText("");


			}
		}
		else if (actionChoice == vvVerbs::BRANCH_ACTION__CREATE)
		{
			mwHeadCountWarning->SetLabel("WARNING:  A new branch named \"" + sBranch + "\" will be created.");
			this->GetSizer()->Layout();
		}
		else if (actionChoice == vvVerbs::BRANCH_ACTION__REOPEN)
		{
			mwHeadCountWarning->SetLabel("WARNING:  The \"" + sBranch + "\" branch will be reopened.");
			this->GetSizer()->Layout();
		}
		this->UpdateDisplay();
	}
	else
	{
		mwHeadCountWarning->SetLabelText("");
	}
}
IMPLEMENT_DYNAMIC_CLASS(vvBranchControl, wxPanel);

BEGIN_EVENT_TABLE(vvBranchControl, wxPanel)
	// when a radio button is selected, update the display to match it
	EVT_CHECKBOX(                   XRCID("wAttachCheckBox"),OnControlValueChange)
	EVT_TEXT    (                   XRCID("wBranchText"),OnBranchChange)
	EVT_BUTTON  (					XRCID("wBranch_Browse"), OnBrowseBranch)
END_EVENT_TABLE()
