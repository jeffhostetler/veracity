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

#ifndef VV_LIST_BOX_COMBO_POPUP_H
#define VV_LIST_BOX_COMBO_POPUP_H

#include "precompiled.h"


/**
 * A wxComboPopup for use with wxComboCtrl.  It attempts to be in all ways
 * consistent with a standard wxComboBox with the sole exception that the
 * width of the popup is not constrained by the width of the main control.
 * This allows the actual text control and button to be relatively narrow
 * while still having values in the popup that are relatively long.
 */
class vvListBoxComboPopup : public wxListBox, public wxComboPopup
{
// wxComboPopup overrides
public:
	/**
	 * Creates the popup control.
	 */
	bool Create(
		wxWindow* wParent //< [in] Parent window for the control.
		);

	/**
	 * Returns the desired size of the popup.
	 */
	wxSize GetAdjustedSize(
		int iMinWidth,        //< [in] Minimum possible width.
		int iPreferredHeight, //< [in] Preferred height.
		int iMaxHeight        //< [in] Maximum possible height.
		);

	/**
	 * Returns the control within the popup (a wxListBox).
	 */
	wxWindow* GetControl();

	/**
	 * Returns the string currently selected in the popup.
	 */
	wxString GetStringValue() const;

	/**
	 * Sets the string currently selected in the popup.
	 */
	void SetStringValue(
		const wxString& sValue //< [in] The string to select.
		);

// event handlers
protected:
	void OnMouseMove(wxMouseEvent& e);
	void OnMouseClick(wxMouseEvent& e);

// declaration macros
private:
	wxDECLARE_EVENT_TABLE();
};

#endif
