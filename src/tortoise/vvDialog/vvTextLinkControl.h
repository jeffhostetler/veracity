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

#ifndef VV_TEXT_LINK_CONTROL_H
#define VV_TEXT_LINK_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"
#include "vvNullable.h"
#include "vvVerbs.h"

class vvTextLinkControl : public wxPanel
{
// types
public:

// construction
public:
	/**
	 * Default constructor.
	 */
	vvTextLinkControl();

	/**
	 * Create contructor.
	 */
	vvTextLinkControl(
		wxString sLinkText,
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator
		);

// functionality
public:
	/**
	 * Creates the control.
	 */
	bool Create(
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator
		);

// event handlers
protected:
	void vvTextLinkControl::OnClick(wxMouseEvent& evt);
	void vvTextLinkControl::OnEnter(wxMouseEvent& evt);
	void vvTextLinkControl::OnLeave(wxMouseEvent& evt);
// private data
private:
	wxStaticText * m_pST;
	wxString m_sText;
// macro declarations
private:
	vvDISABLE_COPY(vvTextLinkControl);
	DECLARE_DYNAMIC_CLASS(vvTextLinkControl);

	DECLARE_EVENT_TABLE();
};


#endif
