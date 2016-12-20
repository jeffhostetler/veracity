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
#include "vvDialogHeaderControl.h"

#include "vvContext.h"
#include "vvCurrentUserControl.h"
#include "vvFlags.h"
#include "vvRepoLocator.h"
#include "vvSelectableStaticText.h"
#include "vvVerbs.h"


/*
**
** Globals
**
*/

const wxArtID vvDialogHeaderControl::scDefaultBitmapName = "SourceGear_64";

const char* vvDialogHeaderControl::sszLabel_Folder = "Folder:";
const char* vvDialogHeaderControl::sszLabel_Repo   = "Repository:";
const char* vvDialogHeaderControl::sszLabel_Branch = "Branch:";


/*
**
** Public Functions
**
*/

vvDialogHeaderControl::vvDialogHeaderControl()
	: mpSizer(NULL)
	, mwBitmap(NULL)
{
}

vvDialogHeaderControl::vvDialogHeaderControl(
	wxWindow*      pParent,
	wxWindowID     cId,
	const wxArtID& cBitmap,
	const wxPoint& cPosition,
	const wxSize&  cSize,
	long           iStyle
	)
	: mpSizer(NULL)
	, mwBitmap(NULL)
{
	this->Create(pParent, cId, cBitmap, cPosition, cSize, iStyle);
}

vvDialogHeaderControl::vvDialogHeaderControl(
	wxWindow*       pParent,
	wxWindowID      cId,
	const wxString& sFolder,
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const wxArtID&  cBitmap,
	const wxPoint&  cPosition,
	const wxSize&   cSize,
	long            iStyle
	)
	: mpSizer(NULL)
	, mwBitmap(NULL)
{
	this->Create(pParent, cId, sFolder, cVerbs, cContext, cBitmap, cPosition, cSize, iStyle);
}

bool vvDialogHeaderControl::Create(
	wxWindow*      pParent,
	wxWindowID     cId,
	const wxArtID& cBitmap,
	const wxPoint& cPosition,
	const wxSize&  cSize,
	long           iStyle
	)
{
	// create the base control
	if (!wxPanel::Create(pParent, cId, cPosition, cSize, iStyle))
	{
		return false;
	}

	// create the bitmap control
	this->mwBitmap = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap(cBitmap));

	// create a flex grid sizer to layout any added controls
	this->mpSizer = new wxFlexGridSizer(2, wxSize(8,8));
	this->mpSizer->AddGrowableCol(1u, 1);

	// create a horizontal box sizer to layout the grid and bitmap
	wxBoxSizer* pBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	pBoxSizer->Add(this->mpSizer, wxSizerFlags().Center().Proportion(1));
	pBoxSizer->AddSpacer(8);
	pBoxSizer->Add(this->mwBitmap, wxSizerFlags().Proportion(0));

	// set the horizontal sizer to this window
	this->SetSizer(pBoxSizer);

	// done
	return true;
}

bool vvDialogHeaderControl::Create(
	wxWindow*       pParent,
	wxWindowID      cId,
	const wxString& sFolder,
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const wxArtID&  cBitmap,
	const wxPoint&  cPosition,
	const wxSize&   cSize,
	long            iStyle
	)
{
	return (
		   this->Create(pParent, cId, cBitmap, cPosition, cSize, iStyle)
		&& this->AddDefaultValues(sFolder, cVerbs, cContext)
		);
}

bool vvDialogHeaderControl::AddControlValue(
	const wxString& sLabel,
	wxWindow*       wControl,
	bool            bExpand
	)
{
	wxASSERT(wControl->GetParent() == this);

	// create a label and add it
	wxStaticText* wLabel = new wxStaticText(this, wxID_ANY, sLabel);
	this->mpSizer->Add(wLabel, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

	// add the control
	wxSizerFlags cFlags;
	cFlags.Align(wxALIGN_CENTER_VERTICAL);
	if (bExpand)
	{
		cFlags.Expand();
	}
	this->mpSizer->Add(wControl, cFlags);

	// re-layout the control
	return this->Layout();
}

bool vvDialogHeaderControl::AddTextValue(
	const wxString& sLabel,
	const wxString& sValue
	)
{
	return this->AddControlValue(sLabel, new vvSelectableStaticText(this, wxID_ANY, sValue), true);
}

bool vvDialogHeaderControl::SetTextValue(
	const wxString& sLabel,
	const wxString& sValue
	)
{
	// iterate through our sizer's even items
	// since we have two columns, these should be the labels in the left column
	for (size_t uIndex = 0u; uIndex < this->mpSizer->GetItemCount(); uIndex += 2u)
	{
		// get the item and make sure it manages a window
		// (as opposed to a spacer or something else)
		wxSizerItem* pItem = this->mpSizer->GetItem(uIndex);
		if (!pItem->IsWindow())
		{
			continue;
		}

		// downcast the managed window to a static text
		// which is what all of our labels are
		wxStaticText* pStaticText = wxDynamicCast(pItem->GetWindow(), wxStaticText);
		if (pStaticText == NULL)
		{
			continue;
		}

		// check if the label matches the one we need to set the value for
		if (pStaticText->GetLabelText() != sLabel)
		{
			continue;
		}

		// get the next item from the sizer
		// which will be managing the control associated with this label
		pItem = this->mpSizer->GetItem(uIndex + 1u);
		if (!pItem->IsWindow())
		{
			continue;
		}

		// downcast that to a text control
		// which is the only kind we can currently set with this function
		// (and also the only kind we really use for values, currently)
		wxTextCtrl* pTextCtrl = wxDynamicCast(pItem->GetWindow(), wxTextCtrl);
		if (pTextCtrl == NULL)
		{
			continue;
		}

		// update the value
		pTextCtrl->SetValue(sValue);
		return true;
	}

	return false;
}

bool vvDialogHeaderControl::AddFolderValue(
	const wxString& sFolder,
	vvVerbs&        cVerbs,
	vvContext&      cContext
	)
{
	wxString sWorkingFolder;
	if (cVerbs.FindWorkingFolder(cContext, sFolder, &sWorkingFolder, NULL))
	{
		return this->AddTextValue(sszLabel_Folder, sWorkingFolder);
	}
	else
	{
		cContext.Error_Reset();
		return this->AddTextValue(sszLabel_Folder, sFolder);
	}
}

bool vvDialogHeaderControl::AddRepoValue(
	const wxString& sFolder,
	vvVerbs&        cVerbs,
	vvContext&      cContext
	)
{
	wxString sRepo;
	return (
		   cVerbs.FindWorkingFolder(cContext, sFolder, NULL, &sRepo)
		&& this->AddTextValue(sszLabel_Repo, sRepo)
		);
}

bool vvDialogHeaderControl::AddFolderAndRepoValues(
	const wxString& sFolderOrRepoName,
	vvVerbs&        cVerbs,
	vvContext&      cContext
	)
{
	wxString sWorkingFolder;
	wxString sRepo;
	if (wxDir::Exists(sFolderOrRepoName) || wxFile::Exists(sFolderOrRepoName) )
	{		   
		return cVerbs.FindWorkingFolder(cContext, sFolderOrRepoName, &sWorkingFolder, &sRepo)
			&& this->AddTextValue(sszLabel_Folder, sWorkingFolder)
			&& this->AddTextValue(sszLabel_Repo, sRepo);
	}
	else
	{
		return (cVerbs.LocalRepoExists(cContext, sFolderOrRepoName) && this->AddTextValue(sszLabel_Repo, sFolderOrRepoName));
	}
}

bool vvDialogHeaderControl::AddBranchValue(
	const wxString& sFolder,
	vvVerbs&        cVerbs,
	vvContext&      cContext
	)
{
	if (!wxDir::Exists(sFolder))
	{
		return false;
	}

	wxString sBranch;
	if (!cVerbs.GetBranch(cContext, sFolder, sBranch))
	{
		return false;
	}
	if (sBranch.IsEmpty())
	{
		sBranch = "(currently detached)";
	}

	if (!this->AddTextValue(sszLabel_Branch, sBranch))
	{
		return false;
	}

	return true;
}

bool vvDialogHeaderControl::AddDefaultValues(
	const wxString& sFolder,
	vvVerbs&        cVerbs,
	vvContext&      cContext
	)
{
	return (
		   this->AddFolderAndRepoValues(sFolder, cVerbs, cContext)
		&& this->AddBranchValue(sFolder, cVerbs, cContext)
		);
}

IMPLEMENT_DYNAMIC_CLASS(vvDialogHeaderControl, wxPanel);

BEGIN_EVENT_TABLE(vvDialogHeaderControl, wxPanel)
END_EVENT_TABLE()
