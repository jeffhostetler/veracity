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
#include "vvStampsControl.h"

#include "vvCppUtilities.h"
#include "vvDataViewToggleIconTextRenderer.h"
#include "vvNullable.h"
#include "vvVerbs.h"


/*
**
** Types
**
*/

/**
 * Models data about the stamps in a repository.
 */
class vvStampsModel : public wxDataViewModel
{
// types
public:
	/**
	 * Enumeration of columns used by the model.
	 *
	 * The B/S/U prefixes indicate the value type of the column (Bool, String, Unsigned).
	 * The prefix codes are always listed alphabetically, if it helps you remember the names.
	 *
	 * B and S values use variant types "bool" and "string" respectively.
	 * U values use variant type "long", but are actually cast unsigned ints, and should be cast back to unsigned int before use.
	 */
	enum Column
	{
		COLUMN_B_CHECKED, //< Item's checked state.
		COLUMN_BS_NAME,   //< Item's name and checked state.
		COLUMN_S_NAME,    //< Item's name.
		COLUMN_U_COUNT,   //< Item's use count.
		COLUMN_COUNT,     //< Number of entries in this enumeration, for iteration.
	};

// construction
public:
	/**
	 * Default constructor.
	 */
	vvStampsModel();

// functionality
public:
	/**
	 * Adds items to the model for every stamp in a given repo.
	 * Returns true if successful or false if an error occurred.
	 */
	bool AddItems(
		vvVerbs&             cVerbs,           //< [in] The vvVerbs implementation to retrieve data from.
		vvContext&           cContext,         //< [in] The context to use with the verbs.
		const wxString&      sRepo,            //< [in] The repo to get stamp data from.
		bool                 bChecked = false, //< [in] The initial selection state of the new items.
		wxDataViewItemArray* pItems   = NULL   //< [out] The added items.
		);

	/**
	 * Adds a new item to the data model as long as no existing item has the same name.
	 * Returns the new item, or a NULL item if it already existed.
	 */
	wxDataViewItem AddItem(
		const wxString& sName,           //< [in] The name of the item to add.
		unsigned int    uCount   = 0u,   //< [in] The use count of the new item.
		bool            bChecked = false //< [in] The initial selection state of the new item.
		);

	/**
	 * Retrieves a data item by name.
	 * Returns a NULL item if the named item isn't in the model.
	 */
	wxDataViewItem FindItem(
		const wxString& sName //< [in] The name to retrieve the item for.
		) const;

	/**
	 * Removes all items from the data model.
	 */
	void Clear();

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
	 * A single data item being represented by the model.
	 */
	struct DataItem
	{
		wxString     sName;    //< Name of the stamp.
		unsigned int uCount;   //< Number of times the stamp has been used.
		bool         bChecked; //< Whether or not the item is currently checked.
	};

	/**
	 * A map of DataItems indexed by their name.
	 */
	typedef std::map<wxString, DataItem> stlDataItemMap;

// private functionality
private:
	/**
	 * Converts an internal data item to a wxDataViewItem.
	 */
	wxDataViewItem vvStampsModel::ConvertDataItem(
		stlDataItemMap::const_iterator it
		) const;

	/**
	 * Converts an internal data item to a wxDataViewItem.
	 */
	wxDataViewItem ConvertDataItem(
		const DataItem& cItem //< [in] The item to convert.
		) const;

	/**
	 * Converts a wxDataViewItem back into an internal data item.
	 */
	const DataItem* ConvertDataItem(
		const wxDataViewItem& cItem //< [in] The item to convert.
		) const;

	/**
	 * Converts a wxDataViewItem back into an internal data item.
	 */
	DataItem* ConvertDataItem(
		const wxDataViewItem& cItem //< [in] The item to convert.
		);

// private data
private:
	stlDataItemMap mcDataItems; //< The data items that we're displaying.

// macro declarations
private:
	vvDISABLE_COPY(vvStampsModel);
};


/*
**
** Globals
**
*/

static const char* gszColumn_Checked = "";
static const char* gszColumn_Name    = "Name";
static const char* gszColumn_Count   = "Count";


/*
**
** Internal Functions
**
*/

vvStampsModel::vvStampsModel()
{
}

bool vvStampsModel::AddItems(
	vvVerbs&             cVerbs,
	vvContext&           cContext,
	const wxString&      sRepo,
	bool                 bChecked,
	wxDataViewItemArray* pItems
	)
{
	// get stamps data
	vvVerbs::stlStampDataList cStamps;
	if (!cVerbs.GetStamps(cContext, sRepo, cStamps))
	{
		return false;
	}

	// add each of the retrieved items
	for (vvVerbs::stlStampDataList::iterator it = cStamps.begin(); it != cStamps.end(); ++it)
	{
		wxDataViewItem cItem = this->AddItem(it->sName, it->uCount, bChecked);
		if (pItems != NULL)
		{
			pItems->Add(cItem);
		}
	}

	// done
	return true;
}

wxDataViewItem vvStampsModel::AddItem(
	const wxString& sName,
	unsigned int    uCount,
	bool            bChecked
	)
{
	// check if this item already exists
	stlDataItemMap::iterator it = this->mcDataItems.find(sName);
	if (it != this->mcDataItems.end())
	{
		return wxDataViewItem();
	}

	// add the item
	DataItem cItem;
	cItem.sName    = sName;
	cItem.uCount   = uCount;
	cItem.bChecked = bChecked;
	it = this->mcDataItems.insert(it, stlDataItemMap::value_type(sName, cItem));
	
	// create a wxDataViewItem from it
	wxDataViewItem cViewItem = this->ConvertDataItem(it);

	// notify anyone that cares about the new item
	this->ItemAdded(wxDataViewItem(), cViewItem);

	// return the new item
	return cViewItem;
}

wxDataViewItem vvStampsModel::FindItem(
	const wxString& sName
	) const
{
	stlDataItemMap::const_iterator it = this->mcDataItems.find(sName);
	if (it == this->mcDataItems.end())
	{
		return wxDataViewItem();
	}
	else
	{
		return this->ConvertDataItem(it);
	}
}

void vvStampsModel::Clear()
{
	this->mcDataItems.clear();
}

bool vvStampsModel::IsContainer(
	const wxDataViewItem& cItem
	) const
{
	// only the invisible root is a container
	// none of our actual nodes are hierarchical
	return !cItem.IsOk();
}

bool vvStampsModel::HasContainerColumns(
	const wxDataViewItem& WXUNUSED(cItem)
	) const
{
	// shouldn't really matter what we return here
	// because we'll never display a container row
	return false;
}

wxDataViewItem vvStampsModel::GetParent(
	const wxDataViewItem& WXUNUSED(cItem)
	) const
{
	// every item's parent is the invisible root
	return wxDataViewItem(NULL);
}

unsigned int vvStampsModel::GetChildren(
	const wxDataViewItem& cItem,
	wxDataViewItemArray&  cChildren
	) const
{
	// if this isn't the invisible root, it has no children
	if (cItem.IsOk())
	{
		return 0u;
	}

	// return a list of all of our items
	for (stlDataItemMap::const_iterator it = this->mcDataItems.begin(); it != this->mcDataItems.end(); ++it)
	{
		cChildren.Add(this->ConvertDataItem(it));
	}
	return this->mcDataItems.size();
}

unsigned int vvStampsModel::GetColumnCount() const
{
	return vvStampsModel::COLUMN_COUNT;
}

wxString vvStampsModel::GetColumnType(
	unsigned int uColumn
	) const
{
	switch (static_cast<Column>(uColumn))
	{
	case COLUMN_B_CHECKED:
		return "bool";

	case COLUMN_BS_NAME:
	case COLUMN_S_NAME:
		return "string";

	case COLUMN_U_COUNT:
		return "long";

	default:
		wxLogError("Unknown StampsModel column: %u", uColumn);
		return wxEmptyString;
	}
}

void vvStampsModel::GetValue(
	wxVariant&            cValue,
	const wxDataViewItem& cItem,
	unsigned int          uColumn
	) const
{
	wxASSERT(cItem.IsOk());

	const DataItem* pItem = this->ConvertDataItem(cItem);

	switch (static_cast<Column>(uColumn))
	{
	case COLUMN_B_CHECKED:
		cValue = pItem->bChecked;
		return;

	case COLUMN_BS_NAME:
	case COLUMN_S_NAME:
		cValue = pItem->sName;
		return;

	case COLUMN_U_COUNT:
		cValue = static_cast<long>(pItem->uCount);
		return;

	default:
		wxLogError("Unknown StampsModel column: %u", uColumn);
		cValue.MakeNull();
		return;
	}
}

bool vvStampsModel::SetValue(
	const wxVariant&      cValue,
	const wxDataViewItem& cItem,
	unsigned int          uColumn
	)
{
	wxASSERT(cItem.IsOk());

	DataItem* pItem   = this->ConvertDataItem(cItem);
	bool      bChanged = false;

	switch (static_cast<Column>(uColumn))
	{
	case COLUMN_B_CHECKED:
		pItem->bChecked = cValue.GetBool();
		bChanged        = true;
		break;

	case COLUMN_BS_NAME:
	case COLUMN_S_NAME:
	case COLUMN_U_COUNT:
		wxLogError("Cannot set values in StampsModel column: %u", uColumn);
		bChanged = false;
		break;

	default:
		wxLogError("Unknown StampsModel column: %u", uColumn);
		bChanged = false;
		break;
	}

	if (bChanged)
	{
		this->ItemChanged(cItem);
	}
	return bChanged;
}

int vvStampsModel::Compare(
	const wxDataViewItem& cLeft,
	const wxDataViewItem& cRight,
	unsigned int          uColumn,
	bool                  bAscending
	) const
{
	const DataItem* pLeft   = this->ConvertDataItem(cLeft);
	const DataItem* pRight  = this->ConvertDataItem(cRight);

	// if we're in descending order, swap the left and right
	if (!bAscending)
	{
		const DataItem* pTemp = pLeft;
		pLeft = pRight;
		pRight = pTemp;
	}

	switch (static_cast<Column>(uColumn))
	{
	case COLUMN_B_CHECKED:
		if (pLeft->bChecked == pRight->bChecked)
		{
			return this->Compare(cLeft, cRight, COLUMN_S_NAME, bAscending);
		}
		else if (pLeft->bChecked < pRight->bChecked)
		{
			return -1;
		}
		else
		{
			return 1;
		}

	case COLUMN_BS_NAME:
	case COLUMN_S_NAME:
		return pLeft->sName.CompareTo(pRight->sName);

	case COLUMN_U_COUNT:
		if (pLeft->uCount == pRight->uCount)
		{
			return this->Compare(cLeft, cRight, COLUMN_S_NAME, bAscending);
		}
		else if (pLeft->uCount < pRight->uCount)
		{
			return -1;
		}
		else
		{
			return 1;
		}

	default:
		wxLogError("Unknown StampsModel column: %u", uColumn);
		return false;
	}
}

wxDataViewItem vvStampsModel::ConvertDataItem(
	stlDataItemMap::const_iterator it
	) const
{
	return this->ConvertDataItem(it->second);
}

wxDataViewItem vvStampsModel::ConvertDataItem(
	const DataItem& cItem
	) const
{
	return wxDataViewItem(static_cast<void*>(const_cast<DataItem*>(&cItem)));
}

const vvStampsModel::DataItem* vvStampsModel::ConvertDataItem(
	const wxDataViewItem& cItem
	) const
{
	return static_cast<const DataItem*>(cItem.GetID());
}

vvStampsModel::DataItem* vvStampsModel::ConvertDataItem(
	const wxDataViewItem& cItem
	)
{
	return static_cast<DataItem*>(cItem.GetID());
}


/*
**
** Public Functions
**
*/

vvStampsControl::vvStampsControl()
	: mpDataModel(NULL)
{
}

vvStampsControl::vvStampsControl(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	)
{
	this->Create(pParent, cId, cPosition, cSize, iStyle, cValidator);
}

bool vvStampsControl::Create(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	)
{
	vvFlags cStyle = iStyle;

	// create our parent
	if (!wxDataViewCtrl::Create(pParent, cId, cPosition, cSize, iStyle, cValidator))
	{
		return false;
	}

	// create our model
	this->mpDataModel = new vvStampsModel();
	this->AssociateModel(this->mpDataModel.get());

	// create our columns
	wxDataViewColumn* pColumn = NULL;

	// dummy expander column
	// This is a dummy column that will always be hidden.
	// It only exists to be the expander column so that no visible columns have to be.
	// The expander column has a built-in left margin for expanders.
	// Since our data isn't hierarchical, we'd rather just not have the margin.
	pColumn = new wxDataViewColumn(wxEmptyString, new wxDataViewToggleRenderer(), vvStampsModel::COLUMN_B_CHECKED);
	pColumn->SetHidden(true);
	this->AppendColumn(pColumn);
	this->SetExpanderColumn(pColumn);

	// checked column
	pColumn = new wxDataViewColumn(gszColumn_Checked, new vvDataViewToggleIconTextRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE, wxALIGN_CENTER), vvStampsModel::COLUMN_B_CHECKED, 25, wxALIGN_CENTER, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
	pColumn->SetMinWidth(25);
	if (!cStyle.HasAnyFlag(STYLE_CHECKABLE))
	{
		pColumn->SetHidden(true);
	}
	this->AppendColumn(pColumn);

	// name column
	pColumn = new wxDataViewColumn(gszColumn_Name, new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_INERT, wxALIGN_LEFT), vvStampsModel::COLUMN_S_NAME, 150, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
	pColumn->SetMinWidth(50);
	this->SetExpanderColumn(pColumn);
	this->AppendColumn(pColumn);

	// count column
	pColumn = new wxDataViewColumn(gszColumn_Count, new wxDataViewTextRenderer("ulonglong", wxDATAVIEW_CELL_INERT, wxALIGN_RIGHT), vvStampsModel::COLUMN_U_COUNT, 50, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
	pColumn->SetMinWidth(25);
	this->AppendColumn(pColumn);

	// bind our events
//	this->Bind(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, &vvStampsControl::OnItemActivated, this);

	// done
	return true;
}

bool vvStampsControl::AddItems(
	vvVerbs&             cVerbs,
	vvContext&           cContext,
	const wxString&      sRepo,
	bool                 bChecked,
	wxDataViewItemArray* pItems
	)
{
	wxASSERT(this->mpDataModel != NULL);
	return this->mpDataModel->AddItems(cVerbs, cContext, sRepo, bChecked, pItems);
}

wxDataViewItem vvStampsControl::AddItem(
	const wxString& sName,
	unsigned int    uCount,
	bool            bChecked
	)
{
	return this->mpDataModel->AddItem(sName, uCount, bChecked);
}

wxDataViewItem vvStampsControl::GetItemByName(
	const wxString& sName
	)
{
	return this->mpDataModel->FindItem(sName);
}

unsigned int vvStampsControl::GetItemData(
	stlItemDataList* pItems,
	vvNullable<bool> nbChecked
	) const
{
	if (this->mpDataModel == NULL)
	{
		return 0u;
	}

	// run through all the items in the control
	unsigned int uCount = 0u;
	wxDataViewItemArray cChildren;
	this->mpDataModel->GetChildren(wxDataViewItem(), cChildren);
	for (wxDataViewItemArray::iterator it = cChildren.begin(); it != cChildren.end(); ++it)
	{
		// get the item's data
		ItemData cData;
		this->GetItemData(*it, cData);

		// check if the item matches our criteria
		if (nbChecked.IsNull() || nbChecked.GetValue() == cData.bChecked)
		{
			if (pItems != NULL)
			{
				pItems->push_back(cData);
			}
			++uCount;
		}
	}

	return uCount;
}

void vvStampsControl::GetItemData(
	const wxDataViewItem& cItem,
	ItemData&             cData
	) const
{
	wxVariant cValue;

	this->mpDataModel->GetValue(cValue, cItem, vvStampsModel::COLUMN_B_CHECKED);
	cData.bChecked = cValue.GetBool();
	this->mpDataModel->GetValue(cValue, cItem, vvStampsModel::COLUMN_S_NAME);
	cData.sName = cValue.GetString();
	this->mpDataModel->GetValue(cValue, cItem, vvStampsModel::COLUMN_U_COUNT);
	cData.uCount = static_cast<unsigned int>(cValue.GetLong());
}

void vvStampsControl::CheckItem(
	const wxDataViewItem& cItem,
	vvNullable<bool>      nbChecked
	)
{
	if (nbChecked.IsValid())
	{
		this->mpDataModel->SetValue(nbChecked.GetValue(), cItem, vvStampsModel::COLUMN_B_CHECKED);
	}
	else
	{
		wxVariant cValue;
		this->mpDataModel->GetValue(cValue, cItem, vvStampsModel::COLUMN_B_CHECKED);
		cValue = !cValue.GetBool();
		this->mpDataModel->SetValue(cValue, cItem, vvStampsModel::COLUMN_B_CHECKED);
	}
}

void vvStampsControl::OnItemActivated(
	wxDataViewEvent& cEvent
	)
{
	this->CheckItem(cEvent.GetItem(), vvNULL);
}

IMPLEMENT_DYNAMIC_CLASS(vvStampsControl, wxDataViewCtrl);

BEGIN_EVENT_TABLE(vvStampsControl, wxDataViewCtrl)
	EVT_DATAVIEW_ITEM_ACTIVATED(wxID_ANY, OnItemActivated)
END_EVENT_TABLE()
