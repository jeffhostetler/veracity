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
#include "vvCheckListBoxComboPopup.h"

#include "vvFlags.h"


/*
**
** Globals
**
*/

const char* vvCheckListBoxComboPopup::sszDefaultFormat_Separator = ", ";
const char* vvCheckListBoxComboPopup::sszDefaultFormat_Summary   = "%u/%u selected";


/*
**
** Public Functionality
**
*/

vvCheckListBoxComboPopup::vvCheckListBoxComboPopup(
	int             iStyle,
	const wxString& sFormat
	)
	: miStyle(iStyle)
{
	this->SetFormat(sFormat);
}

void vvCheckListBoxComboPopup::SetFormat(
	const wxString& sFormat
	)
{
	if (!sFormat.IsEmpty())
	{
		this->msFormat = sFormat;
	}
	else if (vvFlags(this->miStyle).HasAnyFlag(STYLE_SUMMARY))
	{
		this->msFormat = sszDefaultFormat_Summary;
	}
	else
	{
		this->msFormat = sszDefaultFormat_Separator;
	}
}

void vvCheckListBoxComboPopup::RefreshStringValue() const
{
	this->GetComboCtrl()->SetValue(this->GetStringValue());
}

bool vvCheckListBoxComboPopup::Create(
	wxWindow* wParent
	)
{
	wxComboCtrl* pCombo = this->GetComboCtrl();

	// make sure we're not used on writable combo boxes
	// because we have no good way to parse the entered text into multiple selections
	wxASSERT_MSG(vvFlags(pCombo->GetWindowStyle()).HasAnyFlag(wxCB_READONLY), "This popup can only be used on combo controls with wxCB_READONLY set.");

	// make sure we get notified when the popup closes
	// since this is a multi-select popup, we won't close ourselves via Dismiss
	pCombo->Bind(wxEVT_COMMAND_COMBOBOX_CLOSEUP, &vvCheckListBoxComboPopup::OnCloseUp, this);

	// create our window
	return wxCheckListBox::Create(wParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, this->miStyle);
}

wxSize vvCheckListBoxComboPopup::GetAdjustedSize(
	int iMinWidth,
	int WXUNUSED(iPreferredHeight),
	int iMaxHeight
	)
{
	wxSize cSize = this->GetEffectiveMinSize();
	return wxSize(wxMax(iMinWidth, cSize.x), wxMin(iMaxHeight, cSize.y));
}

wxWindow* vvCheckListBoxComboPopup::GetControl()
{
	return this;
}

wxString vvCheckListBoxComboPopup::GetStringValue() const
{
	if (vvFlags(this->miStyle).HasAnyFlag(STYLE_SUMMARY))
	{
		unsigned int uChecked = 0u;
		for (unsigned int uIndex = 0u; uIndex < this->GetCount(); ++uIndex)
		{
			if (this->IsChecked(uIndex))
			{
				++uChecked;
			}
		}
		return wxString::Format(this->msFormat, uChecked, this->GetCount());
	}
	else
	{
		wxString sValue = wxEmptyString;
		for (unsigned int uIndex = 0u; uIndex < this->GetCount(); ++uIndex)
		{
			if (this->IsChecked(uIndex))
			{
				if (uIndex > 0u)
				{
					sValue.Append(this->msFormat);
				}
				sValue.Append(this->GetString(uIndex));
			}
		}
		return sValue;
	}
}

void vvCheckListBoxComboPopup::OnMouseMove(
	wxMouseEvent& e
	)
{
	int iItem = this->HitTest(e.GetPosition());
	if (iItem != wxNOT_FOUND && !this->IsSelected(iItem))
	{
		this->SetSelection(iItem);
	}
}

void vvCheckListBoxComboPopup::OnMouseClick(
	wxMouseEvent& e
	)
{
	this->Toggle(this->HitTest(e.GetPosition()));
	this->SetFocus();
}

void vvCheckListBoxComboPopup::OnCloseUp(
	wxCommandEvent& e
	)
{
	this->GetComboCtrl()->SetValue(this->GetStringValue());
	e.Skip();
}

wxBEGIN_EVENT_TABLE(vvCheckListBoxComboPopup, wxCheckListBox)
	EVT_MOTION(OnMouseMove)
	EVT_LEFT_UP(OnMouseClick)
wxEND_EVENT_TABLE()
