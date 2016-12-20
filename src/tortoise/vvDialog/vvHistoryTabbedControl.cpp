#include "precompiled.h"
#include <wx/notebook.h>
#include <wx/thread.h>
#include "vvHistoryTabbedControl.h"
#include "vvVerbs.h"
#include "vvHistoryObjects.h"
#include "vvContext.h"
#include "vvSgHelpers.h"
#include "vvRepoLocator.h"
#include "wx/persist.h"
#include "vvWxHelpers.h"

vvHistoryTabbedControl::vvHistoryTabbedControl()
{
}

///void vvHistoryTabbedControl::OnListScroll(wxScrollEvent& event)
///{
///	if (event.GetPosition() > 95)
///	{
		///((vvHistoryModel*)m_list->GetModel())->GetMoreData();
	///}
///}
vvHistoryTabbedControl::vvHistoryTabbedControl(
	vvVerbs& cVerbs,
	vvContext& cContext,
	const vvRepoLocator& cRepoLocator,
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator,
	const wxString& sChangesetHID
	) : wxPanel(pParent, cId,cPosition, cSize, iStyle, wxT("History List Control") )
{
	cValidator;
	cVerbs;
	
	m_pContext = &cContext;
	m_pVerbs = &cVerbs;
	wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

	m_notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM);
	
	//Putting these in an extra panel was necessary, because
	//The options control was hanging when I put it into the notebook directly.
	wxPanel * p1 = new wxPanel(m_notebook);
	wxBoxSizer * sizer2 = new wxBoxSizer(wxHORIZONTAL);
	m_listControl = new vvHistoryListControl(cVerbs, cContext, cRepoLocator, p1, wxID_ANY, wxDefaultPosition, wxDefaultSize, iStyle);
	sizer2->Add(m_listControl, 1, wxEXPAND);
	sizer2->SetSizeHints(p1);
	p1->SetSizer(sizer2);

	wxPersistenceManager::Get().RegisterAndRestore(m_listControl);

	wxPanel * p2 = new wxPanel(m_notebook);
	wxBoxSizer * sizer3 = new wxBoxSizer(wxHORIZONTAL);
	m_optionsControl = new vvHistoryOptionsControl(cVerbs, cContext, cRepoLocator.GetRepoName(), p2, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, sChangesetHID);

	sizer3->Add(m_optionsControl, 1, wxEXPAND);
	sizer3->SetSizeHints(p2);
	p2->SetSizer(sizer3);
	
	m_notebook->AddPage(p1, "Results", true);
	m_notebook->AddPage(p2, "Options", false);

	sizer->Add(m_notebook, 1, wxEXPAND);

	sizer->SetSizeHints(this);
	wxPanel::SetSizer(sizer);
}

bool vvHistoryTabbedControl::Create(
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

void vvHistoryTabbedControl::LoadData(
	vvHistoryFilterOptions& filterOptions)
{
	m_listControl->LoadHistory(filterOptions);
}

void vvHistoryTabbedControl::ApplyDefaultOptions(vvHistoryFilterOptions& filterOptions)
{
	vvHistoryColumnPreferences columnPrefs;
	//Apply the column prefs first, so that the persisted settings
	//are reflected by the control.
	this->m_listControl->ApplyColumnPreferences();
	//Ask the control for the current settings
	this->m_listControl->GetColumnSettings(columnPrefs);
	//Let the history control know what the current settings are.
	this->m_optionsControl->ApplyColumnPreferences(columnPrefs);
	
	this->m_optionsControl->ApplyHistoryFilterOptions(filterOptions);
}

void vvHistoryTabbedControl::OnTabChanging(wxNotebookEvent& ev)
{
	if (ev.GetOldSelection() == 1)
	{
		//m_listControl->KillBackgroundThread();

		//They're moving from the options page to the history page.
		//Get the new filter options, and then apply it to the list control.
		vvHistoryFilterOptions newFilterOptions;
		if (m_optionsControl->HaveFilterOptionsChanged())
		{
			vvHistoryFilterOptions newFilterOptions = m_optionsControl->GetHistoryFilterOptions();
			LoadData(newFilterOptions);
		}
		vvHistoryColumnPreferences columnPrefs = m_optionsControl->GetColumnPreferences();
		m_listControl->SetColumnPreferences(columnPrefs);
		m_listControl->ApplyColumnPreferences();
	}
}

wxArrayString vvHistoryTabbedControl::GetSelection()
{
	return this->m_listControl->GetSelection();
}

IMPLEMENT_DYNAMIC_CLASS(vvHistoryTabbedControl, wxPanel);
BEGIN_EVENT_TABLE(vvHistoryTabbedControl, wxPanel)
	EVT_NOTEBOOK_PAGE_CHANGING( wxID_ANY, vvHistoryTabbedControl::OnTabChanging)
END_EVENT_TABLE()
