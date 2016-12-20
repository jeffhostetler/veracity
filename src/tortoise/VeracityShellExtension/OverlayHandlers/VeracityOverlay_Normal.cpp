// VeracityOverlay_Normal.cpp : Implementation of CVeracityOverlay_Normal

#include "../stdafx.h"
#include "VeracityOverlay_Normal.h"


// CVeracityOverlay_Normal


#include "sg.h"
#include "sg_workingdir_prototypes.h"
// CVeracityOverlay_Normal

STDMETHODIMP CVeracityOverlay_Normal::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	pwszIconFile;
	cchMax;
	pIndex;
	pdwFlags;
	return S_OK;
}
//It turns out that TortoiseOverlays never calls GetPriority.  This represents the order
//that I wish things would be used.
STDMETHODIMP CVeracityOverlay_Normal::GetPriority(int *pPriority)
{
	if (pPriority != NULL)
		*pPriority = 4;
	return S_OK;
}
STDMETHODIMP CVeracityOverlay_Normal::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
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
		SG_bool bDirChanged = SG_FALSE;
		SG_wc_status_flags status =  GetStatus(pCtx, SG_pathname__sz(pPathCwd), SG_pathname__sz(pPathTop), &bDirChanged);
		if (!StatusHas(status, SG_WC_STATUS_FLAGS__R__RESERVED) 
			&&  (status == SG_WC_STATUS_FLAGS__T__FILE || status == SG_WC_STATUS_FLAGS__T__DIRECTORY) 
			&& bDirChanged == SG_FALSE
			&& !StatusHas(status, SG_WC_STATUS_FLAGS__U__IGNORED) 
			&& !StatusHas(status, SG_WC_STATUS_FLAGS__L__LOCKED_BY_USER) && !StatusHas(status, SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER)/*not locked at all */)
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
