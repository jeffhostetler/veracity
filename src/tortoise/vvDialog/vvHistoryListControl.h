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

#ifndef VV_HISTORY_LIST_CONTROL_H
#define VV_HISTORY_LIST_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"
#include "vvNullable.h"
#include "vvVerbs.h"
#include "vvHistoryObjects.h"

extern const wxEventType newEVT_HISTORY_STATUS;
 
#define EVT_HISTORY_STATUS(id, fn)                                 \
	DECLARE_EVENT_TABLE_ENTRY( newEVT_HISTORY_STATUS, id, -1,  \
	(wxObjectEventFunction) (wxEventFunction)                 \
	(wxCommandEventFunction) & fn, (wxObject*) NULL ),


/**
 * A custom wxDataViewCtrl for displaying and choosing history.
 */
class vvHistoryListControl : public wxPanel, public wxThreadHelper
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

	/**
	 * Data about a single item in the control.
	 */
	struct ItemData
	{
		bool         bChecked; //< Whether or not the stamp is checked.
		wxString     sName;    //< The stamp's name.
		unsigned int uCount;   //< The stamp's use count.
	};

	/**
	 * Data about a set of items.
	 */
	typedef std::list<ItemData> stlItemDataList;

// construction
public:
	/**
	 * Default constructor.
	 */
	vvHistoryListControl();

	/**
	 * Create contructor.
	 */
	vvHistoryListControl(
		vvVerbs& cVerbs,
		vvContext& cContext,
		const vvRepoLocator& cRepoLocator,
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator

		);

	void SendStatusMessage(wxString sStatusString, bool bHasMoreData);

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

	void ApplyColumnPreferences();

	void SetColumnPreferences(vvHistoryColumnPreferences& columnPrefs);

	void vvHistoryListControl::LoadHistory(
		vvHistoryFilterOptions& vvHistoryFilter
		);
	void vvHistoryListControl::LoadHistory(
		vvVerbs::HistoryData data,
		bool bAreRemote = false
		);

	void vvHistoryListControl::StartGetMoreData(bool bClearModelFirst);

	void GetColumnSettings(vvHistoryColumnPreferences& columnPrefs);
	
	wxArrayString vvHistoryListControl::GetSelection();
	
	vvVerbs::HistoryData GetSelectedHistoryEntries();

	void	SetListBackgroundColour(wxColor color);
	wxColor GetListBackgroundColour();

	/**
	Kill the background thread, abandoning the current history lookup.
	*/
	void KillBackgroundThread();
	//Background thread.
	virtual  wxThread::ExitCode Entry();
	///void OnListScroll(wxScrollEvent& event);

// event handlers
protected:
	void OnContext(wxDataViewEvent& event);
	void OnContextMenuItem(wxCommandEvent& event);
	void OnDoubleClick(wxDataViewEvent& evt);
private:
	/**
	Launch a diff between the two revisions.  If sRepoItemPath is provided, this function will check
	if it's a file first.  Files get a file diff (diffmerge)  All others get the full rev vs. rev diff.
	*/
	void LaunchDiff(wxString rev1 = wxEmptyString, wxString rev2 = wxEmptyString);
	
	/**
	Launch the revision details dialog.  Used by the context menu, and also double-click.
	*/
	void vvHistoryListControl::LaunchRevisionDetailsDialog();
	/**
	Launch a revision choosing dialog to pick betwee a set of revisions.
	*/
	vvNullable<wxString> ChooseOneRevision(wxArrayString& cRevArray, const wxString& sTitle, const wxString& sDescription);

	// private data
private:
	vvVerbs * m_pVerbs;
	vvContext * m_pContext;
	wxArrayString m_aBaselineHids; //An array of the Hids for the working folder baselines. Used for Diffing, and also for determining if Merge should be presented.
	vvRepoLocator m_cRepoLocator;
	wxDataViewListCtrl * m_list;
	wxObjectDataPtr<class vvHistoryModel>  mpDataModel;
	//wxDataViewColumn * m_toggleColumn;
	wxDataViewColumn * m_revisionColumn;
	wxDataViewColumn * m_commentColumn;
	wxDataViewColumn * m_branchColumn;
	wxDataViewColumn * m_stampColumn;
	wxDataViewColumn * m_tagColumn;
	wxDataViewColumn * m_dateColumn;
	wxDataViewColumn * m_userColumn;
	vvHistoryColumnPreferences m_columnPrefs;
	vvHistoryFilterOptions m_historyFilter;
	wxCriticalSection m_historyFilterLock;
	bool m_bRevisionsAreRemote; // Controls whether the history data items only exist in a remote repo.
// macro declarations
private:
	vvDISABLE_COPY(vvHistoryListControl);
	DECLARE_DYNAMIC_CLASS(vvHistoryListControl);

	DECLARE_EVENT_TABLE();
};


#endif
