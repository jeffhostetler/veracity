#include "precompiled.h"
#include <wx/notebook.h>
#include <wx/thread.h>
#include "vvRevisionLinkControl.h"
#include "vvRevDetailsDialog.h"
#include "vvVerbs.h"
#include "vvHistoryObjects.h"
#include "vvContext.h"
#include "vvSgHelpers.h"

vvRevisionLinkControl::vvRevisionLinkControl()
{
}

vvRevisionLinkControl::vvRevisionLinkControl(
	unsigned int nRevisionNum,
	wxString sRevisionHID,
	wxWindow*          pParent,
	wxWindowID         cId,
	const vvRepoLocator& cRepoLocator,
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	bool			   bIgnoreClicks,
	bool			   bShowFullHID,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	) //: wxPanel(pParent, cId,cPosition, cSize, iStyle, wxT("Audit Details Control") )
{
	cId;
	cPosition;
	cSize;
	iStyle;
	cValidator;
	m_cRepoLocator = cRepoLocator;
	m_pVerbs = &cVerbs;
	m_pContext = &cContext;
	this->Create(pParent, cId, cPosition, cSize, iStyle, cValidator);
	m_bIgnoreClicks = bIgnoreClicks;
	m_revHID = sRevisionHID;
	m_bShowFullHID = bShowFullHID;
	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText * pST = new wxStaticText(this, wxID_ANY, "");
	m_pST = pST;
	pST->SetForegroundColour(wxColor("blue"));
	wxPanel::MSWSetTransparentBackground(true);
	wxFont font = pST->GetFont();
	font.SetUnderlined(true);
	pST->SetFont(font);
	sizer->Add(pST, 1, wxEXPAND);
	wxPanel::SetSizer(sizer);
	pST->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(vvRevisionLinkControl::OnClick), NULL, this);
	pST->Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(vvRevisionLinkControl::OnEnter), NULL, this);
	pST->Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(vvRevisionLinkControl::OnLeave), NULL, this);
	this->SetRevision(nRevisionNum, sRevisionHID);
}

bool vvRevisionLinkControl::Create(
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

void vvRevisionLinkControl::SetRevision(long nRevno, wxString& sChangesetHID)
{
	wxString newLabel;
	m_revHID = sChangesetHID;
	wxString tmpChangesetHID = sChangesetHID;
	if (!m_bShowFullHID)
		tmpChangesetHID = sChangesetHID.substr(0, 10) + "...";

	if (nRevno == 0)
	{
		vvVerbs::HistoryEntry cEntry;
		vvVerbs::RevisionList children;
		m_pVerbs->Get_Revision_Details(*m_pContext, m_cRepoLocator.GetRepoName(), sChangesetHID, cEntry, children);
		nRevno = cEntry.nRevno;
	}
	newLabel = wxString::Format("%d:%s", (int)nRevno, tmpChangesetHID);
	
	m_pST->SetLabel(newLabel);
}

void vvRevisionLinkControl::OnClick(wxMouseEvent& evt)
{
	//Fire an event that the parent can catch.
	
	if (m_bIgnoreClicks)
	{
		wxCommandEvent MyEvent( wxEVT_COMMAND_BUTTON_CLICKED ); // Keep it simple, don't give a specific event ID
		MyEvent.SetString(m_revHID);
		wxPostEvent(this, MyEvent); // This posts to ourselves: it'll be caught and sent to a different method
	}
	else
	{
		vvRevDetailsDialog rdd;
		rdd.Create(this, m_cRepoLocator, m_revHID, *m_pVerbs, *m_pContext);
		rdd.ShowModal();
	}
	evt.Skip();
}

void vvRevisionLinkControl::OnEnter(wxMouseEvent& evt)
{
	wxWindow::SetCursor(wxCURSOR_HAND);
	evt.Skip();
}

void vvRevisionLinkControl::OnLeave(wxMouseEvent& evt)
{
	wxWindow::SetCursor(wxCURSOR_ARROW);
	evt.Skip();
}
IMPLEMENT_DYNAMIC_CLASS(vvRevisionLinkControl, wxPanel);
BEGIN_EVENT_TABLE(vvRevisionLinkControl, wxPanel)
	EVT_LEFT_UP(vvRevisionLinkControl::OnClick)
END_EVENT_TABLE()
