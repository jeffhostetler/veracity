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
 * @file sg_vv_verbs__work_items.h
 *
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_VERBS__WORK_ITEMS_H
#define H_SG_VV_VERBS__WORK_ITEMS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_vv_verbs__work_items__text_search(SG_context * pCtx, const char * pszRepoDescriptor, const char * pszSearchString, SG_varray ** ppva_results)
{
	SG_repo * pRepo = NULL;
	SG_varray* pva_results = NULL;
	JSContext* cx = NULL;
    JSObject* glob = NULL;

	SG_NULLARGCHECK_RETURN(ppva_results);
	SG_jscore__new_runtime(pCtx, NULL, NULL, SG_FALSE, NULL);
    SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_ALREADY_INITIALIZED);
	SG_ERR_CHECK(  SG_jscore__new_context(pCtx, &cx, &glob, NULL)  );

	SG_ERR_CHECK(  SG_jscore__check_module_dags(pCtx, cx, JS_GetGlobalObject(cx), pszRepoDescriptor)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoDescriptor, &pRepo)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_results)  );
	
	SG_ERR_CHECK( SG_vc_hooks__ASK__WIT__LIST_ITEMS(pCtx, pRepo, pszSearchString, pva_results)  );

	SG_RETURN_AND_NULL(pva_results, ppva_results);
fail:
	if (cx)
	{
		SG_ERR_IGNORE(  SG_jsglue__set_sg_context(NULL, cx)  );

		JS_EndRequest(cx);
		JS_DestroyContext(cx);

	}
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VARRAY_NULLFREE(pCtx, pva_results);
}

END_EXTERN_C;

#endif//H_SG_VV_VERBS__WORK_ITEMS_H
