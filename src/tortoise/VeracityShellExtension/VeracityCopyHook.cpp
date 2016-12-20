#include "VeracityCopyHook.h"
#include "GlobalHelpers.h"
#if USE_FILESYSTEM_WATCHERS
UINT __stdcall CVeracityCopyHook::CopyCallback(HWND hwnd, UINT wFunc, UINT wFlags, 
		  LPCWSTR pwszSrcFile, DWORD dwSrcAttribs, 
		  LPCWSTR pwszDestFile, DWORD dwDestAttribs)
{
	SG_string * pstr_selectedFile = NULL;
	hwnd;
	wFunc;
	wFlags;
	dwSrcAttribs;
	dwDestAttribs;
	pwszDestFile;
	SG_context* pCtx = NULL;
	SG_pathname * pPathname_top = NULL;
	SG_bool bHasFinalSlash = SG_FALSE;
	if (wFunc == FO_COPY) // Our cache process's lock will not interfere with copy operations.
		return IDYES;

	SG_CONTEXT__ALLOC__TEMPORARY(&pCtx);

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_selectedFile)  );

	SG_ERR_CHECK(  SG_UTF8__INTERN_FROM_OS_BUFFER(pCtx, pstr_selectedFile, pwszSrcFile)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathname_top, SG_string__sz(pstr_selectedFile))  );
	
	SG_ERR_CHECK(  SG_pathname__has_final_slash(pCtx, pPathname_top, &bHasFinalSlash, NULL)  );
	if (bHasFinalSlash == SG_FALSE)
		SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPathname_top)  );
	GlobalHelpers::CloseCache(pCtx, SG_pathname__sz(pPathname_top));
fail:
	SG_STRING_NULLFREE(pCtx, pstr_selectedFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_top);
	SG_CONTEXT_NULLFREE(pCtx);
	return IDYES;
}
#endif