#pragma once

#include "sg.h"
#include "sg_workingdir_prototypes.h"

class GlobalHelpers
{
public:
	GlobalHelpers(void);
	~GlobalHelpers(void);
	static SG_bool CheckDriveType(SG_context* pCtx, const wchar_t * pszPath);
	static void findSiblingFile(SG_context * pCtx, const char * psz_fileName, const char * psz_devDirectoryName, SG_pathname ** ppPathname);
	static SG_wc_status_flags GetStatus(SG_context* pCtx, const char * psz_Path, const char * psz_PathTop, SG_bool * pbDirChanged);
	static void CloseCache(SG_context* pCtx, const char * pszPathTop);
	//static int GlobalHelpers::GetHeadCount(SG_context* pCtx, const char * psz_Path);
};

