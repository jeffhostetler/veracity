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
#include "vvValidatorRevSpecExistsConstraint.h"

#include "vvContext.h"
#include "vvRevSpecControl.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

static const wxString& gsItemType_Revision = "revision";
static const wxString& gsItemType_Tag      = "tag";
static const wxString& gsItemType_Branch   = "branch";


/*
**
** Public Functions
**
*/

vvValidatorRevSpecExistsConstraint::vvValidatorRevSpecExistsConstraint(
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const wxString& sRepo,
	bool            bExists
	)
	: mcVerbs(cVerbs)
	, mcContext(cContext)
	, msRepo(sRepo)
	, mbExists(bExists)
{
}

wxObject* vvValidatorRevSpecExistsConstraint::Clone() const
{
	return new vvValidatorRevSpecExistsConstraint(*this);
}

bool vvValidatorRevSpecExistsConstraint::Validate(
	vvValidator* pValidator
	) const
{
	wxWindow* wWindow = pValidator->GetWindow();

	if (wWindow->IsKindOf(CLASSINFO(vvRevSpecControl)))
	{
		return this->ValidateRevSpecControl(pValidator, wxStaticCast(wWindow, vvRevSpecControl));
	}

	wxFAIL_MSG("This constraint doesn't support the type of window associated with the given validator.");
	return false;
}

bool vvValidatorRevSpecExistsConstraint::ValidateState(
	vvValidator*     pValidator,
	const vvRevSpec& cRevSpec
	) const
{
	wxArrayString cRevisions;
	wxArrayString cTags;
	wxArrayString cBranches;

	// get the lists of things entered in the rev spec
	if (!cRevSpec.GetRevisions(this->mcContext, cRevisions))
	{
		pValidator->ReportError("Unable to retrieve revision list from rev spec.");
		this->mcContext.Error_Reset();
	}
	if (!cRevSpec.GetTags(this->mcContext, cTags))
	{
		pValidator->ReportError("Unable to retrieve tag list from rev spec.");
		this->mcContext.Error_Reset();
	}
	if (!cRevSpec.GetBranches(this->mcContext, cBranches))
	{
		pValidator->ReportError("Unable to retrieve branch list from rev spec.");
		this->mcContext.Error_Reset();
	}

	// run through and validate each one
	bool bValid = true;
	for (wxArrayString::const_iterator it = cRevisions.begin(); it != cRevisions.end(); ++it)
	{
		bool bExists = false;
		if (!this->mcVerbs.RevisionExists(this->mcContext, this->msRepo, *it, bExists, NULL))
		{
			pValidator->ReportError("Unable to check if revision exists.");
			this->mcContext.Error_Reset();
		}
		if (!this->ValidateItem(pValidator, gsItemType_Revision, *it, bExists))
		{
			bValid = false;
		}
	}
	for (wxArrayString::const_iterator it = cTags.begin(); it != cTags.end(); ++it)
	{
		bool bExists = false;
		if (!this->mcVerbs.TagExists(this->mcContext, this->msRepo, *it, bExists))
		{
			pValidator->ReportError("Unable to check if tag exists.");
			this->mcContext.Error_Reset();
		}
		if (!this->ValidateItem(pValidator, gsItemType_Tag, *it, bExists))
		{
			bValid = false;
		}
	}
	for (wxArrayString::const_iterator it = cBranches.begin(); it != cBranches.end(); ++it)
	{
		bool bExists = false;
		bool bOpen = false;
		if (!this->mcVerbs.BranchExists(this->mcContext, this->msRepo, *it, bExists, bOpen))
		{
			pValidator->ReportError("Unable to check if branch exists.");
			this->mcContext.Error_Reset();
		}
		if (!this->ValidateItem(pValidator, gsItemType_Branch, *it, bExists))
		{
			bValid = false;
		}
	}

	// return the result
	return bValid;
}

bool vvValidatorRevSpecExistsConstraint::ValidateItem(
	vvValidator*    pValidator,
	const wxString& sItemType,
	const wxString& sItemName,
	bool            bItemExists
	) const
{
	if (bItemExists)
	{
		if (this->mbExists)
		{
			pValidator->ReportSuccess(wxString::Format("%s already exists: %s", sItemType.Capitalize(), sItemName));
			return true;
		}
		else
		{
			pValidator->ReportError(wxString::Format("%s must not already exist: %s", sItemType.Capitalize(), sItemName));
			return false;
		}
	}
	else
	{
		if (this->mbExists)
		{
			pValidator->ReportError(wxString::Format("%s must already exist: %s", sItemType.Capitalize(), sItemName));
			return false;
		}
		else
		{
			pValidator->ReportSuccess(wxString::Format("%s doesn't already exist: %s", sItemType.Capitalize(), sItemName));
			return true;
		}
	}
}

bool vvValidatorRevSpecExistsConstraint::ValidateRevSpecControl(
	vvValidator*      pValidator,
	vvRevSpecControl* wRevSpec
	) const
{
	return this->ValidateState(pValidator, wRevSpec->GetRevSpec(this->mcContext));
}
