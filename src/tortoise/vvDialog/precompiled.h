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

#ifndef VV_PRECOMPILED_H
#define VV_PRECOMPILED_H

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Ignore static analysis warnings in Windows and WX headers. There are many. */
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include<codeanalysis\sourceannotations.h>
#endif

#include <wx/wxprec.h>
#include <wx/cmdline.h>
#include <wx/combo.h>
#include <wx/dataview.h>
#include <wx/datectrl.h>
#include <wx/datetime.h>
#include <wx/dir.h>
#include <wx/hyperlink.h>
#include <wx/listctrl.h>
#include <wx/mstream.h>
#include <wx/msgqueue.h>
#include <wx/notebook.h>
#include <wx/progdlg.h>
#include <wx/propgrid/manager.h>
#include <wx/regex.h>
#include <wx/renderer.h>
#include <wx/splitter.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/wupdlock.h>
#include <wx/xrc/xmlres.h>

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <vector>

// use MSW CRT memory leak detecting new operator
// must come after STL includes, because some STL headers also manipulate the new operator
#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Stop ignoring static analysis warnings. */
#pragma warning(pop)
#endif

#endif