// VeracityOverlay_Conflict.h : Declaration of the CVeracityOverlay_Conflict

#pragma once
#include "../stdafx.h"
#include "../resource.h"       // main symbols
#include <comsvcs.h>
#include "VeracityTortoise_i.h"
#include "VeracityOverlay_Base.h"

using namespace ATL;



// CVeracityOverlay_Conflict

class ATL_NO_VTABLE CVeracityOverlay_Conflict :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVeracityOverlay_Conflict, &CLSID_VeracityOverlay_Conflict>,
	public IDispatchImpl<IVeracityOverlay_Conflict, &IID_IVeracityOverlay_Conflict, &LIBID_VeracityTortoiseLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellIconOverlayIdentifier,
	public VeracityOverlay_Base
{
public:
	CVeracityOverlay_Conflict()
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

DECLARE_REGISTRY_RESOURCEID(IDR_VERACITYOVERLAY_CONFLICT)

DECLARE_NOT_AGGREGATABLE(CVeracityOverlay_Conflict)

BEGIN_COM_MAP(CVeracityOverlay_Conflict)
	COM_INTERFACE_ENTRY(IVeracityOverlay_Conflict)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
END_COM_MAP()

	//IShellIconOverlayIdentifier
	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP GetPriority(int *pPriority);
    STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);




// IVeracityOverlay_Conflict
public:
};

OBJECT_ENTRY_AUTO(__uuidof(VeracityOverlay_Conflict), CVeracityOverlay_Conflict)
