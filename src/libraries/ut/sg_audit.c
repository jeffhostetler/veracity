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

static void x_audit__init(
        SG_context* pCtx,
        SG_audit* pInfo,
        SG_repo* pRepo,
        SG_int64 itime,
        const char* psz_userid
        )
{
    char* psz = NULL;
    SG_vhash* pvh_user = NULL;

    SG_NULLARGCHECK( pInfo );

    memset(pInfo, 0, sizeof(SG_audit));

    if (itime < 0)
    {
        SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &pInfo->when_int64)  );
    }
    else
    {
        pInfo->when_int64 = itime;
    }

    if (psz_userid)
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, pInfo->who_szUserId, sizeof(pInfo->who_szUserId), psz_userid)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__USERID, pRepo, &psz, NULL)  );
        if (psz)
        {
            SG_bool b_valid = SG_FALSE;

            SG_ERR_CHECK(  SG_gid__verify_format(pCtx, psz, &b_valid)  );
            if (!b_valid)
            {
                SG_ERR_THROW(  SG_ERR_INVALID_USERID  );
            }
            {
                SG_audit q;

                SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &q.when_int64)  );
                SG_ERR_CHECK(  SG_strcpy(pCtx, q.who_szUserId, sizeof(q.who_szUserId), SG_NOBODY__USERID)  );
                SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx, pRepo, &q, psz, &pvh_user)  );
                if (pvh_user)
                {
                    SG_ERR_CHECK(  SG_strcpy(pCtx, pInfo->who_szUserId, sizeof(pInfo->who_szUserId), psz)  );
                }
            }
        }
        else
        {
            if (pRepo)
            {
                SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__USERNAME, pRepo, &psz, NULL)  );
                if (psz)
                {
                    SG_audit q;

                    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &q.when_int64)  );
                    SG_ERR_CHECK(  SG_strcpy(pCtx, q.who_szUserId, sizeof(q.who_szUserId), SG_NOBODY__USERID)  );

                    SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, &q, psz, &pvh_user)  );
                    if (pvh_user)
                    {
                        const char* psz_id = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, SG_ZING_FIELD__RECID, &psz_id)  );
                        SG_ERR_CHECK(  SG_strcpy(pCtx, pInfo->who_szUserId, sizeof(pInfo->who_szUserId), psz_id)  );
                    }
                }
            }
        }
    }

fail:
    SG_NULLFREE(pCtx, psz);
    SG_VHASH_NULLFREE(pCtx, pvh_user);
}

void SG_audit__init__maybe_nobody(
        SG_context* pCtx,
        SG_audit* pInfo,
        SG_repo* pRepo,
        SG_int64 itime,
        const char* psz_userid
        )
{
    SG_ERR_CHECK(  x_audit__init(pCtx, pInfo, pRepo, itime, psz_userid)  );

    if (!pInfo->who_szUserId[0])
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, pInfo->who_szUserId, sizeof(pInfo->who_szUserId), SG_NOBODY__USERID)  );
    }

fail:
    ;
}

void SG_audit__init__nobody(
	SG_context* pCtx,
	SG_audit* pInfo,
	SG_repo* pRepo,
	SG_int64 itime)
{
	SG_ERR_CHECK(  x_audit__init(pCtx, pInfo, pRepo, itime, "")  );
	SG_ERR_CHECK(  SG_strcpy(pCtx, pInfo->who_szUserId, sizeof(pInfo->who_szUserId), SG_NOBODY__USERID)  );

fail:
	;
}

void SG_audit__init(
        SG_context* pCtx,
        SG_audit* pInfo,
        SG_repo* pRepo,
        SG_int64 itime,
        const char* psz_userid
        )
{
    SG_ERR_CHECK(  x_audit__init(pCtx, pInfo, pRepo, itime, psz_userid)  );

    if (!pInfo->who_szUserId[0])
    {
        SG_ERR_THROW(  SG_ERR_NO_CURRENT_WHOAMI  );
    }

fail:
    ;
}

void SG_audit__copy(
        SG_context* pCtx,
        const SG_audit* p,
        SG_audit** pp
        )
{
    SG_audit* pq = NULL;

    SG_ERR_CHECK(  SG_alloc(pCtx, 1,sizeof(SG_audit),&pq)  );
    memcpy(pq, p, sizeof(SG_audit));

    *pp = pq;

fail:
    return;
}
//////////////////////////////////////////////////////////////////

void SG_audit__init__friendly__reponame(SG_context * pCtx,
										SG_audit * pInfo,
										const char * pszRepoName,
										const char * pszWhen,
										const char * pszWho)
{
	SG_repo * pRepo = NULL;

	SG_NONEMPTYCHECK_RETURN( pszRepoName );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  SG_audit__init__friendly(pCtx, pInfo, pRepo, pszWhen, pszWho)  );
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

/**
 * Populate an audit record.  This version allows the
 * inputs to be sloppy (user-supplied) and we try to
 * figure out what was given.
 *
 */
void SG_audit__init__friendly(SG_context * pCtx,
							  SG_audit * pInfo,
							  SG_repo * pRepo,
							  const char * pszWhen,		// formatted date or time-ms-since-epoch or null
							  const char * pszWho)		// user-name or user-id or null
{
	SG_int64 i64Date = SG_AUDIT__WHEN__NOW;
	const char * pszUserId = SG_AUDIT__WHO__FROM_SETTINGS;
	SG_vhash * pvhUser = NULL;

	SG_NULLARGCHECK_RETURN( pInfo );
	SG_NULLARGCHECK_RETURN( pRepo );
	// pszWhen is optional -- defaults to SG_AUDIT__WHEN__NOW
	// pszWho is optional  -- defaults to SG_AUDIT__WHO__FROM_SETTINGS

	if (pszWhen && *pszWhen)
	{
		SG_ERR_CHECK(  SG_time__parse__friendly(pCtx, pszWhen, &i64Date)  );
	}

	if (pszWho && *pszWho)
	{
		SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, pszWho, &pvhUser)  );
		if (!pvhUser)
			SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx, pRepo, NULL, pszWho, &pvhUser)  );
		if (!pvhUser)
			SG_ERR_THROW2(  SG_ERR_USER_NOT_FOUND,
							(pCtx, "The user '%s' does not exist in this repository.", pszWho)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhUser, "recid", &pszUserId)  );
	}
		
	SG_ERR_CHECK(  SG_audit__init(pCtx, pInfo, pRepo, i64Date, pszUserId)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhUser);
}

/**
 * Update the time on an Audit to NOW.
 * (As if you just now created it with __WHEN__NOW.)
 *
 */
void SG_audit__update_time_to_now(SG_context * pCtx,
								  SG_audit * pInfo)
{
	SG_NULLARGCHECK_RETURN( pInfo );

	SG_ERR_CHECK_RETURN(  SG_time__get_milliseconds_since_1970_utc(pCtx, &pInfo->when_int64)  );
}
