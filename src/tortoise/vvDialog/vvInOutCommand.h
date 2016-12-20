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

#ifndef VV_INOUT_COMMAND_H
#define VV_INOUT_COMMAND_H

#include "precompiled.h"
#include "vvCommand.h"
#include "vvVerbs.h"

/**
 * A command that moves a repository object.
 */
class vvInOutCommand : public vvRepoCommand
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvInOutCommand(unsigned int extraFlags = 0);

	vvInOutCommand(bool bIncoming, wxString sLocalRepo, wxString sOtherRepo);

// implementation
public:
	/**
	 * See base class description.
	 */
	virtual bool Execute();

	vvVerbs::HistoryData GetResults();
// member data
private:
	bool			mbIncoming;
	wxString         msLocalRepo;  //< The local repository
	wxString         msOtherRepo;  //< The other repository
	vvVerbs::HistoryData mResults;
};


#endif

