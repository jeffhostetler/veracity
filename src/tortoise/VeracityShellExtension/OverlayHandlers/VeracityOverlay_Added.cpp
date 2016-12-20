// VeracityOverlay_Added.cpp : Implementation of CVeracityOverlay_Added

#include "../stdafx.h"
#include "VeracityOverlay_Added.h"


// CVeracityOverlay_Added


#include "sg.h"
// CVeracityOverlay_Added

STDMETHODIMP CVeracityOverlay_Added::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	pwszIconFile;
	cchMax;
	pIndex;
	pdwFlags;
	return S_OK;
}
//It turns out that TortoiseOverlays never calls GetPriority.  This represents the order
//that I wish things would be used.
STDMETHODIMP CVeracityOverlay_Added::GetPriority(int *pPriority)
{
	if (pPriority != NULL)
		*pPriority = 3;
	return S_OK;
}
STDMETHODIMP CVeracityOverlay_Added::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	pwszPath;
	dwAttrib;
	HRESULT returnVal = S_FALSE;
	//return returnVal;
	SG_context* pCtx = GetAContext();
	
	SG_pathname * pPathCwd = NULL;
	SG_pathname * pPathTop = NULL;
	if (InAWorkingFolder(pCtx, pwszPath, &pPathCwd, &pPathTop) )
	{
		SG_wc_status_flags status = GetStatus(pCtx, SG_pathname__sz(pPathCwd), SG_pathname__sz(pPathTop), NULL);
		if (!StatusHas(status, SG_WC_STATUS_FLAGS__R__RESERVED) && (StatusHas(status, SG_WC_STATUS_FLAGS__S__ADDED) || StatusHas(status, SG_WC_STATUS_FLAGS__S__MERGE_CREATED) || StatusHas(status, SG_WC_STATUS_FLAGS__XR__EXISTENCE)))
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
