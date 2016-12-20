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
// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "VeracityTortoise_i.h"
#include "dllmain.h"
#include "compreg.h"

#include "sg.h"

CVeracityTortoiseModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	hInstance;
	//This block prevents the dll from registering if there is no
	// debugger present.  This is helpful if you happen to be 
	// developing the tortoise client.  It makes sure that the dll
	// never gets loaded by some long-running process (like explorer 
	// or chrome etc).  In order to activate this check, pass 
	// -D TORTOISE_DEV=true to cmake.

	if (dwReason == DLL_PROCESS_ATTACH)
	{
#ifdef TORTOISE_DEV		
		if (::IsDebuggerPresent() == false)
		{
			TCHAR buf[_MAX_PATH + 1];
			GetModuleFileName(NULL, buf, _MAX_PATH);
			LPWSTR fileName = PathFindFileNameW(buf);
#if 0
			//This section allows you to control binding the debug dll
			//by changing the process name that's loading it.  The reason
			//for this is if you're using some other profiler or memory
			//allocation tracker.
			size_t nameLen = wcslen(fileName);
			LPWSTR suffix = _T("__allow_debug_veracity_to_bind.exe");
			size_t suffixLen = wcslen(suffix);
			if (nameLen > suffixLen && StrCmpW(fileName + (nameLen - suffixLen), suffix) == 0) //Allow the debug build to register.
			{
				//Nothing to do.  They are running a program that has been renamed to __allow_debug_veracity_to_bind.exe
			}
			else 
#endif
#ifdef ALLOW_TORTOISE_QDIR_DEBUG
				//Allow the Q32.exe and regsvr32 to run this dll
				if (! (StrCmpW(fileName, _T("Q32.exe")) == 0
					|| StrCmpW(fileName, _T("regsvr32.exe")) == 0) )
				{
					//we're neither Q32, nor regsvr32.  Don't load.
					return false;
				}
#else
				if (StrCmpW(fileName, _T("regsvr32.exe")) != 0) //Allow the debug build to register.
					return false;
#endif
				
				
		}
#endif
		//This global initialization should only happen once for the process.
		SG_context* pCtx = NULL;
		SG_CONTEXT__ALLOC__TEMPORARY(&pCtx);
		SG_ERR_IGNORE(  SG_lib__global_initialize(pCtx)  );
		SG_CONTEXT_NULLFREE(pCtx);
	}

	if (dwReason == DLL_PROCESS_DETACH)
	{
		SG_context* pCtx = NULL;
		SG_CONTEXT__ALLOC__TEMPORARY(&pCtx);
		SG_ERR_IGNORE(  SG_lib__global_cleanup(pCtx)  );
		SG_CONTEXT_NULLFREE(pCtx);

#if DEBUG
		WCHAR buf[1024];
		FILE* pFile = NULL;
		ExpandEnvironmentStringsW(L"%TEMP%\\veracity-tortoise-leaks.log", buf, sizeof(buf));

		_wfopen_s(&pFile, buf, L"w");
		SG__malloc__windows__mem_dump(pFile);
		fclose(pFile);

		HANDLE hLogFile = CreateFileW(buf, FILE_APPEND_DATA, 
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hLogFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND )
		{
			hLogFile = CreateFileW(buf, GENERIC_WRITE, 
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW, 
				FILE_ATTRIBUTE_NORMAL, NULL);
		}

		if (hLogFile != INVALID_HANDLE_VALUE)
		{
			_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
			_CrtSetReportFile(_CRT_WARN, hLogFile);
			_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
			_CrtSetReportFile(_CRT_ERROR, hLogFile);
		}

		int leaksExist = _CrtDumpMemoryLeaks();

		if (hLogFile != INVALID_HANDLE_VALUE)
			CloseHandle(hLogFile);

		if (leaksExist)
			MessageBoxW(NULL, buf, L"Thar be leaks!", MB_OK | MB_ICONEXCLAMATION);

#endif
	}

	return _AtlModule.DllMain(dwReason, lpReserved); 
}
