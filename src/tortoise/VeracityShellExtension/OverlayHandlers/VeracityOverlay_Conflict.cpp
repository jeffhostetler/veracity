// VeracityOverlay_Conflict.cpp : Implementation of CVeracityOverlay_Conflict

#include "../stdafx.h"
#include "VeracityOverlay_Conflict.h"


// CVeracityOverlay_Conflict

#include "sg.h"
#include "sg_workingdir_prototypes.h"

// CVeracityOverlay_Conflict

STDMETHODIMP CVeracityOverlay_Conflict::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	pwszIconFile;
	cchMax;
	pIndex;
	pdwFlags;
	return S_OK;
}
//It turns out that TortoiseOverlays never calls GetPriority.  This represents the order
//that I wish things would be used.
STDMETHODIMP CVeracityOverlay_Conflict::GetPriority(int *pPriority)
{
	if (pPriority != NULL)
		*pPriority = 0;
	return S_OK;
}
STDMETHODIMP CVeracityOverlay_Conflict::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	pwszPath;
	dwAttrib;
	HRESULT returnVal = S_FALSE;
	SG_context* pCtx = GetAContext();
	
	SG_pathname * pPathCwd = NULL;
	SG_pathname * pPathTop = NULL;
	if (InAWorkingFolder(pCtx, pwszPath, &pPathCwd, &pPathTop) )
	{
			SG_wc_status_flags flags = GetStatus(pCtx, SG_pathname__sz(pPathCwd), SG_pathname__sz(pPathTop), NULL);
			if (!StatusHas(flags, SG_WC_STATUS_FLAGS__R__RESERVED) && StatusHas(flags, SG_WC_STATUS_FLAGS__X__UNRESOLVED)  )
				returnVal = S_OK;
			else
			{
				if (!StatusHas(flags, SG_WC_STATUS_FLAGS__R__RESERVED) && StatusHas(flags, SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER) && StatusHas(flags, SG_WC_STATUS_FLAGS__L__PENDING_VIOLATION)) //It's locked by someone else, and we have it modified.
					returnVal = S_OK;
				else
					returnVal = S_FALSE;
			}
	}
	else
		returnVal = S_FALSE;
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_PATHNAME_NULLFREE(pCtx, pPathTop);
	SG_CONTEXT_NULLFREE(pCtx);
	return returnVal;
}

