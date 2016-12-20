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

#ifndef VV_VALIDATOR_STRING_REPORTER_H
#define VV_VALIDATOR_STRING_REPORTER_H

#include "precompiled.h"
#include "vvValidator.h"
#include "vvFlags.h"


/**
 * A collection of validation result strings.
 *
 * vvValidationReporters can add received messages to the collection,
 * and then retrieve them in various formats for display to the user.
 */
class vvValidatorMessageCollection
{
// types
public:
	/**
	 * Flags to customize the behavior of the reporter.
	 */
	enum Flags
	{
		INCLUDE_SUCCESS = 1 << 0, //< include success messages in the generated string
		SORT_BY_LABEL   = 1 << 1, //< sort the list of messages by window label
		GROUP_BY_LABEL  = 1 << 2, //< group the list of messages by window label
	};

// functionality
public:
	/**
	 * Adds a new message to the list.
	 */
	void AddMessage(
		bool            bSuccess, //< [in] True for a success message, false for an error message.
		const wxString& sLabel,   //< [in] The label of the window that the validation message is about.
		const wxString& sMessage  //< [in] The received validation message.
		);

	/**
	 * Gets the number of messages in the list
	 */
	size_t Count(
		bool bIncludeSuccess = true, //< [in] Whether or not to include success messages in the count.
		bool bIncludeError   = true  //< [in] Whether or not to include error messages in the count.
		) const;

	size_t CountError() const;

	/**
	 * Clears the list.
	 */
	void Clear();

	/**
	 * Builds the collected messages into a single combined string.
	 */
	wxString BuildMessageString(
		vvFlags         cFlags, //< [in] Flags to customize the format of the retrieved string.
		const wxString& sFormat //< [in] Usage depends on flags.
		                        //<      GROUP_BY_LABEL - The string to indent items under their label.
		                        //<      Others         - String used to separate label name from message.
		) const;

// private types
private:
	/**
	 * Data about a single message received from a validator.
	 */
	struct Message
	{
		bool     bSuccess; //< True for a success message, false for an error message.
		wxString sLabel;   //< The label of the window that was validated.
		wxString sMessage; //< The message string received from the validator.
	};

	/**
	 * A list of messages.
	 */
	typedef std::list<Message> stlMessageList;

// private functionality
private:
	/**
	 * Sort predicate for reported messages that sorts alphabetically by label.
	 * Returns true if cLeft < cRight.
	 */
	static bool SortByLabel(
		const Message& cLeft, //< [in] Left message.
		const Message& cRight //< [in] Right message.
		);

	/**
	 * Builds a string from the given list of messages in the default format 
	 */
	static wxString BuildDefaultString(
		const stlMessageList& cMessages, //< [in] Messages to include in the string.
		const wxString&       sSeparator //< [in] String used to separate label from message.
		);

	/**
	 * Builds a string from the given list of messages that is grouped by label.
	 * The given message list must be sorted by label.
	 */
	static wxString BuildGroupedString(
		const stlMessageList& cMessages, //< [in] Messages to include in the string, sorted by label.
		const wxString&       sIndent    //< [in] String to use to indent messages within their label.
		);

// member data
private:
	// internal data
	stlMessageList mcMessages; //< List of reported messages.
};


#endif
