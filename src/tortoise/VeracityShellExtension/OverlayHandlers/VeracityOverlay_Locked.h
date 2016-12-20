// VeracityOverlay_Locked.h : Declaration of the CVeracityOverlay_Locked

#pragma once
#include "../stdafx.h"
#include "../resource.h"       // main symbols
#include <comsvcs.h>
#include "VeracityTortoise_i.h"
#include "VeracityOverlay_Base.h"

using namespace ATL;



// CVeracityOverlay_Locked

class ATL_NO_VTABLE CVeracityOverlay_Locked :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityOverlay_Locked, &CLSID_VeracityOverlay_Locked>,
	public IDispatchImpl<IVeracityOverlay_Locked, &IID_IVeracityOverlay_Locked, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellIconOverlayIdentifier,
	public VeracityOverlay_Base
{
public:
	CVeracityOverlay_Locked()
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

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYOVERLAY_LOCKED)

DECLARE_NOT_AGGREGATABLE(CVeracityOverlay_Locked)

BEGIN_COM_MAP(CVeracityOverlay_Locked)
	COM_INTERFACE_ENTRY(IVeracityOverlay_Locked)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
END_COM_MAP()

	//IShellIconOverlayIdentifier
	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP GetPriority(int *pPriority);
    STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);



// IVeracityOverlay_Locked
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityOverlay_Locked), CVeracityOverlay_Locked)
