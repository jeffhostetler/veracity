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
#include "vvSelectableStaticText.h"

#include "vvFlags.h"


/*
**
** Globals
**
*/

int vvSelectableStaticText::siHeight = 0;


/*
**
** Public Functionality
**
*/

int vvSelectableStaticText::GetDefaultHeight(wxWindow* wParent)
{
	// if we haven't figured out the default height yet, calculate it
	if (vvSelectableStaticText::siHeight == 0)
	{
		// use the height of a standard wxStaticText control
		wxStaticText* wText = new wxStaticText(wParent, wxID_ANY, "Dummy");
		vvSelectableStaticText::siHeight = wText->GetSize().GetHeight();
		delete wText;
	}

	// return the default height
	return vvSelectableStaticText::siHeight;
}

vvSelectableStaticText::vvSelectableStaticText()
{
}

vvSelectableStaticText::vvSelectableStaticText(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxString&    sValue,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	)
{
	this->Create(pParent, cId, sValue, cPosition, cSize, iStyle, cValidator);
}

bool vvSelectableStaticText::Create(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxString&    sValue,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	)
{
	// make sure we use the right styles
	iStyle |= wxTE_READONLY | wxBORDER_NONE;

	// calculate the right size
	wxSize cNewSize = cSize;
	if (
		   (cNewSize.GetHeight() == wxDefaultCoord)
		&& (!vvFlags(iStyle).HasAnyFlag(wxTE_MULTILINE))
		)
	{
		cNewSize.SetHeight(vvSelectableStaticText::GetDefaultHeight(pParent));
	}

	// create the base text control
	if (!wxTextCtrl::Create(pParent, cId, sValue, cPosition, cNewSize, iStyle, cValidator))
		return false;
	if (vvFlags(iStyle).HasAnyFlag(wxTE_MULTILINE))
		this->SetBackgroundColour(this->GetParent()->GetBackgroundColour());
	return true;	
}

IMPLEMENT_DYNAMIC_CLASS(vvSelectableStaticText, wxTextCtrl);

BEGIN_EVENT_TABLE(vvSelectableStaticText, wxTextCtrl)
END_EVENT_TABLE()
