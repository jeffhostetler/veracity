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

struct _sg_tncache
{
    SG_repo* pRepo;
    SG_rbtree* prb;
};

void SG_tncache__alloc(
    SG_context* pCtx, 
    SG_repo* pRepo,
    SG_tncache** ppResult
    )
{
    SG_tncache* pcache = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, pcache)  );
    pcache->pRepo = pRepo;
    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &pcache->prb)  );

    *ppResult = pcache;
    pcache = NULL;

fail:
    SG_tncache__free(pCtx, pcache);
}

void SG_tncache__free(
        SG_context* pCtx,
        SG_tncache* pcache
        )
{
    if (!pcache)
    {
        return;
    }

    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pcache->prb, (SG_free_callback *)SG_treenode__free);
    SG_NULLFREE(pCtx, pcache);
}

void SG_tncache__load(
        SG_context* pCtx,
        SG_tncache* pcache,
        const char* psz_hid_treenode,
        SG_treenode** pptn
        )
{
    SG_bool b_found = SG_FALSE;

    SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pcache->prb, psz_hid_treenode, &b_found, (void**) pptn)  );
    if (!b_found)
    {
        SG_ERR_CHECK_RETURN(  SG_treenode__load_from_repo(pCtx, pcache->pRepo, psz_hid_treenode, pptn)  );
        SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx, pcache->prb, psz_hid_treenode, *pptn)  );
    }
}

