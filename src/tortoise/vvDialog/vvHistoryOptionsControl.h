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

#ifndef VV_HISTORY_OPTIONS_CONTROL_H
#define VV_HISTORY_OPTIONS_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"
#include "vvNullable.h"
#include "vvVerbs.h"
#include "wx/propgrid/propgrid.h"
#include "vvHistoryObjects.h"

/**
 * A custom wxPropertyGrid for displaying and choosing history options.
 */
class vvHistoryOptionsControl : public wxPanel
{
// types
public:
// construction
public:
	/**
	 * Default constructor.
	 */
	vvHistoryOptionsControl();

	/**
	 * Create contructor.
	 */
	vvHistoryOptionsControl(
		vvVerbs& cVerbs,
		vvContext& cContext,
		const wxString& sRepoName,
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator,
		const wxString& sChangesetHID = wxEmptyString

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

	void ApplyColumnPreferences(vvHistoryColumnPreferences& columnPrefs);

	vvHistoryColumnPreferences vvHistoryOptionsControl::GetColumnPreferences();

	void ApplyHistoryFilterOptions(vvHistoryFilterOptions& filterOptions);

	vvHistoryFilterOptions GetHistoryFilterOptions();
	vvHistoryFilterOptions GetHistoryFilterOptions__dont_save();
	bool HaveFilterOptionsChanged();

// event handlers
protected:
	void vvHistoryOptionsControl::OnKeyNavigation(wxNavigationKeyEvent& event);
	vvHistoryFilterOptions m_previousFilterOptions;
// private data
private:
	wxPropertyGrid * m_propGrid;
	vvVerbs * m_pVerbs;
	vvContext * m_pContext;
	wxString m_sRepoName;
// macro declarations
private:
	vvDISABLE_COPY(vvHistoryOptionsControl);
	DECLARE_DYNAMIC_CLASS(vvHistoryOptionsControl);
	DECLARE_EVENT_TABLE();
};


#endif
