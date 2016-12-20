#pragma once
#include "stdafx.h"
#include "sg.h"

static CComAutoCriticalSection m_critSec;
class VeracityCacheMessaging
{
public:
	VeracityCacheMessaging(void);
	~VeracityCacheMessaging(void);
	static bool ExecuteCommand(SG_context * pCtx, SG_vhash * pvhMessage, SG_vhash ** pvhResult);
};

