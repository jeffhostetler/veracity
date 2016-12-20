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

#ifndef VV_WorkItems_CONTROL_H
#define VV_WorkItems_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"
#include "vvNullable.h"
#include "vvVerbs.h"

/**
 * A custom wxDataViewCtrl for displaying and choosing WorkItems.
 */
class vvWorkItemsControl : public wxDataViewCtrl
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
		wxString sID;           
		wxString sRecID;
		wxString sStatus;
		int nStatus;
		wxString sTitle;
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
	vvWorkItemsControl();

	/**
	 * Create contructor.
	 */
	vvWorkItemsControl(
		wxWindow*          pParent,
		wxWindowID         cId,
		class vvVerbs *       pVerbs,           //< [in] The vvVerbs implementation to retrieve data from.
		class vvContext *     pContext,         //< [in] The context to use with the verbs.
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
		class vvVerbs *       pVerbs,           //< [in] The vvVerbs implementation to retrieve data from.
		class vvContext *     pContext,         //< [in] The context to use with the verbs.
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator& cValidator = wxDefaultValidator
		);

	/**
	 * Adds all the WorkItems in a repo to the control.
	 * Returns true if successful or false if an error occurred.
	 */
	bool AddItems(
		class vvVerbs&       cVerbs,           //< [in] The vvVerbs implementation to retrieve data from.
		class vvContext&     cContext,         //< [in] The context to use with the verbs.
		const wxString&      sRepo,            //< [in] Repo to get the stamp list from.
		const wxString&      sSearchTerm,
		bool                 bChecked = false //< [in] The initial checked state for the items.
		);

	/**
	 * Adds an item to the control and returns it.
	 * Returns a NULL item if there's already an item with the given name in the control.
	 */
	wxDataViewItem AddItem(
		vvVerbs::WorkItemEntry wie,
		bool            bChecked = false //< [in] Initial checked state for the stamp.
		);

	/**
	 * Finds an item by its name.
	 * Returns a NULL item if the name isn't in the control.
	 */
	wxDataViewItem GetItemByName(
		const wxString& sName //< [in] The name of the item to find.
		);

	/**
	 * Retrieves data about items in the control.
	 * Returns the number of items retrieved.
	 */
	unsigned int GetItemData(
		stlItemDataList* pItems    = NULL,  //< [out] List to add retrieved items to.
		                                    //<       May be NULL if you're only interested in the count.
		vvNullable<bool> nbChecked = vvNULL //< [in] Only retrieve items whose checked state matches this value.
		                                    //<      Or vvNULL to not filter based on checked state.
		) const;

	/**
	 * Retrieves data about a single item in the control.
	 */
	void GetItemData(
		const wxDataViewItem& cItem, //< [in] The item to retrieve data about.
		ItemData&             cData  //< [out] The retrieved data.
		) const;

	/**
	 * Sets the checked state of an item.
	 */
	void CheckItem(
		const wxDataViewItem& cItem,           //< [in] The item to check.
		vvNullable<bool>      nbChecked = true //< [in] The checked state to assign to the item.
		                                       //<      If vvNULL, toggle the checked state.
		);
	void Search(wxString& sRepoDescriptor, wxString& sSearchTerm);
	int GetColumnWidth(int index);
	void SetColumnWidth(int index, int width);

// event handlers
protected:
	void OnItemActivated(wxDataViewEvent& cEvent);

// private data
private:
	wxObjectDataPtr<class vvWorkItemsModel> mpDataModel; //< The data model we're displaying.

// macro declarations
private:
	vvDISABLE_COPY(vvWorkItemsControl);
	DECLARE_DYNAMIC_CLASS(vvWorkItemsControl);
	DECLARE_EVENT_TABLE();
};


#endif
