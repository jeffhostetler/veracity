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
#include "vvAboutDialog.h"

#include "vvVerbs.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvDialogHeaderControl.h"

/*
**
** Public Functions
**
*/

bool vvAboutDialog::Create(
	wxWindow*            pParent,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName(), false))
	{
		return false;
	}

	vvDialogHeaderControl * pwHeader = new vvDialogHeaderControl(this, wxID_ANY, wxEmptyString, cVerbs, cContext);
	wxXmlResource::Get()->AttachUnknownControl("wHeader", pwHeader);
	wxString sVersionString;
	if (cVerbs.GetVersionString(cContext, sVersionString))
		pwHeader->AddTextValue("Version:", sVersionString);
	pwHeader->AddTextValue("Copyright:", wxString::Format("2010-%d, SourceGear, LLC", wxDateTime::Now().GetYear()) );
	pwHeader->AddControlValue("For more information:", new wxHyperlinkCtrl(pwHeader, wxID_ANY, "http://veracity-scm.com", "http://veracity-scm.com", wxDefaultPosition, wxDefaultSize, wxHL_ALIGN_LEFT), true);
	wxSize sz = this->GetTextExtent(sVersionString + "WWWWWWWWWWWWWWWWW");
	this->SetSize(sz.GetWidth() + 60, (sz.GetHeight() * 6) + 75);
	// success
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(vvAboutDialog, vvDialog);
BEGIN_EVENT_TABLE(vvAboutDialog, vvDialog)
END_EVENT_TABLE()
