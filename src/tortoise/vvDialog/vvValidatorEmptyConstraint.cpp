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
#include "vvValidatorEmptyConstraint.h"

#include "vvContext.h"
#include "vvCurrentUserControl.h"
#include "vvRevSpec.h"
#include "vvRevSpecControl.h"
#include "vvStatusControl.h"


/*
**
** Public Functions
**
*/

vvValidatorEmptyConstraint::vvValidatorEmptyConstraint(
	bool       bEmpty,
	vvContext* pContext
	)
	: mbEmpty(bEmpty)
	, mpContext(pContext)
{
}

wxObject* vvValidatorEmptyConstraint::Clone() const
{
	return new vvValidatorEmptyConstraint(*this);
}

bool vvValidatorEmptyConstraint::Validate(
	vvValidator* pValidator
	) const
{
	wxWindow* wWindow = pValidator->GetWindow();

#if wxUSE_CHECKLISTBOX
	if (wWindow->IsKindOf(CLASSINFO(wxCheckListBox)))
	{
		return this->ValidateItemContainer(pValidator, wxStaticCast(wWindow, wxCheckListBox));
	}
#endif

// wxComboBox needs to be checked before wxChoice
// because wxComboBox derives from wxChoice, but we want to validate it differently
#if wxUSE_COMBOBOX
	if (wWindow->IsKindOf(CLASSINFO(wxComboBox)))
	{
		wxComboBox* w = wxStaticCast(wWindow, wxComboBox);
		if (w->HasFlag(wxCB_READONLY))
		{
			// a read-only combo box behaves like a wxChoice
			// so verify that it has a valid selection
			return this->ValidateItemContainer(pValidator, w);
		}
		else
		{
			// other combo boxes might have any arbitrary text value
			// so verify that the value isn't empty
			return this->ValidateTextEntry(pValidator, w);
		}
	}
#endif

#if wxUSE_CHOICE
	if (wWindow->IsKindOf(CLASSINFO(wxChoice)))
	{
		return this->ValidateItemContainer(pValidator, wxStaticCast(wWindow, wxChoice));
	}
#endif

#if wxUSE_DATEPICKCTRL
	if (wWindow->IsKindOf(CLASSINFO(wxDatePickerCtrl)))
	{
		return this->ValidateDatePickerCtrl(pValidator, wxStaticCast(wWindow, wxDatePickerCtrl));
	}
#endif

#if wxUSE_LISTBOX
	if (wWindow->IsKindOf(CLASSINFO(wxListBox)))
	{
		return this->ValidateItemContainer(pValidator, wxStaticCast(wWindow, wxListBox));
	}
#endif

#if wxUSE_RADIOBOX
	if (wWindow->IsKindOf(CLASSINFO(wxRadioBox)))
	{
		return this->ValidateItemContainer(pValidator, wxStaticCast(wWindow, wxRadioBox));
	}
#endif

#if wxUSE_TEXTCTRL
	if (wWindow->IsKindOf(CLASSINFO(wxTextCtrl)))
	{
		return this->ValidateTextEntry(pValidator, wxStaticCast(wWindow, wxTextCtrl));
	}
#endif

	if (wWindow->IsKindOf(CLASSINFO(vvCurrentUserControl)))
	{
		return this->ValidateCurrentUserControl(pValidator, wxStaticCast(wWindow, vvCurrentUserControl));
	}

	if (wWindow->IsKindOf(CLASSINFO(vvRevSpecControl)))
	{
		return this->ValidateRevSpecControl(pValidator, wxStaticCast(wWindow, vvRevSpecControl));
	}

	if (wWindow->IsKindOf(CLASSINFO(vvStatusControl)))
	{
		return this->ValidateStatusControl(pValidator, wxStaticCast(wWindow, vvStatusControl));
	}

	wxFAIL_MSG("This constraint doesn't support the type of window associated with the given validator.");
	return false;
}

bool vvValidatorEmptyConstraint::ValidateState(
	vvValidator* pValidator,
	bool         bItems,
	bool         bIsEmpty
	) const
{
	static const char* szEmpty_Success_Value     = "Field is empty";
	static const char* szEmpty_Fail_Value        = "Field must not be empty";
	static const char* szNotEmpty_Success_Value  = "Field is not empty";
	static const char* szNotEmpty_Fail_Value     = "Field must be empty";
	static const char* szEmpty_Success_Select    = "Field has no selection";
	static const char* szEmpty_Fail_Select       = "Field must have a selection";
	static const char* szNotEmpty_Success_Select = "Field has a selection";
	static const char* szNotEmpty_Fail_Select    = "Field must not have a selection";

	if (bIsEmpty)
	{
		if (this->mbEmpty)
		{
			pValidator->ReportSuccess(bItems ? szEmpty_Success_Select : szEmpty_Success_Value);
			return true;
		}
		else
		{
			pValidator->ReportError(bItems ? szEmpty_Fail_Select : szEmpty_Fail_Value);
			return false;
		}
	}
	else
	{
		if (this->mbEmpty)
		{
			pValidator->ReportError(bItems ? szNotEmpty_Fail_Select : szNotEmpty_Fail_Value);
			return false;
		}
		else
		{
			pValidator->ReportSuccess(bItems ? szNotEmpty_Success_Select : szNotEmpty_Success_Value);
			return true;
		}
	}
}

bool vvValidatorEmptyConstraint::ValidateDatePickerCtrl(
	vvValidator*      pValidator,
	wxDatePickerCtrl* wDatePicker
	) const
{
	return this->ValidateState(pValidator, false, wDatePicker->GetValue() == wxInvalidDateTime);
}

bool vvValidatorEmptyConstraint::ValidateItemContainer(
	vvValidator*              pValidator,
	wxItemContainerImmutable* wContainer
	) const
{
	return this->ValidateState(pValidator, true, wContainer->GetSelection() == wxNOT_FOUND);
}

bool vvValidatorEmptyConstraint::ValidateTextEntry(
	vvValidator* pValidator,
	wxTextEntry* wTextEntry
	) const
{
	return this->ValidateState(pValidator, false, wTextEntry->GetValue().IsEmpty());
}

bool vvValidatorEmptyConstraint::ValidateCurrentUserControl(
	vvValidator*          pValidator,
	vvCurrentUserControl* wUser
	) const
{
	return this->ValidateState(pValidator, true, !wUser->HasCurrentUser());
}

bool vvValidatorEmptyConstraint::ValidateRevSpecControl(
	vvValidator*      pValidator,
	vvRevSpecControl* wRevSpec
	) const
{
	unsigned int uCount = 0u;

	wxASSERT_MSG(this->mpContext != NULL, "Using vvValidatorEmptyConstraint with a vvRevSpecControl requires passing a vvContext to the constructor.");

	if (!wRevSpec->GetRevSpec(*this->mpContext).GetTotalCount(*this->mpContext, uCount))
	{
		this->mpContext->Error_ResetReport("Unable to retrieve count from rev spec control.");
	}

	return this->ValidateState(pValidator, false, uCount == 0u);
}

bool vvValidatorEmptyConstraint::ValidateStatusControl(
	vvValidator*     pValidator,
	vvStatusControl* wStatus
	) const
{
	return this->ValidateState(pValidator, true, wStatus->GetItemData(NULL, 0u, true) == 0u);
}
