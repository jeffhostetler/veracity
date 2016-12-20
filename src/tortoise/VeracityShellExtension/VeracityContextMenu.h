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
// VeracityContextMenu.h : Declaration of the CVeracityContextMenu

#pragma once
#include "VeracityTortoise_i.h"
#include "resource.h"       // main symbols
#include <comsvcs.h>

#include "sg.h"

#include "ICommandBase.h"
#include "Commands.h"

using namespace ATL;



// CVeracityContextMenu

class ATL_NO_VTABLE CVeracityContextMenu :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityContextMenu, &CLSID_VeracityContextMenu>,
	public IDispatchImpl<IVeracityContextMenu, &IID_IVeracityContextMenu, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellExtInit,
    public IContextMenu3
{
public:
	CVeracityContextMenu()
	{
		//m_pCtx = NULL;
		mp_selectedPaths = NULL;
		mp_contextMenuItemsByOurID = NULL;
		mp_contextMenuItemsByMenuID = NULL;
		mp_vecBitmaps = NULL;
		mp_vecStrings = NULL;
		m_sub = NULL;
		m_hInst = NULL;
		bAlreadyConfigured = false;
		bIsVistaOrLater = false;
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
		SG_context * pCtx = NULL;
		SG_CONTEXT__ALLOC__TEMPORARY(&pCtx);
		if (mp_selectedPaths != NULL)
			SG_STRINGARRAY_NULLFREE(pCtx, mp_selectedPaths);
		if (mp_contextMenuItemsByMenuID != NULL)
			SG_RBTREE_NULLFREE(pCtx, mp_contextMenuItemsByMenuID);
		if (mp_contextMenuItemsByOurID != NULL)
			SG_RBTREE_NULLFREE(pCtx, mp_contextMenuItemsByOurID);
		if (mp_vecBitmaps != NULL)
		{
			SG_uint32 nSize = 0;
			SG_uint32 index = 0;
			SG_ERR_CHECK(  SG_vector__length(pCtx, mp_vecBitmaps, &nSize)  );
			for (index = 0; index < nSize; index++)
			{
				HBITMAP hbmp;
				SG_ERR_CHECK(  SG_vector__get(pCtx, mp_vecBitmaps, index, (void**)&hbmp)  );
				DeleteObject(hbmp); 
			}
		}
		SG_VECTOR_NULLFREE(pCtx, mp_vecBitmaps);
		if (mp_vecStrings != NULL)
		{
			SG_uint32 nSize = 0;
			SG_uint32 index = 0;
			SG_ERR_CHECK(  SG_vector__length(pCtx, mp_vecStrings, &nSize)  );
			for (index = 0; index < nSize; index++)
			{
				PTCHAR tch;
				SG_ERR_CHECK(  SG_vector__get(pCtx, mp_vecStrings, index, (void**)&tch)  );
				free(tch);
			}
		}
		SG_VECTOR_NULLFREE(pCtx, mp_vecStrings);
		DeleteObject(m_hInst);
fail:
		SG_CONTEXT_NULLFREE(pCtx);
	}

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYCONTEXTMENU)

DECLARE_NOT_AGGREGATABLE(CVeracityContextMenu)

BEGIN_COM_MAP(CVeracityContextMenu)
	COM_INTERFACE_ENTRY(IVeracityContextMenu)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IContextMenu)
    COM_INTERFACE_ENTRY(IContextMenu2)
	COM_INTERFACE_ENTRY(IContextMenu3)
	COM_INTERFACE_ENTRY(IShellExtInit)
END_COM_MAP()

public:
    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);

    // IContextMenu3
    STDMETHODIMP GetCommandString(UINT_PTR, UINT, UINT*, LPSTR, UINT);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO);
    STDMETHODIMP QueryContextMenu(HMENU, UINT, UINT, UINT, UINT);
	STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

protected:
	//SG_context * m_pCtx;
//The COMMAND_COUNT will need to change when we change the commands in ConfigureContextMenu
#define COMMAND_COUNT 21
	ICommandBase * m_commandsArray[COMMAND_COUNT];
	SubmenuCommand * m_sub;
	SG_stringarray * mp_selectedPaths;
	SG_vector * mp_vecBitmaps;
	SG_vector * mp_vecStrings;
	SG_rbtree * mp_contextMenuItemsByMenuID;
	SG_rbtree * mp_contextMenuItemsByOurID;
	HINSTANCE m_hInst;
	bool bAlreadyConfigured;
	bool bIsVistaOrLater;
	bool bHideMenuIfNoWorkingFolder;
	
	void NotifyError(SG_context * pCtx);
	void ConfigureContextMenu(SG_context* pCtx);
	ICommandBase * CVeracityContextMenu::GetCommandByID(int menuItemID);
// IVeracityContextMenu
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityContextMenu), CVeracityContextMenu)
