// VeracityOverlay_Unversioned.h : Declaration of the CVeracityOverlay_Unversioned

#pragma once
#include "../stdafx.h"
#include "../resource.h"       // main symbols
#include <comsvcs.h>
#include "VeracityTortoise_i.h"
#include "VeracityOverlay_Base.h"
using namespace ATL;



// CVeracityOverlay_Unversioned

class ATL_NO_VTABLE CVeracityOverlay_Unversioned :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityOverlay_Unversioned, &CLSID_VeracityOverlay_Unversioned>,
	public IDispatchImpl<IVeracityOverlay_Unversioned, &IID_IVeracityOverlay_Unversioned, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellIconOverlayIdentifier,
	public VeracityOverlay_Base
{
public:
	CVeracityOverlay_Unversioned()
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

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYOVERLAY_UNVERSIONED)

DECLARE_NOT_AGGREGATABLE(CVeracityOverlay_Unversioned)

BEGIN_COM_MAP(CVeracityOverlay_Unversioned)
	COM_INTERFACE_ENTRY(IVeracityOverlay_Unversioned)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
END_COM_MAP()

	//IShellIconOverlayIdentifier
	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP GetPriority(int *pPriority);
    STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);



// IVeracityOverlay_Unversioned
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityOverlay_Unversioned), CVeracityOverlay_Unversioned)
