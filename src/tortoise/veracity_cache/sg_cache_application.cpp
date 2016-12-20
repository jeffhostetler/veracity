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

#include "sg_cache_application.h"
#include "Shlobj.h"
/*
** This is the main entry point of the veracity_cache executable.
** It is a wxWidgets application, due to the fact that it enabled us
** to use the wxFileSystemWatcher class.  The purpose of the 
** veracity_cache executable is to keep a SG_treediff2 object in memory 
** (in a sg_treediff_cache object), and to answer the following 
** questions, that can be asked over a named pipe.
**
** 1.  Status. -- get the status of a file system object.
** 2.  Conflict. -- is a file system object currently in a "conflict" state.
** 3.  HeadCount. -- Are there multiple heads in the working folder.  
**					This command is deprecated.
**
** keeping the treediff in memory helps to answer the Status question quickly.
**
** In general, this app will wait for a client to connect whenever it gets a 
** WaitForNextClientEvent.  It will then try to read from the named pipe whenever
** it catches the ReadOneMessageEvent.  It will process that event 
** and return a result to the client in the OnReadOneMessage function.
**
** This app will also fire an event every 5 seconds to fire any pending notifications for 
** explorer.  
**
**
*/


/*
**
** Types
**
*/ 

/*
**
** Globals
**
*/
#define	WHATSMYNAME	"veracity_cache"
//This is the name that our pipe is going to use.

#if  DEBUG
#define g_szPipeName2 _T("\\\\.\\Pipe\\VeracityCacheNamedPipe_DEBUG")
#else
#define g_szPipeName2 _T("\\\\.\\Pipe\\VeracityCacheNamedPipe")
#endif
#define BUFFER_SIZE 1024 //1k


/*
 * Identification strings.
 *
 * TODO: Move these to the manifest so they show up in the EXE file correctly.
 */
static const char* gszAppName    = "sg_cache_application";
static const char* gszVendorName = "SourceGear";

/*
**
** Public Functions
**
*/

sg_cache_application::sg_cache_application()
{
	// set some application properties
	this->SetAppName(gszAppName);
	this->SetVendorName(gszVendorName);

	//Null the pointers, so that any NULLFREEs don't 
	//barf.
	pCtx = NULL;
	m_pLogPath = NULL;
	m_bExiting = SG_FALSE;
	m_connectorThread = NULL;
	//Set ourselves as the owner of the timer.
	m_cShellNotifyTimer.SetOwner(this);
}

// This configures the sglib logging
// The logging is directed to the console, and also to the veracity_cache-DATE.log
// file.
void sg_cache_application::SetupSGLogging()
{
	 //Log initialization:
	m_pLogPath           = NULL;
	char*                                 szLogPath          = NULL;
	char*                                 szLogLevel         = NULL;
	
	SG_uint32							  logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;

	//SG_log__init_context(pCtx);

	SG_zero(m_cLogStdData);
	SG_zero(m_cLogFileData);
	SG_zero(m_cLogFileWriterData);
	
	// find the appropriate log path
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_PATH, NULL, &szLogPath, NULL)  );
	if (szLogPath != NULL)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &m_pLogPath, szLogPath)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__LOG_DIRECTORY(pCtx, &m_pLogPath)  );
	}

#ifdef DEBUG
	szLogLevel = "verbose";
#else
	// get the configured log level
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_LEVEL, NULL, &szLogLevel, NULL)  );
#endif
	// register the stdout console logger
#if 0 
	//removed, since in the normal execution mode, noone will see any of the logging to
	//the console.  If this gets back in, look for the unregister command
	//in the OnExit function.
	SG_ERR_CHECK(  SG_log_console__set_defaults(pCtx, &m_cLogStdData)  );
	SG_ERR_CHECK(  SG_log_console__register(pCtx, &m_cLogStdData, SG_LOG__FLAG__HANDLER_TYPE__ALL)  );
#endif
	// register the file logger
	SG_ERR_CHECK(  SG_log_text__set_defaults(pCtx, &m_cLogFileData)  );
	m_cLogFileData.fWriter             = SG_log_text__writer__daily_path;
	m_cLogFileData.pWriterData         = &m_cLogFileWriterData;
	m_cLogFileData.szRegisterMessage   = NULL;
	m_cLogFileData.szUnregisterMessage = NULL;
	if (szLogLevel != NULL)
	{
		if (SG_stricmp(szLogLevel, "quiet") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__QUIET;
			m_cLogFileData.bLogVerboseOperations = SG_FALSE;
			m_cLogFileData.bLogVerboseValues     = SG_FALSE;
		}
		else if (SG_stricmp(szLogLevel, "normal") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__NORMAL;
			m_cLogFileData.bLogVerboseOperations = SG_FALSE;
			m_cLogFileData.bLogVerboseValues     = SG_FALSE;
		}
		else if (SG_stricmp(szLogLevel, "verbose") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;
			m_cLogFileData.szRegisterMessage   = "---- " WHATSMYNAME " started logging ----";
			m_cLogFileData.szUnregisterMessage = "---- " WHATSMYNAME " stopped logging ----";
		}
	}
	logFileFlags |= SG_LOG__FLAG__DETAILED_MESSAGES;
	SG_ERR_CHECK(  SG_log_text__writer__daily_path__set_defaults(pCtx, &m_cLogFileWriterData)  );
	m_cLogFileWriterData.bReopen          = SG_FALSE;
	m_cLogFileWriterData.pBasePath        = m_pLogPath;
	m_cLogFileWriterData.szFilenameFormat = "veracity_cache-%d-%02d-%02d.log";
	SG_ERR_CHECK(  SG_log_text__register(pCtx, &m_cLogFileData, NULL, logFileFlags)  );

fail:
#ifndef DEBUG
	SG_NULLFREE(pCtx, szLogLevel);
#endif
	SG_NULLFREE(pCtx, szLogPath);
	return;
}

#if defined(WINDOWS)
BOOL _GotConsoleCtrlMessage(DWORD fdwCtrlType)
{
	switch(fdwCtrlType)
	{
		case CTRL_BREAK_EVENT:
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
			::wxGetApp().ExitMainLoop();
			return TRUE;

		
		//case CTRL_LOGOFF_EVENT:
		//case CTRL_SHUTDOWN_EVENT:
		default:
			return FALSE;
	}
}
#endif
// Initialize the application
bool sg_cache_application::OnInit()
{
	// let the base class initialize
	if (!wxAppConsole::OnInit())
	{
		return false;
	}

	//This should let us recover gracefully if we are closed or killed.
#if defined(WINDOWS)
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE)_GotConsoleCtrlMessage, TRUE );
#endif
	//Now initialize a context, and sglib;
	SG_error err;
	err = SG_context__alloc(&pCtx);

	SG_lib__global_initialize(pCtx);

	//sglib logging
	SetupSGLogging();

	m_manager = new sg_read_dir_manager(pCtx);

	if (!CreateMyNamedPipe())
	{
		//We couldn't create the named pipe.  There's
		//probably already another instance of veracity_cache
		//running.
		OnExit();
		
		return false;
	}
	return true;
}

//Create the named pipe that will be used by the client.
bool sg_cache_application::CreateMyNamedPipe()
{
	m_hPipe = CreateNamedPipe( 
          g_szPipeName2,             // pipe name 
          PIPE_ACCESS_DUPLEX |       // read/write access 
		  FILE_FLAG_FIRST_PIPE_INSTANCE, //Prevent the possibility of multiple veracity_cache instances.
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          BUFFER_SIZE,              // output buffer size 
          BUFFER_SIZE,              // input buffer size 
          25000, // client time-out 
          NULL);                    // default security attribute 

     
     if (INVALID_HANDLE_VALUE == m_hPipe) 
     {
		 SG_log__report_error(pCtx, "Error occurred while " 
                 "creating the pipe: %d", GetLastError()); 
          return false;  //Error
     }
     else
     {
		 SG_log__report_verbose(pCtx, "CreateNamedPipe() was successful.");
     }

	 return true;
}

//Event fires when we're ready to listen for the next client connection
//It only launches the background thread to listen for the 
//client connection.
void sg_cache_application::OnWaitForNextClient(WaitForNextClientEvent& e)
{
	e;
	/*if (m_connectorThread != NULL)
	{
		//m_connectorThread->Delete();
		m_connectorThread = NULL;
	}*/
	
	m_connectorThread = new sg_wait_for_connection__thread(this);

	if (!m_connectorThread->IsAlive())
	{
		if (   m_connectorThread->Create() != wxTHREAD_NO_ERROR)
		{
			SG_log__report_error(pCtx, "Could not create the worker thread!");
			return;
		}
	}

    if (m_connectorThread->Run() != wxTHREAD_NO_ERROR)
    {
        SG_log__report_error(pCtx, "Could not run the worker thread!");
        return;
    }
}

wxThread::ExitCode sg_wait_for_connection__thread::Entry()
{
	SG_context * pCtx = NULL;
	SG_context__alloc(&pCtx);
	m_pMain->ConnectNextClient(pCtx);
	SG_CONTEXT_NULLFREE(pCtx);
	this->Exit();
	return (wxThread::ExitCode)0;
}
//The thread entry point for waiting for a message on the pipe in
//a background thread.
void sg_cache_application::ConnectNextClient(SG_context * pTheContext)
{
	//Wait for the client to connect

	DWORD lastError = ERROR_UNIDENTIFIED_ERROR;
	BOOL bClientConnected = false;
	{
		//Make sure that we are not trying to read and get a new connection
		//at the same time.
		wxCriticalSectionLocker lock(m_criticalSection);
		bClientConnected = ConnectNamedPipe(m_hPipe, NULL);
		lastError = GetLastError();
		if (lastError == ERROR_NO_DATA)
		{
			SG_log__report_error(pTheContext, "Got ERROR_NO_DATA from ConnectNamedPipe."); 
			DisconnectNamedPipe(m_hPipe);
			//Try to read another event.
			wxQueueEvent(this, new WaitForNextClientEvent());
		}
	}
     
	if (FALSE == bClientConnected && lastError != ERROR_PIPE_CONNECTED)
	{
		SG_log__report_error(pTheContext, "\nError occurred while calling ConnectNamedPipe " 
			"to the client: %d", lastError); 
		//Try to read another event.
		wxQueueEvent(this, new WaitForNextClientEvent());
	}
	else
	{
		SG_log__report_verbose(pTheContext, "ConnectNamedPipe() was successful.");
		wxQueueEvent(this, new ReadOneMessageEvent());
	}
}

//Event fires after a client has connected.
void sg_cache_application::OnReadOneMessage(ReadOneMessageEvent& e)
{
	e;

	SG_byte szBuffer[BUFFER_SIZE];
	const char * psz_wc_root = NULL;
	DWORD cbBytes;
	char * pResult = NULL;
	SG_vhash * pvhMessage = NULL;
	SG_vhash * pvhResult = NULL;
	SG_string * sgstrReturnMessage = NULL;
	SG_bool bHas = SG_FALSE;
	SG_pathname * pPathname = NULL;
	SG_pathname * pPathname_top = NULL;
	SG_string * pStrMessage = NULL;

	//This will lock the critical section througout this
	//function
	wxCriticalSectionLocker lock(m_criticalSection);
	
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrMessage)  );
	
	//Read client message
	BOOL bResult = ReadFile( 
		m_hPipe,                // handle to pipe 
		szBuffer,             // buffer to receive data 
		sizeof(szBuffer),     // size of buffer 
		&cbBytes,             // number of bytes read 
		NULL);                // not overlapped I/O 
		 
	if ( (!bResult) || (0 == cbBytes)) 
	{
		DWORD lastError = GetLastError();
		while (!bResult && lastError == ERROR_MORE_DATA)
		{
			SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pStrMessage, szBuffer, cbBytes)  );
			bResult = ReadFile( 
				m_hPipe,                // handle to pipe 
				szBuffer,             // buffer to receive data 
				sizeof(szBuffer),     // size of buffer 
				&cbBytes,             // number of bytes read 
				NULL);                // not overlapped I/O 
			if (!bResult)
				lastError = GetLastError();
			else
				lastError = ERROR_SUCCESS;
		}
		if (lastError == ERROR_PIPE_LISTENING)
		{
			//Noone is connected to the other end of the pipe.
			//SG_log__report_verbose(pCtx, "\nError occurred while reading " 
				//"from the client: %d", GetLastError()); 
			::wxMilliSleep(500);
			goto fail;
		}
		else if (lastError != ERROR_SUCCESS)
		{
			SG_log__report_error(pCtx, "\nError occurred while reading " 
				"from the client: %d", GetLastError()); 
			goto fail;
		}
	}
	else
	{
		SG_log__report_verbose(pCtx, "ReadFile() was successful.");
	}

	SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pStrMessage, szBuffer, cbBytes)  );
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "got client message", SG_LOG__FLAG__NONE)  );

	SG_log__report_verbose(pCtx, "Client sent the following message: %s", szBuffer);
	
	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhMessage, SG_string__sz(pStrMessage))  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhMessage, "root", &bHas)  );
	if (bHas == SG_TRUE)
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhMessage, "root", &psz_wc_root)  );
	else
	{
		const char * pszPath = NULL;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhMessage, "path", &bHas)  );
		if (bHas)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhMessage, "path", &pszPath)  );
			if (pszPath != NULL && *pszPath != 0)
			{
				SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathname, pszPath)  );
				SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPathname, &pPathname_top, NULL, NULL)  );
				psz_wc_root = SG_pathname__sz(pPathname_top);
			}
		}
	}

	//Explorer is repetitive with its queries, so make sure to see
	//if we just need to resend the last result.
	if (m_manager->IsRepeatQuery(pCtx, SG_string__sz(pStrMessage), psz_wc_root, &pResult))
	{
		SG_log__report_verbose(pCtx, "reusing previous result");
	}

	//It's not a repeat query.
	if (pResult == NULL)
	{
		sg_treediff_cache * pCacheObject = NULL;

		SG_ERR_CHECK(  HandleOneMessage(pvhMessage, psz_wc_root, &pvhResult, &pCacheObject)  );

		if (pvhResult)
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sgstrReturnMessage)  );
			SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhResult, sgstrReturnMessage)  );
			
			//Get result string ready to send to the client.
			SG_STRDUP(pCtx, SG_string__sz(sgstrReturnMessage), &pResult );
			if (pCacheObject != NULL)
			{
				SG_ERR_CHECK(  pCacheObject->RememberLastQuery(pCtx, SG_string__sz(pStrMessage), pResult)  );
			}
		}
	}

	if (pResult != NULL)
	{
		SG_log__report_verbose(pCtx, "Result is: %s", pResult);
		//Reply to client
		bResult = WriteFile( 
			m_hPipe,                // handle to pipe 
			pResult,             // buffer to write from 
			static_cast<DWORD>(strlen(pResult)+1),   // number of bytes to write, include the NULL 
			&cbBytes,             // number of bytes written 
			NULL);                // not overlapped I/O 
	
		if ( (!bResult) || (strlen(pResult)+1 != cbBytes))
		{
			SG_log__report_error(pCtx, "Error occurred while writing" 
					" to the client: %d", GetLastError()); 
			//CloseHandle(m_hPipe);
			this->ExitMainLoop();

		}
		else
		{
			if (FlushFileBuffers(m_hPipe))
			{
				DisconnectNamedPipe(m_hPipe);
			}
		}
		SG_NULLFREE(pCtx, pResult);
	}
	else
	{
		if (FlushFileBuffers(m_hPipe))
		{
			DisconnectNamedPipe(m_hPipe);
		}
	}
	
fail:
	SG_VHASH_NULLFREE(pCtx, pvhMessage);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_NULLFREE(pCtx, pResult);
	SG_STRING_NULLFREE(pCtx, sgstrReturnMessage);
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_top);
	SG_STRING_NULLFREE(pCtx, pStrMessage);
	SG_log__pop_operation(pCtx);
	
	if (SG_context__has_err(pCtx) == SG_TRUE)
	{
		SG_log__report_error__current_error(pCtx);
		SG_context__err_reset(pCtx);
	}
	//Try to read another message off the named pipe.
	if (m_bExiting == SG_FALSE)
		this->QueueEvent(new WaitForNextClientEvent());
}

void sg_cache_application::OnTimerFired(wxTimerEvent& e)
{
	e;
	m_manager->CloseTransactionsIfNecessary(pCtx);
	m_manager->NotifyShellIfNecessary(pCtx);
}

//Route the one message to the appropriate handler.
void sg_cache_application::HandleOneMessage(SG_vhash * pvhMessage, const char * psz_wc_root, SG_vhash ** ppvh_result, sg_treediff_cache ** ppTreediffCache)
{
	const char * commandString = NULL;
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhMessage, "command", &commandString)  );
	if (commandString != NULL)
	{
		if (strcmp(commandString, "status") == 0)
		{
			SG_ERR_CHECK(  HandleStatusQuery(pvhMessage, psz_wc_root, ppvh_result, ppTreediffCache)  );
		}
		else if (strcmp(commandString, "refreshWF") == 0)
		{
			SG_ERR_CHECK(  HandleRefreshWF(pvhMessage, psz_wc_root)  );
		}
		//Headcount is no longer used.
		//else if (strcmp(commandString, "getHeadCount") == 0)
		//{
		//	SG_ERR_CHECK(  HandleHeadCountQuery(pvhMessage, ppvh_result)  );
		//}
#if USE_FILESYSTEM_WATCHERS
		else if (strcmp(commandString, "closeListeners") == 0)
		{
			SG_ERR_CHECK(  HandleCloseListeners(pvhMessage, psz_wc_root)  );
			//m_bExiting = SG_TRUE;
			//this->ExitMainLoop();
		}
#endif
		else if (strcmp(commandString, "exit") == 0)
		{
			m_bExiting = SG_TRUE;
			this->ExitMainLoop();
		}
	}
fail:
	return;
}

//This is the function that actually uses the treediff cache.
void sg_cache_application::HandleRefreshWF(SG_vhash * pvhMessage, const char * psz_wc_root)
{
	wxFileName requestedPath;
	SG_pathname * pPathName = NULL;

	SG_bool bHasPaths = SG_FALSE;
	SG_varray * pva_paths = NULL;

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhMessage, "paths", &bHasPaths)  );
	if (bHasPaths)
	{
		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvhMessage, "paths", &pva_paths)  );
	}
	
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "refreshing working folder", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathName, psz_wc_root)  );

	m_manager->RefreshWF(pCtx, psz_wc_root, pva_paths);

fail:
	SG_log__pop_operation(pCtx);
	SG_PATHNAME_NULLFREE(pCtx, pPathName);

}


#if USE_FILESYSTEM_WATCHERS
//This is the function that actually uses the treediff cache.
void sg_cache_application::HandleCloseListeners(SG_vhash * pvhMessage, const char * psz_wc_root)
{
	wxFileName requestedPath;
	SG_pathname * pPathName = NULL;
	pvhMessage;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "closing listeners", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathName, psz_wc_root)  );

	m_manager->CloseListeners(pCtx, psz_wc_root);

fail:
	SG_log__pop_operation(pCtx);
	SG_PATHNAME_NULLFREE(pCtx, pPathName);

}
#endif

//This is the function that actually uses the treediff cache.
void sg_cache_application::HandleStatusQuery(SG_vhash * pvhMessage, const char * psz_wc_root, SG_vhash ** ppvh_result, sg_treediff_cache ** ppTreediffCache)
{
	SG_vhash * pvh_result = NULL;
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;

	const char * pszPath = NULL;
	wxFileName requestedPath;
	SG_pathname * pPathName = NULL;
	SG_bool * pbDirChanged = NULL;
	SG_bool bDirChanged = SG_FALSE;
	SG_bool bPerformDirStatus = SG_TRUE;
	
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhMessage, "path", &pszPath)  );
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "getting status", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "path", pszPath, SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_vhash__check__bool(pCtx, pvhMessage, "dir_status", &bPerformDirStatus)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathName, pszPath)  );
	if (bPerformDirStatus)
		pbDirChanged = &bDirChanged;
	m_manager->GetStatusForPath(pCtx, pPathName, psz_wc_root, &statusFlags, pbDirChanged, ppTreediffCache);

	SG_ERR_CHECK(  SG_log__set_value__int(pCtx, "status returned", statusFlags, SG_LOG__FLAG__NONE)  );
	
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "status", statusFlags)  );
	if (pbDirChanged != NULL)
		SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_result, "dir_changed", *pbDirChanged)  );
	SG_RETURN_AND_NULL(pvh_result, ppvh_result);
fail:
	SG_log__pop_operation(pCtx);
	SG_PATHNAME_NULLFREE(pCtx, pPathName);
	SG_VHASH_NULLFREE(pCtx, pvh_result);
}

/*
** This function exists to connect our event listeners, and queue the first
** call to read a message from the named pipe.
**/
void sg_cache_application::OnEventLoopEnter(wxEventLoopBase* evtLoop)
{
	evtLoop;
	this->Connect(ReadOneMessageEventType, ReadOneMessageEventHandler(sg_cache_application::OnReadOneMessage), NULL, this);
	this->Connect(WaitForNextClientEventType, WaitForNextClientEventHandler(sg_cache_application::OnWaitForNextClient), NULL, this);
	this->Connect(wxEVT_TIMER, wxTimerEventHandler(sg_cache_application::OnTimerFired));
	this->QueueEvent(new WaitForNextClientEvent());
	m_cShellNotifyTimer.Stop();
	m_cShellNotifyTimer.Start(500, wxTIMER_CONTINUOUS);
}


//Clean up on exit.
int sg_cache_application::OnExit()
{
	SG_log__report_info(pCtx, "exiting successfully"); 
	CloseHandle(m_hPipe);

	delete m_manager;

	SG_ERR_IGNORE(  SG_log_text__unregister(pCtx, &m_cLogFileData)  );
#if 0
	SG_ERR_IGNORE(  SG_log_console__unregister(pCtx, &m_cLogStdData)  );
#endif
	SG_PATHNAME_NULLFREE(pCtx, m_pLogPath); // must come after cLogFileData is unregistered, because it uses pLogPath

	SG_lib__global_cleanup(pCtx);
	SG_CONTEXT_NULLFREE(pCtx);
	//SG_log__global_cleanup();
	//At this point, we might still have a thread running ConnectNamedPipe, Just specifically exit the whole process, instead of waiting forever.
	if (m_connectorThread != NULL)
	{
		//The thread should clean up after itself.
		//delete m_connectorThread;
		//Exit();
	}

	//At this point, we might still have a thread running ConnectNamedPipe, Just specifically exit the whole process, instead of waiting forever.
	if (m_connectorThread != NULL)
	{
		//delete m_connectorThread;
		Exit();
	}

	return wxAppConsole::OnExit();
}
IMPLEMENT_APP_CONSOLE(sg_cache_application);
