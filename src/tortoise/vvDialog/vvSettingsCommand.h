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

#ifndef VV_SETTINGS_COMMAND_H
#define VV_SETTINGS_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"


/**
 * A command that displays a settings dialog.
 */
class vvSettingsCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvSettingsCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * See base class description.
	 */
	bool ParseArguments(
		const wxArrayString& cArguments
		);

	/**
	 * See base class description.
	 */
	bool Init();

	/**
	 * See base class description.
	 */
	virtual class vvDialog* CreateDialog(
		wxWindow* wParent
		);

// member data
private:
	wxString                msPath;   //< The path specified to the command, or empty if none was.
	wxString                msRepo;   //< The repo specified by msPath, or empty if none.
	class vvSettingsDialog* mwDialog; //< The dialog to used to manipulate settings.
};


#endif
