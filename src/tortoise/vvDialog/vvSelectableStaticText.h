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

#ifndef VV_SELECTABLE_STATIC_TEXT_H
#define VV_SELECTABLE_STATIC_TEXT_H

#include "precompiled.h"
#include "vvCppUtilities.h"

class vvSelectableStaticText : public wxTextCtrl
{
// static functionality
public:
	int GetDefaultHeight(wxWindow* wParent);

// construction
public:
	/**
	 * Default constructor.
	 */
	vvSelectableStaticText();

	/**
	 * Create contructor.
	 */
	vvSelectableStaticText(
		wxWindow*           pParent,
		wxWindowID          cId        = wxID_ANY,
		const wxString&     sValue     = wxEmptyString,
		const wxPoint&      cPosition  = wxDefaultPosition,
		const wxSize&       cSize      = wxDefaultSize,
		long                iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator
		);

// functionality
public:
	/**
	 * Creates the control.
	 */
	bool Create(
		wxWindow*           pParent,
		wxWindowID          cId        = wxID_ANY,
		const wxString&     sValue     = wxEmptyString,
		const wxPoint&      cPosition  = wxDefaultPosition,
		const wxSize&       cSize      = wxDefaultSize,
		long                iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator
		);

// private data
private:
	// static
	static int siHeight;

// macro declarations
private:
	vvDISABLE_COPY(vvSelectableStaticText);
	DECLARE_DYNAMIC_CLASS(vvSelectableStaticText);
	DECLARE_EVENT_TABLE();
};


#endif
