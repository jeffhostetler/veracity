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
#include "vvResolveDialog.h"

#include "vvContext.h"
#include "vvCppUtilities.h"
#include "vvDialogHeaderControl.h"
#include "vvSelectableStaticText.h"
#include "vvVerbs.h"
#include "vvWxHelpers.h"


/*
**
** Types
**
*/

/**
 * Contains data about a pair of values that is commonly diffed.
 */
struct CommonValuePair
{
	wxString sName;  //< User-friendly name of the pair.
	wxString sLeft;  //< Label of the left value in the pair.
	wxString sRight; //< Label of the right value in the pair.
};

/**
 * A function that determines whether or not a column should be used.
 */
typedef bool ValueColumnPredicate(
	const vvVerbs::ResolveChoice* pChoice //< [in] Choice that the values in the list will be from.
	);

/**
 * A function that gets the value to display in a column.
 */
typedef wxString ValueColumnFunctor(
	const vvVerbs::ResolveValue* pValue //< [in] The resolve value to get a column value for.
	);

/**
 * Data about a column in the value list.
 */
struct ValueColumn
{
	wxString              sName;      //< Name/header for the column.
	int                   iFormat;    //< Format flags for the column, i.e. wxLIST_FORMAT_*
	int                   iSize;      //< Initial width of the column.
	ValueColumnPredicate* fPredicate; //< Predicate that determines whether or not to use a column.
	                                  //< If NULL the column is always used.
	ValueColumnFunctor*   fFunctor;   //< Functor that retrieves a value for the column.
};

/**
 * A thread that runs resolve diffs.
 */
class _DiffThread : public wxThread
{
public:
	/**
	 * Constructor.
	 */
	_DiffThread(
		vvVerbs&                    cVerbs,  //< [in] Verbs to use to run the diff.
		const wxString&             sFolder, //< [in] Working copy to run the diff in.
		const wxString&             sGid,    //< [in] GID of the conflicting item.
		vvVerbs::ResolveChoiceState eState,  //< [in] Type of the conflict on the item.
		const wxString&             sLeft,   //< [in] Name of the value to diff on the left.
		const wxString&             sRight,  //< [in] Name of the value to diff on the right.
		wxEvtHandler*               pNotify  //< [in] Handler to notify when the diff finishes.
		                                     //<      Uses a _EVT_TOOL_COMPLETED event with ID _TOOL_DIFF.
		);

	/**
	 * Entry point.
	 */
	virtual ExitCode Entry();

private:
	vvVerbs*                    mpVerbs;
	vvContext                   mcContext;
	wxString                    msFolder;
	wxString                    msGid;
	vvVerbs::ResolveChoiceState meState;
	wxString                    msLeft;
	wxString                    msRight;
	wxEvtHandler*               mpNotify;
};

/**
 * A thread that runs resolve merges.
 */
class _MergeThread : public wxThread
{
public:
	/**
	 * Constructor.
	 */
	_MergeThread(
		vvVerbs&                    cVerbs,  //< [in] Verbs to use to run the merge.
		const wxString&             sFolder, //< [in] Working copy to run the merge in.
		const wxString&             sGid,    //< [in] GID of the conflicting item.
		vvVerbs::ResolveChoiceState eState,  //< [in] Type of the conflict on the item.
		wxEvtHandler*               pNotify  //< [in] Handler to notify when the merge finishes.
		                                     //<      Uses a _EVT_TOOL_COMPLETED event with ID _TOOL_MERGE.
		                                     //<      The event's int will contain zero on failure/cancellation or non-zero on success.
		                                     //<      If successful, the event's string will contain the new value's label.
		);

	/**
	 * Entry point.
	 */
	virtual ExitCode Entry();

private:
	vvVerbs*                    mpVerbs;
	vvContext                   mcContext;
	wxString                    msFolder;
	wxString                    msGid;
	vvVerbs::ResolveChoiceState meState;
	wxEvtHandler*               mpNotify;
};

/**
 * Event sent by _DiffThread and _MergeThread to notify of completion.
 */
wxDEFINE_EVENT(_EVT_TOOL_COMPLETED, wxThreadEvent);
#define EVT_TOOL_COMPLETED(id, func) wx__DECLARE_EVT1(_EVT_TOOL_COMPLETED, id, (&func))
enum
{
	_TOOL_DIFF,
	_TOOL_MERGE,
};


/*
**
** Globals
**
*/

/*
 * Labels for data in the header.
 */
static const wxString gsLabel_Header_Item     = "Item:";
static const wxString gsLabel_Header_Baseline = "Baseline:";
static const wxString gsLabel_Header_Conflict = "Conflict:";
static const wxString gsLabel_Header_Cause    = "Cause:";
static const wxString gsLabel_Header_Related  = "Related:";
static const wxString gsLabel_Header_Status   = "Status:";

/*
 * Constant values for the header.
 */
static const wxString gsValue_Header_Baseline_NonExistant = "(non-existent)";

/*
 * Formats for header values.
 */
static const wxString gsItemFormat_Existing     = "%s";
static const wxString gsItemFormat_Deleted      = "%s (non-existent)";
static const wxString gsConflictFormat          = "%s (%s)";
static const wxString gsCauseFormat             = "%s (%s)";
static const wxString gsStatusFormat_Resolved   = "Resolved (accepted %s)";
static const wxString gsStatusFormat_Unresolved = "Unresolved";

/*
 * Formats for buttons.
 */
static const wxString gsDiffFormat_Selected   = "Diff %s vs. %s";
static const wxString gsDiffFormat_Menu       = "Diff";
static const wxString gsAcceptFormat_None     = "Accept";
static const wxString gsAcceptFormat_Selected = "Accept %s";

/**
 * List of commonly diffed value pairs.
 */
static const CommonValuePair gaCommonValuePairs[] =
{
	{ "Baseline Changes", "ancestor",  "baseline" },
	{ "Other Changes",    "ancestor",  "other"    },
	{ "Incoming Changes", "baseline",  "working"  },
	{ "Existing Changes", "other",     "working"  },
	{ "Working Changes",  "automerge", "working"  },
	{ "Total Changes",    "ancestor",  "working"  },
};

/*
 * Prototypes of ValueColumnPredicate functions.
 */
bool _Choice_Mergeable   (const vvVerbs::ResolveChoice* pChoice);
bool _Choice_NonMergeable(const vvVerbs::ResolveChoice* pChoice);

/*
 * Prototypes of ValueColumnFunctor functions.
 */
wxString _Value_Label  (const vvVerbs::ResolveValue* pValue);
wxString _Value_Size   (const vvVerbs::ResolveValue* pValue);
wxString _Value_Summary(const vvVerbs::ResolveValue* pValue);

/**
 * List of data for value columns.
 * Indexed by ValueColumn values.
 */
static const ValueColumn gaValueColumns[] =
{
	{ "Label",       wxLIST_FORMAT_LEFT,   75, NULL,                 _Value_Label   },
	{ "Size",        wxLIST_FORMAT_RIGHT,  50, _Choice_Mergeable,    _Value_Size    },
	{ "Description", wxLIST_FORMAT_LEFT,  390, _Choice_Mergeable,    _Value_Summary },
	{ "Value",       wxLIST_FORMAT_LEFT,  440, _Choice_NonMergeable, _Value_Summary },
};


/*
**
** Internal Functions
**
*/

/**
 * Formats a value label for display.
 */
static wxString _FormatLabel(
	const wxString& sLabel //< [in] The label to format.
	)
{
	return sLabel.Capitalize();
}

/**
 * Formats a disk size for display.
 */
wxString _FormatSize(wxUint64 uSize)
{
	struct ScaleFactor
	{
		wxUint64 uFactor;
		wxString sFormat;
	};

	static const ScaleFactor aScaleFactors[] =
	{
		{ 1000000000000u, "%llut" },
		{    1000000000u, "%llug" },
		{       1000000u, "%llum" },
		{          1000u, "%lluk" },
	};

	static const wxString sDefaultFormat = "%llu";

	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(aScaleFactors); ++uIndex)
	{
		const ScaleFactor* pFactor = aScaleFactors + uIndex;

		if (uSize >= pFactor->uFactor)
		{
			return wxString::Format(pFactor->sFormat, uSize / pFactor->uFactor);
		}
	}

	return wxString::Format(sDefaultFormat, uSize);
}

/**
 * Sort callback for lists of values.
 *
 * Note: Documentation for wxListCtrl::SortItems says that the first two parameters
 *       should be wxIntPtr.  However, code says they should be long.  Even though
 *       they are type long, it would appear they are still memory addresses and not
 *       the indices of items.
 */
static int wxCALLBACK _ListCtrlSort_Values(
	long     pItem1,   //< [in] vvVerbs::ResolveValue being compared on the left.
	long     pItem2,   //< [in] vvVerbs::ResolveValue being compared on the right.
	wxIntPtr pUserData //< [in] Not used.
	)
{
	vvVerbs::ResolveValue* pValue1 = (vvVerbs::ResolveValue*)pItem1;
	vvVerbs::ResolveValue* pValue2 = (vvVerbs::ResolveValue*)pItem2;

	wxUnusedVar(pUserData);

	if (pValue1->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_CHANGESET) != pValue2->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_CHANGESET))
	{
		if (pValue1->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_CHANGESET))
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (pValue1->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_AUTOMATIC) != pValue2->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_AUTOMATIC))
	{
		if (pValue1->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_AUTOMATIC))
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return pValue1->sLabel.compare(pValue2->sLabel);
	}
}

/**
 * ValueColumnPredicate function that checks if the choice is mergeable.
 */
bool _Choice_Mergeable(const vvVerbs::ResolveChoice* pChoice)
{
	return pChoice->bMergeable;
}

/**
 * ValueColumnPredicate function that checks if the choice is not mergeable.
 */
bool _Choice_NonMergeable(const vvVerbs::ResolveChoice* pChoice)
{
	return !pChoice->bMergeable;
}

/**
 * ValueColumnFunctor function that retrieves the value's label.
 */
wxString _Value_Label(const vvVerbs::ResolveValue* pValue)
{
	return _FormatLabel(pValue->sLabel);
}

/**
 * ValueColumnFunctor function that retrieves the value's size.
 */
wxString _Value_Size(const vvVerbs::ResolveValue* pValue)
{
	return _FormatSize(pValue->uSize);
}

/**
 * ValueColumnFunctor function that retrieves the value's summary.
 */
wxString _Value_Summary(const vvVerbs::ResolveValue* pValue)
{
	return pValue->sSummary;
}

_DiffThread::_DiffThread(
	vvVerbs&                    cVerbs,
	const wxString&             sFolder,
	const wxString&             sGid,
	vvVerbs::ResolveChoiceState eState,
	const wxString&             sLeft,
	const wxString&             sRight,
	wxEvtHandler*               pNotify
	)
	: wxThread(wxTHREAD_JOINABLE)
	, mcContext()
	, mpVerbs(&cVerbs)
	, msFolder(sFolder)
	, msGid(sGid)
	, meState(eState)
	, msLeft(sLeft)
	, msRight(sRight)
	, mpNotify(pNotify)
{
}

wxThread::ExitCode _DiffThread::Entry()
{
	if (!this->mpVerbs->Resolve_Diff(this->mcContext, this->msFolder, this->msGid, this->meState, this->msLeft, this->msRight))
	{
		this->mcContext.Error_ResetReport("Failed to diff values '%s' and '%s'.", _FormatLabel(this->msLeft), _FormatLabel(this->msRight));
	}

	if (this->mpNotify != NULL)
	{
		this->mpNotify->QueueEvent(new wxThreadEvent(_EVT_TOOL_COMPLETED, _TOOL_DIFF));
	}

	return NULL;
}

_MergeThread::_MergeThread(
	vvVerbs&                    cVerbs,
	const wxString&             sFolder,
	const wxString&             sGid,
	vvVerbs::ResolveChoiceState eState,
	wxEvtHandler*               pNotify
	)
	: wxThread(wxTHREAD_JOINABLE)
	, mcContext()
	, mpVerbs(&cVerbs)
	, msFolder(sFolder)
	, msGid(sGid)
	, meState(eState)
	, mpNotify(pNotify)
{
}

wxThread::ExitCode _MergeThread::Entry()
{
	wxString sLabel;
	int      iSuccess = 1;

	if (!this->mpVerbs->Resolve_Merge(this->mcContext, this->msFolder, this->msGid, this->meState, vvVerbs::sszResolveLabel_Baseline, vvVerbs::sszResolveLabel_Other, wxEmptyString, wxEmptyString, NULL, &sLabel))
	{
		this->mcContext.Error_ResetReport("Failed to merge values '%s' and '%s'.", _FormatLabel(vvVerbs::sszResolveLabel_Baseline), _FormatLabel(vvVerbs::sszResolveLabel_Other));
		iSuccess = 0;
	}
	else if (sLabel.IsEmpty())
	{
		iSuccess = 0;
	}

	if (this->mpNotify != NULL)
	{
		wxThreadEvent* pEvent = new wxThreadEvent(_EVT_TOOL_COMPLETED, _TOOL_MERGE);
		pEvent->SetInt(iSuccess);
		pEvent->SetString(sLabel);
		this->mpNotify->QueueEvent(pEvent);
	}

	return NULL;
}


/*
**
** Public Functions
**
*/

vvResolveDialog::vvResolveDialog()
	: meState(vvVerbs::RESOLVE_STATE_COUNT)
	, mpChoice(NULL)
	, mpThread(NULL)
	, mwHeader(NULL)
	, mwValues(NULL)
	, mwDiff(NULL)
	, mwDiffMenu(NULL)
	, mwMerge(NULL)
	, mwAccept(NULL)
{
}

bool vvResolveDialog::Create(
	wxWindow*                   wParent,
	const wxString&             sAbsolutePath,
	vvVerbs::ResolveChoiceState eState,
	vvVerbs&                    cVerbs,
	vvContext&                  cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(wParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// store parameters that we can store directly
	this->meState = eState;

	// look up the working folder and repo associated with the given path
	wxString sRepo = wxEmptyString;
	if (!cVerbs.FindWorkingFolder(cContext, sAbsolutePath, &this->msFolder, &sRepo))
	{
		cContext.Error_ResetReport("Cannot find the working folder for path: %s", sAbsolutePath);
		return false;
	}

	// convert the absolute path into a GID
	if (!cVerbs.GetGid(cContext, sAbsolutePath, this->msGid))
	{
		cContext.Error_ResetReport("Cannot find GID for item: %s", sAbsolutePath);
		return false;
	}

	// convert the rest of the absolute path into a repo path
	wxString sRepoPath = wxEmptyString;
	if (!cVerbs.GetRepoPath(cContext, sAbsolutePath, this->msFolder, sRepoPath))
	{
		cContext.Error_ResetReport("Cannot convert absolute path to repo path: %s", sAbsolutePath);
		return false;
	}

	// get the resolve data we need
	if (!this->GetResolveData())
	{
		return false;
	}

	// get all of our widgets
	this->mwValues = XRCCTRL(*this, "wValues",     wxListCtrl);
	this->mwDiff   = XRCCTRL(*this, "wDiff",       wxButton);
	this->mwMerge  = XRCCTRL(*this, "wMerge",      wxButton);
	this->mwAccept = XRCCTRL(*this, "wxID_OK",     wxButton);

	// create our header control
	this->mwHeader = new vvDialogHeaderControl(this);
	this->mwHeader->AddFolderValue(this->msFolder, cVerbs, cContext);
	this->mwHeader->AddTextValue(gsLabel_Header_Item, sRepoPath);
	if (this->mcItem.sRepoPath_Parents.IsEmpty() || this->mcItem.sRepoPath_Parents[0].IsEmpty())
	{
		this->mwHeader->AddTextValue(gsLabel_Header_Baseline, gsValue_Header_Baseline_NonExistant);
	}
	else
	{
		this->mwHeader->AddTextValue(gsLabel_Header_Baseline, this->mcItem.sRepoPath_Parents[0]);
	}
	this->mwHeader->AddTextValue(gsLabel_Header_Conflict, wxString::Format(gsConflictFormat, this->mpChoice->sName.Capitalize(), this->mpChoice->sCommand));
	for (vvVerbs::stlResolveCauseMap::const_iterator it = this->mpChoice->cCauses.begin(); it != this->mpChoice->cCauses.end(); ++it)
	{
		this->mwHeader->AddTextValue(gsLabel_Header_Cause, wxString::Format(gsCauseFormat, it->first, it->second));
	}
	for (vvVerbs::stlResolveRelatedMap::const_iterator it = this->mpChoice->cColliding.begin(); it != this->mpChoice->cColliding.end(); ++it)
	{
		if (it->second.sRepoPath_Working.IsEmpty())
		{
			this->mwHeader->AddTextValue(gsLabel_Header_Related, "(non-existent)");
		}
		else
		{
			this->mwHeader->AddControlValue(gsLabel_Header_Related, new wxHyperlinkCtrl(this->mwHeader, wxID_ANY, it->second.sRepoPath_Working, it->second.sRepoPath_Working, wxDefaultPosition, wxDefaultSize, wxHL_ALIGN_LEFT), true);
		}
	}
	for (vvVerbs::stlResolveRelatedMap::const_iterator it = this->mpChoice->cCycling.begin(); it != this->mpChoice->cCycling.end(); ++it)
	{
		if (it->second.sRepoPath_Working.IsEmpty())
		{
			this->mwHeader->AddTextValue(gsLabel_Header_Related, "(non-existent)");
		}
		else
		{
			this->mwHeader->AddControlValue(gsLabel_Header_Related, new wxHyperlinkCtrl(this->mwHeader, wxID_ANY, it->second.sRepoPath_Working, it->second.sRepoPath_Working, wxDefaultPosition, wxDefaultSize, wxHL_ALIGN_LEFT), true);
		}
	}
	if (this->mpChoice->bResolved)
	{
		wxPanel* wPanel  = new wxPanel(this->mwHeader);
		wxBoxSizer* pSizer = new wxBoxSizer(wxHORIZONTAL);
		pSizer->Add(new wxStaticBitmap(wPanel, wxID_ANY, wxArtProvider::GetBitmap("Status_Resolved_16")), wxSizerFlags());
		pSizer->AddSpacer(4);
		pSizer->Add(new vvSelectableStaticText(wPanel, wxID_ANY, wxString::Format(gsStatusFormat_Resolved, _FormatLabel(this->mpChoice->sResolution))), wxSizerFlags(1));
		wPanel->SetSizer(pSizer);
		this->mwHeader->AddControlValue(gsLabel_Header_Status, wPanel, true);
	}
	else
	{
		wxPanel* wPanel  = new wxPanel(this->mwHeader);
		wxBoxSizer* pSizer = new wxBoxSizer(wxHORIZONTAL);
		pSizer->Add(new wxStaticBitmap(wPanel, wxID_ANY, wxArtProvider::GetBitmap("Status_Conflict_16")), wxSizerFlags());
		pSizer->AddSpacer(4);
		pSizer->Add(new vvSelectableStaticText(wPanel, wxID_ANY, wxString::Format(gsStatusFormat_Unresolved)), wxSizerFlags(1));
		wPanel->SetSizer(pSizer);
		this->mwHeader->AddControlValue(gsLabel_Header_Status, wPanel, true);
	}
	wxXmlResource::Get()->AttachUnknownControl("wHeader", this->mwHeader);

	// create the appropriate columns in the value list
	long iColumn = 0;
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaValueColumns); ++uIndex)
	{
		const ValueColumn* pColumn = gaValueColumns + uIndex;

		if (pColumn->fPredicate == NULL || pColumn->fPredicate(this->mpChoice))
		{
			this->mwValues->InsertColumn(iColumn, pColumn->sName, pColumn->iFormat, pColumn->iSize);
			++iColumn;
		}
	}

	// show/hide the mergeable tools
	vvXRCSIZERITEM(*this, "wMergeable")->Show(this->mpChoice->bMergeable);

	// populate the menu of common diff pairs
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaCommonValuePairs); ++uIndex)
	{
		const CommonValuePair* pPair = gaCommonValuePairs + uIndex;
		if (pPair->sName.IsEmpty())
		{
			this->mwDiffMenu.AppendSeparator();
		}
		else
		{
			this->mwDiffMenu.Append(uIndex, wxString::Format("%s (%s vs. %s)", pPair->sName, _FormatLabel(pPair->sLeft), _FormatLabel(pPair->sRight)));
		}
	}

	// pre-select the live working value if it's been edited (if its value differs from its parent's)
	for (vvVerbs::stlResolveValueMap::const_iterator it = this->mpChoice->cValues.begin(); it != this->mpChoice->cValues.end(); ++it)
	{
		const vvVerbs::ResolveValue& cCurrent = it->second;

		if (cCurrent.cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_LIVE) && !cCurrent.sBaseline.IsEmpty())
		{
			const vvVerbs::ResolveValue& cParent = this->mpChoice->cValues[cCurrent.sBaseline];

			if (cCurrent.cData.HasSameType(cParent.cData))
			{
				if (
					   (cCurrent.cData.CheckType<bool>()     && (cCurrent.cData.As<bool>()     != cParent.cData.As<bool>()))
					|| (cCurrent.cData.CheckType<wxInt64>()  && (cCurrent.cData.As<wxInt64>()  != cParent.cData.As<wxInt64>()))
					|| (cCurrent.cData.CheckType<wxString>() && (cCurrent.cData.As<wxString>() != cParent.cData.As<wxString>()))
					)
				{
					this->mcValues.Add(it->first);
				}
			}
		}
	}

	// if we haven't pre-selected a value yet and this choice is resolved,
	// then pre-select the accepted value
	if (this->mcValues.size() == 0u && this->mpChoice->bResolved)
	{
		this->mcValues.Add(this->mpChoice->sResolution);
	}

	// success
	return true;
}

bool vvResolveDialog::TransferDataToWindow()
{
	// clear the existing values
	this->mwValues->DeleteAllItems();

	// add the choice's values to the list
	long iValue = 0;
	for (vvVerbs::stlResolveValueMap::const_iterator it = this->mpChoice->cValues.begin(); it != this->mpChoice->cValues.end(); ++it)
	{
		const vvVerbs::ResolveValue* pValue = &it->second;

		// add an item for this value
		this->mwValues->InsertItem(iValue, wxEmptyString);
		this->mwValues->SetItemPtrData(iValue, (wxUIntPtr)pValue);

		// set the value's data in each column
		long iColumn = 0;
		for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaValueColumns); ++uIndex)
		{
			const ValueColumn* pColumn = gaValueColumns + uIndex;

			if (pColumn->fPredicate == NULL || pColumn->fPredicate(this->mpChoice))
			{
				this->mwValues->SetItem(iValue, iColumn, pColumn->fFunctor(pValue));
				++iColumn;
			}
		}

		// if this value should be selected, select it
		if (this->mcValues.Index(pValue->sLabel) != wxNOT_FOUND)
		{
			this->mwValues->SetItemState(iValue, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		}

		// if this is the currently accepted value, make it bold
		if (pValue->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_RESULT))
		{
			this->mwValues->SetItemFont(iValue, this->mwValues->GetFont().Bold());
		}

		// move on to the next value
		++iValue;
	}

	// sort the values
	this->mwValues->SortItems(_ListCtrlSort_Values, NULL);

	// make sure the first selected item is visible
	long iSelected = this->mwValues->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (iSelected != wxNOT_FOUND)
	{
		this->mwValues->EnsureVisible(iSelected);
	}

	// check which common diff pairs exist in this choice
	// and enable/disable their menu items appropriately
	for (unsigned int uIndex = 0u; uIndex < vvARRAY_COUNT(gaCommonValuePairs); ++uIndex)
	{
		const CommonValuePair* pPair = gaCommonValuePairs + uIndex;
		this->mwDiffMenu.Enable(uIndex, 
			   !pPair->sName.IsEmpty()
			&& this->mpChoice->cValues.find(pPair->sLeft) != this->mpChoice->cValues.end()
			&& this->mpChoice->cValues.find(pPair->sRight) != this->mpChoice->cValues.end()
			);
	}

	// update the display to account for any changes in the value list
	this->UpdateDisplay();

	// done
	return true;
}

bool vvResolveDialog::RefreshData()
{
	wxArrayString sLabels;
	this->GetSelectedValueLabels(&sLabels);
	return this->RefreshData(sLabels);
}

bool vvResolveDialog::RefreshData(
	const wxString& sLabel
	)
{
	wxArrayString sLabels;
	sLabels.Add(sLabel);
	return this->RefreshData(sLabels);
}

bool vvResolveDialog::RefreshData(
	const wxArrayString& sLabels
	)
{
	// clear out the currently selected values
	this->mcValues.Clear();

	// get new resolve data
	if (!this->GetResolveData())
	{
		return false;
	}

	// transfer the new data to our controls
	this->mcValues = sLabels;
	return this->TransferDataToWindow();
}

vvVerbs::ResolveValue* vvResolveDialog::GetSelectedValue(
	)
{
	// check if exactly one item is selected
	int iSelected = this->mwValues->GetSelectedItemCount();
	if (iSelected != 1)
	{
		return NULL;
	}

	// get the one selected item
	int iIndex = mwValues->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	return (vvVerbs::ResolveValue*)this->mwValues->GetItemData(iIndex);
}

size_t vvResolveDialog::GetSelectedValues(
	wxArrayPtrVoid* pValues
	)
{
	if (pValues != NULL)
	{
		for (
			int iIndex = this->mwValues->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			iIndex >= 0;
			iIndex = this->mwValues->GetNextItem(iIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)
			)
		{
			pValues->Add((void*)this->mwValues->GetItemData(iIndex));
		}
	}

	return static_cast<size_t>(this->mwValues->GetSelectedItemCount());
}

size_t vvResolveDialog::GetSelectedValueLabels(
	wxArrayString* pLabels
	)
{
	if (pLabels != NULL)
	{
		for (
			int iIndex = this->mwValues->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			iIndex >= 0;
			iIndex = this->mwValues->GetNextItem(iIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)
			)
		{
			pLabels->Add(((vvVerbs::ResolveValue*)this->mwValues->GetItemData(iIndex))->sLabel);
		}
	}

	return static_cast<size_t>(this->mwValues->GetSelectedItemCount());
}

void vvResolveDialog::UpdateDisplay()
{
	// get the currently selected data
	wxArrayPtrVoid          cSelected;
	size_t                  uSelected = this->GetSelectedValues(&cSelected);
	vvVerbs::ResolveValue*  pValue    = NULL;
	vvVerbs::ResolveValue*  pValue2   = NULL;
	if (uSelected > 0u)
	{
		pValue = static_cast<vvVerbs::ResolveValue*>(cSelected[0]);
	}
	if (uSelected > 1u)
	{
		pValue2 = static_cast<vvVerbs::ResolveValue*>(cSelected[1]);
	}

	// decide what should be enabled
	bool bDiff   = this->mpThread == NULL && this->mpChoice->bMergeable;
	bool bMerge  = bDiff;
	bool bAccept = this->mpThread == NULL && uSelected == 1u && !pValue->cFlags.HasAnyFlag(vvVerbs::RESOLVE_VALUE_RESULT);

	// figure out the label and handler for the diff button
	wxString sDiff = gsDiffFormat_Menu;
	void (vvResolveDialog::*fDiff)(wxCommandEvent&) = &vvResolveDialog::OnDiff_Menu;
	if (uSelected == 2u)
	{
		sDiff = wxString::Format(gsDiffFormat_Selected, _FormatLabel(pValue->sLabel), _FormatLabel(pValue2->sLabel));
		fDiff = &vvResolveDialog::OnDiff_Selected;
	}

	// figure out the label for the accept button
	wxString sAccept = gsAcceptFormat_None;
	if (uSelected == 1u)
	{
		sAccept = wxString::Format(gsAcceptFormat_Selected, _FormatLabel(pValue->sLabel));
	}

	// set/enable everything appropriately
	this->mwDiff->Enable(bDiff);
	this->mwDiff->SetLabel(sDiff);
	this->mwDiff->SetSize(this->mwDiff->GetBestSize());
	this->mwDiff->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &vvResolveDialog::OnDiff_Menu, this);
	this->mwDiff->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &vvResolveDialog::OnDiff_Selected, this);
	this->mwDiff->Bind(wxEVT_COMMAND_BUTTON_CLICKED, fDiff, this);
	this->mwMerge->Enable(bMerge);
	this->mwAccept->Enable(bAccept);
	this->mwAccept->SetLabel(sAccept);
	this->mwAccept->SetSize(this->mwAccept->GetBestSize());

	// update our layout to take size/position changes into account
	this->Layout();
}

void vvResolveDialog::DiffValues(
	const wxString& sLeft,
	const wxString& sRight
	)
{
	// start a diff thread to run the diff so we don't block on it
	// it will notify us of completion via _EVT_TOOL_COMPLETED
	this->RunThread(new _DiffThread(*this->GetVerbs(), this->msFolder, this->mcItem.sGid, this->mpChoice->eState, sLeft, sRight, this));
}

bool vvResolveDialog::GetResolveData()
{
	// get our item's resolve data
	if (!this->GetVerbs()->Resolve_GetData(*this->GetContext(), this->msFolder, this->msGid, this->mcItem))
	{
		this->GetContext()->Error_ResetReport("Unable to retrieve resolve data for item: %s", this->msGid);
		return false;
	}

	// find the conflict that we care about on the item
	if (this->meState != vvVerbs::RESOLVE_STATE_COUNT)
	{
		this->mpChoice = &this->mcItem.cChoices[this->meState];
	}
	else if (this->mcItem.cChoices.size() == 1u)
	{
		this->mpChoice = &this->mcItem.cChoices.begin()->second;
	}
	else
	{
		this->GetContext()->Error_ResetReport("No conflict type specified on item with multiple conflicts: %s", this->msGid);
		return false;
	}

	// success
	return true;
}

bool vvResolveDialog::RunThread(
	wxThread* pThread
	)
{
	wxASSERT(this->mpThread == NULL);
	wxASSERT(pThread != NULL);

	// store the thread as the current one
	this->mpThread = pThread;

	// create the thread's handle
	if (this->mpThread->Create() != wxTHREAD_NO_ERROR)
	{
		// report and clean up the thread, we own it now
		this->GetContext()->Log_ReportError("Unable to start merge thread.");
		delete this->mpThread;
		this->mpThread = NULL;
		return false;
	}
	else
	{
		// update the display and run the thread
		this->UpdateDisplay();
		this->mpThread->Run();
		return true;
	}
}

void vvResolveDialog::OnUpdate(
	wxListEvent& WXUNUSED(e)
	)
{
	this->UpdateDisplay();
}

void vvResolveDialog::OnRelated(
	wxHyperlinkEvent& e
	)
{
	wxString sRepoPath = e.GetURL();

	// convert the repo path to an absolute one
	wxString sAbsolutePath;
	this->GetVerbs()->GetAbsolutePath(*this->GetContext(), this->msFolder, sRepoPath, sAbsolutePath);

	// pull up a resolve dialog on it
	vvResolveDialog cDialog;
	cDialog.Create(this, sAbsolutePath, vvVerbs::RESOLVE_STATE_COUNT, *this->GetVerbs(), *this->GetContext());
	cDialog.ShowModal();
}

void vvResolveDialog::OnDiff_Selected(
	wxCommandEvent& WXUNUSED(e)
	)
{
	wxArrayString cLabels;
	size_t        uLabels = this->GetSelectedValueLabels(&cLabels);
	wxASSERT(uLabels == 2u);
	this->DiffValues(cLabels[0], cLabels[1]);
}

void vvResolveDialog::OnDiff_Menu(
	wxCommandEvent& WXUNUSED(e)
	)
{
	this->PopupMenu(&this->mwDiffMenu, this->mwDiff->GetRect().GetBottomLeft());
}

void vvResolveDialog::OnDiff_Pair(
	wxCommandEvent& e
	)
{
	this->DiffValues(gaCommonValuePairs[e.GetId()].sLeft, gaCommonValuePairs[e.GetId()].sRight);
}

void vvResolveDialog::OnDiff_Complete(
	wxThreadEvent& WXUNUSED(e)
	)
{
	wxASSERT(this->mpThread != NULL);

	// free up the thread, it's done
	this->mpThread->Wait();
	delete this->mpThread;
	this->mpThread = NULL;

	// update the display
	this->UpdateDisplay();
}

void vvResolveDialog::OnMerge(
	wxCommandEvent& WXUNUSED(e)
	)
{
	// start a merge thread to run the merge so we don't block on it
	// it will notify us of completion via _EVT_TOOL_COMPLETED
	this->RunThread(new _MergeThread(*this->GetVerbs(), this->msFolder, this->mcItem.sGid, this->mpChoice->eState, this));
}

void vvResolveDialog::OnMerge_Complete(
	wxThreadEvent& e
	)
{
	wxASSERT(this->mpThread != NULL);

	// free up the thread, it's done
	this->mpThread->Wait();
	delete this->mpThread;
	this->mpThread = NULL;

	// check if the merge succeeded
	if (e.GetInt() != 0)
	{
		// it succeeded
		// refresh our data
		wxBusyCursor cCursor;
		this->RefreshData(e.GetString());
	}
	else
	{
		// it failed or was canceled
		// just update the display
		this->UpdateDisplay();
	}
}

void vvResolveDialog::OnAccept(
	wxCommandEvent& e
	)
{
	wxBusyCursor cCursor;

	vvVerbs::ResolveValue* pValue = this->GetSelectedValue();
	wxASSERT(pValue != NULL);
	if (!this->GetVerbs()->Resolve_Accept(*this->GetContext(), this->msFolder, this->mcItem.sGid, this->mpChoice->eState, pValue->sLabel))
	{
		this->GetContext()->Error_ResetReport("Failed to accept value: %s", _FormatLabel(pValue->sLabel));
		return;
	}

	// if we succeeded, go ahead and allow the dialog to close
	e.Skip();
}

void vvResolveDialog::OnCancel(
	wxCommandEvent& e
	)
{
	if (this->mpThread != NULL)
	{
		wxMessageBox("A diff or merge tool invoked by this dialog is still running.  You must close that tool before this dialog can be dismissed.", "Tool Still Running", wxOK, this);
		// eat the event so that the default handler doesn't close the window
	}
	else
	{
		// allow the event to continue so the default handler closes the window
		e.Skip();
	}
}

IMPLEMENT_DYNAMIC_CLASS(vvResolveDialog, vvDialog);

BEGIN_EVENT_TABLE(vvResolveDialog, vvDialog)
	EVT_HYPERLINK(wxID_ANY, OnRelated)
	EVT_LIST_ITEM_SELECTED(XRCID("wValues"), OnUpdate)
	EVT_LIST_ITEM_DESELECTED(XRCID("wValues"), OnUpdate)
//	EVT_BUTTON(XRCID("wDiff"), OnDiff_Selected) // We'll dynamically bind to one of these
//	EVT_BUTTON(XRCID("wDiff"), OnDiff_Menu)     // two handlers depending on value selection.
	EVT_MENU(wxID_ANY, OnDiff_Pair)
	EVT_TOOL_COMPLETED(_TOOL_DIFF, OnDiff_Complete)
	EVT_BUTTON(XRCID("wMerge"), OnMerge)
	EVT_TOOL_COMPLETED(_TOOL_MERGE, OnMerge_Complete)
	EVT_BUTTON(XRCID("wxID_OK"), OnAccept)
	EVT_BUTTON(XRCID("wxID_CANCEL"), OnCancel)
END_EVENT_TABLE()
