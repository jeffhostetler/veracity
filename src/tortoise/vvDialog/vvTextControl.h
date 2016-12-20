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

#ifndef VV_TEXT_CONTROL_H
#define VV_TEXT_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"

/**
 * A text control with some additional standard functionality such as
 * CTRL+A to Select All.  To replace an existing wxTextCtrl in an XRC
 * file with one of these, simply add the subclass="vvTextControl"
 * attribute to its <object> tag.  See the "Comment" field on the
 * vvCommitDialog for an example.
 */
class vvTextControl : public wxTextCtrl
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvTextControl();

	/**
	 * Create contructor.
	 */
	vvTextControl(
		wxWindow*           pParent,
		wxWindowID          cId        = wxID_ANY,
		const wxString&     sValue     = wxEmptyString,
		const wxPoint&      cPosition  = wxDefaultPosition,
		const wxSize&       cSize      = wxDefaultSize,
		long                iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator
		);

// Note: If we end up needing a custom "Create" function on this class, then
//       we'll need to revisit how we specify instances of it in XRC files, because
//       the "subclass" method that we currently use to do so doesn't support
//       calling custom "Create" functions.
//       http://docs.wxwidgets.org/trunk/overview_xrcformat.html#overview_xrcformat_extending_subclass

// event handlers
protected:
	void OnKeyDown(wxKeyEvent& e);

// macro declarations
private:
	vvDISABLE_COPY(vvTextControl);
	DECLARE_DYNAMIC_CLASS(vvTextControl);
	DECLARE_EVENT_TABLE();
};


#endif
