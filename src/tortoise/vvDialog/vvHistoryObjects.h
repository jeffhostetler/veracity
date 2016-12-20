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

#ifndef VV_HISTORY_OBJECTS_H
#define VV_HISTORY_OBJECTS_H

#include "precompiled.h"


class vvHistoryColumnPreferences
{
// types
public:
	bool ShowRevisionColumn;
	bool ShowCommentColumn;
	bool ShowBranchColumn;
	bool ShowStampColumn;
	bool ShowTagColumn;
	bool ShowDateColumn;
	bool ShowUserColumn;
	int WidthRevisionColumn;
	int WidthCommentColumn;
	int WidthBranchColumn;
	int WidthStampColumn;
	int WidthTagColumn;
	int WidthDateColumn;
	int WidthUserColumn;
	wxArrayInt columnOrder;
// construction
public:
	/**
	 * Default constructor.
	 */
	vvHistoryColumnPreferences()
	{
		ShowRevisionColumn = true;
		ShowCommentColumn = true;
		ShowBranchColumn = true;
		ShowStampColumn = true;
		ShowTagColumn = true;
		ShowDateColumn = true;
		ShowUserColumn = true;

		WidthRevisionColumn = -1;
		WidthCommentColumn = -1;
		WidthBranchColumn = -1;
		WidthStampColumn = -1;
		WidthTagColumn = -1;
		WidthDateColumn = -1;
		WidthUserColumn = -1;
	};

	void LoadColumnPreferences()
	{
	};

	void SaveColumnPreferences()
	{
	};


};



class vvHistoryFilterOptions
{
// types
public:
	wxString sUser;
	wxString sBranch;
	wxString sTag;
	wxString sStamp;
	wxDateTime dBeginDate;
	wxDateTime dEndDate;
	bool bHideMerges;
	wxString sFileOrFolderPath;
	unsigned int nResultsAtOneTime;
// construction
public:
	/**
	 * Default constructor.
	 */
	vvHistoryFilterOptions()
	{
		bHideMerges = false;
		dBeginDate = wxInvalidDateTime;
		dEndDate = wxInvalidDateTime;
		nResultsAtOneTime = 200;
	}
};

#endif
