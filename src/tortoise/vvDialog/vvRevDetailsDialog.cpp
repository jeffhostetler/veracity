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
#include "vvRevDetailsDialog.h"

#include "vvVerbs.h"
#include "vvContext.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "vvProgressExecutor.h"
#include "vvDialogHeaderControl.h"
#include "vvAuditDetailsControl.h"
#include "vvRevisionLinkControl.h"
#include "vvTextLinkControl.h"
#include "vvHistoryObjects.h"
#include "vvRepoBrowserDialog.h"
#include "vvResourceArtProvider.h"
#include "sg.h"
#include "vvStatusDialog.h"

#define ADDCOMMENT_BUTTON_ID wxID_HIGHEST + 24

wxString gsDiffBitmapName = "RevisionDetails__Diff";
/*
**
** Public Functions
**
*/

bool vvRevDetailsDialog::Create(
	wxWindow*            pParent,
	const vvRepoLocator& cRepoLocator,
	const wxString&      sRevID,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}
	// create our header control
	wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, cRepoLocator.GetRepoName(), cVerbs, cContext));
	
	m_historyEntry.sChangesetHID = "";

	aRevisions.Add(sRevID);
	nCurrentRevision = 0;
	msRevID = sRevID;
	m_pRepoLocator = &cRepoLocator;
	m_nParentRevLinkID_1 = wxWindow::NewControlId();
	m_nParentRevLinkID_2 = wxWindow::NewControlId();
	m_nParentRevLinkID_3 = wxWindow::NewControlId();
	//vvRevisionLinkControl * pRevLink = new vvRevisionLinkControl(0, "", this, m_nParentRevLinkID_1);
	m_pBackLink = new vvTextLinkControl("Back", this, m_nParentRevLinkID_2);
	m_pForwardLink = new vvTextLinkControl("Forward", this, m_nParentRevLinkID_3);
	//wxXmlResource::Get()->AttachUnknownControl("wRevLinkControl", pRevLink, this);
	wxXmlResource::Get()->AttachUnknownControl("wBackLink", m_pBackLink, this);
	wxXmlResource::Get()->AttachUnknownControl("wForwardLink", m_pForwardLink, this);

	XRCCTRL(*this, "wTags", wxListBox)->Connect(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(vvRevDetailsDialog::OnTagsContext), NULL, this);
	XRCCTRL(*this, "wStamps", wxListBox)->Connect(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(vvRevDetailsDialog::OnStampsContext), NULL, this);
	XRCCTRL(*this, "wAddTagText", wxTextCtrl)->Connect(wxEVT_COMMAND_TEXT_ENTER , wxCommandEventHandler(vvRevDetailsDialog::AddTag), NULL, this);
	XRCCTRL(*this, "wAddStampText", wxTextCtrl)->Connect(wxEVT_COMMAND_TEXT_ENTER , wxCommandEventHandler(vvRevDetailsDialog::AddStamp), NULL, this);
	
	m_pForwardLink->Disable();
	XRCCTRL(*this, "wNavigationMenu", wxPanel)->Hide();
	XRCCTRL(*this, "wCommentControl", wxTextCtrl)->SetBackgroundColour(this->GetBackgroundColour());
	//LoadRevision(sRevID);
	//sizer->SetSizeHints(this);
	m_DiffBitmap = wxArtProvider::GetBitmap(gsDiffBitmapName);
	this->Layout();
	this->Fit();
	return true;
}

void vvRevDetailsDialog::AddTag(wxCommandEvent& evt)
{
	evt;
	vvContext& pCtx = *this->GetContext();
	wxTextCtrl * pText = XRCCTRL(*this, "wAddTagText", wxTextCtrl);
	if (this->GetVerbs()->AddTag(pCtx, m_pRepoLocator->GetRepoName(), aRevisions[nCurrentRevision], pText->GetValue()) == false)
	{
		if (pCtx.Error_Equals(SG_ERR_ZING_CONSTRAINT))
		{
			//Prompt to force
			pCtx.Error_Reset();
			wxMessageDialog * msg = new wxMessageDialog(this, "The tag is already applied in this repository, do you want to remove the tag from the repository, and apply it to this revision?", "Tag Exists", wxYES_NO | wxNO_DEFAULT);
			if (msg->ShowModal() == wxID_YES)
			{
				if (this->GetVerbs()->DeleteTag(pCtx, m_pRepoLocator->GetRepoName(), pText->GetValue()) == false)
				{
					pCtx.Log_ReportError_CurrentError();
					pCtx.Error_Reset();
				}
				else
				{
					if (this->GetVerbs()->AddTag(pCtx, m_pRepoLocator->GetRepoName(), aRevisions[nCurrentRevision], pText->GetValue()) == false)
					{
						pCtx.Log_ReportError_CurrentError();
						pCtx.Error_Reset();
					}
					else
					{
						this->LoadRevision(aRevisions[nCurrentRevision]);
						pText->Clear();
					}
				}
			}
		}
		else
		{
			pCtx.Log_ReportError_CurrentError();
			pCtx.Error_Reset();
		}
	}
	else
	{
		this->LoadRevision(aRevisions[nCurrentRevision]);
		pText->Clear();
	}
}

void vvRevDetailsDialog::AddStamp(wxCommandEvent& evt)
{
	evt;
	vvContext& pCtx = *this->GetContext();
	wxTextCtrl * pText = XRCCTRL(*this, "wAddStampText", wxTextCtrl);
	if (this->GetVerbs()->AddStamp(pCtx, m_pRepoLocator->GetRepoName(), aRevisions[nCurrentRevision], pText->GetValue()) == false)
	{
		pCtx.Log_ReportError_CurrentError();
		pCtx.Error_Reset();
	}
	else
	{
		this->LoadRevision(aRevisions[nCurrentRevision]);
		pText->Clear();
	}
}

void vvRevDetailsDialog::LaunchDiff(wxString rev1, wxString rev2)
{
	vvStatusDialog * vsd = new vvStatusDialog();
	vsd->Create(this, *m_pRepoLocator, *this->GetVerbs(), *this->GetContext(), rev1, rev2 );

	vsd->Show();
}

void vvRevDetailsDialog::RevLinkClicked(wxCommandEvent& evt)
{
	evt;
	vvContext& pCtx = *this->GetContext();
	if (evt.GetString() != wxEmptyString)
	{
		if (evt.GetString() == "Back")
		{
			nCurrentRevision--;	
			this->LoadRevision(aRevisions[nCurrentRevision]);
		}
		else if (evt.GetString() == "Forward")
		{
			nCurrentRevision++;
			this->LoadRevision(aRevisions[nCurrentRevision]);
		}
		else
		{
			aRevisions.resize(nCurrentRevision + 1);
			aRevisions.Add(evt.GetString());
			nCurrentRevision++;
			this->LoadRevision(evt.GetString());
			XRCCTRL(*this, "wNavigationMenu", wxPanel)->Show();
		}
	}
	else if (evt.GetId() == ADDCOMMENT_BUTTON_ID)
	{
		wxTextEntryDialog * ted = new wxTextEntryDialog(this, "Enter the text for the new comment.", "Add a New Comment", "",  wxOK | wxCANCEL | wxCENTER | wxTE_MULTILINE);
		if (ted->ShowModal() == wxID_OK && ted->GetValue() != wxEmptyString)
		{
			this->GetVerbs()->AddComment(pCtx, m_pRepoLocator->GetRepoName(), aRevisions[nCurrentRevision], ted->GetValue());
			//Sleep(1000);
			this->LoadRevision(aRevisions[nCurrentRevision]);
			wxScrolledWindow * pSW = XRCCTRL(*this, "wAudits",                 wxScrolledWindow);
			pSW->Scroll(0, pSW->GetVirtualSize().GetHeight());
		}
	}
	else if (evt.GetId() == XRCCTRL(*this, "wAddStampButton", wxButton)->GetId())
	{
		AddStamp(evt);
	}
	else if (evt.GetId() == XRCCTRL(*this, "wAddTagButton", wxButton)->GetId())
	{
		AddTag(evt);
	}
	else if (evt.GetId() == XRCCTRL(*this, "wExport", wxButton)->GetId())
	{
		wxDirDialog * wdd = new wxDirDialog(this, "Choose a location to save the contents of this revision.", wxEmptyString, wxDD_NEW_DIR_BUTTON  |wxDD_DIR_MUST_EXIST | wxDD_DEFAULT_STYLE);
		if (wdd->ShowModal() == wxID_OK)
		{
			//Perform the export.
			if (!this->GetVerbs()->Export_Revision(pCtx, m_pRepoLocator->GetRepoName(), aRevisions[nCurrentRevision], wdd->GetPath()) )
			{
				pCtx.Log_ReportError_CurrentError();
				pCtx.Error_Reset();
			}
		}
	}
	else if (evt.GetId() == XRCCTRL(*this, "wBrowse", wxButton)->GetId())
	{
		vvRepoBrowserDialog dialog = vvRepoBrowserDialog();
		wxString initialSelection;
		dialog.Create(this, m_pRepoLocator->GetRepoName(), aRevisions[nCurrentRevision], initialSelection, false, *this->GetVerbs(), pCtx);
		dialog.ShowModal();
	}
	else if (m_childButtonIds.Index(evt.GetId()) != wxNOT_FOUND)
	{
		int index = m_childButtonIds.Index(evt.GetId());
		LaunchDiff(aRevisions[nCurrentRevision], m_childCSIDs.Item(index));

	}
	else if (m_parentButtonIds.Index(evt.GetId()) != wxNOT_FOUND)
	{
		int index = m_parentButtonIds.Index(evt.GetId());
		LaunchDiff(m_parentCSIDs.Item(index), aRevisions[nCurrentRevision]);
	}

	if ((int)aRevisions.Count() == nCurrentRevision + 1)
	{
		m_pForwardLink->Disable();
	}
	else
	{
		m_pForwardLink->Enable();
	}
	if (nCurrentRevision == 0)
	{
		m_pBackLink->Disable();
	}
	else
	{
		m_pBackLink->Enable();
	}
	
	this->Layout();
	//this->Fit();
	evt.Skip();
}

void vvRevDetailsDialog::SetReadOnly()
{
	XRCCTRL(*this, "wAddStampText", wxTextCtrl)->Enable(false);
	XRCCTRL(*this, "wAddTagText", wxTextCtrl)->Enable(false);

	XRCCTRL(*this, "wAddStampButton", wxButton)->Enable(false);
	XRCCTRL(*this, "wAddTagButton", wxButton)->Enable(false);
	
	XRCCTRL(*this, "wExport", wxButton)->Enable(false);
	XRCCTRL(*this, "wBrowse", wxButton)->Enable(false);
	
}

void vvRevDetailsDialog::DisableNavigation()
{
	XRCCTRL(*this, "wParents", wxPanel)->Enable(false);
	XRCCTRL(*this, "wChildren", wxPanel)->Enable(false);
}

void vvRevDetailsDialog::SetRevision(vvVerbs::HistoryEntry he)
{
	m_historyEntry = he;
	SetReadOnly();
	DisableNavigation();
}
void vvRevDetailsDialog::LoadRevision(wxString sRevHID)
{
	vvContext&     pCtx = *this->GetContext();
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvVerbs::HistoryEntry cEntry;
	vvVerbs::RevisionList children;
	wxBusyCursor wait;
	bool bReadOnly = false;
	if (m_historyEntry.sChangesetHID.IsEmpty())
	{
		if (pVerbs->Get_Revision_Details(pCtx, m_pRepoLocator->GetRepoName(), sRevHID, cEntry, children) == false)
		{
			pCtx.Log_ReportError_CurrentError();
			pCtx.Error_Reset();
			return;
		}
	}
	else
	{
		cEntry = m_historyEntry;
		bReadOnly = true;
	}
	wxPanel* parents = XRCCTRL(*this, "wParents", wxPanel);

	parents->DestroyChildren();
	wxBoxSizer *bsizer = new wxBoxSizer(wxVERTICAL);
	if (cEntry.cParents.size() != 0)
	{
		if (cEntry.cParents.size() == 1)
			XRCCTRL(*this, "wParentsLabel", wxStaticText)->SetLabel("Parent:");
		else
			XRCCTRL(*this, "wParentsLabel", wxStaticText)->SetLabel("Parents:");

		//These two arrays are a hack to get around the fact that wxHashTable only stores 
		//pointers.  In this case, that's more work than is necessary.  Ideally this would be
		//one hash table instead of two arrays.
		m_parentCSIDs.Clear();
		m_parentButtonIds.Clear();
		for (vvVerbs::RevisionList::const_iterator it = cEntry.cParents.begin(); it != cEntry.cParents.end(); ++it)
		{
			wxBoxSizer *bsizer2 = new wxBoxSizer(wxHORIZONTAL);
			bsizer->Add(bsizer2, 1, wxEXPAND);
			bsizer2->Add(new vvRevisionLinkControl(((vvVerbs::RevisionEntry)*it).nRevno, ((vvVerbs::RevisionEntry)*it).sChangesetHID, parents, wxID_ANY, *m_pRepoLocator, *this->GetVerbs(), *this->GetContext()), 1, wxBOTTOM, 5);
			wxBitmapButton * pbb = new wxBitmapButton(parents, wxID_ANY,m_DiffBitmap);
			m_parentButtonIds.Add(pbb->GetId());
			m_parentCSIDs.Add(((vvVerbs::RevisionEntry)*it).sChangesetHID);
			bsizer2->Add(pbb, 0, wxLEFT, 3);
		}
	}
	else
	{
		XRCCTRL(*this, "wParentsLabel", wxStaticText)->SetLabel("Parents:");
		bsizer->Add(new wxStaticText(parents, wxID_ANY, "No parents"), wxEXPAND);
	}
	
	parents->MSWSetTransparentBackground(true);
	parents->SetSizer(bsizer);	
	parents->Layout();
	parents->Fit();
	//}
	
	wxPanel* childrenPanel = XRCCTRL(*this, "wChildren", wxPanel);
	childrenPanel->DestroyChildren();
	wxBoxSizer *bsizer2 = new wxBoxSizer(wxVERTICAL);
	if (children.size() != 0)
	{
		if (children.size() == 1)
			XRCCTRL(*this, "wChildrenLabel", wxStaticText)->SetLabel("Child:");
		else
			XRCCTRL(*this, "wChildrenLabel", wxStaticText)->SetLabel("Children:");
		//These two arrays are a hack to get around the fact that wxHashTable only stores 
		//pointers.  In this case, that's more work than is necessary.  Ideally this would be
		//one hash table instead of two arrays.
		m_childCSIDs.Clear();
		m_childButtonIds.Clear();
		for (vvVerbs::RevisionList::const_iterator it = children.begin(); it != children.end(); ++it)
		{
			wxBoxSizer *bsizer3 = new wxBoxSizer(wxHORIZONTAL);
			bsizer2->Add(bsizer3, 1, wxEXPAND);
			bsizer3->Add(new vvRevisionLinkControl(((vvVerbs::RevisionEntry)*it).nRevno, ((vvVerbs::RevisionEntry)*it).sChangesetHID, childrenPanel, wxID_ANY, *m_pRepoLocator, *this->GetVerbs(), *this->GetContext()), 1, wxBOTTOM, 5);
			wxBitmapButton * pbb = new wxBitmapButton(childrenPanel, wxID_ANY,m_DiffBitmap);
			m_childButtonIds.Add(pbb->GetId());
			m_childCSIDs.Add(((vvVerbs::RevisionEntry)*it).sChangesetHID);
			bsizer3->Add(pbb, 0, wxLEFT, 3);
		}
	}
	else
	{
		XRCCTRL(*this, "wChildrenLabel", wxStaticText)->SetLabel("Children:");
		bsizer2->Add(new wxStaticText(childrenPanel, wxID_ANY, "No children"), wxEXPAND);
	}
	childrenPanel->MSWSetTransparentBackground(true);
	childrenPanel->SetSizer(bsizer2);	
	childrenPanel->Layout();
	childrenPanel->Fit();

	XRCCTRL(*this, "wIDControl", wxTextCtrl)->SetValue(wxString::Format("%d:%s", cEntry.nRevno, cEntry.sChangesetHID));
	XRCCTRL(*this, "wUserControl", wxTextCtrl)->SetValue(cEntry.cAudits[0].sWho);
	XRCCTRL(*this, "wDateControl", wxTextCtrl)->SetValue(cEntry.cAudits[0].nWhen.Format());

	wxScrolledWindow * pSW = XRCCTRL(*this, "wAudits",                 wxScrolledWindow);
	pSW->Freeze();
	wxSizer * sizer = pSW->GetSizer();
	if (sizer == NULL)
		sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Clear(true);
	for (unsigned int i = 0; i < cEntry.cComments.size(); i ++)
	{ 
		sizer->Add(new vvAuditDetailsControl(*pVerbs, pCtx, cEntry.cComments[i].cAudits.size() > 0 ? cEntry.cComments[i].cAudits[0] : vvVerbs::AuditEntry(), cEntry.cComments[i].sCommentText, pSW, wxID_ANY), 1, wxEXPAND | wxLEFT | wxBOTTOM| wxRIGHT, 8); 
	}
	if (!bReadOnly)
		sizer->Add(new wxButton(pSW, ADDCOMMENT_BUTTON_ID, "Add a New Comment..."), 0, wxALIGN_RIGHT | wxRIGHT, 8);
	
	if (pSW->GetSizer() == NULL)
		pSW->SetSizer(sizer);
	pSW->FitInside();
	pSW->SetScrollRate(10, 10);

	if (cEntry.cComments.size() > 0)
		XRCCTRL(*this, "wCommentControl", wxTextCtrl)->SetValue(cEntry.cComments[0].sCommentText);
	else
		XRCCTRL(*this, "wCommentControl", wxTextCtrl)->SetValue("");

	XRCCTRL(*this, "wTopRow", wxPanel)->Layout();
	XRCCTRL(*this, "wTopRow", wxPanel)->Fit();

	wxListBox * tagsList = XRCCTRL(*this, "wTags", wxListBox);
	tagsList->Clear();
	tagsList->Append(cEntry.cTags);
	
	wxListBox * stampsList = XRCCTRL(*this, "wStamps", wxListBox);
	stampsList->Clear();
	stampsList->Append(cEntry.cStamps);
	
	this->Layout();
	//childrenPanel->Raise();
	//parents->Raise();
	pSW->Thaw();	
}

void vvRevDetailsDialog::OnTagsContext(wxContextMenuEvent& event)
{
	wxListBox * tagsList = XRCCTRL(*this, "wTags", wxListBox);
	int itemTheyClicked = tagsList->HitTest(tagsList->ScreenToClient(event.GetPosition()));
	if (itemTheyClicked != wxNOT_FOUND)
	{
		//Select the item they clicked.
		tagsList->Select(itemTheyClicked);
		if (tagsList->GetSelection() != wxNOT_FOUND)
		{
			wxMenu menu;
			menu.Append(wxID_DELETE, wxT("&Delete"), wxT(""));
			PopupMenu(&menu);
		}
	}
}

void vvRevDetailsDialog::OnStampsContext(wxContextMenuEvent& event)
{
	event;
	wxListBox * stampsList = XRCCTRL(*this, "wStamps", wxListBox);
	int itemTheyClicked = stampsList->HitTest(stampsList->ScreenToClient(event.GetPosition()));
	if (itemTheyClicked != wxNOT_FOUND)
	{
		//Select the item they clicked.
		stampsList->Select(itemTheyClicked);
		if (stampsList->GetSelection() != wxNOT_FOUND)
		{
			wxMenu menu;
			menu.Append(wxID_CDROM, wxT("&Delete"), wxT(""));
			PopupMenu(&menu);
		}
	}
}

void vvRevDetailsDialog::OnMenuDelete(wxCommandEvent& event)
{
	if (event.GetId() == wxID_CDROM)
	{
		//Delete Stamp
		wxListBox * pList = XRCCTRL(*this, "wStamps", wxListBox);
		pList;
		this->GetVerbs()->DeleteStamp(*this->GetContext(), m_pRepoLocator->GetRepoName(), aRevisions[nCurrentRevision], pList->GetStringSelection());
		this->LoadRevision(aRevisions[nCurrentRevision]);
	}
	else if (event.GetId() == wxID_DELETE)
	{
		//Delete Tag
		wxListBox * pList = XRCCTRL(*this, "wTags", wxListBox);
		pList;
		this->GetVerbs()->DeleteTag(*this->GetContext(), m_pRepoLocator->GetRepoName(), pList->GetStringSelection());
		this->LoadRevision(aRevisions[nCurrentRevision]);
	}
}

bool vvRevDetailsDialog::TransferDataToWindow()
{ 
	
	this->LoadRevision(msRevID);
	this->Layout();
	this->Fit();
	return true;
}

bool vvRevDetailsDialog::TransferDataFromWindow()
{
	
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(vvRevDetailsDialog, vvDialog);
BEGIN_EVENT_TABLE(vvRevDetailsDialog, vvDialog)
	//EVT_REVISION_LINK_CLICKED(wxID_ANY, vvRevDetailsDialog::RevLinkClicked)
	EVT_BUTTON(wxID_ANY, vvRevDetailsDialog::RevLinkClicked)
	EVT_MENU(wxID_DELETE,   vvRevDetailsDialog::OnMenuDelete)
	EVT_MENU(wxID_CDROM,   vvRevDetailsDialog::OnMenuDelete)
	//EVT_TEXT_ENTER(wxID_ANY, vvRevDetailsDialog::AddTag)
END_EVENT_TABLE()
