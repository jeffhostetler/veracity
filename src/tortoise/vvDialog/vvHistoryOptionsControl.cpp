#include "precompiled.h"
#include "wx/propgrid/propgrid.h"
#include <wx/propgrid/advprops.h>
#include "vvHistoryOptionsControl.h"
#include "vvVerbs.h"
#include "vvContext.h"
#include "vvRepoBrowserDialog.h"
#include "vvSgHelpers.h"
#include "vvBranchSelectDialog.h"

class vvPopUpChoiceProperty : public wxLongStringProperty
{
public:
	enum Prop_Mode
	{
		MODE_USERS,
		MODE_STAMPS,
		MODE_TAGS,
		MODE_BRANCHES,
		MODE_REPO
	};
	DECLARE_DYNAMIC_CLASS(vvPopUpChoiceProperty)
	vvPopUpChoiceProperty::vvPopUpChoiceProperty( Prop_Mode mode = MODE_USERS, vvVerbs * cVerbs = NULL, vvContext * cContext = NULL, wxString sRepoName = wxEmptyString, const wxString& name = wxPG_LABEL, const wxString& label = wxPG_LABEL, const wxString& value  = wxEmptyString, const wxString& sChangesetHID = wxEmptyString)
							: wxLongStringProperty(name, label, value)
	{
		m_pVerbs = cVerbs;
		m_pContext = cContext;
		m_sRepoName = sRepoName;
		m_mode = mode;
		m_flags |= wxPG_PROP_NO_ESCAPE;
		m_sChangesetHID = sChangesetHID;
	}
	vvPopUpChoiceProperty::~vvPopUpChoiceProperty()
	{
	}

	bool vvPopUpChoiceProperty::OnButtonClick( wxPropertyGrid* propGrid, wxString& value )
	{
		propGrid;
		if (m_mode == MODE_REPO && m_sChangesetHID != wxEmptyString)
		{
			vvRepoBrowserDialog dialog = vvRepoBrowserDialog();
			dialog.Create(propGrid, m_sRepoName, m_sChangesetHID, value, true, *m_pVerbs, *m_pContext);
			if (dialog.ShowModal() == wxID_OK)
			{
				value = dialog.GetSelection();
				return true;
			}
		}
		else if (m_mode == MODE_BRANCHES)
		{
			vvBranchSelectDialog bsd;
			bsd.Create(propGrid, vvRepoLocator::FromRepoName(m_sRepoName), value, true, *m_pVerbs, *m_pContext);
			if (bsd.ShowModal() == wxID_OK)
			{
				value = bsd.GetSelectedBranch();
				return true;
			}
		}
		else
		{
			wxArrayString aChoices;
			vvVerbs::BranchList branchList;
			wxSingleChoiceDialog cDialog;
			wxString dialogTitle;
			wxString dialogMessage;
			{
				wxBusyCursor cBusyCursor;
				switch(m_mode)
				{
					case MODE_USERS:
						{
							dialogTitle = "Select User";
							dialogMessage = "Select one of the users below:";
							vvVerbs::stlUserDataList cUsers;
							if (!m_pVerbs->GetUsers(*m_pContext, m_sRepoName, cUsers))
							{
								m_pContext->Log_ReportError_CurrentError();
								m_pContext->Error_Reset();
								return false;
							}
							for (vvVerbs::stlUserDataList::const_iterator it = cUsers.begin(); it != cUsers.end(); ++it)
							{
								aChoices.Add(it->sName);
							}
						}
						break;
					case MODE_TAGS:
						{
							dialogTitle = "Select Tag";
							dialogMessage = "Select one of the tags below:";
							vvVerbs::stlTagDataList cTags;
							if (!m_pVerbs->GetTags(*m_pContext, m_sRepoName, cTags))
							{
								m_pContext->Log_ReportError_CurrentError();
								m_pContext->Error_Reset();
								return false;
							}
							vvSgHelpers::Convert_listTagData_wxArrayString(*m_pContext, cTags, aChoices);
						}
						break;
					case MODE_STAMPS:
						{
							dialogTitle = "Select Stamp";
							dialogMessage = "Select one of the stamps below:";
							vvVerbs::stlStampDataList cStamps;
							if (!m_pVerbs->GetStamps(*m_pContext, m_sRepoName, cStamps))
							{
								m_pContext->Log_ReportError_CurrentError();
								m_pContext->Error_Reset();
								return false;
							}
							vvSgHelpers::Convert_listStampData_wxArrayString(*m_pContext, cStamps, aChoices);
						}
						break;
					default:
						break;
				}
				aChoices.Sort();
				if (!cDialog.Create(propGrid, dialogMessage, dialogTitle, aChoices))
				{
					wxLogError("Error creating selection dialog.");
					return false;
				}
				if (!value.IsEmpty() && aChoices.Index(value) != wxNOT_FOUND)
				{
					cDialog.SetSelection(aChoices.Index(value));
				}
			}

			// show the dialog
			if (cDialog.ShowModal() == wxID_OK && cDialog.GetSelection() != wxNOT_FOUND)
			{
				if (m_mode != MODE_BRANCHES)
					value = aChoices[cDialog.GetSelection()];
				else
					value = branchList[cDialog.GetSelection()].sBranchName;
				return true;
			}
		}
		return false;
	}

private:
	vvVerbs * m_pVerbs;
	vvContext * m_pContext;
	wxString m_sRepoName;
	wxString m_sChangesetHID;
	Prop_Mode m_mode;
};

IMPLEMENT_DYNAMIC_CLASS(vvPopUpChoiceProperty, wxLongStringProperty)

vvHistoryOptionsControl::vvHistoryOptionsControl()
{
}

vvHistoryOptionsControl::vvHistoryOptionsControl(
	vvVerbs& cVerbs,
	vvContext& cContext,
	const wxString& sRepoName,
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator,
	const wxString& sChangesetHID
	) : wxPanel(pParent, cId,cPosition, cSize, iStyle, wxT("History Options Control") )
{
	cValidator;
	m_propGrid = NULL;
	wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
	m_propGrid = new wxPropertyGrid(this,-1,wxDefaultPosition,cSize,
                        wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER/* | wxPG_LIMITED_EDITING */);
	sizer->Add(m_propGrid, 1, wxEXPAND);
	sizer->SetSizeHints(this);
	wxPanel::SetSizer(sizer);

	m_propGrid->Append( new wxPropertyCategory(wxT("Filter")) );
	
	//
	m_propGrid->Append( new vvPopUpChoiceProperty(vvPopUpChoiceProperty::MODE_USERS, &cVerbs, &cContext, sRepoName, wxT("User"),
                                    wxPG_LABEL
									) );

	//vvPopUpChoiceProperty::MODE_
	wxPGProperty * pgprop = m_propGrid->Append( new vvPopUpChoiceProperty(vvPopUpChoiceProperty::MODE_REPO, &cVerbs, &cContext, sRepoName, wxT("File or Folder"),
		wxPG_LABEL, wxEmptyString, sChangesetHID
									) );
	//wxPGProperty * pgprop = m_propGrid->Append( new wxStringProperty(wxT("File or Folder"),
      //                                      wxPG_LABEL));
	pgprop->AppendChild( new wxBoolProperty(wxT("Hide Merges"), wxPG_LABEL, false) );

	m_propGrid->Append( new vvPopUpChoiceProperty(vvPopUpChoiceProperty::MODE_BRANCHES, &cVerbs, &cContext, sRepoName, wxT("Branch"),
                                    wxPG_LABEL
									) );
	
	m_propGrid->Append( new vvPopUpChoiceProperty(vvPopUpChoiceProperty::MODE_STAMPS, &cVerbs, &cContext, sRepoName, wxT("Stamp"),
                                    wxPG_LABEL
									) );

	m_propGrid->Append( new vvPopUpChoiceProperty(vvPopUpChoiceProperty::MODE_TAGS, &cVerbs, &cContext, sRepoName, wxT("Tag"),
                                    wxPG_LABEL
									) );
	
	wxDateProperty * dp = new wxDateProperty(wxT("Earliest Date"),
                                    wxPG_LABEL,
                                    wxInvalidDateTime) ;
	dp->SetAttribute("PickerStyle", wxDP_ALLOWNONE | wxDP_DROPDOWN|wxDP_SHOWCENTURY);
    m_propGrid->Append( dp );

	wxDateProperty * dp2 = new wxDateProperty(wxT("Latest Date"),
                                    wxPG_LABEL,
                                    wxInvalidDateTime) ;
	dp2->SetAttribute("PickerStyle", wxDP_ALLOWNONE | wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	m_propGrid->Append( dp2 );

	m_propGrid->Append( new wxPropertyCategory(wxT("Columns")) );
    m_propGrid->Append( new wxBoolProperty(wxT("Revision"), "ColumnRevision", true) );
	m_propGrid->Append( new wxBoolProperty(wxT("Comment"), "ColumnComment", true) );
	m_propGrid->Append( new wxBoolProperty(wxT("Branch"), "ColumnBranch", true) );
	m_propGrid->Append( new wxBoolProperty(wxT("Stamp"), "ColumnStamp", true) );
	m_propGrid->Append( new wxBoolProperty(wxT("Tag"), "ColumnTag", true) );
	m_propGrid->Append( new wxBoolProperty(wxT("Date"), "ColumnDate", true) );
	m_propGrid->Append( new wxBoolProperty(wxT("User"), "ColumnUser", true) );
	//m_propGrid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_TAB );
	m_pVerbs = &cVerbs;
	m_pContext = &cContext;
	m_sRepoName = sRepoName;

}

bool vvHistoryOptionsControl::Create(
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

void vvHistoryOptionsControl::ApplyColumnPreferences(vvHistoryColumnPreferences& columnPrefs)
{
	m_propGrid->GetProperty("ColumnRevision")->SetValue(columnPrefs.ShowRevisionColumn);
	m_propGrid->GetProperty("ColumnComment")->SetValue(columnPrefs.ShowCommentColumn);
	m_propGrid->GetProperty("ColumnBranch")->SetValue(columnPrefs.ShowBranchColumn);
	m_propGrid->GetProperty("ColumnStamp")->SetValue(columnPrefs.ShowStampColumn);
	m_propGrid->GetProperty("ColumnTag")->SetValue(columnPrefs.ShowTagColumn);
	m_propGrid->GetProperty("ColumnDate")->SetValue(columnPrefs.ShowDateColumn);
	m_propGrid->GetProperty("ColumnUser")->SetValue(columnPrefs.ShowUserColumn);
}


vvHistoryColumnPreferences vvHistoryOptionsControl::GetColumnPreferences()
{
	vvHistoryColumnPreferences columnPrefs;
	columnPrefs.ShowRevisionColumn = m_propGrid->GetProperty("ColumnRevision")->GetValue();
	columnPrefs.ShowCommentColumn = m_propGrid->GetProperty("ColumnComment")->GetValue();
	columnPrefs.ShowBranchColumn = m_propGrid->GetProperty("ColumnBranch")->GetValue();
	columnPrefs.ShowStampColumn = m_propGrid->GetProperty("ColumnStamp")->GetValue();
	columnPrefs.ShowTagColumn = m_propGrid->GetProperty("ColumnTag")->GetValue();
	columnPrefs.ShowDateColumn = m_propGrid->GetProperty("ColumnDate")->GetValue();
	columnPrefs.ShowUserColumn = m_propGrid->GetProperty("ColumnUser")->GetValue();
	return columnPrefs;
}

void vvHistoryOptionsControl::ApplyHistoryFilterOptions(vvHistoryFilterOptions& filterOptions)
{
	m_propGrid->GetProperty("User")->SetValue(filterOptions.sUser);
	m_propGrid->GetProperty("File or Folder")->SetValue(filterOptions.sFileOrFolderPath);
	m_propGrid->GetProperty("File or Folder")->GetPropertyByName("Hide Merges")->SetValue(filterOptions.bHideMerges);
	m_propGrid->GetProperty("Branch")->SetValue(filterOptions.sBranch);
	m_propGrid->GetProperty("Tag")->SetValue(filterOptions.sTag);
	m_propGrid->GetProperty("Stamp")->SetValue(filterOptions.sStamp);
	m_propGrid->GetProperty("Earliest Date")->SetValue(filterOptions.dBeginDate);
	m_propGrid->GetProperty("Latest Date")->SetValue(filterOptions.dEndDate);
	m_previousFilterOptions = filterOptions;
}

vvHistoryFilterOptions vvHistoryOptionsControl::GetHistoryFilterOptions()
{
	vvHistoryFilterOptions filterOptions = GetHistoryFilterOptions__dont_save();
	m_previousFilterOptions = filterOptions;
	return filterOptions;
}

vvHistoryFilterOptions vvHistoryOptionsControl::GetHistoryFilterOptions__dont_save()
{
	vvHistoryFilterOptions filterOptions;
	if (m_propGrid->IsEditorFocused())
	{
		//This is necessary, because the selected date fields weren't committing their change.
		m_propGrid->ClearSelection();
		m_propGrid->CommitChangesFromEditor();
	}
	filterOptions.sUser = (wxString)m_propGrid->GetProperty("User")->GetValueAsString();
	filterOptions.sFileOrFolderPath = (wxString)m_propGrid->GetProperty("File or Folder")->GetValueAsString();
	filterOptions.bHideMerges = (bool)m_propGrid->GetProperty("File or Folder")->GetPropertyByName("Hide Merges")->GetValue();
	filterOptions.sBranch = (wxString)m_propGrid->GetProperty("Branch")->GetValueAsString();
	filterOptions.sTag = (wxString)m_propGrid->GetProperty("Tag")->GetValueAsString();
	filterOptions.sStamp = (wxString)m_propGrid->GetProperty("Stamp")->GetValueAsString();
	filterOptions.dBeginDate = wxInvalidDateTime;
	if (((wxDateProperty*)m_propGrid->GetProperty("Earliest Date"))->GetValueAsString() != wxEmptyString)
	{
		filterOptions.dBeginDate = ((wxDateProperty*)m_propGrid->GetProperty("Earliest Date"))->GetDateValue();
		filterOptions.dBeginDate.ResetTime(); //Sets to 00:00:00
	}
	filterOptions.dEndDate = wxInvalidDateTime;
	if (((wxDateProperty*)m_propGrid->GetProperty("Latest Date"))->GetValueAsString() != wxEmptyString)
	{
		filterOptions.dEndDate = ((wxDateProperty*)m_propGrid->GetProperty("Latest Date"))->GetDateValue();
		filterOptions.dEndDate.SetHour(23);
		filterOptions.dEndDate.SetMinute(59);
		filterOptions.dEndDate.SetSecond(59);
		filterOptions.dEndDate.SetMillisecond(999);
	}
	return filterOptions;
}

bool vvHistoryOptionsControl::HaveFilterOptionsChanged()
{
	vvHistoryFilterOptions newFilterOptions = GetHistoryFilterOptions__dont_save();
	return !(newFilterOptions.sUser == m_previousFilterOptions.sUser 
		&& newFilterOptions.sFileOrFolderPath == m_previousFilterOptions.sFileOrFolderPath
		&& newFilterOptions.bHideMerges == m_previousFilterOptions.bHideMerges
		&& newFilterOptions.sBranch == m_previousFilterOptions.sBranch
		&& newFilterOptions.sTag == m_previousFilterOptions.sTag
		&& newFilterOptions.sStamp == m_previousFilterOptions.sStamp
				//Either they are both valid and equal, or they are both invalid.
		&& (   (newFilterOptions.dBeginDate.IsValid() && m_previousFilterOptions.dBeginDate.IsValid()
				   && newFilterOptions.dBeginDate == m_previousFilterOptions.dBeginDate)
				|| (!newFilterOptions.dBeginDate.IsValid() && !m_previousFilterOptions.dBeginDate.IsValid())  )
				//Either they are both valid and equal, or they are both invalid.
		&& (   (newFilterOptions.dEndDate.IsValid() && m_previousFilterOptions.dEndDate.IsValid()
				   && newFilterOptions.dEndDate == m_previousFilterOptions.dEndDate)
				|| (!newFilterOptions.dEndDate.IsValid() && !m_previousFilterOptions.dEndDate.IsValid())  )
				);
}

IMPLEMENT_DYNAMIC_CLASS(vvHistoryOptionsControl, wxPanel);

BEGIN_EVENT_TABLE(vvHistoryOptionsControl, wxPanel)
END_EVENT_TABLE()


