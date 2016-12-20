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
#include "vvValidatorStatusButton.h"


/*
**
** Globals
**
*/

static const char* const gpValidBitmap[] =
{
"16 16 3 1",
"   c None",
".  c #007F00",
"+  c #FFFFFF",
"                ",
"                ",
"            ..+ ",
"           ..+  ",
"          ..+   ",
"         ..+    ",
" .+    ...+     ",
"  .+  ...+      ",
"  ..+...+       ",
"   ....+        ",
"   ...+         ",
"    .+          ",
"    +           ",
"                ",
"                ",
"                ",
};
static const wxBitmap gcValidBitmap(gpValidBitmap);

static const char* const gpInvalidBitmap[] =
{
"16 16 3 1",
"   c None",
".  c #7F0000",
"+  c #FFFFFF",
"                ",
"                ",
" ..+        ..+ ",
" ....+     ..+  ",
"  ....+   ..+   ",
"    ...+ .+     ",
"     .....+     ",
"      ...+      ",
"     .....+     ",
"    ...+ ..+    ",
"   ...+   ..+   ",
"  ...+     .+   ",
"  ...+      .+  ",
"   .         .  ",
"                ",
"                ",
};
static const wxBitmap gcInvalidBitmap(gpInvalidBitmap);


/*
**
** Public Functions
**
*/

vvValidatorStatusButton::vvValidatorStatusButton()
{
}

vvValidatorStatusButton::vvValidatorStatusButton(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxString&    sLabel,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator,
	const wxString&    sName
	)
	: wxButton(pParent, cId, sLabel, cPosition, cSize, iStyle, cValidator, sName)
{
}

bool vvValidatorStatusButton::Create(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxString&    sLabel,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator,
	const wxString&    sName
	)
{
	return wxButton::Create(pParent, cId, sLabel, cPosition, cSize, iStyle, cValidator, sName);
}

void vvValidatorStatusButton::BeginReporting(
	wxWindow* WXUNUSED(wWindow)
	)
{
	this->mcMessages.clear();
}

void vvValidatorStatusButton::EndReporting(
	wxWindow* WXUNUSED(wWindow)
	)
{
	this->UpdateDisplay();
}

void vvValidatorStatusButton::ReportSuccess(
	vvValidator* pValidator,
	const wxString& sMessage
	)
{
	this->AddMessage(pValidator, true, sMessage);
}

void vvValidatorStatusButton::ReportError(
	vvValidator* pValidator,
	const wxString& sMessage
	)
{
	this->AddMessage(pValidator, false, sMessage);
}

void vvValidatorStatusButton::UpdateDisplay()
{
	bool success = true;
	for (stlMessageList::const_iterator itMessage = this->mcMessages.begin(); itMessage != this->mcMessages.end(); ++itMessage)
	{
		if (itMessage->bSuccess == false)
		{
			success = false;
		}
	}
	this->SetBitmap(success ? gcValidBitmap : gcInvalidBitmap);
}

void vvValidatorStatusButton::OnClick(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	// make a copy of the current message list
	stlMessageList cMessages(this->mcMessages);

	// begin a new reporting session
	this->BeginReporting(NULL);

	// re-run each validator that we'd previously received a message from
	typedef std::list<vvValidator*> stlValidatorList;
	stlValidatorList cInvokedValidators;
	for (stlMessageList::const_iterator itMessage = cMessages.begin(); itMessage != cMessages.end(); ++itMessage)
	{
		// if we've already invoked this validator, ignore it this time
		if (std::find(cInvokedValidators.begin(), cInvokedValidators.end(), itMessage->pValidator) != cInvokedValidators.end())
		{
			continue;
		}

		// invoke the validator and add it to the list
		itMessage->pValidator->Validate();
		cInvokedValidators.push_back(itMessage->pValidator);
	}

	// end our reporting session
	this->EndReporting(NULL);
}

void vvValidatorStatusButton::AddMessage(
	vvValidator*    pValidator,
	bool            bSuccess,
	const wxString& sMessage
	)
{
	Message cMessage;
	cMessage.pValidator = pValidator;
	cMessage.bSuccess   = bSuccess;
	cMessage.sMessage   = sMessage;
	this->mcMessages.push_back(cMessage);
}

IMPLEMENT_DYNAMIC_CLASS(vvValidatorStatusButton, wxButton);

BEGIN_EVENT_TABLE(vvValidatorStatusButton, wxButton)
	EVT_BUTTON(wxID_ANY, vvValidatorStatusButton::OnClick)
END_EVENT_TABLE()
