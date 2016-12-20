#pragma once

#include "sg.h"
#include "sg_workingdir_prototypes.h"

static wxCriticalSection m_critSec;
class vvCacheMessaging
{
public:
	vvCacheMessaging(void);
	~vvCacheMessaging(void);
	static void RequestRefreshWC(SG_context * pCtx, const char * psz_WFRoot, SG_stringarray* psa_Specific_Paths);
private:
	static bool ExecuteCommand(SG_context * pCtx, SG_vhash * pvhMessage, SG_vhash ** pvhResult);
};

