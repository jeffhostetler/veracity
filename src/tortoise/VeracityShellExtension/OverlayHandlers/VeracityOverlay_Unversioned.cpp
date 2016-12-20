// VeracityOverlay_Unversioned.cpp : Implementation of CVeracityOverlay_Found

#include "../stdafx.h"
#include "VeracityOverlay_Unversioned.h"


// CVeracityOverlay_Found


#include "sg.h"
#include "sg_workingdir_prototypes.h"
// CVeracityOverlay_Unversioned

STDMETHODIMP CVeracityOverlay_Unversioned::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	pwszIconFile;
	cchMax;
	pIndex;
	pdwFlags;
	return S_OK;
}
//It turns out that TortoiseOverlays never calls GetPriority.  This represents the order
//that I wish things would be used.
STDMETHODIMP CVeracityOverlay_Unversioned::GetPriority(int *pPriority)
{
	if (pPriority != NULL)
		*pPriority = 6;
	return S_OK;
}
STDMETHODIMP CVeracityOverlay_Unversioned::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	pwszPath;
	dwAttrib;
	HRESULT returnVal = S_FALSE;
	SG_context* pCtx = GetAContext();
	
	SG_pathname * pPathCwd = NULL;
	SG_pathname * pPathTop = NULL;
	if (InAWorkingFolder(pCtx, pwszPath, &pPathCwd, &pPathTop) )
	{
		SG_wc_status_flags status = GetStatus(pCtx, SG_pathname__sz(pPathCwd), SG_pathname__sz(pPathTop), NULL);
		if (!StatusHas(status, SG_WC_STATUS_FLAGS__R__RESERVED) && StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND) )
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
