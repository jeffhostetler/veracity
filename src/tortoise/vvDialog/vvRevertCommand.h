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

#ifndef VV_REVERT_COMMAND_H
#define VV_REVERT_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"
#include "vvRevertDialog.h"

/**
 * A command that reverts a repository object.
 */
class vvRevertCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvRevertCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: revert <workingfolder item>
	 */
	virtual bool ParseArguments(
		const wxArrayString& cArguments
		);

	virtual bool vvRevertCommand::Init();
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

	void vvRevertCommand::SetExecuteData(vvRepoLocator cRepoLocator, bool bAll, wxArrayString * pathsToRevert, bool bSaveBackups);
// member data
private:
	wxArrayString         mcAbsolutePaths;  //< The paths specified as arguments.
	wxArrayString         mcRepoPaths;  //< The repo paths specified as arguments.
	bool				  m_bSaveBackups;
	//The results that the user chose in the dialog.
	bool				  m_bAll; //All changes should be reverted.
	wxArrayString		m_dialogResult_cSelectedFiles;
	bool				m_dialogResult_bSaveBackups;

	class vvRevertDialog* mwDialog;    //< The dialog to use to gather information from the user.
};


#endif
