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

#ifndef VV_VALIDATOR_MESSAGE_BOX_REPORTER_H
#define VV_VALIDATOR_MESSAGE_BOX_REPORTER_H

#include "precompiled.h"
#include "vvValidatorMessageCollection.h"


/**
 * A vvValidatorReporter that reports error messages in a message box.
 */
class vvValidatorMessageBoxReporter : public vvValidatorReporter
{
// functionality
public:
	/**
	 * Sets a help message to be displayed at the top of the message box.
	 */
	void SetHelpMessage(
		const wxString& sMessage //< [in] The help message to display.
		);

// implementation
public:
	/**
	 * See base class documentation.
	 */
	virtual void BeginReporting(
		wxWindow* wParent
		);

	/**
	 * See base class documentation.
	 */
	virtual void EndReporting(
		wxWindow* wParent
		);

	/**
	 * See base class documentation.
	 */
	virtual void ReportSuccess(
		class vvValidator* pValidator,
		const wxString&    sMessage
		);

	/**
	 * See base class documentation.
	 */
	virtual void ReportError(
		class vvValidator* pValidator,
		const wxString&    sMessage
		);

// private data
private:
	wxString                     msHelpMessage; //< Message to display at the top of the message box.
	vvValidatorMessageCollection mcMessages;    //< Messages collected from validation.
};


#endif
