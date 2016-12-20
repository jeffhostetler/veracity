// VeracityOverlay_Ignored.cpp : Implementation of CVeracityOverlay_Ignored

#include "../stdafx.h"
#include "VeracityOverlay_Ignored.h"


// CVeracityOverlay_Ignored

#include "sg.h"
#include "sg_workingdir_prototypes.h"

// CVeracityOverlay_Ignored

STDMETHODIMP CVeracityOverlay_Ignored::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	pwszIconFile;
	cchMax;
	pIndex;
	pdwFlags;
	return S_OK;
}
//It turns out that TortoiseOverlays never calls GetPriority.  This represents the order
//that I wish things would be used.
STDMETHODIMP CVeracityOverlay_Ignored::GetPriority(int *pPriority)
{
	if (pPriority != NULL)
		*pPriority = 5;
	return S_OK;
}
STDMETHODIMP CVeracityOverlay_Ignored::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
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
		SG_wc_status_flags status =  GetStatus(pCtx, SG_pathname__sz(pPathCwd), SG_pathname__sz(pPathTop), NULL);

		if (StatusHas(status, SG_WC_STATUS_FLAGS__U__IGNORED) || StatusHas(status, SG_WC_STATUS_FLAGS__R__RESERVED) )
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

