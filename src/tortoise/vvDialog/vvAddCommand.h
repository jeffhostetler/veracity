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

#ifndef VV_ADD_COMMAND_H
#define VV_ADD_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"
#include "vvAddDialog.h"
#include "vvRepoLocator.h"

/**
 * A command that adds a repository object.
 */
class vvAddCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvAddCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: add <workingfolder item>
	 */
	virtual bool ParseArguments(
		const wxArrayString& cArguments
		);

	virtual bool Init();

	/**
	 * See base class description.
	 */
	virtual class vvDialog* CreateDialog(
		wxWindow* wParent
		);

	void SetExecuteData(const wxString& sBaseFolder, const wxArrayString& repoPathsToAdd);

	/**
	 * See base class description.
	 */
	virtual bool Execute();

// member data
private:
	wxArrayString         mcDiskPaths;  //< The paths specified as arguments.
	wxArrayString         mcRepoPaths;  //< The paths specified as arguments.
	vvAddDialog * mwDialog;
	wxString msBaseFolder;
};


#endif
