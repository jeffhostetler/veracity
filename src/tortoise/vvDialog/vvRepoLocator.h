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

#ifndef VV_REPOLOCATOR_H
#define VV_REPOLOCATOR_H

#include "precompiled.h"


/**
 * This class encapsulates the ways to find a repo.  At the moment, there are two:
 *    1.  A working copy path
 *    2.  The repo descriptor name.
 * This is necessary, because some dialogs and verbs can use either method of referencing a 
 * repo.  History, Status (treediff), and Diff are some examples of commands that can do 
 * it either way.  The way to get a vvRepoLocator object is to use one of the static "From*" methods.
 */
class vvRepoLocator
{
private: 
	bool 		m_bIsWorkingFolder;
	wxString 	m_sWorkingFolderPath;
	wxString 	m_sRepoName;
	

private:
	vvRepoLocator(bool bIsWorkingFolder, const wxString& sWorkingFolder, const wxString& sRepoName)
		:m_sWorkingFolderPath(sWorkingFolder), m_sRepoName(sRepoName)
	{
		m_bIsWorkingFolder = bIsWorkingFolder;
	}
// functionality
public:
	vvRepoLocator()
	{
	}

	static vvRepoLocator FromRepoNameAndWorkingFolderPath(const wxString& sRepoName, const wxString& sWorkingFolderPath)
	{
		return vvRepoLocator(true, sWorkingFolderPath, sRepoName);
	}
	
	static vvRepoLocator FromRepoName(const wxString& sRepoName)
	{
		return vvRepoLocator(false, wxEmptyString, sRepoName);
	}

	static vvRepoLocator FromWorkingFolder(const wxString& sWorkingFolder)
	{
		return vvRepoLocator(true, sWorkingFolder, wxEmptyString);
	}
	
	bool IsWorkingFolder() const
	{
		return m_bIsWorkingFolder;
	}

	const wxString& GetWorkingFolder() const
	{
		return m_sWorkingFolderPath;
	}

	const wxString& GetRepoName() const
	{
		return m_sRepoName;
	}
};

#endif
