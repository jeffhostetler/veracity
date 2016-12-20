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

#ifndef VV_DIFF_COMMAND_H
#define VV_DIFF_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"
#include "vvRepoLocator.h"

/**
 * A command that reverts a repository object.
 */
class vvDiffCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvDiffCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: revert <workingfolder item>
	 */
	virtual bool ParseArguments(
		const wxArrayString& cArguments
		);

	virtual bool vvDiffCommand::Init();

	/**
	 * See base class description.
	 */
	virtual bool Execute();

	void vvDiffCommand::SetExecuteData(vvRepoLocator cRepoLocator, wxString& sRepoPath, wxString pRev1, wxString pRev2);
// member data
private:
	wxString msDiffObject;
	wxString msRepoPath;
	wxString msRev1;
	wxString msRev2;
};


#endif
