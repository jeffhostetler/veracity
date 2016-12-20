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
#include "vvStatusDialog.h"

#include "vvContext.h"
#include "vvDialogHeaderControl.h"
#include "vvFlags.h"
#include "vvStatusControl.h"
#include "vvRepoLocator.h"
#include "vvRevisionLinkControl.h"


/*
**
** Globals
**
*/

/**
 * Style used with the status control.
 */
static const unsigned int guStatusControlStyle   = 0u;

/*
 * Various labels and values used in the header.
 */
static const char* gszDiffTitle           = "Revision Differences";
static const char* gszMergeLabel_Baseline = "Parent:";
static const char* gszMergeLabel_Other    = "Merging:";
static const char* gszDiffLabel_First     = "Comparing:";
static const char* gszDiffLabel_Other     = "with:";
static const char* gszDiffValue_Working   = "working copy";


/*
**
** Public Functions
**
*/

vvStatusDialog::vvStatusDialog()
	:mwStatus(NULL)
{
}

vvStatusDialog::~vvStatusDialog()
{
	if (this->mwStatus != NULL)
	{
		delete this->mwStatus;
	}
}

bool vvStatusDialog::Create(
	wxWindow*            pParent,
	const vvRepoLocator& cRepoLocator,
	vvVerbs&             cVerbs,
	vvContext&           cContext,
	const wxString&      sRevision,
	const wxString&      sRevision2
	)
{
	wxASSERT(cRepoLocator.IsWorkingFolder() || (!sRevision.IsEmpty() && !sRevision2.IsEmpty()));
	wxASSERT(sRevision2.IsEmpty() || !sRevision.IsEmpty());

	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// store the given initialization data
	this->mcRepoLocator = cRepoLocator;
	this->msRevision    = sRevision;
	this->msRevision2   = sRevision2;

	// get XRC-based controls
	this->mwIgnored = XRCCTRL(*this, "wIgnored", wxCheckBox);

	// if we're comparing two revisions, make a few tweaks
	if (!this->msRevision2.IsEmpty())
	{
		this->SetTitle(gszDiffTitle);
		this->mwIgnored->SetValue(false);
		this->mwIgnored->Hide();
	}

	// create our header control
	vvDialogHeaderControl* pHeader = new vvDialogHeaderControl(this, wxID_ANY);
	if (this->msRevision.IsEmpty() && this->msRevision2.IsEmpty())
	{
		// repo locator must be a working folder if no revisions are specified
		wxASSERT(this->mcRepoLocator.IsWorkingFolder());

		// setup the header with standard working copy fields
		pHeader->AddDefaultValues(this->mcRepoLocator.GetWorkingFolder(), cVerbs, cContext);

		// add the parents of the working copy
		wxArrayString cParents;
		this->GetVerbs()->FindWorkingFolder(*this->GetContext(), this->mcRepoLocator.GetWorkingFolder(), NULL, NULL, &cParents);
		for (wxArrayString::const_iterator it = cParents.begin(); it != cParents.end(); it++)
		{
			pHeader->AddControlValue(it == cParents.begin() ? gszMergeLabel_Baseline : gszMergeLabel_Other, new vvRevisionLinkControl(0, *it, pHeader, wxID_ANY, this->mcRepoLocator, *this->GetVerbs(), *this->GetContext(), false, true), true);
		}
	}
	else
	{
		// if any revisions are specified, the first one must be one of them
		wxASSERT(!this->msRevision.IsEmpty());

		// if we're comparing two revisions, only add a repo field to the header
		// otherwise add all the defaults
		if (!this->msRevision2.IsEmpty())
		{
			pHeader->AddRepoValue(this->mcRepoLocator.IsWorkingFolder() ? this->mcRepoLocator.GetWorkingFolder() : this->mcRepoLocator.GetRepoName(), cVerbs, cContext);
		}
		else
		{
			pHeader->AddDefaultValues(this->mcRepoLocator.IsWorkingFolder() ? this->mcRepoLocator.GetWorkingFolder() : this->mcRepoLocator.GetRepoName(), cVerbs, cContext);
		}

		// add the first revision being compared
		pHeader->AddControlValue(gszDiffLabel_First, new vvRevisionLinkControl(0, this->msRevision, pHeader, wxID_ANY, this->mcRepoLocator, *this->GetVerbs(), *this->GetContext(), false, true), true);

		// add the second revision being compared
		if (!this->msRevision2.IsEmpty())
		{
			pHeader->AddControlValue(gszDiffLabel_Other, new vvRevisionLinkControl(0, this->msRevision2, pHeader, wxID_ANY, this->mcRepoLocator, *this->GetVerbs(), *this->GetContext(), false, true), true);
		}
		else
		{
			pHeader->AddTextValue(gszDiffLabel_Other, gszDiffValue_Working);
		}
	}
	wxXmlResource::Get()->AttachUnknownControl("wHeader", pHeader);

	// create our status control
	this->mwStatus = new vvStatusControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, guStatusControlStyle);
	wxXmlResource::Get()->AttachUnknownControl("wStatus", this->mwStatus);

	// success
	return true;
}

bool vvStatusDialog::TransferDataToWindow()
{
	// retrieve status data and populate widgets
	if (!this->mwStatus->FillData(*this->GetVerbs(), *this->GetContext(), this->mcRepoLocator, this->msRevision, this->msRevision2))
	{
		this->GetContext()->Log_ReportError_CurrentError();
		this->GetContext()->Error_Reset();
		return false;
	}

	// update our display to reflect any widget changes
	this->UpdateDisplay();

	// success
	return true;
}

void vvStatusDialog::UpdateDisplay()
{
	// build a status filter
	vvFlags cStatusFilter = vvStatusControl::STATUS_DEFAULT;
	if (this->mwIgnored->GetValue())
	{
		cStatusFilter.AddFlags(vvStatusControl::STATUS_IGNORED);
	}

	// set the status display according to the options
	this->mwStatus->SetDisplay(vvStatusControl::MODE_COLLAPSED, vvStatusControl::ITEM_DEFAULT, cStatusFilter.GetValue());

	// expand all the items
	this->mwStatus->ExpandAll();
}

void vvStatusDialog::OnUpdateEvent(
	wxCommandEvent& WXUNUSED(e)
	)
{
	this->UpdateDisplay();
}

IMPLEMENT_DYNAMIC_CLASS(vvStatusDialog, vvDialog);

BEGIN_EVENT_TABLE(vvStatusDialog, vvDialog)
	EVT_CHECKBOX(XRCID("wIgnored"), OnUpdateEvent)
END_EVENT_TABLE()
