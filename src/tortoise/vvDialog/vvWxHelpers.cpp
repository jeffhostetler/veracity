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
#include "vvWxHelpers.h"
#include "wx/utils.h"

/*
**
** Public Functionality
**
*/

wxWindow* vvFindTopLevelParent(
	wxWindow* wWindow
	)
{
	while (
			wWindow->GetParent() != NULL
		&& wxDynamicCast(wWindow, wxTopLevelWindow) == NULL
		)
	{
		wWindow = wWindow->GetParent();
	}
	return wWindow;
}

wxString vvTrimString(
	const wxString&  sValue,
	const wxString&  sChars,
	vvNullable<bool> nbFromEnd
	)
{
	// start with bounds indicating the entire string
	size_t iStart = 0u;
	size_t iEnd = sValue.Length();

	// if we're trimming from the beginning
	// then move the start bound forward until we find a non-trim character
	if (nbFromEnd.IsNullOrEqual(false))
	{
		while (iStart < iEnd && sChars.Find(sValue[iStart]) != wxNOT_FOUND)
		{
			++iStart;
		}
	}

	// if we're trimming from the end
	// then move the end bound backward until we find a non-trim character
	if (nbFromEnd.IsNullOrEqual(true))
	{
		while (iEnd > iStart && sChars.Find(sValue[iEnd - 1u]) != wxNOT_FOUND)
		{
			--iEnd;
		}
	}

	// return the bounded substring
	if (iStart >= iEnd)
	{
		return wxEmptyString;
	}
	else
	{
		return sValue.Mid(iStart, iEnd - iStart);
	}
}

int vvSortStrings_Insensitive(
	const wxString& sLeft,
	const wxString& sRight
	)
{
	return sLeft.CmpNoCase(sRight);
}

//A class that will enable wxPersistenceManager to handle wxChecBox
vvPersistentCheckBox::vvPersistentCheckBox(wxCheckBox * pCheckBox) : wxPersistentWindow<wxCheckBox>(pCheckBox)
{
}

wxString vvPersistentCheckBox::GetKind() const
{
	return "vvPersistentCheckBox";
}

void vvPersistentCheckBox::Save() const
{
	wxCheckBox * pcb = Get();
	bool valueToSave = pcb->GetValue();
	SaveValue("bChecked", valueToSave);
}

bool vvPersistentCheckBox::Restore()
{
	wxCheckBox * pcb = Get();
	bool valueToRestore = false;
	RestoreValue("bChecked", &valueToRestore);
	pcb->SetValue(valueToRestore);
	return true;
}

vvPersistentCheckBox *wxCreatePersistentObject(wxCheckBox * pCheckBox)
{
	return new vvPersistentCheckBox(pCheckBox);
}

//A class that will enable wxPersistenceManager to handle wxChecBox
vvPersistentComboBox::vvPersistentComboBox(wxComboBox * pComboBox) : wxPersistentWindow<wxComboBox>(pComboBox)
{
}

wxString vvPersistentComboBox::GetKind() const
{
	return "vvPersistentComboBox";
}

void vvPersistentComboBox::Save() const
{
	wxComboBox * pcb = Get();
	wxString valueToSave = pcb->GetValue();
	SaveValue("sPreviousSelection", valueToSave);
}

bool vvPersistentComboBox::Restore()
{
	wxComboBox * pcb = Get();
	wxString valueToRestore;
	RestoreValue("sPreviousSelection", &valueToRestore);
	if (!valueToRestore.IsEmpty())	
		pcb->SetValue(valueToRestore);
	return true;
}

vvPersistentComboBox *wxCreatePersistentObject(wxComboBox * pComboBox)
{
	return new vvPersistentComboBox(pComboBox);
}

//A class that will enable wxPersistenceManager to handle vvHistoryListControl
vvPersistentHistoryList::vvPersistentHistoryList(vvHistoryListControl * pHistoryList) : wxPersistentWindow<vvHistoryListControl>(pHistoryList)
{
}

wxString vvPersistentHistoryList::GetKind() const
{
	return "vvHistoryListControl";
}

#define _REVISION_SETTING "revision"
#define _COMMENT_SETTING "comment"
#define _BRANCH_SETTING "branch"
#define _STAMP_SETTING "stamp"
#define _TAG_SETTING "tag"
#define _DATE_SETTING "date"
#define _USER_SETTING "user"
#define _REVISION_WIDTH_SETTING "revision_width"
#define _COMMENT_WIDTH_SETTING "comment_width"
#define _BRANCH_WIDTH_SETTING "branch_width"
#define _STAMP_WIDTH_SETTING "stamp_width"
#define _TAG_WIDTH_SETTING "tag_width"
#define _DATE_WIDTH_SETTING "date_width"
#define _USER_WIDTH_SETTING "user_width"
#define _COLUMN_ORDER_SETTING "column_order"

void vvPersistentHistoryList::Save() const
{
	vvHistoryListControl * pcb = Get();
	vvHistoryColumnPreferences prefs;
	pcb->GetColumnSettings(prefs);

	SaveValue(_REVISION_SETTING, prefs.ShowRevisionColumn);
	SaveValue(_COMMENT_SETTING, prefs.ShowCommentColumn);
	SaveValue(_BRANCH_SETTING, prefs.ShowBranchColumn);
	SaveValue(_TAG_SETTING, prefs.ShowTagColumn);
	SaveValue(_STAMP_SETTING, prefs.ShowStampColumn);
	SaveValue(_DATE_SETTING, prefs.ShowDateColumn);
	SaveValue(_USER_SETTING, prefs.ShowUserColumn);

	SaveValue(_REVISION_WIDTH_SETTING, prefs.WidthRevisionColumn);
	SaveValue(_COMMENT_WIDTH_SETTING, prefs.WidthCommentColumn);
	SaveValue(_BRANCH_WIDTH_SETTING, prefs.WidthBranchColumn);
	SaveValue(_TAG_WIDTH_SETTING, prefs.WidthTagColumn);
	SaveValue(_STAMP_WIDTH_SETTING, prefs.WidthStampColumn);
	SaveValue(_DATE_WIDTH_SETTING, prefs.WidthDateColumn);
	SaveValue(_USER_WIDTH_SETTING, prefs.WidthUserColumn);

	//column_order is stored as a comma-separated list of integers.
	wxString columnOrder;
	for (wxArrayInt::const_iterator it = prefs.columnOrder.begin(); it != prefs.columnOrder.end(); ++it)
	{
		if (it != prefs.columnOrder.begin())
			columnOrder.Append(",");
		columnOrder << *it;
	}
	SaveValue("column_order", columnOrder);
}

bool vvPersistentHistoryList::Restore()
{
	vvHistoryListControl * pcb = Get();
	vvHistoryColumnPreferences prefs;
	RestoreValue(_REVISION_SETTING, &prefs.ShowRevisionColumn);
	RestoreValue(_COMMENT_SETTING, &prefs.ShowCommentColumn);
	RestoreValue(_BRANCH_SETTING, &prefs.ShowBranchColumn);
	RestoreValue(_TAG_SETTING, &prefs.ShowTagColumn);
	RestoreValue(_STAMP_SETTING, &prefs.ShowStampColumn);
	RestoreValue(_DATE_SETTING, &prefs.ShowDateColumn);
	RestoreValue(_USER_SETTING, &prefs.ShowUserColumn);

	RestoreValue(_REVISION_WIDTH_SETTING, &prefs.WidthRevisionColumn);
	RestoreValue(_COMMENT_WIDTH_SETTING, &prefs.WidthCommentColumn);
	RestoreValue(_BRANCH_WIDTH_SETTING, &prefs.WidthBranchColumn);
	RestoreValue(_TAG_WIDTH_SETTING, &prefs.WidthTagColumn);
	RestoreValue(_STAMP_WIDTH_SETTING, &prefs.WidthStampColumn);
	RestoreValue(_DATE_WIDTH_SETTING, &prefs.WidthDateColumn);
	RestoreValue(_USER_WIDTH_SETTING, &prefs.WidthUserColumn);

	//column_order is stored as a comma-separated list of integers.
	wxString columnOrder;
	RestoreValue("column_order", &columnOrder);
	wxStringTokenizer tok(columnOrder, ",");
	while ( tok.HasMoreTokens() )
    {
        wxString token = tok.GetNextToken();
		long orderValue;
		token.ToLong(&orderValue);
		prefs.columnOrder.Add(orderValue);
    }
	pcb->SetColumnPreferences(prefs);
	pcb->ApplyColumnPreferences();
	return true;
}

vvPersistentHistoryList *wxCreatePersistentObject(vvHistoryListControl * pCheckBox)
{
	return new vvPersistentHistoryList(pCheckBox);
}

//A class that will enable wxPersistenceManager to handle vvWorkItemsControl
vvPersistentWorkItems::vvPersistentWorkItems(vvWorkItemsControl * pHistoryList) : wxPersistentWindow<vvWorkItemsControl>(pHistoryList)
{
}

wxString vvPersistentWorkItems::GetKind() const
{
	return "vvWorkItemsControl";
}

#define _ID_WIDTH_SETTING "id_width"
#define _TITLE_WIDTH_SETTING "title_width"
#define _STATUS_WIDTH_SETTING "status_width"

void vvPersistentWorkItems::Save() const
{
	vvWorkItemsControl * pcb = Get();

	SaveValue(_ID_WIDTH_SETTING, pcb->GetColumnWidth(2) );
	SaveValue(_TITLE_WIDTH_SETTING, pcb->GetColumnWidth(3) );
	SaveValue(_STATUS_WIDTH_SETTING, pcb->GetColumnWidth(4) );
	
}

bool vvPersistentWorkItems::Restore()
{
	vvWorkItemsControl * pcb = Get();
	int id_width;
	int title_width;
	int status_width;
	RestoreValue(_ID_WIDTH_SETTING, &id_width);
	RestoreValue(_TITLE_WIDTH_SETTING, &title_width);
	RestoreValue(_STATUS_WIDTH_SETTING, &status_width);

	if (id_width > 0 && id_width < 10000)
		pcb->SetColumnWidth(2, id_width);
	if (title_width > 0 && title_width < 10000)
		pcb->SetColumnWidth(3, title_width);
	if (status_width > 0 && status_width < 10000)
		pcb->SetColumnWidth(4, status_width);
	return true;
}

vvPersistentWorkItems *wxCreatePersistentObject(vvWorkItemsControl * pCheckBox)
{
	return new vvPersistentWorkItems(pCheckBox);
}