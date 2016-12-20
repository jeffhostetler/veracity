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

#ifndef VV_CREATE_COMMAND_H
#define VV_CREATE_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"


/**
 * A command that creates a working copy in a given folder.
 */
class vvCreateCommand : public vvCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvCreateCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: create <directory>
	 * - directory: the directory to create a working copy in
	 */
	virtual bool ParseArguments(
		const wxArrayString& cArguments
		);

	/**
	 * See base class description.
	 */
	virtual class vvDialog* CreateDialog(
		wxWindow* wParent
		);

	/**
	 * See base class description.
	 */
	virtual bool Execute();

	virtual bool TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode);
	virtual bool CanHandleError(wxUint64 sgErrorCode) const;

// member data
private:
	wxString              msDirectory; //< The directory where we'll create a working copy.
	class vvCreateDialog* mwDialog;    //< The dialog to use to gather information from the user.
};


#endif
