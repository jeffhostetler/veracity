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
 * @file sg_repo__bind_vtable.c
 *
 * @details Private parts of SG_repo related to binding the
 * REPO vtable.
 *
 * This file will contain the static declarations of all VTABLES
 * (and eventually manage the dynamic loading of other VTABLES)
 * to allow a repo to choose between multiple implementations
 * of the REPO interfaces.
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "sg_repo__private.h"

static SG_rbtree* g_prb_repo_vtables = NULL;

void sg_repo__bind_vtable(SG_context* pCtx, SG_repo * pRepo, const char * pszStorage)
{
    SG_uint32 count_vtables = 0;

	SG_NULLARGCHECK(pRepo);

	if (pRepo->p_vtable)			// can only be bound once
    {
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "pRepo->p_vtable is already bound"));
    }

	if (pRepo->pvh_descriptor)
    {
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "pRepo->pvh_descriptor is already bound"));
    }

    if (!g_prb_repo_vtables)
    {
		SG_ERR_THROW2(SG_ERR_UNKNOWN_STORAGE_IMPLEMENTATION, (pCtx, "There are no repo storage plugins installed"));
    }

    SG_ERR_CHECK(  SG_rbtree__count(pCtx, g_prb_repo_vtables, &count_vtables)  );

    if (0 == count_vtables)
    {
		SG_ERR_THROW2(SG_ERR_UNKNOWN_STORAGE_IMPLEMENTATION, (pCtx, "There are no repo storage plugins installed"));
    }

	if (!pszStorage || !*pszStorage)
    {
        if (1 == count_vtables)
        {
            SG_bool b = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, NULL, g_prb_repo_vtables, &b, NULL, (void**) &pRepo->p_vtable)  );
            SG_ASSERT(pRepo->p_vtable);
        }
        else
        {
            SG_ERR_THROW2(SG_ERR_UNKNOWN_STORAGE_IMPLEMENTATION, (pCtx, "Multiple repo storage plugins installed.  Must specify."));
        }
    }
    else
    {
        SG_bool b = SG_FALSE;

        SG_ERR_CHECK(  SG_rbtree__find(pCtx, g_prb_repo_vtables, pszStorage, &b, (void**) &pRepo->p_vtable)  );
        if (!b || !pRepo->p_vtable)
        {
            SG_ERR_THROW(SG_ERR_UNKNOWN_STORAGE_IMPLEMENTATION);
        }
    }

fail:
    ;
}

/*static*/ void sg_repo__unbind_vtable(SG_context* pCtx, SG_repo * pRepo)
{
	SG_NULLARGCHECK_RETURN(pRepo);

	// TODO if we bound the repo to a dynamically-allocated VTABLE,
	// TODO free it.  (may also need to ref-count the containing dll.)

	// whether static or dynamic, we then set the vtable pointers to null.

	pRepo->p_vtable = NULL;
}

//////////////////////////////////////////////////////////////////

void sg_repo__query_implementation__list_vtables(SG_context * pCtx, SG_vhash ** pp_vhash)
{
	SG_vhash * pvh = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_name = NULL;

	SG_ERR_CHECK_RETURN(  SG_VHASH__ALLOC(pCtx, &pvh)  );

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, g_prb_repo_vtables, &b, &psz_name, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh, psz_name)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_name, NULL)  );
    }

	*pp_vhash = pvh;
    pvh = NULL;

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_repo__install_implementation(
        SG_context* pCtx, 
        sg_repo__vtable* pvtable
        )
{
    if (!g_prb_repo_vtables)
    {
        SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx, &g_prb_repo_vtables)  );
    }

    SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx, g_prb_repo_vtables, pvtable->pszStorage, pvtable)  );
}

void SG_repo__free_implementation_plugin_list(
        SG_context* pCtx
        )
{
    // TODO do we need to call each one to free up dynamic resources somehow?

    SG_RBTREE_NULLFREE(pCtx, g_prb_repo_vtables);
}

