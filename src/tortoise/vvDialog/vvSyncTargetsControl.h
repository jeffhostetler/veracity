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

#ifndef VV_SYNC_TARGETS_CONTROL_H
#define VV_SYNC_TARGETS_CONTROL_H

#include "precompiled.h"
#include "vvVerbs.h"
#include "vvContext.h"
#include "vv_wxListViewComboPopup.h"
#include "wx/combo.h"

/**
 * A custom wxComboBox for displaying and choosing sync targets.
 */
class vvSyncTargetsControl : public wxComboCtrl
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvSyncTargetsControl();

	/**
	 * Create contructor.
	 */
	vvSyncTargetsControl(
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator& cValidator = wxDefaultValidator
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
		const wxValidator& cValidator = wxDefaultValidator
		);

	bool Populate(vvContext&           cContext,
					vvVerbs&             cVerbs,
					const wxString& sRepoDescriptor = wxEmptyString);//Use wxEmptyString to load targets for all repos.
		
// event handlers
protected:
	void OnComboBoxDropdown(wxCommandEvent& cEvent);
	void OnComboBoxResized(wxSizeEvent& cEvent);
// private data
private:
	vvVerbs * m_pVerbs;
	vvContext * m_pContext;
	vv_wxListViewComboPopup * mwListViewComboPopup;
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvSyncTargetsControl);
	DECLARE_EVENT_TABLE();
};


#endif
