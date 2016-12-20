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

#ifndef VV_VALIDATOR_STATUS_H
#define VV_VALIDATOR_STATUS_H

#include "precompiled.h"
#include "vvValidator.h"


/**
 * A composite control that displays output from validators
 * as static text, and provides a button to re-run them.
 */
class vvValidatorStatusButton : public wxButton, public vvValidatorReporter
{
// construction
public:
	vvValidatorStatusButton();

	vvValidatorStatusButton(
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxString&    sLabel     = wxEmptyString,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxSize(16,16),
		long               iStyle     = wxBU_EXACTFIT | wxBORDER_NONE,
		const wxValidator& cValidator = wxDefaultValidator,
		const wxString&    sName      = wxButtonNameStr
		);

// initialization
public:
	bool Create(
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxString&    sLabel     = wxEmptyString,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxSize(16,16),
		long               iStyle     = wxBU_EXACTFIT | wxBORDER_NONE,
		const wxValidator& cValidator = wxDefaultValidator,
		const wxString&    sName      = wxButtonNameStr
		);

// functionality
public:
	/**
	 * Clears the set of recorded messages to make room for a new set.
	 */
	virtual void BeginReporting(
		wxWindow* wWindow //< [in] The window that will be validated.
		);

	/**
	 * Updates the button's display to accomodate newly recorded data.
	 */
	virtual void EndReporting(
		wxWindow* wWindow //< [in] The window that was validated.
		);

	/**
	 * Reports a successful message from a validator.
	 */
	virtual void ReportSuccess(
		class vvValidator* pValidator, //< [in] The validator reporting the success.
		const wxString&    sMessage    //< [in] A message describing the successful validation.
		);

	/**
	 * Reports an error message from a validator.
	 */
	virtual void ReportError(
		class vvValidator* pValidator, //< [in] The validator reporting the error.
		const wxString&    sMessage    //< [in] A message describing the error.
		);

// internal helpers
public:
	/**
	 * Update the control's display based on its internal data.
	 */
	void UpdateDisplay();

// event handlers
protected:
	void OnClick(wxCommandEvent& cEvent);

// private types
private:
	/**
	 * Data about a single message received from a validator.
	 */
	struct Message
	{
		vvValidator* pValidator; //< The validator the message was received from.
		bool         bSuccess;   //< True for a success message, false for an error message.
		wxString     sMessage;   //< The message string received from the validator.
	};

	/**
	 * A list of messages.
	 */
	typedef std::list<Message> stlMessageList;

// private functionality
private:
	/**
	 * Adds a new message to the list.
	 */
	void AddMessage(
		vvValidator*    pValidator, //< [in] The validator the message came from.
		bool            bSuccess,   //< [in] True for a success message, false for an error message.
		const wxString& sMessage    //< [in] The received message.
		);

// member data
private:
	// internal data
	stlMessageList mcMessages; //< List of reported messages.

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvValidatorStatusButton);
	DECLARE_EVENT_TABLE();
};


#endif
