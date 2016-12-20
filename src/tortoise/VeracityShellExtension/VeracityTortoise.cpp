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
// VeracityTortoise.cpp : Implementation of DLL Exports.

//
// Note: COM+ 1.0 Information:
//      Please remember to run Microsoft Transaction Explorer to install the component(s).
//      Registration is not done by default. 

#include "stdafx.h"
#include "resource.h"
#include "VeracityTortoise_i.h"
#include "dllmain.h"
#include "compreg.h"




#define MY_GUID _T("{FA2FE306-E2C2-11DF-BEA7-DCBADFD72085}")


// Used to determine whether the DLL can be unloaded by OLE.
STDAPI DllCanUnloadNow(void)
{
			return _AtlModule.DllCanUnloadNow();
	}

// Returns a class factory to create an object of the requested type.
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT returnCode = _AtlModule.DllGetClassObject(rclsid, riid, ppv);
	return returnCode;
}

STDAPI SetApprovedInRegistry(LPCTSTR guid, LPCTSTR name)
{
	 if ( 0 == (GetVersion() & 0x80000000UL) )
    {
		CRegKey reg;
		LONG    lRet;

		lRet = reg.Open ( HKEY_LOCAL_MACHINE,
							_T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
							KEY_SET_VALUE );

		if ( ERROR_SUCCESS != lRet )
			return E_ACCESSDENIED;

		lRet = reg.SetStringValue ( guid, name );

		if ( ERROR_SUCCESS != lRet )
			return E_ACCESSDENIED;
    }
	return S_OK;

}
STDAPI SetTortoiseOverlaysInRegistry(LPCTSTR tortoiseOverlayType, LPCTSTR guid, LPCTSTR name)
{
	//MessageBox(NULL,(const TCHAR *)_T("Overlay Enter"),NULL,MB_OK|MB_ICONERROR);
	 if ( 0 == (GetVersion() & 0x80000000UL) )
    {
		CRegKey reg;
		LONG    lRet;
		TCHAR buf[MAX_PATH];
		wsprintf(buf, _T("Software\\TortoiseOverlays\\%s"), tortoiseOverlayType);
		//MessageBox(NULL,(const TCHAR *)buf,NULL,MB_OK|MB_ICONERROR);
		//lRet = reg.Open ( HKEY_LOCAL_MACHINE,
			//				_T("Software\\TortoiseOverlays"),
				//			KEY_CREATE_SUB_KEY);
		//if ( ERROR_SUCCESS != lRet )
			//return E_ACCESSDENIED;
		lRet = reg.Create(HKEY_LOCAL_MACHINE, buf);//, (LPTSTR)tortoiseOverlayType, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, NULL);

		//lRet = reg.Open ( HKEY_LOCAL_MACHINE,
			//				buf,
				//			KEY_CREATE_SUB_KEY);
		if ( ERROR_SUCCESS != lRet )
			return E_ACCESSDENIED;
		//MessageBox(NULL,(const TCHAR *)_T("Overlay Opened"),NULL,MB_OK|MB_ICONERROR);
		

		lRet = reg.SetStringValue ( name, guid );
		if ( ERROR_SUCCESS != lRet )
			return E_ACCESSDENIED;
		//MessageBox(NULL,(const TCHAR *)_T("Overlay String Set"),NULL,MB_OK|MB_ICONERROR);
    }
	 //MessageBox(NULL,(const TCHAR *)_T("Overlay Exit"),NULL,MB_OK|MB_ICONERROR);
	return S_OK;

}
// DllRegisterServer - Adds entries to the system registry.
STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	 // If we're on NT, add ourselves to the list of approved shell extensions.

    // Note that you should *NEVER* use the overload of CRegKey::SetValue with
    // 4 parameters.  It lets you set a value in one call, without having to 
    // call CRegKey::Open() first.  However, that version of SetValue() has a
    // bug in that it requests KEY_ALL_ACCESS to the key.  That will fail if the
    // user is not an administrator.  (The code should request KEY_WRITE, which
    // is all that's necessary.)
	//MessageBox(NULL,(const TCHAR *)_T("Register"),NULL,MB_OK|MB_ICONERROR);
#if  DEBUG
	HRESULT approvalResult = SetApprovedInRegistry(MY_GUID, _T("VeracityContextMenu extension"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetApprovedInRegistry(_T("{9D6672A8-49BD-4E66-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Added"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetTortoiseOverlaysInRegistry(_T("Added"), _T("{9D6672A8-49BD-4E66-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Added"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetApprovedInRegistry(_T("{9D6672A8-49BD-4E67-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Conflict"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetTortoiseOverlaysInRegistry(_T("Conflict"), _T("{9D6672A8-49BD-4E67-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Conflict"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetApprovedInRegistry(_T("{9D6672A8-49BD-4E68-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Ignored"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetTortoiseOverlaysInRegistry(_T("Ignored"), _T("{9D6672A8-49BD-4E68-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Ignored"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetApprovedInRegistry(_T("{9D6672A8-49BD-4E69-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Modified"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetTortoiseOverlaysInRegistry(_T("Modified"), _T("{9D6672A8-49BD-4E69-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Modified"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetApprovedInRegistry(_T("{9D6672A8-49BD-4E6A-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Normal"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetTortoiseOverlaysInRegistry(_T("Normal"), _T("{9D6672A8-49BD-4E6A-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Normal"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetApprovedInRegistry(_T("{9D6672A8-49BD-4E6B-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Unversioned"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetTortoiseOverlaysInRegistry(_T("Unversioned"), _T("{9D6672A8-49BD-4E6B-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Unversioned"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetApprovedInRegistry(_T("{9D6672A8-49BD-4E6C-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Locked"));
	if (approvalResult != S_OK)
		return approvalResult;
	approvalResult = SetTortoiseOverlaysInRegistry(_T("Locked"), _T("{9D6672A8-49BD-4E6C-A7D0-3AE45397B6F2}"), _T("VeracityOverlay_Locked"));
	if (approvalResult != S_OK)
		return approvalResult;

#endif
	HRESULT hr = _AtlModule.DllRegisterServer();
		return hr;
}

// DllUnregisterServer - Removes entries from the system registry.
STDAPI DllUnregisterServer(void)
{
	    // If we're on NT, remove ourselves from the list of approved shell extensions.
    // Note that if we get an error along the way, I don't bail out since I want
    // to do the normal ATL unregistration stuff too.

    if ( 0 == (GetVersion() & 0x80000000UL) )
        {
        CRegKey reg;
        LONG    lRet;

        lRet = reg.Open ( HKEY_LOCAL_MACHINE,
                          _T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
                          KEY_SET_VALUE );

        if ( ERROR_SUCCESS == lRet )
            {
            lRet = reg.DeleteValue ( MY_GUID );
            }
        }
	HRESULT hr = _AtlModule.DllUnregisterServer();
		return hr;
}

// DllInstall - Adds/Removes entries to the system registry per user per machine.
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
	//MessageBox(NULL,(const TCHAR *)_T("Install"),NULL,MB_OK|MB_ICONERROR);
	HRESULT hr = E_FAIL;
	static const wchar_t szUserSwitch[] = L"user";

	if (pszCmdLine != NULL)
	{
		if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
		{
			ATL::AtlSetPerUserRegistration(true);
		}
	}

	if (bInstall)
	{	
		hr = DllRegisterServer();
		if (FAILED(hr))
		{
			DllUnregisterServer();
		}
	}
	else
	{
		hr = DllUnregisterServer();
	}

	return hr;
}



