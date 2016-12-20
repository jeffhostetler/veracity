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

#include <sg.h>
#include <signal.h>
#include "sg_typedefs.h"
#include "sg_cmd_util_prototypes.h"
#include "../libraries/mongoose/sg_mongoose.h"

static SG_bool shut_down_mg_flag = SG_FALSE;

void shut_down_mg(int SG_UNUSED_PARAM(sig))
{
	SG_UNUSED(sig);
	shut_down_mg_flag = SG_TRUE;
}

extern SG_bool _sg_uridispatch__debug_remote_shutdown;

void do_cmd_serve(SG_context * pCtx, SG_option_state* pOptSt, int * pExitStatus)
{
	struct mg_context * ctx = NULL;
	SG_repo *pRepo = NULL;
	char * psz_enable_diagnostics = NULL;
	SG_bool bEnableDiagnostics = SG_FALSE;

	SG_ASSERT(pExitStatus!=NULL);

	SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL);
	SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOT_A_WORKING_COPY);

	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_ENABLE_DIAGNOSTICS, pRepo, &psz_enable_diagnostics, NULL)  );
	bEnableDiagnostics = (psz_enable_diagnostics!=NULL && SG_stricmp(psz_enable_diagnostics, "true")==0);
	SG_NULLFREE(pCtx, psz_enable_diagnostics);
	SG_REPO_NULLFREE(pCtx, pRepo);

	if(!pOptSt->bPublic)
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Binding to localhost (127.0.0.1). Use '--public' option to allow remote connections.\n")  );

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Serving on port %d.\n", pOptSt->iPort)  );

	SG_ERR_CHECK(  SG_httprequestprofiler__global_init(pCtx)  );
	SG_ERR_CHECK(  mg_start(pCtx, pOptSt->bPublic, (int)pOptSt->iPort, &ctx)  );

	/* TODO: Consider using SetConsoleCtrlHandler on Windows to handle logoff/shutdown messages (K2421). */
	(void)signal(SIGINT,shut_down_mg);
	while(!shut_down_mg_flag && !(_sg_uridispatch__debug_remote_shutdown && bEnableDiagnostics))
		SG_sleep_ms(10);
	(void)signal(SIGINT,SIG_DFL);
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "Shutting down server...\n")  );

fail:
	if(ctx!=NULL)
	{
		if(SG_context__has_err(pCtx))
		{
			// On Windows mg_stop() seems to close the stderr stream. Print the error while we still can.
			SG_string * pStrContextDump = NULL;
			if(SG_ERR_OK==SG_context__err_to_string(pCtx, SG_TRUE, &pStrContextDump) && pStrContextDump!=NULL)
			{
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s", SG_string__sz(pStrContextDump))  );
				SG_STRING_NULLFREE(pCtx, pStrContextDump);
			}
			else
			{
				SG_error err;
				char bufErrorMessage[SG_ERROR_BUFFER_SIZE+1];
				SG_context__get_err(pCtx,&err);
				SG_error__get_message(err, SG_TRUE, bufErrorMessage, SG_ERROR_BUFFER_SIZE);
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s", bufErrorMessage)  );
			}
			SG_context__err_reset(pCtx);
			*pExitStatus = 1;
		}
		
		SG_ERR_IGNORE(  mg_stop(pCtx, ctx)  );
	}
	SG_httprequestprofiler__global_cleanup();

	SG_REPO_NULLFREE(pCtx, pRepo);
}
