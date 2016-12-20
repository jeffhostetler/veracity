// VeracityOverlay_Ignored.h : Declaration of the CVeracityOverlay_Ignored

#pragma once
#include "../stdafx.h"
#include "../resource.h"       // main symbols
#include <comsvcs.h>
#include "VeracityTortoise_i.h"
#include "VeracityOverlay_Base.h"

using namespace ATL;



// CVeracityOverlay_Ignored

class ATL_NO_VTABLE CVeracityOverlay_Ignored :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityOverlay_Ignored, &CLSID_VeracityOverlay_Ignored>,
	public IDispatchImpl<IVeracityOverlay_Ignored, &IID_IVeracityOverlay_Ignored, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellIconOverlayIdentifier,
	public VeracityOverlay_Base
{
public:
	CVeracityOverlay_Ignored()
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

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYOVERLAY_IGNORED)

DECLARE_NOT_AGGREGATABLE(CVeracityOverlay_Ignored)

BEGIN_COM_MAP(CVeracityOverlay_Ignored)
	COM_INTERFACE_ENTRY(IVeracityOverlay_Ignored)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
END_COM_MAP()

	//IShellIconOverlayIdentifier
	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP GetPriority(int *pPriority);
    STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);




// IVeracityOverlay_Ignored
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityOverlay_Ignored), CVeracityOverlay_Ignored)
