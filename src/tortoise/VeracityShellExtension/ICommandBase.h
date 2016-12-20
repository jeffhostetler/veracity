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
#pragma once
#include "sg.h"
#include "sg_workingdir_prototypes.h"

class ICommandBase
{
private:
	void InitializeArgVec(SG_context * pCtx, SG_stringarray * argsArray, SG_exec_argvec ** ppargVec);
public:
	ICommandBase()
	{
		m_paHbm = NULL;
	}
	void GetPathToExe(SG_context * pCtx, SG_pathname ** pp_exe_path);
	virtual char * GetExeSwitch();
	virtual LPCTSTR GetHelpString() = 0;
	virtual bool GetPutOnPrimaryMenu() = 0;
	virtual TCHAR * GetCommandString() = 0;
	virtual bool EnableWhenSelectionCountIs(SG_uint32 selectionCount);
	virtual bool EnableWhenSelectionIsAllFiles();
	virtual bool EnableWhenSelectionIsAllFolders();
	virtual bool EnableWhenSelectionIsFilesAndFolders();
	virtual bool EnableWhenInWF(bool bInWF);
	virtual bool EnableWhenStatusIs(SG_wc_status_flags status);
	virtual bool IsSeparator();
	virtual int GetIconIdentifier();
	virtual bool DisableWhenThereIsOneHead();
	virtual bool EnableWhenThereIsNoConflict();

	HBITMAP GetBitmap(HINSTANCE hInst);

	TCHAR * FormatForPrimaryMenu(TCHAR * pCommandString);
	void RunCommand(SG_context * pCtx, SG_stringarray * pArgs);
	//SG_diffstatus_flags GetStatus(SG_context* pCtx, const char * pPathName, const char * pPathTop);
	bool StatusHas(SG_wc_status_flags testMe, SG_wc_status_flags lookForMe);

private:
	HBITMAP m_paHbm;
};

