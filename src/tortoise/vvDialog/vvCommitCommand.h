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

#ifndef VV_COMMIT_COMMAND_H
#define VV_COMMIT_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"


/**
 * A command that commits a working copy.
 */
class vvCommitCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvCommitCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: status path1 [path2 ...]
	 * - pathN: A path containing the working copy to commit.
	 *          All specified paths must be in the same working copy.
	 */
	virtual bool ParseArguments(
		const wxArrayString& cArguments
		);

	/**
	 * See base class description.
	 */
	virtual bool Init();

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

// member data
private:
	wxArrayString         mcAbsolutePaths; //< The paths specified as arguments.
	wxArrayString         mcRepoPaths;     //< mcAbsolutePaths translated to repo paths relative to msFolder.
	class vvCommitDialog* mwDialog;        //< The dialog to used to gather commit info.
};


#endif
