#pragma once
#include "../stdafx.h"
#include "sg.h"


class VeracityOverlay_Base
{
private:
	
public:
	VeracityOverlay_Base(void);
	~VeracityOverlay_Base(void);
	SG_wc_status_flags GetStatus(SG_context* pCtx, const char * pPathName, const char * pPathTop, SG_bool * pbDirChanged);
	bool StatusHas(SG_wc_status_flags testMe, SG_wc_status_flags lookForMe);
	bool InAWorkingFolder(SG_context* pCtx, LPCWSTR pwszPath, SG_pathname ** ppPathName, SG_pathname ** ppPath_WC_root);
	//bool IsVeracityPrivateDir(SG_context* pCtx, SG_pathname * pPathName);
	//bool IsIgnored(SG_context* pCtx, SG_pathname * ppPathName);
	void NotifyError(SG_context * pCtx);
	SG_context * GetAContext();
};

