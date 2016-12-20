#include "StdAfx.h"
#include "GlobalHelpers.h"
#include "VeracityCacheMessaging.h"


GlobalHelpers::GlobalHelpers(void)
{
}


GlobalHelpers::~GlobalHelpers(void)
{
}

SG_bool GlobalHelpers::CheckDriveType(SG_context* pCtx, const wchar_t * pszPath)
{
	pCtx;
	wchar_t driveLetter_buf[4];
	int nDriveNumber = PathGetDriveNumber(pszPath);
	if (nDriveNumber < 0)
		return SG_FALSE;
	else
	{
		PathBuildRoot(driveLetter_buf, nDriveNumber);
		unsigned int driveType = GetDriveType(driveLetter_buf);
		return driveType == DRIVE_FIXED;
	}
	
}

void GlobalHelpers::findSiblingFile(SG_context * pCtx, const char * psz_fileName, const char * psz_devDirectoryName, SG_pathname ** ppPathname)
{
	SG_pathname * pPathName = NULL;
	TCHAR szBuf [MAX_PATH + 32];
	GetModuleFileName( GetModuleHandle(_T("VeracityTortoise.dll") ), szBuf, MAX_PATH + 32 );
	SG_string * utf8String = NULL;
	SG_bool bExists = SG_FALSE;
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &utf8String)  );
	SG_ERR_CHECK(  SG_UTF8__INTERN_FROM_OS_BUFFER(pCtx, utf8String, szBuf)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathName, SG_string__sz(utf8String))  );
	SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPathName)  ); //Strip off the dll name.
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathName, psz_fileName)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathName, &bExists, NULL, NULL)  );
	if (bExists == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Couldn't find %s at: %s", psz_fileName, SG_pathname__sz(pPathName))  );
		//It's not right next to the dll.  Look for it in the usual build hierarchy.
		//Essentially ../../VeracityDialog/Debug
		SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPathName)  ); //Strip off the exe.
		SG_ERR_IGNORE(  SG_pathname__remove_last(pCtx, pPathName)  ); //Strip off the Debug directory.
		SG_ERR_IGNORE(  SG_pathname__remove_last(pCtx, pPathName)  ); //Strip off the VeracityShellExtension directory.
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathName, psz_devDirectoryName)  );
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathName, "Debug")  );
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathName, psz_fileName)  );
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathName, &bExists, NULL, NULL)  );
		if (bExists == SG_FALSE)
		{
			SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Couldn't find %s at: %s", psz_fileName, SG_pathname__sz(pPathName))  );
			//Still doesn't exist, look for the release directory.
			SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPathName)  ); //Strip off the exe.
			SG_ERR_IGNORE(  SG_pathname__remove_last(pCtx, pPathName)  ); //Strip off the Debug directory.
			SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathName, "Release")  );
			SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathName, psz_fileName)  );
			if (bExists == SG_FALSE)
			{
				SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Couldn't find %s at: %s", psz_fileName, SG_pathname__sz(pPathName))  );
				SG_ERR_CHECK(  SG_log__report_error(pCtx, "Couldn't find: %s", psz_fileName)  );
			}
		}
	}
	SG_STRING_NULLFREE(pCtx, utf8String);
	SG_RETURN_AND_NULL(pPathName, ppPathname);
	return;
fail:
	SG_STRING_NULLFREE(pCtx, utf8String);
	SG_PATHNAME_NULLFREE(pCtx, pPathName);
}

SG_wc_status_flags GlobalHelpers::GetStatus(SG_context* pCtx, const char * pszPath, const char * pszPathTop, SG_bool * pbDirChanged)
{	
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	
	SG_vhash * pvhMessage = NULL;
	SG_vhash * pvhResult = NULL;
	SG_fsobj_stat stat;
	SG_pathname * p_PathName = NULL;
	SG_bool bSucceeded = SG_FALSE;

	if (strstr(pszPath, "/.sgdrawer") != NULL 
		|| strstr(pszPath, "\\.sgdrawer") != NULL)
	{
		return SG_WC_STATUS_FLAGS__R__RESERVED;
	}

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &p_PathName, pszPath)  );
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx,  p_PathName, &stat)  );
	if (stat.type == SG_FSOBJ_TYPE__DIRECTORY)
	{
		SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, p_PathName)  );
	}

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhMessage)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "command", "status")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "path", SG_pathname__sz(p_PathName))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "root", pszPathTop)  );
	if (pbDirChanged == NULL)
		SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhMessage, "dir_status", SG_FALSE)  );
	SG_ERR_CHECK(  bSucceeded = VeracityCacheMessaging::ExecuteCommand(pCtx, pvhMessage, &pvhResult)  );
	if (bSucceeded == SG_TRUE)
	{
		SG_int64 statusResult = 0;
	
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhResult, "status", &statusResult)  );
		if (pbDirChanged != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvhResult, "dir_changed", pbDirChanged)  );
		}
		
		statusFlags = (SG_wc_status_flags)statusResult;
	}
	else
	{
		//This should mark everything as ?
		statusFlags = SG_WC_STATUS_FLAGS__ZERO;
		if (pbDirChanged != NULL)
			*pbDirChanged = SG_FALSE;
	}
fail:
	SG_PATHNAME_NULLFREE(pCtx, p_PathName);
	SG_VHASH_NULLFREE(pCtx, pvhMessage);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	return statusFlags;
	
}

void GlobalHelpers::CloseCache(SG_context* pCtx, const char * pszPathTop)
{	

	SG_vhash * pvhMessage = NULL;
	SG_vhash * pvhResult = NULL;
	SG_bool bSucceeded = SG_FALSE;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhMessage)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "command", "closeListeners")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "root", pszPathTop)  );
	SG_ERR_CHECK(  bSucceeded = VeracityCacheMessaging::ExecuteCommand(pCtx, pvhMessage, &pvhResult)  );
	
fail:
	SG_VHASH_NULLFREE(pCtx, pvhMessage);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
}

/*
bool GlobalHelpers::IsConflict(SG_context* pCtx, const char * psz_Path, const char * psz_PathTop)
{
	pCtx;
	SG_bool returnVal = false;
	SG_bool bSucceeded = SG_FALSE;
	SG_vhash * pvhMessage = NULL;
	SG_vhash * pvhResult = NULL;
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhMessage)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "command", "conflict")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "path", psz_Path)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "root", psz_PathTop)  );
	SG_ERR_CHECK(  bSucceeded = VeracityCacheMessaging::ExecuteCommand(pCtx, pvhMessage, &pvhResult)  );
	
	if (bSucceeded == SG_TRUE)
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvhResult, "isConflict", &returnVal)  );
	else
		returnVal = SG_FALSE;
fail:
	SG_VHASH_NULLFREE(pCtx, pvhMessage);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	if (returnVal == SG_TRUE)
		return true;
	else
		return false;
	
}
*/
/*
int GlobalHelpers::GetHeadCount(SG_context* pCtx, const char * psz_Path)
{
	pCtx;
	SG_int64 headCount = 0;
	SG_bool bSucceeded = SG_FALSE;
	SG_vhash * pvhMessage = NULL;
	SG_vhash * pvhResult = NULL;
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhMessage)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "command", "getHeadCount")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhMessage, "path", psz_Path)  );
	SG_ERR_CHECK(  bSucceeded = VeracityCacheMessaging::ExecuteCommand(pCtx, pvhMessage, &pvhResult)  );
	if (bSucceeded == SG_TRUE)
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhResult, "headCount", &headCount)  );
	else
		headCount = 0;
fail:
	SG_VHASH_NULLFREE(pCtx, pvhMessage);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	return (int)headCount;
}
*/
