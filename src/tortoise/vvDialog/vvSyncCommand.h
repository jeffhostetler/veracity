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

#ifndef VV_SYNC_COMMAND_H
#define VV_SYNC_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"
#include "vvSyncDialog.h"

/**
 * A command that moves a repository object.
 */
class vvSyncCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvSyncCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: move <workingfolder item>
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

	/**
	 * See base class description.
	 */
	virtual bool Execute();

	virtual bool CanHandleError(wxUint64 sgErrorCode) const;

	virtual bool TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode);

// member data
private:
	vvSyncDialog::SyncAction m_selectedAction;  //The operation that the user is performing.
	wxString         msDirectory;  //< The path specified as arguments.
	class vvSyncDialog* mwDialog;    //< The dialog to use to gather information from the user.
	bool m_bPushCloneIsOK;
	bool m_bMerged;
	wxString m_sOtherRepo;
};


#endif
