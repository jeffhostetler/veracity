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
#include "vvValidatorMessageCollection.h"


/*
**
** Public Functions
**
*/

void vvValidatorMessageCollection::AddMessage(
	bool            bSuccess,
	const wxString& sLabel,
	const wxString& sMessage
	)
{
	Message m;
	m.bSuccess = bSuccess;
	m.sLabel   = sLabel;
	m.sMessage = sMessage;
	this->mcMessages.push_back(m);
}

size_t vvValidatorMessageCollection::Count(
	bool bIncludeSuccess,
	bool bIncludeError
	) const
{
	size_t uCount = 0u;
	for (stlMessageList::const_iterator itMessage = this->mcMessages.begin(); itMessage != this->mcMessages.end(); ++itMessage)
	{
		if (itMessage->bSuccess && bIncludeSuccess)
		{
			++uCount;
		}
		if (!itMessage->bSuccess && bIncludeError)
		{
			++uCount;
		}
	}
	return uCount;
}

void vvValidatorMessageCollection::Clear()
{
	this->mcMessages.clear();
}

wxString vvValidatorMessageCollection::BuildMessageString(
	vvFlags         cFlags,
	const wxString& sFormat
	) const
{
	// make a copy of the message list that we can manipulate if necessary
	stlMessageList cMessages;

	// copy the appropriate messages into the new list
	for (stlMessageList::const_iterator itMessage = this->mcMessages.begin(); itMessage != this->mcMessages.end(); ++itMessage)
	{
		// if this is a success message, skip it unless the user asked for it
		if (itMessage->bSuccess && !cFlags.HasAllFlags(INCLUDE_SUCCESS))
		{
			continue;
		}

		// copy the message
		cMessages.push_back(*itMessage);
	}

	// if the list needs to be sorted by label, do that
	if (cFlags.HasAnyFlag(SORT_BY_LABEL | GROUP_BY_LABEL))
	{
		cMessages.sort(SortByLabel);
	}

	// build the appropriate output string
	if (cFlags.HasAllFlags(GROUP_BY_LABEL))
	{
		return BuildGroupedString(cMessages, sFormat);
	}
	else
	{
		return BuildDefaultString(cMessages, sFormat);
	}
}

bool vvValidatorMessageCollection::SortByLabel(
	const Message& cLeft,
	const Message& cRight
	)
{
	return (cLeft.sLabel < cRight.sLabel);
}

wxString vvValidatorMessageCollection::BuildDefaultString(
	const stlMessageList& cMessages,
	const wxString&       sSeparator
	)
{
	wxString sResult = wxEmptyString;
	for (stlMessageList::const_iterator itMessage = cMessages.begin(); itMessage != cMessages.end(); ++itMessage)
	{
		if (itMessage != cMessages.begin())
		{
			sResult += "\n";
		}
		sResult += itMessage->sLabel + sSeparator + itMessage->sMessage;
	}
	return sResult;
}

wxString vvValidatorMessageCollection::BuildGroupedString(
	const stlMessageList& cMessages,
	const wxString&       sIndent
	)
{
	wxString sResult    = wxEmptyString;
	wxString sLastLabel = wxEmptyString;
	for (stlMessageList::const_iterator itMessage = cMessages.begin(); itMessage != cMessages.end(); ++itMessage)
	{
		if (itMessage != cMessages.begin())
		{
			sResult += "\n";
		}
		if (itMessage->sLabel != sLastLabel)
		{
			if (sLastLabel != wxEmptyString)
			{
				sResult += "\n";
			}
			sResult += itMessage->sLabel + "\n";
			sLastLabel = itMessage->sLabel;
		}
		sResult += sIndent + itMessage->sMessage;
	}
	return sResult;
}
