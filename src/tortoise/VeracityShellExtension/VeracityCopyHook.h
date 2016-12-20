// VeracityCopyHook.h : Declaration of the CVeracityCopyHook

#pragma once
#if USE_FILESYSTEM_WATCHERS
#include "stdafx.h"
#include "resource.h"       // main symbols
#include <comsvcs.h>
#include <shlobj.h>
#include "VeracityTortoise_i.h"
using namespace ATL;



// CVeracityCopyHook

class ATL_NO_VTABLE CVeracityCopyHook :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityCopyHook, &CLSID_VeracityCopyHook>,
	public ICopyHook,
	public IDispatchImpl<IVeracityCopyHook, &IID_IVeracityCopyHook, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CVeracityCopyHook()
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

	UINT __stdcall CopyCallback(HWND hwnd, UINT wFunc, UINT wFlags, 
		  LPCWSTR pszSrcFile, DWORD dwSrcAttribs, 
		  LPCWSTR pszDestFile, DWORD dwDestAttribs);

	DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYCOPYHOOK)

DECLARE_NOT_AGGREGATABLE(CVeracityCopyHook)

BEGIN_COM_MAP(CVeracityCopyHook)
	COM_INTERFACE_ENTRY(IVeracityCopyHook)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IID(IID_IShellCopyHook , CVeracityCopyHook)
END_COM_MAP()


// IVeracityCopyHook
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityCopyHook), CVeracityCopyHook)
#endif