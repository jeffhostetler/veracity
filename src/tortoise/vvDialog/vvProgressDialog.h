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

#ifndef VV_PROGRESS_DIALOG_H
#define VV_PROGRESS_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvNullable.h"
#include "vvSgLogHandler.h"
#include "vvCommand.h"
#include "vvProgressExecutor.h"

/**
 * An sglib log handler that displays progress in a dialog box.
 * This dialog is explicitly thread-safe, so it can display progress occurring in another thread.
 */
class vvProgressDialog : public wxDialog, public vvSgGeneralLogHandler
{
// types
public:
	// These would be private, but WX needs it to be accessible from global scope.
	class OperationEvent; //< A type of event used to marshal operation calls across threads.
	class MessageEvent;   //< A type of event used to marshal message reports across threads.
	class ErrorCaughtEvent;   //< A type of event used to request a login dialog.

// constants
public:
	static const char* DefaultTitle; //< Default title for progress dialogs.
	static const long  DefaultStyle; //< Default style for progress dialogs.

// construction
public:
	/**
	 * Default constructor.
	 */
	vvProgressDialog();

	/**
	 * Create constructor.
	 */
	vvProgressDialog(
		wxWindow*       wParent,                       //< [in] The new dialog's parent window, or NULL if it won't have one.
		vvProgressExecutor* pExecutor,				   //< [in] The executor that launched the progress dialog.
		wxWindowID      cId       = wxID_ANY,          //< [in] ID to use for the dialog.
		const wxString& sTitle    = DefaultTitle,      //< [in] Title to use for the dialog.
		const wxPoint&  cPosition = wxDefaultPosition, //< [in] Initial position for the dialog.
		const wxSize&   cSize     = wxDefaultSize,     //< [in] Initial size for the dialog.
		long            iStyle    = DefaultStyle,      //< [in] Style flags to use for the dialog.
		const wxString& sName     = wxDialogNameStr    //< [in] Window class name for the dialog.
		);

	/**
	 * Destructor.
	 */
	~vvProgressDialog();

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*       wParent,                       //< [in] The new dialog's parent window, or NULL if it won't have one.
		wxWindowID      cId       = wxID_ANY,          //< [in] ID to use for the dialog.
		const wxString& sTitle    = DefaultTitle,      //< [in] Title to use for the dialog.
		const wxPoint&  cPosition = wxDefaultPosition, //< [in] Initial position for the dialog.
		const wxSize&   cSize     = wxDefaultSize,     //< [in] Initial size for the dialog.
		long            iStyle    = DefaultStyle,      //< [in] Style flags to use for the dialog.
		const wxString& sName     = wxDialogNameStr    //< [in] Window class name for the dialog.
		);

	/**
	 * Updates the dialog's display based on its data.
	 */
	void UpdateDisplay();

	/**
	 * Checks if the user has pushed the Cancel button.
	 */
	bool CancelRequested();

	void ExecutionStarted();
	void ExecutionStopped();

// event handlers
protected:
	void OnUpdateTimer(wxTimerEvent& cEvent);
	void OnOperationPush(OperationEvent& cEvent);
	void OnOperationPop(OperationEvent& cEvent);
	void OnOperationReplace(OperationEvent& cEvent);
	void OnMessage(MessageEvent& cEvent);
	void OnClose(wxCommandEvent& cEvent);
	void OnCancel(wxCommandEvent& cEvent);
	void OnErrorCaught(ErrorCaughtEvent& cEvent);
	void EnableTheCloseButton();
	void OnShown(wxShowEvent& cEvent);
	void OnUpdateEvent(wxListEvent& cEvent);

// private types
private:
	// forward declarations
	class OperationControl;
	class MessageListCtrl;

	/**
	 * A stack of progress operation data.
	 */
	typedef std::stack<struct _OperationData*> stlOperationStack;

// private constants
private:
	static const char* CloseButtonText;     //< Text used for the dialog's OK/close button.
	static const char* CancelButtonText;    //< Text used for the dialog's cancel button.
	static const char* CancelingButtonText; //< Text used for the cancel button after it's clicked/disabled.
	static const int   UpdateTime;          //< Milliseconds in between display updates.
	static const char* CompletedTitle;		//< The title used when the operation is completed without error.
	static const char* ErrorTitle;			//< The title used when the operation is completed with an error.

// private helpers
private:
	void RemoveOperation();
	void vvProgressDialog::SetTheTitle(wxString sTheTitle);
// private data
private:
	// data
	vvProgressExecutor* mpExecutor;
	stlOperationStack mcOperations;  //< Stack of currently running operations.
	wxTimer           mcUpdateTimer; //< Timer that pings us to update our controls.
	vvNullable<bool>  mnbFinished;   //< Whether or not our last operation has completed.
	                                 //< If NULL, no operations have been started yet.
	vvNullable<bool>  mnbCanceling;  //< Whether or not the user is attempting to cancel the running operations.
	                                 //< If NULL, the current top-level operation is not cancelable.

	int mnExecutionCount;

	// sizers
	wxBoxSizer*       mpOperationSizer;
	wxStaticBoxSizer* mpMessageSizer;

	// widgets
	wxTextCtrl*      mwMessageText;
	MessageListCtrl* mwMessageList;
	wxButton*        mwCloseButton;
	wxButton*        mwCancelButton;
	bool bEnableCloseButton;
	wxString		 msTitle;
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvProgressDialog);
	DECLARE_EVENT_TABLE();
};


/**
 * An event for requesting login credentials.
 */
class vvProgressDialog::ErrorCaughtEvent : public wxEvent
{
// construction
public:
	/**
	 * Constructor.
	 */
	ErrorCaughtEvent(
		int             iWinId,  //< [in] ID of the window to receive the event.
		vvProgressExecutor* pExecutor,
		vvCommand* pCurrentCommand,
		wxUint64 sgErrorCode
		);

public:
	vvCommand* GetCurrentCommand() const
	{
		return mpCurrentCommand;
	}
	vvProgressExecutor* GetExecutor() const
	{
		return mpExecutor;
	}
	wxUint64 GetErrorCode() const
	{
		return m_sgErrorCode;
	}
// implementation
public:
	virtual wxEvent* Clone() const;

private:
	class vvProgressExecutor* mpExecutor;
	class vvCommand* mpCurrentCommand;
	wxUint64 m_sgErrorCode;
};



#endif
