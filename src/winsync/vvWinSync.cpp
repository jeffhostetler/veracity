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

#include "stdafx.h"
#include "vvWinSync.h"

#define WHATSMYNAME "vvWinSync"

#define WORKER_THREAD_WORKS_EVERY_N_MS 10000

/* We use the XP style of identifying our icon, which is hWnd + a uint ID. This is the uint ID. */
#define MY_ICON_ID 0

#define TIMER_WORKER		1
#define TIMER_ANIMATE		2

#define STATUS_READY			0
#define STATUS_BUSY				1
#define STATUS_OFFLINE			2
#define STATUS_MISCONFIGURED	3
#define STATUS_AUTH_NEEDED		4

// Global Variables:
HINSTANCE g_hInst = NULL; // current instance
HMENU g_hContextMenu = NULL;
SG_context* g_pCtx = NULL; // The main thread's sglib context.
HWND g_hConfigDlg = NULL;
HWND g_hLoginDlg = NULL;

SG_log_text__data                     g_cLogFileData;
SG_log_text__writer__daily_path__data g_cLogFileWriterData;

WCHAR g_szTitle[MAX_LOADSTRING];					// The title bar text
WCHAR g_szWindowClass[MAX_LOADSTRING];				// the main window class name

CHAR g_szProfile[MAX_LOADSTRING];					// The profile specified on the command line
WCHAR g_szTooltip[MAX_TOOLTIP];						// The current tooltip text.

char g_szLogFileName[MAX_LOADSTRING];

// Forward declarations
static void	InitInstance(SG_context*, HINSTANCE);
static void	CleanupInstance(SG_context*);
static void RegisterWindowClass(PCWSTR, PCWSTR, WNDPROC);
static LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK ConfigDlgProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK LoginDlgProc(HWND, UINT, WPARAM, LPARAM);
static BOOL ShowBalloon(HWND, LPWSTR, LPWSTR);
static BOOL AddNotificationIcon(HWND);
static BOOL	RemoveNotificationIcon(HWND hWnd);
static void	ShowContextMenu(HWND, POINT, int);
static void ShowConfigDlg(HWND, int, volatile WorkerThreadParms*);
static void ShowLoginDlg(HWND, volatile WorkerThreadParms*);
static BOOL SetToolTipFromGlobalBuffer(HWND);
static BOOL SetIconAndTooltip(HWND, int);
static void ErrorMsgBox(const LPWSTR, DWORD);
static void EnableDisableMenuItems(HMENU, int);
static void SyncNow(volatile WorkerThreadParms*);
static void StopWorkerIfRunning(HWND, volatile WorkerThreadParms*);
static void StartWorkerIfNotRunning (HWND, volatile WorkerThreadParms*);

int APIENTRY wWinMain(HINSTANCE hInstance,
					  HINSTANCE hPrevInstance,
					  LPWSTR    lpwCmdLine,
					  int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	SG_context* pCtx = NULL;
	SG_string* pstrArg = NULL;

	// sglib initialization
	SG_error err = SG_context__alloc(&pCtx);
	if (SG_IS_ERROR(err))
		return -2;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, g_szTitle, ARRAYSIZE(g_szTitle));
	LoadString(hInstance, IDS_MAINWIN_CLASS, g_szWindowClass, ARRAYSIZE(g_szWindowClass));
	LoadString(hInstance, IDS_TRAY_DEFAULT, g_szTooltip, ARRAYSIZE(g_szTooltip));

	int argc = 0;
	LPWSTR* aszArgList = NULL;
	if (lpwCmdLine && *lpwCmdLine)
		aszArgList = CommandLineToArgvW(lpwCmdLine, &argc);
	if (argc > 0)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrArg)  );
		SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrArg, aszArgList[0])  );
		/* We somewhat arbitrarily choose 100 bytes here to try to keep the full path under 260. */
		if (SG_string__length_in_bytes(pstrArg) > 100)
		{
			MessageBox(NULL, L"Veracity Sync startup failure:\n\nThe specified profile is too long.", 
				L"Veracity Sync Startup Failure", MB_OK|MB_ICONEXCLAMATION);
			goto fail;
		}
		SG_ERR_CHECK(  SG_strcpy(pCtx, g_szProfile, ARRAYSIZE(g_szProfile), SG_string__sz(pstrArg))  );
		SG_STRING_NULLFREE(pCtx, pstrArg);
	}
	else
		SG_ERR_CHECK(  SG_strcpy(pCtx, g_szProfile, ARRAYSIZE(g_szProfile), CONFIG_DEFAULT)  );

	RegisterWindowClass(g_szWindowClass, MAKEINTRESOURCE(IDC_TRAY_CONTEXT), WndProc);

	// Perform application initialization:
	SG_ERR_CHECK(  InitInstance (pCtx, hInstance)  );
	g_pCtx = pCtx;

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	g_pCtx = NULL;
	SG_ERR_CHECK(  CleanupInstance(pCtx)  );
	SG_CONTEXT_NULLFREE(pCtx);
	
	return (int) msg.wParam;

fail:

	SG_STRING_NULLFREE(pCtx, pstrArg);
	SG_CONTEXT_NULLFREE(pCtx);
	return FALSE;
}

static void RegisterWindowClass(PCWSTR pszClassName, PCWSTR pszMenuName, WNDPROC lpfnWndProc)
{
	WNDCLASSEX wcex = {sizeof(wcex)};
	//wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = lpfnWndProc;
	wcex.hInstance      = g_hInst;
	//wcex.hIcon          = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_TRAY_DEFAULT));
	//wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = pszMenuName;
	wcex.lpszClassName  = pszClassName;
	RegisterClassEx(&wcex);
}

static void InitInstance(SG_context* pCtx, HINSTANCE hInstance)
{
	HWND hWnd;
	char* szLogPath = NULL;
	char* szLogLevel = NULL;
	SG_pathname* pLogPath = NULL;
	SG_uint32							  logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;

	SG_ERR_CHECK(  SG_lib__global_initialize(pCtx)  );

#ifdef DEBUG

	/* ***This has to come after SG_lib__global_initialize or these settings will get changed.*** 
	
		Track memory leaks.
	*/
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags |= _CRTDBG_ALLOC_MEM_DF;
	flags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(flags);
	
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);
	
#endif

	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_PATH, NULL, &szLogPath, NULL)  );
	if (szLogPath != NULL)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pLogPath, szLogPath)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__LOG_DIRECTORY(pCtx, &pLogPath)  );
	}

	// get the configured log level
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_LEVEL, NULL, &szLogLevel, NULL)  );

	// register the file logger
	SG_zero(g_cLogFileData);
	SG_zero(g_cLogFileWriterData);
	SG_ERR_CHECK(  SG_log_text__set_defaults(pCtx, &g_cLogFileData)  );
	g_cLogFileData.fWriter             = SG_log_text__writer__daily_path;
	g_cLogFileData.pWriterData         = &g_cLogFileWriterData;
	if (szLogLevel != NULL)
	{
		if (SG_stricmp(szLogLevel, "quiet") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__QUIET;
			g_cLogFileData.bLogVerboseOperations = SG_FALSE;
			g_cLogFileData.bLogVerboseValues     = SG_FALSE;
		}
		else if (SG_stricmp(szLogLevel, "normal") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__NORMAL;
			g_cLogFileData.bLogVerboseOperations = SG_FALSE;
			g_cLogFileData.bLogVerboseValues     = SG_FALSE;
		}
		else if (SG_stricmp(szLogLevel, "verbose") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;
			g_cLogFileData.szRegisterMessage   = "---- " WHATSMYNAME " started logging ----";
			g_cLogFileData.szUnregisterMessage = "---- " WHATSMYNAME " stopped logging ----";
		}
	}
	logFileFlags |= SG_LOG__FLAG__DETAILED_MESSAGES;

	SG_ERR_CHECK(  SG_log_text__writer__daily_path__set_defaults(pCtx, &g_cLogFileWriterData)  );
	g_cLogFileWriterData.bReopen          = SG_FALSE;
	g_cLogFileWriterData.pBasePath        = pLogPath;

	if ( g_szProfile[0] && strcmp("default", g_szProfile) )
	{
		SG_ERR_CHECK(  SG_sprintf(pCtx, g_szLogFileName, sizeof(g_szLogFileName), "vv-winsync-%s-%%d-%%02d-%%02d.log", g_szProfile)  );
		g_cLogFileWriterData.szFilenameFormat = g_szLogFileName;
	}
	else
		g_cLogFileWriterData.szFilenameFormat = "vv-winsync-%d-%02d-%02d.log";

	SG_ERR_CHECK(  SG_log_text__register(pCtx, &g_cLogFileData, NULL, logFileFlags)  );

	g_hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(g_szWindowClass, g_szTitle, 0,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

	SG_NULLFREE(pCtx, szLogLevel);
	SG_NULLFREE(pCtx, szLogPath);

	return;

fail:
	SG_NULLFREE(pCtx, szLogLevel);
	SG_NULLFREE(pCtx, szLogPath);
	SG_PATHNAME_NULLFREE(pCtx, pLogPath); // must come after g_cLogFileData is unregistered, because it uses pLogPath
}

static void CleanupInstance(SG_context* pCtx)
{
	SG_ERR_IGNORE(  SG_log_text__unregister(pCtx, &g_cLogFileData)  );
	SG_PATHNAME_NULLFREE(pCtx, g_cLogFileWriterData.pBasePath); // must come after g_cLogFileData is unregistered, because it uses pLogPath
	SG_ERR_CHECK_RETURN(  SG_lib__global_cleanup(pCtx)  );
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int iStatus = STATUS_READY;
	static WorkerThreadParms threadParms;
	static int animationState = 0;

	int wmId, wmEvent;
	
	switch (message)
	{
	case WM_CREATE:

		if (!AddNotificationIcon(hWnd))
		{
			DWORD err = GetLastError();
			ErrorMsgBox(L"Veracity Sync: Unable to add tray icon", err);
			ExitProcess(err);
		}

		threadParms.szProfile = g_szProfile;
		threadParms.hWnd = hWnd;
		threadParms.bRunning = FALSE;
		threadParms.bShutdown = FALSE;
		threadParms.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		threadParms.lLastHearbeat = 0;
		threadParms.lLastSyncAttempt = 0;
		threadParms.lLastSyncSuccess = 0;
		
		threadParms.bRememberCredentials = SG_FALSE;
		threadParms.pszPassword = NULL;
		threadParms.pszUser = NULL;
		threadParms.pszSrcUlrForAuth = NULL;

		_beginthread(WorkerThread, 0, &threadParms);
		StartWorkerIfNotRunning(hWnd, &threadParms);


		break;

	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_WORKER:
			SetEvent(threadParms.hEvent); // Signal the worker thread to go to work.
			break;
		case TIMER_ANIMATE:
			if (animationState == 7)
				animationState = 0;
			else 
				animationState++;
			switch (animationState)
			{
			case 0:
			case 4:
				SetIconAndTooltip(hWnd, IDI_TRAY_DEFAULT);
				break;
			case 1:
			case 3:
				SetIconAndTooltip(hWnd, IDI_WORKING_2);
				break;
			case 2:
				SetIconAndTooltip(hWnd, IDI_WORKING_3);
				break;
			case 5:
			case 7:
				SetIconAndTooltip(hWnd, IDI_WORKING_4);
				break;
			case 6:
				SetIconAndTooltip(hWnd, IDI_WORKING_5);
				break;
			}
		}
		break;

	case WM_APP_READY:
		{
			if (iStatus == STATUS_BUSY)
				KillTimer(hWnd, TIMER_ANIMATE);

			/* We resume the worker thread if we were previously misconfigured. */
			iStatus = STATUS_READY;
			StartWorkerIfNotRunning(hWnd, &threadParms);

			SetIconAndTooltip(hWnd, IDI_TRAY_DEFAULT); 

			// If the context menu is showing, update it.
			if (g_hContextMenu)
			{
				EnableDisableMenuItems(g_hContextMenu, iStatus);
				DrawMenuBar(hWnd);
			}
		}
		break;

	case WM_APP_BUSY:

		iStatus = STATUS_BUSY;

		// If the context menu is showing, update it.
		if (g_hContextMenu)
		{
			EnableDisableMenuItems(g_hContextMenu, iStatus);
			DrawMenuBar(hWnd);
		}

		animationState = 0;
		SetIconAndTooltip(hWnd, IDI_TRAY_DEFAULT);
		SetTimer(hWnd, TIMER_ANIMATE, 200, NULL);

		break;

	case WM_APP_OFFLINE:
		if (iStatus == STATUS_BUSY)
			KillTimer(hWnd, TIMER_ANIMATE);
		iStatus = STATUS_OFFLINE;
		SetIconAndTooltip(hWnd, IDI_OFFLINE);
		break;

	case WM_APP_SHOW_BALLOON:
		ShowBalloon(hWnd, (LPWSTR)wParam, (LPWSTR)lParam);
		free((LPWSTR)lParam);
		break;

	case WM_APP_CONFIG_ERR:
		{
			/* If the config error state has changed, suspend the worker thread. */
			int iOldStatus = iStatus;
			iStatus = STATUS_MISCONFIGURED;

			if (iOldStatus != STATUS_MISCONFIGURED)
			{
				SetIconAndTooltip(hWnd, IDI_OFFLINE);
				StopWorkerIfRunning(hWnd, &threadParms);
			}
			break;
		}

	case WM_APP_WORKER_DONE:
		DestroyWindow(hWnd);
		break;

	case WM_APP_NEED_LOGIN:
		iStatus = STATUS_AUTH_NEEDED;
		ShowLoginDlg(hWnd, &threadParms);
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_EXIT:
			if (iStatus != STATUS_BUSY)
			{
				if (threadParms.bShutdown)
				{
					DestroyWindow(hWnd);
				}
				else
				{
					threadParms.bShutdown = TRUE;
					SetEvent(threadParms.hEvent);
					/* Worker thread will post WM_APP_WORKER_DONE 
					   when it has shut down. */
				}
			}
			else
			{
				/* This shouldn't ever happen. The menu is disabled. This is here just in case. */
				MessageBox(hWnd,
					L"Working: unable to exit. Try again in a moment.",
					L"Veracity Sync", MB_OK);
			}
			break;
		case IDM_OPTIONS:
			if (iStatus != STATUS_BUSY && g_hLoginDlg == NULL)
				ShowConfigDlg(hWnd, iStatus, &threadParms);
			break;
		case IDM_SYNCNOW:
			SyncNow(&threadParms);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_DESTROY:

		if (threadParms.bShutdown)
		{
			if (!RemoveNotificationIcon(hWnd))
				ErrorMsgBox(L"Unable to remove icon", GetLastError());

			PostQuitMessage(0);
		}
		else
		{
			threadParms.bShutdown = TRUE;
			SetEvent(threadParms.hEvent);
		}

		break;
	
	case WM_APP_NOTIFYCALLBACK:
		switch (LOWORD(lParam))
		{
		case WM_LBUTTONDBLCLK:
			if (iStatus != STATUS_BUSY && g_hLoginDlg == NULL)
			{
				if (g_hConfigDlg)
				{
					SetForegroundWindow(g_hConfigDlg);
				}
				else
				{
					WPARAM wpm = MAKEWPARAM(IDM_OPTIONS, 0);
					SendMessage(hWnd, WM_COMMAND, wpm, 0);
				}
			}
			else if (iStatus == STATUS_AUTH_NEEDED)
			{
				if (g_hLoginDlg)
				{
					SetForegroundWindow(g_hLoginDlg);
				}
				else
				{
					WPARAM wpm = MAKEWPARAM(WM_APP_NEED_LOGIN, 0);
					SendMessage(hWnd, WM_COMMAND, wpm, 0);
				}
			}
			break;

		case WM_CONTEXTMENU: // XP and later
			{
				POINT pt = {};
				if( GetCursorPos(&pt) )
					ShowContextMenu(hWnd, pt, iStatus);
			}
			break;

		case NIN_BALLOONUSERCLICK:
			if (iStatus == STATUS_MISCONFIGURED)
			{
				if (g_hConfigDlg)
				{
					SetForegroundWindow(g_hConfigDlg);
				}
				else
				{
					WPARAM wpm = MAKEWPARAM(IDM_OPTIONS, 0);
					SendMessage(hWnd, WM_COMMAND, wpm, 0);
				}
			}
			else if (iStatus == STATUS_AUTH_NEEDED)
			{
				if (g_hLoginDlg)
				{
					SetForegroundWindow(g_hLoginDlg);
				}
				else
				{
					WPARAM wpm = MAKEWPARAM(WM_APP_NEED_LOGIN, 0);
					SendMessage(hWnd, WM_COMMAND, wpm, 0);
				}
			}
			break;
			
		case NIN_BALLOONTIMEOUT:
		case NIN_BALLOONHIDE:
			/* After a balloon goes away, you have to restore the tooltip. */
			SetToolTipFromGlobalBuffer(hWnd);
			break;
		}
		break;

	/* RestartManager compliance: http://msdn.microsoft.com/en-us/library/aa373651.aspx */
	case WM_QUERYENDSESSION:
		if (ENDSESSION_CLOSEAPP == lParam)
		{
			/* RestartManager is asking if we can shut down, probably for a pending update or uninstall. */
			return 1;
		}

	case WM_ENDSESSION:
		if (ENDSESSION_CLOSEAPP == lParam)
		{
			if (wParam)
			{
				/* RestartManager is shutting us down, probably for a pending update or uninstall. */
				if (threadParms.bShutdown)
				{
					DestroyWindow(hWnd);
				}
				else
				{
					threadParms.bShutdown = TRUE;
					SetEvent(threadParms.hEvent);
					/* Worker thread will post WM_APP_WORKER_DONE 
					   when it has shut down. */
				}
				return 1;
			}
		}


	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

static BOOL ShowBalloon(HWND hWnd, LPWSTR title, LPWSTR msg)
{
	if (!title || !msg)
		return FALSE;

	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.hWnd = hWnd;
	nid.uID = MY_ICON_ID;
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_USER;

	// Balloon icon only supported on Vista and later
	// LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_TRAY_DEFAULT), LIM_LARGE, &nid.hBalloonIcon);

	// Deliberately not checking return values here. Let them truncate messages as needed.
	StringCchCopy(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), title);
	StringCchCopy(nid.szInfo, ARRAYSIZE(nid.szInfo), msg);

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

static BOOL AddNotificationIcon(HWND hwnd)
{
	
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.hWnd = hwnd;
	nid.uID = MY_ICON_ID;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.uCallbackMessage = WM_APP_NOTIFYCALLBACK;

	// Large icons only supported on Vista and later
	// LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_TRAY_DEFAULT), LIM_SMALL, &nid.hIcon);

	nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_OFFLINE));

	if ( !SUCCEEDED(StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), g_szTooltip)) )
		return FALSE;


	/* Give explorer a chance to launch, if it hasn't already.
	 * This can come up when we run on startup or when we're restarting after an upgrade. */
	BOOL ret = FALSE;
	for (int i = 0; !ret && (i < 20); i++)
	{
		ret = Shell_NotifyIcon(NIM_ADD, &nid);
		if (!ret)
			Sleep(500);
	}
	if (!ret)
		return ret;

	// Going lowest-common-denominator for now. We want to work on XP, so we specify Windows 2000 behavior:
	nid.uVersion = NOTIFYICON_VERSION;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

/* Changes the icon and restores the current tooltip so it will show up during animation. */
static BOOL SetIconAndTooltip(HWND hwnd, int id)
{
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.hWnd = hwnd;
	nid.uID = MY_ICON_ID;
	nid.uFlags = NIF_TIP | NIF_ICON;

	// Large icons only supported on Vista and later
	// 	if ( FAILED(LoadIconMetric(g_hInst, MAKEINTRESOURCE(id), LIM_SMALL, &nid.hIcon)) )
	// 		return FALSE;

	nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(id));

	if ( FAILED(StringCchPrintf(nid.szTip, ARRAYSIZE(nid.szTip), g_szTooltip)) )
		return FALSE;

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

static BOOL RemoveNotificationIcon(HWND hWnd)
{
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.hWnd = hWnd;
	nid.uID = MY_ICON_ID;
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

static void EnableDisableMenuItems(HMENU contextMenu, int iStatus)
{
	if (iStatus == STATUS_BUSY || g_hConfigDlg != NULL || g_hLoginDlg != NULL)
	{
		EnableMenuItem(contextMenu, IDM_OPTIONS, MF_GRAYED | MF_BYCOMMAND); // Disable options
		EnableMenuItem(contextMenu, IDM_SYNCNOW, MF_GRAYED | MF_BYCOMMAND); // Disable Sync Now
		EnableMenuItem(contextMenu, IDM_EXIT, MF_GRAYED | MF_BYCOMMAND); // Disable exit
	}
	else
	{
		EnableMenuItem(contextMenu, IDM_OPTIONS, MF_ENABLED | MF_BYCOMMAND); // Enable options
		EnableMenuItem(contextMenu, IDM_SYNCNOW, MF_ENABLED | MF_BYCOMMAND); // Enable Sync Now
		EnableMenuItem(contextMenu, IDM_EXIT, MF_ENABLED | MF_BYCOMMAND); // Enable exit
	}
}

static void ShowContextMenu(HWND hwnd, POINT pt, int iStatus)
{
	HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_TRAY_CONTEXT));
	if (hMenu)
	{
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (hSubMenu)
		{
			// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
			SetForegroundWindow(hwnd);

			// respect menu drop alignment
			UINT uFlags = TPM_RIGHTBUTTON;
			if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
			{
				uFlags |= TPM_RIGHTALIGN;
			}
			else
			{
				uFlags |= TPM_LEFTALIGN;
			}

			// Make "Options..." the default
			MENUITEMINFO info = {sizeof(MENUITEMINFO)};
			info.fMask = MIIM_STATE;
			info.fState = MFS_DEFAULT;
			SetMenuItemInfo(hSubMenu, IDM_OPTIONS, FALSE, &info);

			EnableDisableMenuItems(hSubMenu, iStatus);

			g_hContextMenu = hSubMenu;
			TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
			g_hContextMenu = NULL;
		}

		DestroyMenu(hMenu);
	}
}

static BOOL SetToolTipFromGlobalBuffer(HWND hWnd)
{
	if (!g_szTooltip)
		return FALSE;

	NOTIFYICONDATA nid = {sizeof(nid)};
	SG_STATIC_ASSERT(ARRAYSIZE(nid.szTip) == MAX_TOOLTIP);

	nid.uFlags = NIF_TIP;
	nid.hWnd = hWnd;
	nid.uID = MY_ICON_ID;

	if ( FAILED(StringCchPrintf(nid.szTip, ARRAYSIZE(nid.szTip), g_szTooltip)) )
		return FALSE;

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

HRESULT SetToolTip(LPWSTR pszFormat, ...)
{
	HRESULT hr;
	va_list argList;

	va_start(argList, pszFormat);

	hr = StringCchVPrintf(g_szTooltip,
		ARRAYSIZE(g_szTooltip),
		pszFormat,
		argList);

	va_end(argList);

	return hr;
}

static INT_PTR CALLBACK LoginDlgProc(
	HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	SG_context* pCtx = g_pCtx;
	static volatile WorkerThreadParms* pThreadParms = NULL;

	SG_string* pstrUser = NULL;
	SG_string* pstrPass = NULL;

	HWND hCtl = NULL;
	HWND hParent = NULL;
	LRESULT checked = BST_INDETERMINATE;
	LPWSTR pwsz = NULL;
	char* pszDest = NULL;
	SG_repo* pRepo = NULL;
	SG_varray* pvaUsers = NULL;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			g_hLoginDlg = hDlg;
			SetForegroundWindow(hDlg);

			pThreadParms = (volatile WorkerThreadParms*)lParam;

			hCtl = GetDlgItem(hDlg, IDC_STATIC_URL);
			if (hCtl)
			{
				ComboBox_LimitText(hCtl, MAX_OPTION_LEN);
				SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pThreadParms->pszSrcUlrForAuth, &pwsz, NULL)  );
				Static_SetText(hCtl, pwsz);
				SG_NULLFREE(pCtx, pwsz);
			}

			hCtl = GetDlgItem(hDlg, IDC_COMBO_USER);
			if (hCtl)
			{
				ComboBox_LimitText(hCtl, MAX_OPTION_LEN);
				SG_ERR_CHECK(  GetProfileSettings(pCtx, pThreadParms->szProfile, &pszDest, NULL, NULL, NULL)  );
				SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDest, &pRepo)  );
				SG_ERR_CHECK(  SG_user__list_active(pCtx, pRepo, &pvaUsers)  );
				{
					SG_uint32 count = 0;
					SG_uint32 i;
					SG_vhash* pvh;
					const char* pszUsername;

					SG_ERR_CHECK(  SG_varray__count(pCtx, pvaUsers, &count)  );
					for (i = 0; i < count; i++)
					{
						SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaUsers, i, &pvh)  );
						SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "name", &pszUsername)  );
						SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszUsername, &pwsz, NULL)  );
						ComboBox_AddString(hCtl, pwsz);
						SG_NULLFREE(pCtx, pwsz);
					}
				}

				if (pThreadParms->pszUser)
				{
					SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pThreadParms->pszUser, &pwsz, NULL)  );
					ComboBox_SetText(hCtl, pwsz);
					SG_NULLFREE(pCtx, pwsz);
				}

				SG_NULLFREE(pCtx, pszDest);
				SG_VARRAY_NULLFREE(pCtx, pvaUsers);
				SG_REPO_NULLFREE(pCtx, pRepo);
			}
		}

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			
			WCHAR buf[MAX_OPTION_LEN];

			SG_NULLFREE(pCtx, pThreadParms->pszUser);
			SG_NULLFREE(pCtx, pThreadParms->pszPassword);

			SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrUser, ARRAYSIZE(buf))  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrPass, ARRAYSIZE(buf))  );

			//Get the username.
			hCtl = GetDlgItem(hDlg, IDC_COMBO_USER);
			ComboBox_GetText(hCtl, buf, ARRAYSIZE(buf));
			if (buf[0])
				SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrUser, buf)  );

			//Get the password.
			hCtl = GetDlgItem(hDlg, IDC_EDIT_PASS);
			Edit_GetText(hCtl, buf, ARRAYSIZE(buf));
			if (buf[0])
				SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrPass, buf)  );

			hCtl = GetDlgItem(hDlg, IDC_CHECK_REMEMBER);
			checked = Button_GetCheck(hCtl);

			pThreadParms->bRememberCredentials = (BST_CHECKED == checked);
	
			SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrUser, (SG_byte**)&pThreadParms->pszUser, NULL)  );
			SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrPass, (SG_byte**)&pThreadParms->pszPassword, NULL)  );

			pThreadParms->bCredentialsChanged = TRUE;

			hParent = GetParent(hDlg);
			SendMessage(hParent, WM_APP_READY, 0, 0);

			EndDialog(hDlg, 1);
			return TRUE;
			
		case IDCANCEL:
			EndDialog(hDlg, 2);
			return TRUE;
		}
		break;
	}
	return FALSE;
fail:
	SG_STRING_NULLFREE(pCtx, pstrUser);
	SG_STRING_NULLFREE(pCtx, pstrPass);
	SG_NULLFREE(pCtx, pwsz);
	SG_NULLFREE(pCtx, pszDest);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VARRAY_NULLFREE(pCtx, pvaUsers);
	return TRUE;
}

static void _populateSrcCombo(SG_context* pCtx, HWND hDlg, const char* pszSrc, const char* pszDest)
{
	HWND hCtl = NULL;
	SG_string* pstrConfigPath = NULL;
	SG_varray* pvaConfigSyncTargets = NULL;
	LPWSTR pwszSrc = NULL;
	WCHAR bufPrevText[MAX_OPTION_LEN];

	hCtl = GetDlgItem(hDlg, IDC_COMBO_SRC);
	if (hCtl)
	{
		ComboBox_GetText(hCtl, bufPrevText, ARRAYSIZE(bufPrevText));

		ComboBox_ResetContent(hCtl);
		if (pszDest && *pszDest)
		{
			SG_uint32 count = 0;
			SG_uint32 i;
			const char* psz;

			SG_STRING__ALLOC(pCtx, &pstrConfigPath);
			SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstrConfigPath, "/instance/%s/sync_targets", pszDest)  );
			SG_ERR_CHECK(  SG_localsettings__get__varray(pCtx, SG_string__sz(pstrConfigPath), NULL, &pvaConfigSyncTargets, NULL)  );

			if (pvaConfigSyncTargets)
			{
				SG_ERR_CHECK(  SG_varray__count(pCtx, pvaConfigSyncTargets, &count)  );
				for (i = 0; i < count; i++)
				{
					SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaConfigSyncTargets, i, &psz)  );
					SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, psz, &pwszSrc, NULL)  );
					ComboBox_AddString(hCtl, pwszSrc);
					SG_NULLFREE(pCtx, pwszSrc);
				}

				SG_VARRAY_NULLFREE(pCtx, pvaConfigSyncTargets);						
			}
			SG_STRING_NULLFREE(pCtx, pstrConfigPath);
		}

		if (pszSrc)
		{
			SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszSrc, &pwszSrc, NULL)  );
			ComboBox_SetText(hCtl, pwszSrc);
			SG_NULLFREE(pCtx, pwszSrc);
		}
		else
		{
			if (bufPrevText[0])
				ComboBox_SetText(hCtl, bufPrevText);
		}
	}

	return;

fail:
	SG_NULLFREE(pCtx, pwszSrc);
	SG_STRING_NULLFREE(pCtx, pstrConfigPath);
	SG_VARRAY_NULLFREE(pCtx, pvaConfigSyncTargets);
}

static INT_PTR CALLBACK ConfigDlgProc(
	HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	static volatile WorkerThreadParms* pThreadParms = NULL;
	static BOOL bDestChanged = FALSE;

	SG_context* pCtx = g_pCtx;
	char* pszDest = NULL;
	char* pszSrc = NULL;
	SG_int64 lMinutes = DEFAULT_SYNC_EVERY_N_MINUTES;

	HWND hParent = NULL;

	HWND hCtl = NULL;
	LPWSTR pwszDest = NULL;
	LPWSTR pwszMinutes = NULL;


	SG_string* pstrDest = NULL;
	SG_string* pstrSrc = NULL;
	SG_string* pstrMinutes = NULL;

	SG_vhash* pvhDescriptors = NULL;
	SG_varray* pvaDescriptors = NULL;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			g_hConfigDlg = hDlg;
			SetForegroundWindow(hDlg);
			pThreadParms = (volatile WorkerThreadParms*)lParam;

			SG_ERR_CHECK(  GetProfileSettings(pCtx, g_szProfile, &pszDest, &pszSrc, &lMinutes, NULL)  );

			hCtl = GetDlgItem(hDlg, IDC_COMBO_DEST);
			if (hCtl)
			{
				SG_uint32 count = 0;

				ComboBox_LimitText(hCtl, MAX_OPTION_LEN);
				ComboBox_SetMinVisible(hCtl, 10);

				SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pvhDescriptors)  );
				SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvhDescriptors, &pvaDescriptors)  );
				SG_ERR_CHECK(  SG_varray__count(pCtx, pvaDescriptors, &count)  );

				for (SG_uint32 i=0; i<count; i++)
				{
					const char* pszName_ref;
					LPWSTR pwszName = NULL;

					SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaDescriptors, i, &pszName_ref)  );
					SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszName_ref, &pwszName, NULL)  );
					ComboBox_AddString(hCtl, pwszName);
					SG_NULLFREE(pCtx, pwszName);
				}

				SG_VARRAY_NULLFREE(pCtx, pvaDescriptors);
				SG_VHASH_NULLFREE(pCtx, pvhDescriptors);

				if (pszDest && *pszDest)
				{
					if (hCtl)
					{
						SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszDest, &pwszDest, NULL)  );
						ComboBox_SetText(hCtl, pwszDest);
						SG_NULLFREE(pCtx, pwszDest);
					}
				}
			}

			hCtl = GetDlgItem(hDlg, IDC_COMBO_SRC);
			if (hCtl)
			{
				ComboBox_LimitText(hCtl, MAX_OPTION_LEN);
				ComboBox_SetMinVisible(hCtl, 10);
			}
			if (pszSrc && *pszSrc)
				SG_ERR_CHECK(  _populateSrcCombo(pCtx, hDlg, pszSrc, pszDest)  );

			SG_NULLFREE(pCtx, pszDest);
			SG_NULLFREE(pCtx, pszSrc);

			hCtl = GetDlgItem(hDlg, IDC_EDIT_MINUTES);
			if (hCtl)
			{
				SG_int_to_string_buffer buf;
				ComboBox_LimitText(hCtl, 3);
				SG_int64_to_sz(lMinutes, buf);
				SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, buf, &pwszMinutes, NULL)  );
				Edit_SetText(hCtl, pwszMinutes);
				SG_NULLFREE(pCtx, pwszMinutes);
			}
		}

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			
			WCHAR buf[MAX_OPTION_LEN];

			SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrDest, ARRAYSIZE(buf))  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrSrc, ARRAYSIZE(buf))  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrMinutes, ARRAYSIZE(buf))  );

			hCtl = GetDlgItem(hDlg, IDC_COMBO_DEST);
			ComboBox_GetText(hCtl, buf, ARRAYSIZE(buf));
			if (buf[0])
				SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrDest, buf)  );

			hCtl = GetDlgItem(hDlg, IDC_COMBO_SRC);
			ComboBox_GetText(hCtl, buf, ARRAYSIZE(buf));
			if (buf[0])
				SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrSrc, buf)  );

			hCtl = GetDlgItem(hDlg, IDC_EDIT_MINUTES);
			Edit_GetText(hCtl, buf, ARRAYSIZE(buf));
			if (buf[0])
			{
				/* These are always digits, so this is probably overkill, but it's safe and works. */
				SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrMinutes, buf)  );
				SG_ERR_CHECK(  SG_int64__parse(pCtx, &lMinutes, SG_string__sz(pstrMinutes))  );
			}

			SG_ERR_CHECK(  SetProfileSettings(pCtx, g_szProfile, SG_string__sz(pstrDest), SG_string__sz(pstrSrc), lMinutes)  );
			SG_NULLFREE(pCtx, pThreadParms->pszSrcUlrForAuth);
			SG_NULLFREE(pCtx, pThreadParms->pszUser);
			SG_NULLFREE(pCtx, pThreadParms->pszPassword);

			SG_STRING_NULLFREE(pCtx, pstrDest);
			SG_STRING_NULLFREE(pCtx, pstrSrc);
			SG_STRING_NULLFREE(pCtx, pstrMinutes);

			hParent = GetParent(hDlg);
			SendMessage(hParent, WM_APP_READY, 0, 0);

			EndDialog(hDlg, 1);
			return TRUE;
			
		case IDCANCEL:
			EndDialog(hDlg, 2);
			return TRUE;

		case IDC_COMBO_DEST:
			switch (HIWORD(wParam))
			{
				case CBN_EDITCHANGE:
				case CBN_SELCHANGE:
					bDestChanged = TRUE;
					break;
				case CBN_KILLFOCUS:
					if (bDestChanged)
					{
						SG_bool bPopulated = SG_FALSE;
						hCtl = GetDlgItem(hDlg, IDC_COMBO_DEST);
						if (hCtl)
						{
							ComboBox_GetText(hCtl, buf, ARRAYSIZE(buf));
							if (buf[0])
							{
								SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrDest)  );
								SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrDest, buf)  );
								SG_ERR_CHECK(  _populateSrcCombo(pCtx, hDlg, NULL, SG_string__sz(pstrDest))  );
								SG_STRING_NULLFREE(pCtx, pstrDest);
								bPopulated = SG_TRUE;
							}
						}
						if (!bPopulated)
							SG_ERR_CHECK(  _populateSrcCombo(pCtx, hDlg, NULL, NULL)  );
						bDestChanged = FALSE;
					}
					break;
			}
		}
		break;
	}
	return FALSE;

fail:
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pwszDest);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pwszMinutes);
	SG_STRING_NULLFREE(pCtx, pstrDest);
	SG_STRING_NULLFREE(pCtx, pstrSrc);
	SG_STRING_NULLFREE(pCtx, pstrMinutes);
	SG_VARRAY_NULLFREE(pCtx, pvaDescriptors);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptors);
	
	return TRUE;
}

static void ErrorMsgBox(const LPWSTR lpMsgPrefix, DWORD err)
{
	LPVOID lpMsgBuf = NULL;
	LPVOID lpDisplayBuf = NULL;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message and exit the process
	size_t len_needed = (lstrlen((LPCWSTR)lpMsgBuf) + 256) * sizeof(WCHAR);
	if (lpMsgPrefix)
		len_needed += (lstrlen((LPCWSTR)lpMsgPrefix) + 256) * sizeof(WCHAR);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, len_needed); 
	if (!lpDisplayBuf)
		exit(-1);

	if (lpMsgPrefix)
		StringCchPrintf((LPWSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(WCHAR),
		L"%s: (%d) %s", lpMsgPrefix,
		err, lpMsgBuf); 
	else
		StringCchPrintf((LPWSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(WCHAR),
		L"Error %d: %s", 
		err, lpMsgBuf); 

	MessageBox(NULL, (LPCWSTR)lpDisplayBuf, L"Veracity Sync: Error", MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

static void ShowConfigDlg(HWND hWnd, int iStatus, volatile WorkerThreadParms* pThreadParms)
{
	if (iStatus == STATUS_BUSY)
		return;

	if (g_hConfigDlg)
	{
		SetForegroundWindow(g_hConfigDlg);
		return;
	}

	BOOL bWasRunning = pThreadParms->bRunning;
	StopWorkerIfRunning(hWnd, pThreadParms); // We may change data the worker thread uses, so we suspend it.

	SetForegroundWindow(hWnd);
	INT_PTR dr = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_OPTIONS), hWnd, ConfigDlgProc, (LPARAM)pThreadParms);
	g_hConfigDlg = NULL;

	/* If the worker was running before the config dialog was opened, start it again.
	 * If it wasn't, start it only if OK was clicked. */
	if (bWasRunning || dr == 1)
		StartWorkerIfNotRunning(hWnd, pThreadParms);

	if (dr < 0)
		ErrorMsgBox(L"Unable to display configuration dialog", GetLastError());
	else if (dr == 1)
	{
		/* If OK was clicked, do a heartbeat/sync right away. This will clear STATUS_MISCONFIGURED when appropriate. */
		SyncNow(pThreadParms);
	}
}

static void ShowLoginDlg(HWND hWnd, volatile WorkerThreadParms* pThreadParms)
{
	if (g_hLoginDlg)
	{
		SetForegroundWindow(g_hLoginDlg);
		return;
	}

	StopWorkerIfRunning(hWnd, pThreadParms); // We may change data the worker thread uses, so we suspend it.

	KillTimer(hWnd, TIMER_ANIMATE);
	SetToolTip(L"Offline: Login Required");
	SetIconAndTooltip(hWnd, IDI_OFFLINE);
	ShowBalloon(hWnd, L"Veracity Sync Offline", L"Login Required");

	SetForegroundWindow(hWnd);
	INT_PTR dr = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_LOGIN), hWnd, LoginDlgProc, (LPARAM)pThreadParms);
	g_hLoginDlg = NULL;

	if (dr < 0)
		ErrorMsgBox(L"Unable to display login dialog", GetLastError());
	else if (dr == 1) // OK clicked on login dialog
	{
		StartWorkerIfNotRunning(hWnd, pThreadParms);

		/* If OK was clicked, do a heartbeat/sync right away. This will clear STATUS_AUTH_NEEDED when appropriate. */
		SyncNow(pThreadParms);
	}
}

static void SyncNow(volatile WorkerThreadParms* pThreadParms)
{
	pThreadParms->lLastHearbeat = 0;
	pThreadParms->lLastSyncAttempt = 0;
	SetEvent(pThreadParms->hEvent);
}

static void StopWorkerIfRunning(HWND hWnd, volatile WorkerThreadParms* pThreadParms)
{
	if (pThreadParms->bRunning)
	{
		INT_PTR tr = KillTimer(hWnd, TIMER_WORKER); // We may change data the worker thread uses, so we suspend it.
		if (!tr)
		{
			DWORD err = GetLastError();
			ErrorMsgBox(L"Unable to suspend worker thread", err);
			ExitProcess(err);
		}
		pThreadParms->bRunning = FALSE;
	}
}

static void StartWorkerIfNotRunning (HWND hWnd, volatile WorkerThreadParms* pThreadParms)
{
	if (!pThreadParms->bRunning)
	{
		UINT_PTR tr = SetTimer(hWnd, TIMER_WORKER, WORKER_THREAD_WORKS_EVERY_N_MS, NULL);
		if (!tr)
		{
			DWORD err = GetLastError();
			ErrorMsgBox(L"Unable to set worker thread timer", err);
			ExitProcess(err);
		}
		pThreadParms->bRunning = TRUE;
	}
}