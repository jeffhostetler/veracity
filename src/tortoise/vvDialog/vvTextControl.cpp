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
#include "vvTextControl.h"


/*
**
** Public Functionality
**
*/

vvTextControl::vvTextControl()
{
}

vvTextControl::vvTextControl(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxString&    sValue,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	)
	: wxTextCtrl(pParent, cId, sValue, cPosition, cSize, iStyle, cValidator)
{
}

void vvTextControl::OnKeyDown(
	wxKeyEvent& e
	)
{
	if (e.ControlDown())
	{
		switch(e.GetKeyCode())
		{
		case 'a':
		case 'A':
			this->SetSelection(-1, -1);
			break;
		default:
			e.Skip();
			break;
		}
	}
	else
	{
		e.Skip();
	}
}

IMPLEMENT_DYNAMIC_CLASS(vvTextControl, wxTextCtrl);

BEGIN_EVENT_TABLE(vvTextControl, wxTextCtrl)
	EVT_KEY_DOWN(OnKeyDown)
END_EVENT_TABLE()
