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
#include "vvProgressDialog.h"

#include "sg.h"
#include "vvContext.h"
#include "vvSgHelpers.h"
#include "vvWxHelpers.h"
#include "vvLoginDialog.h"
#include "vvCommand.h"
#include "vvProgressExecutor.h"

/*
**
** Types
**
*/

// forward declarations of internal functions
static SG_log__handler__operation _Handler_Operation;
static SG_log__handler__message   _Handler_Message;

/**
 * Data about an operation being displayed in the dialog.
 */
struct _OperationData
{
	wxDateTime   cStart;       //< Time that the operation was created/started.
	unsigned int uFlags;       //< Flags associated with the operation.
	wxString     sFile;        //< The filename of the function call that created this operation.
	unsigned int uLine;        //< The line in sFile of the function call that created this operation.
	wxString     sDescription; //< Description of the operation (might be empty).
	wxString     sStep;        //< Description of the current step (might be empty).
	unsigned int uTotal;       //< Total number of steps in the operation, or zero if unknown.
	unsigned int uFinished;    //< Number of steps that have been reported finished so far.
	SG_error     eResult;      //< Result of the operation.
};

/**
 * Data about a log message.
 */
struct _MessageData
{
	wxDateTime           cReceived; //< Date/time that the message was received.
	SG_log__message_type eType;     //< The type of the message.
	wxString             sMessage;  //< The message text string.
};

/**
 * An event for marshaling operation calls to the main thread from a background thread.
 */
class vvProgressDialog::OperationEvent : public wxEvent
{
// construction
public:
	/**
	 * Constructor.
	 */
	OperationEvent(
		int             iWinId,  //< [in] ID of the window to receive the event.
		_OperationData* pData    //< [in] Data about the operation that occurred.
		);

// functionality
public:
	/**
	 * Retrieves the operation data from the event.
	 * Caller assumes ownership of the data, the event's pointer is NULLed by this call.
	 */
	_OperationData* UnwrapData();

// implementation
public:
	virtual wxEvent* Clone() const;

// private data
private:
	_OperationData* mpData; //< Data associated with the event.
};

/**
 * Set of IDs used with OperationEvent events.
 */
enum _OperationEventId
{
	ID_OPERATION_PUSH,    //< Recipient should push the operation onto its stack.
	ID_OPERATION_POP,     //< Recipient should pop the operation off of its stack.
	ID_OPERATION_REPLACE, //< Recipient should replace its current top operation with the new one.
};

/**
 * An event for marshaling message reports to the main thread from a background thread.
 */
class vvProgressDialog::MessageEvent : public wxEvent
{
// construction
public:
	/**
	 * Constructor.
	 */
	MessageEvent(
		int           iWinId, //< [in] ID of the window to receive the event.
		_MessageData* pData   //< [in] Data about the operation that occurred.
		);

// functionality
public:
	/**
	 * Retrieves the message data from the event.
	 * Caller assumes ownership of the data, the event's pointer is NULLed by this call.
	 */
	_MessageData* UnwrapData();

// implementation
public:
	virtual wxEvent* Clone() const;

// private data
private:
	_MessageData* mpData; //< Data associated with the event.
};

/**
 * A control that displays data about a single operation in progress.
 */
class vvProgressDialog::OperationControl : public wxPanel
{
// constants
public:
	static const char* FinishedStep; //< Text displayed as the step description while "finished".
	static const char* TimeFormat;   //< Format for displaying elapsed time.

// construction
public:
	/**
	 * Constructor.
	 */
	OperationControl();

// functionality
public:
	/**
	 * Creates the control.
	 */
	virtual bool Create(
		wxWindow*       pParent,   //< [in] The new control's parent window.
		_OperationData* pOperation //< [in] The operation to display data about.
		);

	/**
	 * Updates the control to reflect changes in the underlying operation.
	 */
	void UpdateDisplay();

	/**
	 * Changes the control to display a "finished" state.
	 */
	void ShowFinished();

// private data
private:
	// data
	_OperationData* mpOperation; //< The operation data we're displaying.
	bool            mbFinished;  //< Whether or not the control is showing a finished state.

	// widgets
	wxStaticText* mwStep;
	wxStaticText* mwElapsed;
	wxGauge*      mwGauge;
};

/**
 * Our implementation of wxListCtrl, which allows us to use it in virtual mode.
 * This allows us to store message data in our own data structure and reference it as needed.
 */
class vvProgressDialog::MessageListCtrl : public wxListCtrl
{
// types
public:
	/**
	 * The set of columns displayed in the list.
	 */
	enum Columns
	{
		COLUMN_MESSAGE, //< Column that displays the message text.
		COLUMN_TIME,    //< Column that displays the message's arrival time.
		COLUMN_COUNT    //< Number of columns.
	};

	/**
	 * Configuration data for a single column.
	 */
	struct ColumnConfig
	{
		const char* szName;  //< Column name/header.
		int         iWidth;  //< Default width.
		int         iFormat; //< Format/style flags.
	};

// constants
public:
	static const ColumnConfig ColumnData[COLUMN_COUNT]; //< Config data for columns, indexed by a COLUMN_* enum value.
	static const char*        TimeFormat;               //< Format for displaying elapsed time.

// construction
public:
	/**
	 * Constructor.
	 */
	MessageListCtrl(
		wxWindow*  wParent,                       //< [in] Control's parent window.
		wxWindowID cId,                           //< [in] Control's ID.
		wxPoint    cPosition = wxDefaultPosition, //< [in] Control's initial position.
		wxSize     cSize     = wxDefaultSize      //< [in] Control's initial size.
		);

	/**
	 * Destructor.
	 */
	~MessageListCtrl();

// functionality
public:
	/**
	 * Adds a new message to the list.
	 * Returns the index of the new message.
	 */
	long AddMessage(
		const _MessageData* PData //< [in] Data about the received message.
		);

	/**
	 * Gets the full message from the given index.
	 */
	const _MessageData* GetMessage(
		long iIndex //< [in] Index of the message to retrieve.
		);

// implementation
public:
	/**
	 * Retrieves column-specific text for a virtual item.
	 */
	virtual wxString OnGetItemText(
		long iItem,  //< [in] Index of the item to get text for.
		long iColumn //< [in] Index of the column to get text for.
		) const;

	/**
	 * Retrieves a column-specific image for a virtual item.
	 */
	virtual int OnGetItemColumnImage(
		long iItem,  //< [in] Index of the item to get an image for.
		long iColumn //< [in] Index of the column to get an image for.
		) const;

	// Note: OnGetItemImage, OnGetItemAttr, and OnGetItemColumnAttr can also be overridden.

// internal functionality
protected:
	wxString SummarizeMessage(
		const wxString& sMessage
		) const;

// event handlers
protected:
	void OnItemActivated(wxListEvent& e);

// private types
private:
	/**
	 * A vector of message data objects.
	 * Needs to be a vector to provide quick lookups.
	 */
	typedef std::vector<const _MessageData*> stlMessageVector;

// private data
private:
	stlMessageVector mcMessages;                             //< List of messages being displayed.
	wxImageList      mcImages;                               //< Set of images used in the list.
	int              maImageIndices[SG_LOG__MESSAGE__COUNT]; //< Indices into mcImages, for different message types.
};


/*
**
** Globals
**
*/

wxDEFINE_EVENT(vvEVT_OPERATION, vvProgressDialog::OperationEvent);
wxDEFINE_EVENT(vvEVT_MESSAGE,   vvProgressDialog::MessageEvent);
wxDEFINE_EVENT(vvEVT_ERRORCAUGHT,   vvProgressDialog::ErrorCaughtEvent);
#define EVT_OPERATION(id, func) wx__DECLARE_EVT1(vvEVT_OPERATION, id, (&func))
#define EVT_MESSAGE(id, func)   wx__DECLARE_EVT1(vvEVT_MESSAGE,   id, (&func))
#define EVT_ERRORCAUGHT(id, func)   wx__DECLARE_EVT1(vvEVT_ERRORCAUGHT,   id, (&func))

// some control IDs
enum
{
	ID_MESSAGE_LIST
};

const char* vvProgressDialog::OperationControl::FinishedStep = "Done";
const char* vvProgressDialog::OperationControl::TimeFormat = "%H:%M:%S";

const vvProgressDialog::MessageListCtrl::ColumnConfig vvProgressDialog::MessageListCtrl::ColumnData[] =
{
	{ "Message", 325, 0 },
	{ "Time",     60, 0 },
};
const char* vvProgressDialog::MessageListCtrl::TimeFormat = "%H:%M:%S";

const char* vvProgressDialog::DefaultTitle        = "Working";
const char* vvProgressDialog::CompletedTitle      = "Completed";
const char* vvProgressDialog::ErrorTitle		  = "Error";
const long  vvProgressDialog::DefaultStyle        = wxDEFAULT_DIALOG_STYLE;
const char* vvProgressDialog::CloseButtonText     = "Close";
const char* vvProgressDialog::CancelButtonText    = "Cancel";
const char* vvProgressDialog::CancelingButtonText = "Canceling...";
const int   vvProgressDialog::UpdateTime          = 1000;

static SG_log__handler gcProgressDialogHandler =
{
	NULL,
	_Handler_Operation,
	_Handler_Message,
	NULL,
};

/**
 * Strings for different message types.
 * Declaration order must match SG_log__message_type.
 */
static const char* gaMessageTypeStrings[] =
{
	"Verbose", //< SG_LOG__MESSAGE__VERBOSE
	"Info",    //< SG_LOG__MESSAGE__INFO
	"Warning", //< SG_LOG__MESSAGE__WARNING
	"Error",   //< SG_LOG__MESSAGE__ERROR
};


/*
**
** Internal Functions
**
*/

/**
 * Shortens a string by trimming off anything past a given substring
 * (generally something like a line break) and replacing it with a
 * "..." style notation. If the substring doesn't appear, the entire
 * string is returned.
 */
static wxString _ShortenString(
	const wxString& sString,    //< [in] String to shorten.
	const wxString& sTerminator //< [in] Terminator substring to remove everything after.
	)
{
	int iIndex = sString.Find(sTerminator);
	if (iIndex != wxNOT_FOUND)
	{
		return sString.SubString(0u, iIndex) + " [...]";
	}
	else
	{
		return sString;
	}
}

/**
 * Shortens a string by trimming it to a maximum length.  If the string
 * is longer than that length it is trimmed and a "..." style notation is
 * added to the end, such that its length is the given maximum.  If the
 * string is already shorter than the length, the entire string is returned.
 */
static wxString _ShortenString(
	const wxString& sString, //< [in] String to shorten.
	unsigned int    uLength  //< [in] Maximum length to shorten the string to.
	)
{
	if (sString.Length() > uLength)
	{
		return sString.SubString(0u, uLength - 6u) + " [...]";
	}
	else
	{
		return sString;
	}
}

// ----- vvProgressDialog::OperationEvent -----

vvProgressDialog::OperationEvent::OperationEvent(
	int             iWinId,
	_OperationData* pData
	)
	: wxEvent(iWinId, vvEVT_OPERATION)
	, mpData(pData)
{
	wxASSERT(pData != NULL);
}

_OperationData* vvProgressDialog::OperationEvent::UnwrapData()
{
	_OperationData* pData = this->mpData;
	this->mpData = NULL;
	return pData;
}

wxEvent* vvProgressDialog::OperationEvent::Clone() const
{
	return new vvProgressDialog::OperationEvent(*this);
}

// ----- vvProgressDialog::MessageEvent -----

vvProgressDialog::MessageEvent::MessageEvent(
	int           iWinId,
	_MessageData* pData
	)
	: wxEvent(iWinId, vvEVT_MESSAGE)
	, mpData(pData)
{
	wxASSERT(pData != NULL);
}

_MessageData* vvProgressDialog::MessageEvent::UnwrapData()
{
	_MessageData* pData = this->mpData;
	this->mpData = NULL;
	return pData;
}

wxEvent* vvProgressDialog::MessageEvent::Clone() const
{
	return new vvProgressDialog::MessageEvent(*this);
}


// ----- vvProgressDialog::LoginRequestedEvent -----

vvProgressDialog::ErrorCaughtEvent::ErrorCaughtEvent(
	int             iWinId,
	class vvProgressExecutor* pExecutor,
	class vvCommand* pCurrentCommand,
	wxUint64 sgErrorCode
	)
	: wxEvent(iWinId, vvEVT_ERRORCAUGHT),
	mpExecutor(pExecutor),
	mpCurrentCommand(pCurrentCommand),
	m_sgErrorCode(sgErrorCode)
{
	wxASSERT(pExecutor != NULL);
	wxASSERT(pCurrentCommand != NULL);
}

wxEvent* vvProgressDialog::ErrorCaughtEvent::Clone() const
{
	return new vvProgressDialog::ErrorCaughtEvent(*this);
}

// ----- vvProgressDialog::OperationControl -----

vvProgressDialog::OperationControl::OperationControl()
	: mbFinished(false)
{
	this->SetDoubleBuffered(true);
}

bool vvProgressDialog::OperationControl::Create(
	wxWindow*       pParent,
	_OperationData* pOperation
	)
{
	if (!wxPanel::Create(pParent))
	{
		return false;
	}

	wxStaticBoxSizer* pOuterSizer = new wxStaticBoxSizer(wxVERTICAL, this);
	wxBoxSizer*       pInnerSizer = new wxBoxSizer(wxHORIZONTAL);

	this->mwStep    = new wxStaticText(pOuterSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END);
	this->mwElapsed = new wxStaticText(pOuterSizer->GetStaticBox(), wxID_ANY, wxEmptyString);
	this->mwGauge   = new wxGauge     (pOuterSizer->GetStaticBox(), wxID_ANY,             1, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL | wxGA_SMOOTH);

	pInnerSizer->Add(this->mwStep,    wxSizerFlags().Center().Proportion(1).Border(wxRIGHT, 8));
	pInnerSizer->Add(this->mwElapsed, wxSizerFlags().Center());
	pOuterSizer->Add(pInnerSizer,     wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 8));
	pOuterSizer->Add(this->mwGauge,   wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 8));

	this->SetSizer(pOuterSizer);
	this->mpOperation = pOperation;
	this->UpdateDisplay();
	this->Layout();
	this->Fit();

	return true;
}

void vvProgressDialog::OperationControl::UpdateDisplay()
{
	// prevent drawing while we make updates
	wxWindowUpdateLocker cDrawLock(this);

	if (this->mbFinished)
	{
		this->mwStep->SetLabelText(FinishedStep);
		this->mwGauge->SetValue(this->mwGauge->GetRange());
	}
	else if (this->mpOperation == NULL)
	{
		wxDynamicCast(this->GetSizer(), wxStaticBoxSizer)->GetStaticBox()->SetLabelText(wxEmptyString);
		this->mwStep->SetLabelText(wxEmptyString);
		this->mwElapsed->SetLabelText(wxEmptyString);
		this->mwGauge->Disable();
	}
	else
	{
		wxDynamicCast(this->GetSizer(), wxStaticBoxSizer)->GetStaticBox()->SetLabelText(_ShortenString(this->mpOperation->sDescription, 60u));
		this->mwStep->SetLabelText(this->mpOperation->sStep);
		this->mwElapsed->SetLabelText((wxDateTime::UNow() - this->mpOperation->cStart).Format(TimeFormat));

		if (this->mpOperation->uTotal == 0u)
		{
			this->mwGauge->Pulse();
		}
		else
		{
			this->mwGauge->SetRange(wxMax(this->mpOperation->uTotal, this->mpOperation->uFinished));
			this->mwGauge->SetValue(this->mpOperation->uFinished);
		}
	}
}

void vvProgressDialog::OperationControl::ShowFinished()
{
	wxASSERT(this->mbFinished == false);
	wxASSERT(this->mpOperation != NULL);

	if (this->mbFinished || this->mpOperation == NULL)
	{
		return;
	}

	this->mbFinished = true;
	this->mwElapsed->SetLabelText((wxDateTime::UNow() - this->mpOperation->cStart).Format(TimeFormat));
	this->mpOperation = NULL;
	this->UpdateDisplay();
}

// ----- vvProgressDialog::MessageListCtrl -----

vvProgressDialog::MessageListCtrl::MessageListCtrl(
	wxWindow*  wParent,
	wxWindowID cId,
	wxPoint    cPosition,
	wxSize     cSize
	)
	: mcImages(16, 16, true, 4)
{
	// needs to be in at least vvProgressDialog scope, because MessageListCtrl is private
	// otherwise this would be at global scope near the definition of ColumnData
	wxCOMPILE_TIME_ASSERT(WXSIZEOF(ColumnData) == COLUMN_COUNT, EnumStringMismatch);

	if (!this->Create(wParent, cId, cPosition, cSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_VIRTUAL))
	{
		return;
	}

	for (unsigned int uIndex = 0u; uIndex < COLUMN_COUNT; ++uIndex)
	{
		this->InsertColumn(uIndex, ColumnData[uIndex].szName, ColumnData[uIndex].iFormat, ColumnData[uIndex].iWidth);
	}
	wxArrayInt cOrder(COLUMN_COUNT);
	cOrder[0] = COLUMN_TIME;
	cOrder[1] = COLUMN_MESSAGE;
	this->SetColumnsOrder(cOrder);

	this->maImageIndices[SG_LOG__MESSAGE__VERBOSE] = this->mcImages.Add(wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MENU));
	this->maImageIndices[SG_LOG__MESSAGE__INFO]    = this->mcImages.Add(wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MENU));
	this->maImageIndices[SG_LOG__MESSAGE__WARNING] = this->mcImages.Add(wxArtProvider::GetBitmap(wxART_WARNING,     wxART_MENU));
	this->maImageIndices[SG_LOG__MESSAGE__ERROR]   = this->mcImages.Add(wxArtProvider::GetBitmap(wxART_ERROR,       wxART_MENU));
	this->SetImageList(&this->mcImages, wxIMAGE_LIST_SMALL);
}

vvProgressDialog::MessageListCtrl::~MessageListCtrl()
{
	for (stlMessageVector::iterator itMessage = this->mcMessages.begin(); itMessage != this->mcMessages.end(); ++itMessage)
	{
		delete *itMessage;
	}
}

long vvProgressDialog::MessageListCtrl::AddMessage(
	const _MessageData* pData
	)
{
	this->mcMessages.push_back(pData);
	long iSize = (long)this->mcMessages.size();
	this->SetItemCount(iSize);
	return iSize - 1;
}

const _MessageData* vvProgressDialog::MessageListCtrl::GetMessage(
	long iIndex
	)
{
	return this->mcMessages[iIndex];
}

wxString vvProgressDialog::MessageListCtrl::OnGetItemText(
	long iItem,
	long iColumn
	) const
{
	const _MessageData* pMessage = this->mcMessages[iItem];

	switch (iColumn)
	{
	case COLUMN_TIME:
		return pMessage->cReceived.Format(TimeFormat);
	case COLUMN_MESSAGE:
		return _ShortenString(pMessage->sMessage, "\n");
	default:
		wxFAIL_MSG("Unknown MessageListCtrl column.");
		return wxEmptyString;
	}
}

int vvProgressDialog::MessageListCtrl::OnGetItemColumnImage(
	long iItem,
	long iColumn
	) const
{
	if (iColumn == COLUMN_MESSAGE)
	{
		return this->maImageIndices[this->mcMessages[iItem]->eType];
	}
	else
	{
		return -1;
	}
}

/**
 * Our implementation of SG_log__handler__operation.
 */
static void _Handler_Operation(
	SG_context*              pCtx,
	void*                    pThis,
	const SG_log__operation* pOperation,
	SG_log__operation_change eChange,
	SG_bool*                 pCancel
	)
{
	static const SG_uint32 uMaxStepReports = 200u; //< send at most this many step completion reports to the dialog per operation
	static const SG_uint32 uMaxStackSize   = 3u;   //< ignore operations deeper down the stack than this, so we don't overload the dialog

	/**
	 * This map tracks the number of finished steps that we last reported
	 * to the dialog about each operation in progress.
	 */
	static std::map<const SG_log__operation*, SG_uint32> cFinishedSteps;

	vvProgressDialog* pDialog       = static_cast<vvProgressDialog*>(pThis);
	SG_uint32         uStackSize    = 0u;
	SG_int64          iStart        = 0;
	SG_uint32         uFlags        = 0u;
	const char*       szFile        = NULL;
	SG_uint32         uLine         = 0u;
	const char*       szDescription = NULL;
	const char*       szStep        = NULL;
	SG_uint32         uTotal        = 0u;
	SG_uint32         uFinished     = 0u;
	SG_error          eResult       = SG_ERR_OK;

	// we'll handle the PUSHED/POPPED changes instead of these
	// if we handle these, the dialog will get confused about operation changes happening while the stack is empty
	if (eChange == SG_LOG__OPERATION__CREATED || eChange == SG_LOG__OPERATION__COMPLETED)
	{
		return;
	}

	// get the current size of the operation stack
	SG_ERR_CHECK(  SG_log__stack__get_count(pCtx, &uStackSize)  );
	if (eChange == SG_LOG__OPERATION__PUSHED)
	{
		// start a record for this operation
		cFinishedSteps[pOperation] = 0u;
	}
	else if (eChange == SG_LOG__OPERATION__POPPED)
	{
		// during a POPPED change, the stack size has already been decremented
		// however, for our purposes we'd like to consider the operation still on the stack
		uStackSize += 1u;
	}

	// ignore operations deeper down the stack than we want to display
	// this will prevent operations from overflowing the dialog
	if (uStackSize > uMaxStackSize)
	{
		return;
	}

	// retrieve data about the current operation
	SG_ERR_CHECK(  SG_log__operation__get_time(pCtx, pOperation, &iStart, NULL, NULL)  );
	SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, &szDescription, &szStep, &uFlags)  );
	SG_ERR_CHECK(  SG_log__operation__get_source(pCtx, pOperation, &szFile, &uLine)  );
	SG_ERR_CHECK(  SG_log__operation__get_progress(pCtx, pOperation, NULL, &uFinished, &uTotal, NULL, 0u)  );
	SG_ERR_CHECK(  SG_log__operation__get_result(pCtx, pOperation, &eResult)  );

	// filter step completion messages
	// We can't send every single step completion to the dialog, because they might
	// be rapid enough to overwhelm the Windows message queue that we use to send
	// the data, which in turn causes wxWidgets to overflow the stack and crash.
	if (eChange == SG_LOG__OPERATION__STEPS_FINISHED)
	{
		// check where we are in the operation
		if (uFinished > uTotal)
		{
			// no need to send steps that are above the total expected
			// The dialog will show correctly in both cases where this occurs:
			// 1) The total step count is unknown, but steps are being reported anyway,
			//    in which case the dialog will just show an endless progress bar
			//    animation whether we report the steps to it or not.
			// 2) The number of reported steps has exceeded the total expected count,
			//    in which case the dialog will show a full/complete progress bar
			//    no matter how many steps over the total we report.
			return;
		}
		else if (uFinished < uTotal)
		{
			// check if the number of steps finished since the last time we reported
			// step completion is a significant enough portion of the total to bother
			if ( (uFinished - cFinishedSteps[pOperation]) < (uTotal / uMaxStepReports) )
			{
				return;
			}
		}
		else
		{
			// the last step in the operation has finished
			// we should always report this one
		}

		// we're going to send the step completion, record what it was
		cFinishedSteps[pOperation] = uFinished;
	}

	// translate/copy the data to send to the main thread
	_OperationData* pData = new _OperationData();
	pData->cStart       = vvSgHelpers::Convert_sgTime_wxDateTime(iStart);
	pData->uFlags       = uFlags;
	pData->sFile        = vvSgHelpers::Convert_sglibString_wxString(szFile);
	pData->uLine        = uLine;
	pData->sDescription = vvSgHelpers::Convert_sglibString_wxString(szDescription);
	pData->sStep        = vvSgHelpers::Convert_sglibString_wxString(szStep);
	pData->uTotal       = uTotal;
	pData->uFinished    = uFinished;
	pData->eResult      = eResult;

	// setup the correct event ID
	int iWinId = ID_OPERATION_REPLACE;
	if (eChange == SG_LOG__OPERATION__PUSHED)
	{
		iWinId = ID_OPERATION_PUSH;
	}
	else if (eChange == SG_LOG__OPERATION__POPPED)
	{
		// make sure we're not about to pop an empty stack
		// if this trips we must be handling mismatched push/pop calls
		wxASSERT(uStackSize > 0u);
		iWinId = ID_OPERATION_POP;

		// we don't need a record for this operation anymore
		cFinishedSteps.erase(pOperation);
	}

	// return whether or not the user has pushed the dialog's cancel button
	*pCancel = vvSgHelpers::Convert_cppBool_sgBool(pDialog->CancelRequested());

	// send the event to the dialog on the main thread
	wxQueueEvent(pDialog, new vvProgressDialog::OperationEvent(iWinId, pData));

fail:
	return;
}

/**
 * Our implementation of SG_log__handler__message.
 */
static void _Handler_Message(
	SG_context*              pCtx,
	void*                    pThis,
	const SG_log__operation* WXUNUSED(pOperation),
	SG_log__message_type     eType,
	const SG_string*         pMessage,
	SG_bool*                 pCancel
	)
{
	vvProgressDialog* pDialog = static_cast<vvProgressDialog*>(pThis);
	SG_int64          iTime   = 0;

	SG_ERR_CHECK(  SG_log__stack__get_time(pCtx, &iTime)  );

	_MessageData* pData = new _MessageData();
	pData->cReceived = vvSgHelpers::Convert_sgTime_wxDateTime(iTime);
	pData->eType     = eType;
	pData->sMessage  = vvSgHelpers::Convert_sgString_wxString(pMessage);

	*pCancel = vvSgHelpers::Convert_cppBool_sgBool(pDialog->CancelRequested());
	wxQueueEvent(pDialog, new vvProgressDialog::MessageEvent(wxID_ANY, pData));

fail:
	return;
}


/*
**
** Public Functions
**
*/

vvProgressDialog::vvProgressDialog()
	: vvSgGeneralLogHandler(gcProgressDialogHandler, static_cast<void*>(this))
	, mcUpdateTimer(this)
	, mnbFinished(vvNULL)
	, mnbCanceling(vvNULL)
	, mpOperationSizer(NULL)
	, mwMessageList(NULL)
	, mnExecutionCount(0)
	, msTitle(DefaultTitle)
{
}

vvProgressDialog::vvProgressDialog(
	wxWindow*       wParent,
	vvProgressExecutor* pExecutor,
	wxWindowID      cId,
	const wxString& sTitle,
	const wxPoint&  cPosition,
	const wxSize&   cSize,
	long            iStyle,
	const wxString& sName
	)
	: vvSgGeneralLogHandler(gcProgressDialogHandler, static_cast<void*>(this))
	, mpExecutor(pExecutor)
	, mcUpdateTimer(this)
	, mnbFinished(vvNULL)
	, mnbCanceling(vvNULL)
	, mpOperationSizer(NULL)
	, mwMessageList(NULL)
	, mnExecutionCount(0)
	, bEnableCloseButton(false)
	, msTitle(DefaultTitle)
{
	(void)this->Create(wParent, cId, sTitle, cPosition, cSize, iStyle, sName);
}

vvProgressDialog::~vvProgressDialog()
{
	wxASSERT(this->mcOperations.size() == 0u || this->mnbFinished.IsNullOrEqual(true));

	while (this->mcOperations.size() > 0u)
	{
		_OperationData* pData = this->mcOperations.top();
		delete pData;
		this->mcOperations.pop();
	}
}

bool vvProgressDialog::Create(
	wxWindow*       wParent,
	wxWindowID      cId,
	const wxString& sTitle,
	const wxPoint&  cPosition,
	const wxSize&   cSize,
	long            iStyle,
	const wxString& sName
	)
{
	// configure ourselves for normal logging level
	this->SetLevel(vvSgLogHandler::LEVEL_NORMAL);

	// configure ourselves for detailed messages in debug builds, brief messages otherwise
	this->SetReceiveDetailedMessages(vvNULL);

	// create ourselves
	if (!wxDialog::Create(wParent, cId, sTitle, cPosition, cSize, iStyle, sName))
	{
		return false;
	}
	this->SetIcon(wxICON(ApplicationIcon));

	// top-level sizer provides a minimum size and border around the whole dialog
	wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);
	if (cSize == wxDefaultSize)
	{
		pTopSizer->SetMinSize(450, 0);
	}
	else
	{
		pTopSizer->SetMinSize(cSize.x, 0);
	}

	// layout sizer lays out the operation list, message list, and bottom buttons
	// layout sizer is the only child of the top-level sizer
	wxBoxSizer* pLayoutSizer = new wxBoxSizer(wxVERTICAL);
	pTopSizer->Add(pLayoutSizer, wxSizerFlags().Proportion(1).Expand().Border(wxALL, 16));

	// the first box sizer in the layout will contain our operation controls
	this->mpOperationSizer = new wxBoxSizer(wxVERTICAL);
	pLayoutSizer->Add(this->mpOperationSizer, wxSizerFlags().Expand());

	// the next static box sizer in the layout will contain any messages that occur
	this->mpMessageSizer = new wxStaticBoxSizer(wxVERTICAL, this);
	pLayoutSizer->Add(this->mpMessageSizer, wxSizerFlags().Expand());

	// the first item in the message sizer is the list for multiple messages
	this->mwMessageList = new MessageListCtrl(this, ID_MESSAGE_LIST, wxDefaultPosition, wxSize(0, 130));
	this->mpMessageSizer->Add(this->mwMessageList, wxSizerFlags().Expand().Border(wxBOTTOM, 8));

	// the next item in the message sizer is the text box for single messages
	this->mwMessageText = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(0, 90), wxTE_MULTILINE | wxTE_READONLY);
	this->mpMessageSizer->Add(this->mwMessageText, wxSizerFlags().Expand());

	// the last item in the layout is a sizer for standard dialog buttons
	wxStdDialogButtonSizer* pButtonSizer = new wxStdDialogButtonSizer();
	this->mwCloseButton = new wxButton(this, wxID_OK, CloseButtonText);
	this->mwCloseButton->Disable();
	this->mwCloseButton->SetDefault();
	pButtonSizer->SetAffirmativeButton(this->mwCloseButton);
	this->mwCancelButton = new wxButton(this, wxID_CANCEL, CancelButtonText);
	pButtonSizer->SetCancelButton(this->mwCancelButton);
	pButtonSizer->Realize();
	pLayoutSizer->Add(pButtonSizer, wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxTOP, 16));

	// set our sizer to the top level one
	this->SetSizer(pTopSizer);

	// update the display
	this->UpdateDisplay();

	// orient ourselves to our parent
	if (wParent != NULL)
	{
		this->CenterOnParent();

		// Move the window up a bit to account for the fact that we will probably
		// dynamically grow downwards.  If we don't grow, having the window be a
		// little too high still looks okay (better than the window looks when it
		// does grow and we don't adjust at all for it, anyway).
		this->Move(this->GetPosition() + wxPoint(0, -50));
	}

	// assume that things are working until they don't
	this->SetReturnCode(wxID_OK);

	// success
	return true;
}

void vvProgressDialog::ExecutionStarted()
{
	mnExecutionCount++;
	EnableTheCloseButton();
}

void vvProgressDialog::ExecutionStopped()
{
	mnExecutionCount--;
	EnableTheCloseButton();
	SetTheTitle(CompletedTitle);
}

void vvProgressDialog::SetTheTitle(wxString sTheTitle)
{
	// configure the close button
	if (this->IsShown())
		this->SetTitle(sTheTitle);
	else
		msTitle = sTheTitle;
}

void vvProgressDialog::EnableTheCloseButton()
{
	// configure the close button
	if (this->IsShown())
		this->mwCloseButton->Enable(mnExecutionCount == 0 && this->mnbFinished.IsNullOrEqual(true) && this->mnbCanceling.IsNullOrEqual(false));
	else
		bEnableCloseButton = true;
}

void vvProgressDialog::UpdateDisplay()
{
	// prevent drawing while we make layout changes
	wxWindowUpdateLocker cDrawLock(this);

	// make sure we repaint the whole window
	// Without this, if our layout changes we might end up with controls
	// still being partially visible in their old location rather than being
	// cleared by the dialog's background.
	this->Refresh();

	// run through all of our operation controls and update them
	for (size_t uIndex = 0u; uIndex < this->mpOperationSizer->GetItemCount(); ++uIndex)
	{
		OperationControl* wControl = wxDynamicCast(this->mpOperationSizer->GetItem(uIndex)->GetWindow(), OperationControl);
		wControl->Show();
		wControl->UpdateDisplay();
	}

	// check how many messages there are
	int iMessageCount = this->mwMessageList->GetItemCount();

	// display the selected message in the text box
	// If the user hasn't selected a message, then the selected one will always be
	// the one most recently added.
	long iSelected = this->mwMessageList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (iSelected >= 0)
	{
		this->mwMessageText->SetValue(this->mwMessageList->GetMessage(iSelected)->sMessage);
	}

	// make sure the label on the message group box makes sense
	if (iMessageCount == 1)
	{
		const _MessageData* pMessage = this->mwMessageList->GetMessage(0);
		this->mpMessageSizer->GetStaticBox()->SetLabel(gaMessageTypeStrings[pMessage->eType]);
	}
	else
	{
		this->mpMessageSizer->GetStaticBox()->SetLabel("Messages");
	}

	EnableTheCloseButton();

	// display the message list/box as needed
	this->mpMessageSizer->Show(iMessageCount > 0);
	this->mwMessageText->Show(iMessageCount > 0);
	this->mwMessageList->Show(iMessageCount > 1);

	// configure the cancel button
	this->mwCancelButton->SetLabelText(this->mnbCanceling.IsValidAndEqual(true) ? CancelingButtonText : CancelButtonText);
	this->mwCancelButton->Enable(this->mnbFinished.IsValidAndEqual(false) && this->mnbCanceling.IsNullOrEqual(false));
	this->mwCancelButton->Show(this->mnbCanceling.IsValid());

	// re-layout the window
	this->Layout();
	this->Fit();
}

bool vvProgressDialog::CancelRequested()
{
	return this->mnbCanceling.IsValidAndEqual(true);
}

void vvProgressDialog::OnErrorCaught(
	ErrorCaughtEvent& cEvent
	)
{
	vvLoginDialog ld;
	vvContext     cContext;
	if (cEvent.GetCurrentCommand()->TryToHandleError(&cContext, this, cEvent.GetErrorCode()))
	{
		mpExecutor->PostExecuteMessage();
	}
	else
		this->SetTheTitle(ErrorTitle);
}

void vvProgressDialog::OnUpdateTimer(
	wxTimerEvent& WXUNUSED(cEvent)
	)
{
	this->UpdateDisplay();
}

void vvProgressDialog::OnOperationPush(
	OperationEvent& cEvent
	)
{
	// prevent drawing while we make layout changes
	wxWindowUpdateLocker cDrawLock(this);

	// if we're in the "finished" state
	// meaning our last running operation completed, but is still displaying
	// then go ahead and remove it now since we're starting something new
	if (this->mnbFinished.IsValidAndEqual(true))
	{
		this->RemoveOperation();
	}

	// get the data from the event
	_OperationData* pData = cEvent.UnwrapData();

	// add the new operation to our stack
	this->mcOperations.push(pData);

	// create a control to display the operation
	// We create it hidden in order to avoid any flickering caused by it being
	// displayed briefly at 0,0 before being correctly laid out by our sizer.
	OperationControl* wOperation = new OperationControl();
	wOperation->Hide();
	if (!wOperation->Create(this, pData))
	{
		wxLogError("Failed to create operation control.");
		this->mcOperations.pop();
		delete wOperation;
		delete pData;
		return;
	}
	// insert the control at the bottom of the list
	this->mpOperationSizer->Insert(this->mpOperationSizer->GetItemCount(), wOperation, wxSizerFlags().Expand());

	// if that was our first operation, do a little more setup
	if (this->mcOperations.size() == 1u)
	{
		// start our update timer
		this->mcUpdateTimer.Start(UpdateTime);

		// enable cancellation as appropriate
		if (vvFlags(pData->uFlags).HasAnyFlag(SG_LOG__FLAG__CAN_CANCEL))
		{
			this->mnbCanceling = false;
		}
		else
		{
			this->mnbCanceling = vvNULL;
		}
	}

	// we're not finished now that we have an operation
	this->mnbFinished = false;

	// update our display
	this->UpdateDisplay();
}

void vvProgressDialog::OnOperationPop(
	OperationEvent& cEvent
	)
{
	// get the event data
	_OperationData* pData = cEvent.UnwrapData();

	// check if this was the last operation
	if (this->mcOperations.size() == 1u)
	{
		// just trip our "finished" flag
		wxASSERT(this->mnbFinished == false);
		this->mnbFinished = true;

		// stop trying to cancel, if we were
		if (this->mnbCanceling.IsValid())
		{
			this->mnbCanceling = false;
		}

		// if the operation failed, update our return code
		if (!SG_IS_OK(pData->eResult))
		{
			this->SetReturnCode(wxID_CANCEL);
		}

		// leave the last operation displaying in a finished state
		OperationControl* pTop = wxDynamicCast(this->mpOperationSizer->GetItem((size_t)0u)->GetWindow(), OperationControl);
		wxASSERT(pTop != NULL);
		pTop->ShowFinished();

		// stop our update timer
		this->mcUpdateTimer.Stop();
	}
	else
	{
		// remove the operation from the display
		this->RemoveOperation();
	}

	// update our display
	this->UpdateDisplay();

	// we own the memory from the event
	delete pData;
}

void vvProgressDialog::OnOperationReplace(
	OperationEvent& cEvent
	)
{
	wxASSERT(this->mcOperations.size() > 0u);

	// get the data from the event
	_OperationData* pNew = cEvent.UnwrapData();

	// get the data for our current operation
	_OperationData* pOld = this->mcOperations.top();

	// replace the old with the new
	*pOld = *pNew;

	// free the new data, since we own it and aren't going to store it
	delete pNew;

	// update our display
	this->UpdateDisplay();
}

void vvProgressDialog::OnMessage(
	MessageEvent& cEvent
	)
{
	// get the message data
	_MessageData* pData = cEvent.UnwrapData();

	// if this is an error, update our return code
	if (pData->eType == SG_LOG__MESSAGE__ERROR)
	{
		this->SetReturnCode(wxID_CANCEL);
		this->SetTheTitle(ErrorTitle);
	}

	// add the message to our list and select it
	long iMessage = this->mwMessageList->AddMessage(pData);
	this->mwMessageList->SetItemState(iMessage, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

	// update our display
	this->UpdateDisplay();
}

void vvProgressDialog::OnClose(
	wxCommandEvent& WXUNUSED(cEvent)
	)
{
	// make sure our data validates and transfers
	// (though we don't have any, as of this comment being written)
	if (!this->Validate() || !this->TransferDataFromWindow())
	{
		return;
	}

	// close the window using whatever result code is already set
	// should have been set when the last operation was popped
	if (this->IsModal())
	{
		this->EndModal(this->GetReturnCode());
	}
	else
	{
		this->Hide();
	}
}

void vvProgressDialog::OnShown(
	wxShowEvent& WXUNUSED(cEvent)
	)
{
	if (msTitle != DefaultTitle)
		SetTheTitle(msTitle);
	if (bEnableCloseButton)
		EnableCloseButton();
}

void vvProgressDialog::OnCancel(
	wxCommandEvent& cEvent
	)
{
	if (this->mnbCanceling.IsValid())
	{
		// if we can cancel the operation, signal it to cancel
		this->mnbCanceling = true;
		this->UpdateDisplay();
	}
	else if (mnExecutionCount == 0 && this->mnbFinished.IsNullOrEqual(true))
	{
		// if we're finished, let the default wxID_CANCEL logic close the window
		cEvent.Skip();
		// Note: This can cause strange behavior if the operation finished
		//       successfully and our return code is currently wxID_OK, because the
		//       default logic will return wxID_CANCEL even though there wasn't any
		//       problem or cancellation.  Currently this will cause the parent
		//       vvDialog to remain open, even though it should close itself.  It's
		//       not a huge problem when it occurs, and it only occurs if the user
		//       uses the X button to close the window rather than the Close button,
		//       so I'm not going to spend a bunch of time here figuring out how to
		//       make it return whatever return code we currently have set rather
		//       than wxID_CANCEL.  However, it would be preferable if it did that.
	}

	// just eat the message, overriding the logic that closes the window
	// we can't allow it to be closed until the operation finishes
}

void vvProgressDialog::RemoveOperation()
{
	// get the index of the current operation's control
	unsigned int uIndex = this->mpOperationSizer->GetItemCount() - 1u;

	// get the control for the top-most operation
	wxWindow* wWindow = this->mpOperationSizer->GetItem(uIndex)->GetWindow();

	// remove the control from the window and destroy it
	this->mpOperationSizer->Remove(uIndex);
	wWindow->Destroy();

	// pop the operation data off our stack
	_OperationData* pData = this->mcOperations.top();
	this->mcOperations.pop();
	delete pData;
}

void vvProgressDialog::OnUpdateEvent(
	wxListEvent& WXUNUSED(cEvent)
	)
{
	this->UpdateDisplay();
}

IMPLEMENT_DYNAMIC_CLASS(vvProgressDialog, wxDialog);

BEGIN_EVENT_TABLE(vvProgressDialog, wxDialog)
	EVT_TIMER(wxID_ANY, vvProgressDialog::OnUpdateTimer)
	EVT_OPERATION(ID_OPERATION_PUSH, vvProgressDialog::OnOperationPush)
	EVT_OPERATION(ID_OPERATION_POP, vvProgressDialog::OnOperationPop)
	EVT_OPERATION(ID_OPERATION_REPLACE, vvProgressDialog::OnOperationReplace)
	EVT_MESSAGE(wxID_ANY, vvProgressDialog::OnMessage)
	EVT_ERRORCAUGHT(wxID_ANY, vvProgressDialog::OnErrorCaught)
	EVT_BUTTON(wxID_OK, vvProgressDialog::OnClose)
	EVT_BUTTON(wxID_CANCEL, vvProgressDialog::OnCancel)
	EVT_SHOW(vvProgressDialog::OnShown)
	EVT_LIST_ITEM_SELECTED(ID_MESSAGE_LIST, vvProgressDialog::OnUpdateEvent)
	EVT_LIST_ITEM_DESELECTED(ID_MESSAGE_LIST, vvProgressDialog::OnUpdateEvent)
END_EVENT_TABLE()
