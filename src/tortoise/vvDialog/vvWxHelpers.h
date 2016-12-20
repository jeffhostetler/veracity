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

#ifndef VV_WX_HELPERS_H
#define VV_WX_HELPERS_H

#include "precompiled.h"
#include "vvNullable.h"
#include "wx/persist.h"
#include "wx/persist/window.h"
#include "vvHistoryListControl.h"
#include "vvWorkItemsControl.h"

/**
 * Helper macro for retrieving sizer items from an XRC layout.
 * Identical to XRCSIZERITEM, except that it searches recursively.
 */
#define vvXRCSIZERITEM(window, id) \
	((window).GetSizer() ? (window).GetSizer()->GetItemById(XRCID(id), true) : NULL)

/**
 * Finds the highest-level parent of a window.
 *
 * Iterates through parent windows until it finds one of:
 * 1) A window of type wxTopLevelWindow.
 * 2) A window whose parent is NULL.
 */
wxWindow* vvFindTopLevelParent(
	wxWindow* wWindow
	);

/**
 * Trims a set of characters from the beginning and/or end of a string.
 *
 * Like wxString.Trim() except that you can specify the characters, rather
 * than having it hard-coded to always trim whitespace.
 *
 * Returns the trimmed string.
 */
wxString vvTrimString(
	const wxString&  sValue,                //< [in] The string to trim.
	const wxString&  sChars    = " \t\r\n", //< [in] The set of characters to remove.
	vvNullable<bool> nbFromEnd = vvNULL     //< [in] True to remove from the end.
	                                        //<      False to remove from the beginning.
	                                        //<      Null to remove from both.
	);

/**
 * A string CompareFunction that sorts in a case-insensitive manner.
 * Usable with wxArrayString::Sort.
 */
int vvSortStrings_Insensitive(
	const wxString& sLeft, //< [in] String being compared on the left.
	const wxString& sRight //< [in] String being compared on the right.
	);

//A class that will enable wxPersistenceManager to handle wxChecBox
class vvPersistentCheckBox : public wxPersistentWindow<wxCheckBox>
{
public:
	 vvPersistentCheckBox(wxCheckBox * pCheckBox);

	 virtual wxString GetKind() const;
	 
	 virtual void Save() const;

	 virtual bool Restore();
private:
};

vvPersistentCheckBox *wxCreatePersistentObject(wxCheckBox * pCheckBox);

//A class that will enable wxPersistenceManager to handle wxChecBox
class vvPersistentComboBox : public wxPersistentWindow<wxComboBox>
{
public:
	 vvPersistentComboBox(wxComboBox * pComboBox);

	 virtual wxString GetKind() const;
	 
	 virtual void Save() const;

	 virtual bool Restore();
private:
};

vvPersistentComboBox *wxCreatePersistentObject(wxComboBox * pComboBox);

//A class that will enable wxPersistenceManager to handle vvHistoryListControl
class vvPersistentHistoryList : public wxPersistentWindow<vvHistoryListControl>
{
public:
	 vvPersistentHistoryList(vvHistoryListControl * pCheckBox);

	 virtual wxString GetKind() const;
	 
	 virtual void Save() const;

	 virtual bool Restore();
private:
};


vvPersistentHistoryList *wxCreatePersistentObject(vvHistoryListControl * pCheckBox);

//A class that will enable wxPersistenceManager to handle vvHistoryListControl
class vvPersistentWorkItems : public wxPersistentWindow<vvWorkItemsControl>
{
public:
	 vvPersistentWorkItems(vvWorkItemsControl * pCheckBox);

	 virtual wxString GetKind() const;
	 
	 virtual void Save() const;

	 virtual bool Restore();
private:
};

vvPersistentWorkItems *wxCreatePersistentObject(vvWorkItemsControl * pCheckBox);
#endif

