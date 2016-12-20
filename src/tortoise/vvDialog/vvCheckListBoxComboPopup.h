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

#ifndef VV_CHECK_LIST_BOX_COMBO_POPUP_H
#define VV_CHECK_LIST_BOX_COMBO_POPUP_H

#include "precompiled.h"


/**
 * A wxComboPopup for use with wxComboCtrl that contains a wxCheckListBox.
 *
 * Since this popup allows for multiple selection, it can only be used on combo
 * controls that have the wxCB_READONLY style set.  It has no good way to parse
 * the entered text into a set of values to select.
 *
 * If you're not getting a value displayed initially, call RefreshStringValue
 * after adding your items.
 */
class vvCheckListBoxComboPopup : public wxCheckListBox, public wxComboPopup
{
// types
public:
	/**
	 * Available style flags (in addition to wxCheckListBox flags).
	 * Start at bit 15 and work down, to avoid colliding with wxCheckListBox styles.
	 */
	enum Style
	{
		STYLE_SUMMARY = 1 << 15, //< If this style is used, the string representation of the
		                         //< checked items is a numeric summary rather than a list.
		                         //< See SetFormat.
	};

// globals
public:
	/**
	 * The default format used when STYLE_SUMMARY is not specified.
	 */
	static const char* sszDefaultFormat_Separator;

	/**
	 * The default format used when STYLE_SUMMARY is specified.
	 */
	static const char* sszDefaultFormat_Summary;

// functionality
public:
	/**
	 * Default constructor.
	 */
	vvCheckListBoxComboPopup(
		int             iStyle  = 0,            //< [in] Style flags to use.
		const wxString& sFormat = wxEmptyString //< [in] Format to use to represent the checked items as a string.
		                                        //<      If empty, a default format is used.
		                                        //<      See SetFormat.
		);

	/**
	 * Sets the format to use to build a string representation of the selected items.
	 *
	 * There are different possible ways to build the string representation:
	 * 1) Generate a list of all the selected items (default).
	 * 2) Generate a numeric summary of how many items are selected (STYLE_SUMMARY).
	 *
	 * The first option is used by default.  A different option can be specified
	 * instead by passing the indicated style flag to the constructor.
	 *
	 * The format set here is used differently depending on which type of
	 * string representation is used:
	 * 1) The format is the separator to place between listed items.
	 * 2) The format is a printf-style format string which is passed two uint32 values,
	 *    the first one being the number of selected items, and the second one being
	 *    the total number of available items.
	 */
	void SetFormat(
		const wxString& sFormat //< [in] The format string to use.
		                        //<      If empty, a default format is set.
		);

	/**
	 * Refreshes the string value displayed on the combo control.
	 *
	 * This function is necessary because SetPopupControl must be called BEFORE
	 * adding items to this popup, yet we cannot accurately create an initial
	 * string value until AFTER all the items are added.  This function can be
	 * used after adding all the items to generate that initial value.
	 */
	void RefreshStringValue() const;

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
	 * Returns the control within the popup (a wxCheckListBox).
	 */
	wxWindow* GetControl();

	/**
	 * Returns the string currently selected in the popup.
	 */
	wxString GetStringValue() const;

// event handlers
protected:
	void OnMouseMove(wxMouseEvent& e);
	void OnMouseClick(wxMouseEvent& e);
	void OnCloseUp(wxCommandEvent& e);

// private data
private:
	int      miStyle;  //< Style flags to use when creating the control.
	wxString msFormat; //< Format to use to generate a string representation.

// declaration macros
private:
	wxDECLARE_EVENT_TABLE();
};

#endif
