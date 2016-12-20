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
#include "vvValidatorValueCompareConstraint.h"


/*
**
** Public Functions
**
*/

vvValidatorValueCompareConstraint::vvValidatorValueCompareConstraint(
	const wxString& sValue,
	bool            bCaseSensitive,
	bool            bEqual
	)
	: msValue(sValue)
	, meCase(bCaseSensitive ? wxString::exact : wxString::ignoreCase)
	, mbEqual(bEqual)
{
}

wxObject* vvValidatorValueCompareConstraint::Clone() const
{
	return new vvValidatorValueCompareConstraint(*this);
}

bool vvValidatorValueCompareConstraint::Validate(
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

	wxFAIL_MSG("This constraint doesn't support the type of window associated with the given validator.");
	return false;
}

bool vvValidatorValueCompareConstraint::ValidateState(
	vvValidator*    pValidator,
	const wxString& sValue
	) const
{
	static const char* szEqual_Success   = "Field's value equals: %s";
	static const char* szEqual_Fail      = "Field's value must equal: %s";
	static const char* szUnequal_Success = "Field's value doesn't equal: %s";
	static const char* szUnequal_Fail    = "Field's value must not equal: %s";

	if (this->mbEqual)
	{
		if (this->msValue.CompareTo(sValue, this->meCase) == 0)
		{
			pValidator->ReportSuccess(wxString::Format(szEqual_Success, this->msValue));
			return true;
		}
		else
		{
			pValidator->ReportError(wxString::Format(szEqual_Fail, this->msValue));
			return false;
		}
	}
	else
	{
		if (this->msValue.CompareTo(sValue, this->meCase) == 0)
		{
			pValidator->ReportError(wxString::Format(szUnequal_Fail, this->msValue));
			return false;
		}
		else
		{
			pValidator->ReportSuccess(wxString::Format(szUnequal_Success, this->msValue));
			return true;
		}
	}
}

bool vvValidatorValueCompareConstraint::ValidateItemContainer(
	vvValidator*              pValidator,
	wxItemContainerImmutable* wContainer
	) const
{
	return this->ValidateState(pValidator, wContainer->GetStringSelection());
}

bool vvValidatorValueCompareConstraint::ValidateTextEntry(
	vvValidator* pValidator,
	wxTextEntry* wTextEntry
	) const
{
	return this->ValidateState(pValidator, wTextEntry->GetValue());
}
