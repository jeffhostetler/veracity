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
// VeracityContextMenu.cpp : Implementation of CVeracityContextMenu

#include "stdafx.h"
#include "VeracityContextMenu.h"
#include "ICommandBase.h"
#include "Commands.h"
#include <cstringt.h>
#include "dllmain.h"
#include "resource.h"
#include "GlobalHelpers.h"

#include "sg.h"
#include "sg_workingdir_prototypes.h"

//Global
//This global has two purposes.  It's the text to use for the menu item,
//but it's global so that we can check to see if the menu has been added 
//to the context menu before.
LPWSTR gVeracityMenuItemText = _T("Veracity");



///////////////////////////////////////////////////////////////////////////////////
// They are necessary, because of an error report at:
// http://veracity-scm.com/qa/questions/1529/tortoise-problem-with-folder-shortcuts
// and tracked at bug A4815.
//
// This code will translate a IDList to the target path.
///////////////////////////////////////////////////////////////////////////////////
// Retrieves the UIObject interface for the specified full PIDLstatic 
HRESULT SHGetUIObjectFromFullPIDL(LPCITEMIDLIST pidl, HWND hwnd, REFIID riid, void **ppv)
{    
    LPCITEMIDLIST pidlChild;    
    IShellFolder* psf;    
    *ppv = NULL;    
    
    HRESULT hr = SHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&psf, &pidlChild);    
    if (SUCCEEDED(hr))    
    {        
        hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, NULL, ppv);        
        psf->Release();    
    }    
    return hr;
}

static HRESULT SHILClone(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl)
{    
    DWORD cbTotal = 0;    
    if (pidl)
    {        
        LPCITEMIDLIST pidl_temp = pidl;        
        cbTotal += pidl_temp->mkid.cb;        
        
        while (pidl_temp->mkid.cb)         
        {            
            cbTotal += pidl_temp->mkid.cb;            
            pidl_temp = ILNext(pidl_temp);        
        }    
    }    
    
    *ppidl = (LPITEMIDLIST)CoTaskMemAlloc(cbTotal);    
    if (*ppidl)        
        CopyMemory(*ppidl, pidl, cbTotal);    
        
    return  *ppidl ? S_OK: E_OUTOFMEMORY;
}
    
// Get the target PIDL for a folder PIDL. This also deals with cases of a folder  
// shortcut or an alias to a real folder.
static HRESULT SHGetTargetFolderIDList(LPCITEMIDLIST pidlFolder, LPITEMIDLIST *ppidl)
{    
    IShellLink *psl;    
    *ppidl = NULL;    
    
    HRESULT hr = SHGetUIObjectFromFullPIDL(pidlFolder, NULL, IID_IShellLink, (LPVOID*)&psl);    
    if (SUCCEEDED(hr))    
    {        
        hr = psl->GetIDList(ppidl);        
        psl->Release();    
    }    
    
    // It's not a folder shortcut so get the PIDL normally.    
    if (FAILED(hr))        
        hr = SHILClone(pidlFolder, ppidl);    
    return hr;
}

// Get the target folder for a folder PIDL. This deals with cases where a folder
// is an alias to a real folder, folder shortcuts, the My Documents folder, 
// and so on.
STDAPI SHGetTargetFolderPath(LPCITEMIDLIST pidlFolder, LPWSTR pszPath, UINT cchPath)
{    
    LPITEMIDLIST pidlTarget;    
	cchPath;
    *pszPath = 0;    
    
    HRESULT hr = SHGetTargetFolderIDList(pidlFolder, &pidlTarget);    
    if (SUCCEEDED(hr))    
    {        
        SHGetPathFromIDListW(pidlTarget, pszPath);   
        
        // Make sure it is a path        
        CoTaskMemFree(pidlTarget);    
    }    
    
    return *pszPath ? S_OK : E_FAIL;
}
///////////////////////////////////////////////////////////////////////////////////
// End IDL translation code.
///////////////////////////////////////////////////////////////////////////////////

// CVeracityContextMenu
/////////////////////////////////////////////////////////////////////////////
// CVeracityContextMenu

STDMETHODIMP CVeracityContextMenu::Initialize (
    LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hProgID )
{
	FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stg = { TYMED_HGLOBAL };
	HDROP     hDrop;
	pDataObj;
	hProgID;
	pidlFolder;
    // Look for CF_HDROP data in the data object.
	TCHAR path [MAX_PATH];

	//MessageBox(NULL,(const TCHAR *)_T(__FUNCTION__),NULL,MB_OK|MB_ICONERROR);

	SG_context * pCtx = NULL;
	SG_CONTEXT__ALLOC__TEMPORARY(&pCtx);
	SG_string * pstr_selectedFile = NULL;
	SG_STRINGARRAY_NULLFREE(pCtx, mp_selectedPaths);
	SG_RBTREE__ALLOC(pCtx, &mp_contextMenuItemsByMenuID);
	SG_RBTREE__ALLOC(pCtx, &mp_contextMenuItemsByOurID);
	SG_VECTOR__ALLOC(pCtx, &mp_vecBitmaps, COMMAND_COUNT);
	SG_VECTOR__ALLOC(pCtx, &mp_vecStrings, COMMAND_COUNT);
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &mp_selectedPaths, 5)  );
	if (pDataObj == NULL)
	{
		if (SUCCEEDED(SHGetTargetFolderPath(pidlFolder, path, 0)))
		{
			if (GlobalHelpers::CheckDriveType(pCtx, path))
			{
				SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_selectedFile)  );
				SG_ERR_CHECK(  SG_UTF8__INTERN_FROM_OS_BUFFER(pCtx, pstr_selectedFile, path)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, mp_selectedPaths, SG_string__sz(pstr_selectedFile))  );
				SG_ERR_CHECK(  SG_STRING_NULLFREE(pCtx, pstr_selectedFile)  );
				SG_CONTEXT_NULLFREE(pCtx);
				return S_OK;
			}
			SG_CONTEXT_NULLFREE(pCtx);
			return E_INVALIDARG;
			
		}
		else
		{
			SG_CONTEXT_NULLFREE(pCtx);
			return E_INVALIDARG;
		}
	}
    if ( FAILED( pDataObj->GetData ( &fmt, &stg ) ))
        {
        // Nope! Return an "invalid argument" error back to Explorer.
		SG_CONTEXT_NULLFREE(pCtx);
        return E_INVALIDARG;
        }
    // Get a pointer to the actual data.
    hDrop = (HDROP) GlobalLock ( stg.hGlobal );

	// Make sure it worked.
    if ( NULL == hDrop )
	{
		SG_CONTEXT_NULLFREE(pCtx);
        return E_INVALIDARG;
	}

    // Sanity check - make sure there is at least one filename.
	UINT uNumFiles = DragQueryFile ( hDrop, 0xFFFFFFFF, NULL, 0 );
	HRESULT hr = S_OK;

    if ( 0 == uNumFiles )
    {
        GlobalUnlock ( stg.hGlobal );
        ReleaseStgMedium ( &stg );
		SG_CONTEXT_NULLFREE(pCtx);
        return E_INVALIDARG;
    }
	UINT index = 0;
	for (index = 0; index < uNumFiles; index++)
	{
		// Get the name of the first file and store it in our member variable m_szFile.
		if ( 0 == DragQueryFile ( hDrop, index, path, MAX_PATH ) )
			hr = E_INVALIDARG;
		if (GlobalHelpers::CheckDriveType(pCtx, path))
		{
			LPITEMIDLIST idl = ILCreateFromPath(path);
			if (SUCCEEDED(SHGetTargetFolderPath(idl, path, 0)))
			{
				SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_selectedFile)  );
				SG_ERR_CHECK(  SG_UTF8__INTERN_FROM_OS_BUFFER(pCtx, pstr_selectedFile, path)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, mp_selectedPaths, SG_string__sz(pstr_selectedFile))  );
				SG_ERR_CHECK(  SG_STRING_NULLFREE(pCtx, pstr_selectedFile)  );
			}
			ILFree(idl);
		}
	}
	SG_uint32 nPathCount = 0;

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, mp_selectedPaths, &nPathCount)  );
	//None of the paths they passed us panned out.  Return invalid.
	if (nPathCount == 0)
	{
		GlobalUnlock ( stg.hGlobal );
        ReleaseStgMedium ( &stg );
		SG_CONTEXT_NULLFREE(pCtx);
        return E_INVALIDARG;
	}
    GlobalUnlock ( stg.hGlobal );
    ReleaseStgMedium ( &stg );
	SG_CONTEXT_NULLFREE(pCtx);
    return hr;
fail:
	SG_ERR_CHECK(  SG_STRING_NULLFREE(pCtx, pstr_selectedFile)  );
	SG_STRINGARRAY_NULLFREE(pCtx, mp_selectedPaths);
	SG_VECTOR_NULLFREE(pCtx, mp_vecBitmaps);
	SG_VECTOR_NULLFREE(pCtx, mp_vecStrings);
	SG_RBTREE_NULLFREE(pCtx, mp_contextMenuItemsByMenuID);
	SG_RBTREE_NULLFREE(pCtx, mp_contextMenuItemsByOurID);
	SG_CONTEXT_NULLFREE(pCtx);
	return E_UNEXPECTED;
}

void CVeracityContextMenu::ConfigureContextMenu(SG_context* pCtx)
{
	if (bAlreadyConfigured)
		return;
	
	//MessageBox(NULL,(const TCHAR *)_T(__FUNCTION__),NULL,MB_OK|MB_ICONERROR);

	m_hInst = GetModuleHandle(_T("VeracityTortoise.dll") );

	/////////Version checking to determine if we're on Visat or later////////////
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op=VER_GREATER_EQUAL;

	// Initialize the OSVERSIONINFOEX structure.

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 0;
	osvi.wServicePackMajor = 0;
	osvi.wServicePackMinor = 0;

	// Initialize the condition mask.

	VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, (BYTE)op );
	VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, (BYTE)op );
	VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, (BYTE)op );
	VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, (BYTE)op );

	// Perform the test.

	if (VerifyVersionInfo(
		&osvi, 
		VER_MAJORVERSION | VER_MINORVERSION | 
		VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
		dwlConditionMask))
	{
		bIsVistaOrLater = true;
	}
	//////////////End of version checking//////////////////

	//////////////Load sg settings that control context menu behavior//////////////
	char * pszSettingValue = NULL;
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, 
		SG_LOCALSETTING__TORTOISE_EXPLORER__HIDE_MENU_IF_NO_WORKING_COPY,
		NULL,
		&pszSettingValue,
		NULL)  );

	bHideMenuIfNoWorkingFolder = false;
	//Very specifically, we interperet any value that is not "true" as SG_FALSE,
	//since we would rather err on the side of showing the context menu.
	if (SG_stricmp__null(pszSettingValue, "true") == 0)
	{
		bHideMenuIfNoWorkingFolder = true;
	}
	//////////////End of sg settings load///////////////////

	m_sub = new SubmenuCommand();
	int i = 0;
	m_commandsArray[i++] = new CreateWFCommand();
	m_commandsArray[i++] = new SyncCommand();
	m_commandsArray[i++] = new CommitCommand();
	m_commandsArray[i++] = new SeparatorCommand();
	m_commandsArray[i++] = new StatusCommand();
	m_commandsArray[i++] = new DiffCommand();
	m_commandsArray[i++] = new RevertCommand();
	m_commandsArray[i++] = new SeparatorCommand();
	m_commandsArray[i++] = new AddCommand();
	m_commandsArray[i++] = new MoveCommand();
	m_commandsArray[i++] = new DeleteCommand();
	m_commandsArray[i++] = new RenameCommand();
	m_commandsArray[i++] = new LockCommand();
	m_commandsArray[i++] = new SeparatorCommand();
	m_commandsArray[i++] = new HistoryCommand();
	m_commandsArray[i++] = new MergeCommand();
	//m_commandsArray[i++] = new ResolveCommand();
	//m_commandsArray[i++] = new BranchesCommand();
	m_commandsArray[i++] = new UpdateCommand();
	m_commandsArray[i++] = new SeparatorCommand();
	//m_commandsArray[i++] = new DagCommand();
	m_commandsArray[i++] = new ServeCommand();
	m_commandsArray[i++] = new SettingsCommand();
	m_commandsArray[i++] = new AboutCommand();
	/*m_commandsArray[i++] = new ManageReposCommand(); 
	m_commandsArray[i++] = new HelpCommand();*/

	bAlreadyConfigured = true;
fail:
	SG_NULLFREE(pCtx, pszSettingValue);

}
STDMETHODIMP CVeracityContextMenu::QueryContextMenu (
    HMENU hmenu, UINT uMenuIndex, UINT uidFirstCmd,
    UINT uidLastCmd, UINT uFlags )
{
	//MessageBox(NULL,(const TCHAR *)_T(__FUNCTION__),NULL,MB_OK|MB_ICONERROR);
//	int number_of_commands =0;
	SG_context* pCtx = NULL;
	SG_pathname * pPathCwd = NULL;
	SG_bool bInWF = SG_TRUE;
	uidLastCmd;
    // If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
    if ( uFlags & CMF_DEFAULTONLY )
        return MAKE_HRESULT ( SEVERITY_SUCCESS, FACILITY_NULL, 0 );
	
	/////////////////////////////////////////////////////////
	//Begin checks to see if we should even 
	//put ourselves on the context menu.

	//// First check to see if we've already put ourselves 
	//// onto this menu.  This occurs most commonly in the 
	//// tree pane of Windows Explorer.

	int nMenuItemCount = GetMenuItemCount(hmenu);

	for (int i = 0; i < nMenuItemCount; i++)
	{
		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA;
		GetMenuItemInfo(hmenu, i, true, &mii);
		if (mii.dwItemData == (ULONG_PTR)gVeracityMenuItemText)
			return S_OK; //We are already on this menu.
	}

	SG_CONTEXT__ALLOC(&pCtx);
	ConfigureContextMenu(pCtx);

	//Create the submenu.
	HMENU hSubmenu = CreatePopupMenu();

	//Style it properly.
	MENUINFO subMenuInfo;
	memset(&subMenuInfo, 0, sizeof(subMenuInfo));

	subMenuInfo.cbSize  = sizeof(subMenuInfo);
	subMenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
	subMenuInfo.dwStyle = MNS_CHECKORBMP;
	SetMenuInfo(hSubmenu, &subMenuInfo);

	MENUITEMINFO miiSubMenu = { sizeof(MENUITEMINFO) };
    UINT uID = uidFirstCmd;

	bool haveInspectedPaths = false;
	bool allFiles = false;
	bool allFolders = false;
	bool mixed = false;
	SG_uint32 selectionCount = 0;
	const char * pszCurrentPath = NULL;
	SG_pathname * pCurrentPath = NULL;
	SG_string * pstrRepoDescriptorName = NULL;
	
	SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, mp_selectedPaths, 0, &pszCurrentPath)  );
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, mp_selectedPaths, &selectionCount)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathCwd, pszCurrentPath)  );
	
	SG_workingdir__find_mapping(pCtx, pPathCwd, NULL, &pstrRepoDescriptorName, NULL);
	
	if (SG_context__err_equals(pCtx, SG_ERR_NOT_A_WORKING_COPY) || SG_context__err_equals(pCtx, SG_ERR_NOTAREPOSITORY))
	{
		bInWF = SG_FALSE;
		SG_context__err_reset(pCtx);
	}
	else
		SG_ERR_CHECK_CURRENT;
	
	int commandIndex = 0;
	SG_uint32 subMenuIndex = 0;
	bool bLastWasSeparator = true;
	bool bWFHasOneHead = false;
	SG_int_to_string_buffer buf;

	if (bInWF == SG_TRUE)
	{
		int headCount = 1;
		//For performance reasons, we no longer check the
		//number of heads.  We want to minize the work done 
		//in the context menu.
		//SG_ERR_CHECK(  headCount = GlobalHelpers::GetHeadCount(pCtx, pszCurrentPath)  );
		bWFHasOneHead = headCount == 1;
	}
	else if (bHideMenuIfNoWorkingFolder)
	{
		//They set the option to disable the context menu
		//if they're outside the working folder.  Just return.
		//Jump to fail, in order to clean up.
		SG_PATHNAME_NULLFREE(pCtx,  pPathCwd);
		SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
		SG_CONTEXT_NULLFREE(pCtx);
		return S_OK;
	}

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pCurrentPath)  );

	//Start things off with a separator.
	InsertMenu(hmenu, uMenuIndex++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
	
	for (commandIndex =0; commandIndex < COMMAND_COUNT; commandIndex++)
	{
		ICommandBase * CurrentCommand = m_commandsArray[commandIndex];
		if (CurrentCommand->IsSeparator())
		{
			if (!bLastWasSeparator)
				InsertMenu(hSubmenu, subMenuIndex++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
			bLastWasSeparator = true;
			continue;
		}

		if (!CurrentCommand->EnableWhenInWF(bInWF == SG_TRUE))
			continue;
		//See above comment about minimizing work in the context menu.
		//if (!bWFHasOneHead && CurrentCommand->DisableWhenThereIsOneHead())
		//	continue;
		if (!CurrentCommand->EnableWhenSelectionCountIs(selectionCount)  )
			continue;
		SG_uint32 selectionIndex = 0;
		if (haveInspectedPaths == false)
		{
			//Loop over the strings to identify if it's all files, all folders or a mix.
			for (selectionIndex = 0; selectionIndex < selectionCount; selectionIndex++)
			{
				SG_fsobj_stat statResults;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, mp_selectedPaths, selectionIndex, &pszCurrentPath)  );
				SG_ERR_CHECK(  SG_pathname__set__from_sz(pCtx, pCurrentPath, pszCurrentPath)  );
				SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pCurrentPath, &statResults)  );
				if (statResults.type == SG_FSOBJ_TYPE__DIRECTORY)
				{
					if (allFiles)
					{
						//Once we determine that we're mixed, we can quit.
						allFiles = false;
						mixed = true;
						SG_PATHNAME_NULLFREE(pCtx,  pCurrentPath);
						break;
					}
					else if (!allFolders)
						allFolders = true;
				}
				else if(statResults.type == SG_FSOBJ_TYPE__REGULAR)
				{
					if (allFolders)
					{
						//Once we determine that we're mixed, we can quit.
						allFolders = false;
						mixed = true;
						SG_PATHNAME_NULLFREE(pCtx,  pCurrentPath);
						break;
					}
					else if (!allFiles)
						allFiles = true;
				}
				else
				{
					mixed = true;
					SG_PATHNAME_NULLFREE(pCtx,  pCurrentPath);
					break;
				}
			}
			haveInspectedPaths = true;
		}
		if (mixed && !CurrentCommand->EnableWhenSelectionIsFilesAndFolders()  )
			continue;
		if (allFiles && !CurrentCommand->EnableWhenSelectionIsAllFiles()  )
			continue;
		if (allFolders && !CurrentCommand->EnableWhenSelectionIsAllFolders()  )
			continue;
		bool bSkipThisCommand = false;
		if (bInWF)
		{
			//Skip enable/disable by status.
		/*	for (selectionIndex = 0; selectionIndex < selectionCount; selectionIndex++)
			{
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, mp_selectedPaths, selectionIndex, &pszCurrentPath)  );
				SG_diffstatus_flags status = CurrentCommand->GetStatus(pCtx, pszCurrentPath);
				if (!CurrentCommand->EnableWhenStatusIs(status))
				{
					bSkipThisCommand = true;
					break;
				}
			}
			//We aren't checking for conflicts anymore, either.
			//due to performance considerations.
			if (!bSkipThisCommand)
			{
				for (selectionIndex = 0; selectionIndex < selectionCount; selectionIndex++)
				{
					SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, mp_selectedPaths, selectionIndex, &pszCurrentPath)  );
					bool isConflict = GlobalHelpers::IsConflict(pCtx, pszCurrentPath);
					if (!isConflict && !CurrentCommand->EnableWhenThereIsNoConflict())
					{
						bSkipThisCommand = true;
						break;
					}
				}
			}*/
		}
		if (bSkipThisCommand)
			continue;
		//From diffmerge
	
		
		int iconID = CurrentCommand->GetIconIdentifier();
		TCHAR * ptchar_CommandString = CurrentCommand->GetCommandString();

		if (iconID != 0)
		{
			MENUITEMINFO mii; 
			memset(&mii,0,sizeof(mii));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_BITMAP ;
			mii.fType = MFT_STRING;
			mii.wID = uID++;
			mii.dwTypeData = ptchar_CommandString;
			mii.cch = (UINT)wcslen(mii.dwTypeData);
			mii.fState = MFS_ENABLED;
			
			if (bIsVistaOrLater)
			{
				//Vista and later use a more sophistcated technique for drawing bitmaps.
				HBITMAP hbmp = CurrentCommand->GetBitmap(m_hInst);
				SG_ERR_CHECK(  SG_vector__append(pCtx, mp_vecBitmaps, hbmp, NULL)  );
				mii.hbmpItem = hbmp;
			}
			else
			{
				//XP uses a different mechanism.  Look at the HandleMenuMsg2 function below.
				mii.hbmpItem = HBMMENU_CALLBACK;
			}
			if (CurrentCommand->GetPutOnPrimaryMenu() == true)
			{
				InsertMenuItem ( hmenu, uMenuIndex++, TRUE, &mii );
				SG_ERR_CHECK(  SG_vector__append(pCtx, mp_vecStrings, ptchar_CommandString, NULL)  ); //When we put something on the primary menu, we do a malloc.  We need to free it
			}
			else
				InsertMenuItem( hSubmenu, subMenuIndex++, false, &mii);
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, mp_contextMenuItemsByOurID, SG_int64_to_sz(uID - uidFirstCmd - 1, buf), CurrentCommand)  );
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, mp_contextMenuItemsByMenuID, SG_int64_to_sz(uID - 1, buf), CurrentCommand)  );
		}
		else
		{
			if (CurrentCommand->GetPutOnPrimaryMenu() == true)
			{
				InsertMenu ( hmenu, uMenuIndex++, MF_BYPOSITION, uID++, ptchar_CommandString);
				SG_ERR_CHECK(  SG_vector__append(pCtx, mp_vecStrings, ptchar_CommandString, NULL)  ); //When we put something on the primary menu, we do a malloc.  We need to free it
			}
			else
				InsertMenu ( hSubmenu, subMenuIndex++, MF_BYPOSITION, uID++, ptchar_CommandString);
			
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, mp_contextMenuItemsByOurID, SG_int64_to_sz(uID - uidFirstCmd - 1, buf), CurrentCommand)  );
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, mp_contextMenuItemsByMenuID, SG_int64_to_sz(uID - 1, buf), CurrentCommand)  );
		}
		//If we didn't put the current command on the secondary menu
		//then we still need to guard against putting a separator on the 
		//secondary menu.  
		if (CurrentCommand->GetPutOnPrimaryMenu() == false)
			bLastWasSeparator = false;
	}

	

	if ((uID - uidFirstCmd) > 0)
	{
		memset(&miiSubMenu,0,sizeof(miiSubMenu));
		miiSubMenu.cbSize = sizeof(MENUITEMINFO);
		miiSubMenu.fMask = MIIM_SUBMENU | MIIM_FTYPE | MIIM_BITMAP | MIIM_STRING | MIIM_ID | MIIM_STATE | MIIM_DATA;
		miiSubMenu.fType = MFT_STRING;
		miiSubMenu.wID = uID++;
		miiSubMenu.hSubMenu = hSubmenu;
		miiSubMenu.fState = MFS_ENABLED;
		//The string that is displayed
		miiSubMenu.dwTypeData = gVeracityMenuItemText;
		//a pointer that is passed around.  This is used to check if we've already 
		//been added.
		miiSubMenu.dwItemData = (ULONG_PTR)gVeracityMenuItemText;
		miiSubMenu.cch = (UINT)wcslen(miiSubMenu.dwTypeData);
		if (bIsVistaOrLater)
		{
			//Vista and later use a more sophistcated technique for drawing bitmaps.
			miiSubMenu.hbmpItem = m_sub->GetBitmap(m_hInst);
		}
		else
		{
			//XP uses a different mechanism.  Look at the HandleMenuMsg2 function below.
			miiSubMenu.hbmpItem = HBMMENU_CALLBACK;
		}
		InsertMenuItem ( hmenu, uMenuIndex++, TRUE, &miiSubMenu );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, mp_contextMenuItemsByOurID, SG_int64_to_sz(uID - uidFirstCmd - 1, buf), m_sub)  );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, mp_contextMenuItemsByMenuID, SG_int64_to_sz(uID - 1, buf), m_sub)  );
	}
	//Append a separator.
	InsertMenu(hmenu, uMenuIndex++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
	
	SG_PATHNAME_NULLFREE(pCtx,  pCurrentPath);
	SG_PATHNAME_NULLFREE(pCtx,  pPathCwd);
	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
	SG_CONTEXT_NULLFREE(pCtx);
	//MessageBox(NULL,(const TCHAR *)_T(__FUNCTION__),NULL,MB_OK|MB_ICONERROR);
	return MAKE_HRESULT ( SEVERITY_SUCCESS, FACILITY_NULL, uID - uidFirstCmd);
fail:
	NotifyError(pCtx);
	SG_PATHNAME_NULLFREE(pCtx,  pPathCwd);
	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
	SG_PATHNAME_NULLFREE(pCtx,  pCurrentPath);
	SG_CONTEXT_NULLFREE(pCtx);
	//MessageBox(NULL,(const TCHAR *)_T(__FUNCTION__),NULL,MB_OK|MB_ICONERROR);
	return MAKE_HRESULT ( SEVERITY_SUCCESS, FACILITY_NULL, uID - uidFirstCmd );
}

void CVeracityContextMenu::NotifyError(SG_context * pCtx)
{
	SG_UNUSED_PARAM(pCtx);

	
#if  DEBUG
	wchar_t * pBuf = NULL;
	SG_string * errorMessage = NULL;
	SG_context * pSubContext = NULL;
	SG_CONTEXT__ALLOC(&pSubContext);
	SG_context__err_to_string(pCtx, SG_TRUE, &errorMessage);
	SG_utf8__extern_to_os_buffer__wchar(pSubContext, SG_string__sz(errorMessage), &pBuf, NULL);
	MessageBox(NULL,(const TCHAR *)pBuf,NULL,MB_OK|MB_ICONERROR);
	SG_STRING_NULLFREE(pSubContext, errorMessage);
	SG_CONTEXT_NULLFREE(pSubContext);
	free(pBuf);
#else
	SG_log__report_error__current_error(pCtx);
#endif
	if (SG_CONTEXT__HAS_ERR(pCtx))
		SG_context__err_reset(pCtx);
}

STDMETHODIMP CVeracityContextMenu::GetCommandString (
    UINT_PTR idCmd, UINT uFlags, UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
USES_CONVERSION;
pwReserved;
    // If Explorer is asking for a help string, copy our string into the
    // supplied buffer.
	SG_context * pCtx = NULL;
	SG_CONTEXT__ALLOC(&pCtx);
	if ( uFlags & GCS_HELPTEXT )
    {
		LPCTSTR szText = _T("help string");
		ICommandBase * CurrentCommand = NULL;
		SG_bool bFound = SG_FALSE;
		SG_int_to_string_buffer buf;
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, mp_contextMenuItemsByOurID, SG_int64_to_sz(idCmd, buf), &bFound, (void**)&CurrentCommand)  );
		if (!bFound)
		{
			SG_CONTEXT_NULLFREE(pCtx);
			return E_INVALIDARG;
		}
		szText = CurrentCommand->GetHelpString();
		
        if ( uFlags & GCS_UNICODE )
        {
            // We need to cast pszName to a Unicode string, and then use the
            // Unicode string copy API.
            lstrcpynW ( (LPWSTR) pszName, T2CW(szText), cchMax );
        }
        else
        {
            // Use the ANSI string copy API to return the help string.
            lstrcpynA ( pszName, T2CA(szText), cchMax );
        }

		SG_CONTEXT_NULLFREE(pCtx);
        return S_OK;
    }
	SG_CONTEXT_NULLFREE(pCtx);
	return E_INVALIDARG;
fail:
	NotifyError(pCtx);
	SG_CONTEXT_NULLFREE(pCtx);
    return E_INVALIDARG;
}

STDMETHODIMP CVeracityContextMenu::InvokeCommand ( LPCMINVOKECOMMANDINFO pCmdInfo )
{
    // If lpVerb really points to a string, ignore this function call and bail out.
    if ( 0 != HIWORD( pCmdInfo->lpVerb ) )
        return E_INVALIDARG;
	SG_context * pCtx = NULL;
	SG_CONTEXT__ALLOC(&pCtx);
	UINT_PTR idCmd = LOWORD( pCmdInfo->lpVerb);
	SG_int_to_string_buffer buf;

	ICommandBase * CurrentCommand = NULL;
	SG_bool bFound = SG_FALSE;
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, mp_contextMenuItemsByOurID, SG_int64_to_sz(idCmd, buf), &bFound, (void**)&CurrentCommand)  );
	if (!bFound)
	{
		SG_CONTEXT_NULLFREE(pCtx);
		return E_INVALIDARG;
	}
	SG_ERR_CHECK(  CurrentCommand->RunCommand(pCtx, mp_selectedPaths)  );
	SG_CONTEXT_NULLFREE(pCtx);
	return S_OK;
fail:
	NotifyError(pCtx);
	SG_CONTEXT_NULLFREE(pCtx);
	return S_OK;
}
STDMETHODIMP CVeracityContextMenu::HandleMenuMsg(
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	LRESULT lResult;
	//MessageBox(NULL,(const TCHAR *)_T(__FUNCTION__),NULL,MB_OK|MB_ICONERROR);
	//Forward the message to the other handler.
	return HandleMenuMsg2(uMsg, wParam, lParam, &lResult);
}

ICommandBase * CVeracityContextMenu::GetCommandByID(int menuItemID)
{
	SG_context * pCtx = NULL;
	SG_bool bFound = SG_FALSE;
	ICommandBase * pCurrentCommand = NULL;
	SG_int_to_string_buffer buf;

	SG_CONTEXT__ALLOC__TEMPORARY(&pCtx);
	SG_int64_to_sz(menuItemID, buf);
	
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, mp_contextMenuItemsByOurID, buf, &bFound, (void**)&pCurrentCommand)  );
	if (bFound == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, mp_contextMenuItemsByMenuID, buf, &bFound, (void**)&pCurrentCommand)  );
	}

fail:
	SG_CONTEXT_NULLFREE(pCtx);
	if (bFound)
		return pCurrentCommand;
	else
		return NULL;
}

STDMETHODIMP CVeracityContextMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
	//MessageBox(NULL,(const TCHAR *)_T(__FUNCTION__),NULL,MB_OK|MB_ICONERROR);
	wParam;
	LRESULT result;
	if (plResult == NULL)
	{
		//For some reason, XP will send a NULL result pointer, but only when you
		//right-click on the Folder pane.
		plResult = &result;
	}
	else
		*plResult = FALSE;
	switch ( uMsg )
    {
    case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* pmis = (MEASUREITEMSTRUCT*) lParam;
			if (pmis)
			{
				pmis->itemWidth  = 16 + 2;
				pmis->itemHeight = 16;
				*plResult = TRUE;  // we handled the message
			}
		}
    break;
 
    case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT * pDrawItem = (DRAWITEMSTRUCT *)lParam;
			if (pDrawItem != NULL )
			{
				ICommandBase * CurrentCommand = GetCommandByID(pDrawItem->itemID);
				if (CurrentCommand != NULL)
				{
					HICON hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(CurrentCommand->GetIconIdentifier()), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
					if (hIcon)
					{
						DrawIconEx(pDrawItem->hDC, pDrawItem->rcItem.left, pDrawItem->rcItem.top, hIcon, 16, 16, SG_FALSE, NULL, DI_NORMAL); 
						DestroyIcon(hIcon);
						*plResult = TRUE;  // we handled the message
					}
				}
			}
		}
    break;
    }
 
  return S_OK;
}
