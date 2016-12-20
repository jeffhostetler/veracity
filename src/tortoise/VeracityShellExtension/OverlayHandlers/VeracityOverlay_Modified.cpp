// VeracityOverlay_Modified.cpp : Implementation of CVeracityOverlay_Modified

#include "../stdafx.h"
#include "VeracityOverlay_Modified.h"


// CVeracityOverlay_Modified

#include "sg.h"
#include "sg_workingdir_prototypes.h"
// CVeracityOverlay_Modified

STDMETHODIMP CVeracityOverlay_Modified::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	pwszIconFile;
	cchMax;
	pIndex;
	pdwFlags;
	return S_OK;
}
//It turns out that TortoiseOverlays never calls GetPriority.  This represents the order
//that I wish things would be used.
STDMETHODIMP CVeracityOverlay_Modified::GetPriority(int *pPriority)
{
	if (pPriority != NULL)
		*pPriority = 1;
	return S_OK;
}
STDMETHODIMP CVeracityOverlay_Modified::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	pwszPath;
	dwAttrib;
	HRESULT returnVal = S_FALSE;
	SG_context* pCtx = GetAContext();
	
	SG_pathname * pPathCwd = NULL;
	SG_pathname * pPathTop = NULL;
	if (InAWorkingFolder(pCtx, pwszPath, &pPathCwd, &pPathTop) )
	{
		SG_bool bDirChanged = SG_FALSE;
		SG_wc_status_flags status =  GetStatus(pCtx, SG_pathname__sz(pPathCwd), SG_pathname__sz(pPathTop), &bDirChanged);
		if ( !StatusHas(status, SG_WC_STATUS_FLAGS__R__RESERVED) && !StatusHas(status, SG_WC_STATUS_FLAGS__X__UNRESOLVED) 
			&& !StatusHas(status, SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER) 
			&& (bDirChanged || StatusHas(status, SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED) ||StatusHas(status, SG_WC_STATUS_FLAGS__S__MOVED) || StatusHas(status, SG_WC_STATUS_FLAGS__S__RENAMED) || StatusHas(status, SG_WC_STATUS_FLAGS__XR__NAME) || StatusHas(status, SG_WC_STATUS_FLAGS__XR__LOCATION))  )
			returnVal = S_OK;
		else
			returnVal = S_FALSE;
	}
	else
		returnVal = S_FALSE;
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_PATHNAME_NULLFREE(pCtx, pPathTop);
	SG_CONTEXT_NULLFREE(pCtx);
	return returnVal;
}

