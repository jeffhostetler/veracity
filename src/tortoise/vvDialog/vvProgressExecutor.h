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

#ifndef VV_PROGRESS_EXECUTOR_H
#define VV_PROGRESS_EXECUTOR_H

#include "precompiled.h"


/**
 * Executes vvCommand instances in a background thread
 * while displaying a modal vvProgressDialog in the main GUI thread.
 */
class vvProgressExecutor : public wxThreadHelper
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvProgressExecutor(
		class vvCommand& cCommand //< [in] The command to execute.
		);

	/**
	 * Destructor.
	 */
	~vvProgressExecutor();

// functionality
public:
	/**
	 * Checks if the executor was constructed successfully.
	 */
	bool IsOk() const;

	/**
	 * Gets the progress dialog timeout.  See SetTimeout.
	 */
	unsigned int GetTimeout() const;

	/**
	 * Sets the timeout in milliseconds that the executor will wait
	 * for the command to finish before displaying a progress dialog.
	 */
	void SetTimeout(
		unsigned int uTimeout //< [in] Milliseconds to wait before displaying a progress dialog.
		);

	void vvProgressExecutor::PostExecuteMessage();

	/**
	 * Executes the configured command in a background thread.
	 * Returns true if the command executed successfully, or false if there was an error.
	 */
	bool Execute(
		wxWindow* wParent = NULL //< [in] A parent window for the progress dialog.
		);

// private types
private:
	/**
	 * Messages sent from the main thread to the background thread.
	 * These are sent via a wxMessageQueue.
	 */
	enum ThreadMessage
	{
		MESSAGE_EXECUTE, //< Background thread should execute the command.
		MESSAGE_EXIT,    //< Background thread should exit.
		MESSAGE_COUNT,   //< Number of values in the enumeration, for iteration.
	};

// private functionality
private:
	/**
	 * Main function for the background execution thread.
	 */
	wxThread::ExitCode Entry();

// private data
private:
	class vvCommand*              mpCommand;   //< The command to execute.
	wxMessageQueue<ThreadMessage> mcMessages;  //< Message queue to pass messages to the background thread.
	unsigned int                  muTimeout;   //< Timeout to wait before displaying the progress dialog.
	class vvProgressDialog*       mwProgress;  //< Progress dialog currently being displayed (NULL while command isn't being executed).
	wxCondition*                  mpCondition; //< Condition to signal when the background thread finishes (NULL while command isn't being executed).
	wxMutex*                      mpMutex;     //< Mutex associated with mpCondition (NULL when mpCondition is).
};


#endif
