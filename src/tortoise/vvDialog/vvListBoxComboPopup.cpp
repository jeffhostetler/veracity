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
#include "vvListBoxComboPopup.h"


/*
**
** Public Functionality
**
*/

bool vvListBoxComboPopup::Create(
	wxWindow* wParent
	)
{
	return wxListBox::Create(wParent, wxID_ANY);
}

wxSize vvListBoxComboPopup::GetAdjustedSize(
	int iMinWidth,
	int WXUNUSED(iPreferredHeight),
	int iMaxHeight
	)
{
	wxSize cSize = this->GetEffectiveMinSize();
	return wxSize(wxMax(iMinWidth, cSize.x), wxMin(iMaxHeight, cSize.y));
}

wxWindow* vvListBoxComboPopup::GetControl()
{
	return this;
}

wxString vvListBoxComboPopup::GetStringValue() const
{
	return this->GetStringSelection();
}

void vvListBoxComboPopup::SetStringValue(
	const wxString& sValue
	)
{
	this->SetStringSelection(sValue);
}

void vvListBoxComboPopup::OnMouseMove(
	wxMouseEvent& e
	)
{
	this->SetSelection(this->HitTest(e.GetPosition()));
}

void vvListBoxComboPopup::OnMouseClick(
	wxMouseEvent& WXUNUSED(e)
	)
{
	this->Dismiss();
}

wxBEGIN_EVENT_TABLE(vvListBoxComboPopup, wxListBox)
	EVT_MOTION(OnMouseMove)
	EVT_LEFT_UP(OnMouseClick)
wxEND_EVENT_TABLE()
