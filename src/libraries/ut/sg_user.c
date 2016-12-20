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

// TODO minlength for who in audit template?

// commas are invalid in usernames because usernames are explicity supported as WIT Stamps,
// and WIT Stamps currently ban commas so that they can be stored in a comma-delimited list
#define SG_USER__BANNED_CHARS SG_VALIDATE__BANNED_CHARS ","
#define SG_USER__MIN_LENGTH 1u
#define SG_USER__MAX_LENGTH 256u

void SG_user__sanitize_username(
	SG_context*     pCtx,
	const char*     psz_username,
	char**          ppsz_sanitized
	)
{
	SG_string* pString_sanitized = NULL;

	SG_NULLARGCHECK(psz_username);
	SG_NULLARGCHECK(ppsz_sanitized);

	SG_ERR_CHECK(  SG_validate__sanitize__trim(pCtx, psz_username, SG_USER__MIN_LENGTH, SG_USER__MAX_LENGTH, SG_USER__BANNED_CHARS, SG_VALIDATE__RESULT__ALL, "_", "_", &pString_sanitized)  );
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pString_sanitized, (SG_byte**)ppsz_sanitized, NULL)  );

fail:
	SG_STRING_NULLFREE(pCtx, pString_sanitized);
	return;
}

void SG_user__uniqify_username(
	SG_context*     pCtx,
	const char*     psz_username,
	const SG_vhash* pvh_userlist,
	char**          ppsz_uniqified
	)
{
	SG_string* pString_unique = NULL;
	SG_bool    exists         = SG_FALSE;
	SG_uint32  suffix         = 0u;

	SG_NULLARGCHECK(psz_username);
	SG_NULLARGCHECK(pvh_userlist);
	SG_NULLARGCHECK(ppsz_uniqified);

	// start with the given username
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pString_unique, psz_username)  );

	// loop until we find a username that doesn't exist
	// we'll just append a number and keep incrementing it
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_userlist, SG_string__sz(pString_unique), &exists)  );
	while (exists != SG_FALSE)
	{
		SG_int_to_string_buffer sz_suffix;

		// free up the unique name from last iteration for reuse
		SG_STRING_NULLFREE(pCtx, pString_unique);

		// increment to a new numeric suffix and convert it to a string
		++suffix;
		SG_ERR_CHECK(  SG_uint64_to_sz(suffix, sz_suffix)  );

		// start again with the given username
		// and ensure that it has enough room for the suffix
		// without blowing the max username length
		SG_ERR_CHECK(  SG_validate__sanitize(pCtx, psz_username, 0u, SG_USER__MAX_LENGTH - SG_STRLEN(sz_suffix), NULL, SG_VALIDATE__RESULT__TOO_LONG, NULL, NULL, &pString_unique)  );

		// append the suffix string to create a hopefully unique username
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pString_unique, sz_suffix)  );

		// check if the newly constructed username exists
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_userlist, SG_string__sz(pString_unique), &exists)  );
	}

	// return our unique username
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pString_unique, (SG_byte**)ppsz_uniqified, NULL)  );

fail:
	SG_STRING_NULLFREE(pCtx, pString_unique);
	return;
}

void SG_user__set_user__repo(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_name
    )
{
    SG_vhash* pvh_user = NULL;
    char* psz_admin_id = NULL;
    const char* psz_userid = NULL;
    SG_string* pstr_path = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );

    if (pRepo)
    {
        SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, psz_name, &pvh_user)  );
        if (!pvh_user)
        {
            SG_ERR_THROW(  SG_ERR_USER_NOT_FOUND  );
        }

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, SG_ZING_FIELD__RECID, &psz_userid)  );

        if (0 == strcmp(psz_userid, SG_NOBODY__USERID))
        {
            SG_ERR_THROW(  SG_ERR_GOTTA_BE_SOMEBODY  );
        }

        SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &psz_admin_id)  );

        // we store this userid under the admin scope of the repo we were given
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s/%s",
                    SG_LOCALSETTING__SCOPE__ADMIN,
                    psz_admin_id,
                    SG_LOCALSETTING__USERID
                    )  );
        SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, SG_string__sz(pstr_path), psz_userid)  );
    }

    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s",
                SG_LOCALSETTING__SCOPE__MACHINE,
                SG_LOCALSETTING__USERNAME
                )  );
    SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, SG_string__sz(pstr_path), psz_name)  );

fail:
    SG_STRING_NULLFREE(pCtx, pstr_path);
    SG_NULLFREE(pCtx, psz_admin_id);
    SG_VHASH_NULLFREE(pCtx, pvh_user);
}

void SG_user__create_nobody(
	SG_context* pCtx,
    SG_repo* pRepo
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    SG_audit q;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record__force_recid(pCtx, pztx, "user", SG_NOBODY__USERID, &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, SG_NOBODY__USERNAME) );
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "prefix", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, SG_NOBODY__PREFIX) );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

void SG_user__create_force_recid(
	SG_context* pCtx,
    const char* psz_username,
    const char* psz_recid,
    SG_bool b_inactive,
    SG_repo* pRepo
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    SG_audit q;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record__force_recid(pCtx, pztx, "user", psz_recid, &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_username) );
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "inactive", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__bool(pCtx, prec, pzfa, b_inactive) );
   
    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
      if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
  
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

static void SG_user__create__specify_inactive(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username,
    SG_bool b_inactive,
    char** ppsz_userid
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    SG_audit q;
    const char* psz_recid = NULL;
    char* psz_userid = NULL;
    char* psz_username_trimmed = NULL;

    SG_ERR_CHECK(  SG_validate__ensure__trim(pCtx, psz_username, SG_USER__MIN_LENGTH, SG_USER__MAX_LENGTH, SG_USER__BANNED_CHARS, SG_TRUE, SG_ERR_INVALIDARG, "username", &psz_username_trimmed)  );

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "user", &prec)  );
    SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, prec, &psz_recid)  );
    SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_recid, &psz_userid)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_username_trimmed) );
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "inactive", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__bool(pCtx, prec, pzfa, b_inactive) );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    if (ppsz_userid)
    {
        *ppsz_userid = psz_userid;
        psz_userid = NULL;
    }

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
	SG_NULLFREE(pCtx, psz_username_trimmed);
    SG_NULLFREE(pCtx, psz_userid);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

void SG_user__create(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username,
    char** ppsz_userid
    )
{
    SG_ERR_CHECK(  SG_user__create__specify_inactive(pCtx, pRepo, psz_username, SG_FALSE, ppsz_userid)  );

fail:
    ;
}

void SG_user__create__inactive(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username,
    char** ppsz_userid
    )
{
    SG_ERR_CHECK(  SG_user__create__specify_inactive(pCtx, pRepo, psz_username, SG_TRUE, ppsz_userid)  );

fail:
    ;
}

void SG_user__rename(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_userid,
	const char* psz_username
	)
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    SG_audit q;
    char* psz_username_trimmed = NULL;

    SG_ERR_CHECK(  SG_validate__ensure__trim(pCtx, psz_username, SG_USER__MIN_LENGTH, SG_USER__MAX_LENGTH, SG_USER__BANNED_CHARS, SG_TRUE, SG_ERR_INVALIDARG, "username", &psz_username_trimmed)  );

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
    SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

    SG_ERR_CHECK(  SG_zingtx__get_record(pCtx, pztx, "user", psz_userid, &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_username_trimmed) );

    /* commit the changes */
    SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
	SG_NULLFREE(pCtx, psz_username_trimmed);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

void SG_user__get_username_for_repo(SG_context * pCtx, SG_repo* pRepo, char ** ppsz_username)
{
    SG_audit q;
    SG_vhash* pvh_user = NULL;
    const char* psz_username = NULL;

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );
    SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx, pRepo, &q, q.who_szUserId, &pvh_user)  );
    if (pvh_user)
    {
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, "name", &psz_username)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_username, ppsz_username)  );
    }
    else
    {
        *ppsz_username = NULL;
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_user);
}

void SG_user__lookup_by_userid(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_audit* pq,
    const char* psz_userid,
    SG_vhash** ppvh
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_user = NULL;
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, pq, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );

    SG_ERR_CHECK(  SG_zing__get_record__vhash__fields(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "user", psz_userid, pva_fields, &pvh_user)  );

    *ppvh = pvh_user;
    pvh_user = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VHASH_NULLFREE(pCtx, pvh_user);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
}

static void SG_user__lookup_name(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_hid_cs_leaf,
    const char* psz_name,
    char** ppsz
    )
{
    SG_vhash* pvh_user = NULL;
    SG_varray* pva_fields = NULL;
    const char* psz_recid = NULL;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );
    SG_ERR_CHECK(  SG_repo__dbndx__query__one(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "user", "name", psz_name, pva_fields, &pvh_user)  );

    if (pvh_user)
    {
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, "recid", &psz_recid)  );
    }

    if (psz_recid)
    {
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_recid, ppsz)  );
    }
    else
    {
        *ppsz = NULL;
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_user);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_user__lookup_by_name(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_audit* pq,
    const char* psz_name,
    SG_vhash** ppvh
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_user = NULL;
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, pq, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );
    SG_ERR_CHECK(  SG_repo__dbndx__query__one(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "user", "name", psz_name, pva_fields, &pvh_user)  );

    *ppvh = pvh_user;
    pvh_user = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VHASH_NULLFREE(pCtx, pvh_user);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
}

void SG_user__list_all(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_varray** ppva
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_varray* pva_users = NULL;
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );
    SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "user", NULL, "name #ASC", 0, 0, pva_fields, &pva_users)  );

    *ppva = pva_users;
    pva_users = NULL;

fail:
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_VARRAY_NULLFREE(pCtx, pva_users);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_user__list_active(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_varray** ppva
	)
{
	char* psz_hid_cs_leaf = NULL;
	SG_varray* pva_users = NULL;
	SG_varray* pva_fields = NULL;

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );
	SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "user", "((inactive==#FALSE) || (inactive==#NULL)) && (name!='nobody')", "name #ASC", 0, 0, pva_fields, &pva_users)  );

	*ppva = pva_users;
	pva_users = NULL;

fail:
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
	SG_VARRAY_NULLFREE(pCtx, pva_users);
	SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_group__create(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_name
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    SG_audit q;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "group", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "group", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_name) );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

static void sg_group__fields(
	SG_context* pCtx,
    SG_varray** ppva
    )
{
    SG_varray* pva_fields = NULL;
    SG_vhash* pvh_member_user = NULL;
    SG_varray* pva_user_fields = NULL;
    SG_vhash* pvh_member_subgroup = NULL;
    SG_varray* pva_group_fields = NULL;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__RECID)  );
    // TODO use #defines for all these

    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "name")  );

    SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_fields, &pvh_member_user)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_user, "type", "xref")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_user, "xref", "member_user")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_user, "xref_recid_alias", "xref_recid")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_user, "ref_to_me", "parent")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_user, "ref_to_other", "child")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_user, "alias", "users")  );
    SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_member_user, "fields", &pva_user_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_user_fields, SG_ZING_FIELD__RECID)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_user_fields, "name")  );

    SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_fields, &pvh_member_subgroup)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_subgroup, "type", "xref")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_subgroup, "xref", "member_group")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_subgroup, "xref_recid_alias", "xref_recid")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_subgroup, "ref_to_me", "parent")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_subgroup, "ref_to_other", "child")  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_member_subgroup, "alias", "subgroups")  );
    SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_member_subgroup, "fields", &pva_group_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_group_fields, SG_ZING_FIELD__RECID)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_group_fields, "name")  );

    *ppva = pva_fields;
    pva_fields = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_group__list(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_varray** ppva
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_varray* pva_result = NULL;
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  sg_group__fields(pCtx, &pva_fields)  );

    SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "group", NULL, "name #ASC", 0, 0, pva_fields, &pva_result)  );

    *ppva = pva_result;
    pva_result = NULL;

fail:
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_VARRAY_NULLFREE(pCtx, pva_result);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_group__list__vhash_by_name(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_vhash** ppvh_result
    )
{
    SG_varray* pva_all_groups = NULL;
    SG_vhash* pvh_result = NULL;
    SG_uint32 count_groups = 0;
    SG_uint32 i_group = 0;

    SG_ERR_CHECK(  SG_group__list(pCtx, pRepo, &pva_all_groups)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_all_groups, &count_groups)  );
    for (i_group=0; i_group<count_groups; i_group++)
    {
        const char* psz_group_name = NULL;

        SG_vhash* pvh_group__old = NULL;
        SG_vhash* pvh_group__new = NULL;

        SG_vhash* pvh_users__new = NULL;
        SG_vhash* pvh_subgroups__new = NULL;
        SG_varray* pva_users__old = NULL;
        SG_varray* pva_subgroups__old = NULL;

        SG_uint32 count_users = 0;
        SG_uint32 count_subgroups = 0;
        SG_uint32 i_user = 0;
        SG_uint32 i_subgroup = 0;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_all_groups, i_group, &pvh_group__old)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_group__old, "name", &psz_group_name)  );

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_result, psz_group_name, &pvh_group__new)  );

        // users
        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_group__old, "users", &pva_users__old)  );
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_group__new, "users", &pvh_users__new)  );
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_users__old, &count_users)  );
        for (i_user=0; i_user<count_users; i_user++)
        {
            SG_vhash* pvh_user__old = NULL;
            SG_vhash* pvh_user__new = NULL;
            const char* psz_name = NULL;
            const char* psz_recid = NULL;
            const char* psz_xref_recid = NULL;

            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_users__old, i_user, &pvh_user__old)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user__old, "name", &psz_name)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user__old, SG_ZING_FIELD__RECID, &psz_recid)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user__old, "xref_recid", &psz_xref_recid)  );
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_users__new, psz_name, &pvh_user__new)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_user__new, SG_ZING_FIELD__RECID, psz_recid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_user__new, "xref_recid", psz_xref_recid)  );
        }

        // subgroups
        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_group__old, "subgroups", &pva_subgroups__old)  );
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_group__new, "subgroups", &pvh_subgroups__new)  );
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_subgroups__old, &count_subgroups)  );
        for (i_subgroup=0; i_subgroup<count_subgroups; i_subgroup++)
        {
            SG_vhash* pvh_subgroup__old = NULL;
            SG_vhash* pvh_subgroup__new = NULL;
            const char* psz_name = NULL;
            const char* psz_recid = NULL;
            const char* psz_xref_recid = NULL;

            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_subgroups__old, i_subgroup, &pvh_subgroup__old)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_subgroup__old, "name", &psz_name)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_subgroup__old, SG_ZING_FIELD__RECID, &psz_recid)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_subgroup__old, "xref_recid", &psz_xref_recid)  );
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_subgroups__new, psz_name, &pvh_subgroup__new)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_subgroup__new, SG_ZING_FIELD__RECID, psz_recid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_subgroup__new, "xref_recid", psz_xref_recid)  );
        }
    }

    *ppvh_result = pvh_result;
    pvh_result = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VARRAY_NULLFREE(pCtx, pva_all_groups);
}

void SG_group__get_by_name(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_name,
    SG_vhash** ppvh
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_result = NULL;
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  sg_group__fields(pCtx, &pva_fields)  );

    SG_ERR_CHECK(  SG_repo__dbndx__query__one(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "group", "name", psz_name, pva_fields, &pvh_result)  );

    *ppvh = pvh_result;
    pvh_result = NULL;

fail:
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_group__get_by_recid(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_recid,
    SG_vhash** ppvh
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_result = NULL;
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  sg_group__fields(pCtx, &pva_fields)  );

    SG_ERR_CHECK(  SG_repo__dbndx__query__one(pCtx, pRepo, SG_DAGNUM__USERS, psz_hid_cs_leaf, "group", SG_ZING_FIELD__RECID, psz_recid, pva_fields, &pvh_result)  );

    *ppvh = pvh_result;
    pvh_result = NULL;

fail:
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_group__is_username_a_direct_member(
	SG_context* pCtx,
    SG_vhash* pvh_group,
    const char* psz_name,
    const char** ppsz_xref_recid
    )
{
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_varray* pva_users = NULL;
    const char* psz_xref_recid = NULL;

    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_group, "users", &pva_users)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_users, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh_one_user = NULL;
        const char* psz_username = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_users, i, &pvh_one_user)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_user, "name", &psz_username)  );
        if (0 == strcmp(psz_username, psz_name))
        {
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_user, "xref_recid", &psz_xref_recid)  );
            break;
        }
    }

    *ppsz_xref_recid = psz_xref_recid;

fail:
    ;
}

void SG_group__is_groupname_a_direct_member(
	SG_context* pCtx,
    SG_vhash* pvh_group,
    const char* psz_group_name,
    const char** ppsz_xref_recid
    )
{
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_varray* pva_members = NULL;
    const char* psz_xref_recid = NULL;

    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_group, "groups", &pva_members)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_members, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh_one_member = NULL;
        const char* psz_name = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_members, i, &pvh_one_member)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_member, "name", &psz_name)  );
        if (0 == strcmp(psz_name, psz_group_name))
        {
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_member, "xref_recid", &psz_xref_recid)  );
            break;
        }
    }

    *ppsz_xref_recid = psz_xref_recid;

fail:
    ;
}

void SG_group__is_userid_a_direct_member(
	SG_context* pCtx,
    SG_vhash* pvh_group,
    const char* psz_userid,
    SG_bool* pb
    )
{
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_varray* pva_users = NULL;

    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_group, "users", &pva_users)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_users, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh_one_user = NULL;
        const char* psz_recid = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_users, i, &pvh_one_user)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_user, SG_ZING_FIELD__RECID, &psz_recid)  );
        if (0 == strcmp(psz_recid, psz_userid))
        {
            *pb = SG_TRUE;
            goto done;
        }
    }

    *pb = SG_FALSE;

done:
fail:
    ;
}

void SG_group__add_users(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    )
{
    SG_uint32 i = 0;
    char* psz_recid_user = NULL;
    char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_group = NULL;
    SG_zingtx* pztx = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_audit q;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa_parent = NULL;
    SG_zingfieldattributes* pzfa_child = NULL;
    const char* psz_recid_group = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_group__get_by_name(pCtx, pRepo, psz_group_name, &pvh_group)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_group, SG_ZING_FIELD__RECID, &psz_recid_group)  );

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    SG_ERR_CHECK(  SG_zing__get_cached_template__static_dagnum(pCtx, SG_DAGNUM__USERS, &pzt)  );
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "member_user", "parent", &pzfa_parent)  );
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "member_user", "child", &pzfa_child)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );

    for (i=0; i<count_names; i++)
    {
        // lookup the recid of the user
        SG_ERR_CHECK(  SG_user__lookup_name(pCtx, pRepo, psz_hid_cs_leaf, paszMemberNames[i], &psz_recid_user)  );
        if (psz_recid_user)
        {
            SG_bool b_already = SG_FALSE;

            SG_ERR_CHECK(  SG_group__is_userid_a_direct_member(pCtx, pvh_group, psz_recid_user, &b_already)  );
            if (!b_already)
            {
                SG_zingrecord* prec = NULL;

                SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "member_user", &prec)  );

                SG_ERR_CHECK(  SG_zingrecord__set_field__reference(pCtx, prec, pzfa_parent, psz_recid_group) );
                SG_ERR_CHECK(  SG_zingrecord__set_field__reference(pCtx, prec, pzfa_child, psz_recid_user) );
            }
            SG_NULLFREE(pCtx, psz_recid_user);
        }
        else
        {
            SG_ERR_THROW(  SG_ERR_NOT_FOUND  );
        }
    }

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_NULLFREE(pCtx, psz_recid_user);
    SG_VHASH_NULLFREE(pCtx, pvh_group);
}

void SG_group__add_subgroups(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    )
{
    SG_vhash* pvh_all_groups_by_name = NULL;
    SG_uint32 i = 0;
    char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_group = NULL;
    SG_vhash* pvh_group__subgroups = NULL;
    SG_zingtx* pztx = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_audit q;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa_parent = NULL;
    SG_zingfieldattributes* pzfa_child = NULL;
    const char* psz_recid_group = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_group__list__vhash_by_name(pCtx, pRepo, &pvh_all_groups_by_name)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_all_groups_by_name, psz_group_name, &pvh_group)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_group, SG_ZING_FIELD__RECID, &psz_recid_group)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_group, "subgroups", &pvh_group__subgroups)  );

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    SG_ERR_CHECK(  SG_zing__get_cached_template__static_dagnum(pCtx, SG_DAGNUM__USERS, &pzt)  );
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "member_group", "parent", &pzfa_parent)  );
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "member_group", "child", &pzfa_child)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );

    for (i=0; i<count_names; i++)
    {
        // lookup the recid of the user
        SG_vhash* pvh_subgroup = NULL;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_all_groups_by_name, paszMemberNames[i], &pvh_subgroup)  );

        // TODO check for cycles

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_group__subgroups, paszMemberNames[i], &b_already)  );
        if (!b_already)
        {
            SG_zingrecord* prec = NULL;
            const char* psz_recid_subgroup = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_subgroup, SG_ZING_FIELD__RECID, &psz_recid_subgroup)  );
            SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "member_group", &prec)  );

            SG_ERR_CHECK(  SG_zingrecord__set_field__reference(pCtx, prec, pzfa_parent, psz_recid_group) );
            SG_ERR_CHECK(  SG_zingrecord__set_field__reference(pCtx, prec, pzfa_child, psz_recid_subgroup) );
        }
    }

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_VHASH_NULLFREE(pCtx, pvh_group);
    SG_VHASH_NULLFREE(pCtx, pvh_all_groups_by_name);
}

#define MY_MEMBER_TYPE__USER  (1)
#define MY_MEMBER_TYPE__GROUP (2)

static void sg_group__remove_stuff(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    SG_uint32 member_type,
    const char** paszMemberNames,
    SG_uint32 count_names
    )
{
    SG_uint32 i = 0;
    char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_group = NULL;
    SG_zingtx* pztx = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_audit q;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_group__get_by_name(pCtx, pRepo, psz_group_name, &pvh_group)  );

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );

    for (i=0; i<count_names; i++)
    {
        const char* psz_xref_recid = NULL;

        switch (member_type)
        {
            case MY_MEMBER_TYPE__USER:
            SG_ERR_CHECK(  SG_group__is_username_a_direct_member(pCtx, pvh_group, paszMemberNames[i], &psz_xref_recid)  );
            if (psz_xref_recid)
            {
                SG_ERR_CHECK(  SG_zingtx__delete_record(pCtx, pztx, "member_user", psz_xref_recid)  );
            }
            else
            {
                SG_ERR_THROW(  SG_ERR_NOT_FOUND  );
            }
            break;

            case MY_MEMBER_TYPE__GROUP:
            SG_ERR_CHECK(  SG_group__is_groupname_a_direct_member(pCtx, pvh_group, paszMemberNames[i], &psz_xref_recid)  );
            if (psz_xref_recid)
            {
                SG_ERR_CHECK(  SG_zingtx__delete_record(pCtx, pztx, "member_group", psz_xref_recid)  );
            }
            else
            {
                SG_ERR_THROW(  SG_ERR_NOT_FOUND  );
            }
            break;
        }
    }

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_VHASH_NULLFREE(pCtx, pvh_group);
}

void SG_group__remove_users(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    )
{
    SG_ERR_CHECK_RETURN(  sg_group__remove_stuff(pCtx, pRepo, psz_group_name, MY_MEMBER_TYPE__USER, paszMemberNames, count_names)  );
}

void SG_group__remove_subgroups(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    )
{
    SG_ERR_CHECK_RETURN(  sg_group__remove_stuff(pCtx, pRepo, psz_group_name, MY_MEMBER_TYPE__GROUP, paszMemberNames, count_names)  );
}

void sg_group__fetch_all_users(
	SG_context* pCtx,
    SG_vhash* pvh_all_groups_by_name,
    const char* psz_cur_group_name,
    SG_vhash* pvh_result
    )
{
    SG_uint32 count_users = 0;
    SG_uint32 count_subgroups = 0;
    SG_uint32 i = 0;
    SG_vhash* pvh_cur_group = NULL;
    SG_vhash* pvh_users = NULL;
    SG_vhash* pvh_subgroups = NULL;

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_all_groups_by_name, psz_cur_group_name, &pvh_cur_group)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cur_group, "users", &pvh_users)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cur_group, "subgroups", &pvh_subgroups)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_users, &count_users)  );
    for (i=0; i<count_users; i++)
    {
        const char* psz_user_name = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_users, i, &psz_user_name, NULL)  );
        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_result, psz_user_name)  );
    }

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_subgroups, &count_subgroups)  );
    for (i=0; i<count_subgroups; i++)
    {
        const char* psz_subgroup_name = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_subgroups, i, &psz_subgroup_name, NULL)  );
        SG_ERR_CHECK(  sg_group__fetch_all_users(pCtx, pvh_all_groups_by_name, psz_subgroup_name, pvh_result)  );
    }

fail:
    ;
}

void SG_group__list_all_users(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    SG_vhash** ppvh
    )
{
    SG_vhash* pvh = NULL;
    SG_vhash* pvh_all_groups_by_name = NULL;

    SG_ERR_CHECK(  SG_group__list__vhash_by_name(pCtx, pRepo, &pvh_all_groups_by_name)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    SG_ERR_CHECK(  sg_group__fetch_all_users(pCtx, pvh_all_groups_by_name, psz_group_name, pvh)  );

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_all_groups_by_name);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_user__set_active(
	SG_context *pCtx,
	SG_repo *pRepo,
	const char *psz_userid,
	SG_bool active)
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    SG_audit q;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW,  SG_AUDIT__WHO__FROM_SETTINGS)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__USERS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
    SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

    SG_ERR_CHECK(  SG_zingtx__get_record(pCtx, pztx, "user", psz_userid, &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "user", "inactive", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__bool(pCtx, prec, pzfa, ! active) );

    /* commit the changes */
    SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}


void SG_user__is_inactive(
	SG_context *pCtx,
	SG_vhash *user,
	SG_bool *inactive)
{
	SG_bool has = SG_FALSE;

	SG_NULLARGCHECK_RETURN(user);
	SG_NULLARGCHECK_RETURN(inactive);

	*inactive = SG_FALSE;

	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, user, "inactive", &has)  );

	if (has)
		SG_ERR_CHECK_RETURN(  SG_vhash__get__bool(pCtx, user, "inactive", inactive)  );
}
