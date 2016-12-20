#include "precompiled.h"
#include <wx/notebook.h>
#include <wx/thread.h>
#include "vvTextLinkControl.h"
#include "vvVerbs.h"
#include "vvHistoryObjects.h"
#include "vvContext.h"
#include "vvSgHelpers.h"

vvTextLinkControl::vvTextLinkControl()
{
}

vvTextLinkControl::vvTextLinkControl(
	wxString sLinkText,
	wxWindow*          pParent,
	wxWindowID         cId,
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
	this->Create(pParent, cId, cPosition, cSize, iStyle, cValidator);
	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText * pST = new wxStaticText(this, wxID_ANY, "");
	m_pST = pST;
	pST->SetForegroundColour(wxColor("blue"));
	wxFont font = pST->GetFont();
	font.SetUnderlined(true);
	pST->SetFont(font);
	sizer->Add(pST, 1, wxEXPAND);
	wxPanel::SetSizer(sizer);
	pST->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(vvTextLinkControl::OnClick), NULL, this);
	pST->Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(vvTextLinkControl::OnEnter), NULL, this);
	pST->Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(vvTextLinkControl::OnLeave), NULL, this);
	m_sText = sLinkText;
	m_pST->SetLabel(sLinkText);
}

bool vvTextLinkControl::Create(
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

void vvTextLinkControl::OnEnter(wxMouseEvent& evt)
{
	wxWindow::SetCursor(wxCURSOR_HAND);
	evt.Skip();
}

void vvTextLinkControl::OnLeave(wxMouseEvent& evt)
{
	wxWindow::SetCursor(wxCURSOR_ARROW);
	evt.Skip();
}

void vvTextLinkControl::OnClick(wxMouseEvent& evt)
{
	//Fire an event that the parent can catch.
	//wxCommandEvent ev(newEVT_REVISION_LINK_CLICKED, GetId() );
	//ev.SetString(m_revHID);
	//ev.SetEventObject(this);
	//ev.ResumePropagation(wxEVENT_PROPAGATE_MAX);

	wxCommandEvent MyEvent( wxEVT_COMMAND_BUTTON_CLICKED ); // Keep it simple, don't give a specific event ID
	MyEvent.SetString(m_sText);
	wxPostEvent(this, MyEvent); // This posts to ourselves: it'll be caught and sent to a different method
//wxPostEvent(pBar, MyEvent); // Posts to a different class, Bar, where pBar points to an instance of Bar

	//wxEvtHandler::AddPendingEvent(ev);
	//wxPostEvent(this->GetParent()->GetEventHandler(), ev);
	//this->ProcessEvent(ev);
	evt.Skip();
}

IMPLEMENT_DYNAMIC_CLASS(vvTextLinkControl, wxPanel);
BEGIN_EVENT_TABLE(vvTextLinkControl, wxPanel)
	EVT_LEFT_UP(vvTextLinkControl::OnClick)
END_EVENT_TABLE()
