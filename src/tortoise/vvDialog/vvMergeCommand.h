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

#ifndef VV_MERGE_COMMAND_H
#define VV_MERGE_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"
#include "vvRevSpec.h"

/**
 * A command that merges a revision into a working copy.
 */
class vvMergeCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvMergeCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: merge <directory>
	 * - directory: the directory of the working copy to merge a revision into
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

	void SetExecuteData(vvContext & pCtx, vvRepoLocator cRepoLocator, vvRevSpec revSpec);

	/**
	 * See base class description.
	 */
	virtual bool Execute();

// member data
private:
	wxString               msInitialSelection;  //< The initial path they passed in to the command.
	vvRevSpec			   mcRevSpec;			//< The revspec that the user chose.
	class vvRevSpecDialog* mwDialog;    //< The dialog to use to gather information from the user.
	wxString			   m_sBranch;
};


#endif
