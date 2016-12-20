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
#include "vvProgressExecutor.h"

#include "vvCommand.h"
#include "vvContext.h"
#include "vvProgressDialog.h"
#include "vvLoginDialog.h"

#include "sg.h"

/*
**
** Globals
**
*/

/**
 * Default timeout before displaying the progress dialog.
 */
static const unsigned int guDefaultTimeout = 2000u;


/*
**
** Public Functions
**
*/

vvProgressExecutor::vvProgressExecutor(
	vvCommand& cCommand
	)
	: mpCommand(&cCommand)
	, mcMessages()
	, muTimeout(guDefaultTimeout)
	, mwProgress(NULL)
	, mpCondition(NULL)
	, mpMutex(NULL)
{
	// create our background thread and start it
	wxThreadError eThreadError = this->CreateThread(wxTHREAD_JOINABLE);
	if (eThreadError != wxTHREAD_NO_ERROR)
	{
		wxLogError("Error creating background command execution thread: %d", eThreadError);
		return;
	}
	eThreadError = this->GetThread()->Run();
	if (eThreadError != wxTHREAD_NO_ERROR)
	{
		wxLogError("Error starting background command execution thread: %d", eThreadError);
		return;
	}
}

vvProgressExecutor::~vvProgressExecutor()
{
	// clear the message queue
	this->mcMessages.Clear();

	// send an exit message to the background thread
	this->mcMessages.Post(MESSAGE_EXIT);

	// wait until the thread exits
	this->GetThread()->Wait();
}

bool vvProgressExecutor::IsOk() const
{
	wxThread* pThread = this->GetThread();
	return pThread != NULL && pThread->IsAlive() && pThread->IsRunning();
}

unsigned int vvProgressExecutor::GetTimeout() const
{
	return this->muTimeout;
}

void vvProgressExecutor::SetTimeout(
	unsigned int uTimeout
	)
{
	this->muTimeout = uTimeout;
}

void vvProgressExecutor::PostExecuteMessage()
{
	// send a message to the background thread to execute the command
	this->mcMessages.Post(MESSAGE_EXECUTE);
}

bool vvProgressExecutor::Execute(
	wxWindow* wParent
	)
{
	wxASSERT(wxThread::IsMain());
	wxASSERT(this->IsOk());

	// create a progress dialog to display the execution's progress to the user
	vvProgressDialog cDialog(wParent, this);
	cDialog.EnableStatTracking();

	// create a condition for the background thread to signal when finished
	wxMutex     cMutex;
	wxCondition cCondition(cMutex);

	// start with the mutex locked
	// The mutex will be atomically unlocked while we're waiting on the condition.
	// The background thread will attempt to lock this mutex before signalling the condition.
	// This setup ensures that the background thread can't signal the condition before we start waiting on it.
	cMutex.Lock();

	// store cross-thread data to member variables
	// these being non-NULL also signifies that execution is in progress
	this->mwProgress  = &cDialog;
	this->mpCondition = &cCondition;
	this->mpMutex     = &cMutex;

	// send a message to the background thread to execute the command
	this->mcMessages.Post(MESSAGE_EXECUTE);

	// wait for the background thread to signal that it's finished
	// or for the maximum wait time to elapse, whichever occurs first
	wxBeginBusyCursor();
	wxCondError cCondError = cCondition.WaitTimeout(this->muTimeout);
	wxEndBusyCursor();

	// check what happened
	int iResult = 0;
	if (cCondError == wxCOND_TIMEOUT)
	{
		// timeout elapsed

		// unlock the mutex
		// Since we're not waiting anymore, the mutex is once again locked.
		// This will make it impossible for the background thread to lock it
		// before signalling the condition when it finally finishes.
		// Therefore, we need to unlock it here to allow the background
		// thread to continue on and finish correctly.
		cMutex.Unlock();

		// display the progress dialog for the remainder of the execution
		// It's modal so that the user can't interact with anything while the command executes.
		// This will be closed either by the background thread or by the user clicking OK/Cancel.
		iResult = cDialog.ShowModal();
	}
	else
	{
		// background thread finished before the timeout

		// check if any problems were reported to the progress dialog
		if (
			   cDialog.GetMessageCount(vvContext::LOG_MESSAGE_ERROR) > 0u
			|| cDialog.GetMessageCount(vvContext::LOG_MESSAGE_WARNING) > 0u
			|| cDialog.GetOperationCount(false) > 0u
			)
		{
			// the dialog saw a problem, so we'd better display it anyway
			iResult = cDialog.ShowModal();
		}
		else
		{
			// don't bother showing the dialog, just get its return code and go on
			iResult = cDialog.GetReturnCode();
		}
	}

	// blank out the cross-thread member variables
	// no execution in progress anymore
	this->mwProgress  = NULL;
	this->mpCondition = NULL;
	this->mpMutex     = NULL;

	// return the result
	return (iResult == wxID_OK);
}

wxThread::ExitCode vvProgressExecutor::Entry()
{
	ThreadMessage eMessage;
	vvContext     cContext;

	// wait for a message from the main thread
	while (this->mcMessages.Receive(eMessage) == wxMSGQUEUE_NO_ERROR)
	{
		// check for status changes in the thread
		if (this->GetThread()->TestDestroy())
		{
			break;
		}

		// if this is an exit message, then exit
		if (eMessage == MESSAGE_EXIT)
		{
			break;
		}

		// otherwise, assume we have an execute message
		// since no other message types exist right now
		wxASSERT(eMessage == MESSAGE_EXECUTE);

		// switch the command's context to the one for this thread
		// we'll restore its old one after we're finished
		vvContext* pOldContext = this->mpCommand->GetContext();
		this->mpCommand->SetContext(&cContext);

		// register the current progress dialog to receive sglib progress
		wxASSERT(this->mwProgress != NULL);
		if (!this->mwProgress->RegisterSgLogHandler(cContext, true))
		{
			this->mwProgress->SetReturnCode(wxID_CANCEL);
			wxLogError("Error registering progress dialog as sglib logger: %s", cContext.Error_GetMessage());
			cContext.Error_Reset();
		}
		else
		{
			// execute the command
			this->mwProgress->ExecutionStarted();
			if (this->mpCommand->Execute())
			{
				this->mwProgress->ExecutionStopped();

				// finished successfully
				this->mwProgress->SetReturnCode(wxID_OK);

				if (this->mpCommand->GetNeedToSavePassword())
					this->mpCommand->SaveRememberedPassword();

				// make sure a successful execution didn't leave an errored context
				wxASSERT_MSG(!cContext.Error_Check(), "Command execution succeeded, but left the context in an error state.");
			}
			else
			{
				this->mwProgress->ExecutionStopped();
				// execution failed
				this->mwProgress->SetReturnCode(wxID_CANCEL);
				if (cContext.Error_Check())
				{
					if(this->mpCommand->CanHandleError(cContext.Get_Error_Code()))
					{
						while (!mwProgress->IsShown())
						{
							wxThread::Sleep(100);
						}
						vvProgressDialog::ErrorCaughtEvent * lre = new vvProgressDialog::ErrorCaughtEvent(wxID_ANY, this, this->mpCommand, cContext.Get_Error_Code());
						cContext.Error_Reset();
						wxQueueEvent(mwProgress, lre);
					}
					else
					{
						// if the context has an error, report it and reset
						cContext.Log_ReportError_CurrentError();
						cContext.Error_Reset();
					}
				}
				else
				{
					// otherwise, something must have gone wrong at the wx level
					// so log the problem directly to the wx level
					wxLogError("Command execution failed.  There may be reported and/or logged errors with more information.");
				}
			}

			// done receiving progress for the dialog
			if (!this->mwProgress->UnregisterSgLogHandler(cContext))
			{
				cContext.Error_Reset();
			}
		}

		// restore the command's old context
		this->mpCommand->SetContext(pOldContext);

		// signal that we're done executing
		// We need to lock the associated mutex first so that we
		// know that the main thread is actually waiting for the signal.
		wxMutexLocker cLocker(*this->mpMutex);
		this->mpCondition->Signal();
	}

	// return success
	return (wxThread::ExitCode)0;
}
