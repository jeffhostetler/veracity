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

#include "StdAfx.h"
#include "ICommandBase.h"
#include "GlobalHelpers.h"
#include "VistaIconConverter.h"

#include "sg.h"

char * ICommandBase::GetExeSwitch()
{
	return "";
}

void ICommandBase::InitializeArgVec(SG_context * pCtx, SG_stringarray * argsArray, SG_exec_argvec ** ppargVec)
{
	SG_exec_argvec * pArgVec = NULL;
	SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx, &pArgVec)  );
	if (strlen(GetExeSwitch()) != 0)
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, GetExeSwitch())  );
	if (argsArray != NULL)
	{
		SG_uint32 argsLen = 0, i = 0;
		const char * pCurrentArg = NULL;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, argsArray, &argsLen)  );
		for (i = 0; i < argsLen; i++)
		{
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, argsArray, i, &pCurrentArg)  );
			SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, pCurrentArg)  );
		}
	}
	SG_RETURN_AND_NULL(pArgVec, ppargVec);
	return;
fail:
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
}

void ICommandBase::GetPathToExe(SG_context * pCtx, SG_pathname ** pp_exe_path)
{
	GlobalHelpers::findSiblingFile(pCtx, "vvDialog.exe", "vvDialog", pp_exe_path);
}

void ICommandBase::RunCommand(SG_context * pCtx, SG_stringarray * pArgs)
{
	USES_CONVERSION;
	//CA2W wide = NULL;
	SG_exec_argvec * pArgVec = NULL;
	SG_pathname * pExePath = NULL;
	wchar_t * szFilePath = NULL;
	SG_ERR_CHECK( this->GetPathToExe(pCtx, &pExePath)  );
	SG_pathname__to_unc_wchar(pCtx, pExePath, (wchar_t**)&szFilePath, NULL);
	
	SG_ERR_CHECK( InitializeArgVec(pCtx, pArgs, &pArgVec)  );
	SG_process_id procID;
	SG_ERR_CHECK( SG_exec__exec_async__files(pCtx, SG_pathname__sz(pExePath), pArgVec, NULL, NULL, NULL, &procID)  );
	//TCHAR szBuf [MAX_PATH + 32];
	//wsprintf ( szBuf, _T("The selected file was:\n\n%s"), szFilePath );
	//MessageBox(NULL,(const TCHAR *)szBuf,NULL,MB_OK|MB_ICONERROR);
fail:
	SG_NULLFREE(pCtx, szFilePath);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	SG_PATHNAME_NULLFREE(pCtx, pExePath);
}

TCHAR * ICommandBase::FormatForPrimaryMenu(TCHAR * pCommandString)
{
	if (GetPutOnPrimaryMenu())
	{
		//Concatenate "Veracity " in front of the command string, if we need to put it on the main menu
		TCHAR * newBuff = NULL;
		size_t commandLen = wcslen(pCommandString);
		//"Veracity " has 10 chars + a null
		size_t buffLen = (commandLen + 11);
		newBuff = (TCHAR *)malloc(buffLen * sizeof(TCHAR));
		newBuff[0] = 0;
		_tcscat_s(newBuff, buffLen, _T("Veracity "));
		_tcscat_s(newBuff, buffLen, pCommandString);
		return newBuff;
	}
	else 
		return pCommandString;
}

bool ICommandBase::EnableWhenSelectionCountIs(SG_uint32 selectionCount)
{
	selectionCount;
	return true;
}
bool ICommandBase::EnableWhenSelectionIsAllFiles()
{
	return true;
}
bool ICommandBase::EnableWhenSelectionIsAllFolders()
{
	return true;
}
bool ICommandBase::EnableWhenSelectionIsFilesAndFolders()
{
	return true;
}
bool ICommandBase::EnableWhenStatusIs(SG_wc_status_flags status)
{
	status;
	return true;
}
bool ICommandBase::EnableWhenInWF(bool bInWF)
{
	return bInWF;
}
bool ICommandBase::IsSeparator()
{
	return false;
}
bool ICommandBase::DisableWhenThereIsOneHead()
{
	return false;
}
int ICommandBase::GetIconIdentifier()
{
	return 0;
}
/*SG_wc_status_flags ICommandBase::GetStatus(SG_context* pCtx, const char * pPathName, const char * pPathTop, SG_uint16 * pnLockStatus)
{	
	SG_wc_status_flags statusFlags = SG_wc_status_flags__ZERO;
	SG_ERR_CHECK(  statusFlags = GlobalHelpers::GetStatus(pCtx, pPathName, pPathTop, pnLockStatus)  );
fail:
	return statusFlags;
	
}*/

//This actually tries to load the icon and draw the
//special bitmap format the Vista requires.
HBITMAP ICommandBase::GetBitmap(HINSTANCE hInst)
{
	HBITMAP paHbm = NULL;
	if (m_paHbm == NULL)
	{
		HICON myIcon = LoadIcon(hInst, MAKEINTRESOURCE(GetIconIdentifier()));
		if (myIcon != NULL)
			CreateBitmapForVista(myIcon, &paHbm);
		DestroyIcon(myIcon);
		m_paHbm = paHbm;
	}
	return m_paHbm;
}

bool ICommandBase::StatusHas(SG_wc_status_flags testMe, SG_wc_status_flags lookForMe)
{
	return (testMe & lookForMe) == lookForMe;
}

bool ICommandBase::EnableWhenThereIsNoConflict()
{
	return true;
}
