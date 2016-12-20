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
#include "vvCurrentUserControl.h"

#include "vvContext.h"
#include "vvRepoLocator.h"
#include "vvSelectableStaticText.h"
#include "vvSettingsDialog.h"
#include "vvVerbs.h"
#include "vvWxHelpers.h"


/*
**
** Constants
**
*/

const wxString vvCurrentUserControl::ssDefaultLinkText = "set current user";


/*
**
** Public Functions
**
*/

vvCurrentUserControl::vvCurrentUserControl()
	: mpVerbs(NULL)
	, mpContext(NULL)
	, mwText(NULL)
	, mwLink(NULL)
{
}

vvCurrentUserControl::vvCurrentUserControl(
	wxWindow*            pParent,
	wxWindowID           cId,
	const vvRepoLocator* pSource,
	vvVerbs*             pVerbs,
	vvContext*           pContext,
	const wxString&      sLinkText,
	const wxPoint&       cPosition,
	const wxSize&        cSize,
	long                 iStyle
	)
	: mpVerbs(NULL)
	, mpContext(NULL)
	, mwText(NULL)
	, mwLink(NULL)
{
	this->Create(pParent, cId, pSource, pVerbs, pContext, sLinkText, cPosition, cSize, iStyle);
}

bool vvCurrentUserControl::Create(
	wxWindow*            pParent,
	wxWindowID           cId,
	const vvRepoLocator* pSource,
	vvVerbs*             pVerbs,
	vvContext*           pContext,
	const wxString&      sLinkText,
	const wxPoint&       cPosition,
	const wxSize&        cSize,
	long                 iStyle
	)
{
	// create the base control
	if (!wxPanel::Create(pParent, cId, cPosition, cSize, iStyle))
	{
		return false;
	}

	// create our child controls
	this->mwLink = new wxHyperlinkCtrl(this, wxID_ANY, sLinkText, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_ALIGN_LEFT);
	this->mwText = new vvSelectableStaticText(this, wxID_ANY);

	// add the children to a sizer
	wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);
	pSizer->Add(new wxSizerItem(this->mwText, wxSizerFlags().Expand()));
	pSizer->Add(new wxSizerItem(this->mwLink, wxSizerFlags().Expand()));

	// set the sizer to manage this window
	this->SetSizer(pSizer);

	// if information was specified, populate the control
	if (pSource != NULL && pVerbs != NULL && pContext != NULL)
	{
		if (!this->Populate(*pSource, *pVerbs, *pContext))
		{
			return false;
		}
	}

	// done
	return true;
}

bool vvCurrentUserControl::Populate(
	const vvRepoLocator& cSource,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// find the repo that we need to look up the current user in
	wxString sRepo;
	if (cSource.IsWorkingFolder())
	{
		if (!cVerbs.FindWorkingFolder(cContext, cSource.GetWorkingFolder(), NULL, &sRepo))
		{
			return false;
		}
	}
	else
	{
		sRepo = cSource.GetRepoName();
	}

	// look up the current user
	wxString sUser;
	if (!cVerbs.GetCurrentUser(cContext, sRepo, sUser))
	{
		return false;
	}

	// store the data that we'll be displaying now
	this->msRepo    = sRepo;
	this->mpVerbs   = &cVerbs;
	this->mpContext = &cContext;

	// update the control state
	this->mwText->SetValue(sUser);
	this->mwText->Show(!sUser.IsEmpty());
	this->mwLink->Show(sUser.IsEmpty());

	// refresh the display
	this->Layout();
	return true;
}

bool vvCurrentUserControl::HasCurrentUser() const
{
	return this->mwText->IsShown();
}

void vvCurrentUserControl::OnLink(
	wxHyperlinkEvent& WXUNUSED(e)
	)
{
	vvSettingsDialog cDialog;

	{
		wxBusyCursor cCursor;
		cDialog.Create(vvFindTopLevelParent(this), *this->mpVerbs, *this->mpContext, vvRepoLocator::FromRepoName(this->msRepo));
	}

	if (cDialog.ShowModal() == wxID_OK)
	{
		if (!this->Populate(vvRepoLocator::FromRepoName(this->msRepo), *this->mpVerbs, *this->mpContext))
		{
			this->mpContext->Log_ReportError("Unable to repopulate current user.");
			this->mpContext->Error_ResetReport(wxEmptyString);
		}
	}
}

IMPLEMENT_DYNAMIC_CLASS(vvCurrentUserControl, wxPanel);

BEGIN_EVENT_TABLE(vvCurrentUserControl, wxPanel)
	EVT_HYPERLINK(wxID_ANY, OnLink)
END_EVENT_TABLE()
