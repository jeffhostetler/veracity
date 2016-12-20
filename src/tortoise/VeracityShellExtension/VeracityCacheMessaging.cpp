#include "VeracityCacheMessaging.h"
#include "GlobalHelpers.h"
#if  DEBUG
#define g_szPipeName _T("\\\\.\\Pipe\\VeracityCacheNamedPipe_DEBUG")
#else
#define g_szPipeName _T("\\\\.\\Pipe\\VeracityCacheNamedPipe")
#endif
//Pipe name format - \\servername\pipe\pipename
//This pipe is for server on the same computer, 
//however, pipes can be used to
//connect to a remote server

#define BUFFER_SIZE 1024 //1k

VeracityCacheMessaging::VeracityCacheMessaging(void)
{
}


VeracityCacheMessaging::~VeracityCacheMessaging(void)
{
}

bool VeracityCacheMessaging::ExecuteCommand(SG_context * pCtx, SG_vhash * pvhMessage, SG_vhash ** ppvhResult)
{
	HANDLE hPipe;
	SG_pathname * pPathname = NULL;
	SG_bool bFailed = SG_FALSE;
	SG_string * sgstringJSON = NULL;
     CComCritSecLock<CComCriticalSection> lock(m_critSec);
     //Connect to the server pipe using CreateFile()

     hPipe = CreateFile( 
          g_szPipeName,   // pipe name 
          GENERIC_READ |  // read and write access 
          GENERIC_WRITE, 
          0,              // no sharing 
          NULL,           // default security attributes
          OPEN_EXISTING,  // opens existing pipe 
          0,              // default attributes 
          NULL);          // no template file 

     DWORD error = GetLastError();
	 if (INVALID_HANDLE_VALUE == hPipe && (error == ERROR_FILE_NOT_FOUND|| error == ERROR_PIPE_BUSY) ) 
     {
		 if (error == ERROR_FILE_NOT_FOUND)
		 {
			 //Start the cache process.
			 SG_ERR_CHECK(  GlobalHelpers::findSiblingFile(pCtx, "veracity_cache.exe", "veracity_cache", &pPathname)  );
			 SG_process_id procID;
			 SG_exec__exec_async__files(pCtx, SG_pathname__sz(pPathname), NULL, NULL, NULL, NULL, &procID);
			 if (SG_context__has_err(pCtx))
			 {
				 SG_log__report_error__current_error(pCtx);
				 SG_ERR_CHECK(  SG_context__err_reset(pCtx)  );
				 bFailed = SG_TRUE;
				 goto fail;
			 }
			 SG_ERR_CHECK(  SG_sleep_ms(500)  );
		 }
		 int triesLimit = 10; //Try for 10 seconds
		 DWORD error = GetLastError();
		 for (int triesSoFar = 0; triesSoFar < triesLimit; triesSoFar++)
		 {
			 if (WaitNamedPipe (g_szPipeName, 1000))
				 break;
			 error = GetLastError();
			 if (error != ERROR_FILE_NOT_FOUND && error != ERROR_INVALID_HANDLE)
				 break;
		 }
		 error = GetLastError();
			//The pipe finally came up.
			   hPipe = CreateFile( 
				  g_szPipeName,   // pipe name 
				  GENERIC_READ |  // read and write access 
				  GENERIC_WRITE, 
				  0,              // no sharing 
			 	  NULL,           // default security attributes
				  OPEN_EXISTING,  // opens existing pipe 
				  0,              // default attributes 
				  NULL);          // no template file 
	}
     
	 
	 if (INVALID_HANDLE_VALUE == hPipe)
	 {
		 DWORD error = GetLastError();
		  printf("\nError occurred while connecting" 
                 " to the server: %d", error); 
          bFailed = SG_TRUE;
		  goto fail;
	 }
	 else
     {
          //printf("\nCreateFile() was successful.");
     }
     
     //We are done connecting to the server pipe, 
     //we can start communicating with 
     //the server using ReadFile()/WriteFile() 
     //on handle - hPipe
     
	 SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sgstringJSON)  );
	 SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhMessage, sgstringJSON)  );
	 const char *szBuffer = SG_string__sz(sgstringJSON);
	 DWORD cbBytes;
     //Send the message to server
     BOOL bResult = WriteFile( 
          hPipe,                // handle to pipe 
          szBuffer,             // buffer to write from 
          static_cast<DWORD>(strlen(szBuffer)+1),   // number of bytes to write, include the NULL
          &cbBytes,             // number of bytes written 
          NULL);                // not overlapped I/O 
     
     if ( (!bResult) || (strlen(szBuffer)+1 != cbBytes))
     {
          printf("\nError occurred while writing" 
                 " to the server: %d", GetLastError()); 
          CloseHandle(hPipe);
		  hPipe = NULL;
          bFailed = SG_TRUE;
		  goto fail;
     }
     else
     {
          //printf("\nWriteFile() was successful.");
     }
     
     //Read server response
	 char szBuffer2[BUFFER_SIZE];
     bResult = ReadFile( 
          hPipe,                // handle to pipe 
          szBuffer2,             // buffer to receive data 
          sizeof(szBuffer2),     // size of buffer 
          &cbBytes,             // number of bytes read 
          NULL);                // not overlapped I/O 

     
     if ( (!bResult) || (0 == cbBytes)) 
     {
          printf("\nError occurred while reading" 
                 " from the server: %d", GetLastError()); 
          CloseHandle(hPipe);
		  hPipe = NULL;
		  bFailed = SG_TRUE;

     }
     else
     {
		 SG_vhash * pvh_result = NULL;
		 SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvh_result, szBuffer2)  );
		 SG_RETURN_AND_NULL(pvh_result, ppvhResult);
     }
     
	 SG_STRING_NULLFREE(pCtx, sgstringJSON);
fail:
	 if (hPipe != NULL)
		CloseHandle(hPipe);
	 SG_PATHNAME_NULLFREE(pCtx, pPathname);
	 SG_STRING_NULLFREE(pCtx, sgstringJSON);
	 return bFailed == SG_FALSE;
	 
}
