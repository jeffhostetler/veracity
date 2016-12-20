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

#ifndef VV_UPDATE_COMMAND_H
#define VV_UPDATE_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"
#include "vvVerbs.h"
#include "vvRevSpec.h"
/**
 * A command that Updates a repository object.
 */
class vvUpdateCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvUpdateCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: update <local path to working copy>
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

	void SetExecuteData(vvRepoLocator cRepoLocator, vvRevSpec cRevSpec);
	/**
	 * See base class description.
	 */
	virtual bool Execute();

	virtual bool CanHandleError(wxUint64 sgErrorCode) const;
	virtual bool TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode);

// member data
private:
	wxArrayString         mcPaths;  //< The paths specified as arguments.
	vvRevSpec m_cRevSpec;
	vvNullable<bool> m_nbDetach;
	wxString m_sBranch;
	class vvRevSpecDialog* mwDialog;    //< The dialog to use to gather information from the user.
};


#endif
