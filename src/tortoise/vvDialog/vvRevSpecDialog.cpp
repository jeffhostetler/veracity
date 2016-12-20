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
#include "vvRevSpecDialog.h"

#include "vvContext.h"
#include "vvDialogHeaderControl.h"
#include "vvRepoLocator.h"
#include "vvRevSpecControl.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRevSpecExistsConstraint.h"
#include "vvVerbs.h"
#include "vvRevisionLinkControl.h"
#include "vvRevisionChoiceDialog.h"

/*
**
** Globals
**
*/

/**
 * String displayed at the top of the validation error message box.
 */
static const wxString gsValidationErrorMessage = "There was an error in the specified revision.  Please correct the following problems and try again.";

/*
 * Strings displayed as field labels when validation errors occur.
 */
static const wxString gsValidationField_Spec = "Revision";


/*
**
** Public Functions
**
*/

vvRevSpecDialog::~vvRevSpecDialog()
{
	this->mcSpec.Nullfree(*this->GetContext());
}

bool vvRevSpecDialog::Create(
	wxWindow*            pParent,
	const vvRepoLocator& cRepo,
	class vvVerbs&       cVerbs,
	class vvContext&     cContext,
	vvRevSpec            cRevSpec,
	const wxString&      sTitle,
	const wxString&      sLabel,
	bool				 bMerging
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	m_cRepoLocator = cRepo;

	// get the data we need for the repo
	wxString sFolder = wxEmptyString;
	wxString sRepo   = wxEmptyString;
	wxString sBranch = wxEmptyString;
	
	if (cRepo.IsWorkingFolder())
	{
		if (!cVerbs.FindWorkingFolder(cContext, cRepo.GetWorkingFolder(), &sFolder, &sRepo, &m_arBaselineHIDs))
		{
			cContext.Log_ReportError_CurrentError();
			return false;
		}
		if (!cVerbs.GetBranch(cContext, sFolder, sBranch))
		{
			cContext.Log_ReportError_CurrentError();
			return false;
		}
	}
	else
	{
		sRepo = cRepo.GetRepoName();
	}

	// store our initial value
	this->mcSpec = cRevSpec;
	m_bMerging = bMerging;

	// if they gave us a title, override the default one
	if (!sTitle.IsEmpty())
	{
		this->SetTitle(sTitle);
	}

	// if they gave us a label, override the default one
	if (!sLabel.IsEmpty())
	{
		XRCCTRL(*this, "wBox",  wxStaticBoxSizer)->GetStaticBox()->SetLabelText(sLabel);
	}

	// create our header control
	vvDialogHeaderControl* wHeader = new vvDialogHeaderControl(this);
	wHeader->AddDefaultValues(cRepo.GetWorkingFolder(), cVerbs, cContext);
	for (wxArrayString::const_iterator it = m_arBaselineHIDs.begin(); it != m_arBaselineHIDs.end(); it++)
	{
		vvRevisionLinkControl * pRevLink = new vvRevisionLinkControl(0, *it, wHeader, wxID_ANY, cRepo, *this->GetVerbs(), *this->GetContext(), false, true);
		if (it == m_arBaselineHIDs.begin())
			wHeader->AddControlValue("Parent:", pRevLink, true);
		else
			wHeader->AddControlValue("Merging:", pRevLink, true);
	}
	
	wxXmlResource::Get()->AttachUnknownControl("wHeader", wHeader);

	// create our rev spec control
	this->mwSpec = new vvRevSpecControl(this, sRepo, &cVerbs, &cContext, this->mcSpec);
	wxXmlResource::Get()->AttachUnknownControl("wSpec", this->mwSpec);

	// setup validators
	this->mcValidatorReporter.SetHelpMessage(gsValidationErrorMessage);
	this->mwSpec->SetValidator(vvValidator(gsValidationField_Spec)
		.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
		.SetReporter(&this->mcValidatorReporter)
		.AddConstraint(vvValidatorEmptyConstraint(false, &cContext))
		.AddConstraint(vvValidatorRevSpecExistsConstraint(cVerbs, cContext, sRepo, true))
	);

	// focus on the spec control
	this->mwSpec->SetFocus();

	// fit the window to our contained controls and re-center it
	this->Fit();
	this->CenterOnParent();

	// success
	return true;
}

bool vvRevSpecDialog::TransferDataToWindow()
{
	this->mwSpec->SetRevSpec(this->mcSpec, *this->GetContext());
	return true;
}

bool vvRevSpecDialog::TransferDataFromWindow()
{
	vvContext * cContext = this->GetContext();
	this->mcSpec.Nullfree(*cContext);
	this->mcSpec = this->mwSpec->GetRevSpec(*cContext);

	//Now check to see if the branch actually has multiple heads.
	vvRevSpec cSpec_Disambiguated(*cContext);
	vvRevSpecControl::BranchAmbiguationState branchAmbiguationState = this->mwSpec->DisambiguateBranchIfNecessary(m_bMerging, m_arBaselineHIDs, cSpec_Disambiguated);
	if (branchAmbiguationState == vvRevSpecControl::BRANCH__NO_LONGER_AMBIGUOUS)
	{
		this->mcSpec.Nullfree(*cContext);
		this->mcSpec = cSpec_Disambiguated;
	}
	else if (branchAmbiguationState == vvRevSpecControl::BRANCH__STILL_AMBIGUOUS)
	{
		return false;
	}
	//The only other possible value is vvRevSpecControl::BRANCH__WAS_NOT_AMBIGUOUS
	//which means we just return true, as normal
	return true;
}

const vvRevSpec& vvRevSpecDialog::GetRevSpec() const
{
	return this->mcSpec;
}

IMPLEMENT_DYNAMIC_CLASS(vvRevSpecDialog, vvDialog);

BEGIN_EVENT_TABLE(vvRevSpecDialog, vvDialog)
END_EVENT_TABLE()
