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
 *
 * @file sg_vv_verbs__upgrade.h
 *
 * @details Routines to perform most of 'vv upgrade'.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_VERBS__UPGRADE_H
#define H_SG_VV_VERBS__UPGRADE_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_vv_verbs__upgrade__find_old_repos( SG_context* pCtx,
        SG_vhash** ppvh)
{
 SG_vhash* pvh_repos = NULL;
    SG_vhash* pvh_old_repos = NULL;
    SG_uint32 count_repos = 0;
    SG_uint32 i = 0;
    SG_repo* pRepo = NULL;
    SG_varray* pva = NULL;
    SG_int64 t1 = -1;
    SG_int64 t2 = -1;

    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t2)  );
    t1 = t2 - (60*1000);

    SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pvh_repos)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_repos, &count_repos)  );
    for (i=0; i<count_repos; i++)
    {
        const char* psz_repo_name = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_repos, i, &psz_repo_name, NULL)  );

		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo_name, &pRepo)  );

        SG_repo__query_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, t1, t2, NULL, &pva);
        if (SG_context__err_equals(pCtx, SG_ERR_OLD_AUDITS))
        {
			SG_context__err_reset(pCtx);
            if (!pvh_old_repos)
            {
                SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_old_repos)  );
            }
            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_old_repos, psz_repo_name)  );
        }
        else
        {
			SG_ERR_CHECK_CURRENT;
		}
        SG_VARRAY_NULLFREE(pCtx, pva);

        SG_REPO_NULLFREE(pCtx, pRepo);
    }

    *ppvh = pvh_old_repos;
    pvh_old_repos = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_VHASH_NULLFREE(pCtx, pvh_old_repos);
    SG_VHASH_NULLFREE(pCtx, pvh_repos);
    SG_REPO_NULLFREE(pCtx, pRepo);	
}

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
static void _kill_veracity_cache(SG_context* pCtx)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE hProcess = NULL;
	SG_bool bFoundProc = SG_FALSE;
	
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_wcsicmp(entry.szExeFile, L"veracity_cache.exe") == 0)
			{
				bFoundProc = SG_TRUE;
				hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
				if (hProcess)
				{
					BOOL success = TerminateProcess(hProcess, 0);
					if (success)
						SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Successfully killed veracity_cache.exe.")  );
					else
						SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Failed to kill veracity_cache.exe: %d", GetLastError())  );
					CloseHandle(hProcess);
					hProcess = NULL;
				}
				else
				{
					SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Failed to retrieve veracity_cache.exe process handle", GetLastError())  );
				}
			}
		}
	}

	if (!bFoundProc)
		SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "No veracity_cache.exe process found.")  );

	/* fall through */
fail:
	if (hProcess)
		CloseHandle(hProcess);
}
#endif

void SG_vv_verbs__upgrade_repos(SG_context* pCtx,
        SG_vhash* pvh_old_repos)
{
	SG_uint32 count_repos = 0;
    SG_uint32 i = 0;
	SG_string * pstr_upgradingMessage = NULL;
	SG_bool bCurrentlyCloning = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_old_repos, &count_repos)  );
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Upgrading all repositories", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_repos, "repositories")  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_upgradingMessage)  );

	for (i=0; i<count_repos; i++)
    {
        const char* psz_old_repo_name = NULL;
        char buf_new_repo_name[256];

#if defined(WINDOWS)
		SG_ERR_CHECK(  _kill_veracity_cache(pCtx)  );
#endif
	
        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_old_repos, i, &psz_old_repo_name, NULL)  );
        SG_ERR_CHECK(  SG_sprintf(pCtx, buf_new_repo_name, sizeof(buf_new_repo_name), "upgraded_%s", psz_old_repo_name)  );

		SG_ERR_CHECK(  SG_string__clear(pCtx, pstr_upgradingMessage)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_upgradingMessage, "Upgrading %s...", psz_old_repo_name)  );
		//SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s", SG_string__sz(pstr_upgradingMessage))  );
		SG_ERR_CHECK(  SG_log__push_operation(pCtx, SG_string__sz(pstr_upgradingMessage), SG_LOG__FLAG__NONE)  );
        // TODO timers
        bCurrentlyCloning = SG_TRUE;    
        SG_ERR_CHECK(  SG_clone__to_local(
                    pCtx, 
                    psz_old_repo_name, 
                    NULL, 
                    NULL, 
                    buf_new_repo_name, 
                    NULL,
                    NULL,
                    NULL
                    )  );
        SG_ERR_CHECK(  SG_repo__delete_repo_instance(pCtx, psz_old_repo_name)  );
        SG_ERR_CHECK(  SG_closet__descriptors__remove(pCtx, psz_old_repo_name)  );
		SG_ERR_CHECK(  SG_closet__descriptors__rename(pCtx, buf_new_repo_name, psz_old_repo_name)  );
		SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
		bCurrentlyCloning = SG_FALSE;
		//SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "done\n")  );
		
    }
fail:
	if (bCurrentlyCloning == SG_TRUE)
	{
		SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
		SG_ERR_IGNORE(  SG_log__finish_step(pCtx)  );
	}
	SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
	SG_STRING_NULLFREE(pCtx, pstr_upgradingMessage);
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV_VERBS__upgrade_H
