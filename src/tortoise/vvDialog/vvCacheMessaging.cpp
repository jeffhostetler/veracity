#include "precompiled.h"
#include "vvCacheMessaging.h"

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

vvCacheMessaging::vvCacheMessaging(void)
{
}


vvCacheMessaging::~vvCacheMessaging(void)
{
}

void vvCacheMessaging::RequestRefreshWC(SG_context * pCtx, const char * psz_WCRoot, SG_stringarray* psa_Specific_Paths)
{
	SG_vhash * pvh_message = NULL;
	SG_vhash * pvh_result = NULL;
	SG_pathname * pPathname = NULL;
	SG_varray * pva_paths = NULL;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx,  &pvh_message) );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathname, psz_WCRoot)  );
	if (psa_Specific_Paths)
	{
		SG_ERR_CHECK(  SG_varray__alloc__stringarray(pCtx, &pva_paths, psa_Specific_Paths)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_message, "paths", &pva_paths)  );
	}
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_message, "command", "refreshWF")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_message, "root", SG_pathname__sz(pPathname))  );
	
	SG_ERR_CHECK(  ExecuteCommand(pCtx, pvh_message, &pvh_result)  );
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_VHASH_NULLFREE(pCtx, pvh_message);
	SG_VHASH_NULLFREE(pCtx, pvh_result);
}

bool vvCacheMessaging::ExecuteCommand(SG_context * pCtx, SG_vhash * pvhMessage, SG_vhash ** ppvhResult)
{
	HANDLE hPipe;
	SG_pathname * pPathname = NULL;
	SG_bool bFailed = SG_FALSE;
	SG_UNUSED(ppvhResult);
	wxCriticalSectionLocker lock(m_critSec);
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
			bFailed = SG_TRUE;
			goto fail;
		 }
		 int triesLimit = 4;
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
          printf("\nCreateFile() was successful.");
     }
     
     //We are done connecting to the server pipe, 
     //we can start communicating with 
     //the server using ReadFile()/WriteFile() 
     //on handle - hPipe
     
	 SG_string * sgstringJSON = NULL;
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
          printf("\nWriteFile() was successful.");
     }
     
     //Read server response
	 /*char szBuffer2[BUFFER_SIZE];
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
     }*/
     
fail:
	 if (hPipe != NULL)
		CloseHandle(hPipe);
	 SG_PATHNAME_NULLFREE(pCtx, pPathname);
	 return bFailed == SG_FALSE;
	 
}
