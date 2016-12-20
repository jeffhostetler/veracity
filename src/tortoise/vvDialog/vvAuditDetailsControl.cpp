#include "precompiled.h"
#include <wx/notebook.h>
#include <wx/thread.h>
#include "vvAuditDetailsControl.h"
#include "vvVerbs.h"
#include "vvHistoryObjects.h"
#include "vvContext.h"
#include "vvSgHelpers.h"

vvAuditDetailsControl::vvAuditDetailsControl()
{
}

vvAuditDetailsControl::vvAuditDetailsControl(
	vvVerbs& cVerbs,
	vvContext& cContext,
	vvVerbs::AuditEntry auditEntry,
	wxString sComment,
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
	cVerbs;
	auditEntry;
	m_pContext = &cContext;
	m_pVerbs = &cVerbs;
	// create the dialog from XRC resources
//	this->Create(pParent, cId, cPosition, cSize, iStyle, cValidator);
	if (!wxXmlResource::Get()->LoadPanel(this, pParent, "vvAuditDetailsControl"))
	{
		wxLogError("Couldn't load dialog layout from resources.");
	}
	this->MSWSetTransparentBackground(true);
	XRCCTRL(*this, "vvAuditDetailsControl", wxPanel)->MSWSetTransparentBackground(true);
	wxTextCtrl * pUserTextCtrl = XRCCTRL(*this, "wUserText",                 wxTextCtrl);
	if (!auditEntry.sWho.IsEmpty())
		pUserTextCtrl->SetValue(auditEntry.sWho);
	else
		pUserTextCtrl->SetValue("Unknown User");
	wxTextCtrl * pDateTextCtrl = XRCCTRL(*this, "wDateText",                 wxTextCtrl);
	if (auditEntry.nWhen.IsValid())
		pDateTextCtrl->SetValue(auditEntry.nWhen.Format());
	else
		pDateTextCtrl->SetValue("Unknown date");
	wxTextCtrl * pCommentTextCtrl = XRCCTRL(*this, "wCommentText",                 wxTextCtrl);
	pCommentTextCtrl->SetValue(sComment);
	pCommentTextCtrl->SetBackgroundColour(this->GetBackgroundColour());
}

bool vvAuditDetailsControl::Create(
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

IMPLEMENT_DYNAMIC_CLASS(vvAuditDetailsControl, wxPanel);
BEGIN_EVENT_TABLE(vvAuditDetailsControl, wxPanel)
END_EVENT_TABLE()
