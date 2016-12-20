// VeracityOverlay_Normal.h : Declaration of the CVeracityOverlay_Normal

#pragma once
#include "../stdafx.h"
#include "../resource.h"       // main symbols
#include <comsvcs.h>
#include "VeracityTortoise_i.h"
#include "VeracityOverlay_Base.h"
using namespace ATL;



// CVeracityOverlay_Normal

class ATL_NO_VTABLE CVeracityOverlay_Normal :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityOverlay_Normal, &CLSID_VeracityOverlay_Normal>,
	public IDispatchImpl<IVeracityOverlay_Normal, &IID_IVeracityOverlay_Normal, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellIconOverlayIdentifier,
	public VeracityOverlay_Base
{
public:
	CVeracityOverlay_Normal()
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

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYOVERLAY_NORMAL)

DECLARE_NOT_AGGREGATABLE(CVeracityOverlay_Normal)

BEGIN_COM_MAP(CVeracityOverlay_Normal)
	COM_INTERFACE_ENTRY(IVeracityOverlay_Normal)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
END_COM_MAP()

	//IShellIconOverlayIdentifier
	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP GetPriority(int *pPriority);
    STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);



// IVeracityOverlay_Normal
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityOverlay_Normal), CVeracityOverlay_Normal)
