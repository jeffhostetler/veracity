// VeracityOverlay_Added.h : Declaration of the CVeracityOverlay_Added

#pragma once
#include "../stdafx.h"
#include "../resource.h"       // main symbols
#include <comsvcs.h>
#include "VeracityTortoise_i.h"
#include "VeracityOverlay_Base.h"
using namespace ATL;



// CVeracityOverlay_Added

class ATL_NO_VTABLE CVeracityOverlay_Added :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityOverlay_Added, &CLSID_VeracityOverlay_Added>,
	public IDispatchImpl<IVeracityOverlay_Added, &IID_IVeracityOverlay_Added, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellIconOverlayIdentifier,
	public VeracityOverlay_Base
{
public:
	CVeracityOverlay_Added()
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

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYOVERLAY_ADDED)

DECLARE_NOT_AGGREGATABLE(CVeracityOverlay_Added)

BEGIN_COM_MAP(CVeracityOverlay_Added)
	COM_INTERFACE_ENTRY(IVeracityOverlay_Added)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
END_COM_MAP()

	//IShellIconOverlayIdentifier
	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP GetPriority(int *pPriority);
    STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);


// IVeracityOverlay_Added
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityOverlay_Added), CVeracityOverlay_Added)
