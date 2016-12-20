/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once

// We want commctrl v6, and it's available in XP.
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

#include <shellapi.h>
#include <commctrl.h>

#include "resource.h"
#include "sg.h"

#include "WorkerThread.h"
#include "Config.h"

/* Globals that are also used by WorkerThread */
//static char * p_gUser = NULL;
//static char * p_gPass = NULL;


#define MAX_LOADSTRING 512
#define MAX_OPTION_LEN 512


/* This isn't arbitrary. It's the length of NOTIFYICONDATA's szTip.
   We SG_STATIC_ASSERT that they match. */
#define MAX_TOOLTIP 128

UINT const WM_APP_NOTIFYCALLBACK= WM_APP + 1;
UINT const WM_APP_READY			= WM_APP + 2;
UINT const WM_APP_BUSY			= WM_APP + 3;
UINT const WM_APP_OFFLINE		= WM_APP + 4;
UINT const WM_APP_SHOW_BALLOON	= WM_APP + 5;
UINT const WM_APP_CONFIG_ERR	= WM_APP + 6;
UINT const WM_APP_WORKER_DONE	= WM_APP + 7;
UINT const WM_APP_NEED_LOGIN	= WM_APP + 8;

HRESULT SetToolTip(LPWSTR pszFormat, ...);