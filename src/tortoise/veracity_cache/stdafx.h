// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Ignore static analysis warnings in WX headers. There are many. */
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include<codeanalysis\sourceannotations.h>
#endif

#include "wx/app.h"

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Stop ignoring static analysis warnings. */
#pragma warning(pop)
#endif

#include <stdio.h>
#include "targetver.h"
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>

#include <vector>
#include <list>

using namespace std;



// TODO: reference additional headers your program requires here
