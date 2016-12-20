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
#include <wx/treectrl.h>
#include "vvCppUtilities.h"
#include "vvVerbs.h"

#ifndef VV_REPOBROWSER_CONTROL_H
#define VV_REPOBROWSER_CONTROL_H


class vvRepoBrowserControl : public wxPanel
{
public:
	vvRepoBrowserControl::vvRepoBrowserControl()
	{
	}

	vvRepoBrowserControl::vvRepoBrowserControl(
								vvVerbs& cVerbs,
								vvContext& cContext,
								const wxString& sRepoName,
								const wxString& sChangesetHID,
								wxWindow*          pParent,
								wxWindowID         cId,
								const wxPoint&     cPosition = wxDefaultPosition,
								const wxSize&      cSize = wxDefaultSize,
								long               iStyle = 0,
								const wxValidator& cValidator = wxDefaultValidator
								);

	bool Create(
								wxWindow*          pParent,
								wxWindowID         cId,
								const wxPoint&     cPosition,
								const wxSize&      cSize,
								long               iStyle,
								const wxValidator&  cValidator
								);
	void ExpandTreeItem(wxTreeEvent& event);
	wxString GetItemPath(wxTreeItemId id);
	void SetSelection(wxString& sPath);
	wxString GetSelection();
private:
	void SetSelection(wxTreeItemId& itemID, wxStringTokenizer& sPath);
	void ItemActivated(wxTreeEvent& event);
	vvDISABLE_COPY(vvRepoBrowserControl);
	DECLARE_DYNAMIC_CLASS(vvRepoBrowserControl);

	DECLARE_EVENT_TABLE();
private:
	wxImageList * m_pImageList;
	wxTreeCtrl * m_treeControl;
	vvVerbs * m_pVerbs;
	vvContext * m_pContext;
	wxString m_sRepoName;
	wxString m_sChangesetHID;
};

wxDECLARE_EVENT(EVT_REPO_BROWSER_ITEM_ACTIVATED, wxCommandEvent);

#endif
