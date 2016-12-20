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

/**
 * @file    SG_thread.c
 * @details Implements the SG_thread module.
 */

#include <sg.h>
#if defined(MAC) || defined(LINUX)
#include <sys/syscall.h>
#endif

/*
**
** Public Functions
**
*/

void SG_thread__get_current_process(
	SG_context*    pCtx,
	SG_process_id* pProcess
	)
{
	SG_UNUSED(pCtx);

#if defined(WINDOWS)
	*pProcess = GetCurrentProcessId();
#endif
	
#if defined(MAC) || defined(LINUX)
	*pProcess = getpid();
#endif
}

void SG_thread__get_current_thread(
	SG_context*   pCtx,
	SG_thread_id* pThread
	)
{
	SG_UNUSED(pCtx);

#if defined(WINDOWS)
	*pThread = GetCurrentThreadId();
#endif
	
#if defined(MAC)
	*pThread = (pid_t) syscall(SYS_thread_selfid);
#endif

#if defined(LINUX)
	*pThread = (pid_t) syscall(SYS_gettid);
#endif
}

void SG_thread__threads_equal(
	SG_context*  pCtx,
	SG_thread_id cThread1,
	SG_thread_id cThread2,
	SG_bool*     pEqual
	)
{
	SG_UNUSED(pCtx);

	*pEqual = (cThread1 == cThread2) ? SG_TRUE : SG_FALSE;
}

void SG_thread__current_thread_equals(
	SG_context*  pCtx,
	SG_thread_id cThread,
	SG_bool*     pEqual
	)
{
	SG_thread_id cId;

	SG_ERR_CHECK(  SG_thread__get_current_thread(pCtx, &cId)  );
	SG_ERR_CHECK(  SG_thread__threads_equal(pCtx, cThread, cId, pEqual)  );

fail:
	return;
}

const char* SG_thread__sz(
	SG_context*  pCtx,
	SG_thread_id cThread,
	char*        pBuffer
	)
{
	SG_ERR_CHECK(  SG_sprintf(pCtx, pBuffer, SG_THREAD_TO_STRING_BUFFER_LENGTH, "%d", cThread)  );

fail:
	return pBuffer;
}
