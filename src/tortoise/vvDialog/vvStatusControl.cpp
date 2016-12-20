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
#include "vvStatusControl.h"

#include "vvCppUtilities.h"
#include "vvDataViewToggleIconTextRenderer.h"
#include "vvNullable.h"
#include "vvVerbs.h"
#include "vvContext.h"
#include "vvRevertCommand.h"
#include "vvLockCommand.h"
#include "vvDiffCommand.h"
#include "vvDiffThread.h"
#include "vvProgressExecutor.h"
#include "vvAddCommand.h"
#include "vvRemoveCommand.h"
#include "vvResolveDialog.h"
#include "vvRevertSingleDialog.h"
#include "vvWxHelpers.h"

#define ID_CONTEXT_DIFF			wxID_HIGHEST + 72
#define ID_CONTEXT_REVERT		wxID_HIGHEST + 73
#define ID_CONTEXT_REFRESH		wxID_HIGHEST + 74
#define ID_CONTEXT_ADD			wxID_HIGHEST + 75
#define ID_CONTEXT_DELETE		wxID_HIGHEST + 76
#define ID_CONTEXT_DISK_DELETE	wxID_HIGHEST + 77
#define ID_CONTEXT_RESOLVE      wxID_HIGHEST + 78
#define ID_CONTEXT_UNLOCK       wxID_HIGHEST + 79

/*
**
** Types
**
*/

/**
 * Models data about the source control status of files in a working copy.
 */
class vvStatusModel : public wxDataViewModel
{
// types
public:
	/**
	 * Enumeration of columns used by the model.
	 *
	 * The B/E/I/S/T prefixes indicate the value type of the column (Bool, Enum, Icon, String, Tri-state).
	 * The prefix codes are always listed alphabetically, if it helps you remember the names.
	 *
	 * B, E, and S values use variant types "bool", "long", and "string" respectively.
	 * T values use variant type "bool" with the additional possibility of the variant being NULL (the third state).
	 * IS combinations use variant type "wxDataViewIconText".
	 * IST combinations use variant type "vvDataViewToggleIconText".
	 *
	 * The *_FILENAME columns are hierarchical/expander columns.
	 */
	enum Column
	{
		COLUMN_I_FILENAME,      //< Item's type icon, same as COLUMN_I_TYPE.
		COLUMN_IST_FILENAME,    //< Item's filename, type icon, and selection state.
		COLUMN_S_FILENAME,      //< Item's filename.
		COLUMN_IS_FILENAME,     //< Item's filename and type icon.
		COLUMN_I_PATH,          //< Item's type icon, same as COLUMN_I_TYPE.
		COLUMN_IST_PATH,        //< Item's repo path, type icon, and selection state.
		COLUMN_S_PATH,          //< Item's repo path.
		COLUMN_IS_PATH,         //< Item's repo path and type icon.
		COLUMN_E_STATUS,        //< Item's vvStatusControl::Status value.
		COLUMN_I_STATUS,        //< Item's status icon.
		COLUMN_S_STATUS,        //< Item's status name.
		COLUMN_IS_STATUS,       //< Item's status name and icon.
		COLUMN_S_FROM,          //< Item's old location/name.
		COLUMN_T_SELECTED,      //< Item's selection state.
		COLUMN_E_TYPE,          //< Item's vvStatusControl::ItemType value.
		COLUMN_I_TYPE,          //< Item's type icon.
		COLUMN_S_TYPE,          //< Item's type name.
		COLUMN_IS_TYPE,         //< Item's type name and icon.
		COLUMN_E_SUBTYPE,       //< Item's subtype within its type.  Meaning varies with item type:
		                        //< CONFLICT: The type of conflict (vvVerbs::ResolveChoiceState).
		                        //< Useful for others?  File types, etc?
		COLUMN_B_IGNORED,       //< Whether or not the item should be ignored by source control.
		//COLUMN_B_ATTRIBUTES,    //< Whether or not the item's attributes were modified.
		COLUMN_B_MULTIPLE,      //< Whether or not the item's status contains multiple actions.
		COLUMN_S_GID,           //< Item's GID.
		COLUMN_S_HID_NEW,       //< Item's current/new content's HID.
		COLUMN_S_HID_OLD,       //< Item's old content's HID.
		COLUMN_S_NOTES,         //< Combination of S_FROM and B_IGNORED.
		COLUMN_COUNT,           //< Number of entries in this enumeration, for iteration.
	};

// construction
public:
	/**
	 * Default constructor.
	 */
	vvStatusModel(
		vvStatusControl::Mode eMode      = vvStatusControl::MODE_FLAT, //< [in] Display mode to use.
		unsigned int          uItemTypes = vvStatusControl::ITEM_ALL,  //< [in] Mask of item types to display.
		unsigned int          uStatuses  = vvStatusControl::STATUS_ALL,//< [in] Mask of statuses to display.
		unsigned int		  uDisabledStatuses = 0					   //< [in] Mask of statuses that will render as disabled.
		);

// functionality
public:
	/**
	 * Sets what records are displayed by the model and how they're organized.
	 */
	void SetDisplay(
		vvStatusControl::Mode eMode,      //< [in] The mode to use to organize the data.
		vvFlags               cItemTypes, //< [in] The item types to display (bitmask of vvStatusControl::ItemType).
		vvFlags               cStatuses   //< [in] The statuses to display (bitmask of vvStatusControl::Status).
		);

	/**
	 * Gets the current mode of organizing the data.
	 */
	vvStatusControl::Mode GetDisplayMode() const;

	/**
	 * Gets the set of item types currently being displayed.
	 * The value is a mask of vvStatusControl::ItemType flags.
	 */
	vvFlags GetDisplayTypes() const;

	/**
	 * Gets the set of statuses currently being displayed by the model.
	 * The value is a mask of vvStatusControl::Status flags.
	 */
	vvFlags GetDisplayStatuses() const;

	/**
	 * Fills the model with status data retrieved from the given vvVerbs implementation.
	 */
	bool FillData(
		vvVerbs&        cVerbs,                   //< [in] The vvVerbs implementation to retrieve status data from.
		vvContext&      cContext,                 //< [in] The context to use with the verbs.
		const vvRepoLocator&	cRepoLocator,	  //< [in] The repo to use.
		const wxString& sRevision = wxEmptyString, //< [in] The revision to compare the folder to.
		                                          //<      If empty, then the folder's first parent will be used.
		const wxString&  sRevision2 = wxEmptyString
		);

// implementation
public:
	/**
	 * Checks if an item is a leaf node (not a container) or an internal node (container).
	 */
	virtual bool IsContainer(
		const wxDataViewItem& cItem //< [in] The item to check.
		) const;

	/**
	 * Checks if a container item should display values in non-expander columns.
	 */
	virtual bool HasContainerColumns(
		const wxDataViewItem& cItem //< [in] The item to check.
		) const;

	/**
	 * Check if the item has one of the statuses that was passed in to disable.
	 */
	bool vvStatusModel::IsPermanentlyDisabled(
		const wxDataViewItem& cItem
		) const;

	/**
	 * Gets the parent of an item.
	 */
	virtual wxDataViewItem GetParent(
		const wxDataViewItem& cItem //< [in] The item to get the parent of.
		) const;

	/**
	 * Gets the children of a container item.
	 * Returns the number of children.
	 */
	virtual unsigned int GetChildren(
		const wxDataViewItem& cItem,    //< [in] The item to get the children of.
		wxDataViewItemArray&  cChildren //< [out] Array to add the children to.
		) const;

	/**
	 * Gets the number of columns used by the model.
	 */
	virtual unsigned int GetColumnCount() const;

	/**
	 * Gets the type of a column.
	 */
	virtual wxString GetColumnType(
		unsigned int uColumn //< [in] The column to get the type of.
		) const;

	/**
	 * Gets the value of an item in a column.
	 */
	virtual void GetValue(
		wxVariant&            cValue, //< [out] The requested value.
		const wxDataViewItem& cItem,  //< [in] The item to get a value from.
		unsigned int          uColumn //< [in] The column to get the value for.
		) const;

	/**
	 * Sets the value of an item in a column.
	 */
	virtual bool SetValue(
		const wxVariant&      cValue, //< [in] The value to set.
		const wxDataViewItem& cItem,  //< [in] The item to set the value on.
		unsigned int          uColumn //< [in] The column to set the value in.
		);

	/**
	 * Compares two items for sorting purposes.
	 */
	virtual int Compare(
		const wxDataViewItem& cLeft,     //< [in] Item to compare on the left.
		const wxDataViewItem& cRight,    //< [in] Item to compare on the right.
		unsigned int          uColumn,   //< [in] Column to compare item values in.
		bool                  bAscending //< [in] Whether or not the sort is in ascending order.
		) const;

// private types
private:
	/**
	 * A single mapping from a set of change flags to a status.
	 */
	struct ChangesToStatusMapping
	{
		unsigned int            uChanges; //< The change flags to map to a status.
		vvStatusControl::Status eStatus;  //< The status that the change flags map to.
	};

	/**
	 * Data about a single change to a file with multiple changes.
	 */
	struct FileChange
	{
		vvFlags  cChanges; //< The subset of pDiffEntry's cItemChanges that this change represents.
		wxString sFrom;    //< The old filename/path for RENAMED/MOVED changes.
	};

	/**
	 * A list of FileChanges.
	 */
	typedef std::list<FileChange> stlFileChangeList;

	/**
	 * Data about a single conflict on a file.
	 */
	struct FileConflict
	{
		vvVerbs::ResolveChoice* pChoice;
		wxString                sFrom;
	};

	/**
	 * A list of FileConflicts.
	 */
	typedef std::list<FileConflict> stlFileConflictList;

	/**
	 * A single data item being represented by the model.
	 */
	struct DataItem
	{
		vvVerbs::DiffEntry*     pDiffEntry;     //< The DiffEntry data for the item.
		vvVerbs::ResolveItem*   pConflictEntry; //< The ConflictEntry data for the item.
		vvVerbs::LockList		cLocks;			//< The LockEntry data for the item.
		vvStatusControl::Status eStatus;        //< The item's status.
		stlFileChangeList       cFileChanges;   //< A list of individual changes in this item's diff entry.
		stlFileConflictList     cFileConflicts; //< A list of conflicts on the file.
		bool                    bSelected;      //< Whether or not the item is currently selected/checked.

		// These two members are only used in the rare case that an item is both
		// removed/deleted AND something else (it could be found/added/lost if a new
		// item was created in its place, or it could be renamed/moved if an existing
		// item was renamed/moved into its place).  In that case this single DataItem
		// represents both the removed/deleted item and the other item,
		// each of which has their own GID and other details.  The data model does
		// its best to present them as a single item.
		vvVerbs::DiffEntry*     pOldDiffEntry;     //< DiffEntry of the removed/deleted item.
		vvVerbs::ResolveItem*   pOldConflictEntry; //< ConflictEntry of the removed/deleted item.
	};

	/**
	 * A list of DataItems.
	 */
	typedef std::list<DataItem> stlDataItemList;

	/**
	 * A node in a tree that stores the hierarchical structure of the data.
	 */
	struct TreeNode
	{
		/**
		 * A list of tree nodes.
		 */
		typedef std::list<TreeNode*> stlChildList;

		// data
		TreeNode*     pParent;   //< The parent of this node.
		stlChildList  cChildren; //< The children of this node.
		wxString      sName;     //< The node's name, which is displayed as its filename.
		DataItem*     pItem;     //< The data item for this node, if it has one.
		FileChange*   pChange;   //< The individual file change associated with this node, if any.
		                         //< Will point to a change in pItem->cFileChanges.
		FileConflict* pConflict; //< The individual file conflict associated with this node, if any.
		                         //< Will point to a conflict in pItem->cFileConflicts.
	};

	/**
	 * A list of TreeNodes.
	 */
	typedef std::list<TreeNode> stlTreeNodeList;

// private constants
private:
	/**
	 * Mappings from vvVerbs::DiffEntry::ItemType to vvStatusControl::ItemType.
	 * Use the vvVerbs::DiffEntry::ItemType as the index.
	 */
	static const vvStatusControl::ItemType saItemTypeMappings[];

	/**
	 * Mappings from change flags to a specific status.
	 */
	static const ChangesToStatusMapping saChangesToStatusMappings[];

// private static functionality
private:
	/**
	 * Gets the appropriate vvStatusControl::ItemType for a vvVerbs::DiffEntry::ItemType.
	 * If no appropriate type can be found, ITEM_LAST is returned.
	 */
	static vvStatusControl::ItemType MapItemType(
		vvVerbs::DiffEntry::ItemType eType //< [in] The vvVerbs::DiffEntry::ItemType to map to a vvStatusControl::ItemType.
		);

	/**
	 * Gets data about a given item type.
	 * Never returns NULL.  If a record can't be found, the record for ITEM_LAST is returned.
	 */
	static const vvStatusControl::ItemTypeData* GetItemTypeData(
		vvVerbs::DiffEntry::ItemType eType //< [in] The item type to retrieve data for.
		);

	/**
	 * Gets the appropriate Status for a vvVerbs::DiffEntry::ChangeFlags.
	 * If no appropriate status can be found, STATUS_LAST is returned.
	 */
	static vvStatusControl::Status MapStatus(
		vvFlags cChanges //< [in] The change flags to map to a status.
		);

	/**
	 * Gets the appropriate Status for a DataItem.
	 * If no appropriate status can be found, STATUS_LAST is returned.
	 */
	static vvStatusControl::Status MapStatus(
		const DataItem& cItem //< [in] The item to map to a status.
		);

	/**
	 * Combination of MapStatus and GetStatusData.
	 * Never returns NULL.  If no appropriate status is found, the data for STATUS_LAST is returned.
	 */
	static const vvStatusControl::StatusData* GetStatusData(
		vvFlags cChanges //< [in] The change flags to retrieve status data for.
		);

	/**
	 * Combination of MapStatus and GetStatusData.
	 * Never returns NULL.  If no appropriate status is found, the data for STATUS_LAST is returned.
	 */
	static const vvStatusControl::StatusData* GetStatusData(
		const DataItem& cItem //< [in] The item to retrieve status data for.
		);

// private functionality
private:
	// disable copying
	vvStatusModel(const vvStatusModel& that);
	vvStatusModel& operator=(const vvStatusModel& that);

	/**
	 * Clears all display data built from the current items.
	 */
	void ClearDisplayData();

	/**
	 * Ensures that the data necessary to display the given configuration has been built.
	 */
	void EnsureDisplayData(
		vvStatusControl::Mode eMode,      //< [in] The mode to ensure that data exists for.
		vvFlags               cItemTypes, //< [in] The item types to ensure that data exists for.
		vvFlags               cStatuses   //< [in] The statuses to ensure that data exists for.
		);

	/**
	 * Finds any conflict for an item with a given GID from among the given conflicts.
	 */
	vvVerbs::ResolveItem* FindConflictByGid(
		const wxString&              sGid,      //< [in] The GID to find conflicts for.
		vvVerbs::stlResolveItemList& cConflicts //< [in] The conflicts to search in.
		);

	/**
	 * Builds a set of data items from the given diff data.
	 */
	void BuildDataItems(
		vvVerbs::DiffData&           cDiff,      //< [in] The diff data to build items from.
		vvVerbs::stlResolveItemList& cConflicts, //< [in] The conflict data to build items from.
		vvFlags                      cItemTypes, //< [in] The set of types to build items for.
		vvFlags                      cStatuses,  //< [in] The set of statuses to build items for.
		stlDataItemList&             cItems      //< [out] The built items.
		);

	/**
	 * Builds a flat tree from the given diff data.
	 * Returns the root node of the created tree.
	 */
	TreeNode* BuildFlatTree(
		stlDataItemList& cItems, //< [in] The items to build a flat tree from.
		stlTreeNodeList& cNodes  //< [in] The list to add created tree nodes to.
		);

	/**
	 * Builds a folder tree from the given diff data.
	 * Returns the root node of the created tree.
	 */
	TreeNode* BuildFolderTree(
		stlDataItemList& cItems, //< [in] The items to build a folder tree from.
		stlTreeNodeList& cNodes  //< [in] The list to add created tree nodes to.
		);

	/**
	 * Builds a collapsed tree from the given folder tree.
	 * Returns the root node of the created tree.
	 */
	TreeNode* BuildCollapsedTree(
		TreeNode*        pFolderTree, //< [in] The folder tree to build a collapsed tree from.
		stlTreeNodeList& cNodes       //< [in] The list to add created tree nodes to.
		);

	/**
	 * Adds a new tree node.
	 */
	TreeNode* CreateTreeNode(
		stlTreeNodeList& cNodes,        //< [in] The list to store the node in.
		TreeNode*        pParent = NULL //< [in] The parent to add the new node to, if any.
		);

	/**
	 * Creates nodes under a given node for each of its individual file changes.
	 */
	bool CreateChangeNodes(
		stlTreeNodeList& cNodes, //< [in] The list to store the new nodes in.
		TreeNode*        pParent //< [in] The parent to create change nodes under.
		);

	/**
	 * Creates nodes under a given node for each of its individual conflicts.
	 */
	bool CreateConflictNodes(
		stlTreeNodeList& cNodes, //< [in] The list to store the new nodes in.
		TreeNode*        pParent //< [in] The parent to create conflict nodes under.
		);

	/**
	 * Finds a TreeNode's child by name.
	 * Returns NULL if the child wasn't found.
	 */
	TreeNode* FindTreeNodeChild(
		TreeNode*       pParent, //< [in] The node to look for a child of.
		const wxString& sName    //< [in] The name of the child to look for.
		);

	/**
	 * Finds a TreeNode's descendent node using a range of child names.
	 * Optionally creates the nodes that are missing, instead of returning NULL.
	 */
	template <typename Iterator>
	TreeNode* FindTreeNodeDecendent(
		TreeNode*            pParent, //< [in] The node to look for a descendent of.
		Iterator             itBegin, //< [in] The beginning of a range of child names (wxString).
		Iterator             itEnd,   //< [in] The end of a range of child names (wxString).
		stlTreeNodeList*     pNodes   //< [in] If non-NULL, then create missing nodes by adding them to this list.
		                              //<      If NULL, then a missing node will result in a NULL return value.
		);

	/**
	 * Gets the selection state of a node's children.
	 * If all the children are selected, true is returned.
	 * If none of the children are selected, false is returned.
	 * If some of the children are selected and others aren't, null is returned.
	 */
	vvNullable<bool> GetNodeChildrenSelected(
		TreeNode* pNode //< [in] The node whose children to get the selection state of.
		                //<      The node must have a non-zero number of children.
		) const;

	/**
	 * Gets the item type of a node, along with associated data.
	 */
	const vvStatusControl::ItemTypeData* GetNodeItemTypeData(
		const TreeNode* pNode //< [in] The node to get the item type of.
		) const;

	/**
	 * Gets the status of a node, along with associated data.
	 */
	const vvStatusControl::StatusData* GetNodeStatusData(
		const TreeNode* pNode //< [in] The node to get the status of.
		) const;

	/**
	 * Gets a node's bool value for a column.
	 * Returns true if the value was filled in, or false if it wasn't found/available.
	 */
	bool GetNodeValue_Bool(
		TreeNode*         pNode,   //< [in] The node to get a value from.
		Column            eColumn, //< [in] The column to get the value for.
		vvNullable<bool>* pValue   //< [out] If true is returned, the requested value.
		                           //<       If false is returned, the value is not modified.
		) const;

	/**
	 * Gets a node's int value for a column.
	 * Returns true if the value was filled in, or false if it wasn't found/available.
	 */
	bool GetNodeValue_Int(
		TreeNode* pNode,   //< [in] The node to get a value from.
		Column    eColumn, //< [in] The column to get the value for.
		int*      pValue   //< [out] If true is returned, the requested value.
		                   //<       If false is returned, the value is not modified.
		) const;

	/**
	 * Gets a node's string value for a column.
	 * Returns true if the value was filled in, or false if it wasn't found/available.
	 */
	bool GetNodeValue_String(
		TreeNode* pNode,   //< [in] The node to get a value from.
		Column    eColumn, //< [in] The column to get the value for.
		wxString* pValue   //< [out] If true is returned, the requested value.
		                   //<       If false is returned, the value is not modified.
		) const;

	/**
	 * Gets a node's icon value for a column.
	 * Returns true if the value was filled in, or false if it wasn't found/available.
	 */
	bool GetNodeValue_Icon(
		TreeNode* pNode,   //< [in] The node to get a value from.
		Column    eColumn, //< [in] The column to get the value for.
		wxIcon*   pValue   //< [out] If true is returned, the requested value.
		                   //<       If false is returned, the value is not modified.
		) const;

	/**
	 * Sets a node's bool value for a column.
	 * Returns true if the value was set, or false if it couldn't be.
	 */
	bool SetNodeValue_Bool(
		TreeNode*        pNode,   //< [in] The node to set a value in.
		Column           eColumn, //< [in] The column to set the value for.
		vvNullable<bool> nbValue  //< [in] The value to set.
		);

	/**
	 * Sets a node's int value for a column.
	 * Returns true if the value was set, or false if it couldn't be.
	 */
	bool SetNodeValue_Int(
		TreeNode*        pNode,   //< [in] The node to set a value in.
		Column           eColumn, //< [in] The column to set the value for.
		int              iValue  //< [in] The value to set.
		);

	/**
	 * Sets a node's string value for a column.
	 * Returns true if the value was set, or false if it couldn't be.
	 */
	bool SetNodeValue_String(
		TreeNode*       pNode,   //< [in] The node to set a value in.
		Column          eColumn, //< [in] The column to set the value for.
		const wxString& sValue   //< [in] The value to set.
		);

	/**
	 * Sets a node's icon value for a column.
	 * Returns true if the value was set, or false if it couldn't be.
	 */
	bool SetNodeValue_Icon(
		TreeNode* pNode,   //< [in] The node to set a value in.
		Column    eColumn, //< [in] The column to set the value for.
		wxIcon    cValue   //< [in] The value to set.
		);

// private data
private:
	// basic data
	vvStatusControl::Mode       meMode;             //< The mode we're currently displaying the data in.
	vvFlags                     mcItemTypes;        //< The set of item types currently being displayed.
	vvFlags                     mcStatuses;         //< The set of statuses currently being displayed
	vvFlags                     mcDisabledStatuses; //< The set of statuses that will be rendered disabled.
	vvVerbs::DiffData           mcDiffData;         //< The diff data we're displaying.
	vvVerbs::stlResolveItemList mcConflictData;     //< The conflict data we're displaying.
	stlDataItemList             mcDataItems;        //< The data items that we're displaying (points to mcDiffData and mcConflictData).
	stlTreeNodeList             mcTreeNodes;        //< Nodes being pointed to by our tree(s) (points to mcDataItems).
	TreeNode*                   mpFlatTree;         //< Tree used by MODE_FLAT (points to mcTreeNodes).
	TreeNode*                   mpFolderTree;       //< Tree used by MODE_FOLDER_TREE (points to mcTreeNodes).
	TreeNode*                   mpCollapsedTree;    //< Tree used by MODE_COLLAPSED_TREE (points to mcTreeNodes).
	wxString					msCurrentUser;		//< The current user.  This is used to determine which locks are violated.
};

/**
 * Internal data about columns in the control.
 */
struct _InternalColumnData
{
	vvStatusControl::Column eColumn;           //< The column value this data is about.
	unsigned int            iStyle;            //< The Style flags that this data applies to.
	vvStatusModel::Column   eModelColumn;      //< The corresponding Column value from vvStatusModel.
	wxDataViewCellMode      eCellMode;         //< The wxDATAVIEW_CELL_* mode for cells in this column.
	wxAlignment             eAlignment_Header; //< The alignment for the column header.
	int                     iAlignment_Column; //< The alignment for the column cells.
	int                     iWidth_Start;      //< The initial width of the column.
	int                     iWidth_Minimum;    //< The minimum width of the column.
	int                     iFlags;            //< wxDATAVIEW_COL_* flags associated with the column.
};


/*
**
** Globals
**
*/

static const wxString gsCollapsedPathSeparator = "/";

const vvStatusControl::ItemType vvStatusModel::saItemTypeMappings[] =
{
	vvStatusControl::ITEM_FILE,      // The index of this value must be vvVerbs::DiffEntry::ITEM_FILE
	vvStatusControl::ITEM_FOLDER,    // The index of this value must be vvVerbs::DiffEntry::ITEM_FOLDER
	vvStatusControl::ITEM_SYMLINK,   // The index of this value must be vvVerbs::DiffEntry::ITEM_SYMLINK
	vvStatusControl::ITEM_SUBMODULE, // The index of this value must be vvVerbs::DiffEntry::ITEM_SUBMODULE
	vvStatusControl::ITEM_LAST,      // The index of this value must be vvVerbs::DiffEntry::ITEM_COUNT
};

const vvStatusModel::ChangesToStatusMapping vvStatusModel::saChangesToStatusMappings[] =
{
	{ vvVerbs::DiffEntry::CHANGE_ADDED,      vvStatusControl::STATUS_ADDED     },
	{ vvVerbs::DiffEntry::CHANGE_DELETED,    vvStatusControl::STATUS_DELETED   },
	{ vvVerbs::DiffEntry::CHANGE_MODIFIED |
	  vvVerbs::DiffEntry::CHANGE_ATTRS,      vvStatusControl::STATUS_MODIFIED  },
	{ vvVerbs::DiffEntry::CHANGE_LOST,       vvStatusControl::STATUS_LOST      },
	{ vvVerbs::DiffEntry::CHANGE_FOUND,      vvStatusControl::STATUS_FOUND     },
	{ vvVerbs::DiffEntry::CHANGE_RENAMED,    vvStatusControl::STATUS_RENAMED   },
	{ vvVerbs::DiffEntry::CHANGE_MOVED,      vvStatusControl::STATUS_MOVED     },
	{ vvVerbs::DiffEntry::CHANGE_LOCKED,      vvStatusControl::STATUS_LOCKED     },
	{ vvVerbs::DiffEntry::CHANGE_LOCK_CONFLICT,      vvStatusControl::STATUS_LOCK_CONFLICT     },
	{ vvVerbs::DiffEntry::CHANGE_NONE,       vvStatusControl::STATUS_UNCHANGED },
	{ 0u,                                    vvStatusControl::STATUS_LAST      },
};

static const vvStatusControl::ItemTypeData gaItemTypeDatas[] =
{
	{ vvStatusControl::ITEM_FILE,      "File",         "ItemType_File_16"      },
	{ vvStatusControl::ITEM_FOLDER,    "Folder",       "ItemType_Folder_16"    },
	{ vvStatusControl::ITEM_SYMLINK,   "Symlink",      "ItemType_Symlink_16"   },
	{ vvStatusControl::ITEM_SUBMODULE, "Submodule",    "ItemType_Submodule_16" },
	{ vvStatusControl::ITEM_CONTAINER, "Container",    "ItemType_Folder_16"    },
	{ vvStatusControl::ITEM_CHANGE,    "",             NULL                    },
	{ vvStatusControl::ITEM_CONFLICT,  "",             NULL                    },
	{ vvStatusControl::ITEM_LOCK,  "",             NULL                    },
	{ vvStatusControl::ITEM_LAST,      "Unknown Type", NULL                    },
};

static const vvStatusControl::StatusData gaStatusDatas[] =
{
	{ vvStatusControl::STATUS_ADDED,     "Added",          "Status_Added_16"    },
	{ vvStatusControl::STATUS_DELETED,   "Deleted",        "Status_Deleted_16"  },
	{ vvStatusControl::STATUS_MODIFIED,  "Modified",       "Status_Modified_16" },
	{ vvStatusControl::STATUS_LOST,      "Lost",           "Status_Lost_16"     },
	{ vvStatusControl::STATUS_FOUND,     "Found",          "Status_Found_16"    },
	{ vvStatusControl::STATUS_RENAMED,   "Renamed",        "Status_Renamed_16"  },
	{ vvStatusControl::STATUS_MOVED,     "Moved",          "Status_Moved_16"    },
	{ vvStatusControl::STATUS_CONFLICT,  "Conflict",       "Status_Conflict_16" },
	{ vvStatusControl::STATUS_RESOLVED,  "Resolved",       "Status_Resolved_16" },
	{ vvStatusControl::STATUS_MULTIPLE,  "Multiple",       "Status_Multiple_16" },
	{ vvStatusControl::STATUS_IGNORED,   "Ignored",        "Status_Ignored_16"  },
	{ vvStatusControl::STATUS_UNCHANGED, "Unchanged",      "Status_Normal_16"   },
	{ vvStatusControl::STATUS_LOCKED,	 "Locked",		   "Status_Locked_16"   },
	{ vvStatusControl::STATUS_LOCK_CONFLICT,	 "Lock Violation",		   "Status_Conflict_16"   },
	{ vvStatusControl::STATUS_LAST,      "Unknown Status", NULL                 },
};

static const vvStatusControl::ColumnData gaColumnDatas[] =
{
	{ vvStatusControl::COLUMN_FILENAME,      "Filename"       },
	{ vvStatusControl::COLUMN_STATUS,        "Status"         },
	{ vvStatusControl::COLUMN_FROM,          "From"           },
//	{ vvStatusControl::COLUMN_ATTRIBUTES,    "Attrs"          },
	{ vvStatusControl::COLUMN_GID,           "GID"            },
	{ vvStatusControl::COLUMN_HID_NEW,       "HID (New)"      },
	{ vvStatusControl::COLUMN_HID_OLD,       "HID (Old)"      },
	{ vvStatusControl::COLUMN_LAST,          "Unknown Column" },
};

static const _InternalColumnData gaInternalColumnDatas[] =
{
	{ vvStatusControl::COLUMN_FILENAME,      vvStatusControl::STYLE_SELECT, vvStatusModel::COLUMN_IST_FILENAME,     wxDATAVIEW_CELL_INERT, wxALIGN_LEFT,   wxALIGN_LEFT,   250, 50, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
	{ vvStatusControl::COLUMN_FILENAME,      0u,                            vvStatusModel::COLUMN_IS_FILENAME,      wxDATAVIEW_CELL_INERT,       wxALIGN_LEFT,   wxALIGN_LEFT,   250, 50, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
	{ vvStatusControl::COLUMN_STATUS,        0u,                            vvStatusModel::COLUMN_IS_STATUS,        wxDATAVIEW_CELL_INERT,       wxALIGN_LEFT,   wxALIGN_LEFT,   100, 25, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
	{ vvStatusControl::COLUMN_FROM,          0u,                            vvStatusModel::COLUMN_S_FROM,           wxDATAVIEW_CELL_INERT,       wxALIGN_LEFT,   wxALIGN_LEFT,   175, 25, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
	//{ vvStatusControl::COLUMN_ATTRIBUTES,    0u,                            vvStatusModel::COLUMN_B_ATTRIBUTES,     wxDATAVIEW_CELL_INERT,       wxALIGN_CENTER, wxALIGN_CENTER,  60, 25, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE                            },
	{ vvStatusControl::COLUMN_GID,           0u,                            vvStatusModel::COLUMN_S_GID,            wxDATAVIEW_CELL_INERT,       wxALIGN_LEFT,   wxALIGN_LEFT,   410, 25, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
	{ vvStatusControl::COLUMN_HID_NEW,       0u,                            vvStatusModel::COLUMN_S_HID_NEW,        wxDATAVIEW_CELL_INERT,       wxALIGN_LEFT,   wxALIGN_LEFT,   260, 25, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
	{ vvStatusControl::COLUMN_HID_OLD,       0u,                            vvStatusModel::COLUMN_S_HID_OLD,        wxDATAVIEW_CELL_INERT,       wxALIGN_LEFT,   wxALIGN_LEFT,   260, 25, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
	{ vvStatusControl::COLUMN_LAST,          0u,                            vvStatusModel::COLUMN_COUNT,            wxDATAVIEW_CELL_INERT,       wxALIGN_LEFT,   wxALIGN_LEFT,   666, 11, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE },
};


/*
**
** Internal Functions
**
*/

static const _InternalColumnData* _GetInternalColumnData(
	vvStatusControl::Column eColumn,
	unsigned int            uStyle
	)
{
	vvFlags cStyle = uStyle;
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaInternalColumnDatas); ++uIndex)
	{
		if (gaInternalColumnDatas[uIndex].eColumn == eColumn && cStyle.HasAllFlags(gaInternalColumnDatas[uIndex].iStyle))
		{
			return gaInternalColumnDatas + uIndex;
		}
	}

	wxASSERT(eColumn != vvStatusControl::COLUMN_LAST);
	return _GetInternalColumnData(vvStatusControl::COLUMN_LAST, 0u);
}

static void _VerifyItem(
	wxDataViewModel* pModel,
	unsigned int     uNameColumn,
	wxDataViewItem&  cItem
	)
{
	wxDataViewItemArray cChildren;
	pModel->GetChildren(cItem, cChildren);

	for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
	{
		wxDataViewItem cChild = *it;

		wxDataViewItem cParent = pModel->GetParent(cChild);
		if (cParent.GetID() != cItem.GetID())
		{
			wxVariant cItemName   = "NULL";
			wxVariant cParentName = "NULL";
			wxVariant cChildName  = "NULL";

			if (cItem.IsOk())
			{
				pModel->GetValue(cItemName, cItem, uNameColumn);
			}
			if (cParent.IsOk())
			{
				pModel->GetValue(cParentName, cParent, uNameColumn);
			}
			if (cChild.IsOk())
			{
				pModel->GetValue(cChildName, cChild, uNameColumn);
			}

			wxLogError("'%s' lists '%s' as child, but '%s' lists '%s' as parent.\n", cItemName.GetString(), cChildName.GetString(), cChildName.GetString(), cParentName.GetString());
		}

		_VerifyItem(pModel, uNameColumn, cChild);
	}
}

static void _VerifyModel(
	wxDataViewModel* pModel,
	unsigned int     uNameColumn
	)
{
	wxDataViewItem cRoot;
	_VerifyItem(pModel, uNameColumn, cRoot);
}

vvStatusModel::vvStatusModel(
	vvStatusControl::Mode eMode,
	unsigned int          uItemTypes,
	unsigned int          uStatuses,
	unsigned int		  uDisabledStatuses
	)
	: meMode(eMode)
	, mcItemTypes(uItemTypes)
	, mcStatuses(uStatuses)
	, mcDisabledStatuses(uDisabledStatuses)
	, mpFlatTree(NULL)
	, mpFolderTree(NULL)
	, mpCollapsedTree(NULL)
{
	this->EnsureDisplayData(this->meMode, this->mcItemTypes, this->mcStatuses);
}

void vvStatusModel::SetDisplay(
	vvStatusControl::Mode eMode,
	vvFlags               cItemTypes,
	vvFlags               cStatuses
	)
{
	// if we're already using the given settings, bail now
	if (eMode == this->meMode && cItemTypes == this->mcItemTypes && cStatuses == this->mcStatuses)
	{
		return;
	}

	// if the type or status filters changed, we need to clear and re-build our display data
	// if only the mode changed, we can just build a tree from the existing data if necessary
	if (cItemTypes != this->mcItemTypes || cStatuses != this->mcStatuses)
	{
		this->ClearDisplayData();
	}

	// make sure we have the data available for these settings
	this->EnsureDisplayData(eMode, cItemTypes, cStatuses);

	// store the new settings
	this->meMode      = eMode;
	this->mcItemTypes = cItemTypes;
	this->mcStatuses  = cStatuses;

	// verify that our parent/child relationships are exposed correctly
	_VerifyModel(this, COLUMN_S_FILENAME);

	// let everybody know that our data has been reset
	this->Cleared();
}

vvStatusControl::Mode vvStatusModel::GetDisplayMode() const
{
	return this->meMode;
}

vvFlags vvStatusModel::GetDisplayTypes() const
{
	return this->mcItemTypes;
}

vvFlags vvStatusModel::GetDisplayStatuses() const
{
	return this->mcStatuses;
}

bool vvStatusModel::FillData(
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const vvRepoLocator& cRepoLocator,
	const wxString& sRevision,
	const wxString&  sRevision2
	)
{
	// get new diff data
	vvVerbs::DiffData cDiffData;
	if (!cVerbs.Status(cContext, cRepoLocator, sRevision, sRevision2, cDiffData))
	{
		cContext.Log_ReportError_CurrentError();
		cContext.Error_Reset();
		return false;
	}

	// get new conflict data for working copy vs. baseline status results
	vvVerbs::stlResolveItemList cConflictData;
	if (sRevision2.IsEmpty() && cRepoLocator.IsWorkingFolder())
	{
		bool bConflicts = false;

		// check if the status is against the working copy's baseline
		if (sRevision.IsEmpty())
		{
			// no revisions were specified
			// definitely working copy vs. baseline
			// look up conflicts
			bConflicts = true;
		}
		else
		{
			wxArrayString cParents;

			// one revision specified
			// check if it's the working folder's baseline
			if (!cVerbs.FindWorkingFolder(cContext, cRepoLocator.GetWorkingFolder(), NULL, NULL, &cParents))
			{
				return false;
			}
			if (cParents.Count() > 0u && sRevision.CmpNoCase(cParents[0]) == 0)
			{
				// the working copy's baseline was specified
				// look up conflicts
				bConflicts = true;
			}
			else
			{
				// some other revision was specified
				// so it's working copy vs. arbitrary revision
				// we don't want conflicts
				bConflicts = false;
			}
		}

		// if we need the conflict data, look it up
		if (bConflicts && !cVerbs.Resolve_GetData(cContext, cRepoLocator.GetWorkingFolder(), cConflictData))
		{
			return false;
		}
	}

	// filter out data that we don't want
	for (vvVerbs::DiffData::iterator it = cDiffData.begin(); it != cDiffData.end(); )
	{
		//The new WC code can send back Ignored items as changed
		//wxASSERT(!it->cItemChanges.FlagsEmpty());

		// remove the MODIFIED flag from folders, we never want to show a folder as modified
		if (it->eItemType == vvVerbs::DiffEntry::ITEM_FOLDER)
		{
			it->cItemChanges.RemoveFlags(vvVerbs::DiffEntry::CHANGE_MODIFIED);
		}

		// if the item has no more flags, remove it from the list entirely
		if (it->cItemChanges.FlagsEmpty() && !it->bItemIgnored)
		{
			it = cDiffData.erase(it);
		}
		else
		{
			++it;
		}
	}

	//Get the username of the current user.
	if (!cVerbs.GetCurrentUser(cContext, cRepoLocator.GetRepoName(), msCurrentUser))
	{
		if (cContext.Error_Equals(SG_ERR_NO_CURRENT_WHOAMI))
			cContext.Error_Reset();
	}

	// store the new data
	this->mcDiffData.clear();
	this->mcDiffData.swap(cDiffData);
	this->mcConflictData.clear();
	this->mcConflictData.swap(cConflictData);

	// clear and rebuild the data for our current configuration
	this->ClearDisplayData();
	this->EnsureDisplayData(this->GetDisplayMode(), this->GetDisplayTypes(), this->GetDisplayStatuses());

	// verify that our parent/child relationships are exposed correctly
	_VerifyModel(this, COLUMN_S_FILENAME);

	// let everybody know that our data has been reset
	this->Cleared();

	// done
	return true;
}

bool vvStatusModel::IsContainer(
	const wxDataViewItem& cItem
	) const
{
	if (!cItem.IsOk())
	{
		return true;
	}

	TreeNode* pNode = static_cast<TreeNode*>(cItem.GetID());
	wxASSERT(pNode != NULL);
	return (pNode->cChildren.size() > 0u);
}

bool vvStatusModel::HasContainerColumns(
	const wxDataViewItem& cItem
	) const
{
	wxASSERT(cItem.IsOk());
	TreeNode* pNode = static_cast<TreeNode*>(cItem.GetID());
	wxASSERT(pNode != NULL);
	return pNode->pItem != NULL;
}


bool vvStatusModel::IsPermanentlyDisabled(
	const wxDataViewItem& cItem
	) const
{
	if (!cItem.IsOk() || mcDisabledStatuses.FlagsEmpty())
	{
		return false;
	}

	wxVariant statusVariant;
	GetValue(statusVariant, cItem, COLUMN_E_STATUS);
	vvStatusControl::Status status = (vvStatusControl::Status)statusVariant.GetLong();
	bool bIsExplicitlyDisabled = mcDisabledStatuses.HasAnyFlag(status);
	if (bIsExplicitlyDisabled)
		return true;
	wxVariant typeVariant;
	GetValue(typeVariant, cItem, COLUMN_E_TYPE);

	vvStatusControl::ItemType type = static_cast<vvStatusControl::ItemType>(typeVariant.GetInteger());
	if (type == vvStatusControl::ITEM_CONTAINER) // It's a folder which doesn't have any status itself.
	{
		wxDataViewItemArray cChildren;
		this->GetChildren(cItem, cChildren);
		for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
		{
			if (!IsPermanentlyDisabled(*it))
			{
				return false;
			}
		}
		//All of its children are permanently disabled.
		return true;
	}
	
	return false;
}

wxDataViewItem vvStatusModel::GetParent(
	const wxDataViewItem& cItem
	) const
{
	// the invisible root has no parent
	if (!cItem.IsOk())
	{
		return wxDataViewItem(NULL);
	}

	// get the node for this item
	TreeNode* pNode = static_cast<TreeNode*>(cItem.GetID());
	wxASSERT(pNode != NULL);

	// if this node's parent is the flat tree root
	// then pretend its parent is the invisible root
	// because we don't actually want the flat tree root displayed
	if (pNode->pParent == this->mpFlatTree)
	{
		return wxDataViewItem(NULL);
	}

	// return the node's parent
	return wxDataViewItem(static_cast<void*>(pNode->pParent));
}

unsigned int vvStatusModel::GetChildren(
	const wxDataViewItem& cItem,
	wxDataViewItemArray&  cChildren
	) const
{
	TreeNode* pNode = static_cast<TreeNode*>(cItem.GetID());

	// this will happen for the root
	// it's another way to check !cItem.IsOk()
	if (pNode == NULL)
	{
		switch (this->GetDisplayMode())
		{
		// in the case of the flat tree, we want the flat tree's children to show up at the top level
		// so add all the flat tree's children to the invisible root, rather than adding the flat tree itself
		case vvStatusControl::MODE_FLAT:
			wxASSERT(this->mpFlatTree != NULL);
			pNode = this->mpFlatTree;
			break;

		// for the folder and collapsed trees, we want a single visible root node
		// so the only child of the invisible root will be the tree root itself
		// however, if the tree root has no children, the control is basically empty
		// so in that case it's better to return no children and leave the control blank
		case vvStatusControl::MODE_FOLDER:
			wxASSERT(this->mpFolderTree != NULL);
			pNode = this->mpFolderTree;
			// fall through, because most of the code is common
		case vvStatusControl::MODE_COLLAPSED:
			if (pNode == NULL)
			{
				wxASSERT(this->mpCollapsedTree != NULL);
				pNode = this->mpCollapsedTree;
			}
			if (pNode->cChildren.size() == 0u)
			{
				return 0u;
			}
			else
			{
				cChildren.Add(wxDataViewItem(static_cast<void*>(pNode)));
				return 1u;
			}

		default:
			wxLogError("Unknown vvStatusModel display mode: %u", this->GetDisplayMode());
			return 0u;
		}
	}

	// add all the children of the given node
	wxASSERT(pNode != NULL);
	for (TreeNode::stlChildList::iterator it = pNode->cChildren.begin(); it != pNode->cChildren.end(); ++it)
	{
		cChildren.Add(wxDataViewItem(static_cast<void*>(*it)));
	}
	return pNode->cChildren.size();
}

unsigned int vvStatusModel::GetColumnCount() const
{
	return vvStatusModel::COLUMN_COUNT;
}

wxString vvStatusModel::GetColumnType(
	unsigned int uColumn
	) const
{
	Column eColumn = static_cast<Column>(uColumn);

	bool bBool   = this->GetNodeValue_Bool(NULL, eColumn, NULL);
	bool bInt    = this->GetNodeValue_Int(NULL, eColumn, NULL);
	bool bString = this->GetNodeValue_String(NULL, eColumn, NULL);
	bool bIcon   = this->GetNodeValue_Icon(NULL, eColumn, NULL);

	if (bBool && bString && bIcon)
	{
		return "vvDataViewToggleIconText";
	}
	else if (bString && bIcon)
	{
		return "wxDataViewIconText";
	}
	else if (bBool)
	{
		return "bool";
	}
	else if (bInt)
	{
		return "long";
	}
	else if (bString)
	{
		return "string";
	}
	else
	{
		wxLogError("No value(s) types to get for StatusModel column: %u", uColumn);
		return wxEmptyString;
	}
}

void vvStatusModel::GetValue(
	wxVariant&            cValue,
	const wxDataViewItem& cItem,
	unsigned int          uColumn
	) const
{
	wxASSERT(cItem.IsOk());

	Column    eColumn = static_cast<Column>(uColumn);
	TreeNode* pNode   = static_cast<TreeNode*>(cItem.GetID());
	wxASSERT(pNode != NULL);

	vvNullable<bool> nbBool     = false;
	wxString         sString    = wxEmptyString;
	int              iInt       = 0;
	wxIcon           cIcon      = wxNullIcon;
	bool             bHasBool   = this->GetNodeValue_Bool(pNode, eColumn, &nbBool);
	bool             bHasInt    = this->GetNodeValue_Int(pNode, eColumn, &iInt);
	bool             bHasString = this->GetNodeValue_String(pNode, eColumn, &sString);
	bool             bHasIcon   = this->GetNodeValue_Icon(pNode, eColumn, &cIcon);

	bool permanentDisable = false;
	// If they are requesting a toggle column for selection, check the status
	// to see if it's in the "disable" flags the control sent down.
	if (uColumn == COLUMN_IST_FILENAME || uColumn == COLUMN_T_SELECTED || uColumn == COLUMN_IST_PATH)
	{
		permanentDisable = IsPermanentlyDisabled(cItem);
	}

	if (bHasBool && bHasString && bHasIcon)
	{
		cValue << vvDataViewToggleIconText(sString, cIcon, nbBool, permanentDisable);
	}
	else if (bHasString && bHasIcon)
	{
		cValue << wxDataViewIconText(sString, cIcon);
	}
	else if (bHasBool)
	{
		if (nbBool.IsNull())
		{
			cValue.MakeNull();
		}
		else
		{
			cValue = nbBool.GetValue();
		}
	}
	else if (bHasInt)
	{
		cValue = static_cast<long>(iInt);
	}
	else if (bHasString)
	{
		cValue = sString;
	}
	else
	{
		wxLogError("No value(s) to get for StatusModel column: %u", uColumn);
	}
}

bool vvStatusModel::SetValue(
	const wxVariant&      cValue,
	const wxDataViewItem& cItem,
	unsigned int          uColumn
	)
{
	wxASSERT(cItem.IsOk());

	Column    eColumn   = static_cast<Column>(uColumn);
	TreeNode* pNode     = static_cast<TreeNode*>(cItem.GetID());
	wxASSERT(pNode != NULL);

	bool bValueSet = false;
	if (cValue.GetType() == "vvDataViewToggleIconText")
	{
		vvDataViewToggleIconText cData;
		cData << cValue;
		bValueSet = this->SetNodeValue_Bool(pNode, eColumn, cData.GetChecked()) || bValueSet;
		bValueSet = this->SetNodeValue_String(pNode, eColumn, cData.GetText())  || bValueSet;
		bValueSet = this->SetNodeValue_Icon(pNode, eColumn, cData.GetIcon())    || bValueSet;
	}
	else if (cValue.GetType() == "wxDataViewIconText")
	{
		wxDataViewIconText cData;
		cData << cValue;
		bValueSet = this->SetNodeValue_String(pNode, eColumn, cData.GetText()) || bValueSet;
		bValueSet = this->SetNodeValue_Icon(pNode, eColumn, cData.GetIcon())   || bValueSet;
	}
	else if (cValue.GetType() == "bool")
	{
		vvNullable<bool> nbValue;
		if (cValue.IsNull())
		{
			nbValue.SetNull();
		}
		else
		{
			nbValue.SetValue(cValue.GetBool());
		}
		bValueSet = this->SetNodeValue_Bool(pNode, eColumn, nbValue) || bValueSet;
	}
	else if (cValue.GetType() == "long")
	{
		bValueSet = this->SetNodeValue_Int(pNode, eColumn, cValue.GetInteger()) || bValueSet;
	}
	else if (cValue.GetType() == "string")
	{
		bValueSet = this->SetNodeValue_String(pNode, eColumn, cValue.GetString()) || bValueSet;
	}
	else
	{
		wxLogError("SetValue called with invalid value type: %s", cValue.GetType());
	}
	return bValueSet;
}

int vvStatusModel::Compare(
	const wxDataViewItem& cLeft,
	const wxDataViewItem& cRight,
	unsigned int          uColumn,
	bool                  bAscending
	) const
{
	Column    eColumn = static_cast<vvStatusModel::Column>(uColumn);
	TreeNode* pLeft   = static_cast<TreeNode*>(cLeft.GetID());
	TreeNode* pRight  = static_cast<TreeNode*>(cRight.GetID());

	// if we're in descending order, swap the left and right
	if (!bAscending)
	{
		TreeNode* pTemp = pLeft;
		pLeft = pRight;
		pRight = pTemp;
	}

	vvNullable<bool> nbLeftBool   = false;
	vvNullable<bool> nbRightBool  = false;
	wxString         sLeftString  = wxEmptyString;
	wxString         sRightString = wxEmptyString;

	vvStatusControl::ItemType leftType = this->GetNodeItemTypeData(pLeft)->eType;
	vvStatusControl::ItemType rightType = this->GetNodeItemTypeData(pRight)->eType;

	if ((leftType == vvStatusControl::ITEM_FOLDER || leftType == vvStatusControl::ITEM_CONTAINER)
		&& (rightType != vvStatusControl::ITEM_FOLDER && rightType != vvStatusControl::ITEM_CONTAINER))
		return -1;
	if ((rightType == vvStatusControl::ITEM_FOLDER || rightType == vvStatusControl::ITEM_CONTAINER)
		&& (leftType != vvStatusControl::ITEM_FOLDER && leftType != vvStatusControl::ITEM_CONTAINER))
		return 1;

	if (this->GetNodeValue_String(pLeft, eColumn, &sLeftString) && this->GetNodeValue_String(pRight, eColumn, &sRightString))
	{
		int iResult = sLeftString.CompareTo(sRightString, wxString::ignoreCase);
		if (iResult == 0 && eColumn != COLUMN_S_FILENAME)
		{
			return this->Compare(cLeft, cRight, COLUMN_S_FILENAME, bAscending);
		}
		else
		{
			return iResult;
		}
	}
	else if (this->GetNodeValue_Bool(pLeft, eColumn, &nbLeftBool) && this->GetNodeValue_Bool(pRight, eColumn, &nbRightBool))
	{
		if (nbLeftBool == nbRightBool)
		{
			if (eColumn != COLUMN_S_FILENAME)
			{
				return this->Compare(cLeft, cRight, COLUMN_S_FILENAME, bAscending);
			}
			else
			{
				return 0;
			}
		}
		else if (nbLeftBool < nbRightBool)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		wxLogError("No value(s) to compare for StatusModel column: %u", uColumn);
		return wxDataViewModel::Compare(cLeft, cRight, uColumn, bAscending);
	}
}

vvStatusControl::ItemType vvStatusModel::MapItemType(
	vvVerbs::DiffEntry::ItemType eType
	)
{
	// Need to check these here, inside a vvStatusModel function, otherwise they won't have access to the private variable.
	// Hmm, can't get these to work as compile-time asserts.
	wxASSERT(saItemTypeMappings[vvVerbs::DiffEntry::ITEM_FILE]      == vvStatusControl::ITEM_FILE);
	wxASSERT(saItemTypeMappings[vvVerbs::DiffEntry::ITEM_FOLDER]    == vvStatusControl::ITEM_FOLDER);
	wxASSERT(saItemTypeMappings[vvVerbs::DiffEntry::ITEM_SYMLINK]   == vvStatusControl::ITEM_SYMLINK);
	wxASSERT(saItemTypeMappings[vvVerbs::DiffEntry::ITEM_SUBMODULE] == vvStatusControl::ITEM_SUBMODULE);
	wxASSERT(saItemTypeMappings[vvVerbs::DiffEntry::ITEM_COUNT]     == vvStatusControl::ITEM_LAST);

	return saItemTypeMappings[eType];
}

const vvStatusControl::ItemTypeData* vvStatusModel::GetItemTypeData(
	vvVerbs::DiffEntry::ItemType eType
	)
{
	return vvStatusControl::GetItemTypeData(vvStatusModel::MapItemType(eType));
}

vvStatusControl::Status vvStatusModel::MapStatus(
	vvFlags cChanges
	)
{
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(saChangesToStatusMappings); ++uIndex)
	{
		if (cChanges.FlagsEmpty() && saChangesToStatusMappings[uIndex].uChanges == 0u)
		{
			return saChangesToStatusMappings[uIndex].eStatus;
		}
		else if (cChanges.HasAnyFlag(saChangesToStatusMappings[uIndex].uChanges))
		{
			return saChangesToStatusMappings[uIndex].eStatus;
		}
	}

	wxFAIL_MSG(wxString::Format("No entry in saChangesToStatusMappings for flag(s): %u", (unsigned int)cChanges.GetValue()));
	return vvStatusControl::STATUS_LAST;
}

vvStatusControl::Status vvStatusModel::MapStatus(
	const DataItem& cItem
	)
{
	// check what the item's status should be
	if (cItem.pDiffEntry->bItemIgnored || (cItem.pOldDiffEntry != NULL && cItem.pOldDiffEntry->bItemIgnored))
	{
		// if either of the item's entries is ignored, then its status is IGNORED
		return vvStatusControl::STATUS_IGNORED;
	}
	else if (
		   (cItem.pConflictEntry != NULL && !cItem.pConflictEntry->bResolved)
		|| (cItem.pOldConflictEntry != NULL && !cItem.pOldConflictEntry->bResolved)
		)
	{
		// if the item has a conflict record that's unresolved, then its status is CONFLICT
		return vvStatusControl::STATUS_CONFLICT;
	}
	else if (cItem.cFileChanges.size() > 1u)
	{
		// if the item contains multiple changes, then its status is MULTIPLE
		return vvStatusControl::STATUS_MULTIPLE;
	}
	else
	{
		// build a mask of all changes across all of the item's diff entries
		vvFlags cTotalChanges = cItem.pDiffEntry->cItemChanges;
		if (cItem.pOldDiffEntry != NULL)
		{
			cTotalChanges.AddFlags(cItem.pOldDiffEntry->cItemChanges);
		}

		// try the hard-coded change flag mappings
		return vvStatusModel::MapStatus(cTotalChanges);
	}
}

const vvStatusControl::StatusData* vvStatusModel::GetStatusData(
	vvFlags cChanges
	)
{
	return vvStatusControl::GetStatusData(vvStatusModel::MapStatus(cChanges));
}

const vvStatusControl::StatusData* vvStatusModel::GetStatusData(
	const DataItem& cItem
	)
{
	return vvStatusControl::GetStatusData(vvStatusModel::MapStatus(cItem));
}

void vvStatusModel::ClearDisplayData()
{
	this->mpCollapsedTree = NULL;
	this->mpFolderTree    = NULL;
	this->mpFlatTree      = NULL;
	this->mcTreeNodes.clear();
	this->mcDataItems.clear();
}

void vvStatusModel::EnsureDisplayData(
	vvStatusControl::Mode eMode,
	vvFlags               cItemTypes,
	vvFlags               cStatuses
	)
{
	// if we need data items, build them
	if (this->mcDataItems.size() == 0u || cItemTypes != this->GetDisplayTypes() || cStatuses != this->GetDisplayStatuses())
	{
		this->BuildDataItems(this->mcDiffData, this->mcConflictData, cItemTypes, cStatuses, this->mcDataItems);
	}

	// if we need flat tree data, build it
	if (eMode == vvStatusControl::MODE_FLAT && this->mpFlatTree == NULL)
	{
		this->mpFlatTree = this->BuildFlatTree(this->mcDataItems, this->mcTreeNodes);
	}

	// if we need folder tree data, build it
	if ((eMode == vvStatusControl::MODE_FOLDER || eMode == vvStatusControl::MODE_COLLAPSED) && this->mpFolderTree == NULL)
	{
		this->mpFolderTree = this->BuildFolderTree(this->mcDataItems, this->mcTreeNodes);
	}

	// if we need collapsed tree data, build it
	if (eMode == vvStatusControl::MODE_COLLAPSED && this->mpCollapsedTree == NULL)
	{
		this->mpCollapsedTree = this->BuildCollapsedTree(this->mpFolderTree, this->mcTreeNodes);
	}
}

vvVerbs::ResolveItem* vvStatusModel::FindConflictByGid(
	const wxString&              sGid,
	vvVerbs::stlResolveItemList& cConflicts
	)
{
	for (vvVerbs::stlResolveItemList::iterator it = cConflicts.begin(); it != cConflicts.end(); ++it)
	{
		if (it->sGid == sGid)
		{
			return &(*it);
		}
	}
	return NULL;
}

void vvStatusModel::BuildDataItems(
	vvVerbs::DiffData&           cDiff,
	vvVerbs::stlResolveItemList& cConflicts,
	vvFlags                      cItemTypes,
	vvFlags                      cStatuses,
	stlDataItemList&             cItems
	)
{
	typedef std::set<wxString>                            stlMissingSet;
	typedef std::map<wxString, stlDataItemList::iterator> stlDuplicateMap;

	// we'll collect our newly build data items here
	stlDataItemList cNewItems;

	// build a list of item GIDs that have conflicts but are unchanged
	// These are items that we want to display even though they won't appear in cDiff (the status data).
	// This type of item occurs if you resolve a conflict in such a way that the item is identical to its baseline.
	// To display them, later in this function we'll add dummy diff entries for corresponding diff data items to point to.
	// If we later re-build our data items without getting fresh status data, however, we risk adding duplicates.
	// Each GID in the list we generate here is *potentially* missing from cDiff.
	// It will already be present if we're being refreshed with the same status data, it won't be if we're using new status data.
	// As we iterate cDiff later, we'll remove GIDs from this list that we find an existing entry for.
	// After that, any remaining GIDs are *definitely* missing, and will need new dummy entries.
	stlMissingSet cMissing;
	for (vvVerbs::stlResolveItemList::iterator itConflict = cConflicts.begin(); itConflict != cConflicts.end(); ++itConflict)
	{
		if (itConflict->cStatus == vvVerbs::DiffEntry::CHANGE_NONE)
		{
			cMissing.insert(itConflict->sGid);
		}
	}

	// run through each diff entry and build a data item for it
	for (vvVerbs::DiffData::iterator itEntry = cDiff.begin(); itEntry != cDiff.end(); ++itEntry)
	{
		// ignore items whose type doesn't match the given flags
		if (!cItemTypes.HasAnyFlag(vvStatusModel::MapItemType(itEntry->eItemType)))
		{
			continue;
		}

		// create an item
		DataItem cItem;
		cItem.pDiffEntry        = &(*itEntry);
		cItem.pConflictEntry    = this->FindConflictByGid(itEntry->sItemGid, cConflicts);
		cItem.eStatus           = vvStatusControl::STATUS_LAST;
		cItem.bSelected         = false;
		cItem.pOldDiffEntry     = NULL;
		cItem.pOldConflictEntry = NULL;

		// add the item to the list
		cNewItems.push_back(cItem);

		// we have a diff entry for this item and thus already built a data item for it
		// remove it from the list of potentially missing items
		// see long comment near cMissing declaration for more info
		cMissing.erase(itEntry->sItemGid);
	}

	// run through each missing item and build a diff entry and data item for it
	// these are items that aren't in the cDiff/status data that we still want to display
	// see the comments above near the declaration of cMissing
	for (stlMissingSet::const_iterator itMissing = cMissing.begin(); itMissing != cMissing.end(); ++itMissing)
	{
		// find the conflict that we're missing an item for
		vvVerbs::ResolveItem* pConflictEntry = this->FindConflictByGid(*itMissing, cConflicts);
		wxASSERT(pConflictEntry->cStatus == vvVerbs::DiffEntry::CHANGE_NONE);

		// create a dummy diff entry, as the item would have come back from status
		bool bNonExistent = false;
		vvVerbs::DiffEntry cDiffEntry;
		cDiffEntry.eItemType        = pConflictEntry->eType;
		cDiffEntry.sItemGid         = pConflictEntry->sGid;
		cDiffEntry.sItemPath        = pConflictEntry->FindBestRepoPath(&bNonExistent);
		if (bNonExistent)
		{
			cDiffEntry.sItemPath += " (non-existent)";
		}
		cDiffEntry.cItemChanges     = pConflictEntry->cStatus;
		cDiffEntry.bItemIgnored     = false;
		// TODO: Set sContentHid and sOldContentHid somehow?  They'd be equal.
		cDiffEntry.bSubmodule_Dirty = false;
		cDiff.push_back(cDiffEntry);

		// create a data item for the new dummy entry
		DataItem cItem;
		cItem.pDiffEntry        = &cDiff.back();
		cItem.pConflictEntry    = pConflictEntry;
		cItem.eStatus           = vvStatusControl::STATUS_LAST;
		cItem.bSelected         = false;
		cItem.pOldDiffEntry     = NULL;
		cItem.pOldConflictEntry = NULL;

		// add the item to the list
		cNewItems.push_back(cItem);
	}

	// run through each item we created for some post-processing
	stlDuplicateMap cDuplicates;
	for (stlDataItemList::iterator itItem = cNewItems.begin(); itItem != cNewItems.end(); )
	{
		// make a canonical version of the item's repo path for use as a key in cDuplicates
		wxString sStandardRepoPath = vvTrimString(itItem->pDiffEntry->sItemPath, wxFileName::GetPathSeparators(), true);
		sStandardRepoPath.LowerCase();
		// TODO: Paths are case-insensitive on Windows, and since Tortoise only
		//       runs on Windows at the moment, we'll lowercase all of the paths
		//       to get a unique version we can use as a key.  If Tortoise ever
		//       makes its way to other platforms, this should be reconsidered.

		// this is a mask of change flags that denote that an item has a potential
		// to be a duplicate, meaning it might share a repo path with another item
		static vvFlags cDuplicateMask =
			  vvVerbs::DiffEntry::CHANGE_DELETED
			| vvVerbs::DiffEntry::CHANGE_FOUND
			| vvVerbs::DiffEntry::CHANGE_ADDED
			| vvVerbs::DiffEntry::CHANGE_LOST
			| vvVerbs::DiffEntry::CHANGE_MOVED
			| vvVerbs::DiffEntry::CHANGE_RENAMED
			| vvVerbs::DiffEntry::CHANGE_LOCKED
			;

		// check if this item is a potential duplicate
		vvNullable<stlDuplicateMap::iterator> nitOriginal = vvNULL;
		if (itItem->pDiffEntry->cItemChanges.HasAnyFlag(cDuplicateMask))
		{
			// might be a duplicate, look it up to find out for sure
			nitOriginal.SetValue(cDuplicates.find(sStandardRepoPath));
			if (nitOriginal.GetValue() == cDuplicates.end())
			{
				// not a duplicate, add it to the list to check future items against
				cDuplicates[sStandardRepoPath] = itItem;
			}
			else
			{
				// there is already an item with this repo path, we have found a pair of duplicates
				// see comments on pOldDiffEntry and pOldConflictEntry for details
				// What we need to do here is absorb the original item's entries
				// into this current item and then remove the original item from
				// the list, resulting in a single item that contains entries for
				// both.  The entries representing a deletion should be stored in
				// the composite item's "Old" entry members.
				stlDataItemList::iterator itOriginal = nitOriginal.GetValue()->second;
				wxASSERT(itItem->pOldDiffEntry         == NULL);
				wxASSERT(itItem->pOldConflictEntry     == NULL);
				wxASSERT(itOriginal->pOldDiffEntry     == NULL);
				wxASSERT(itOriginal->pOldConflictEntry == NULL);

				// one of the items must be the deleted one
				if (itItem->pDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_DELETED))
				{
					// the current item is the deleted one
					// the original one must NOT be the deleted one
					wxASSERT(!itOriginal->pDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_DELETED));
					itItem->pOldDiffEntry     = itItem->pDiffEntry;
					itItem->pOldConflictEntry = itItem->pConflictEntry;
					itItem->pDiffEntry        = itOriginal->pDiffEntry;
					itItem->pConflictEntry    = this->FindConflictByGid(itOriginal->pDiffEntry->sItemGid, cConflicts);
				}
				else
				{
					// the original item is the deleted one
					wxASSERT(itOriginal->pDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_DELETED));
					itItem->pOldDiffEntry     = itOriginal->pDiffEntry;
					itItem->pOldConflictEntry = itOriginal->pConflictEntry;
				}

				// remove the original item from the main list
				// since we've absorbed its data into its duplicate
				// Note: This works even though we're currently iterating the same
				//       container that we're erasing from, because it's a std::list.
				//       They don't invalidate other iterators when erasing, so our
				//       current iterator won't be affected.
				cNewItems.erase(itOriginal);

				// remove this path from the list of potential duplicates
				// to save lookup time in cDuplicates later
				cDuplicates.erase(nitOriginal.GetValue());
			}
		}

		// figure out the item's status
		itItem->eStatus = vvStatusModel::MapStatus(*itItem);

		// remove items whose status doesn't match the given flags
		if (!cStatuses.HasAnyFlag(itItem->eStatus))
		{
			// if we're removing the item,
			// we shouldn't consider it a potential duplicate
			cDuplicates.erase(sStandardRepoPath);

			// remove the item from the list
			// and move on to the next item
			itItem = cNewItems.erase(itItem);
			continue;
		}

		// run through each individual change on the item and make a FileChange
		// Note that it's possible for an item to have several diff entries,
		// each with its own set of changes.  We need to run through all of them.
		vvVerbs::DiffEntry* aDiffEntries[] = { itItem->pDiffEntry, itItem->pOldDiffEntry };
		for (unsigned int uEntry = 0u; uEntry < vvARRAY_COUNT(aDiffEntries); ++uEntry)
		{
			// get the current entry
			vvVerbs::DiffEntry* pEntry = aDiffEntries[uEntry];
			if (pEntry == NULL)
			{
				continue;
			}

			// run through each change on this entry
			for (vvFlags::const_iterator itFlag = pEntry->cItemChanges.begin(); itFlag != pEntry->cItemChanges.end(); ++itFlag)
			{
				FileChange cChange;
				cChange.cChanges = *itFlag;
				cChange.sFrom    = wxEmptyString;
				if (cChange.cChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_RENAMED))
				{
					cChange.sFrom = pEntry->sRenamed_From;
				}
				else if (cChange.cChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_MOVED))
				{
					cChange.sFrom = pEntry->sMoved_From;
				}
				else if (cChange.cChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_LOCKED))
				{
					if (!cStatuses.HasAnyFlag(vvStatusControl::STATUS_LOCKED))
						continue;
					cChange.sFrom = msCurrentUser;
				}
				else if (cChange.cChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_LOCK_CONFLICT))
				{
					if (!cStatuses.HasAnyFlag(vvStatusControl::STATUS_LOCK_CONFLICT))
						continue;
					for (vvVerbs::LockList::const_iterator itLock = pEntry->cLocks.begin();
						itLock != pEntry->cLocks.end();
						++itLock)
					{
						if (msCurrentUser.CompareTo(itLock->sUsername) != 0)
						//&& cItem.pDiffEntry->sContentHid.CompareTo(cItem.pLockEntry->sStartHid) != 0
						{
							//Some other user has a lock on the file, and we have a pending change.  Tell the user about it.
							cChange.sFrom = "Your pending change violates a lock by ";
						}
						cChange.sFrom += itLock->sUsername;
						if (!itLock->sEndCSID.IsEmpty())
							cChange.sFrom += wxString::Format(" -- waiting for changeset %0.8s to be pulled", itLock->sEndCSID);
					}
				}
				itItem->cFileChanges.push_back(cChange);
			}
		}

		// run through each conflict on the item and make a FileConflict
		// Note that it's possible for an item to have several conflict entries,
		// each with its own set of conflicts.  We need to run through all of them.
		vvVerbs::ResolveItem* aConflictEntries[] = { itItem->pConflictEntry, itItem->pOldConflictEntry };
		for (unsigned int uEntry = 0u; uEntry < vvARRAY_COUNT(aConflictEntries); ++uEntry)
		{
			// get the current entry
			vvVerbs::ResolveItem* pEntry = aConflictEntries[uEntry];
			if (pEntry == NULL)
			{
				continue;
			}

			// run through each conflict on this entry
			if (pEntry != NULL)
			{
				for (vvVerbs::stlResolveChoiceMap::iterator itChoice = pEntry->cChoices.begin(); itChoice != pEntry->cChoices.end(); ++itChoice)
				{
					FileConflict cConflict;
					cConflict.pChoice = &(itChoice->second);
					for (vvVerbs::stlResolveCauseMap::const_iterator itCause = itChoice->second.cCauses.begin(); itCause != itChoice->second.cCauses.end(); ++itCause)
					{
						cConflict.sFrom += itCause->first;
						cConflict.sFrom += ", ";
					}
					cConflict.sFrom.RemoveLast(2u);
					if (itChoice->second.bResolved)
					{
						cConflict.sFrom += " (";
						if (itChoice->second.bDeleted)
						{
							cConflict.sFrom += "deleted";
						}
						else
						{
							cConflict.sFrom += itChoice->second.sResolution.Capitalize();
						}
						cConflict.sFrom += ")";
					}
					itItem->cFileConflicts.push_back(cConflict);
				}
			}
		}

		// make sure that if our status is MULTIPLE, then we have several file change entries
		wxASSERT(itItem->eStatus != vvStatusControl::STATUS_MULTIPLE || itItem->cFileChanges.size() > 1u);

		// make sure that if our status is CONFLICT, then we actually have conflicts
		wxASSERT(itItem->eStatus != vvStatusControl::STATUS_CONFLICT || itItem->cFileConflicts.size() > 0u);

		// move on to the next item
		++itItem;
	}

	// splice the new items into the output list
	cItems.splice(cItems.end(), cNewItems);
}

vvStatusModel::TreeNode* vvStatusModel::BuildFlatTree(
	stlDataItemList& cItems,
	stlTreeNodeList& cNodes
	)
{
	// create a root node
	TreeNode* pRoot = this->CreateTreeNode(cNodes);

	// run through each item and make a node for it under the root
	for (stlDataItemList::iterator it = cItems.begin(); it != cItems.end(); ++it)
	{
		DataItem& cItem = *it;
		TreeNode* pNode = this->CreateTreeNode(cNodes, pRoot);
		pNode->sName = cItem.pDiffEntry->sItemPath;
		pNode->pItem = &cItem;
		this->CreateChangeNodes(cNodes, pNode);
		this->CreateConflictNodes(cNodes, pNode);
	}

	// return the root
	return pRoot;
}

vvStatusModel::TreeNode* vvStatusModel::BuildFolderTree(
	stlDataItemList& cItems,
	stlTreeNodeList& cNodes
	)
{
	// create a root node
	TreeNode* pRoot = this->CreateTreeNode(cNodes);

	// run through each item
	for (stlDataItemList::iterator it = cItems.begin(); it != cItems.end(); ++it)
	{
		DataItem& cItem = *it;

		// split the item's path into its constituent folder/file parts
		std::list<wxString> cParts;
		wxStringTokenizer cTokenizer(cItem.pDiffEntry->sItemPath, L"\\/");
		while (cTokenizer.HasMoreTokens())
		{
			cParts.push_back(cTokenizer.GetNextToken());
		}
		//With the new WC, there are multiple valid repo prefixes (@, @b, @c, @g...)
		if (!cParts.front().StartsWith(vvVerbs::sszRepoRoot))
		{
			wxLogError("Omitting invalid path that doesn't start with '%s': %s", vvVerbs::sszRepoRoot, cItem.pDiffEntry->sItemPath);
			continue;
		}
		if (cParts.front() != vvVerbs::sszRepoRoot)
		{
			if (cItem.pDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_DELETED))
			{
				//This decorates deleted items with (), so that they can't conflict with 
				//another item at the same path (for example an ADD operation)
				cParts.back() = wxString::Format("(%s)", cParts.back());

				//Doing this instead would segregate all of the deleted items into a separate
				//section of the control.
				//cParts.insert(++cParts.begin(), "__DELETED_ITEMS__");
			}
			cParts.front() = vvVerbs::sszRepoRoot;
		}
		// descend the tree and find/create the correct node(s) for this item
		TreeNode* pNode = this->FindTreeNodeDecendent(pRoot, cParts.begin(), cParts.end(), &cNodes);
		if (pNode->pItem != NULL)
		{
			wxLogError("Duplicate path in status: %s", cItem.pDiffEntry->sItemPath);
		}

		// set the node's item
		pNode->pItem = &cItem;

		// create any child change/conflict nodes that this node needs
		this->CreateChangeNodes(cNodes, pNode);
		this->CreateConflictNodes(cNodes, pNode);
	}

	// if we have a child with name gsRepoPathRoot, return that as the root
	// otherwise all of the paths were invalid and we'll have no children
	TreeNode* pResult = this->FindTreeNodeChild(pRoot, vvVerbs::sszRepoRoot);
	if (pResult == NULL)
	{
		wxASSERT(pRoot->cChildren.size() == 0u);
		return pRoot;
	}
	else
	{
		pResult->pParent = NULL;
		return pResult;
	}
}

vvStatusModel::TreeNode* vvStatusModel::BuildCollapsedTree(
	TreeNode*        pFolderTreeNode,
	stlTreeNodeList& cNodes
	)
{
	// make a copy of the node for the collapsed tree
	TreeNode* pCollapsedNode = this->CreateTreeNode(cNodes);

	// we'll have a NULL parent for now
	// if we called ourselves, then our caller will set the parent appropriately
	// if someone else called us, then they'll be expecting a root node anyway
	pCollapsedNode->pParent = NULL;

	// copy the current node's name and item/change data
	pCollapsedNode->sName     = pFolderTreeNode->sName;
	pCollapsedNode->pItem     = pFolderTreeNode->pItem;
	pCollapsedNode->pChange   = pFolderTreeNode->pChange;
	pCollapsedNode->pConflict = pFolderTreeNode->pConflict;

	// if our collapsed node has no item/change/conflict data
	// then we have a chance to collapse its children into it
	TreeNode* pDeepestCollapsibleNode = pFolderTreeNode;
	if (pCollapsedNode->pItem == NULL && pCollapsedNode->pChange == NULL && pCollapsedNode->pConflict == NULL)
	{
		// descend through the current node's single children that have no item data
		// we will collapse the entire chain of them into the new collapsed node
		// append each intermediate node's name onto ours as we descend
		while (
			   pDeepestCollapsibleNode->cChildren.size() == 1u
			&& pDeepestCollapsibleNode->cChildren.front()->pItem == NULL
			&& pDeepestCollapsibleNode->cChildren.front()->pChange == NULL
			)
		{
			pDeepestCollapsibleNode = pDeepestCollapsibleNode->cChildren.front();
			pCollapsedNode->sName += wxString::Format("%s%s", gsCollapsedPathSeparator, pDeepestCollapsibleNode->sName);
		}
	}

	// the children of the deepest child we found are the children we want on our collapsed node
	// Note: if we didn't collapse anything, then the deepest collapsible node will still be the original one
	for (TreeNode::stlChildList::iterator it = pDeepestCollapsibleNode->cChildren.begin(); it != pDeepestCollapsibleNode->cChildren.end(); ++it)
	{
		// instead of making a simple copy, recurse into ourselves
		// this allows our children to collapse their children as well
		TreeNode* pChildCopy = this->BuildCollapsedTree(*it, cNodes);
		pChildCopy->pParent = pCollapsedNode;
		pCollapsedNode->cChildren.push_back(pChildCopy);
	}

	// return the collapsed node
	return pCollapsedNode;
}

vvStatusModel::TreeNode* vvStatusModel::CreateTreeNode(
	stlTreeNodeList& cNodes,
	TreeNode*        pParent
	)
{
	TreeNode cNode;
	cNode.pParent   = pParent;
	cNode.sName     = wxEmptyString;
	cNode.pItem     = NULL;
	cNode.pChange   = NULL;
	cNode.pConflict = NULL;
	cNodes.push_back(cNode);

	TreeNode* pNode = &cNodes.back();

	if (pParent != NULL)
	{
		pParent->cChildren.push_back(pNode);
	}

	return pNode;
}

bool vvStatusModel::CreateChangeNodes(
	stlTreeNodeList& cNodes,
	TreeNode*        pParent
	)
{
	// if the parent would only have a single child node (change, lock, OR conflict), then it's unnecessary
	unsigned int changeCount = pParent->pItem->cFileChanges.size() + pParent->pItem->cFileConflicts.size();
	if (pParent->pItem == NULL ||   changeCount <= 1u)
	{
		return false;
	}

	// make a child node for each change
	for (stlFileChangeList::iterator it = pParent->pItem->cFileChanges.begin(); it != pParent->pItem->cFileChanges.end(); ++it)
	{
		TreeNode* pChild = this->CreateTreeNode(cNodes, pParent);
		pChild->pItem   = pParent->pItem;
		pChild->pChange = &(*it);
	}
	return true;
}

bool vvStatusModel::CreateConflictNodes(
	stlTreeNodeList& cNodes,
	TreeNode*        pParent
	)
{
	// if the parent has no conflicts, don't bother
	if (pParent->pItem == NULL || pParent->pItem->cFileConflicts.size() == 0u)
	{
		return false;
	}

	// make a child node for each conflict
	for (stlFileConflictList::iterator it = pParent->pItem->cFileConflicts.begin(); it != pParent->pItem->cFileConflicts.end(); ++it)
	{
		TreeNode* pChild = this->CreateTreeNode(cNodes, pParent);
		pChild->pItem     = pParent->pItem;
		pChild->pConflict = &(*it);
	}
	return true;
}

vvStatusModel::TreeNode* vvStatusModel::FindTreeNodeChild(
	TreeNode*       pParent,
	const wxString& sName
	)
{
	for (TreeNode::stlChildList::iterator it = pParent->cChildren.begin(); it != pParent->cChildren.end(); ++it)
	{
		TreeNode* pChild = *it;
		if (pChild->sName == sName)
		{
			return pChild;
		}
	}
	return NULL;
}

template <typename Iterator>
vvStatusModel::TreeNode* vvStatusModel::FindTreeNodeDecendent(
	TreeNode*            pParent,
	Iterator             itBegin,
	Iterator             itEnd,
	stlTreeNodeList*     pNodes
	)
{
	// if the range of children is empty, then this is the node we're looking for
	if (itBegin == itEnd)
	{
		return pParent;
	}

	// look for the first child in the list
	TreeNode* pChild = this->FindTreeNodeChild(pParent, *itBegin);
	if (pChild == NULL)
	{
		// didn't find one, check if we should create it
		if (pNodes == NULL)
		{
			return NULL;
		}
		else
		{
			pChild = this->CreateTreeNode(*pNodes, pParent);
			pChild->sName = *itBegin;
			pChild->pItem = NULL;
		}
	}

	// recursively descend into the child, looking for the next child in the list
	return this->FindTreeNodeDecendent(pChild, ++itBegin, itEnd, pNodes);
}

vvNullable<bool> vvStatusModel::GetNodeChildrenSelected(
	TreeNode* pNode
	) const
{
	wxASSERT(pNode->cChildren.size() > 0u);

	vvNullable<bool> nbReturnVal = false;

	int nCountTrue = 0;
	int nCountFalse = 0;
	for (TreeNode::stlChildList::const_iterator it = pNode->cChildren.begin(); it != pNode->cChildren.end(); ++it)
	{
		// get this child's selection state, recursively
		vvNullable<bool> nbCurrent;
		if (!this->GetNodeValue_Bool(*it, COLUMN_T_SELECTED, &nbCurrent)
			|| this->IsPermanentlyDisabled(*it) )
		{
			//Permanently disabled child nodes do not contribute to the parent's checked state.
			continue;
		}

		if (nbCurrent == true)
		{
			nCountTrue++;
		}
		else if (nbCurrent == false)
		{
			nCountFalse++;
		}

		if (nbCurrent.IsNull() || (nCountTrue != 0 && nCountFalse != 0))
		{
			//We can quit looking at children now. As soon as one of our children is NULL, 
			//or two of our children disagree, we will return NULL.
			nbReturnVal.SetNull();
			return nbReturnVal;
		}
	}
	//It's either all true, or all false
	if (nCountTrue != 0)
		nbReturnVal = true;
	else if (nCountFalse != 0)
		nbReturnVal = false;
	else 
		nbReturnVal = false; //There are no children, or all children are permanently disabled.
	return nbReturnVal;
}

const vvStatusControl::ItemTypeData* vvStatusModel::GetNodeItemTypeData(
	const TreeNode* pNode
	) const
{
	if (pNode->pChange != NULL)
	{
		return vvStatusControl::GetItemTypeData(vvStatusControl::ITEM_CHANGE);
	}
	else if (pNode->pConflict != NULL)
	{
		return vvStatusControl::GetItemTypeData(vvStatusControl::ITEM_CONFLICT);
	}
	else if (pNode->pItem != NULL && pNode->pItem->pDiffEntry != NULL)
	{
		// Note that here we might have an item whose pDiffEntry and pOldDiffEntry
		// don't have the same item type.  We're going to choose to use pDiffEntry's
		// type, both because it's easier (don't have to check if pOldDiffEntry is
		// non-NULL) and because it represents what is currently on the file system.
		return vvStatusModel::GetItemTypeData(pNode->pItem->pDiffEntry->eItemType);
	}
	else if (pNode->cChildren.size() > 0u)
	{
		return vvStatusControl::GetItemTypeData(vvStatusControl::ITEM_CONTAINER);
	}
	else
	{
		return vvStatusControl::GetItemTypeData(vvStatusControl::ITEM_LAST);
	}
}

const vvStatusControl::StatusData* vvStatusModel::GetNodeStatusData(
	const TreeNode* pNode
	) const
{
	if (pNode->pConflict != NULL)
	{
		return vvStatusControl::GetStatusData(pNode->pConflict->pChoice->bResolved ? vvStatusControl::STATUS_RESOLVED : vvStatusControl::STATUS_CONFLICT);
	}
	else if (pNode->pChange != NULL)
	{
		return vvStatusModel::GetStatusData(pNode->pChange->cChanges);
	}
	else if (pNode->pItem != NULL)
	{
		return vvStatusModel::GetStatusData(*pNode->pItem);
	}
	else
	{
		return vvStatusControl::GetStatusData(vvStatusControl::STATUS_LAST);
	}
}

bool vvStatusModel::GetNodeValue_Bool(
	TreeNode*         pNode,
	Column            eColumn,
	vvNullable<bool>* pValue
	) const
{
	wxASSERT(pNode != NULL || pValue == NULL);
	bool             bHasValue = false;
	vvNullable<bool> bValue    = false;

	switch (eColumn)
	{
	case COLUMN_IST_FILENAME:
	case COLUMN_IST_PATH:
	case COLUMN_T_SELECTED:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pItem != NULL && pNode->pItem->bSelected) //If the item is already explicitly selected, then return yes.
			{
				bValue = true;
			}
			else if (pNode->cChildren.size() > 0u)
			{
				bValue = GetNodeChildrenSelected(pNode);
			}
			else if (pNode->pItem == NULL)
			{
				bValue = false;
			}
			else
			{
				bValue = pNode->pItem->bSelected;
			}
		}
		break;
	case COLUMN_B_IGNORED:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pItem != NULL && pNode->pItem->pDiffEntry != NULL)
			{
				bValue = pNode->pItem->pDiffEntry->bItemIgnored;
				if (pNode->pItem->pOldDiffEntry != NULL)
				{
					bValue = bValue && pNode->pItem->pOldDiffEntry->bItemIgnored;
				}
			}
			else
			{
				bValue = false;
			}
			wxASSERT(bValue  || pNode->pItem->eStatus != vvStatusControl::STATUS_IGNORED);
			wxASSERT(!bValue || pNode->pItem->eStatus == vvStatusControl::STATUS_IGNORED);
		}
		break;
#if 0
	case COLUMN_B_ATTRIBUTES:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL)
			{
				bValue = pNode->pChange->cChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_ATTRS);
			}
			else if (pNode->pConflict != NULL)
			{
				bValue = false;
			}
			else if (pNode->pItem != NULL && pNode->pItem->pDiffEntry != NULL)
			{
				bValue = pNode->pItem->pDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_ATTRS);
				if (pNode->pItem->pOldDiffEntry != NULL)
				{
					bValue = bValue || pNode->pItem->pOldDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_ATTRS);
				}
			}
			else
			{
				bValue = false;
			}
		}
		break;
#endif
	case COLUMN_B_MULTIPLE:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL || pNode->pConflict != NULL)
			{
				bValue = false;
			}
			else if (pNode->pItem != NULL)
			{
				wxASSERT(pNode->pItem->eStatus == vvStatusControl::STATUS_MULTIPLE);
				bValue = pNode->pItem->cFileChanges.size() > 1u;
			}
			else
			{
				bValue = false;
			}
		}
		break;
	default:
		break;
	}

	if (bHasValue && pValue != NULL)
	{
		*pValue = bValue;
	}

	return bHasValue;
}

bool vvStatusModel::GetNodeValue_Int(
	TreeNode* pNode,
	Column    eColumn,
	int*      pValue
	) const
{
	wxASSERT(pNode != NULL || pValue == NULL);
	bool             bHasValue = false;
	int              iValue    = 0;

	switch (eColumn)
	{
	case COLUMN_E_STATUS:
		bHasValue = true;
		if (pValue != NULL)
		{
			iValue = this->GetNodeStatusData(pNode)->eStatus;
		}
		break;
	case COLUMN_E_TYPE:
		bHasValue = true;
		if (pValue != NULL)
		{
			iValue = this->GetNodeItemTypeData(pNode)->eType;
		}
		break;
	case COLUMN_E_SUBTYPE:
		bHasValue = true;
		if (pValue != NULL)
		{
			// Note: Switch expression should be equivalent to the result of
			//       calling this function with COLUMN_E_TYPE.
			switch (this->GetNodeItemTypeData(pNode)->eType)
			{
			case vvStatusControl::ITEM_CONFLICT:
				iValue = pNode->pConflict->pChoice->eState;
				break;
			default:
				iValue = -1;
				break;
			}
		}
		break;
	default:
		break;
	}

	if (bHasValue && pValue != NULL)
	{
		*pValue = iValue;
	}

	return bHasValue;
}

bool vvStatusModel::GetNodeValue_String(
	TreeNode* pNode,
	Column    eColumn,
	wxString* pValue
	) const
{
	wxASSERT(pNode != NULL || pValue == NULL);
	bool     bHasValue = false;
	wxString sValue    = wxEmptyString;

	switch (eColumn)
	{
	case COLUMN_IS_FILENAME:
	case COLUMN_IST_FILENAME:
	case COLUMN_S_FILENAME:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL)
			{
				sValue = vvStatusModel::GetNodeStatusData(pNode)->szName;
			}
			else if (pNode->pConflict != NULL)
			{
				sValue = pNode->pConflict->pChoice->sName.Capitalize();
			}
			else
			{
				sValue = pNode->sName;
			}
		}
		break;
	case COLUMN_IS_PATH:
	case COLUMN_IST_PATH:
	case COLUMN_S_PATH:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pItem != NULL && pNode->pItem->pDiffEntry != NULL)
			{
				sValue = pNode->pItem->pDiffEntry->sItemPath;
			}
			else if (pNode->sName != wxEmptyString)
			{
				// build a path from this node's name and its parent's path, if any
				wxString sPath;
				if (pNode->pParent != NULL)
				{
					this->GetNodeValue_String(pNode->pParent, eColumn, &sPath);
					if (!sPath.IsEmpty())
					{
						sPath.Append(gsCollapsedPathSeparator);
					}
				}
				sPath.Append(pNode->sName);
				sValue = sPath;
			}
			else
			{
				sValue = wxEmptyString;
			}
		}
		break;
	case COLUMN_IS_STATUS:
	case COLUMN_S_STATUS:
		bHasValue = true;
		if (pValue != NULL)
		{
			sValue = this->GetNodeStatusData(pNode)->szName;
		}
		break;
	case COLUMN_S_FROM:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL)
			{
				sValue = pNode->pChange->sFrom;
			}
			else if (pNode->pConflict != NULL)
			{
				sValue = pNode->pConflict->sFrom;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->cFileChanges.size() <= 1u
				&& pNode->pItem->pDiffEntry != NULL
				&& pNode->pItem->pDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_MOVED)
				)
			{
				sValue = pNode->pItem->pDiffEntry->sMoved_From;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->cFileChanges.size() <= 1u
				&& pNode->pItem->pDiffEntry != NULL
				&& pNode->pItem->pDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_RENAMED)
				)
			{
				sValue = pNode->pItem->pDiffEntry->sRenamed_From;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->cFileChanges.size() <= 1u
				&& pNode->pItem->pOldDiffEntry != NULL
				&& pNode->pItem->pOldDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_MOVED)
				)
			{
				sValue = pNode->pItem->pOldDiffEntry->sMoved_From;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->cFileChanges.size() <= 1u
				&& pNode->pItem->pOldDiffEntry != NULL
				&& pNode->pItem->pOldDiffEntry->cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_RENAMED)
				)
			{
				sValue = pNode->pItem->pOldDiffEntry->sRenamed_From;
			}
			else
			{
				sValue = wxEmptyString;
			}
		}
		break;
	case COLUMN_IS_TYPE:
	case COLUMN_S_TYPE:
		bHasValue = true;
		if (pValue != NULL)
		{
			sValue = this->GetNodeItemTypeData(pNode)->szName;
		}
		break;
	case COLUMN_S_GID:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL || pNode->pConflict != NULL)
			{
				sValue = wxEmptyString;
			}
			else if (pNode->pItem != NULL && pNode->pItem->pDiffEntry != NULL)
			{
				// Note that the item may also have a pOldDiffEntry with a different
				// GID.  We'll just have to pick one of the GIDs to return, so we'll
				// pick pDiffEntry's because it's the one currently in the file
				// system.  Plus it's easier because we don't have to check if
				// pOldDiffEntry is NULL or not.
				sValue = pNode->pItem->pDiffEntry->sItemGid;
			}
			else
			{
				sValue = wxEmptyString;
			}
		}
		break;
	case COLUMN_S_HID_NEW:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL || pNode->pConflict != NULL)
			{
				sValue = wxEmptyString;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->pDiffEntry != NULL
				&& !pNode->pItem->pDiffEntry->sContentHid.IsEmpty()
				)
			{
				sValue = pNode->pItem->pDiffEntry->sContentHid;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->pOldDiffEntry != NULL
				)
			{
				sValue = pNode->pItem->pOldDiffEntry->sContentHid;
			}
			else
			{
				sValue = wxEmptyString;
			}
		}
		break;
	case COLUMN_S_HID_OLD:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL || pNode->pConflict != NULL)
			{
				sValue = wxEmptyString;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->pOldDiffEntry != NULL
				&& !pNode->pItem->pOldDiffEntry->sOldContentHid.IsEmpty()
				)
			{
				sValue = pNode->pItem->pOldDiffEntry->sOldContentHid;
			}
			else if (
				   pNode->pItem != NULL
				&& pNode->pItem->pDiffEntry != NULL
				)
			{
				sValue = pNode->pItem->pDiffEntry->sOldContentHid;
			}
			else
			{
				sValue = wxEmptyString;
			}
		}
		break;
	case COLUMN_S_NOTES:
		bHasValue = true;
		if (pValue != NULL)
		{
			wxString         sFrom    = wxEmptyString;
			vvNullable<bool> bIgnored = false;
			if (this->GetNodeValue_String(pNode, COLUMN_S_FROM, &sFrom) && sFrom != wxEmptyString)
			{
				sValue = wxString::Format("From: %s", sFrom);
			}
			else if (this->GetNodeValue_Bool(pNode, COLUMN_B_IGNORED, &bIgnored) && bIgnored.IsValid() && bIgnored.GetValue())
			{
				sValue = "Ignored";
			}
			else
			{
				sValue = wxEmptyString;
			}
		}
		break;
	default:
		break;
	}

	if (bHasValue && pValue != NULL)
	{
		*pValue = sValue;
	}

	return bHasValue;
}

bool vvStatusModel::GetNodeValue_Icon(
	TreeNode* pNode,
	Column    eColumn,
	wxIcon*   pValue
	) const
{
	wxASSERT(pNode != NULL || pValue == NULL);
	bool     bHasValue = false;
	wxIcon   cValue    = wxNullIcon;

	switch (eColumn)
	{
	case COLUMN_I_FILENAME:
	case COLUMN_IS_FILENAME:
	case COLUMN_IST_FILENAME:
		bHasValue = true;
		if (pValue != NULL)
		{
			if (pNode->pChange != NULL || pNode->pConflict != NULL)
			{
				cValue = wxArtProvider::GetIcon(vvStatusModel::GetNodeStatusData(pNode)->szIcon, wxART_OTHER, wxSize(16,16));
			}
			else
			{
				cValue = wxArtProvider::GetIcon(vvStatusModel::GetNodeItemTypeData(pNode)->szIcon, wxART_OTHER, wxSize(16,16));
			}
		}
		break;
	case COLUMN_I_STATUS:
	case COLUMN_IS_STATUS:
		bHasValue = true;
		if (pValue != NULL)
		{
			cValue = wxArtProvider::GetIcon(this->GetNodeStatusData(pNode)->szIcon, wxART_OTHER, wxSize(16,16));
		}
		break;
	case COLUMN_I_TYPE:
	case COLUMN_IS_TYPE:
		bHasValue = true;
		if (pValue != NULL)
		{
			cValue = wxArtProvider::GetIcon(this->GetNodeItemTypeData(pNode)->szIcon, wxART_OTHER, wxSize(16,16));
		}
		break;
	default:
		break;
	}

	if (bHasValue && pValue != NULL)
	{
		*pValue = cValue;
	}

	return bHasValue;
}

bool vvStatusModel::SetNodeValue_Bool(
	TreeNode*        pNode,
	Column           eColumn,
	vvNullable<bool> nbValue
	)
{
	wxASSERT(pNode != NULL);

	switch (eColumn)
	{
	case COLUMN_IST_FILENAME:
		// don't allow the selected/checked state to be indeterminate
		// we don't want to actually SET an indeterminate state, only retrieve it sometimes
		if (nbValue.IsNull())
		{
			wxLogError("Cannot set a file to an indeterminate selected state.");
			return false;
		}

		// if this is a file item, store its new selected state
		if (pNode->pItem != NULL)
		{
			// this is a file, just store the current selected state
			pNode->pItem->bSelected = nbValue.GetValue();
		}

		// recursively propagate the new selected state to the node's children
		for (TreeNode::stlChildList::iterator it = pNode->cChildren.begin(); it != pNode->cChildren.end(); ++it)
		{
			this->SetNodeValue_Bool(*it, eColumn, nbValue);
		}

		// done
		return true;
	case COLUMN_T_SELECTED:
		if (nbValue.IsNull())
		{
			wxLogError("Cannot set a file to an indeterminate selected state.");
			return false;
		}
		else if (pNode->pItem != NULL)
		{
			pNode->pItem->bSelected = nbValue.GetValue();
		}
		return true;
	default:
		return false;
	}
}

bool vvStatusModel::SetNodeValue_Int(
	TreeNode*       pNode,
	Column          WXUNUSED(eColumn),
	int             WXUNUSED(iValue)
	)
{
	wxASSERT(pNode != NULL);
	return false;
}

bool vvStatusModel::SetNodeValue_String(
	TreeNode*       pNode,
	Column          WXUNUSED(eColumn),
	const wxString& WXUNUSED(sValue)
	)
{
	wxASSERT(pNode != NULL);
	return false;
}

bool vvStatusModel::SetNodeValue_Icon(
	TreeNode* pNode,
	Column    WXUNUSED(eColumn),
	wxIcon    WXUNUSED(cValue)
	)
{
	wxASSERT(pNode != NULL);
	return false;
}


/*
**
** Public Functions
**
*/

const vvStatusControl::ItemTypeData* vvStatusControl::GetItemTypeData(
	ItemType eType
	)
{
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaItemTypeDatas); ++uIndex)
	{
		if (gaItemTypeDatas[uIndex].eType == eType)
		{
			return gaItemTypeDatas + uIndex;
		}
	}

	wxASSERT(eType != ITEM_LAST);
	return vvStatusControl::GetItemTypeData(ITEM_LAST);
}

const vvStatusControl::StatusData* vvStatusControl::GetStatusData(
	Status eStatus
	)
{
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaStatusDatas); ++uIndex)
	{
		if (gaStatusDatas[uIndex].eStatus == eStatus)
		{
			return gaStatusDatas + uIndex;
		}
	}

	wxASSERT(eStatus != STATUS_LAST);
	return vvStatusControl::GetStatusData(STATUS_LAST);
}

const vvStatusControl::ColumnData* vvStatusControl::GetColumnData(
	Column eColumn
	)
{
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaColumnDatas); ++uIndex)
	{
		if (gaColumnDatas[uIndex].eColumn == eColumn)
		{
			return gaColumnDatas + uIndex;
		}
	}

	wxASSERT(eColumn != vvStatusControl::COLUMN_LAST);
	return vvStatusControl::GetColumnData(vvStatusControl::COLUMN_LAST);
}

vvStatusControl::vvStatusControl()
	: mpDataModel(NULL)
	, meHierarchyMode(MODE_COLLAPSED)
{

}

vvStatusControl::vvStatusControl(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	Mode               eMode,
	unsigned int       uItemTypes,
	unsigned int       uStatuses,
	unsigned int       uDisabledStatuses,
	unsigned int       uColumns,
	const wxValidator& cValidator
	)
{
	this->Create(pParent, cId, cPosition, cSize, iStyle, eMode, uItemTypes, uStatuses, uDisabledStatuses, uColumns, cValidator);
}

bool vvStatusControl::Create(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	Mode               eMode,
	unsigned int       uItemTypes,
	unsigned int       uStatuses,
	unsigned int       uDisabledStatuses,
	unsigned int       uColumns,
	const wxValidator& cValidator
	)
{
	vvFlags cStyle = iStyle;

	// create our parent
	if (!wxDataViewCtrl::Create(pParent, cId, cPosition, cSize, iStyle, cValidator))
	{
		return false;
	}

	// base class settings
	this->SetIndent(12);

	// create our model
	this->mpDataModel = new vvStatusModel(eMode, uItemTypes, uStatuses, uDisabledStatuses);
	this->AssociateModel(this->mpDataModel.get());

	// create our columns
	for (unsigned int uColumn = 1u; uColumn != COLUMN_LAST; uColumn <<= 1u)
	{
		// get data about the current column
		Column                     eColumn       = static_cast<Column>(uColumn);
		const ColumnData*          pPublicData   = vvStatusControl::GetColumnData(eColumn);
		const _InternalColumnData* pInternalData = _GetInternalColumnData(eColumn, iStyle);
		wxASSERT(pPublicData->eColumn != COLUMN_LAST);
		wxASSERT(pInternalData->eColumn != COLUMN_LAST);
		wxASSERT(pInternalData->eModelColumn != vvStatusModel::COLUMN_COUNT);


		// decide which type of renderer to use and create one
		wxString            sVariantType = this->mpDataModel->GetColumnType(pInternalData->eModelColumn);
		wxDataViewRenderer* pRenderer    = NULL;
		if (sVariantType == "vvDataViewToggleIconText")
		{
			pRenderer = new vvDataViewToggleIconTextRenderer(sVariantType, pInternalData->eCellMode, pInternalData->iAlignment_Column);
		}
		else if (sVariantType == "wxDataViewIconText")
		{
			pRenderer = new wxDataViewIconTextRenderer(sVariantType, pInternalData->eCellMode, pInternalData->iAlignment_Column);
		}
		else if (sVariantType == "string")
		{
			pRenderer = new wxDataViewTextRenderer(sVariantType, pInternalData->eCellMode, pInternalData->iAlignment_Column);
		}
		else if (sVariantType == "bool")
		{
			pRenderer = new wxDataViewToggleRenderer(sVariantType, pInternalData->eCellMode, pInternalData->iAlignment_Column);
		}

		// create the column
		wxDataViewColumn* pColumn = new wxDataViewColumn(pPublicData->szName, pRenderer, pInternalData->eModelColumn, pInternalData->iWidth_Start, pInternalData->eAlignment_Header, pInternalData->iFlags);
		pColumn->SetMinWidth(pInternalData->iWidth_Minimum);

		// keep track of the column in our map
		this->mcColumns[eColumn] = pColumn;

		// add the column to our display
		this->AppendColumn(pColumn);
	}

	// set the filename column to be the expander
	this->SetExpanderColumn(this->mcColumns[COLUMN_FILENAME]);

	// set the column visibility appropriately
	this->SetColumns(uColumns);

	// store settings
	if (eMode == MODE_FOLDER || eMode == MODE_COLLAPSED)
	{
		this->meHierarchyMode = eMode;
	}

	this->SetMinSize(wxSize(-1, 200));
	this->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(vvStatusControl::OnContext), NULL, this);

	// done
	return true;
}

void vvStatusControl::OnContext(wxDataViewEvent& event)
{
	event;
	event.GetItem();
	{
		{
			wxMenu menu;
			wxDataViewItem item = GetSelection();
			
			if (item.IsOk())
			{
				bool bWorkingFolderDiff = m_sRevision2.IsEmpty();
				wxVariant cValue;
				this->mpDataModel->GetValue(cValue, item, vvStatusModel::COLUMN_E_TYPE);
				ItemType type = static_cast<ItemType>(cValue.GetInteger());
				this->mpDataModel->GetValue(cValue, item, vvStatusModel::COLUMN_E_STATUS);
				unsigned int status = static_cast<unsigned int>(cValue.GetInteger());
				bool bJustAddedSeparator = false;
				if (type != ITEM_CONTAINER)
				{
					
					if (type != ITEM_FILE && type != ITEM_CHANGE && type != ITEM_CONFLICT && type != ITEM_FOLDER && type != ITEM_LOCK)
						return;  //Nothing to display for anything that isn't a file, a change, a conflict, or folder.

					vvFlags cRevertable(STATUS_ADDED | STATUS_DELETED | STATUS_MODIFIED | STATUS_RENAMED | STATUS_MOVED | STATUS_LOST | STATUS_LOCK_CONFLICT);
					vvFlags cDiffable(STATUS_ADDED | STATUS_DELETED | STATUS_MODIFIED);
					vvFlags cDeleteable(STATUS_LOST);
					vvFlags cUnlockable(STATUS_LOCKED);
					vvFlags cDiskDeleteable(STATUS_FOUND | STATUS_IGNORED);
					vvFlags cAddable(STATUS_FOUND | STATUS_IGNORED);
					vvFlags cResolvable(STATUS_CONFLICT | STATUS_RESOLVED);

					// Add the status of any of the item's CHANGE or CONFLICT
					// children to the status we'll be considering.
					wxDataViewItemArray cChildren;
					this->mpDataModel->GetChildren(item, cChildren);
					for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
					{
						wxVariant cValueSub;
						this->mpDataModel->GetValue(cValueSub, *it, vvStatusModel::COLUMN_E_TYPE);
						ItemType subtype = static_cast<ItemType>(cValueSub.GetInteger());
						if (subtype == ITEM_CHANGE || subtype == ITEM_CONFLICT)
						{
							this->mpDataModel->GetValue(cValueSub, *it, vvStatusModel::COLUMN_E_STATUS);
							unsigned int substatus = static_cast<unsigned int>(cValueSub.GetInteger());
							status |= substatus;
						}
					}

					if ((type == ITEM_FILE || type == ITEM_CHANGE ) && cDiffable.HasAnyFlag(status))
					{
						wxMenuItem *pItem = new wxMenuItem(&menu, ID_CONTEXT_DIFF, "&Diff");
						//This ended up looking really weird on my machine. -Jeremy
						//wxFont& font = pItem->GetFont();
						//font.SetWeight(wxFONTWEIGHT_BOLD);
						//pItem ->SetFont(font);
						menu.Append(pItem);
					}
					
					if (bWorkingFolderDiff && cUnlockable.HasAnyFlag(status))
					{
						menu.Append(ID_CONTEXT_UNLOCK, wxT("&Unlock..."));
					}

					if (type == ITEM_CONFLICT && cResolvable.HasAnyFlag(status))
					{
						menu.Append(ID_CONTEXT_RESOLVE, wxT("Re&solve..."));
					}
					if (bWorkingFolderDiff && cRevertable.HasAnyFlag(status))
						menu.Append(ID_CONTEXT_REVERT, wxT("Re&vert"), wxT(""));

					if (bWorkingFolderDiff && menu.GetMenuItemCount() != 0)
					{
						menu.AppendSeparator();
						bJustAddedSeparator = true;
					}

					if (bWorkingFolderDiff && cAddable.HasAnyFlag(status))
					{
						menu.Append(ID_CONTEXT_ADD, wxT("&Add"), wxT(""));
						bJustAddedSeparator = false;
					}
					
					if (bWorkingFolderDiff && cDeleteable.HasAnyFlag(status))
					{
						menu.Append(ID_CONTEXT_DELETE, wxT("De&lete"), wxT(""));
						bJustAddedSeparator = false;
					}
					else if (bWorkingFolderDiff && cDiskDeleteable.HasAnyFlag(status))
					{
						menu.Append(ID_CONTEXT_DISK_DELETE, wxT("De&lete"), wxT(""));
						bJustAddedSeparator = false;
					}
				}
				if (bWorkingFolderDiff && menu.GetMenuItemCount() != 0 && !bJustAddedSeparator)
					menu.AppendSeparator();
				if (bWorkingFolderDiff)
					menu.Append(ID_CONTEXT_REFRESH, wxT("&Refresh All"), wxT(""));
				PopupMenu(&menu);
			}
		}
	}
}

void vvStatusControl::LaunchDiffFromSelection()
{
	wxVariant variant;
		
	wxString sFileName = wxEmptyString;
	//Get the repo path from the status model
	this->GetModel()->GetValue(variant, GetSelection(), vvStatusModel::COLUMN_S_PATH);
	sFileName = variant.GetString();
	if (sFileName != wxEmptyString)
	{
		vvDiffCommand * pCommand = new vvDiffCommand();		
		pCommand->SetVerbs(m_pVerbs);
		wxArrayString cRepoPaths;
		cRepoPaths.Add(sFileName);
		pCommand->SetExecuteData(*m_pRepoLocator, sFileName, m_sRevision1, m_sRevision2); 
		//It is specifically OK to new this without explicitly 
		//deleting it.  The wxThread class always destructs itself
		//when the thread exits.
		vvDiffThread * bg = new vvDiffThread(&pCommand);
		bg;
			
	}
}
void vvStatusControl::OnItemSelected(wxDataViewEvent& event)
{
	event;
	wxDataViewItem item = GetSelection();
			
	if (item.IsOk())
	{
		wxVariant cValue;
		this->mpDataModel->GetValue(cValue, event.GetItem(), vvStatusModel::COLUMN_T_SELECTED);
		bool bCurrentlySelected = !cValue.IsNull() && cValue.GetBool();
		cValue = !bCurrentlySelected;
		this->mpDataModel->SetValue(cValue, event.GetItem(), vvStatusModel::COLUMN_T_SELECTED);
	}
}

void vvStatusControl::OnMouse(wxMouseEvent& event)
{
	event.Skip();
	if (event.LeftDClick())
	{
	}
}

void vvStatusControl::OnItemDoubleClick(wxDataViewEvent& event)
{
	event;
	wxDataViewItem item = GetSelection();
			
	if (item.IsOk())
	{
		vvNullable<vvStatusControl::ItemData> n_cItem = GetItemDataForSingleItem(item);
		if (!n_cItem.IsNull())
		{
			vvStatusControl::ItemData cData = n_cItem.GetValue();
			if (cData.eType == ITEM_FILE)
			{
				//Fire off a diff.
				LaunchDiffFromSelection();
			}
		}
	}
}

void vvStatusControl::OnContextMenuItem_Diff(
	wxCommandEvent& WXUNUSED(e)
	)
{
	LaunchDiffFromSelection();
}

void vvStatusControl::OnContextMenuItem_Resolve(
	wxCommandEvent& WXUNUSED(e)
	)
{
	wxVariant cValue;

	// get the repo path of the selected item
	this->GetModel()->GetValue(cValue, this->GetSelection(), vvStatusModel::COLUMN_S_PATH);
	wxString sRepoPath = cValue.GetString();
	if (sRepoPath.IsEmpty())
	{
		return;
	}

	// convert the repo path to an absolute one
	wxString sAbsolutePath;
	if (!this->m_pVerbs->GetAbsolutePath(*this->m_pContext, this->m_pRepoLocator->GetWorkingFolder(), sRepoPath, sAbsolutePath))
	{
		this->m_pContext->Error_ResetReport("Cannot convert repo path to absolute path: %s", sRepoPath);
		return;
	}

	// get the state/type of the conflict
	this->GetModel()->GetValue(cValue, this->GetSelection(), vvStatusModel::COLUMN_E_SUBTYPE);
	vvVerbs::ResolveChoiceState eState = static_cast<vvVerbs::ResolveChoiceState>(cValue.GetInteger());

	// show a resolve dialog for the item
	vvResolveDialog cDialog;
	{
		wxBusyCursor cCursor;
		if (!cDialog.Create(vvFindTopLevelParent(this), sAbsolutePath, eState, *this->m_pVerbs, *this->m_pContext))
		{
			return;
		}
	}
	cDialog.ShowModal();
	this->RefreshData();
}

void vvStatusControl::OnContextMenuItem_Unlock(
	wxCommandEvent& WXUNUSED(e)
	)
{
	//This is pretty crappy.  We should come up with a nicer way to 
	//do this.  We're calling the remove command in order to show 
	//the Remove dialog.
	vvLockCommand command = vvLockCommand();
	command.SetVerbs(m_pVerbs);
	command.SetContext(m_pContext);

	//Get the selection's repo path, then convert it to a disk path.
	wxArrayString cRepoPaths;
	wxVariant variant;
	wxString sFileName = wxEmptyString;
	this->GetModel()->GetValue(variant, GetSelection(), vvStatusModel::COLUMN_S_PATH);
	sFileName = variant.GetString();
	cRepoPaths.Add(sFileName);
	wxArrayString cDiskPaths;
	//Convert the repo paths to disk paths.
	m_pVerbs->GetAbsolutePaths(*m_pContext, m_pRepoLocator->GetWorkingFolder(), cRepoPaths, cDiskPaths);

	//Give the path to the command, then ask it to create a dialog
	command.ParseArguments(cDiskPaths);
	if (command.Init())
	{
		vvDialog * lockDialog = command.CreateDialog(vvFindTopLevelParent(this));
		if (lockDialog != NULL)
		{
			vvProgressExecutor exec = vvProgressExecutor(command);
			lockDialog->SetExecutor(&exec);
			//After this call the dialog will take care of executing the command.
			lockDialog->ShowModal();
			lockDialog->Destroy();
		}
	}
	this->RefreshData();
}



void vvStatusControl::OnContextMenuItem_Revert(
	wxCommandEvent& WXUNUSED(e)
	)
{
	wxVariant variant;
		
	wxString sFileName = wxEmptyString;
		
	//Get the repo path from the status model
	this->GetModel()->GetValue(variant, GetSelection(), vvStatusModel::COLUMN_S_PATH);
	sFileName = variant.GetString();
	if (sFileName != wxEmptyString)
	{
		//Get the user's setting for revert and backups.
		bool bSaveBackups = false;
		if (!this->m_pVerbs->GetSetting_Revert__Save_Backups(*m_pContext, &bSaveBackups))
		{
			m_pContext->Error_ResetReport("Could not get setting for revert");
			return;
		}
		vvRevertSingleDialog * rsd = new vvRevertSingleDialog();
		rsd->Create(this, sFileName, *this->m_pVerbs, *this->m_pContext);
		if (rsd->ShowModal() == wxID_OK)
		{
			vvRevertCommand command = vvRevertCommand();
			command.SetVerbs(m_pVerbs);
			wxArrayString cRepoPaths;
			cRepoPaths.Add(sFileName);
			command.SetExecuteData(*m_pRepoLocator, false, &cRepoPaths, rsd->GetSaveBackups()); 
			vvProgressExecutor(command).Execute(vvFindTopLevelParent(this));
			this->RefreshData();
		}
		delete rsd;
	}
}

void vvStatusControl::OnContextMenuItem_Add(
	wxCommandEvent& WXUNUSED(e)
	)
{
	// get the selected item
	wxDataViewItem cSelectedItem = this->GetSelection();
	wxVariant variant;

	// get the item's filename
	wxString sFileName = wxEmptyString;
	this->GetModel()->GetValue(variant, cSelectedItem, vvStatusModel::COLUMN_S_PATH);
	sFileName = variant.GetString();

	// run an add command on the filename
	wxArrayString cRepoPaths;
	cRepoPaths.Add(sFileName);
	vvAddCommand command = vvAddCommand();
	command.SetVerbs(m_pVerbs);
	command.SetExecuteData(m_pRepoLocator->GetWorkingFolder(), cRepoPaths); 
	vvProgressExecutor(command).Execute(vvFindTopLevelParent(this));

	// set the item as checked/selected
	variant = true;
	this->GetModel()->SetValue(variant, cSelectedItem, vvStatusModel::COLUMN_T_SELECTED);

	// let users/callers know about the update
	// Note: We're using the equivalent CONTROL's column here, not the MODEL's column.
	this->GetModel()->ValueChanged(cSelectedItem, vvStatusControl::COLUMN_FILENAME);

	// refresh the display
	this->RefreshData();
}

void vvStatusControl::OnContextMenuItem_Delete(
	wxCommandEvent& WXUNUSED(e)
	)
{
	//This is pretty crappy.  We should come up with a nicer way to 
	//do this.  We're calling the remove command in order to show 
	//the Remove dialog.
	vvRemoveCommand command = vvRemoveCommand();
	command.SetVerbs(m_pVerbs);
	command.SetContext(m_pContext);

	//Get the selection's repo path, then convert it to a disk path.
	wxArrayString cRepoPaths;
	wxVariant variant;
	wxString sFileName = wxEmptyString;
	this->GetModel()->GetValue(variant, GetSelection(), vvStatusModel::COLUMN_S_PATH);
	sFileName = variant.GetString();
	cRepoPaths.Add(sFileName);
	wxArrayString cDiskPaths;
	//Convert the repo paths to disk paths.
	m_pVerbs->GetAbsolutePaths(*m_pContext, m_pRepoLocator->GetWorkingFolder(), cRepoPaths, cDiskPaths);

	//Give the path to the command, then ask it to create a dialog
	command.ParseArguments(cDiskPaths);
	vvDialog * removeDialog = command.CreateDialog(vvFindTopLevelParent(this));
	if (removeDialog != NULL)
	{
		vvProgressExecutor exec = vvProgressExecutor(command);
		removeDialog->SetExecutor(&exec);
		//After this call the dialog will take care of executing the command.
		removeDialog->ShowModal();
		removeDialog->Destroy();
	}
	this->RefreshData();
}

void vvStatusControl::OnContextMenuItem_DiskDelete(
	wxCommandEvent& WXUNUSED(e)
	)
{
	wxVariant variant;
	wxString sFileName = wxEmptyString;
	this->GetModel()->GetValue(variant, GetSelection(), vvStatusModel::COLUMN_S_PATH);
	sFileName = variant.GetString();
	wxString sDiskPath;
	//Convert the repo paths to disk paths.
	if (!m_pVerbs->GetAbsolutePath(*m_pContext, m_pRepoLocator->GetWorkingFolder(), sFileName, sDiskPath))
	{
		m_pContext->Error_ResetReport("Couldn't get disk path for: " + sFileName);
		return;
	}
	if (!m_pVerbs->DeleteToRecycleBin(*m_pContext, sDiskPath, true))
	{
		m_pContext->Error_ResetReport("Couldn't send " + sDiskPath + " to the recycle bin");
		return;
	}
	this->RefreshData();
}

void vvStatusControl::OnContextMenuItem_Refresh(
	wxCommandEvent& WXUNUSED(e)
	)
{
	this->RefreshData();
}

void vvStatusControl::SetDisplay(
	Mode         eMode,
	unsigned int uItemTypes,
	unsigned int uStatuses
	)
{
	wxASSERT(this->mpDataModel != NULL);

	// if this is a hierarchical mode, store it as our current one
	if (eMode == MODE_FOLDER || eMode == MODE_COLLAPSED)
	{
		this->meHierarchyMode = eMode;
	}

	int iSort = this->SetSortColumnIndex(wxNOT_FOUND);
	this->mpDataModel->SetDisplay(eMode, uItemTypes, uStatuses);
	this->SetSortColumnIndex(iSort);
}

vvStatusControl::Mode vvStatusControl::GetDisplayMode() const
{
	wxASSERT(this->mpDataModel != NULL);
	return this->mpDataModel->GetDisplayMode();
}

vvStatusControl::Mode vvStatusControl::GetDisplayModeHierarchical() const
{
	return this->meHierarchyMode;
}

unsigned int vvStatusControl::GetDisplayTypes() const
{
	wxASSERT(this->mpDataModel != NULL);
	return this->mpDataModel->GetDisplayTypes();
}

unsigned int vvStatusControl::GetDisplayStatuses() const
{
	wxASSERT(this->mpDataModel != NULL);
	return this->mpDataModel->GetDisplayStatuses();
}

void vvStatusControl::SetColumns(
	unsigned int uColumns
	)
{
	vvFlags cColumns = uColumns;
	for (stlColumnMap::iterator it = this->mcColumns.begin(); it != this->mcColumns.end(); ++it)
	{
		it->second->SetHidden(!cColumns.HasAnyFlag(it->first));
	}
}

unsigned int vvStatusControl::GetColumns() const
{
	vvFlags cColumns = 0u;
	for (stlColumnMap::const_iterator it = this->mcColumns.begin(); it != this->mcColumns.end(); ++it)
	{
		if (!it->second->IsHidden())
		{
			cColumns.AddFlags(it->first);
		}
	}
	return cColumns.GetValue();
}

void vvStatusControl::RefreshData()
{
	wxBusyCursor cCursor;

	//Get the selection state.
	wxArrayString cSelectedPaths;
	vvStatusControl::stlItemDataList cStatusItems;
	this->GetItemData(&cStatusItems, vvStatusControl::COLUMN_FILENAME);
	bool bAll = true;
	for (vvStatusControl::stlItemDataList::const_iterator it = cStatusItems.begin(); it != cStatusItems.end(); ++it)
	{
		if (it->bSelected)
		{
			cSelectedPaths.Add(it->sRepoPath);
		}
		else
		{
			bAll = false;
		}
	}

	//GetExpanded does a treewalk as well, but it's much more efficient
	//because only container nodes can be expanded.
	wxDataViewItem cRoot1;
	wxArrayString cExpandedNodes;
	GetExpandedPathsRecursive(cRoot1, cExpandedNodes);
	
	//Get the path to the currently highlighted selection (not related to the checkbox on the item).
	wxVariant variant;
	this->GetModel()->GetValue(variant, GetSelection(), vvStatusModel::COLUMN_S_PATH);

	//TODO: It would be great if we could save and restore expansion.
	// The problem is that most of the folder nodes are "fake", in that they don't
	// have any diff item behind them.  This means that they don't have a COLUMN_S_PATH.

	this->FillData(*m_pVerbs, *m_pContext, *m_pRepoLocator, m_sRevision1, m_sRevision2);
	wxDataViewItem cRoot2;
	if (bAll || cSelectedPaths.Count() == 0)
	{
		this->SetItemSelectionRecursive(cRoot2, NULL, true, true, variant.GetString(), &cExpandedNodes);
	}
	else
	{
		this->SetItemSelectionRecursive(cRoot2, &cSelectedPaths, true, true, variant.GetString(), &cExpandedNodes);
	}

	this->GetModel()->ItemChanged(cRoot2);
//	this->ExpandAll();
}

bool vvStatusControl::FillData(
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const vvRepoLocator&	cRepoLocator,
	const wxString& sRevision,
	const wxString&  sRevision2
	)
{
	wxASSERT(this->mpDataModel != NULL);
	m_pVerbs = &cVerbs;
	m_pContext = &cContext;
	m_sRevision1 = sRevision;
	m_sRevision2 = sRevision2;
	m_pRepoLocator = &cRepoLocator;
	int iSort = this->SetSortColumnIndex(wxNOT_FOUND);
	bool bResult = this->mpDataModel->FillData(cVerbs, cContext, cRepoLocator, sRevision, sRevision2);
	//If they haven't sorted yet, sort by the first column.
	if (iSort == wxNOT_FOUND)
		this->SetSortColumnIndex(0);
	else
		this->SetSortColumnIndex(iSort);
	return bResult;
}

void vvStatusControl::ExpandAll()
{
	if (this->mpDataModel == NULL)
	{
		return;
	}

	wxDataViewItem cRoot;
	//The dataview will resort on expand, unless we turn
	//off sorting.
	int iSort = this->SetSortColumnIndex(wxNOT_FOUND);
	this->ExpandItemRecursive(cRoot);
	if (iSort == wxNOT_FOUND)
		this->SetSortColumnIndex(0);
	else
		this->SetSortColumnIndex(iSort);
}

unsigned int vvStatusControl::GetItemData(
	stlItemDataList* pItems,
	unsigned int     uColumns,
	vvNullable<bool> nbSelected,
	unsigned int     uItemTypes,
	unsigned int     uStatuses
	) const
{
	if (this->mpDataModel == NULL)
	{
		return 0u;
	}

	wxDataViewItem cRoot;
	return this->GetItemDataRecursive(pItems, cRoot, uColumns, nbSelected, uItemTypes, uStatuses);
}

vvNullable<vvStatusControl::ItemData> vvStatusControl::GetItemDataForSingleItem(
	wxDataViewItem& cItem,
	unsigned int     uColumns,
	vvNullable<bool> nbSelected,
	unsigned int     uItemTypes,
	unsigned int     uStatuses
	) const
{
	wxVariant cValue;
	ItemData     cData;
	vvFlags      cColumns = uColumns;
	vvFlags      cTypes   = uItemTypes;
	vvFlags      cStatus  = uStatuses;

	// if we have a valid (not the invisible root) item, process it
	if (cItem.IsOk())
	{
		// get the values we need to determine if we care about this item
		this->mpDataModel->GetValue(cValue, cItem, vvStatusModel::COLUMN_T_SELECTED);
		cData.bSelected = !cValue.IsNull() && cValue.GetBool();
		this->mpDataModel->GetValue(cValue, cItem, vvStatusModel::COLUMN_E_TYPE);
		cData.eType = static_cast<ItemType>(cValue.GetInteger());
		this->mpDataModel->GetValue(cValue, cItem, vvStatusModel::COLUMN_E_STATUS);
		cData.eStatus = static_cast<Status>(cValue.GetInteger());
		// if we care about this item, fill out the remainder of the requested data and add it to the list
		if (
			   cData.eType != ITEM_CONTAINER
			&& cData.eType != ITEM_CHANGE
			&& nbSelected.IsNullOrEqual(cData.bSelected)
			&& cTypes.HasAnyFlag(cData.eType)
			&& cStatus.HasAnyFlag(cData.eStatus)
			)
		{
			#define CHECK_COLUMN(ControlColumn, ModelColumn, DataField, VariantGet)     \
			if (cColumns.HasAnyFlag(ControlColumn))                                     \
			{                                                                           \
				this->mpDataModel->GetValue(cValue, cItem, vvStatusModel::ModelColumn); \
				cData.DataField = cValue.VariantGet();                                  \
			}
			CHECK_COLUMN(COLUMN_FILENAME,      COLUMN_S_PATH,          sRepoPath,     GetString);
			CHECK_COLUMN(COLUMN_FILENAME,      COLUMN_S_FILENAME,      sFilename,     GetString);
			CHECK_COLUMN(COLUMN_FROM,          COLUMN_S_FROM,          sFrom,         GetString);
//			CHECK_COLUMN(COLUMN_ATTRIBUTES,    COLUMN_B_ATTRIBUTES,    bAttributes,   GetBool);
			CHECK_COLUMN(COLUMN_GID,           COLUMN_S_GID,           sGid,          GetString);
			CHECK_COLUMN(COLUMN_HID_NEW,       COLUMN_S_HID_NEW,       sHidNew,       GetString);
			CHECK_COLUMN(COLUMN_HID_OLD,       COLUMN_S_HID_OLD,       sHidOld,       GetString);
			#undef CHECK_COLUMN

			return cData;
		}
	}
	return vvNULL;
}

unsigned int vvStatusControl::SetItemSelectionAll(
	bool bSelected
	)
{
	wxDataViewItem cRoot;
	return this->SetItemSelectionRecursive(cRoot, NULL, true, bSelected);
}

unsigned int vvStatusControl::SetItemSelectionByPath(
	const wxString& sPath,
	bool            bSelected
	)
{
	wxArrayString cPaths;
	cPaths.Add(sPath);
	wxDataViewItem cRoot;
	return this->SetItemSelectionRecursive(cRoot, &cPaths, true, bSelected);
}

unsigned int vvStatusControl::SetItemSelectionByPath(
	const wxArrayString& cPaths,
	bool                 bSelected
	)
{
	wxDataViewItem cRoot;
	return this->SetItemSelectionRecursive(cRoot, &cPaths, true, bSelected);
}

unsigned int vvStatusControl::SetItemSelectionByGid(
	const wxString& sGid,
	bool            bSelected
	)
{
	wxArrayString cGids;
	cGids.Add(sGid);
	wxDataViewItem cRoot;
	return this->SetItemSelectionRecursive(cRoot, &cGids, false, bSelected);
}

unsigned int vvStatusControl::SetItemSelectionByGid(
	const wxArrayString& cGids,
	bool                 bSelected
	)
{
	wxDataViewItem cRoot;
	return this->SetItemSelectionRecursive(cRoot, &cGids, false, bSelected);
}

void vvStatusControl::OnColumnHeaderClick(
	wxDataViewEvent& cEvent
	)
{
	// the default handling of this event is to sort, which we still want to occur
	// so allow default processing of the event to continue
	cEvent.Skip();

	// change our model's mode so that sorting produces expected results
	vvStatusModel::Column eColumn = static_cast<vvStatusModel::Column>(cEvent.GetDataViewColumn()->GetModelColumn());
	switch (eColumn)
	{
	// if we're sorting by path/filename, then use collapsed mode
	case vvStatusModel::COLUMN_I_FILENAME:
	case vvStatusModel::COLUMN_IS_FILENAME:
	case vvStatusModel::COLUMN_IST_FILENAME:
	case vvStatusModel::COLUMN_S_FILENAME:
	{
		this->SetDisplay(this->meHierarchyMode, this->GetDisplayTypes(), this->GetDisplayStatuses());
		this->ExpandAll();
		break;
	}

	// otherwise use a flat display
	default:
		this->SetDisplay(MODE_FLAT, this->GetDisplayTypes(), this->GetDisplayStatuses());
		break;
	}
}

void vvStatusControl::ExpandItemRecursive(
	wxDataViewItem& cItem
	)
{
	wxASSERT(this->mpDataModel != NULL);

	if (this->mpDataModel->IsContainer(cItem))
	{
		this->Expand(cItem);
	}

	wxDataViewItemArray cChildren;
	this->mpDataModel->GetChildren(cItem, cChildren);
	for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
	{
		this->ExpandItemRecursive(*it);
	}
}


void vvStatusControl::GetExpandedPathsRecursive(
	wxDataViewItem&  cItem,
	wxArrayString& cOutputArray
	) const
{
	if (cItem.IsOk())
	{
		if (this->IsExpanded(cItem))
		{
			wxVariant cVariant;
			this->mpDataModel->GetValue(cVariant, cItem, vvStatusModel::COLUMN_S_PATH);
			if (!cVariant.GetString().IsEmpty())
				cOutputArray.Add(cVariant.GetString());
		}
	}
	// run through the item's children and process them as well
	wxDataViewItemArray cChildren;
	this->mpDataModel->GetChildren(cItem, cChildren);
	for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
	{
		if (this->mpDataModel->IsContainer(*it))
			this->GetExpandedPathsRecursive(*it, cOutputArray);
	}
}


unsigned int vvStatusControl::GetItemDataRecursive(
	stlItemDataList* pItems,
	wxDataViewItem&  cItem,
	unsigned int     uColumns,
	vvNullable<bool> nbSelected,
	unsigned int     uItemTypes,
	unsigned int     uStatuses
	) const
{
	unsigned int uCount   = 0u;

	if (cItem.IsOk())
	{
		vvNullable<vvStatusControl::ItemData> n_cItem = GetItemDataForSingleItem(cItem, uColumns, nbSelected, uItemTypes, uStatuses);
		if (!n_cItem.IsNull())
		{
			if (pItems != NULL)
			{
				pItems->push_back(n_cItem.GetValue());
			}
			if (!this->mpDataModel->IsPermanentlyDisabled(cItem))
				++uCount;
			//If this is a selected folder, and they requested selected folders, 
			//(but not all the files and folders under the selected folder)
			if (n_cItem.GetValue().bSelected && n_cItem.GetValue().eType == ITEM_FOLDER &&
				((uItemTypes & ITEM_FOLDER__DONT_RECURSE_IF_SELECTED) != 0) )
			{
				return uCount;
			}
		}
	}
	
	// run through the item's children and process them as well
	wxDataViewItemArray cChildren;
	this->mpDataModel->GetChildren(cItem, cChildren);
	for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
	{
		uCount += this->GetItemDataRecursive(pItems, *it, uColumns, nbSelected, uItemTypes, uStatuses);
	}

	// return the number of items we added
	return uCount;
}

unsigned int vvStatusControl::SetItemSelectionRecursive(
	wxDataViewItem&      cItem,
	const wxArrayString* pItems,
	bool                 bPaths,
	bool                 bSelected, 
	const wxString&	     sPathToFocus,
	const wxArrayString* pExpandedNodes
	)
{
	unsigned int uCount = 0u;
	//Used to skip reselecting paths if we've already found a match.
	bool bHaveAlreadyDoneFocus = false;
	bool bMatch = false;

	// if we have an empty list of items, nothing will be selected
	if (pItems != NULL && pItems->size() == 0u)
	{
		return uCount;
	}

	// if we have a valid (not the invisible root) item, process it
	if (cItem.IsOk())
	{
		// prepare variables appropriately
		vvStatusModel::Column eColumn        = vvStatusModel::COLUMN_S_GID;
		bool                  bCaseSensitive = false;
		if (bPaths)
		{
			eColumn        = vvStatusModel::COLUMN_S_PATH;
			bCaseSensitive = wxFileName::IsCaseSensitive();
		}

		// check if this item matches one of the given ones
		wxVariant cValue;
		this->mpDataModel->GetValue(cValue, cItem, eColumn);
		wxString sValue = cValue.GetString();

		bool permanentDisable = this->mpDataModel->IsPermanentlyDisabled(cItem);

		//Only select the node if it's not permanently disabled.
		bMatch = !permanentDisable && (pItems == NULL || pItems->Index(sValue, bCaseSensitive) != wxNOT_FOUND);

		// if we didn't find a path and our item is a folder
		// then we should also try removing any trailing path separators it contains
		if (pItems != NULL && bPaths && !bMatch)
		{
			// check if the current item is a folder
			this->mpDataModel->GetValue(cValue, cItem, vvStatusModel::COLUMN_E_TYPE);
			if (cValue.GetInteger() == vvStatusControl::ITEM_FOLDER)
			{
				// loop over each path separator used by the native system
				// (also stop the loop if we find a match)
				wxString sSeparators = wxFileName::GetPathSeparators();
				for (wxString::iterator it = sSeparators.begin(); it != sSeparators.end() && !bMatch; ++it)
				{
					// if the path ends with this separator, remove it
					wxString sWithoutSeparator = wxEmptyString;
					if (sValue.EndsWith(*it, &sWithoutSeparator))
					{
						// check if the trimmed path is found among the given paths
						wxASSERT(pItems != NULL); // we're inside "if !bMatch", and bMatch can only be false if pItems is non-NULL
						bMatch = pItems->Index(sWithoutSeparator, bCaseSensitive) != wxNOT_FOUND;
					}
				}
			}
		}

		//Check to see if we need to expand the node.
		if (pExpandedNodes != NULL && this->mpDataModel->IsContainer(cItem))
		{
			bool bMatch = pExpandedNodes->Index(sValue, bCaseSensitive) != wxNOT_FOUND;
			if (bMatch)
				this->Expand(cItem);
		}

		//If they requested that a particular path get the Focus Highlight, do that.
		if (!sPathToFocus.IsEmpty() && sPathToFocus.CompareTo(sValue, bCaseSensitive ? wxString::exact : wxString::ignoreCase) == 0)
		{
			this->Select(cItem);
			bHaveAlreadyDoneFocus = true;
		}

		// if the item matched one of the given ones, set its selection state accordingly
		if (bMatch)
		{
			cValue = bSelected;
			this->mpDataModel->SetValue(cValue, cItem, vvStatusModel::COLUMN_T_SELECTED);
			++uCount;
		}
	}

	// run through the item's children and process them as well
	wxDataViewItemArray cChildren;
	this->mpDataModel->GetChildren(cItem, cChildren);
	for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
	{
		//If this folder matched, then everything below it must match as well.
		//Pass NULL down for pItems.
		uCount += this->SetItemSelectionRecursive(*it, bMatch ? NULL : pItems, bPaths, bSelected, bHaveAlreadyDoneFocus ? wxEmptyString : sPathToFocus, pExpandedNodes);
	}

	// done
	return uCount;
}

int vvStatusControl::SetSortColumnIndex(
	int iIndex
	)
{
	// unset the current sort
	int iCurrent = this->GetSortingColumnIndex();
	if (iCurrent != wxNOT_FOUND)
	{
		this->GetColumn(iCurrent)->UnsetAsSortKey();
		this->OnColumnChange(iCurrent);
		this->SetSortingColumnIndex(wxNOT_FOUND);
	}

	// set the requested sort
	this->SetSortingColumnIndex(iIndex);
	if (iIndex != wxNOT_FOUND)
	{
		this->GetColumn(iIndex)->SetAsSortKey();
		if (iIndex != iCurrent)
			this->GetModel()->Resort();
	}

	// return the old sort
	return iCurrent;
}

IMPLEMENT_DYNAMIC_CLASS(vvStatusControl, wxDataViewCtrl);

BEGIN_EVENT_TABLE(vvStatusControl, wxDataViewCtrl)
EVT_DATAVIEW_COLUMN_HEADER_CLICK(wxID_ANY, OnColumnHeaderClick)
EVT_MENU(ID_CONTEXT_DIFF, vvStatusControl::OnContextMenuItem_Diff)
EVT_MENU(ID_CONTEXT_RESOLVE, vvStatusControl::OnContextMenuItem_Resolve)
EVT_MENU(ID_CONTEXT_REVERT, vvStatusControl::OnContextMenuItem_Revert)
EVT_MENU(ID_CONTEXT_UNLOCK, vvStatusControl::OnContextMenuItem_Unlock)
EVT_MENU(ID_CONTEXT_ADD, vvStatusControl::OnContextMenuItem_Add)
EVT_MENU(ID_CONTEXT_DELETE, vvStatusControl::OnContextMenuItem_Delete)
EVT_MENU(ID_CONTEXT_DISK_DELETE, vvStatusControl::OnContextMenuItem_DiskDelete)
EVT_MENU(ID_CONTEXT_REFRESH, vvStatusControl::OnContextMenuItem_Refresh)
EVT_MOUSE_EVENTS(vvStatusControl::OnMouse)
EVT_DATAVIEW_ITEM_ACTIVATED( wxID_ANY, vvStatusControl::OnItemDoubleClick)
//EVT_DATAVIEW_SELECTION_CHANGED( wxID_ANY, vvStatusControl::OnItemSelected)
END_EVENT_TABLE()
