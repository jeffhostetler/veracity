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
#include "sg_typedefs.h"
#include "sg_prototypes.h"

/**
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if the cwd's not inside a working copy.
 */
void SG_cmd_util__get_descriptor_name_from_cwd(SG_context* pCtx, SG_string** ppstrDescriptorName, SG_pathname** ppPathCwd)
{
	SG_pathname* pPathCwd = NULL;
	SG_string* pstrRepoDescriptorName = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathCwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathCwd)  );

	SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPathCwd, NULL, &pstrRepoDescriptorName, NULL)  );

	SG_RETURN_AND_NULL(pstrRepoDescriptorName, ppstrDescriptorName);
	SG_RETURN_AND_NULL(pPathCwd, ppPathCwd);

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
}

/**
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if no --repo was specified and the cwd's not inside a working copy.
 */
void SG_cmd_util__get_descriptor_name_from_options_or_cwd(SG_context* pCtx, SG_option_state* pOptSt, SG_string** ppstrDescriptorName, SG_pathname** ppPathCwd)
{
	SG_string* pstr = NULL;
	SG_pathname* pPathCwd = NULL;

	SG_NULLARGCHECK_RETURN(pOptSt);

	if (pOptSt->psz_repo)
		SG_STRING__ALLOC__SZ(pCtx, &pstr, pOptSt->psz_repo);
	else
		SG_ERR_CHECK(  SG_cmd_util__get_descriptor_name_from_cwd(pCtx, &pstr, &pPathCwd)  );

	SG_RETURN_AND_NULL(pstr, ppstrDescriptorName);
	SG_RETURN_AND_NULL(pPathCwd, ppPathCwd);

fail:
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
}

/**
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if no --repo was specified and the cwd's not inside a working copy.
 */
void SG_cmd_util__get_repo_from_options_or_cwd(SG_context* pCtx, SG_option_state* pOptSt, SG_repo** ppRepo, SG_pathname** ppPathCwd)
{
	SG_string* pstr = NULL;

	SG_ERR_CHECK(  SG_cmd_util__get_descriptor_name_from_options_or_cwd(pCtx, pOptSt, &pstr, ppPathCwd)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pstr), ppRepo)  );

fail:
	SG_STRING_NULLFREE(pCtx, pstr);
}

/**
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if the cwd's not inside a working copy.
 */
void SG_cmd_util__get_repo_from_cwd(SG_context* pCtx, SG_repo** ppRepo, SG_pathname** ppPathCwd)
{
	SG_string* pstrRepoDescriptorName = NULL;

	SG_ERR_CHECK(  SG_cmd_util__get_descriptor_name_from_cwd(pCtx, &pstrRepoDescriptorName, ppPathCwd)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pstrRepoDescriptorName), ppRepo)  );

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
}

static void _format_comment(SG_context* pCtx, SG_bool onlyIncludeFirstLine, const char* szLinePrefix, const char* szComment, char** ppszReturn)
{
	SG_bool bFoundLineBreak = SG_FALSE;
	SG_string* pstr = NULL;
	SG_uint32 lenPrefix = SG_STRLEN(szLinePrefix);
	SG_uint32 offset;

	{
		const char* pos;
		for (pos = szComment; *pos; pos++)
		{
			if (*pos == SG_CR || *pos == SG_LF)
			{
				bFoundLineBreak = SG_TRUE;
				break;
			}
		}
		if (!bFoundLineBreak)
			return;
		if(onlyIncludeFirstLine)
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &pstr, (const SG_byte*)szComment, (SG_uint32)(pos-szComment))  );
			SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszReturn, NULL)  );
			return;
		}
		offset = (SG_uint32)(pos-szComment);
	}

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstr, szComment)  );
	while (offset < SG_string__length_in_bytes(pstr))
	{
		SG_byte current;
		SG_ERR_CHECK(  SG_string__get_byte_l(pCtx, pstr, offset, &current)  );
		if (current == SG_CR)
		{
			SG_byte next;
			bFoundLineBreak = SG_TRUE;
			SG_ERR_CHECK(  SG_string__get_byte_l(pCtx, pstr, offset+1, &next)  );
			if (next != SG_LF)
			{
				// Mac format, lines end with \r only. Consoles will not advance a line.
				SG_ERR_CHECK(  SG_string__insert__sz(pCtx, pstr, offset+1, "\n")  );
			}
			offset++;
		}
		else if (current == SG_LF)
		{
			bFoundLineBreak = SG_TRUE;
		}
		
		offset++;

		if (bFoundLineBreak)
		{
			SG_ERR_CHECK(  SG_string__insert__sz(pCtx, pstr, offset, szLinePrefix)  );
			offset += lenPrefix;
			bFoundLineBreak = SG_FALSE;
		}
	}

	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszReturn, NULL)  );

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstr);
}

static void _dump_branch_name(
	SG_context* pCtx,
	SG_console_stream cs,
	const char* pszRefHid, 
	SG_bool bShowOnlyOpenBranchNames,
	const SG_vhash* pvhRefBranchValues,
	const SG_vhash* pvhRefClosedBranches)
{
	SG_vhash* pvhRefBranchNames = NULL;

	if (pvhRefBranchValues)
	{
		SG_bool b_has = SG_FALSE;

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranchValues, pszRefHid, &b_has)  );
		if (b_has)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefBranchValues, pszRefHid, &pvhRefBranchNames)  );
		}
	}

	if (pvhRefBranchNames)
	{
		SG_uint32 count_branch_names = 0;
		SG_uint32 i;

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefBranchNames, &count_branch_names)  );
		for (i=0; i<count_branch_names; i++)
		{
			const char* psz_branch_name = NULL;
			SG_bool bClosed = SG_FALSE;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefBranchNames, i, &psz_branch_name, NULL)  );

			if (pvhRefClosedBranches)
				SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefClosedBranches, psz_branch_name, &bClosed)  );

			if ( !bShowOnlyOpenBranchNames || (bShowOnlyOpenBranchNames && !bClosed) )
			{
				SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s:  %s%s\n", "branch", 
					psz_branch_name, bClosed ? " (closed)" : "")  );
			}
		}
	}

fail:
	;
}

void SG_cmd_util__dump_history_results(
	SG_context * pCtx, 
	SG_console_stream cs,
	SG_history_result* pHistResult, 
	SG_vhash* pvh_pile,
	SG_bool bShowOnlyOpenBranchNames,
	SG_bool bShowFullComments,
	SG_bool bHideRevnums)
{
	//print the information for each
	SG_bool bFound = (pHistResult != NULL);
	const char* currentInfoItem = NULL;
	SG_uint32 revno;
	SG_uint32 nCount = 0;
	SG_uint32 nIndex = 0;
	const char * pszTag = NULL;
	const char * pszComment = NULL;
	const char * pszStamp = NULL;
	const char * pszParent = NULL;
	SG_uint32 nResultCount = 0;
	SG_vhash* pvhRefBranchValues = NULL;
	SG_vhash* pvhRefClosedBranches = NULL;
	char* pszMyComment = NULL;

	if (pvh_pile)
	{
		SG_bool bHas = SG_FALSE;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile, "closed", &bHas)  );
		if (bHas)
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvhRefClosedBranches)  );

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "values", &pvhRefBranchValues)  );
	}

	SG_ERR_CHECK(  SG_history_result__count(pCtx, pHistResult, &nResultCount)  );
	while (nResultCount != 0 && bFound)
	{
		SG_ERR_CHECK(  SG_history_result__get_revno(pCtx, pHistResult, &revno)  );
		SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, pHistResult, &currentInfoItem)  );
		if(!bHideRevnums)
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\n\t%8s:  %d:%s\n", "revision", revno, currentInfoItem)  );
		else
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\n\t%8s:  %s\n", "revision", currentInfoItem)  );

		SG_ERR_CHECK(  _dump_branch_name(pCtx, cs, currentInfoItem, bShowOnlyOpenBranchNames, 
			pvhRefBranchValues, pvhRefClosedBranches)  );

		SG_ERR_CHECK(  SG_history_result__get_audit__count(pCtx, pHistResult, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_int64 itime = -1;
			char buf_time_formatted[256];
			const char * pszUser = NULL;

			SG_ERR_CHECK(  SG_history_result__get_audit__who(pCtx, pHistResult, nIndex, &pszUser)  );
			SG_ERR_CHECK(  SG_history_result__get_audit__when(pCtx, pHistResult, nIndex, &itime)  );
			SG_ERR_CHECK(  SG_time__format_local__i64(pCtx, itime, buf_time_formatted, sizeof(buf_time_formatted))  );
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s:  %s\n", "who", pszUser)  );
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s:  %s\n", "when", buf_time_formatted)  );
		}

		SG_ERR_CHECK(  SG_history_result__get_tag__count(pCtx, pHistResult, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_ERR_CHECK(  SG_history_result__get_tag__text(pCtx, pHistResult, nIndex, &pszTag)  );
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s:  %s\n", "tag", pszTag)  );
		}

		SG_ERR_CHECK(  SG_history_result__get_comment__count(pCtx, pHistResult, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_ERR_CHECK(  SG_history_result__get_comment__text(pCtx, pHistResult, nIndex, &pszComment)  );
			if (pszComment)
			{
				SG_ERR_CHECK(  _format_comment(pCtx, !bShowFullComments, "\t           ", pszComment, &pszMyComment)  );
				if (pszMyComment)
					pszComment = pszMyComment;
			}
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s:  %s\n", "comment", pszComment)  );
			SG_NULLFREE(pCtx, pszMyComment);
		}

		SG_ERR_CHECK(  SG_history_result__get_stamp__count(pCtx, pHistResult, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_ERR_CHECK(  SG_history_result__get_stamp__text(pCtx, pHistResult, nIndex, &pszStamp)  );
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s:  %s\n", "stamp", pszStamp)  );
		}

		SG_ERR_CHECK(  SG_history_result__get_parent__count(pCtx, pHistResult, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_ERR_CHECK(  SG_history_result__get_parent(pCtx, pHistResult, nIndex, &pszParent, &revno)  );
			SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s:  %d:%s\n", "parent", revno, pszParent)  );
		}

		SG_ERR_CHECK(  SG_history_result__next(pCtx, pHistResult, &bFound)  );
	}
fail:
	SG_NULLFREE(pCtx, pszMyComment);
}

void SG_cmd_util__dump_log(
	SG_context * pCtx, 
	SG_console_stream cs,
	SG_repo* pRepo, 
	const char* psz_hid_cs, 
	SG_vhash* pvhCleanPileOfBranches, 
	SG_bool bShowOnlyOpenBranchNames,
	SG_bool bShowFullComments)
{
	SG_history_result* pHistResult = NULL;
	SG_stringarray * psaHids = NULL;
	
	SG_STRINGARRAY__ALLOC(pCtx, &psaHids, 1);
	SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaHids, psz_hid_cs)  );
	SG_history__get_revision_details(pCtx, pRepo, psaHids, NULL, &pHistResult);

    if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
    {
		/* There's a branch that references a changeset that doesn't exist. Show what we can. */

		SG_vhash* pvhRefClosedBranches = NULL;
		SG_vhash* pvhRefBranchValues = NULL;

		SG_context__err_reset(pCtx);

		if (pvhCleanPileOfBranches)
		{
			SG_bool bHas = SG_FALSE;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhCleanPileOfBranches, "closed", &bHas)  );
			if (bHas)
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhCleanPileOfBranches, "closed", &pvhRefClosedBranches)  );

			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhCleanPileOfBranches, "values", &pvhRefBranchValues)  );
		}

		SG_ERR_CHECK(  SG_console(pCtx, cs, "\n\t%8s:  %s\n", "revision", psz_hid_cs)  );
		SG_ERR_CHECK(  _dump_branch_name(pCtx, cs, psz_hid_cs, bShowOnlyOpenBranchNames, 
			pvhRefBranchValues, pvhRefClosedBranches)  );
		SG_ERR_CHECK(  SG_console(pCtx, cs, "\t%8s   %s\n", "", "(not present in repository)")  );
    }
    else
    {
		SG_ERR_CHECK_CURRENT;
        SG_ERR_CHECK(  SG_cmd_util__dump_history_results(pCtx, cs, pHistResult, pvhCleanPileOfBranches, bShowOnlyOpenBranchNames, bShowFullComments, SG_FALSE)  );
    }

fail:
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHids);
}

struct _sg_cmd_util__dump_log__revspec__baton
{
	SG_console_stream cs;
	SG_vhash* pvhCleanPileOfBranches;
	SG_bool bShowOnlyOpenBranchNames;
	SG_bool bShowFullComments;
};
static SG_rev_spec_foreach_callback _sg_cmd_util__dump_log__revspec__cb;
static void _sg_cmd_util__dump_log__revspec__cb(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_rev_spec_type specType,
	const char* pszGiven,   /* The value as added to the SG_rev_spec. */
	const char* pszFullHid, /* The full HID of the looked-up spec. */
	void* ctx
	)
{
	struct _sg_cmd_util__dump_log__revspec__baton * b = (struct _sg_cmd_util__dump_log__revspec__baton *)ctx;
	SG_UNUSED(specType);
	SG_UNUSED(pszGiven);
	SG_ERR_CHECK_RETURN(  SG_cmd_util__dump_log(pCtx, b->cs, pRepo, pszFullHid, b->pvhCleanPileOfBranches, b->bShowOnlyOpenBranchNames, b->bShowFullComments)  );
}
void SG_cmd_util__dump_log__revspec(
	SG_context * pCtx, 
	SG_console_stream cs,
	SG_repo* pRepo, 
	SG_rev_spec* pRevSpec, 
	SG_vhash* pvhCleanPileOfBranches, 
	SG_bool bShowOnlyOpenBranchNames,
	SG_bool bShowFullComments)
{
	struct _sg_cmd_util__dump_log__revspec__baton baton;
	baton.cs=cs;
	baton.pvhCleanPileOfBranches=pvhCleanPileOfBranches;
	baton.bShowOnlyOpenBranchNames=bShowOnlyOpenBranchNames;
	baton.bShowFullComments=bShowFullComments;
	SG_ERR_CHECK_RETURN(  SG_rev_spec__foreach__repo(pCtx, pRepo, pRevSpec, _sg_cmd_util__dump_log__revspec__cb, &baton)  );
}

void SG_cmd_util__get_username_for_repo(
	SG_context *pCtx,
	const char *szRepoName,
	char **ppUsername
	)
{
	SG_string * pUsername = NULL;
	SG_repo * pRepo = NULL;
	char * psz_username = NULL;
	SG_curl * pCurl = NULL;
	SG_string * pUri = NULL;
	SG_string * pResponse = NULL;
	SG_int32 responseStatusCode = 0;
	SG_vhash * pRepoInfo = NULL;
	char * psz_userid = NULL;
	SG_varray * pUsers = NULL;

	SG_NULLARGCHECK_RETURN(ppUsername);

	if(!szRepoName)
	{
		// Look up username based on 'whoami' of repo associated with cwd.

		SG_ERR_IGNORE(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );
		if(pRepo)
			SG_ERR_IGNORE(  SG_user__get_username_for_repo(pCtx, pRepo, &psz_username)  );
		SG_REPO_NULLFREE(pCtx, pRepo);
	}
	else if(SG_sz__starts_with(szRepoName, "http://") || SG_sz__starts_with(szRepoName, "https://"))
	{
		// Look up username based on 'whoami' of admin id of remote repo.

		SG_ERR_CHECK(  SG_curl__alloc(pCtx, &pCurl)  );

		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pUri, szRepoName)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pUri, ".json")  );

		SG_ERR_CHECK(  SG_curl__reset(pCtx, pCurl)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_URL, SG_string__sz(pUri))  );

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pResponse)  );
		SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pCurl, pResponse)  );

		SG_ERR_CHECK(  SG_curl__perform(pCtx, pCurl)  );
		SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pCurl, CURLINFO_RESPONSE_CODE, &responseStatusCode)  );

		if(responseStatusCode==200)
		{
			const char * szAdminId = NULL;
			SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__STRING(pCtx, &pRepoInfo, pResponse)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pRepoInfo, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &szAdminId)  );

			SG_ERR_CHECK(  SG_string__clear(pCtx, pUri)  );
			SG_ERR_CHECK(  SG_string__append__format(pCtx, pUri, "/admin/%s/whoami/userid", szAdminId)  );
			SG_ERR_IGNORE(  SG_localsettings__get__sz(pCtx, SG_string__sz(pUri), NULL, &psz_userid, NULL)  );

			if(psz_userid)
			{
				// We now have the userid. Look up the username.


				SG_ERR_CHECK(  SG_string__clear(pCtx, pUri)  );
				SG_ERR_CHECK(  SG_string__append__format(pCtx, pUri, "%s/users.json", szRepoName)  );

				SG_ERR_CHECK(  SG_curl__reset(pCtx, pCurl)  );
				SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_URL, SG_string__sz(pUri))  );

				SG_ERR_CHECK(  SG_string__clear(pCtx, pResponse)  );
				SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pCurl, pResponse)  );

				SG_ERR_CHECK(  SG_curl__perform(pCtx, pCurl)  );
				SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pCurl, CURLINFO_RESPONSE_CODE, &responseStatusCode)  );

				if(responseStatusCode==200)
				{
					SG_uint32 i, nUsers;
					SG_ERR_CHECK(  SG_VARRAY__ALLOC__FROM_JSON__STRING(pCtx, &pUsers, pResponse)  );
					SG_ERR_CHECK(  SG_varray__count(pCtx, pUsers, &nUsers)  );
					for(i=0; i<nUsers; ++i)
					{
						SG_vhash * pUser = NULL;
						const char * psz_recid = NULL;
						SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pUsers, i, &pUser)  );
						SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, "recid", &psz_recid)  );
						if(!strcmp(psz_recid, psz_userid))
						{
							const char * psz_name = NULL;
							SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, "name", &psz_name)  );
							SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_name, &psz_username)  );
							break;
						}
					}
					SG_VARRAY_NULLFREE(pCtx, pUsers);
				}
				
				SG_NULLFREE(pCtx, psz_userid);
			}

			SG_VHASH_NULLFREE(pCtx, pRepoInfo);
		}

		SG_STRING_NULLFREE(pCtx, pResponse);
		SG_STRING_NULLFREE(pCtx, pUri);
		SG_CURL_NULLFREE(pCtx, pCurl);
	}
	else
	{
		// Look up username based on 'whoami' of repo provided.

		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, szRepoName, &pRepo)  );
		SG_ERR_IGNORE(  SG_user__get_username_for_repo(pCtx, pRepo, &psz_username)  );
		SG_REPO_NULLFREE(pCtx, pRepo);
	}

	*ppUsername = psz_username;

	return;
fail:
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, psz_username);
	SG_CURL_NULLFREE(pCtx, pCurl);
	SG_STRING_NULLFREE(pCtx, pUri);
	SG_STRING_NULLFREE(pCtx, pResponse);
	SG_VHASH_NULLFREE(pCtx, pRepoInfo);
	SG_NULLFREE(pCtx, psz_userid);
	SG_VARRAY_NULLFREE(pCtx, pUsers);
}

void SG_cmd_util__get_username_and_password(
	SG_context *pCtx,
	const char *szWhoami,
	SG_bool force_whoami,
	SG_bool bHadSavedCredentials,
	SG_uint32 kAttempt,
	SG_string **ppUsername,
	SG_string **ppPassword
	)
{
	SG_string * pUsername = NULL;
	SG_string * pPassword = NULL;

	SG_NULLARGCHECK_RETURN(ppPassword);
	SG_NULLARGCHECK_RETURN(ppUsername);

	if (kAttempt == 0)
	{
		if (bHadSavedCredentials)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR,
									  "\nAuthorization required.  Saved username/password not valid.\n")  );
		}
		else
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "\nAuthorization required.")  );
			if (SG_password__supported())
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, " Use --remember to save this password.")  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "\n")  );
		}
	}
	else if (kAttempt >= 3)
	{
		SG_ERR_THROW(  SG_ERR_AUTHORIZATION_TOO_MANY_ATTEMPTS  );
	}
	else
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "\nInvalid username or password. Please try again.\n")  );
	}

	if(szWhoami!=NULL && force_whoami)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pUsername, szWhoami)  );

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Enter password for %s: ", szWhoami)  );
		SG_ERR_CHECK(  SG_console__get_password(pCtx, &pPassword)  );
	}
	else
	{
		if(szWhoami)
		{
			SG_bool bAllWhitespace = SG_FALSE;
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Enter username [%s]: ", szWhoami)  );
			SG_ERR_CHECK(  SG_console__readline_stdin(pCtx, &pUsername)  );
			SG_ERR_CHECK(  SG_sz__is_all_whitespace(pCtx, SG_string__sz(pUsername), &bAllWhitespace)  );
			if(bAllWhitespace)
				SG_ERR_CHECK(  SG_string__set__sz(pCtx, pUsername, szWhoami)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Enter username: ")  );
			SG_ERR_CHECK(  SG_console__readline_stdin(pCtx, &pUsername)  );
		}

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Enter password: ")  );
		SG_ERR_CHECK(  SG_console__get_password(pCtx, &pPassword)  );
	}

	*ppUsername = pUsername;
	*ppPassword = pPassword;

	return;
fail:
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
}

void SG_cmd_util__set_password(SG_context * pCtx,
							   const char * pszSrcRepoSpec,
							   SG_string * pStringUsername,
							   SG_string * pStringPassword)
{
	if (SG_password__supported())
	{
		SG_password__set(pCtx, pszSrcRepoSpec, pStringUsername, pStringPassword);
		if (!SG_CONTEXT__HAS_ERR(pCtx))
			return;
		if (!SG_context__err_equals(pCtx, SG_ERR_NOTIMPLEMENTED))
			SG_ERR_RETHROW;
		SG_context__err_reset(pCtx);
	}

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR,
							  "Could not remember your password in the keyring.\n")  );

fail:
	return;
}
