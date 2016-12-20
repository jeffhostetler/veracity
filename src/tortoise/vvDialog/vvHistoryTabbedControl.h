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

#ifndef VV_HISTORY_TABBED_CONTROL_H
#define VV_HISTORY_TABBED_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"
#include "vvNullable.h"
#include "vvVerbs.h"
#include "vvHistoryListControl.h"
#include "vvHistoryOptionsControl.h"
#include "vvHistoryObjects.h"

/**
 * A custom wxDataViewCtrl for displaying and choosing history.
 */
class vvHistoryTabbedControl : public wxPanel
{
// types
public:
	/**
	 * Available style flags (in addition to wxDataViewCtrl flags).
	 * Start at bit 15 and work down, to avoid colliding with wxDataViewCtrl styles.
	 */
	enum Style
	{
		STYLE_CHECKABLE = 1 << 15, //< Display check boxes with each item for the user to toggle.
	};

// construction
public:
	/**
	 * Default constructor.
	 */
	vvHistoryTabbedControl();

	/**
	 * Create contructor.
	 */
	vvHistoryTabbedControl(
		vvVerbs& cVerbs,
		vvContext& cContext,
		const vvRepoLocator& cRepoLocator,
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
	void vvHistoryTabbedControl::LoadData(
		vvHistoryFilterOptions& filterOptions);
	void vvHistoryTabbedControl::ApplyDefaultOptions(vvHistoryFilterOptions& filterOptions);
	wxArrayString GetSelection();
// event handlers
protected:
	void OnTabChanging(wxNotebookEvent& ev);
// private data
private:
	vvContext* m_pContext;
	vvVerbs*   m_pVerbs;
	vvHistoryOptionsControl * m_optionsControl;
	vvHistoryListControl * m_listControl;
	wxNotebook * m_notebook;
// macro declarations
private:
	vvDISABLE_COPY(vvHistoryTabbedControl);
	DECLARE_DYNAMIC_CLASS(vvHistoryTabbedControl);

	DECLARE_EVENT_TABLE();
};


#endif
