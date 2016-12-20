// VeracityOverlay_Modified.h : Declaration of the CVeracityOverlay_Modified

#pragma once
#include "../stdafx.h"
#include "../resource.h"       // main symbols
#include <comsvcs.h>
#include "VeracityTortoise_i.h"
#include "VeracityOverlay_Base.h"

using namespace ATL;



// CVeracityOverlay_Modified

class ATL_NO_VTABLE CVeracityOverlay_Modified :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityOverlay_Modified, &CLSID_VeracityOverlay_Modified>,
	public IDispatchImpl<IVeracityOverlay_Modified, &IID_IVeracityOverlay_Modified, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellIconOverlayIdentifier,
	public VeracityOverlay_Base
{
public:
	CVeracityOverlay_Modified()
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

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYOVERLAY_MODIFIED)

DECLARE_NOT_AGGREGATABLE(CVeracityOverlay_Modified)

BEGIN_COM_MAP(CVeracityOverlay_Modified)
	COM_INTERFACE_ENTRY(IVeracityOverlay_Modified)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
END_COM_MAP()

	//IShellIconOverlayIdentifier
	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP GetPriority(int *pPriority);
    STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);



// IVeracityOverlay_Modified
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityOverlay_Modified), CVeracityOverlay_Modified)
