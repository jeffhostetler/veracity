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
#include "vvRepoBrowserControl.h"

/*
static wxBitmap _LoadImageByName(
	const wxString& sName,          //< [in] The name of the resource to load.
	LPCTSTR         szResourceType, //< [in] The type of resource, in the Win32 sense (RT_* or a string name).
	wxBitmapType    eImageType      //< [in] The wxBitmapType to load the image as.
	)
{
	HRSRC hResource = FindResource(wxGetInstance(), sName.wc_str(), szResourceType);
	if (hResource == NULL)
	{
		return wxNullBitmap;
	}

	HGLOBAL hData = LoadResource(wxGetInstance(), hResource);
	if (hData == NULL)
	{
		return wxNullBitmap;
	}

	LPVOID pData = LockResource(hData);
	DWORD  uSize = SizeofResource(wxGetInstance(), hResource);
	wxMemoryInputStream cStream(pData, uSize);
	wxImage cImage(cStream, eImageType);
	if (!cImage.Ok())
	{
		return wxNullBitmap;
	}

	return wxBitmap(cImage);
}
*/

wxDEFINE_EVENT(EVT_REPO_BROWSER_ITEM_ACTIVATED, wxCommandEvent);

vvRepoBrowserControl::vvRepoBrowserControl(
	vvVerbs& cVerbs,
	vvContext& cContext,
	const wxString& sRepoName,
	const wxString& sChangesetHID,
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	) : wxPanel(pParent, cId,cPosition, cSize, iStyle, wxT("Repo Browser Control") )
{
	cValidator;
	
	
	m_pVerbs = &cVerbs;
	m_pContext = &cContext;
	m_sRepoName = sRepoName;
	m_sChangesetHID = sChangesetHID;
	
	m_treeControl = NULL;
	wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
	m_treeControl = new wxTreeCtrl(this,-1,wxDefaultPosition,cSize, wxTR_HAS_BUTTONS| wxTR_SINGLE | wxTR_FULL_ROW_HIGHLIGHT | wxTR_DEFAULT_STYLE);
	sizer->Add(m_treeControl, 1, wxEXPAND);
	sizer->SetSizeHints(this);
	wxPanel::SetSizer(sizer);
	
	m_pImageList = new wxImageList(16, 16);
	
	// /*int folderIcon = */m_pImageList->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER));
	// /*int fileIcon = */m_pImageList->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER));
	m_pImageList->Add(wxArtProvider::GetIcon("ItemType_Folder_16", wxART_OTHER, wxSize(16,16)));
	m_pImageList->Add(wxArtProvider::GetIcon("ItemType_File_16", wxART_OTHER, wxSize(16,16)));
	m_treeControl->AssignImageList(m_pImageList);
	wxTreeItemId rootID = m_treeControl->AddRoot("@", 0);
	m_treeControl->SetItemHasChildren(rootID);
	m_treeControl->Expand(rootID);

}

bool vvRepoBrowserControl::Create(
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator&  cValidator
	)
{
	cValidator;
	// create our parent
	if (!wxPanel::Create(pParent, cId, cPosition, cSize, iStyle))
	{
		return false;
	}

	// done
	return true;
}
wxString vvRepoBrowserControl::GetItemPath(wxTreeItemId id)
{
	if (id.IsOk())
	{
		wxTreeItemId parentID = m_treeControl->GetItemParent(id);
		if (parentID.IsOk())
		{
			return GetItemPath(parentID)  + "/" + m_treeControl->GetItemText(id);
		}
		else
			return m_treeControl->GetItemText(id);
	}
	else
		return "";
}
void vvRepoBrowserControl::ExpandTreeItem(wxTreeEvent& event)
{
	vvVerbs::RepoItemList cChildren;
	m_treeControl->DeleteChildren(event.GetItem()); 
	m_pVerbs->Repo_Item_Get_Children(*m_pContext, m_sRepoName, m_sChangesetHID, GetItemPath(event.GetItem()), cChildren);
	for (vvVerbs::RepoItemList::const_iterator it = cChildren.begin();  it != cChildren.end(); ++it)
	{
		bool isDir = (*it).eItemType == 2;
		wxTreeItemId id = m_treeControl->AppendItem(event.GetItem(), (*it).sItemName, isDir ? 0 : 1);
		if (isDir)
			m_treeControl->SetItemHasChildren(id);
	}
}

void vvRepoBrowserControl::SetSelection(wxTreeItemId& itemID, wxStringTokenizer& sPath)
{
	if (!sPath.HasMoreTokens())
		m_treeControl->SelectItem(itemID, true);
	wxString token = sPath.GetNextToken();
	//Eat any empty tokens "@/src//tortoise"
	while (token == wxEmptyString && sPath.HasMoreTokens())
		token = sPath.GetNextToken();
	if (!sPath.HasMoreTokens())
		m_treeControl->SelectItem(itemID, true);
	//Ignore empty tokens at the end.  "@/src/"
	if (token == wxEmptyString && !sPath.HasMoreTokens())
		m_treeControl->SelectItem(itemID, true);
	wxTreeItemIdValue cookie;
	m_treeControl->Expand(itemID);
	wxTreeItemId child = m_treeControl->GetFirstChild(itemID, cookie);
	while ( child.IsOk())
	{
		if (token.CmpNoCase(m_treeControl->GetItemText(child)) ==  0)
		{
			SetSelection(child, sPath);
			return;
		}
		child = m_treeControl->GetNextChild(itemID, cookie);
	}

}

void vvRepoBrowserControl::SetSelection(wxString& sPath)
{
	sPath;
	wxStringTokenizer pathElements = wxStringTokenizer(sPath, "\\/");
	if (! pathElements.HasMoreTokens())
		return;
	wxString token = pathElements.GetNextToken();
	if (token != "@")
		return;
	wxTreeItemId currentItem = m_treeControl->GetRootItem();
	SetSelection(currentItem, pathElements);
}

wxString vvRepoBrowserControl::GetSelection()
{
	return GetItemPath(m_treeControl->GetSelection());
}

void vvRepoBrowserControl::ItemActivated(wxTreeEvent&)
{
	wxCommandEvent event(EVT_REPO_BROWSER_ITEM_ACTIVATED, this->GetId());
	event.SetEventObject(this);
	event.SetString(this->GetSelection());
	this->ProcessWindowEvent(event);
}

IMPLEMENT_DYNAMIC_CLASS(vvRepoBrowserControl, wxPanel);
BEGIN_EVENT_TABLE(vvRepoBrowserControl, wxPanel)
	EVT_TREE_ITEM_EXPANDING(wxID_ANY, vvRepoBrowserControl::ExpandTreeItem)
	EVT_TREE_ITEM_ACTIVATED(wxID_ANY, vvRepoBrowserControl::ItemActivated)
END_EVENT_TABLE()