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
#include "vvValidatorMessageBoxReporter.h"


/*
**
** Public Functions
**
*/

void vvValidatorMessageBoxReporter::SetHelpMessage(
	const wxString& sMessage
	)
{
	this->msHelpMessage = sMessage;
}

void vvValidatorMessageBoxReporter::BeginReporting(
	wxWindow* WXUNUSED(wParent)
	)
{
	this->mcMessages.Clear();
}

void vvValidatorMessageBoxReporter::EndReporting(
	wxWindow* wParent
	)
{
	if (this->mcMessages.Count(false, true) == 0u)
	{
		return;
	}

	wxString sMessage = this->mcMessages.BuildMessageString(vvValidatorMessageCollection::GROUP_BY_LABEL, " - ");
	if (this->msHelpMessage != wxEmptyString)
	{
		sMessage = this->msHelpMessage + "\n\n" + sMessage;
	}
	wxMessageBox(sMessage, "Validation Error", wxOK | wxICON_ERROR | wxCENTRE, wParent);
}

void vvValidatorMessageBoxReporter::ReportSuccess(
	class vvValidator* pValidator,
	const wxString&    sMessage
	)
{
	this->mcMessages.AddMessage(true, pValidator->GetLabel(), sMessage);
}

void vvValidatorMessageBoxReporter::ReportError(
	class vvValidator* pValidator,
	const wxString&    sMessage
	)
{
	this->mcMessages.AddMessage(false, pValidator->GetLabel(), sMessage);
}
