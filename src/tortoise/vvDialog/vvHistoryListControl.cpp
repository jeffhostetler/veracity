#include "precompiled.h"
#include <wx/dataview.h>
#include <wx/thread.h>
#include "vvHistoryListControl.h"
#include "vvVerbs.h"
#include "vvHistoryObjects.h"
#include "vvContext.h"
#include "vvSgHelpers.h"
#include "vvDataViewToggleIconTextRenderer.h"
#include "vvRevDetailsDialog.h"
#include "vvStatusDialog.h"
#include "vvRevisionChoiceDialog.h"
#include "vvUpdateCommand.h"
#include "vvMergeCommand.h"
#include "vvProgressExecutor.h"
#include "vvDiffCommand.h"
#include "vvDiffThread.h"

#define ID_DETAILS wxID_HIGHEST + 25
#define ID_UPDATE wxID_HIGHEST + 26
#define ID_MERGE wxID_HIGHEST + 27

static wxString History_sDiff_WorkingFolder__text("Against &Working Copy");
#define ID_DIFF_WORKINGFOLDER wxID_HIGHEST + 29
static wxString History_sDiff_Baseline__text("Against &Baseline");
#define ID_DIFF_BASELINE wxID_HIGHEST + 30
static wxString History_sDiff_Baseline2__text("Against &Merge Target");
#define ID_DIFF_BASELINE2 wxID_HIGHEST + 31
static wxString History_sDiff_Parent__text("Against &Parent Changeset");
#define ID_DIFF_PARENT wxID_HIGHEST + 32
static wxString History_sDiff_Branch__text("Against \"%s\" &Branch");
#define ID_DIFF_BRANCH wxID_HIGHEST + 33
#define ID_DIFF_OTHER wxID_HIGHEST + 40
#define ID_DIFF_TWO wxID_HIGHEST + 45

wxDECLARE_EVENT(wxEVT_COMMAND_MODEL_FETCH_MORE__COMPLETE, wxThreadEvent);
/**
 * Models data about the history in a repository.
 */
class vvHistoryModel : public wxDataViewVirtualListModel
{
// types
public:
	/**
	 * Enumeration of columns used by the model.
	 *
	 * The B/S/U prefixes indicate the value type of the column (Bool, String, Unsigned).
	 * The prefix codes are always listed alphabetically, if it helps you remember the names.
	 *
	 * B and S values use variant types "bool" and "string" respectively.
	 * U values use variant type "long", but are actually cast unsigned ints, and should be cast back to unsigned int before use.
	 */
	enum Column
	{
		//COLUMN_B_CHECKED, //< Item's checked state.
		COLUMN_S_REVISION,
		COLUMN_S_COMMENT,
		COLUMN_S_BRANCH,
		COLUMN_S_STAMP,
		COLUMN_S_TAG,
		COLUMN_U_DATE,
		COLUMN_S_USER,
		COLUMN_COUNT,     //< Number of entries in this enumeration, for iteration.
	};

// construction
public:
	/**
	 * Default constructor.
	 */
	vvHistoryModel(vvVerbs& cVerbs,	vvContext& cContext, vvHistoryListControl * parent, wxString sRepoName);

// functionality
public:
	
	/**
	 * Removes all items from the data model.
	 */
	void Clear();

// implementation
public:
	void LoadHistory(vvContext * pContext);
	void GetMoreData(vvContext * pContext);
	void SetData(vvVerbs::HistoryData data);
 	virtual unsigned int GetColumnCount() const;
	virtual unsigned int vvHistoryModel::GetCount() const;
	virtual wxString GetColumnType(
		unsigned int uColumn //< [in] The column to get the type of.
		) const;
	virtual  bool vvHistoryModel::GetAttrByRow	(	unsigned int 	row,
			unsigned int 	col,
			wxDataViewItemAttr & 	attr
		)		 const;

	void GetValueByRow(
		wxVariant&            cValue,
		unsigned int 		  row,
		unsigned int 		  uColumn
		) const;

	bool SetValueByRow(const wxVariant &variant,
	                               unsigned row, unsigned col);

	void SetFilter(vvHistoryFilterOptions vvHistoryFilter);
	vvVerbs::HistoryEntry GetHistoryDataAt(int row) const
	{
		vvHistoryModel* nonConst = (vvHistoryModel*)this;
		wxCriticalSectionLocker lock(nonConst->m_dataAccessLock);
		return this->mcDataItems[row];
	}

private:
	vvVerbs * mpVerbs;
	vvContext * mpContext;
	
	void * pHistoryToken;
	unsigned int nEndOfPage;
	wxCriticalSection m_lock;
	wxCriticalSection m_fetchCheckCriticalSection;
	wxCriticalSection m_dataAccessLock;
	bool m_bAlreadyFetching;
	wxString m_sRepoName;
	vvVerbs::HistoryData mcDataItems; //< The data items that we're displaying.
	vvHistoryFilterOptions m_historyFilter;
	wxCriticalSection m_historyFilterLock;
// macro declarations
private:
	vvHistoryListControl * mpListControl;

	vvDISABLE_COPY(vvHistoryModel);
};

vvHistoryModel::vvHistoryModel(vvVerbs& cVerbs,	vvContext& cContext, vvHistoryListControl * parent, wxString sRepoName)
{
	mpVerbs = &cVerbs;
	mpContext = &cContext;
	pHistoryToken = NULL;
	nEndOfPage = 0;
	mpListControl = parent;
	m_sRepoName = sRepoName;
	m_bAlreadyFetching = false;
}

void vvHistoryModel::Clear()
{
	{
		wxCriticalSectionLocker lock(m_dataAccessLock);
		this->mcDataItems.clear();
		nEndOfPage = wxUINT32_MAX;
	}
	this->Reset(0);
}

void vvHistoryModel::SetData(vvVerbs::HistoryData data)
{
	Clear();
	wxCriticalSectionLocker lock(this->m_dataAccessLock);
	this->mcDataItems = data;
	this->pHistoryToken = NULL;
	this->Reset(mcDataItems.size());
}

unsigned int vvHistoryModel::GetCount() const
{
	vvHistoryModel* nonConst = (vvHistoryModel*)this;
	wxCriticalSectionLocker lock(nonConst->m_dataAccessLock);
	return this->mcDataItems.size();
}

void vvHistoryModel::SetFilter(vvHistoryFilterOptions vvHistoryFilter)
{
	wxCriticalSectionLocker lock(m_historyFilterLock);
	m_historyFilter = vvHistoryFilter;
}
void vvHistoryModel::LoadHistory(vvContext *pContext)
{
	if (pContext == NULL)
		pContext = mpContext;
	vvVerbs::HistoryData newData;
	{
		vvHistoryFilterOptions tempFilterOptions;
		{
			wxCriticalSectionLocker lock(m_historyFilterLock);
			tempFilterOptions = m_historyFilter;
		}
		mpListControl->SendStatusMessage("Loading...", false);
		if (!mpVerbs->History__Free_Token(*pContext, this->pHistoryToken))
		{
			pContext->Log_ReportError_CurrentError();
			pContext->Error_Reset();
			mpListControl->SendStatusMessage("Failed to load history.", false);
			return;
		}
		this->pHistoryToken = NULL;
		if (mpVerbs->History(*pContext, m_sRepoName, tempFilterOptions, newData, &pHistoryToken))
		{
			wxCriticalSectionLocker lock(m_dataAccessLock);
			mcDataItems = newData;
			this->Reset(newData.size());
		}
		else
		{
			pContext->Log_ReportError_CurrentError();
			pContext->Error_Reset();
			mpListControl->SendStatusMessage("Failed to load history.", false);
			return;
		}
	}
	
	int count = 0;
	{
		wxCriticalSectionLocker lock(m_dataAccessLock);
		count = mcDataItems.size();
	}
	if (count != 1)
		mpListControl->SendStatusMessage(wxString::Format("%d results", count), pHistoryToken != NULL);
	else
		mpListControl->SendStatusMessage(wxString::Format("%d result", count), pHistoryToken != NULL);
	
	if (pHistoryToken != NULL)
		nEndOfPage = newData.size() - 10;
	else
		nEndOfPage = wxUINT32_MAX;
	
	//m_bAlreadyFetching = false;

}

void vvHistoryModel::GetMoreData(vvContext *pContext)
{
	void * pNewHistoryToken = NULL;
	vvVerbs::HistoryData cDataItemsNew;
	if (pContext == NULL)
		pContext = mpContext;
	//wxCriticalSectionLocker lock(m_lock);
	{
		wxCriticalSectionLocker lock(m_fetchCheckCriticalSection);
		m_bAlreadyFetching = true;
	}

	if (this->pHistoryToken)
	{
		mpListControl->SendStatusMessage("Loading More...", false);
		if (mpVerbs->History__Fetch_More(*pContext, m_sRepoName, 200, this->pHistoryToken, cDataItemsNew, &pNewHistoryToken))
		{
			wxCriticalSectionLocker lock(m_dataAccessLock);
			mcDataItems.insert(mcDataItems.end(), cDataItemsNew.begin(), cDataItemsNew.end());
			this->Reset(mcDataItems.size());
			this->pHistoryToken = pNewHistoryToken;
			this->nEndOfPage = mcDataItems.size() - 10;
		}
		else
		{
			pContext->Log_ReportError_CurrentError();
			pContext->Error_Reset();
			this->pHistoryToken = NULL;
			mpListControl->SendStatusMessage("Failed to load more history.", false);
			return;
		}
		int count = mcDataItems.size();
		if (count != 1)
			mpListControl->SendStatusMessage(wxString::Format("%d results", count), pHistoryToken != NULL);
		else
			mpListControl->SendStatusMessage(wxString::Format("%d result", count), pHistoryToken != NULL);
	}
	{
		wxCriticalSectionLocker lock(m_fetchCheckCriticalSection);
		m_bAlreadyFetching = false;
	}

}

//This seems to never be called.
unsigned int vvHistoryModel::GetColumnCount() const
{
	return vvHistoryModel::COLUMN_COUNT;
}

//This seems to never be called.
wxString vvHistoryModel::GetColumnType(
	unsigned int uColumn
	) const
{
	switch (static_cast<Column>(uColumn))
	{
	//case COLUMN_B_CHECKED:
	//	return "bool";

	case COLUMN_S_REVISION:
	case COLUMN_S_COMMENT:
	case COLUMN_S_BRANCH:
	case COLUMN_S_TAG:
	case COLUMN_S_STAMP:
	case COLUMN_S_USER:
		return "string";

	case COLUMN_U_DATE:
		return "date";

	default:
		wxLogError("Unknown StampsModel column: %u", uColumn);
		return wxEmptyString;
	}
}

bool vvHistoryModel::SetValueByRow(const wxVariant &variant,
                               unsigned row, unsigned col)
{
	row;
	col;
	variant;
	return true;
}

bool vvHistoryModel::GetAttrByRow	(	unsigned int 	row,
			unsigned int 	col,
			wxDataViewItemAttr & 	attr
			)		 const
{
	vvVerbs::HistoryEntry he = GetHistoryDataAt(row);
	if(he.bIsMissing)
	{
		attr.SetColour(wxColour("red"));
		return true;
	}
	return false;
}


void vvHistoryModel::GetValueByRow(
	wxVariant&            cValue,
	unsigned int 		  row,
	unsigned int 		  uColumn
	) const
{
	row;
	vvVerbs::HistoryEntry entry;
	//if (static_cast<Column>(uColumn) == COLUMN_B_CHECKED)
	//	cValue = "false";
	//if (m_bAlreadyFetching) //We're fetching data right now.
	//	return;
	vvHistoryModel* nonConst = (vvHistoryModel*)this;
	{
		wxCriticalSectionLocker lock(nonConst->m_dataAccessLock);
		if (row >= this->mcDataItems.size())
			return;
		 entry = this->mcDataItems[row];
	}
	if (row >= this->nEndOfPage)
	{
		bool bGoAheadAndFetch = false;
		{
			wxCriticalSectionLocker lock2(nonConst->m_fetchCheckCriticalSection);
			if (m_bAlreadyFetching == false)
			{
			  bGoAheadAndFetch = true;
			}
		}
		if (bGoAheadAndFetch)
		{
			mpListControl->StartGetMoreData(false);
		}
	}
	wxString rev;
	switch (static_cast<Column>(uColumn))
	{
	//case COLUMN_B_CHECKED:
		//Todo: Jeremy should fix this.
		//cValue = true;
		//return;

	case COLUMN_S_REVISION:
		rev << entry.nRevno;
		rev << ":";
		rev << entry.sChangesetHID;
		cValue = rev;
		return;

	case COLUMN_S_COMMENT:
		if (entry.cComments.size() > 0)
		{
			wxString comment = entry.cComments[0].sCommentText;
			comment.Replace("\r", " ", true);
			comment.Replace("\n", " ", true);
			cValue = comment;
		}
		return;

	case COLUMN_S_BRANCH:
		for (unsigned int i = 0; i < entry.cBranches.size(); i++)
		{
			if (i > 0)
				rev << ", ";
			rev << entry.cBranches[i];
		}
		cValue = rev;
		return;

	case COLUMN_S_STAMP:
		for (unsigned int i = 0; i < entry.cStamps.size(); i++)
		{
			if (i > 0)
				rev << ", ";
			rev << entry.cStamps[i];
		}
		cValue = rev;
		return;

	case COLUMN_S_TAG:
		for (unsigned int i = 0; i < entry.cTags.size(); i++)
		{
			if (i > 0)
				rev << ", ";
			rev << entry.cTags[i];
		}
		cValue = rev;
		return;

	case COLUMN_U_DATE:
		if (entry.cAudits.size() > 0)
			cValue = entry.cAudits[0].nWhen;
		return;

	case COLUMN_S_USER:
			if (entry.cAudits.size() > 0)
				cValue = entry.cAudits[0].sWho;
			return;
	default:
		wxLogError("Unknown StampsModel column: %u", uColumn);
		cValue.MakeNull();
		return;
	}
}


vvHistoryListControl::vvHistoryListControl()
{
}

vvHistoryListControl::vvHistoryListControl(
	vvVerbs& cVerbs,
	vvContext& cContext,
	const vvRepoLocator& cRepoLocator,
	wxWindow*          pParent,
	wxWindowID         cId,
	const wxPoint&     cPosition,
	const wxSize&      cSize,
	long               iStyle,
	const wxValidator& cValidator
	) : wxPanel(pParent, cId,cPosition, cSize, iStyle, wxT("History List Control") )
{
	cValidator;
	cVerbs;
	&cContext;

	m_bRevisionsAreRemote = false;

	m_list = NULL;
	wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
	if ((iStyle & wxDV_MULTIPLE) == 0)
	{
		//If they didn't request Multiple, then assume they want single.
		iStyle |= wxDV_SINGLE;
	}
	m_list = new wxDataViewListCtrl(this,-1,wxDefaultPosition,cSize,
			wxDV_HORIZ_RULES | iStyle);
	sizer->Add(m_list, 1, wxEXPAND);
	sizer->SetSizeHints(this);
	wxPanel::SetSizer(sizer);
	m_list->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(vvHistoryListControl::OnContext), NULL, this);
	m_pVerbs = &cVerbs;
	m_pContext = &cContext;
	m_cRepoLocator = cRepoLocator;

	if (m_cRepoLocator.IsWorkingFolder())
	{
		if (!m_pVerbs->FindWorkingFolder(*m_pContext, m_cRepoLocator.GetWorkingFolder(), NULL, NULL, &m_aBaselineHids))
		{
			m_pContext->Log_ReportError_CurrentError();
			m_pContext->Error_Reset();
			return;
		}
	}
	this->SetMinSize(wxSize(120, 170));
}

bool vvHistoryListControl::Create(
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

void vvHistoryListControl::LoadHistory(
	vvVerbs::HistoryData data,
	bool bAreRemote
	)
{
	m_bRevisionsAreRemote = bAreRemote;
	((vvHistoryModel*)m_list->GetModel())->SetData(data);
}

void vvHistoryListControl::LoadHistory(
		vvHistoryFilterOptions& vvHistoryFilter
		)
{
	{
		m_bRevisionsAreRemote = false;
		m_historyFilter = vvHistoryFilter;
		((vvHistoryModel*)m_list->GetModel())->SetFilter(vvHistoryFilter);
	}
	StartGetMoreData(true);
}

void vvHistoryListControl::SendStatusMessage(wxString sStatusString, bool bHasMoreData)
{
	wxCommandEvent ev(newEVT_HISTORY_STATUS, GetId() );

	ev.SetString(sStatusString);
	ev.SetInt(bHasMoreData);
	ev.SetEventObject(this);
	this->ProcessEvent(ev);
}

void vvHistoryListControl::KillBackgroundThread()
{
	if (GetThread()->IsRunning())
		GetThread()->Kill();
}

void vvHistoryListControl::StartGetMoreData(bool bClearModelFirst)
{
	//Try to wait for the other thread to finish.
	if (GetThread() != NULL && GetThread()->IsAlive())
		GetThread()->Wait();

	if (bClearModelFirst)
		((vvHistoryModel*)m_list->GetModel())->Clear();

	// we want to start a long task, but we don't want our GUI to block
    // while it's executed, so we use a thread to do it.
	if (GetThread() == NULL || !GetThread()->IsAlive())
		if ( CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)
		{
			wxLogError("Could not create the worker thread!");
			return;
		}


    // go!
	if (!GetThread()->IsRunning())
		GetThread()->Run();
}

wxThread::ExitCode vvHistoryListControl::Entry()
{
    // IMPORTANT:
    // this function gets executed in the secondary thread context!
	try
	{
		vvContext NewContext = vvContext();
		if (((vvHistoryModel*)m_list->GetModel())->GetCount() == 0)
		{
			((vvHistoryModel*)m_list->GetModel())->LoadHistory(&NewContext);
		}
		else
			((vvHistoryModel*)m_list->GetModel())->GetMoreData(&NewContext);
	}
	catch (std::exception e)
	{
		int tmp = 4;
		tmp++;
	}


    return (wxThread::ExitCode)0;
}

vvVerbs::HistoryData vvHistoryListControl::GetSelectedHistoryEntries()
{
	vvVerbs::HistoryData returnVal;
	wxDataViewItemArray selectionArray;
	int selectionCount = m_list->GetSelections(selectionArray);
	for (int selectionIndex = 0; selectionIndex < selectionCount; selectionIndex++)
	{
		int row = ((vvHistoryModel*)m_list->GetModel())->GetRow(selectionArray[selectionIndex]);
		returnVal.push_back(((vvHistoryModel*)m_list->GetModel())->GetHistoryDataAt(row));
	}
  
	return returnVal;
}
wxArrayString vvHistoryListControl::GetSelection()
{
	wxArrayString returnVal;
	wxDataViewItemArray selectionArray;
	int selectionCount = m_list->GetSelections(selectionArray);
	for (int selectionIndex = 0; selectionIndex < selectionCount; selectionIndex++)
	{
		int row = ((vvHistoryModel*)m_list->GetModel())->GetRow(selectionArray[selectionIndex]);
		returnVal.Add(((vvHistoryModel*)m_list->GetModel())->GetHistoryDataAt(row).sChangesetHID);
	}
  
	return returnVal;
}

void SetColumnWidth(wxDataViewColumn * column, int width)
{
	if (width > 0)
		column->SetWidth(width);
}

void vvHistoryListControl::OnContext(wxDataViewEvent& event)
{
	event;
	event.GetItem();
	{
		{
			wxMenu menu;
			wxArrayString selection = GetSelection();
			if (selection.Count() == 1)
				menu.Append(ID_DETAILS, wxT("&Details"), wxT(""));
			wxMenu * submenu = new wxMenu();
			
			if (selection.Count() == 1)
			{
				if (m_cRepoLocator.IsWorkingFolder())
				{
					//TODO: Restore this, when we can diff arbitrary changeset versus the working folder.
					//submenu->Append(ID_DIFF_WORKINGFOLDER, History_sDiff_WorkingFolder__text, wxT(""));
					submenu->Append(ID_DIFF_BASELINE, History_sDiff_Baseline__text, wxT(""));
				}
				submenu->Append(ID_DIFF_PARENT, History_sDiff_Parent__text, wxT(""));
				if (m_cRepoLocator.IsWorkingFolder())
				{
					wxString sBranch;
					if (!m_pVerbs->GetBranch(*m_pContext, m_cRepoLocator.GetWorkingFolder(), sBranch))
					{
						m_pContext->Log_ReportError_CurrentError();
						m_pContext->Error_Reset();
						return;
					}
					if (!sBranch.IsEmpty())
						submenu->Append(ID_DIFF_BRANCH, wxString::Format(History_sDiff_Branch__text, sBranch), wxT(""));
				}
				if (!m_bRevisionsAreRemote)
					menu.AppendSubMenu(submenu, wxT("D&iff"), wxT(""));
			}
			else if (!m_bRevisionsAreRemote && selection.Count() == 2)
			{
				menu.Append(ID_DIFF_TWO, wxT("D&iff"), wxT(""));
			}

			//submenu->Append(ID_DIFF_OTHER, "Other...", wxT(""));
				
			if (selection.Count() == 1 && !m_bRevisionsAreRemote && m_cRepoLocator.IsWorkingFolder())
			{
				menu.Append(ID_UPDATE, wxT("&Update"), wxT(""));
				if (m_aBaselineHids.Count() == 1) //Only put merge on the menu if we're not already merged.
					menu.Append(ID_MERGE, wxT("&Merge"), wxT(""));
			}
				
			PopupMenu(&menu);
		}
	}
}
vvNullable<wxString> vvHistoryListControl::ChooseOneRevision(wxArrayString& cRevArray, const wxString& sTitle, const wxString& sDescription)
{
	vvRevisionChoiceDialog * vrcd = new vvRevisionChoiceDialog();
	vvVerbs::HistoryData data;
	for (unsigned int i = 0; i < cRevArray.Count(); i++)
	{
		vvVerbs::HistoryEntry e;
		vvVerbs::RevisionList children;
		if (!m_pVerbs->Get_Revision_Details(*m_pContext, m_cRepoLocator.GetRepoName(), cRevArray[i], e, children))
		{
			m_pContext->Error_ResetReport("Could not load details for revision: " + cRevArray[i]);
			return vvNULL;
		}
		data.push_back(e);
	}
	vrcd->Create(this, m_cRepoLocator, data, sTitle, sDescription, *m_pVerbs, *m_pContext);
	if (vrcd->ShowModal() == wxID_OK)
	{
		vrcd->Destroy();
		return vrcd->GetSelection();
	}
	else
	{
		vrcd->Destroy();
		return vvNULL;
	}
	
}
void vvHistoryListControl::LaunchDiff(wxString rev1, wxString rev2)
{
	//The goal of this next section is to determine if the requested object is a file.
	//If it is, then launch diffmerge instead of the status dialog-based diff.
	bool bIsFile = false;
	wxString path = m_historyFilter.sFileOrFolderPath;
	if (!m_historyFilter.sFileOrFolderPath.IsEmpty())
	{
		//Get the revision from the latest history result, since the path to the file or folder 
		//is probably correct in that one.  It's better to try that than do nothing.
		wxString sChangesetHID;
		if (!rev1.IsEmpty())
			sChangesetHID = rev1;
		else if (!rev2.IsEmpty())
			sChangesetHID = rev2;
		
		wxString sGid;
		wxString sDiskPath;

		if (m_cRepoLocator.IsWorkingFolder())
		{
			if (m_pVerbs->GetAbsolutePath(*m_pContext, m_cRepoLocator.GetWorkingFolder(), path, sDiskPath))
			{
				if (m_pVerbs->GetGid(*m_pContext, sDiskPath, sGid))
				{
					path = "@" + sGid;
				}
				else
					m_pContext->Error_Reset();
			}
			else
				m_pContext->Error_Reset();
		}

		vvVerbs::RepoItem cDetails;
		if (!m_pVerbs->Repo_Item_Get_Details(*m_pContext, m_cRepoLocator.GetWorkingFolder(), sChangesetHID, path, cDetails))
		{
			m_pContext->Error_ResetReport("Could not find repo path " + m_historyFilter.sFileOrFolderPath + " in changeset: " +sChangesetHID);
			return;
		}
		bIsFile = cDetails.eItemType == vvVerbs::REGULAR_FILE;
	}
	if (bIsFile)
	{
		vvDiffCommand * pCommand = new vvDiffCommand();		
		pCommand->SetVerbs(m_pVerbs);
		pCommand->SetExecuteData(m_cRepoLocator, path, rev1, rev2); 
		//It is specifically OK to new this without explicitly 
		//deleting it.  The wxThread class always destructs itself
		//when the thread exits.
		vvDiffThread * bg = new vvDiffThread(&pCommand);
		bg;
	}
	else
	{
		vvStatusDialog * vsd = new vvStatusDialog();
		vsd->Create(this, m_cRepoLocator, *m_pVerbs, *m_pContext, rev1, rev2 );

		vsd->Show();
	}
}
void vvHistoryListControl::OnContextMenuItem(wxCommandEvent& event)
{
	wxArrayString selection = GetSelection();
	if (selection.Count() != 1 && selection.Count() != 2)
	{
		return;
	}

	wxString sCurrentlySelectedRevision = GetSelection()[0];
	if (event.GetId() == ID_DETAILS)
	{
		//Details
		LaunchRevisionDetailsDialog();

	}
	else if (event.GetId() == ID_UPDATE)
	{
		//Update
		vvUpdateCommand command = vvUpdateCommand();		
		command.SetVerbs(m_pVerbs);
		vvRevSpec cRevSpec(*m_pContext);
		cRevSpec.AddRevision(*m_pContext, sCurrentlySelectedRevision);
		command.SetExecuteData(m_cRepoLocator, cRevSpec);
		vvProgressExecutor exec = vvProgressExecutor(command);
		exec.Execute(this);
	}
	else if (event.GetId() == ID_MERGE)
	{
		//Merge
		vvMergeCommand command = vvMergeCommand();		
		command.SetVerbs(m_pVerbs);
		vvRevSpec cRevSpec(*m_pContext);
		cRevSpec.AddRevision(*m_pContext, sCurrentlySelectedRevision);
		command.SetExecuteData(*m_pContext, m_cRepoLocator, cRevSpec);
		vvProgressExecutor exec = vvProgressExecutor(command);
		exec.Execute(this);

		//Since we just changed the baseline, refresh our array of baseline HIDs
		m_aBaselineHids.Clear();
		if (!m_pVerbs->FindWorkingFolder(*m_pContext, m_cRepoLocator.GetWorkingFolder(), NULL, NULL, &m_aBaselineHids))
		{
			m_pContext->Log_ReportError_CurrentError();
			m_pContext->Error_Reset();
			return;
		}
	}
	else if (event.GetId() == ID_DIFF_WORKINGFOLDER)
	{
		LaunchDiff(sCurrentlySelectedRevision);
	}
	else if (event.GetId() == ID_DIFF_BASELINE)
	{
		if (m_aBaselineHids.Count() > 1)
		{
			vvNullable<wxString> chosenHID = ChooseOneRevision(m_aBaselineHids, wxString("Choose a Baseline"), wxString("The working copy has an uncommitted merge.  Please choose one revision."));
			if (!chosenHID.IsNull())
				LaunchDiff(chosenHID, sCurrentlySelectedRevision);
		}
		else
		{
			LaunchDiff(m_aBaselineHids[0], sCurrentlySelectedRevision);
			
		}
	}
	else if (event.GetId() == ID_DIFF_BRANCH)
	{
		wxString sBranch;
		wxArrayString sBranchHeadHids;
		if (!m_pVerbs->GetBranch(*m_pContext, m_cRepoLocator.GetWorkingFolder(), sBranch))
		{
			m_pContext->Log_ReportError_CurrentError();
			m_pContext->Error_Reset();
			return;
		}
		if (!m_pVerbs->GetBranchHeads(*m_pContext, m_cRepoLocator.GetRepoName(), sBranch, sBranchHeadHids) )
		{
			m_pContext->Log_ReportError_CurrentError();
			m_pContext->Error_Reset();
			return;
		}

		if (sBranchHeadHids.Count() > 1)
		{
			vvNullable<wxString> chosenHID = ChooseOneRevision(sBranchHeadHids, "Multiple Branch Heads", "The \"" + sBranch + "\" branch has multiple heads.  Please choose one revision" );
			if (!chosenHID.IsNull())
				LaunchDiff(chosenHID, sCurrentlySelectedRevision);
		}
		else
		{
			LaunchDiff( sBranchHeadHids[0], sCurrentlySelectedRevision);
		}
	}
	else if (event.GetId() == ID_DIFF_PARENT)
	{
		vvVerbs::HistoryEntry he = GetSelectedHistoryEntries()[0];
		if (he.cParents.size() > 1)
		{
			//This doesn't use the usual ChooseOneRevision function, since this has an array of revisions, instead of wxArrayString
			vvRevisionChoiceDialog * vrcd = new vvRevisionChoiceDialog();
			vvVerbs::HistoryData data;
			for (unsigned int i = 0; i < he.cParents.size(); i++)
			{
				vvVerbs::HistoryEntry e;
				vvVerbs::RevisionList children;
				if (!m_pVerbs->Get_Revision_Details(*m_pContext, m_cRepoLocator.GetRepoName(), he.cParents[i].sChangesetHID, e, children))
				{
					m_pContext->Log_ReportError_CurrentError();
					m_pContext->Error_Reset();
					return;
				}
				data.push_back(e);
			}
			vrcd->Create(this, m_cRepoLocator, data, "Choose a Parent", "The selected revision has multiple parents.  Please choose one parent.", *m_pVerbs, *m_pContext);
			if (vrcd->ShowModal() == wxID_OK)
			{
				LaunchDiff(vrcd->GetSelection(), he.sChangesetHID);
			}
			vrcd->Destroy();
		}
		else
		{
			LaunchDiff(he.cParents[0].sChangesetHID, he.sChangesetHID);
		}
	}
	else if (event.GetId() == ID_DIFF_TWO)
	{
		LaunchDiff(selection[1], selection[0]); //GetSelection seems to return them reversed.  Weird
	}
	event.Skip();
}

void vvHistoryListControl::SetColumnPreferences(vvHistoryColumnPreferences& columnPrefs)
{
	m_columnPrefs = columnPrefs;
}

void vvHistoryListControl::ApplyColumnPreferences()
{
	if (m_list->GetColumnCount() == 0)
	{
		const int columnCount = 7;
		unsigned int model_column = 0;
		wxDataViewColumn* columnArray[columnCount];
	
		//columnArray[model_column++] = m_toggleColumn = new wxDataViewColumn( "", new wxDataViewToggleRenderer( wxT("bool"), wxDATAVIEW_CELL_ACTIVATABLE), model_column, 20);
		columnArray[model_column++] = m_revisionColumn = new wxDataViewColumn( "Revision", new wxDataViewTextRenderer( wxT("string"), wxDATAVIEW_CELL_INERT), model_column, 80, wxALIGN_LEFT, wxCOL_REORDERABLE | wxCOL_RESIZABLE);
		columnArray[model_column++] = m_commentColumn = new wxDataViewColumn( "Comment", new wxDataViewTextRenderer( wxT("string"), wxDATAVIEW_CELL_INERT), model_column, 80, wxALIGN_LEFT, wxCOL_REORDERABLE | wxCOL_RESIZABLE);
		columnArray[model_column++] = m_branchColumn = new wxDataViewColumn( "Branch", new wxDataViewTextRenderer( wxT("string"), wxDATAVIEW_CELL_INERT), model_column, 60, wxALIGN_LEFT, wxCOL_REORDERABLE | wxCOL_RESIZABLE);
		columnArray[model_column++] = m_stampColumn = new wxDataViewColumn( "Stamp", new wxDataViewTextRenderer( wxT("string"), wxDATAVIEW_CELL_INERT), model_column, 60, wxALIGN_LEFT, wxCOL_REORDERABLE | wxCOL_RESIZABLE);
		columnArray[model_column++] = m_tagColumn = new wxDataViewColumn( "Tag", new wxDataViewTextRenderer( wxT("string"), wxDATAVIEW_CELL_INERT), model_column, 60, wxALIGN_LEFT, wxCOL_REORDERABLE | wxCOL_RESIZABLE);
		columnArray[model_column++] = m_dateColumn = new wxDataViewColumn( "Date", new wxDataViewTextRenderer( wxT("datetime"), wxDATAVIEW_CELL_INERT), model_column, 60, wxALIGN_LEFT, wxCOL_REORDERABLE | wxCOL_RESIZABLE);
		columnArray[model_column++] = m_userColumn = new wxDataViewColumn( "User", new wxDataViewTextRenderer( wxT("string"), wxDATAVIEW_CELL_INERT), model_column, 80, wxALIGN_LEFT, wxCOL_REORDERABLE | wxCOL_RESIZABLE);

		if (m_columnPrefs.columnOrder.size() != columnCount)
		{
			for (int i=0; i < columnCount; i++)
				m_list->AppendColumn(columnArray[i]);
		}
		else
		{
			//The column order array contains numbers, which correspond to "where should I put the 
			//column which would normally go at the Nth position?"  For example, the array "0, 3, 2 1" means
			//that the revision column (usually column 1), should go at position 3 instead.
			//the comment column (usually column 2), should go at position 2 as normal.
			//the branch column (usually column 3), should go at position 1 instead.
			for (int i=0; i < columnCount; i++)
				for (int j=0; j < columnCount; j++)
				{
					if (m_columnPrefs.columnOrder[j] == i)
						m_list->AppendColumn(columnArray[j]);
				}
		}
		
		// create our model
		this->mpDataModel = new vvHistoryModel(*m_pVerbs, *m_pContext, this, m_cRepoLocator.GetRepoName());
		m_list->AssociateModel(this->mpDataModel.get());

		SetColumnWidth(m_revisionColumn, m_columnPrefs.WidthRevisionColumn);
		SetColumnWidth(m_commentColumn, m_columnPrefs.WidthCommentColumn);
		SetColumnWidth(m_branchColumn, m_columnPrefs.WidthBranchColumn);
		SetColumnWidth(m_stampColumn, m_columnPrefs.WidthStampColumn);
		SetColumnWidth(m_tagColumn, m_columnPrefs.WidthTagColumn);
		SetColumnWidth(m_dateColumn, m_columnPrefs.WidthDateColumn);
		SetColumnWidth(m_userColumn, m_columnPrefs.WidthUserColumn);
	}

	//Just do the hidden/shown settings 
	m_revisionColumn->SetHidden(!m_columnPrefs.ShowRevisionColumn);
	m_commentColumn->SetHidden(!m_columnPrefs.ShowCommentColumn);
	m_branchColumn->SetHidden(!m_columnPrefs.ShowBranchColumn);
	m_stampColumn->SetHidden(!m_columnPrefs.ShowStampColumn);
	m_tagColumn->SetHidden(!m_columnPrefs.ShowTagColumn);
	m_dateColumn->SetHidden(!m_columnPrefs.ShowDateColumn);
	m_userColumn->SetHidden(!m_columnPrefs.ShowUserColumn);
}

void vvHistoryListControl::GetColumnSettings(vvHistoryColumnPreferences& columnPrefs)
{
	columnPrefs.WidthRevisionColumn = m_revisionColumn->GetWidth();
	columnPrefs.WidthCommentColumn	= m_commentColumn->GetWidth();
	columnPrefs.WidthBranchColumn	= m_branchColumn->GetWidth();
	columnPrefs.WidthStampColumn	= m_stampColumn->GetWidth();
	columnPrefs.WidthTagColumn		= m_tagColumn->GetWidth();
	columnPrefs.WidthDateColumn		= m_dateColumn->GetWidth();
	columnPrefs.WidthUserColumn		= m_userColumn->GetWidth();
	columnPrefs.columnOrder.clear();

	columnPrefs.ShowRevisionColumn	= !m_revisionColumn->IsHidden();
	columnPrefs.ShowCommentColumn	= !m_commentColumn->IsHidden();
	columnPrefs.ShowBranchColumn	= !m_branchColumn->IsHidden();
	columnPrefs.ShowStampColumn		= !m_stampColumn->IsHidden();
	columnPrefs.ShowTagColumn		= !m_tagColumn->IsHidden();
	columnPrefs.ShowDateColumn		= !m_dateColumn->IsHidden();
	columnPrefs.ShowUserColumn		= !m_userColumn->IsHidden();

	//columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_toggleColumn));
	columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_revisionColumn));
	columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_commentColumn));
	columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_branchColumn));
	columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_stampColumn));
	columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_tagColumn));
	columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_dateColumn));
	columnPrefs.columnOrder.Add(m_list->GetColumnPosition(m_userColumn));
	
}

void vvHistoryListControl::SetListBackgroundColour(wxColor color)
{
	m_list->SetBackgroundColour(color);
	m_list->Refresh();
}

wxColor vvHistoryListControl::GetListBackgroundColour()
{
	return m_list->GetBackgroundColour();
}

void vvHistoryListControl::LaunchRevisionDetailsDialog()
{
	wxArrayString selection = GetSelection();
	if (selection.Count() == 1)
	{
		wxString sCurrentlySelectedRevision = selection[0];
		vvRevDetailsDialog * rdd = new vvRevDetailsDialog();
		rdd->Create(this, m_cRepoLocator, sCurrentlySelectedRevision, *m_pVerbs, *m_pContext);
		if (m_bRevisionsAreRemote)
		{
			vvVerbs::HistoryEntry he = GetSelectedHistoryEntries()[0];
			rdd->SetRevision(he);
		}
		rdd->ShowModal();
	}
}

void vvHistoryListControl::OnDoubleClick(wxDataViewEvent& evt)
{
	evt;
	LaunchRevisionDetailsDialog();
}

wxDEFINE_EVENT(wxEVT_COMMAND_MODEL_FETCH_MORE__COMPLETE, wxThreadEvent);
IMPLEMENT_DYNAMIC_CLASS(vvHistoryListControl, wxDataViewCtrl);
BEGIN_EVENT_TABLE(vvHistoryListControl, wxPanel)
	EVT_MENU(wxID_ANY,   vvHistoryListControl::OnContextMenuItem)
	EVT_DATAVIEW_ITEM_ACTIVATED(wxID_ANY, vvHistoryListControl::OnDoubleClick)
	//EVT_MENU(ID_UPDATE,   vvHistoryListControl::OnContextMenuItem)
END_EVENT_TABLE()
