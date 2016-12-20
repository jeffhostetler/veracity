#include "VeracityOverlay_Base.h"
#include "../VeracityCacheMessaging.h"
#include "../GlobalHelpers.h"

#include "sg.h"

VeracityOverlay_Base::VeracityOverlay_Base(void)
{
}


VeracityOverlay_Base::~VeracityOverlay_Base(void)
{
}

bool VeracityOverlay_Base::StatusHas(SG_wc_status_flags testMe, SG_wc_status_flags lookForMe)
{
	return (testMe & lookForMe) == lookForMe;
}

SG_wc_status_flags VeracityOverlay_Base::GetStatus(SG_context* pCtx, const char * psz_Path, const char * psz_PathTop, SG_bool * pbDirChanged)
{	
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	SG_ERR_CHECK(  statusFlags = GlobalHelpers::GetStatus(pCtx, psz_Path, psz_PathTop, pbDirChanged)  );
fail:
	return statusFlags;
}

bool VeracityOverlay_Base::InAWorkingFolder(SG_context* pCtx, LPCWSTR pwszPath, SG_pathname ** ppPathName, SG_pathname ** ppPath_WC_root)
{
	SG_pathname * pPathCwd = NULL;
	SG_pathname * pPathTop = NULL;
	SG_string * pstr_selectedFile = NULL;
	SG_string * pstrRepoDescriptorName = NULL;
	//Jump out early if the path isn't on the right kind of drive.
	if (GlobalHelpers::CheckDriveType(pCtx, pwszPath) == SG_FALSE)
	{
		return false;
	}
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_selectedFile)  );
	
	SG_ERR_CHECK(  SG_UTF8__INTERN_FROM_OS_BUFFER(pCtx, pstr_selectedFile, pwszPath)  );
	
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathCwd, SG_string__sz(pstr_selectedFile))  );
	
	SG_workingdir__find_mapping(pCtx, pPathCwd, &pPathTop, &pstrRepoDescriptorName, NULL);
	
	if (SG_context__err_equals(pCtx, SG_ERR_NOT_A_WORKING_COPY) || SG_context__err_equals(pCtx, SG_ERR_NOTAREPOSITORY))
	{
		SG_context__err_reset(pCtx);
		goto cleanup;
	}
	else
	{
		SG_ERR_CHECK_CURRENT;
	}
	*ppPathName = pPathCwd;
	pPathCwd = NULL;
	//SG_RETURN_AND_NULL(pPathCwd, ppPathName);
	SG_RETURN_AND_NULL(pPathTop, ppPath_WC_root);
	SG_STRING_NULLFREE(pCtx, pstr_selectedFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathTop);
	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
	return true;;
fail:
	NotifyError(pCtx);
cleanup:
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_PATHNAME_NULLFREE(pCtx, pPathTop);
	SG_STRING_NULLFREE(pCtx, pstr_selectedFile);
	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
	return false;
}

SG_context * VeracityOverlay_Base::GetAContext()
{
	SG_context* pCtx = NULL;
	SG_error err;
	err = SG_context__alloc(&pCtx);
	return pCtx;
}

void VeracityOverlay_Base::NotifyError(SG_context * pCtx)
{
	SG_UNUSED_PARAM(pCtx);

	
#if  DEBUG
	wchar_t * pBuf = NULL;
	SG_string * errorMessage = NULL;
	SG_context * pSubContext = NULL;
	if (SG_context__err_equals(pCtx, SG_ERR_UNSUPPORTED_DRAWER_VERSION) == SG_TRUE)
	{
		//Don't bother notifying for this one.  It ends up spamming a lot when
		//switching between versions.
		return;
	}
	SG_CONTEXT__ALLOC(&pSubContext);
	SG_context__err_to_string(pCtx, SG_TRUE, &errorMessage);
	SG_utf8__extern_to_os_buffer__wchar(pSubContext, SG_string__sz(errorMessage), &pBuf, NULL);
	MessageBox(NULL,(const TCHAR *)pBuf,NULL,MB_OK|MB_ICONERROR);
	SG_STRING_NULLFREE(pSubContext, errorMessage);
	SG_CONTEXT_NULLFREE(pSubContext);
	free(pBuf);
#else
	SG_log__report_error__current_error(pCtx);
#endif
}
