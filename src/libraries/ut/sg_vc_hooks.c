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

void SG_vc_hooks__install(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_interface,
    const char* psz_js,
	const char *module,
	SG_uint32 version,
	SG_bool replaceOld,
    const SG_audit* pq
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
	SG_varray *oldRecs = NULL;

	SG_UNUSED(module);
	SG_UNUSED(version);

	if (replaceOld)
	{
		SG_ERR_CHECK(  SG_vc_hooks__lookup_by_interface(pCtx, pRepo, psz_interface, &oldRecs)  );
	}

    // TODO consider validating the JS by compiling it
    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_HOOKS, &psz_hid_cs_leaf)  );
    
    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_HOOKS, pq->who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );

	if (replaceOld)
	{
		SG_uint32 i, count;

		SG_ERR_CHECK(  SG_varray__count(pCtx, oldRecs, &count)  );

		for ( i = 0; i < count; ++i )
		{
			const char *hidrec = NULL;
			SG_vhash *rec = NULL;

			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, oldRecs, i, &rec)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, rec, "hidrec", &hidrec)  );
			SG_ERR_CHECK(  SG_zingtx__delete_record__hid(pCtx, pztx, "hook", hidrec)  );
		}
	}

	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "hook", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "hook", "interface", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_interface) );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "hook", "js", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_js) );

	if (module)
	{
		SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "hook", "module", &pzfa)  );
		SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, module) );
	}
	if (version)
	{
		SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "hook", "version", &pzfa)  );
		SG_ERR_CHECK(  SG_zingrecord__set_field__int(pCtx, prec, pzfa, (SG_int64)version) );
	}

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
	SG_VARRAY_NULLFREE(pCtx, oldRecs);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

void sg_vc_hooks__lookup_by_interface__single_result(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_interface, 
        SG_vhash** ppvh_latest_version
        )
{
	//This version will return only the hook with the largest version.
	//If multiple versions of the hook are defined,
	//all old versions will be deleted.
	SG_varray* pva_hooks = NULL;
	SG_vhash* pvh_latest_hook = NULL;
	SG_zingtx* pztx = NULL;
	char* psz_hid_cs_leaf = NULL;
	SG_dagnode* pdn = NULL;
	SG_changeset* pcs = NULL;

	SG_ERR_CHECK(  SG_vc_hooks__lookup_by_interface(
                pCtx, 
                pRepo, 
                psz_interface,
                &pva_hooks
                )  );

	if (!pva_hooks)
    {
        SG_ERR_THROW2(  SG_ERR_VC_HOOK_MISSING,
                (pCtx, "%s", psz_interface)
                );
    }
    else
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_hooks, &count)  );
        if (0 == count)
        {
            SG_ERR_THROW2(  SG_ERR_VC_HOOK_MISSING,
                    (pCtx, "%s", psz_interface)
                    );
        }
        else
        {
            if (count > 1)
            {
				SG_uint32 i  = 0;
				SG_int32 nVersion = 0;
				SG_int32 nHighestVersion = -1;
				SG_int32 nAmbiguousVersion = -2;
				const char * hidRecToSave = NULL;
				const char * hidrec = NULL;
				SG_vhash * pvh_current_hook = NULL;

                //There are multiple versions installed for this hook.
				//delete the lesser numbered versions.

				for (i=0; i < count; i++)
				{
					SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_hooks, i, &pvh_current_hook)   );
					SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_current_hook, "version", &nVersion)  );
					if (nVersion == nHighestVersion)
					{
						nAmbiguousVersion = nHighestVersion;
					}
					if (nVersion > nHighestVersion)
					{
						nHighestVersion = nVersion;
						SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_current_hook, "hidrec", &hidRecToSave)  );
					}
				}
				if (nAmbiguousVersion == nHighestVersion)
					SG_ERR_THROW2( SG_ERR_VC_HOOK_AMBIGUOUS, (pCtx, "%s defined multiple times at version %d", psz_interface, nHighestVersion) );
				if (nHighestVersion > 0 && hidRecToSave != NULL)
				{
					SG_audit q;
					SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

					SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_HOOKS, &psz_hid_cs_leaf)  );
    
					/* start a changeset */
					SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_HOOKS, q.who_szUserId, psz_hid_cs_leaf, &pztx)  );
					SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );

					for (i=0; i < count; i++)
					{
						SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_hooks, i, &pvh_current_hook)   );
						SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_current_hook, "hidrec", &hidrec)  );
						if (SG_strcmp__null(hidrec, hidRecToSave) != 0)
						{
							//This isn't the recid to save.  Delete it!
							SG_ERR_CHECK(  SG_zingtx__delete_record__hid(pCtx, pztx, "hook", hidrec)  );
						}
					}

					/* commit the changes */
					SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );
				}
            }
			else
			{
				//There's only one hook installed return it.
				SG_vhash * pvh_temp = NULL;
				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_hooks, 0, &pvh_temp)  );
				SG_ERR_CHECK( SG_VHASH__ALLOC__COPY(pCtx, &pvh_latest_hook, pvh_temp)  );
			}
		}
	}
	SG_RETURN_AND_NULL(pvh_latest_hook, ppvh_latest_version);

fail:
	if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_CHANGESET_NULLFREE(pCtx, pcs);
	SG_VARRAY_NULLFREE(pCtx, pva_hooks);

}
void SG_vc_hooks__lookup_by_interface(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_interface, 
        SG_varray** ppva
        )
{
    char* psz_hid_cs_leaf = NULL;
    SG_varray* pva_fields = NULL;
    SG_varray* pva = NULL;
    char buf_where[SG_HID_MAX_BUFFER_LENGTH + 64];
	char *iEscape = NULL;
	const char *ivar = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_HOOKS, &psz_hid_cs_leaf)  );

    SG_ASSERT(psz_hid_cs_leaf);

	SG_ERR_CHECK(  SG_sqlite__escape(pCtx, psz_interface, &iEscape)  );
	if (iEscape)
		ivar = iEscape;
	else
		ivar = psz_interface;

    SG_ERR_CHECK(  SG_sprintf(pCtx, buf_where, sizeof(buf_where), "interface == '%s'", ivar)  );
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "js")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "module")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "version")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__HIDREC)  );
    SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__VC_HOOKS, psz_hid_cs_leaf, "hook", buf_where, NULL, 0, 0, pva_fields, &pva)  );

    *ppva = pva;
    pva= NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
	SG_NULLFREE(pCtx, iEscape);
}


static JSBool x_print(
        JSContext *cx, 
        uintN argc, 
        jsval *argv
        )
{
    uintN i;
    JSString *str;
    char *bytes;

    for (i = 0; i < argc; i++) 
    {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
        {
            return JS_FALSE;
        }
        bytes = (char*)SG_ALLOC_FOREIGN( JS_EncodeString(cx, str) );
        if (!bytes)
        {
            return JS_FALSE;
        }
        printf("%s%s", i ? " " : "", bytes);
        JS_free(cx, SG_FREE_FOREIGN(bytes) );
    }

    printf("\n");
    fflush(stdout);

    return JS_TRUE;
}

static JSFunctionSpec global_functions[] = {
    JS_FS("print",          x_print,          0,0),
    JS_FS_END
};

void SG_vc_hooks__execute(
        SG_context* pCtx, 
        const char* psz_js,
        SG_vhash* pvh_params,
        SG_vhash** ppvh_result
        )
{
	jsval rval;
    JSContext* cx = NULL;
    JSObject* glob = NULL;
    SG_vhash* pvh_result = NULL;
    JSObject* jso_params = NULL;

	SG_NULLARGCHECK_RETURN(psz_js);

    SG_jscore__new_runtime(pCtx, NULL, global_functions, SG_FALSE, NULL);
    SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_ALREADY_INITIALIZED);
    SG_ERR_CHECK(  SG_jscore__new_context(pCtx, &cx, &glob, NULL)  );
	
	//This should tell the javascript engine how to report javascript errors to our jsglue.
	JS_SetErrorReporter(cx,SG_jsglue__error_reporter);

	if(!JS_EvaluateScript(cx, glob, psz_js, (uintN) strlen(psz_js), "hook_script", 1, &rval))
	{
		SG_ERR_CHECK_CURRENT;
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "An error occurred evaluating hook"));
	}

    // note that we don't care about the rval from eval of the script

	{
		jsval params;
		jsval rval;
		
        if (pvh_params)
        {
            jso_params = JS_NewObject(cx, NULL, NULL, NULL);
            if(!jso_params)
                SG_ERR_THROW2(SG_ERR_MALLOCFAILED, (pCtx, "Failed to create new JS object for params to vc hook."));
            params = OBJECT_TO_JSVAL(jso_params);
            SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_params, jso_params)  );
        }
        else
        {
            params = JSVAL_NULL;
        }

		if(!JS_CallFunctionName(cx, glob, "hook", 1, &params, &rval))
		{
			SG_ERR_CHECK_CURRENT;
			SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "An error occurred calling JS hook"));
		}

        if (JSVAL_IS_OBJECT(rval))
        {
            SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(rval), &pvh_result)  );
        }
	}

    if (ppvh_result)
    {
        *ppvh_result = pvh_result;
        pvh_result = NULL;
    }

fail:
    if (cx)
    {
		SG_ERR_IGNORE(  SG_jsglue__set_sg_context(NULL, cx)  );

		JS_EndRequest(cx);
		JS_DestroyContext(cx);

    }

    SG_VHASH_NULLFREE(pCtx, pvh_result);
}

// TODO not sure we really want to pass this much stuff to this interface
void SG_vc_hooks__ASK__WIT__ADD_ASSOCIATIONS(
    SG_context* pCtx, 
    SG_repo* pRepo, 
    SG_changeset* pcs,
    const char* psz_tied_branch_name,
    const SG_audit* pq,
    const char* psz_comment,
    const char* const* paszAssocs,
    SG_uint32 count_assocs,
    const SG_stringarray* psa_stamps
    )
{
    SG_vhash* pvh_hook = NULL;
    SG_vhash* pvh_params = NULL;
    SG_vhash* pvh_result = NULL;
    char* psz_repo_id = NULL;
    char* psz_admin_id = NULL;

    SG_ERR_CHECK(  sg_vc_hooks__lookup_by_interface__single_result(
                pCtx, 
                pRepo, 
                SG_VC_HOOK__INTERFACE__ASK__WIT__ADD_ASSOCIATIONS,
                &pvh_hook
                )  );

	if (pvh_hook)
    {
        const char* psz_js = NULL;
        SG_uint32 i = 0;
        SG_varray* pva_ids = NULL;
        const char* psz_descriptor_name = NULL;
        const char* psz_csid = NULL;
        SG_vhash* pvh_changeset = NULL;

        SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &psz_admin_id)  );
        SG_ERR_CHECK(  SG_repo__get_repo_id( pCtx, pRepo, &psz_repo_id )  );
        SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &psz_descriptor_name)  );
        SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs, &psz_csid)  );
        SG_ERR_CHECK(  SG_changeset__get_vhash_ref(pCtx, pcs, &pvh_changeset)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_hook, "js", &psz_js)  );

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_params)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "csid", psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "repo_id", psz_repo_id)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "admin_id", psz_admin_id)  );
        if (psz_descriptor_name)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "descriptor_name", psz_descriptor_name)  );
        }
        if (pq)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "userid", pq->who_szUserId)  );
        }
        if (psz_comment)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "comment", psz_comment)  );
        }
        if (psz_tied_branch_name)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "branch", psz_tied_branch_name)  );
        }

        SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvh_params, "changeset", pvh_changeset)  );

        SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_params, "wit_ids", &pva_ids)  );
        for (i=0; i<count_assocs; i++)
        {
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_ids, paszAssocs[i])  );
        }

        if (psa_stamps)
        {
            SG_uint32 count = 0;
            SG_uint32 i = 0;
            SG_varray* pva_stamps = NULL;

            SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_params, "stamps", &pva_stamps)  );
            SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_stamps, &count)  );
            for (i=0; i<count; i++)
            {
                const char* psz_stamp = NULL;

                SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_stamps, i, &psz_stamp)  );
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_stamps, psz_stamp)  );
            }
        }

        SG_ERR_CHECK(  SG_vc_hooks__execute(pCtx, psz_js, pvh_params, &pvh_result)  );
        // TODO process the result

		if (pvh_result)
		{
			SG_bool hasErrors = SG_FALSE;

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_result, "error", &hasErrors)  );

			if (hasErrors)
			{
				const char *emsg = NULL;

				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_result, "error", &emsg)  );

				SG_ERR_THROW2( SG_ERR_VC_HOOK_REFUSED, (pCtx, "\n:%s: %s", SG_VC_HOOK__INTERFACE__ASK__WIT__ADD_ASSOCIATIONS, emsg) );
			}
		}
    }
    
fail:
    SG_VHASH_NULLFREE(pCtx, pvh_params);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_hook);
	SG_NULLFREE(pCtx, psz_repo_id);
	SG_NULLFREE(pCtx, psz_admin_id);
}

void SG_vc_hooks__ASK__WIT__LIST_ITEMS(
    SG_context* pCtx, 
    SG_repo* pRepo,
	const char * psz_search_term,
	SG_varray *pBugs
	)
{
	SG_vhash* pvh_params = NULL;
    SG_vhash* pvh_result = NULL;
	SG_vhash* pvh_hook = NULL;
	const char* psz_js = NULL;
	const char* psz_descriptor_name = NULL;
	SG_bool hasBugs = SG_FALSE;

	SG_ERR_CHECK(  sg_vc_hooks__lookup_by_interface__single_result(
                pCtx, 
                pRepo, 
                SG_VC_HOOK__INTERFACE__ASK__WIT__LIST_ITEMS,
                &pvh_hook
                )  );

    if (!pvh_hook)
		return;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_hook, "js", &psz_js)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_params)  );

	

	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &psz_descriptor_name)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "descriptor_name", psz_descriptor_name)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "text", psz_search_term)  );

	SG_ERR_CHECK(  SG_vc_hooks__execute(pCtx, psz_js, pvh_params, &pvh_result)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_result, "items", &hasBugs)  );

	if (hasBugs && pBugs)
	{
		SG_varray *bugs = NULL;
		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_result, "items", &bugs)  );

		SG_ERR_CHECK(  SG_varray__copy_items(pCtx, bugs, pBugs)  );
	}
fail:
	SG_VHASH_NULLFREE(pCtx, pvh_params);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_hook);
}

// TODO not sure we really want to pass this much stuff to this interface
void SG_vc_hooks__ASK__WIT__VALIDATE_ASSOCIATIONS(
    SG_context* pCtx, 
    SG_repo* pRepo, 
    const char* const* paszAssocs,
    SG_uint32 count_assocs,
	SG_varray *pBugs
    )
{
    SG_vhash* pvh_params = NULL;
    SG_vhash* pvh_result = NULL;
    char* psz_repo_id = NULL;
    char* psz_admin_id = NULL;
	SG_vhash* pvh_hook = NULL;
	const char* psz_js = NULL;
	SG_uint32 i = 0;
	SG_varray* pva_ids = NULL;
	const char* psz_descriptor_name = NULL;

    SG_ERR_CHECK(  sg_vc_hooks__lookup_by_interface__single_result(
                pCtx, 
                pRepo, 
                SG_VC_HOOK__INTERFACE__ASK__WIT__VALIDATE_ASSOCIATIONS,
                &pvh_hook
                )  );

    if (!pvh_hook)
		return;

	SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &psz_admin_id)  );
	SG_ERR_CHECK(  SG_repo__get_repo_id( pCtx, pRepo, &psz_repo_id )  );
	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &psz_descriptor_name)  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_hook, "js", &psz_js)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_params)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "repo_id", psz_repo_id)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "admin_id", psz_admin_id)  );
	if (psz_descriptor_name)
	{
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "descriptor_name", psz_descriptor_name)  );
	}
	SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_params, "wit_ids", &pva_ids)  );
	for (i=0; i<count_assocs; i++)
	{
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_ids, paszAssocs[i])  );
	}

	SG_ERR_CHECK(  SG_vc_hooks__execute(pCtx, psz_js, pvh_params, &pvh_result)  );

	// TODO process the result
	if (pvh_result)
	{
		SG_bool hasErrors = SG_FALSE;
		SG_bool hasBugs = SG_FALSE;

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_result, "error", &hasErrors)  );

		if (hasErrors)
		{
			const char *emsg = NULL;

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_result, "error", &emsg)  );

			SG_ERR_THROW2( SG_ERR_VC_HOOK_REFUSED, (pCtx, "%s", emsg) );
		}

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_result, "bugs", &hasBugs)  );

		if (hasBugs && pBugs)
		{
			SG_varray *bugs = NULL;
			SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_result, "bugs", &bugs)  );

			SG_ERR_CHECK(  SG_varray__copy_items(pCtx, bugs, pBugs)  );
		}
	}

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_params);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_hook);
	SG_NULLFREE(pCtx, psz_repo_id);
	SG_NULLFREE(pCtx, psz_admin_id);
}

void SG_vc_hooks__BROADCAST__AFTER_COMMIT(
    SG_context* pCtx, 
    SG_repo* pRepo, 
    SG_changeset* pcs,
    const char* psz_tied_branch_name,
    const SG_audit* pq,
    const char* psz_comment,
    const char* const* paszAssocs,
    SG_uint32 count_assocs,
    const SG_stringarray* psa_stamps
    )
{
    SG_varray* pva_hooks = NULL;
    SG_vhash* pvh_params = NULL;
    char* psz_repo_id = NULL;
    char* psz_admin_id = NULL;

    SG_ERR_CHECK(  SG_vc_hooks__lookup_by_interface(
                pCtx, 
                pRepo, 
                SG_VC_HOOK__INTERFACE__BROADCAST__AFTER_COMMIT,
                &pva_hooks
                )  );
    if (pva_hooks)
    {
        SG_uint32 count_hooks = 0;
        SG_uint32 i_hook = 0;
        const char* psz_descriptor_name = NULL;

        SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &psz_admin_id)  );
        SG_ERR_CHECK(  SG_repo__get_repo_id( pCtx, pRepo, &psz_repo_id )  );
        SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &psz_descriptor_name)  );

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_hooks, &count_hooks)  );
        for (i_hook=0; i_hook<count_hooks; i_hook++)
        {
            SG_vhash* pvh_hook = NULL;
            const char* psz_js = NULL;
            const char* psz_csid = NULL;
            SG_vhash* pvh_changeset = NULL;

            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_hooks, i_hook, &pvh_hook)  );

            SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs, &psz_csid)  );
            SG_ERR_CHECK(  SG_changeset__get_vhash_ref(pCtx, pcs, &pvh_changeset)  );

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_hook, "js", &psz_js)  );

            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_params)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "csid", psz_csid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "repo_id", psz_repo_id)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "admin_id", psz_admin_id)  );
            if (psz_descriptor_name)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "descriptor_name", psz_descriptor_name)  );
            }
            if (pq)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "userid", pq->who_szUserId)  );
            }
            if (psz_comment)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "comment", psz_comment)  );
            }
            if (psz_tied_branch_name)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_params, "branch", psz_tied_branch_name)  );
            }

            SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvh_params, "changeset", pvh_changeset)  );

            if (paszAssocs && count_assocs)
            {
                SG_uint32 i = 0;
                SG_varray* pva_ids = NULL;

                SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_params, "wit_ids", &pva_ids)  );
                for (i=0; i<count_assocs; i++)
                {
                    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_ids, paszAssocs[i])  );
                }
            }

            if (psa_stamps)
            {
                SG_uint32 count = 0;
                SG_uint32 i = 0;
                SG_varray* pva_stamps = NULL;

                SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_params, "stamps", &pva_stamps)  );
                SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_stamps, &count)  );
                for (i=0; i<count; i++)
                {
                    const char* psz_stamp = NULL;

                    SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_stamps, i, &psz_stamp)  );
                    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_stamps, psz_stamp)  );
                }
            }

            SG_ERR_CHECK(  SG_vc_hooks__execute(pCtx, psz_js, pvh_params, NULL)  );
            SG_VHASH_NULLFREE(pCtx, pvh_params);
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_params);
    SG_VARRAY_NULLFREE(pCtx, pva_hooks);
	SG_NULLFREE(pCtx, psz_repo_id);
	SG_NULLFREE(pCtx, psz_admin_id);
}

