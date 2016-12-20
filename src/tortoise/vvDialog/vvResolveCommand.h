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

#ifndef VV_RESOLVE_COMMAND_H
#define VV_RESOLVE_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"


/**
 * A command that manipulates a single conflict.
 */
class vvResolveCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvResolveCommand(unsigned int extraFlags = 0);

// implementation
public:
	/**
	 * Usage: resolve PATH
	 * - PATH:     Path of an item with a conflict.
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

// member data
private:
	wxString               msPath;   //< Path to the item we're working on.
	class vvResolveDialog* mwDialog; //< The dialog to used to manipulate the item.
};


#endif
