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

#ifndef SG_CACHE_APPLICATION_H
#define SG_CACHE_APPLICATION_H

#include "stdafx.h"
#include <vector>

#include "sg_status_cache.h"
#include "sg_read_dir_manager.h"
#include "sg.h"
#include "sg_workingdir_prototypes.h"

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Ignore static analysis warnings in WX headers. There are many. */
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include<codeanalysis\sourceannotations.h>
#endif

#include "wx/timer.h"
#include "wx\file.h"
#include <atlstr.h>

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Stop ignoring static analysis warnings. */
#pragma warning(pop)
#endif


/**
 * Represents the application itself, and contains application-wide functionality.
 */
class sg_cache_application : public wxAppConsole
{
private:
	SG_context *				pCtx;
	sg_read_dir_manager * m_manager;

	
	//Log stuff that is initialized in SetupSGLogging and freed in OnExit
	SG_pathname*				m_pLogPath;
	SG_log_console__data                  m_cLogStdData;
	SG_log_text__data                     m_cLogFileData;
	SG_log_text__writer__daily_path__data m_cLogFileWriterData;
	class sg_wait_for_connection__thread * m_connectorThread;
	HANDLE						m_hPipe;
	
	SG_bool						m_bExiting;
	wxCriticalSection			m_criticalSection; 
	
// types
public:
	/**
	 * Exit codes.
	 */
	enum ResultCode
	{
		RESULT_SUCCESS =  0, //< Command execution succeeded.
		RESULT_ERROR   = -1, //< An error occurred (-1 because that's what wx returns if OnInit fails).
	};

// construction
public:
	/**
	 * Default constructor.
	 */
	sg_cache_application();

// implementation
public:
	/**
	 * Initializes the app
	 */
	virtual bool OnInit();

	/**
	* This function just needs to pend the event to read a message from 
	* the named pipe.
	*/
	virtual void OnEventLoopEnter(wxEventLoopBase* evtLoop);

	virtual int OnExit();

	void SetupSGLogging();
	void OnWaitForNextClient(class WaitForNextClientEvent& e);
	void ConnectNextClient(SG_context * pTheContext);
	void OnReadOneMessage(class ReadOneMessageEvent& e);

	bool CreateMyNamedPipe();
	void HandleOneMessage(SG_vhash * message, const char * psz_wc_root, SG_vhash ** ppvh_result, sg_treediff_cache ** ppTreediffCache);
	void HandleConflictQuery(SG_vhash * message, const char * psz_wc_root, SG_vhash ** ppvh_result, sg_treediff_cache ** ppTreediffCache);
	void HandleStatusQuery(SG_vhash * message, const char * psz_wc_root, SG_vhash ** ppvh_result, sg_treediff_cache ** ppTreediffCache);
	void HandleRefreshWF(SG_vhash * pvhMessage, const char * psz_wc_root);
	void sg_cache_application::HandleCloseListeners(SG_vhash * pvhMessage, const char * psz_wc_root);
	//void HandleHeadCountQuery(SG_vhash * message, SG_vhash ** ppvh_result, sg_treediff_cache ** ppTreediffCache);

	wxTimer m_cShellNotifyTimer;

protected:
	void OnTimerFired(wxTimerEvent& e);

}; 

DECLARE_APP(sg_cache_application);

const wxEventType ReadOneMessageEventType = wxNewEventType();

class ReadOneMessageEvent : public wxEvent
{
public: 

	ReadOneMessageEvent(wxEventType commandType = ReadOneMessageEventType, int id = 0)
		: wxEvent(id, commandType)
	{
	}

	wxEvent* Clone() const { return new ReadOneMessageEvent(*this); }	
};

typedef void (wxEvtHandler::*ReadOneMessageEventFunction)(ReadOneMessageEvent &);
 
// This #define simplifies the one below, and makes the syntax less
// ugly if you want to use Connect() instead of an event table.
#define ReadOneMessageEventHandler(func)                                         \
	(wxObjectEventFunction)(wxEventFunction)(wxEventFunction)\
	wxStaticCastEvent(ReadOneMessageEventFunction, &func)                    
 
// Define the event table entry. Yes, it really *does* end in a comma.
#define EVT_READONEMESSAGE(id, fn)                                            \
	DECLARE_EVENT_TABLE_ENTRY( ReadOneMessageEvent, id, wxID_ANY,  \
	(wxObjectEventFunction)(wxEventFunction)                     \
	(wxEventFunction) wxStaticCastEvent(                  \
	ReadOneMessageEventFunction, &fn ), (wxObject*) NULL ),


const wxEventType WaitForNextClientEventType = wxNewEventType();

class WaitForNextClientEvent : public wxEvent
{
public: 

	WaitForNextClientEvent(wxEventType commandType = WaitForNextClientEventType, int id = 0)
		: wxEvent(id, commandType)
	{
	}

	wxEvent* Clone() const { return new WaitForNextClientEvent(*this); }	
};


typedef void (wxEvtHandler::*WaitForNextClientEventFunction)(WaitForNextClientEvent &);
 
// This #define simplifies the one below, and makes the syntax less
// ugly if you want to use Connect() instead of an event table.
#define WaitForNextClientEventHandler(func)                                         \
	(wxObjectEventFunction)(wxEventFunction)(wxEventFunction)\
	wxStaticCastEvent(WaitForNextClientEventFunction, &func)                    
 
// Define the event table entry. Yes, it really *does* end in a comma.
#define EVT_WAITFORNEXTMESSAGE(id, fn)                                            \
	DECLARE_EVENT_TABLE_ENTRY( WaitForNextClientEvent, id, wxID_ANY,  \
	(wxObjectEventFunction)(wxEventFunction)                     \
	(wxEventFunction) wxStaticCastEvent(                  \
	WaitForNextClientEventFunction, &fn ), (wxObject*) NULL ),


class sg_wait_for_connection__thread : public wxThread
{
public:
	sg_wait_for_connection__thread(sg_cache_application * mainPtr) : wxThread(wxTHREAD_DETACHED)
	{
		m_pMain = mainPtr;
	}
	~sg_wait_for_connection__thread()
	{
		int tmp = 4;
		tmp;
	}
private:
	virtual wxThread::ExitCode Entry();

	sg_cache_application * m_pMain;
};
#endif
