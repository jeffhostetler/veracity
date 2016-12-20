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

void SG_area__list(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        SG_vhash** ppvh
        )
{
    SG_rbtree* prbLeaves = NULL;
	char* psz_hid_cs_leaf = NULL;
    SG_vhash* pvh_result = NULL;
    SG_varray* pva_fields = NULL;
    SG_varray* pva = NULL;

	SG_NULLARGCHECK_RETURN(ppvh);

	/* HACK: 

	Push asks for a list of areas in the destination repo before getting started in earnest.
	
	If you query a zing DAG with a hardwired template that doesn't yet exist, zing creates the DAG.

	When pushing into a truly empty repo, this results in creating the area DAG with a meaningless
	leaf which later needs to be merged with the incoming leaf.
	
	Changing zing's behavior in this case is the more thorough fix.  Until then, this looks to 
	see if the area DAG exists before querying it with zing, potentially creating a superfluous leaf. */
	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, SG_DAGNUM__AREAS, &prbLeaves)  );
	{
		SG_uint32 count = 0;
		SG_ERR_CHECK(  SG_rbtree__count(pCtx, prbLeaves, &count)  );
		SG_RBTREE_NULLFREE(pCtx, prbLeaves);
		if (!count)
		{
			*ppvh = NULL;
			return;
		}
	}
	
	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__AREAS, &psz_hid_cs_leaf)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );
    SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__AREAS, psz_hid_cs_leaf, "area", NULL, NULL, 0, 0, pva_fields, &pva)  );
    if (pva)
    {
        SG_uint32 count = 0;
        SG_uint32 i;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_result)  );

        for (i=0; i<count; i++)
        {
            SG_vhash* pvh = NULL;
            const char* psz_area_name = NULL;
            SG_int64 area_id = 0;

            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "name", &psz_area_name)  );
            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "id", &area_id)  );
            SG_ASSERT(area_id >= 0);
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, psz_area_name, area_id)  );
        }
    }

    *ppvh = pvh_result;
    pvh_result = NULL;

fail:
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_NULLFREE(pCtx, psz_hid_cs_leaf);
}

void SG_area__add(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_area_name,
    SG_uint64 area_id,
    SG_audit* pq
    )
{
    char* psz_hid_cs_leaf = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, pq, SG_DAGNUM__AREAS, &psz_hid_cs_leaf)  );

    /* start a changeset */
    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__AREAS, pq->who_szUserId, psz_hid_cs_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "area", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "area", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_area_name) );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "area", "id", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__int(pCtx, prec, pzfa, area_id) );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

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

