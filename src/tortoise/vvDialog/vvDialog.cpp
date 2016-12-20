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
#include "vvDialog.h"

#include "vvProgressExecutor.h"
#include "wx/persist.h"
#include "wx/persist/window.h"
#include "wx/persist/toplevel.h"

/*
**
** Public Functions
**
*/

vvDialog::vvDialog()
	: mpVerbs(NULL)
	, mpContext(NULL)
	, mpExecutor(NULL)
{
	//This is needed for the PersistenctObject stuff, which 
	//wxWidgets uses to restore size and position.
	SetName(this->GetClassInfo()->GetClassName());
}

bool vvDialog::Create(
	wxWindow*        pParent,
	class vvVerbs*   pVerbs,
	class vvContext* pContext,
	const wxString&  sResource,
	bool bRestoreSizeAndPosition
	)
{
	// create the dialog window
	if (sResource.IsEmpty())
	{
		// run default dialog creation logic
		if (!wxDialog::Create(pParent, wxID_ANY, ""))
		{
			return false;
		}

		// set our icon to the default one
		this->SetIcon(wxICON(ApplicationIcon));
	}
	else
	{
		// create the dialog from XRC resources
		if (!wxXmlResource::Get()->LoadDialog(this, pParent, sResource))
		{
			wxLogError("Couldn't load dialog layout from resources.");
			return false;
		}
	}

	// make sure this window and intermediate children validate recursively
	vvDialog::SetValidateRecursiveFlag(this);

	// center ourselves on our parent
	this->CenterOnParent();

	if (bRestoreSizeAndPosition == true)
		wxPersistenceManager::Get().RegisterAndRestore(this);
	// store the verbs and context
	this->mpVerbs   = pVerbs;
	this->mpContext = pContext;

	// success
	return true;
}

void vvDialog::SetExecutor(
	vvProgressExecutor* pExecutor
	)
{
	this->mpExecutor = pExecutor;
}

bool vvDialog::Validate()
{
	bool bResult  = true;

#if wxUSE_VALIDATORS

	// call BeginReporting on all the relevant validators' reporters
	stlReporterList cReporters;
	vvDialog::NotifyReporters(this, cReporters, &vvValidatorReporter::BeginReporting, this);

	// call Validate on all the relevant validators
	bResult = vvDialog::ValidateWindow(this, this);

	// call EndReporting on all the relevant validators' reporters
	cReporters.clear();
	vvDialog::NotifyReporters(this, cReporters, &vvValidatorReporter::EndReporting, this);

#endif // wxUSE_VALIDATORS

	// return the validation result
	return bResult;
}

class vvVerbs* vvDialog::GetVerbs() const
{
	return this->mpVerbs;
}

class vvContext* vvDialog::GetContext() const
{
	return this->mpContext;
}

void vvDialog::OnOk(
	wxCommandEvent& cEvent
	)
{
	// if we don't have an action to execute, allow the base class to close the window
	if (this->mpExecutor == NULL)
	{
		cEvent.Skip();
		return;
	}

	// make sure our data validates and transfers
	if (!this->Validate() || !this->TransferDataFromWindow())
	{
		return;
	}

	// execute our action
	if (this->mpExecutor->Execute(this))
	{
		if (this->IsModal())
		{
			this->EndModal(wxID_OK);
		}
		else
		{
			this->SetReturnCode(wxID_OK);
			this->Hide();
		}
	}
}

void vvDialog::SetValidateRecursiveFlag(
	wxWindow* wWindow
	)
{
#if wxUSE_VALIDATORS

	// set the flag on the window
	wWindow->SetExtraStyle(wWindow->GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);

	// set the flag on its children
	for (wxWindowList::compatibility_iterator it = wWindow->GetChildren().GetFirst(); it; it = it->GetNext() )
	{
		vvDialog::SetValidateRecursiveFlag(it->GetData());
	}

#endif // wxUSE_VALIDATORS
}

void vvDialog::NotifyReporters(
	wxWindow*        wWindow,
	stlReporterList& cReporters,
	NotifyFunction   fNotify,
	wxWindow*        wParent
	)
{
	// get the wx validator assigned to the window
	wxValidator* pWxValidator = wWindow->GetValidator();
	if (pWxValidator != NULL)
	{
		// check if the wx validator is actually a vv validator
		vvValidator* pVvValidator = wxDynamicCast(pWxValidator, vvValidator);
		if (pVvValidator != NULL)
		{
			// check if the vv validator has a reporter assigned
			vvValidatorReporter* pReporter = pVvValidator->GetReporter();
			if (pReporter != NULL)
			{
				// check if the assigned reporter has already been notified
				if (std::find(cReporters.begin(), cReporters.end(), pReporter) == cReporters.end())
				{
					// notify the reporter and add it to the already-notified list
					(pReporter->*fNotify)(wParent);
					cReporters.push_back(pReporter);
				}
			}
		}
	}

	// if the window should be validated recursively,
	// then call this function recursively on each of its children
	if (vvFlags(wWindow->GetExtraStyle()).HasAnyFlag(wxWS_EX_VALIDATE_RECURSIVELY))
	{
		for (wxWindowList::compatibility_iterator it = wWindow->GetChildren().GetFirst(); it; it = it->GetNext() )
		{
			vvDialog::NotifyReporters(it->GetData(), cReporters, fNotify, wParent);
		}
	}
}

bool vvDialog::ValidateWindow(
	wxWindow* wWindow,
	wxWindow* wParent
	)
{
	bool bResult = true;

#if wxUSE_VALIDATORS

	// validate the window
	wxValidator* pValidator = wWindow->GetValidator();
	if (pValidator != NULL && !pValidator->Validate(wParent))
	{
		bResult = false;
	}

	// if the window should be validated recursively,
	// then call this function recursively on each of its children
	if (vvFlags(wWindow->GetExtraStyle()).HasAnyFlag(wxWS_EX_VALIDATE_RECURSIVELY))
	{
		for (wxWindowList::compatibility_iterator it = wWindow->GetChildren().GetFirst(); it; it = it->GetNext() )
		{
			if (!vvDialog::ValidateWindow(it->GetData(), wParent))
			{
				bResult = false;
			}
		}
	}

#endif // wxUSE_VALIDATORS

	return bResult;
}

IMPLEMENT_CLASS(vvDialog, wxDialog);

BEGIN_EVENT_TABLE(vvDialog, wxDialog)
	EVT_BUTTON(wxID_OK, vvDialog::OnOk)
END_EVENT_TABLE()
