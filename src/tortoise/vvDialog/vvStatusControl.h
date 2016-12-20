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

#ifndef VV_STATUS_CONTROL_H
#define VV_STATUS_CONTROL_H

#include "precompiled.h"
#include "vvNullable.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvRepoLocator.h"

/**
 * A custom wxDataViewCtrl for displaying status information.
 */
class vvStatusControl : public wxDataViewCtrl
{
// types
public:
	/**
	 * Available style flags (in addition to wxDataViewCtrl flags).
	 * Start at bit 15 and work down, to avoid colliding with wxDataViewCtrl styles.
	 */
	enum Style
	{
		STYLE_SELECT = 1 << 15, //< Allow items to be selected via check boxes.
	};

	/**
	 * Available modes of organizing the status data.
	 */
	enum Mode
	{
		MODE_FLAT,      //< Data is a flat grid.
		MODE_FOLDER,    //< Data is hierarchical based on folders.
		MODE_COLLAPSED, //< Like FOLDER, but chains of single children are collapsed.
		MODE_COUNT,     //< Number of entries in this enumeration, for iteration.

		// default mode
		MODE_DEFAULT = MODE_COLLAPSED,
	};

	/**
	 * Possible types of items that can be displayed in the control.
	 */
	enum ItemType
	{
		ITEM_FILE      = 1 << 0, //< Item is a file with status info.
		ITEM_FOLDER    = 1 << 1, //< Item is a folder with status info.
		ITEM_SYMLINK   = 1 << 2, //< Item is a symlink with status info.
		ITEM_SUBMODULE = 1 << 3, //< Item is a submodule with status info.
		ITEM_CONTAINER = 1 << 4, //< Item is just a container for other items.
		ITEM_CHANGE    = 1 << 5, //< Item is a single change of another item with multiple changes.
		ITEM_CONFLICT  = 1 << 6, //< Item is a single conflict on another item.
		ITEM_LOCK	   = 1 << 7, //< Item is a lock
		ITEM_FOLDER__DONT_RECURSE_IF_SELECTED    = 1 << 8, //< If a folder is fully selected, don't look for items underneath it.
		ITEM_LAST      = 1 << 9, //< Last entry in this enumeration, for iteration.

		// mask for all item flags
		ITEM_ALL       = ITEM_FILE
		               | ITEM_FOLDER
		               | ITEM_SYMLINK
		               | ITEM_SUBMODULE
		               | ITEM_CONTAINER
		               | ITEM_CHANGE
					   | ITEM_LOCK
		               | ITEM_CONFLICT,

		// mask for all item flags
		ITEM_ALL__DONT_RECURSE_IF_SELECTED       = ITEM_FILE
					   | ITEM_FOLDER
		               | ITEM_FOLDER__DONT_RECURSE_IF_SELECTED
		               | ITEM_SYMLINK
		               | ITEM_SUBMODULE
		               | ITEM_CONTAINER
		               | ITEM_CHANGE
					   | ITEM_LOCK
		               | ITEM_CONFLICT,
		// default item type mask
		ITEM_DEFAULT   = ITEM_ALL,
	};

	/**
	 * Data about an ItemType value.
	 */
	struct ItemTypeData
	{
		ItemType    eType;  //< The item type value this data is about.
		const char* szName; //< The friendly display name for the type.
		const char* szIcon; //< The icon associated with the type.
	};

	/**
	 * Possible status that can be displayed in the control.
	 *
	 * These flags are mutually exclusive on a per-file basis.  They are organized
	 * as bit-flags so that users of vvStatusControl may specify a set of statuses
	 * with a single number for things like choosing which statuses to display.
	 */
	enum Status
	{
		STATUS_ADDED     = 1 <<  0, //< Item was added.
		STATUS_DELETED   = 1 <<  1, //< Item was deleted.
		STATUS_MODIFIED  = 1 <<  2, //< Item was modified.
		STATUS_LOST      = 1 <<  3, //< Item was removed from disk, but not deleted from source control.
		STATUS_FOUND     = 1 <<  4, //< Item was found on disk, but not added to source control.
		STATUS_RENAMED   = 1 <<  5, //< Item was renamed.
		STATUS_MOVED     = 1 <<  6, //< Item was moved to a different folder.
		STATUS_CONFLICT  = 1 <<  7, //< Item has unresolved conflicts.
		STATUS_RESOLVED  = 1 <<  8, //< Conflict has been resolved.
		                            //< This status is never used with real items, only individual conflicts.
		STATUS_MULTIPLE  = 1 <<  9, //< Item has multiple independent changes.
		STATUS_IGNORED   = 1 << 10, //< Item is FOUND, but also ignored by the ignore settings.
		STATUS_UNCHANGED = 1 << 11, //< Item is unchanged.
		STATUS_LOCKED	 = 1 << 12, //< Item is locked.
		STATUS_LOCK_CONFLICT	 = 1 << 13, //< Item is locked.
		STATUS_LAST      = 1 << 14, //< Last entry in this enumeration, for iteration.

		// mask for all status flags
		STATUS_ALL      = STATUS_ADDED
		                | STATUS_DELETED
		                | STATUS_MODIFIED
		                | STATUS_LOST
		                | STATUS_FOUND
		                | STATUS_RENAMED
		                | STATUS_MOVED
		                | STATUS_LOCKED
						| STATUS_LOCK_CONFLICT
						| STATUS_CONFLICT
		                | STATUS_RESOLVED
		                | STATUS_MULTIPLE
		                | STATUS_IGNORED
		                | STATUS_UNCHANGED,

		// default status mask
		STATUS_DEFAULT  = STATUS_ALL & ~STATUS_IGNORED
	};

	/**
	 * Data about a Status value.
	 */
	struct StatusData
	{
		Status      eStatus; //< The status value this data is about.
		const char* szName;  //< The friendly display name for the status.
		const char* szIcon;  //< The icon associated with the status.
	};

	/**
	 * Available columns to be displayed.
	 */
	enum Column
	{
		COLUMN_FILENAME      = 1 << 0, //< Displays the filename, hierarchically if sorted.
		COLUMN_STATUS        = 1 << 1, //< Displays the status.
		COLUMN_FROM          = 1 << 2, //< Displays the old location/name of moves/renames.
		//COLUMN_ATTRIBUTES    = 1 << 3, //< Displays whether or not an item's attributes were modified.
		//COLUMN_EX_ATTRIBUTES = 1 << 4, //< Displays whether or not an item's extended attributes were modified.
		COLUMN_GID           = 1 << 3, //< Displays the item's GID.
		COLUMN_HID_NEW       = 1 << 4, //< Displays the item's content's new/current HID.
		COLUMN_HID_OLD       = 1 << 5, //< Displays the item's content's old HID.
		COLUMN_LAST          = 1 << 6, //< Last entry in this enumeration, for iteration.

		// mask for all column flags
		COLUMN_ALL        = COLUMN_FILENAME
		                  | COLUMN_STATUS
		                  | COLUMN_FROM
//		                  | COLUMN_ATTRIBUTES
//		                  | COLUMN_EX_ATTRIBUTES
		                  | COLUMN_GID
		                  | COLUMN_HID_NEW
		                  | COLUMN_HID_OLD,

		// default column mask
		COLUMN_DEFAULT    = COLUMN_FILENAME | COLUMN_STATUS | COLUMN_FROM,
	};

	/**
	 * Data about a Column value.
	 */
	struct ColumnData
	{
		Column      eColumn; //< The column value this data is about.
		const char* szName;  //< The friendly display name for the column.
	};

	/**
	 * Data about a single item in the control.
	 */
	struct ItemData
	{
		bool     bSelected;     //< Whether or not the item is selected.
		wxString sRepoPath;     //< The item's full repo path and filename.
		wxString sFilename;     //< The item's filename (no path info).
		ItemType eType;         //< The item's type.
		Status   eStatus;       //< The item's status.
		wxString sFrom;         //< Where the item was moved/renamed from, if applicable.
//		bool     bAttributes;   //< Whether or not the item's attributes were modified.
		bool     bUndone;       //< Whether or not the item's changes were "undone".
		wxString sGid;          //< The item's GID.
		wxString sHidNew;       //< The item's current/new content's HID.
		wxString sHidOld;       //< The item's old content's HID.
	};

	/**
	 * Data about a set of items.
	 */
	typedef std::list<ItemData> stlItemDataList;

// static functionality
public:
	/**
	 * Gets data about an ItemType value.
	 */
	static const ItemTypeData* GetItemTypeData(
		ItemType eType //< [in] The value to get data about.
		);

	/**
	 * Gets data about a Status value.
	 */
	static const StatusData* GetStatusData(
		Status eStatus //< [in] The value to get data about.
		);

	/**
	 * Gets data about a Column value.
	 */
	static const ColumnData* GetColumnData(
		Column eColumn //< [in] The value to get data about.
		);

// construction
public:
	/**
	 * Default constructor.
	 */
	vvStatusControl();

	/**
	 * Create contructor.
	 */
	vvStatusControl(
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		Mode               eMode      = MODE_DEFAULT,
		unsigned int       uItemTypes = ITEM_DEFAULT,
		unsigned int       uStatuses  = STATUS_DEFAULT,
		unsigned int       uDisabledStatuses = 0,
		unsigned int       uColumns   = COLUMN_DEFAULT,
		const wxValidator& cValidator = wxDefaultValidator
		);

// functionality
public:
	/**
	 * Creates the status control.
	 */
	bool Create(
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		Mode               eMode      = MODE_DEFAULT,
		unsigned int       uItemTypes = ITEM_DEFAULT,
		unsigned int       uStatuses  = STATUS_DEFAULT,
		unsigned int       uDisabledStatuses = 0,
		unsigned int       uColumns   = COLUMN_DEFAULT,
		const wxValidator& cValidator = wxDefaultValidator
		);

	/**
	 * Sets which records are displayed by the control and how they're organized.
	 */
	void SetDisplay(
		Mode         eMode,      //< [in] The mode to use to organize the data.
		unsigned int uItemTypes, //< [in] The types of items to display (bitmask of ItemType values).
		unsigned int uStatuses   //< [in] The statuses of items to display (bitmask of Status values).
		);

	/**
	 * Gets the current mode of organizing the data.
	 */
	Mode GetDisplayMode() const;

	/**
	 * Gets the mode used when sorted by filename (hierarchically).
	 */
	Mode GetDisplayModeHierarchical() const;

	/**
	 * Gets the set of item types being displayed.
	 */
	unsigned int GetDisplayTypes() const;

	/**
	 * Gets the set of statuses being displayed.
	 */
	unsigned int GetDisplayStatuses() const;

	/**
	 * Sets which columns should be displayed.
	 */
	void SetColumns(
		unsigned int uColumns //< [in] Bitmask of Column values to display.
		);

	/**
	 * Gets the set of columns currently being displayed as a bitmask of Column values.
	 */
	unsigned int GetColumns() const;

	/**
	 * Rerun the status.
	 **/
	void vvStatusControl::RefreshData();

	/**
	 * Fills the control with status data retrieved from the given vvVerbs implementation.
	 */
	bool FillData(
		class vvVerbs&   cVerbs,                   //< [in] The vvVerbs implementation to retrieve status data from.
		class vvContext& cContext,                 //< [in] The context to use with the verbs.
		const vvRepoLocator&	cRepoLocator,	   //< [in] The repo to use.
		const wxString&  sRevision = wxEmptyString, //< [in] The revision to compare the folder to.
		                                           //<      If empty, then the folder's first parent will be used.
		const wxString&  sRevision2 = wxEmptyString //< [in] If both revisions are supplied, We will show the differences between those two revisions.
		);

	/**
	 * Expands all nodes in the control.
	 */
	void ExpandAll();

	/**
	 * Retrieves data about items in the control.
	 * Returns the number of items retrieved.
	 */
	unsigned int GetItemData(
		stlItemDataList* pItems     = NULL,       //< [out] List to add retrieved items to.
		                                          //<       May be NULL if you're only interested in the count.
		unsigned int     uColumns   = COLUMN_ALL, //< [in] Mask of columns to retrieve data from for each item.
		                                          //<      Ignored if pItems is NULL.
		vvNullable<bool> nbSelected = vvNULL,     //< [in] Only retrieve items whose selection state matches this value.
		                                          //<      Or vvNULL to not filter based on selection state.
		unsigned int     uItemTypes = ITEM_ALL,   //< [in] Only retrieve items whose type matches this mask.
		unsigned int     uStatuses  = STATUS_ALL  //< [in] Only retrieve items whose status matches this mask.
		) const;

	/**
	 * Sets the selection state of all items in the control.
	 * Returns the number of items whose selection state was set.
	 */
	unsigned int SetItemSelectionAll(
		bool bSelected //< [in] True to select all items, false to unselect all items.
		);

	/**
	 * Sets whether or not an item is selected.
	 * Returns the number of items whose selection state was set.
	 */
	unsigned int SetItemSelectionByPath(
		const wxString& sPath,    //< [in] Full repo path of the item to set the selection of.
		bool            bSelected //< [in] True to select the item, false to unselect it.
		);

	/**
	 * Sets whether or not some items are selected.
	 * Returns the number of items whose selection state was set.
	 */
	unsigned int SetItemSelectionByPath(
		const wxArrayString& cPaths,   //< [in] Full repo paths of the items to set the selection of.
		bool                 bSelected //< [in] True to select the items, false to unselect them.
		);

	/**
	 * Sets whether or not an item is selected.
	 * Returns the number of items whose selection state was set.
	 */
	unsigned int SetItemSelectionByGid(
		const wxString& sGid,     //< [in] GID of the item to set the selection of.
		bool            bSelected //< [in] True to select the item, false to unselect it.
		);

	/**
	 * Sets whether or not some items are selected.
	 * Returns the number of items whose selection state was set.
	 */
	unsigned int SetItemSelectionByGid(
		const wxArrayString& cGids,    //< [in] GIDs of the items to set the selection of.
		bool                 bSelected //< [in] True to select the items, false to unselect them.
		);

// event handlers
protected:
	void OnColumnHeaderClick(wxDataViewEvent& cEvent);
	void OnContext(wxDataViewEvent& event);
	void OnContextMenuItem_Diff(wxCommandEvent& event);
	void OnContextMenuItem_Resolve(wxCommandEvent& event);
	void OnContextMenuItem_Revert(wxCommandEvent& event);
	void OnContextMenuItem_Unlock(wxCommandEvent& event);
	void OnContextMenuItem_Add(wxCommandEvent& event);
	void OnContextMenuItem_Delete(wxCommandEvent& event);
	void OnContextMenuItem_DiskDelete(wxCommandEvent& event);
	void OnContextMenuItem_Refresh(wxCommandEvent& event);
	void OnItemDoubleClick(wxDataViewEvent& event);
	void OnItemSelected(wxDataViewEvent& event);
	void OnMouse(wxMouseEvent& event);

// private types
private:
	typedef std::map<Column, wxDataViewColumn*> stlColumnMap;

// private functionality
private:
	// disable copying
	vvStatusControl(const vvStatusControl& that);
	vvStatusControl& operator=(const vvStatusControl& that);

	/**
	 * Expands an item and all of its children.
	 */
	void ExpandItemRecursive(
		wxDataViewItem& cItem //< [in] The item to expand.
		);

	/**
	 * Get the list of expanded nodes.
	 */
	void GetExpandedPathsRecursive(
		wxDataViewItem&  cItem,
		wxArrayString& cOutputArray
		) const;
	/**
	 * Get data about a single item.
	 */
	vvNullable<vvStatusControl::ItemData> vvStatusControl::GetItemDataForSingleItem(
		wxDataViewItem&  cItem,                   //< [in] The item to get data about.
		unsigned int     uColumns   = COLUMN_ALL, //< [in] Mask of columns to retrieve data from for each item.
		                                          //<      Ignored if pItems is NULL.
		vvNullable<bool> nbSelected = vvNULL,     //< [in] Only retrieve items whose selection state matches this value.
		                                          //<      Or vvNULL to not filter based on selection state.
		unsigned int     uItemTypes = ITEM_ALL,   //< [in] Only retrieve items whose type matches this mask.
		unsigned int     uStatuses  = STATUS_ALL  //< [in] Only retrieve items whose status matches this mask.
		) const;

	/**
	 * Gets data about an item and all of its children.
	 */
	unsigned int GetItemDataRecursive(
		stlItemDataList* pItems,                  //< [out] List to add retrieved items to.
		                                          //<       May be NULL if you only care about the count.
		wxDataViewItem&  cItem,                   //< [in] The item to get data about.
		unsigned int     uColumns   = COLUMN_ALL, //< [in] Mask of columns to retrieve data from for each item.
		                                          //<      Ignored if pItems is NULL.
		vvNullable<bool> nbSelected = vvNULL,     //< [in] Only retrieve items whose selection state matches this value.
		                                          //<      Or vvNULL to not filter based on selection state.
		unsigned int     uItemTypes = ITEM_ALL,   //< [in] Only retrieve items whose type matches this mask.
		unsigned int     uStatuses  = STATUS_ALL  //< [in] Only retrieve items whose status matches this mask.
		) const;

	/**
	 * Sets the selection state of some items.
	 * Returns the number of items whose selection state was set.
	 */
	unsigned int SetItemSelectionRecursive(
		wxDataViewItem&      cItem,    //< [in] The current item to set selection state on.
		const wxArrayString* pItems,   //< [in] The set of items to modify.
		                               //<      If NULL, all items will be modified.
		bool                 bPaths,   //< [in] True if pItems contains paths, false if it contains GIDs.
		                               //<      Ignored if pItems is NULL.
		bool                 bSelected, //< [in] The selection state to set the items to.
		const wxString&	     sPathToFocus = wxEmptyString, //< [in] Optional path that will get the Focus Highlight.  Send wxEmptyString to ignore
		const wxArrayString* pExpandedNodes = NULL  //< [in] The set of nodes to expand
													//<      If NULL, no items will be expanded.
		);

	/**
	 * Sets which column is being sorted.
	 * Returns the column that was previously being sorted.
	 *
	 * This function is basically an optimization hack used by SetDataMode and FillData.
	 * Without it, clicking a column to change the sort order takes unreasonably long.
	 * This is because of an algorithmic flaw in wxDataViewCtrl.
	 * When it builds its data tree, it sorts the entire tree each time it adds a new node.
	 * This can cause column clicks to take several seconds with only ~100 items.
	 * This occurs around lines 289-303 of src/generic/datavgen.cpp.
	 * The hack is to unset the current sort while updating the control's data.
	 */
	int SetSortColumnIndex(
		int iIndex //< [in] The column index to set the sort to, or wxNOT_FOUND to unset the sort.
		);

	void LaunchDiffFromSelection();
// private data
private:
	vvVerbs*		m_pVerbs;
	vvContext*      m_pContext;
	const vvRepoLocator*	m_pRepoLocator;
	wxString		m_sRevision1;
	wxString		m_sRevision2;
	wxObjectDataPtr<class vvStatusModel> mpDataModel;     //< The data model we're displaying.
	Mode                                 meHierarchyMode; //< Mode to use for filename sorting.
	stlColumnMap                         mcColumns;       //< The set of columns we have created.

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvStatusControl);
	DECLARE_EVENT_TABLE();
};


#endif
