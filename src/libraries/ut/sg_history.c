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

//////////////////////////////////////////////////////////////////////////

#define HISTORY_RESULT_KEY__HISTORY			"history"
#define HISTORY_RESULT_KEY__CHILDREN		"children"
#define HISTORY_RESULT_KEY_PARENT_COUNT		"parent_count"
#define HISTORY_RESULT_KEY__REVNO			"revno"
#define HISTORY_RESULT_KEY__CS_HID			"changeset_id"
#define HISTORY_RESULT_KEY__GENERATION		"generation"
#define HISTORY_RESULT_KEY__PARENTS			"parents"
#define HISTORY_RESULT_KEY__PSEUDO_PARENTS	"pseudo_parents"
#define HISTORY_RESULT_KEY__PSEUDO_CHILDREN	"pseudo_children"

#define	HISTORY_RESULT_KEY__AUDITS			"audits"

#define	HISTORY_RESULT_KEY__TAGS			"tags"

#define	HISTORY_RESULT_KEY__STAMPS			"stamps"

#define	HISTORY_RESULT_KEY__COMMENTS		"comments"
#define	HISTORY_RESULT_KEY__COMMENTS_TEXT	"text"

#define	ZING_FIELD_NAME__TAG				"tag"
#define	ZING_FIELD_NAME__STAMP				"stamp"

//////////////////////////////////////////////////////////////////////////

//This is used to pass around caches and repos while calling back to fill in the history results details.
struct _fill_in_result_details_data
{
    // TODO revisit this
	SG_repo *		pRepo;
};

typedef struct
{
	SG_varray* pva;
	SG_uint32 idx;
} _history_result;

//////////////////////////////////////////////////////////////////////////

void _history_result_free(SG_context* pCtx, _history_result* pResult)
{
	if (!pResult)
		return;

	SG_VARRAY_NULLFREE(pCtx, pResult->pva);
	SG_NULLFREE(pCtx, pResult);
}

//////////////////////////////////////////////////////////////////////////

static void _sg_history__get_id_and_parents(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        SG_dagnode * pCurrentDagnode, 
        SG_vhash* pvh_changesetDescription
        )
{
    SG_vhash * pvh_parents = NULL;
    SG_vhash * pvh_parents_ref = NULL;
    const char* pszDagNodeHID = NULL;
	SG_uint32 revno;
	SG_int32 generation;
	SG_dagnode* pdnParent = NULL;
    SG_uint32 count_parents = 0;
    const char** parents = NULL;

    SG_UNUSED(pRepo);

    SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pCurrentDagnode, &pszDagNodeHID)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_changesetDescription, HISTORY_RESULT_KEY__CS_HID, pszDagNodeHID)  );

	SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pCurrentDagnode, &revno)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_changesetDescription, HISTORY_RESULT_KEY__REVNO, revno)  );

	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pCurrentDagnode, &generation)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_changesetDescription, HISTORY_RESULT_KEY__GENERATION, generation)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_parents)  );
    pvh_parents_ref = pvh_parents;
    SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_changesetDescription, HISTORY_RESULT_KEY__PARENTS, &pvh_parents)  );
    SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pCurrentDagnode, &count_parents, &parents)  );
    if (parents)
    {
        SG_uint32 i = 0;

        for (i=0; i<count_parents; i++)
        {
			SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, parents[i], &pdnParent)  );
			SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pdnParent, &revno)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_parents_ref, parents[i], revno)  );
			SG_DAGNODE_NULLFREE(pCtx, pdnParent);
        }
    }

	return;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_parents);

}

void _sg_history__inclusion_check__gid(
    SG_context * pCtx,
    SG_repo * pRepo,
	SG_bool bHideObjectMerges,
    const char* pszDagNodeHID,
    SG_stringarray * pStringArrayGIDs,
    SG_bool* pb
    )
{
	SG_changeset * pcs = NULL;
	SG_uint32 count_gids = 0;
	SG_uint32 i_gid = 0;
	const char* psz_gid = NULL;
	SG_bool b_include_this_changeset = SG_FALSE;

    SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, pszDagNodeHID, &pcs)  );
    SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayGIDs, &count_gids )  );

    for(i_gid = 0; i_gid < count_gids; i_gid++)
    {
        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringArrayGIDs, i_gid, &psz_gid)  );
		if (bHideObjectMerges)
			SG_ERR_CHECK(  SG_changeset__check_gid_change(pCtx, pcs, psz_gid, bHideObjectMerges, &b_include_this_changeset)  );
        if (b_include_this_changeset)
        {
            break;
        }
    }

    *pb = b_include_this_changeset;

fail:
	SG_CHANGESET_NULLFREE(pCtx, pcs);
}

static void _sg_history_check_dagnode_for_inclusion(SG_context * pCtx,
											 SG_repo * pRepo,
											 SG_dagnode * pCurrentDagnode,
											 SG_bool bHistoryOnRoot,
											 SG_bool bHideObjectMerges,
											 SG_stringarray * pStringArrayGIDs,
											 SG_ihash * pih_candidate_changesets,
											 SG_varray * pArrayReturnResults)
{
	SG_bool bIncludeChangeset = SG_FALSE;
	SG_bool bFoundItInCandidateList = SG_FALSE;

	const char * pszDagNodeHID = NULL;

	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pCurrentDagnode, &pszDagNodeHID)  );
	
	if (pih_candidate_changesets)
	{
		SG_ERR_CHECK(  SG_ihash__has(pCtx, pih_candidate_changesets, pszDagNodeHID, &bFoundItInCandidateList)  );
		//If the DagNode HID is not in the Candidate list, skip it.
		if (!bFoundItInCandidateList)
        {
			return;
        }
	}
	
	if (bHistoryOnRoot)
    {
		bIncludeChangeset = SG_TRUE;
    }
    else if (bHideObjectMerges == SG_FALSE)
	{
		bIncludeChangeset = SG_TRUE; //They want us to include this changeset, since it's 
	}
	else
	{
        SG_ERR_CHECK(  _sg_history__inclusion_check__gid(pCtx, pRepo, bHideObjectMerges, pszDagNodeHID, pStringArrayGIDs, &bIncludeChangeset)  );
	}

	if (bIncludeChangeset)
	{
        SG_vhash* pvh_changesetDescription = NULL;

		SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pArrayReturnResults, &pvh_changesetDescription)  );
		SG_ERR_CHECK(  _sg_history__get_id_and_parents(pCtx, pRepo, pCurrentDagnode, pvh_changesetDescription)  );
	}

fail:
    ;
}


static void sg_history__slice__one(
        SG_context* pCtx,
        SG_vector* pvec,
        const char* psz_field,
        SG_varray** ppva
        )
{
    SG_varray* pva = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    SG_ERR_CHECK(  SG_vector__length(pCtx, pvec, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh_orig = NULL;
        const char* psz_val = NULL;

        SG_ERR_CHECK(  SG_vector__get(pCtx, pvec, i, (void**) &pvh_orig)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_orig, psz_field, &psz_val)  );
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, psz_val)  );
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void sg_history__slice__audits(
        SG_context* pCtx,
        SG_vector* pvec,
        SG_varray** ppva
        )
{
    SG_varray* pva = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    SG_ERR_CHECK(  SG_vector__length(pCtx, pvec, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh_orig = NULL;

        SG_ERR_CHECK(  SG_vector__get(pCtx, pvec, i, (void**) &pvh_orig)  );
        // TODO why do we just make copies of this?
        SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pva, pvh_orig, NULL)  );
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void sg_history__comments(
        SG_context* pCtx,
        SG_vector* pvec,
        SG_varray** ppva
        )
{
    SG_varray* pva = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    SG_ERR_CHECK(  SG_vector__length(pCtx, pvec, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh_orig = NULL;

        SG_ERR_CHECK(  SG_vector__get(pCtx, pvec, i, (void**) &pvh_orig)  );

        SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pva, pvh_orig, NULL)  );
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void sg_history__details_for_one_changeset(
        SG_context* pCtx,
        SG_vhash * pCurrentHash,
        SG_rbtree* prb_all_audits,
        SG_rbtree* prb_all_stamps,
        SG_rbtree* prb_all_tags,
        SG_rbtree* prb_all_comments
        )
{
    SG_bool b_found = SG_FALSE;
    SG_vector* pvec = NULL;
    SG_varray * pva_audits = NULL;
    SG_varray * pva_tags = NULL;
    SG_varray * pva_stamps = NULL;
    SG_varray * pva_comments = NULL;
    const char* psz_csid = NULL;

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pCurrentHash, HISTORY_RESULT_KEY__CS_HID, &psz_csid)  );

    // -------- audits --------
    b_found = SG_FALSE;
    pvec = NULL;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_all_audits, psz_csid, &b_found, (void**) &pvec)  );
    if (pvec)
    {
        SG_ERR_CHECK(  sg_history__slice__audits(pCtx, pvec, &pva_audits)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_audits)  );
    }
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pCurrentHash, HISTORY_RESULT_KEY__AUDITS, &pva_audits)  );

    // -------- tags --------
    b_found = SG_FALSE;
    pvec = NULL;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_all_tags, psz_csid, &b_found, (void**) &pvec)  );
    if (pvec)
    {
        SG_ERR_CHECK(  sg_history__slice__one(pCtx, pvec, ZING_FIELD_NAME__TAG, &pva_tags)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_tags)  );
    }
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pCurrentHash, HISTORY_RESULT_KEY__TAGS, &pva_tags)  );

    // -------- stamps --------
    b_found = SG_FALSE;
    pvec = NULL;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_all_stamps, psz_csid, &b_found, (void**) &pvec)  );
    if (pvec)
    {
        SG_ERR_CHECK(  sg_history__slice__one(pCtx, pvec, ZING_FIELD_NAME__STAMP, &pva_stamps)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_stamps)  );
    }
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pCurrentHash, HISTORY_RESULT_KEY__STAMPS, &pva_stamps)  );

    // -------- comments --------
    b_found = SG_FALSE;
    pvec = NULL;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_all_comments, psz_csid, &b_found, (void**) &pvec)  );
    if (pvec)
    {
        SG_ERR_CHECK(  sg_history__comments(pCtx, pvec, &pva_comments)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_comments)  );
    }
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pCurrentHash, HISTORY_RESULT_KEY__COMMENTS, &pva_comments)  );

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_audits);
    SG_VARRAY_NULLFREE(pCtx, pva_tags);
    SG_VARRAY_NULLFREE(pCtx, pva_stamps);
    SG_VARRAY_NULLFREE(pCtx, pva_comments);
}

void SG_history__get_changeset_comments(SG_context* pCtx, SG_repo* pRepo, const char* pszDagNodeHID, SG_varray** ppvaComments)
{
	SG_varray* pvaComments = NULL;

	SG_ERR_CHECK(  SG_vc_comments__lookup(pCtx, pRepo, pszDagNodeHID, &pvaComments)  );

	*ppvaComments = pvaComments;
	pvaComments = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaComments);
}

void SG_history__get_children_description(SG_context* pCtx, SG_repo* pRepo, const char* pChangesetHid, SG_varray** ppvaChildren)
{
	SG_bool b = SG_FALSE;	
	const char * pszChildID = NULL;
	SG_dagnode* pdn = NULL;
	SG_uint32 crevno;
	SG_int32 cGeneration;
	SG_rbtree* prbChildren = NULL;
	SG_rbtree_iterator * pit = NULL;
	SG_varray * pvaChildren = NULL;
	SG_vhash* pvhChildren = NULL;
	SG_varray* pvaAudits = NULL;
	SG_varray* pvaAuditsResult = NULL;
	SG_varray* pvaComments = NULL;	
	SG_vhash* pvhAuditResult = NULL;
	SG_uint32 count_audits;
	SG_uint32 i;

	SG_ERR_CHECK(  SG_repo__fetch_dagnode_children(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pChangesetHid, &prbChildren)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbChildren, &b, &pszChildID, NULL)  );
	SG_VARRAY__ALLOC(pCtx, &pvaChildren);
	while (b)
	{	
		SG_VHASH__ALLOC(pCtx, &pvhChildren);
		SG_VARRAY__ALLOC(pCtx, &pvaAuditsResult);
		SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszChildID, &pdn)  );  
		SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pdn, &crevno)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhChildren, HISTORY_RESULT_KEY__REVNO, crevno)  );
		SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &cGeneration)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhChildren, HISTORY_RESULT_KEY__GENERATION, cGeneration)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChildren, HISTORY_RESULT_KEY__CS_HID, pszChildID)  );

		SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszChildID, &pvaAudits)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaAudits, &count_audits)  );
		for (i=0; i<count_audits; i++)
		{
			SG_vhash* pvh = NULL;  
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaAudits, i, &pvh)  );
			SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pvaAuditsResult, pvh, NULL)  );	
			
		}	
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhChildren, HISTORY_RESULT_KEY__AUDITS, &pvaAuditsResult)  );
		SG_VARRAY_NULLFREE(pCtx, pvaAudits);

		SG_ERR_CHECK(  SG_history__get_changeset_comments(pCtx, pRepo, pszChildID, &pvaComments)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhChildren, HISTORY_RESULT_KEY__COMMENTS, &pvaComments)  );		
		SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pvaChildren, &pvhChildren)  );
		SG_VHASH_NULLFREE(pCtx, pvhChildren);
		SG_DAGNODE_NULLFREE(pCtx, pdn);	
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &pszChildID, NULL)  );	
		
	}
	SG_RBTREE_NULLFREE(pCtx, prbChildren);

	*ppvaChildren = pvaChildren;
fail:
	SG_RBTREE_NULLFREE(pCtx, prbChildren);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_VHASH_NULLFREE(pCtx, pvhChildren);
	SG_VARRAY_NULLFREE(pCtx, pvaComments);
	SG_VARRAY_NULLFREE(pCtx, pvaAuditsResult);		
	SG_VARRAY_NULLFREE(pCtx, pvaAudits);
	SG_VHASH_NULLFREE(pCtx, pvhAuditResult);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}

void sg_history__get_parents_description(SG_context* pCtx, SG_repo* pRepo, const char* pChangesetHid, SG_vhash** ppvhParents)
{	  
    SG_uint32 count_parents = 0;
    const char** parents = NULL;
    SG_vhash * pvhParents = NULL;
	SG_vhash* pvhAuditResult = NULL;
	SG_uint32 count_audits;
	SG_varray* pvaAudits = NULL;
	SG_varray* pvaAuditsResult = NULL;
	SG_varray* pvaComments = NULL;			
	SG_vhash* pvh_result = NULL;
	SG_dagnode* pdn = NULL;
	

	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pChangesetHid, &pdn)  );
 
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhParents)  );    
   
    SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pdn, &count_parents, &parents)  );
	
    if (parents)
    {       
        SG_uint32 i_parent = 0;

        for (i_parent=0; i_parent<count_parents; i_parent++)
		{				
			SG_uint32 i; 
			SG_uint32 revno;
			SG_dagnode* pdnParent = NULL;

			SG_VHASH__ALLOC(pCtx, &pvh_result);
			SG_VARRAY__ALLOC(pCtx, &pvaAuditsResult);
				
			SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, parents[i_parent], &pdnParent)  );
			SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pdnParent, &revno)  );			
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, HISTORY_RESULT_KEY__REVNO, revno)  );
			SG_DAGNODE_NULLFREE(pCtx, pdnParent);

			SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, parents[i_parent], &pvaAudits)  );
			SG_ERR_CHECK(  SG_varray__count(pCtx, pvaAudits, &count_audits)  );
			for (i=0; i<count_audits; i++)
			{
				SG_vhash* pvh = NULL;  
				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaAudits, i, &pvh)  );
				SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pvaAuditsResult, pvh, NULL)  );	
			}	
			
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, HISTORY_RESULT_KEY__AUDITS, &pvaAuditsResult)  );
				
			SG_ERR_CHECK(  SG_history__get_changeset_comments(pCtx, pRepo, parents[i_parent], &pvaComments)  );
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, HISTORY_RESULT_KEY__COMMENTS, &pvaComments)  );		
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhParents,  parents[i_parent], &pvh_result)  );
			SG_VHASH_NULLFREE(pCtx, pvh_result);
			SG_VARRAY_NULLFREE(pCtx, pvaComments);	
			SG_VARRAY_NULLFREE(pCtx, pvaAudits);
			SG_VHASH_NULLFREE(pCtx, pvh_result);
		}
    }
	*ppvhParents = pvhParents;
	pvhParents = NULL;
	
fail:
    SG_VHASH_NULLFREE(pCtx, pvhParents);
	SG_VARRAY_NULLFREE(pCtx, pvaComments);
	SG_VARRAY_NULLFREE(pCtx, pvaAuditsResult);	
	SG_VHASH_NULLFREE(pCtx, pvh_result);
	SG_VARRAY_NULLFREE(pCtx, pvaAudits);
	SG_VHASH_NULLFREE(pCtx, pvhAuditResult);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	
}

void SG_history__get_changeset_description(SG_context* pCtx, SG_repo* pRepo, const char* pChangesetHid, SG_bool bChildren, SG_vhash** ppvhChangesetDescription)
{
	SG_rbtree* prbChildren = NULL;
    SG_varray * pvaUsers = NULL;
    SG_varray * pva_tags = NULL;
    SG_varray * pva_stamps = NULL;
    SG_varray * pvaTags = NULL;
    SG_varray * pvaComments = NULL;
	SG_varray * pvaChildren = NULL;
    SG_varray * pvaStamps = NULL;
 	SG_varray * pvaAudits = NULL;
	SG_vhash * pvhParents = NULL;
	SG_vhash* pvhChildren = NULL;
	SG_uint32 count_audits;
	SG_uint32 i;
	SG_vhash* pvh_result = NULL;
	SG_uint32 parent_count;
	SG_dagnode* pCurrentDagnode = NULL;
	SG_uint32 revno = 0;
	SG_int32 generation = 0;
 
	SG_ERR_CHECK( SG_VHASH__ALLOC(pCtx, &pvh_result)  );  
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_result, HISTORY_RESULT_KEY__CS_HID, pChangesetHid)  );

	SG_ERR_CHECK( SG_VARRAY__ALLOC(pCtx, &pvaUsers)  );
    SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pChangesetHid, &pvaAudits)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaAudits, &count_audits)  );
 
    for (i=0; i<count_audits; i++)
    {
		SG_vhash* pvh = NULL;
        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaAudits, i, &pvh)  );
		SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pvaUsers, pvh, NULL)  );
	}
	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pChangesetHid, &pCurrentDagnode)  );  
	SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pCurrentDagnode, &revno)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, HISTORY_RESULT_KEY__REVNO, revno)  );
	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pCurrentDagnode, &generation)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, HISTORY_RESULT_KEY__GENERATION, generation)  );
	SG_DAGNODE_NULLFREE(pCtx, pCurrentDagnode);
	
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, HISTORY_RESULT_KEY__AUDITS, &pvaUsers)  );
	SG_VARRAY_NULLFREE(pCtx, pvaUsers);
	SG_VARRAY_NULLFREE(pCtx, pvaAudits );

	SG_ERR_CHECK(  sg_history__get_parents_description(pCtx, pRepo, pChangesetHid, &pvhParents)  );	
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhParents, &parent_count) );	

	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, HISTORY_RESULT_KEY__PARENTS, &pvhParents)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, HISTORY_RESULT_KEY_PARENT_COUNT, (SG_int64) parent_count)  );
	SG_VHASH_NULLFREE(pCtx, pvhParents);

    SG_ERR_CHECK(  SG_vc_tags__lookup(pCtx, pRepo, pChangesetHid, &pva_tags)  );
    if (pva_tags)
    {
        SG_ERR_CHECK(  SG_zing__extract_one_from_slice__string(pCtx, pva_tags, ZING_FIELD_NAME__TAG, &pvaTags)  );
        SG_VARRAY_NULLFREE(pCtx, pva_tags);
    }
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, HISTORY_RESULT_KEY__TAGS, &pvaTags)  );
	SG_VARRAY_NULLFREE(pCtx, pvaTags);

    SG_ERR_CHECK(  SG_vc_stamps__lookup(pCtx, pRepo, pChangesetHid, &pva_stamps)  );
    if (pva_stamps)
    {
        SG_ERR_CHECK(  SG_zing__extract_one_from_slice__string(pCtx, pva_stamps, ZING_FIELD_NAME__STAMP, &pvaStamps)  );
        SG_VARRAY_NULLFREE(pCtx, pva_stamps);
    }
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, HISTORY_RESULT_KEY__STAMPS, &pvaStamps)  );
    SG_VARRAY_NULLFREE(pCtx, pva_stamps);

    SG_ERR_CHECK(  SG_history__get_changeset_comments(pCtx, pRepo, pChangesetHid, &pvaComments)  );
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, HISTORY_RESULT_KEY__COMMENTS, &pvaComments)  );
    SG_VARRAY_NULLFREE(pCtx, pvaComments);	

	if (bChildren)
	{		
		SG_ERR_CHECK(  SG_history__get_children_description(pCtx, pRepo, pChangesetHid, &pvaChildren) );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, HISTORY_RESULT_KEY__CHILDREN, &pvaChildren)  );
		SG_VARRAY_NULLFREE(pCtx, pvaChildren);
		SG_RBTREE_NULLFREE(pCtx, prbChildren);
		
	}

	*ppvhChangesetDescription = pvh_result;
	pvh_result = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaUsers);
	SG_VARRAY_NULLFREE(pCtx, pvaAudits );
    SG_VARRAY_NULLFREE(pCtx, pva_tags);
	SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VARRAY_NULLFREE(pCtx, pva_stamps);
    SG_VARRAY_NULLFREE(pCtx, pvaTags);
    SG_VARRAY_NULLFREE(pCtx, pvaComments);	
	SG_VARRAY_NULLFREE(pCtx, pvaChildren);
	SG_RBTREE_NULLFREE(pCtx, prbChildren);
	SG_VHASH_NULLFREE(pCtx, pvhChildren);
	SG_VHASH_NULLFREE(pCtx, pvhParents);	
	SG_DAGNODE_NULLFREE(pCtx, pCurrentDagnode);
}

struct _my_history_dagwalk_data
{
	SG_uint32 nResultLimit;
	SG_stringarray * pStringArrayGIDs;
	SG_ihash * pih_candidate_changesets;
	SG_varray * pArrayReturnResults;
	SG_bool bHistoryOnRoot;
	SG_bool bHideObjectMerges;
};

void _my_history__dag_walk_callback(
        SG_context * pCtx, 
        SG_repo * pRepo, 
        void * myData, 
        SG_dagnode * currentDagnode, 
        SG_rbtree * pDagnodeCache, 
        SG_dagwalker_continue * pContinue
        )
{
	struct _my_history_dagwalk_data * pData = NULL;
	SG_uint32 resultsSoFar = 0;

	SG_UNUSED(pDagnodeCache);
	pData = (struct _my_history_dagwalk_data*)myData;

	SG_ERR_CHECK(  _sg_history_check_dagnode_for_inclusion(pCtx, pRepo, currentDagnode, pData->bHistoryOnRoot, pData->bHideObjectMerges, pData->pStringArrayGIDs, pData->pih_candidate_changesets, pData->pArrayReturnResults)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pArrayReturnResults, &resultsSoFar)  );

	if (resultsSoFar >= pData->nResultLimit)
    {
		*pContinue = SG_DAGWALKER_CONTINUE__STOP_NOW;
    }

	return;
fail:

	return;

}

void SG_tuple_array__build_reverse_lookup__multiple(
        SG_context * pCtx,
        SG_varray* pva,
        const char* psz_field,
        SG_rbtree** pprb
        )
{
    SG_rbtree* prb = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_vector* pvec_new = NULL;

    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &prb)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_val = NULL;
        SG_vector* pvec = NULL;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, psz_field, &psz_val)  );
        SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb, psz_val, &b_already, (void**) &pvec)  );
        if (!b_already)
        {
            SG_ERR_CHECK(  SG_vector__alloc(pCtx, &pvec_new, 1)  );
            SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb, psz_val, pvec_new)  );
            pvec = pvec_new;
            pvec_new = NULL;
        }
        SG_ERR_CHECK(  SG_vector__append(pCtx, pvec, pvh, NULL)  );
    }

    *pprb = prb;
    prb = NULL;

fail:
    SG_VECTOR_NULLFREE(pCtx, pvec_new);
    SG_RBTREE_NULLFREE(pCtx, prb);
}

static void x_get_users(
        SG_context * pCtx,
        SG_varray* pva,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_users = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_users)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_username = NULL;
        const char* psz_userid = NULL;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "name", &psz_username)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_FIELD__RECID, &psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_users, psz_userid, &b_already)  );
        if (!b_already)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_users, psz_userid, psz_username)  );
        }
    }

    *ppvh = pvh_users;
    pvh_users = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_users);
}

void SG_history__fill_in_details(
        SG_context * pCtx,
        SG_repo * pRepo,
        SG_varray* pva_history
        )
{
    SG_varray* pva_all_audits = NULL;
    SG_varray* pva_all_tags = NULL;
    SG_varray* pva_all_stamps = NULL;
    SG_varray* pva_all_comments = NULL;
    SG_vhash* pvh_users = NULL;
    SG_varray* pva_all_users = NULL;
    SG_rbtree* prb_all_audits = NULL;
    SG_rbtree* prb_all_stamps = NULL;
    SG_rbtree* prb_all_tags = NULL;
    SG_rbtree* prb_all_comments = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_varray* pva_csid_list__comments = NULL;
    SG_varray* pva_csid_list__stamps = NULL;
    SG_varray* pva_csid_list__tags = NULL;
    SG_varray* pva_csid_list__audits = NULL;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_csid_list__comments)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_history, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_csid = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_history, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, HISTORY_RESULT_KEY__CS_HID, &psz_csid)  );
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_csid_list__comments, psz_csid)  );
    }
    // TODO it's very silly that we have to make four copies of this list
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_csid_list__stamps)  );
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_csid_list__audits)  );
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_csid_list__tags)  );

    SG_ERR_CHECK(  SG_varray__copy_items(pCtx, pva_csid_list__comments, pva_csid_list__stamps)  );
    SG_ERR_CHECK(  SG_varray__copy_items(pCtx, pva_csid_list__comments, pva_csid_list__audits)  );
    SG_ERR_CHECK(  SG_varray__copy_items(pCtx, pva_csid_list__comments, pva_csid_list__tags)  );


    // TODO if we're getting all the history, it would be
    // faster to just call list_all like we did before

    SG_ERR_CHECK(  SG_vc_comments__list_for_given_changesets(pCtx, pRepo, &pva_csid_list__comments, &pva_all_comments)  );

    SG_ERR_CHECK(  SG_repo__list_audits_for_given_changesets(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pva_csid_list__audits, &pva_all_audits)  );
    SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pva_all_users)  );
    SG_ERR_CHECK(  x_get_users(pCtx, pva_all_users, &pvh_users)  );

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_all_audits, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_userid = NULL;
        const char* psz_username = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_all_audits, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_AUDIT__USERID, &psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_users, psz_userid, &psz_username)  );
        if (psz_username)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_AUDIT__USERNAME, psz_username)  );
        }
    }

    SG_ERR_CHECK(  SG_vc_stamps__list_for_given_changesets(pCtx, pRepo, &pva_csid_list__stamps, &pva_all_stamps)  );
    SG_ERR_CHECK(  SG_vc_tags__list_for_given_changesets(pCtx, pRepo, &pva_csid_list__tags, &pva_all_tags)  );

    SG_ERR_CHECK(  SG_tuple_array__build_reverse_lookup__multiple(pCtx, pva_all_audits, "csid", &prb_all_audits)  );
    SG_ERR_CHECK(  SG_tuple_array__build_reverse_lookup__multiple(pCtx, pva_all_stamps, "csid", &prb_all_stamps)  );
    SG_ERR_CHECK(  SG_tuple_array__build_reverse_lookup__multiple(pCtx, pva_all_tags, "csid", &prb_all_tags)  );
    SG_ERR_CHECK(  SG_tuple_array__build_reverse_lookup__multiple(pCtx, pva_all_comments, "csid", &prb_all_comments)  );

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_history, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_history, i, &pvh)  );
        SG_ERR_CHECK(  sg_history__details_for_one_changeset(pCtx, pvh, prb_all_audits, prb_all_stamps, prb_all_tags, prb_all_comments)  );
    }

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_csid_list__comments);
    SG_VARRAY_NULLFREE(pCtx, pva_csid_list__stamps);
    SG_VARRAY_NULLFREE(pCtx, pva_csid_list__audits);
    SG_VARRAY_NULLFREE(pCtx, pva_csid_list__tags);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb_all_audits, (SG_free_callback*) SG_vector__free);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb_all_stamps, (SG_free_callback*) SG_vector__free);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb_all_tags, (SG_free_callback*) SG_vector__free);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb_all_comments, (SG_free_callback*) SG_vector__free);
    SG_VARRAY_NULLFREE(pCtx, pva_all_tags);
    SG_VARRAY_NULLFREE(pCtx, pva_all_audits);
    SG_VARRAY_NULLFREE(pCtx, pva_all_stamps);
    SG_VARRAY_NULLFREE(pCtx, pva_all_comments);
    SG_VARRAY_NULLFREE(pCtx, pva_all_users);
    SG_VHASH_NULLFREE(pCtx, pvh_users);
}

void sg_history__get_filtered_candidates(SG_context* pCtx, SG_repo* pRepo, SG_stringarray * pStringArray_single_revisions, SG_stringarray * pStringArrayGIDs, const char* psz_username, const char* pszStamp, SG_int64 nFromDate, SG_int64 nToDate, SG_ihash** ppIHCandidateChangesets)
{
    SG_varray* pva_candidates = NULL;
	SG_ihash* pIHCandidateChangesets = NULL;
	SG_stringarray * pstringarray_csids_with_stamps = NULL;
	SG_vhash * pvhUserInfo = NULL;
	const char * pszUser = NULL;
	SG_vhash * pvh_result_from_treendx = NULL;
	SG_ihash * pih_candidates_new = NULL;

	if (psz_username && psz_username[0])
	{
		SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, psz_username, &pvhUserInfo)  );
		if (pvhUserInfo == NULL)
			SG_ERR_THROW( SG_ERR_USERNOTFOUND );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhUserInfo, "recid", &pszUser)  );
	}
	if ((pszUser && pszUser[0]) || nToDate != SG_INT64_MAX || nFromDate != 0)
    {
		SG_ERR_CHECK(  SG_repo__query_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, 
                    nFromDate!=0?nFromDate:-1, 
                    nToDate!=SG_INT64_MAX?nToDate:-1, 
                    (pszUser&&pszUser[0])?pszUser:NULL, 
                    &pva_candidates
                    )  );

        SG_ERR_CHECK(  SG_IHASH__ALLOC(pCtx, &pIHCandidateChangesets)  );

        if (pva_candidates)
        {
            SG_uint32 count = 0;
            SG_uint32 i = 0;

            SG_ERR_CHECK(  SG_varray__count(pCtx, pva_candidates, &count)  );
            for (i=0; i<count; i++)
            {
                SG_vhash* pvh = NULL;
                const char* psz_csid = NULL;
				SG_bool bAlreadyThere = SG_FALSE;

                SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_candidates, i, &pvh)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "csid", &psz_csid)  );
				SG_ERR_CHECK(  SG_ihash__has(pCtx, pIHCandidateChangesets, psz_csid, &bAlreadyThere)  );
				if (bAlreadyThere == SG_FALSE)
					SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pIHCandidateChangesets, psz_csid, 0)  );
            }
        }
	}

    //Now filter by stamps.
    if (pszStamp != NULL && pszStamp[0] != '\0')
	{
		SG_ERR_CHECK(  SG_vc_stamps__lookup_by_stamp(pCtx, pRepo, NULL, pszStamp, &pstringarray_csids_with_stamps)  );

      	if (pstringarray_csids_with_stamps != NULL)
       	{
       		SG_uint32 csidCount = 0;
       		SG_uint32 csidIndex = 0;
       		SG_bool bAlreadyThere = SG_FALSE;
       		const char * psz_currentcsid = NULL;
			
			SG_ERR_CHECK(  SG_stringarray__count(pCtx, pstringarray_csids_with_stamps, &csidCount)  );
			if (csidCount == 0)
			{
				SG_ERR_THROW(SG_ERR_STAMP_NOT_FOUND);
			}
			SG_ERR_CHECK(  SG_IHASH__ALLOC(pCtx, &pih_candidates_new)  );
			for (csidIndex = 0; csidIndex < csidCount; csidIndex++)
			{
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pstringarray_csids_with_stamps, csidIndex, &psz_currentcsid)  );
				
				if (pIHCandidateChangesets != NULL)  //We need to look to see if it's already there before adding it to the new rbtree.
				{
					SG_ERR_CHECK(  SG_ihash__has(pCtx, pIHCandidateChangesets, psz_currentcsid, &bAlreadyThere)  );
					if (bAlreadyThere)
                    {
						SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_candidates_new, psz_currentcsid, 0)  );
                    }
				}
				else//We just put our entry in, no matter what.
				{
					SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_candidates_new, psz_currentcsid, 0)  );
				}
			}
			SG_IHASH_NULLFREE(pCtx, pIHCandidateChangesets);
			pIHCandidateChangesets = pih_candidates_new;
			pih_candidates_new = NULL;
        }
	}

	if (pStringArrayGIDs != NULL)
	{
		SG_uint32 nCountGIDs = 0;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayGIDs, &nCountGIDs)  );
		if (nCountGIDs != 0)
		{
			SG_ERR_CHECK(  SG_repo__treendx__search_changes(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pStringArrayGIDs, &pvh_result_from_treendx)  );
			if (pvh_result_from_treendx)
			{
				SG_uint32 count = 0;
				SG_uint32 i = 0;

				SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_result_from_treendx, &count)  );
				for(i=0; i < count; i++)
				{
					const char* szKey = NULL;
					
					if (pih_candidates_new == NULL)
						SG_ERR_CHECK(  SG_IHASH__ALLOC(pCtx, &pih_candidates_new)  );

					SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_result_from_treendx, i, &szKey, NULL)  );
					if (pIHCandidateChangesets == NULL)
					{
						SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_candidates_new, szKey, 0)  );
					}
					else
					{
						//Only add to the new rbtree if it was in the old rbtree.
						SG_bool bInOld = SG_FALSE;
						SG_ERR_CHECK(  SG_ihash__has(pCtx, pIHCandidateChangesets, szKey, &bInOld)  );
						if (bInOld)
							SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_candidates_new, szKey, 0)  );
					}
				}
				SG_IHASH_NULLFREE(pCtx, pIHCandidateChangesets);
				pIHCandidateChangesets = pih_candidates_new;
				pih_candidates_new = NULL;
			}

			SG_VHASH_NULLFREE(pCtx, pvh_result_from_treendx);
		}
	}

	if (pStringArray_single_revisions != NULL)
	{
		SG_uint32 nCountSingleRevs = 0;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArray_single_revisions, &nCountSingleRevs)  );
		if (nCountSingleRevs != 0)
		{
			SG_uint32 i = 0;
			for(i=0; i < nCountSingleRevs; i++)
			{
				const char* szKey = NULL;
					
				if (pih_candidates_new == NULL)
					SG_IHASH__ALLOC(pCtx, &pih_candidates_new);

				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringArray_single_revisions, i, &szKey)  );
				if (pIHCandidateChangesets == NULL)
				{
					SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_candidates_new, szKey, 0)  );
				}
				else
				{
					//Only add to the new rbtree if it was in the old rbtree.
					SG_bool bInOld = SG_FALSE;
					SG_ERR_CHECK(  SG_ihash__has(pCtx, pIHCandidateChangesets, szKey, &bInOld)  );
					if (bInOld)
						SG_ERR_CHECK(  SG_ihash__update__int64(pCtx, pih_candidates_new, szKey, 0)  );
				}
			}
			SG_IHASH_NULLFREE(pCtx, pIHCandidateChangesets);
			pIHCandidateChangesets = pih_candidates_new;
			pih_candidates_new = NULL;
		}
	}

	*ppIHCandidateChangesets = pIHCandidateChangesets;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_candidates);
	SG_VHASH_NULLFREE(pCtx, pvhUserInfo);
	SG_IHASH_NULLFREE(pCtx, pih_candidates_new);
    SG_STRINGARRAY_NULLFREE(pCtx, pstringarray_csids_with_stamps);
}

void _sg_history__list__loop(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint32 iNumChangesetsRequested,
	SG_uint32 iStartWithRevNo,
	SG_ihash * pIHCandidateChangesets,
	SG_stringarray * pStringArrayGIDs,
	SG_bool bHideObjectMerges,
	SG_bool* pbHasResult,
	SG_history_result** ppResult,
	SG_history_token ** ppHistoryToken)
{
	SG_varray * pvaResults = NULL;
	SG_repo_fetch_dagnodes_handle* pdh = NULL;

	SG_uint32 count = 0;
	SG_uint32 iNumToFetch = iNumChangesetsRequested;
	SG_uint32 iRevsReturned = 0;
	SG_uint32 i = 0;
	SG_bool bFiltered = pIHCandidateChangesets != NULL;
	SG_varray * pvarray_gids = NULL;
	_history_result* pResult = NULL;
	SG_dagnode * pdn = NULL;
	char** paszHidsReturned = NULL;
	SG_uint32 nResultCount = 0;

	SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pvaResults)  );
	SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &pdh)  );
	
	if (bFiltered)
		iNumToFetch = 1000;
	while (count < iNumChangesetsRequested)
	{
		// Fetch dagnode list
		SG_ERR_CHECK(  SG_repo__fetch_chrono_dagnode_list(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, iStartWithRevNo, iNumToFetch, 
			&iRevsReturned, &paszHidsReturned)  );
	
		// Build result varray	
		{
			const char* szHid = (const char*)paszHidsReturned;
			SG_bool bInclude = SG_TRUE;	
		
			for (i = 0; i < iRevsReturned; i++)
			{				
				if (pIHCandidateChangesets)
				{
					bInclude = SG_FALSE;					
					SG_ERR_CHECK(  SG_ihash__has(pCtx, pIHCandidateChangesets, szHid, &bInclude)  );
					if (bInclude)
					{
						if (bHideObjectMerges == SG_TRUE)
						{
							SG_ERR_CHECK(  _sg_history__inclusion_check__gid(pCtx, pRepo, bHideObjectMerges, szHid, pStringArrayGIDs, &bInclude)  );
						}
					}
					/*if (bInclude)
					{
						if (count++ >= iNumChangesetsRequested)
						{
							//We've gone over the limit.
							//Remember the start rev number.
							SG_ERR_CHECK(  SG_repo__fetch_dagnode__one(pCtx, pRepo, pdh, szHid, &pdn)  );
							SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pdn, &iStartWithRevNo)  );	
							iStartWithRevNo--;
							SG_DAGNODE_NULLFREE(pCtx, pdn);
						}
					} */
				}
				if (bInclude || (i == iRevsReturned -1))
				{
					SG_ERR_CHECK(  SG_REPO__FETCH_DAGNODES__ONE(pCtx, pRepo, pdh, szHid, &pdn)  );
					if (bInclude)
					{						
                        SG_vhash* pvh = NULL;
						SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaResults, &pvh)  );
                        SG_ERR_CHECK(  _sg_history__get_id_and_parents(pCtx, pRepo, pdn, pvh)  );
						count++;
					}
					if (i == (iRevsReturned - 1))
					{
						SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pdn, &iStartWithRevNo)  );					
						//iStartWithRevNo--;
					}
					SG_DAGNODE_NULLFREE(pCtx, pdn);
				}
				if (count >= iNumChangesetsRequested)
				{
					//We've filled up the number of things requested.
					SG_ERR_CHECK(  SG_REPO__FETCH_DAGNODES__ONE(pCtx, pRepo, pdh, szHid, &pdn)  );
					SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pdn, &iStartWithRevNo)  );					
					//iStartWithRevNo--;
					SG_DAGNODE_NULLFREE(pCtx, pdn);
					break;
				}
				szHid += SG_HID_MAX_BUFFER_LENGTH;
			}					
        
			if (!bFiltered || (bFiltered && ((iRevsReturned < iNumToFetch) ||  (count >= iNumChangesetsRequested))))
				break;		
			SG_NULLFREE(pCtx, paszHidsReturned); //Free the hids from this round, because we will be getting more in the next pass through this loop.

		}
	
	}

	//Check the results to see if it maxed out.
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaResults, &nResultCount)  );
	if (nResultCount >= iNumChangesetsRequested && iStartWithRevNo > 1 && ppHistoryToken != NULL)
	{
		SG_vhash * pvh_token = NULL;
		SG_varray * pvarray_candidates = NULL;
		//They hit the max, without running out of data.
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_token)  );
		iStartWithRevNo--;
		SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_token, "hide_merges", bHideObjectMerges)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_token, "resume_from", iStartWithRevNo)  );
		if (pIHCandidateChangesets != NULL)
		{
			SG_ERR_CHECK(  SG_ihash__get_keys(pCtx, pIHCandidateChangesets, &pvarray_candidates)  );
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_token, "candidates", &pvarray_candidates)  );
		}
		if (pStringArrayGIDs != NULL)
		{
			SG_ERR_CHECK(  SG_varray__alloc__stringarray(pCtx, &pvarray_gids, pStringArrayGIDs)  );
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_token, "gids", &pvarray_gids)  );
		}
		*ppHistoryToken = (SG_history_token *)pvh_token;
	}

	SG_ERR_CHECK(  SG_history__fill_in_details(pCtx, pRepo, pvaResults)  );

	if (pvaResults)
	{
		/* If the caller asked for the simple results flag, give it to him... */
		if (pbHasResult)
		{
			SG_uint32 c;
			SG_ERR_CHECK(  SG_varray__count(pCtx, pvaResults, &c)  );
			/* ...but only set it true if there really are results. */
			if (c)
				*pbHasResult = SG_TRUE;
			else
				*pbHasResult = SG_FALSE;
		}

		/* If there are results, build the result structure. */
		SG_ERR_CHECK(  SG_alloc1(pCtx, pResult)  );
		pResult->pva = pvaResults;
		pvaResults = NULL; /* We're returning this to caller. Don't free it below. */
		pResult->idx = 0;
		*ppResult = (SG_history_result*)pResult;
		pResult = NULL; /* We're returning this to caller. Don't free it below. */
	}
	else
	{
		if (pbHasResult) 
			*pbHasResult = SG_FALSE;
	}

fail:
	SG_NULLFREE(pCtx, paszHidsReturned);
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_NULLFREE(pCtx, pResult);
	SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pdh)  );
}

void _sg_history__list(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint32 iNumChangesetsRequested,
	SG_uint32 iStartWithRevNo,	
	SG_ihash* pIHCandidateChangesets,
	SG_stringarray * pStringArrayGIDs,
	SG_bool bHideObjectMerges,
	SG_bool* pbHasResult,
	SG_history_result** ppResult,
	SG_history_token ** ppHistoryToken)
{
	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  _sg_history__list__loop(pCtx, pRepo, iNumChangesetsRequested, iStartWithRevNo, pIHCandidateChangesets, pStringArrayGIDs, bHideObjectMerges, pbHasResult, ppResult, ppHistoryToken)  );
	
	/* fall through */
fail:
	;	
}

void _sg_history__getRootGID(SG_context * pCtx,
        SG_repo * pRepo,
		char ** ppszRootGID)
{
	SG_rbtree * prbIdset = NULL;
	SG_rbtree_iterator * prbit = NULL;
	const char * pszFirstHead = NULL;
	char * pszHidForRoot = NULL;
	SG_treenode * pTreeNodeRoot = NULL;
	SG_bool bFound = SG_FALSE;
	SG_treenode_entry * ptne = NULL;

	//Since we are looking for root, we can pick a head at random.
	//ASSUMPTION:  The GID for @ never changes.
	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &prbIdset)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &prbit, prbIdset, &bFound, &pszFirstHead, NULL)  );
	if (bFound)
	{
		SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, pszFirstHead, &pszHidForRoot)  );
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidForRoot, &pTreeNodeRoot)  );
		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreeNodeRoot, "@", ppszRootGID, &ptne)  );
	}
fail:
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
	SG_TREENODE_NULLFREE(pCtx, pTreeNodeRoot);
	SG_NULLFREE(pCtx, pszHidForRoot);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, prbit);
	SG_RBTREE_NULLFREE(pCtx, prbIdset);
}


void _sg_history__is_history_on_root(SG_context * pCtx,
	SG_repo * pRepo,
	SG_stringarray * pStringArrayGIDs,
	SG_bool * pbIsHistoryOnRoot)
{
	const char * pszTestGID = NULL;
	char * pszRootGID = NULL;
	SG_uint32 count_gids = 0;

	SG_NULLARGCHECK_RETURN(pbIsHistoryOnRoot);

	*pbIsHistoryOnRoot = SG_FALSE;

	if (pStringArrayGIDs != NULL)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayGIDs, &count_gids)  );
    if (count_gids == 0)
	{
		*pbIsHistoryOnRoot = SG_TRUE;
	}
	
	if (count_gids >= 1 )
	{
		//Check to see if one of the provided GIDs is root.
		SG_uint32 index = 0;
		SG_ERR_CHECK( _sg_history__getRootGID(pCtx, pRepo, &pszRootGID)  );
		for (index = 0; index < count_gids; index++)
		{
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringArrayGIDs, 0, &pszTestGID)  );
			if (strcmp(pszRootGID, pszTestGID) == 0)
			{
				*pbIsHistoryOnRoot = SG_TRUE;
				count_gids = 0;
				break;
			}
		}
	}
fail:
	SG_NULLFREE(pCtx, pszRootGID);
}

void _sg_history__put_my_data_into_token(SG_context * pCtx, 
										 SG_history_token * pHistoryToken, 
										struct _my_history_dagwalk_data * pMyData)
{
	SG_vhash * pvh_token = (SG_vhash *) pHistoryToken;
	SG_varray * pvarray_gids = NULL;
	SG_varray * pvarray_candidates = NULL;
	if (pMyData->pStringArrayGIDs != NULL)
	{
		SG_ERR_CHECK(  SG_varray__alloc__stringarray(pCtx, &pvarray_gids, pMyData->pStringArrayGIDs)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_token, "gids", &pvarray_gids)  );
	}
	if (pMyData->pih_candidate_changesets != NULL)
	{
		SG_ERR_CHECK(  SG_ihash__get_keys(pCtx, pMyData->pih_candidate_changesets, &pvarray_candidates)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_token, "candidates", &pvarray_candidates)  );
	}
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_token, "history_on_root", pMyData->bHistoryOnRoot)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_token, "hide_merges", pMyData->bHideObjectMerges)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_token, "result_limit", pMyData->nResultLimit)  );
	//fall through.
fail:
	SG_VARRAY_NULLFREE(pCtx, pvarray_gids);
	SG_VARRAY_NULLFREE(pCtx, pvarray_candidates);
	return;
}

void SG_history_token__free(SG_context * pCtx, SG_history_token * pHistoryToken)
{
	SG_vhash * pvh = (SG_vhash *)pHistoryToken;
	SG_VHASH_NULLFREE(pCtx, pvh) ;
}

void _sg_history__check_provided_revisions(
		SG_context * pCtx, 
		SG_repo * pRepo, 
		SG_stringarray * pStringArrayStartingChangesets, 
		SG_ihash * pIHCandidateChangesets,
		SG_stringarray * pStringArrayGIDs,
		SG_bool bHideObjectMerges, 
		SG_bool * pbHasResult, 
		SG_history_result ** ppResult)
{
	SG_varray * pVArrayResults = NULL;
	SG_uint32 nStartingRevisionsCount = 0;
	SG_uint32 starting_rev_index = 0;
	SG_dagnode * pdnCurrent = NULL;
	const char * currentRev = NULL;
	const char * currentStartingRev = NULL;
	SG_bool bHistoryOnRoot = SG_FALSE;
	SG_vhash * pvhCurrent = NULL;
	SG_bool bSkipThisRev = SG_TRUE;
	
	_history_result* pResult = NULL;

	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pVArrayResults)  );

	SG_ERR_CHECK(  _sg_history__is_history_on_root(pCtx, pRepo, pStringArrayGIDs, &bHistoryOnRoot)  );

	if (pStringArrayStartingChangesets)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayStartingChangesets, &nStartingRevisionsCount)  );

	if (pIHCandidateChangesets != NULL)
	{
		SG_uint32 i = 0;
		SG_uint32 count = 0;
		SG_ERR_CHECK(  SG_ihash__count(pCtx, pIHCandidateChangesets, &count)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_ihash__get_nth_pair(pCtx, pIHCandidateChangesets, i, &currentRev, NULL)  );
			
			if (nStartingRevisionsCount == 0)
				bSkipThisRev = SG_FALSE;
			else
				bSkipThisRev = SG_TRUE;

			//First thing to do is, if starting revisions were specified, we need to verify that the 
			//specified revision is an ancestor of the starting revision.
			for (starting_rev_index = 0; starting_rev_index < nStartingRevisionsCount; starting_rev_index++)
			{
				SG_dagquery_relationship dag_relationship = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringArrayStartingChangesets, starting_rev_index, &currentStartingRev)  );
				SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL,currentRev, currentStartingRev, SG_TRUE,SG_FALSE, &dag_relationship)  );
				if (dag_relationship == SG_DAGQUERY_RELATIONSHIP__SAME || dag_relationship == SG_DAGQUERY_RELATIONSHIP__ANCESTOR)
				{
					//The two revisions are related.  Don't skip it!
					bSkipThisRev = SG_FALSE;
					break;
				}
			}

			if (bSkipThisRev == SG_TRUE)
				continue;

			SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, currentRev, &pdnCurrent)  );
			//Next, we use the dagwalk inclusion check to make sure that it passes all of the 
			//other checks.
			if (pStringArrayGIDs != NULL)
				SG_ERR_CHECK(  _sg_history_check_dagnode_for_inclusion(pCtx, pRepo, pdnCurrent, bHistoryOnRoot, bHideObjectMerges, pStringArrayGIDs, pIHCandidateChangesets, pVArrayResults)  );
			else
			{
				SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pVArrayResults, &pvhCurrent)  );
				SG_ERR_CHECK(  _sg_history__get_id_and_parents(pCtx, pRepo, pdnCurrent, pvhCurrent)  );
			}
			SG_DAGNODE_NULLFREE(pCtx, pdnCurrent);
		}
	}

	SG_ERR_CHECK(  SG_history__fill_in_details(pCtx, pRepo, pVArrayResults)  );
	if (pVArrayResults)
	{
		SG_ERR_CHECK(  SG_alloc1(pCtx, pResult)  );
		pResult->pva = pVArrayResults;
		pResult->idx = 0;
		*ppResult = (SG_history_result*)pResult;

		if (pbHasResult)
		{
			SG_uint32 c;
			SG_ERR_CHECK(  SG_varray__count(pCtx, pVArrayResults, &c)  );
			if (c)
				*pbHasResult = SG_TRUE;
			else
				*pbHasResult = SG_FALSE;
		}
	}
	else
	{
		if (pbHasResult)
			*pbHasResult = SG_TRUE;
	}

	SG_DAGNODE_NULLFREE(pCtx, pdnCurrent);
	return;
fail:
	SG_DAGNODE_NULLFREE(pCtx, pdnCurrent);
    SG_VARRAY_NULLFREE(pCtx, pVArrayResults);

}

void SG_history__get_revision_details(
		SG_context * pCtx, 
		SG_repo * pRepo, 
		SG_stringarray * pStringArrayChangesets_single_revisions, 
		SG_bool * pbHasResult, 
		SG_history_result ** ppResult)
{
	SG_ihash * pih_candidate_changesets = NULL;

	SG_ERR_CHECK(  SG_stringarray__to_ihash_keys(pCtx, pStringArrayChangesets_single_revisions, &pih_candidate_changesets)  );

	SG_ERR_CHECK(  _sg_history__check_provided_revisions(pCtx, pRepo, NULL, pih_candidate_changesets, NULL, SG_FALSE, pbHasResult, ppResult)  );
fail:
	SG_IHASH_NULLFREE(pCtx, pih_candidate_changesets);
	return;
}


void _sg_history__query(
        SG_context * pCtx,
        SG_repo * pRepo,
		SG_stringarray * pStringArrayStartingChangesets,
		SG_ihash * pIHCandidateChangesets,
        SG_stringarray * pStringArrayGIDs,
        SG_uint32 nResultLimit,
		SG_bool bHideObjectMerges,
		SG_bool* pbHasResult,
        SG_history_result ** ppResult,
		SG_history_token ** ppHistoryToken
        )
{
	SG_varray * pVArrayResults = NULL;
	SG_pathname* pPathCwd = NULL;  
	SG_dagnode * pdnCurrent = NULL;

	SG_uint32 candidateCount = 0;
	SG_bool bHistoryOnRoot = SG_FALSE;

	_history_result* pResult = NULL;
	SG_uint32 revisionsCount = 0;

	SG_NULLARGCHECK_RETURN(pStringArrayStartingChangesets);

	SG_ERR_CHECK(  _sg_history__is_history_on_root(pCtx, pRepo, pStringArrayGIDs, &bHistoryOnRoot)  );

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayStartingChangesets, &revisionsCount)  );
	if (revisionsCount == 0 )
		SG_ERR_THROW2(SG_ERR_INVALIDARG,
						(pCtx, "_sg_history__query was called without starting revisions."));

	

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pVArrayResults)  );
	
	{
		struct _my_history_dagwalk_data  myData;
        memset(&myData, 0, sizeof(myData));
     
		myData.bHideObjectMerges = bHideObjectMerges;
		myData.nResultLimit = nResultLimit;
		myData.pArrayReturnResults = pVArrayResults;
		myData.pStringArrayGIDs = pStringArrayGIDs;
		myData.bHistoryOnRoot = bHistoryOnRoot;
		
		if (pIHCandidateChangesets != NULL)
        {
			SG_ERR_CHECK(  SG_ihash__count(pCtx, pIHCandidateChangesets, &candidateCount)  );
        }
		
		//We now have the GIDs, and the candidate change sets.
		myData.pih_candidate_changesets = pIHCandidateChangesets;
		SG_ERR_CHECK(  SG_dagwalker__walk_dag(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pStringArrayStartingChangesets, _my_history__dag_walk_callback, (void*)&myData, (SG_dagwalker_token**) ppHistoryToken)  );

		//The dagwalk gave us back a token to continue, if we want.  Put the history-specific
		//info in it.
		if (ppHistoryToken != NULL && *ppHistoryToken != NULL)
		{
			SG_ERR_CHECK(  _sg_history__put_my_data_into_token(pCtx, *ppHistoryToken, &myData)  );
		}
	}

#if 0
    SG_STRINGARRAY_STDERR(pStringArrayGIDs);
    SG_VARRAY_STDERR(pVArrayResults);
#endif

    SG_ERR_CHECK(  SG_history__fill_in_details(pCtx, pRepo, pVArrayResults)  );
	if (pVArrayResults)
	{
		SG_ERR_CHECK(  SG_alloc1(pCtx, pResult)  );
		pResult->pva = pVArrayResults;
		pResult->idx = 0;
		*ppResult = (SG_history_result*)pResult;

		if (pbHasResult)
		{
			SG_uint32 c;
			SG_ERR_CHECK(  SG_varray__count(pCtx, pVArrayResults, &c)  );
			if (c)
				*pbHasResult = SG_TRUE;
			else
				*pbHasResult = SG_FALSE;
		}
	}
	else
	{
		if (pbHasResult)
			*pbHasResult = SG_TRUE;
	}

	SG_DAGNODE_NULLFREE(pCtx, pdnCurrent);

	return;

fail:  
    SG_VARRAY_NULLFREE(pCtx, pVArrayResults);
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_DAGNODE_NULLFREE(pCtx, pdnCurrent);
	SG_NULLFREE(pCtx, pResult);
}

void SG_history_result__to_json(SG_context* pCtx, SG_history_result** ppResult, SG_string* pStr)
{
	_history_result* p = NULL;
	SG_vhash* pvhContainer = NULL;

	SG_NULL_PP_CHECK_RETURN(ppResult);
	SG_NULLARGCHECK_RETURN(pStr);

	p = (_history_result*)*ppResult;

	SG_ERR_CHECK(  SG_varray__to_json(pCtx, p->pva, pStr)  );

	SG_HISTORY_RESULT_NULLFREE(pCtx, *ppResult);

	/* fall through */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhContainer);
}

void SG_history_result__from_json(SG_context* pCtx, const char* pszJson, SG_history_result** ppNew)
{
	SG_varray* pva = NULL;
	_history_result* p = NULL;

	SG_NULLARGCHECK_RETURN(pszJson);
	SG_NULLARGCHECK_RETURN(ppNew);

	SG_ERR_CHECK(  SG_VARRAY__ALLOC__FROM_JSON__SZ(pCtx, &pva, pszJson)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, p)  );
	p->pva = pva;
	pva = NULL;
	p->idx = 0;

	*ppNew = (SG_history_result*)p;
	p = NULL;

	/* fall through */
fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_ERR_IGNORE(  _history_result_free(pCtx, p)  );
}

void SG_history_result__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piCount)
{
	SG_varray* pva;

	SG_NULLARGCHECK_RETURN(pHistory);

	pva = ((_history_result*)pHistory)->pva;
	SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pva, piCount)  );
}

void SG_history_result__reverse(SG_context* pCtx, SG_history_result* pHistory)
{
	SG_varray* pva;

	SG_NULLARGCHECK_RETURN(pHistory);

	pva = ((_history_result*)pHistory)->pva;
	SG_ERR_CHECK_RETURN(  SG_varray__reverse(pCtx, pva)  );
}

static SG_qsort_compare_function _compare_generation__descending;

static int _compare_generation__descending(SG_context * pCtx,
						 const void * pVoid_ppv1, // const SG_variant ** ppv1
						 const void * pVoid_ppv2, // const SG_variant ** ppv2
						 void * pVoidData)
{
	const SG_variant** ppv1 = (const SG_variant **)pVoid_ppv1;
	const SG_variant** ppv2 = (const SG_variant **)pVoid_ppv2;
	SG_vhash * pvh1;
	SG_vhash * pvh2;
	SG_uint32 nGen1;
	SG_uint32 nGen2;
	int result = 0;

	SG_UNUSED( pVoidData );

	if (*ppv1 == NULL && *ppv2 == NULL)
		return 0;
	if (*ppv1 == NULL)
		return 1; //Descending!
	if (*ppv2 == NULL)
		return -1;

	SG_variant__get__vhash(pCtx, *ppv1, &pvh1);
	SG_variant__get__vhash(pCtx, *ppv2, &pvh2);

	SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh1, "generation", &nGen1)  );
	SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh2, "generation", &nGen2)  );

	if (nGen1 < nGen2)
		return 1;
	else if (nGen1 < nGen2)
		return 0;
	else if (nGen1 > nGen2)
		return -1;

fail:
	return result;
}

void SG_history_result__sort_by_generation(SG_context* pCtx, SG_history_result* pHistory)
{
	SG_varray* pva;

	SG_NULLARGCHECK_RETURN(pHistory);

	pva = ((_history_result*)pHistory)->pva;
	SG_ERR_CHECK_RETURN(  SG_varray__sort(pCtx, pva, _compare_generation__descending, NULL)  );
}

void SG_history_result__start_at_end(SG_context* pCtx, SG_history_result* pHistory, SG_bool* pbOk)
{
	_history_result* p = (_history_result*)pHistory;
	SG_uint32 count;

	/* SG_history_result__count checks pHistory for NULL */
	SG_ERR_CHECK_RETURN(  SG_history_result__count(pCtx, pHistory, &count)  );

	if (count > 0)
	{
		p->idx = count - 1;
		if (pbOk)
			*pbOk = SG_TRUE;
	}
	else
	{
		if (pbOk)
			*pbOk = SG_FALSE;
	}
}

void SG_history_result__set_index(SG_context * pCtx, SG_history_result * pHistory, SG_uint32 idx)
{
	_history_result* p = (_history_result*)pHistory;
	SG_uint32 count;

	/* SG_history_result__count checks pHistory for NULL */
	SG_ERR_CHECK_RETURN(  SG_history_result__count(pCtx, pHistory, &count)  );
	SG_ARGCHECK_RETURN( (idx < count), "idx" );

	p->idx = idx;
}

void SG_history_result__get_index(SG_context * pCtx, SG_history_result * pHistory, SG_uint32 * p_idx)
{
	_history_result* p = (_history_result*)pHistory;

	SG_NULLARGCHECK_RETURN(pHistory);
	*p_idx = p->idx;
}

void SG_history_result__prev(SG_context* pCtx, SG_history_result* pHistory, SG_bool* pbOk)
{
	_history_result* p = (_history_result*)pHistory;
	SG_uint32 count;

	/* SG_history_result__count checks pHistory for NULL */
	SG_ERR_CHECK_RETURN(  SG_history_result__count(pCtx, pHistory, &count)  );

	if (p->idx != 0)
	{
		p->idx--;
		if (pbOk)
			*pbOk = SG_TRUE;
	}
	else
	{
		if (pbOk)
			*pbOk = SG_FALSE;
	}
}

void SG_history_result__next(SG_context* pCtx, SG_history_result* pHistory, SG_bool* pbOk)
{
	_history_result* p = (_history_result*)pHistory;
	SG_uint32 count;

	/* SG_history_result__count checks pHistory for NULL */
	SG_ERR_CHECK_RETURN(  SG_history_result__count(pCtx, pHistory, &count)  );

	if ((p->idx + 1) < count)
	{
		p->idx++;
		if (pbOk)
			*pbOk = SG_TRUE;
	}
	else
	{
		if (pbOk)
			*pbOk = SG_FALSE;
	}
}

void _history_result__get_current_vhash(SG_context* pCtx, SG_history_result* pHistory, SG_vhash** ppvhRef)
{
	_history_result* p = (_history_result*)pHistory;

	SG_NULLARGCHECK_RETURN(pHistory);

	SG_ERR_CHECK_RETURN(  SG_varray__get__vhash(pCtx, p->pva, p->idx, ppvhRef)  );
}

void _sg_history_result__add_pseudo_parent(SG_context * pCtx, SG_history_result *pHistory, const char * psz_pseudo_parent_csid, SG_uint32 nRevNo)
{
	SG_vhash* pvhRef;
	SG_vhash* pvhPseudoParentsRef = NULL;

	SG_ERR_CHECK(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK( SG_vhash__check__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PSEUDO_PARENTS, &pvhPseudoParentsRef) );
	if (pvhPseudoParentsRef == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PSEUDO_PARENTS, &pvhPseudoParentsRef)  );
	}

	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhPseudoParentsRef, psz_pseudo_parent_csid, nRevNo)  );	
fail:
	return;
}

void _sg_history_result__add_pseudo_child(SG_context * pCtx, SG_history_result *pHistory, const char * psz_pseudo_child_csid, SG_uint32 nRevNo)
{
	SG_vhash* pvhRef;
	SG_vhash* pvhPseudoParentsRef = NULL;

	SG_ERR_CHECK(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK( SG_vhash__check__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PSEUDO_CHILDREN, &pvhPseudoParentsRef) );
	if (pvhPseudoParentsRef == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PSEUDO_CHILDREN, &pvhPseudoParentsRef)  );
	}

	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhPseudoParentsRef, psz_pseudo_child_csid, nRevNo)  );	
fail:
	return;
}

void SG_history_result__get_cshid(SG_context* pCtx, SG_history_result* pHistory, const char** pszCsidRef)
{
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvhRef, HISTORY_RESULT_KEY__CS_HID, pszCsidRef)  );
}	

void SG_history_result__get_revno(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piRevno)
{
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_vhash__get__uint32(pCtx, pvhRef, HISTORY_RESULT_KEY__REVNO, piRevno);
	if (SG_context__err_equals(pCtx, SG_ERR_VHASH_KEYNOTFOUND))
	{
		SG_ERR_DISCARD;
		*piRevno = 0;
	}
	else
		SG_ERR_CHECK_RETURN_CURRENT;
}

void SG_history_result__get_generation(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piGen)
{
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_vhash__get__uint32(pCtx, pvhRef, HISTORY_RESULT_KEY__GENERATION, piGen);
	if (SG_context__err_equals(pCtx, SG_ERR_VHASH_KEYNOTFOUND))
	{
		SG_ERR_DISCARD;
		*piGen = 0;
	}
	else
		SG_ERR_CHECK_RETURN_CURRENT;
}

void SG_history_result__get_parent__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piParentCount)
{
	SG_vhash* pvhRef;
	SG_vhash* pvhParentsRef;
	SG_bool bHasParents;
	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN( SG_vhash__has(pCtx, pvhRef, HISTORY_RESULT_KEY__PARENTS, &bHasParents) );
	if (bHasParents)
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PARENTS, &pvhParentsRef)  );
		SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvhParentsRef, piParentCount)  );
	}
	else
		*piParentCount = 0;
}


void SG_history_result__get_pseudo_parent__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piPseudoParentCount)
{
	SG_vhash* pvhRef;
	SG_vhash* pvhParentsRef;
	SG_bool bHasParents;
	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN( SG_vhash__has(pCtx, pvhRef, HISTORY_RESULT_KEY__PSEUDO_PARENTS, &bHasParents) );
	if (bHasParents)
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PSEUDO_PARENTS, &pvhParentsRef)  );
		SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvhParentsRef, piPseudoParentCount)  );
	}
	else
		*piPseudoParentCount = 0;
}

void SG_history_result__get_pseudo_parent(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 parentIndex, const char** ppszParentHidRef, SG_uint32* piRevno)
{
	SG_vhash* pvhParentsRef;
	SG_vhash* pvhRef;
	const SG_variant* pvRef;
	SG_uint64 i64;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PSEUDO_PARENTS, &pvhParentsRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvhParentsRef, parentIndex, ppszParentHidRef, &pvRef)  );

	if (piRevno)
	{
		SG_ERR_CHECK_RETURN(  SG_variant__get__uint64(pCtx, pvRef, &i64)  );
		*piRevno = (SG_uint32)i64;
	}
}

void SG_history_result__check_for_parent(SG_context* pCtx, SG_history_result* pHistory, const char* pszParentHidRef, SG_bool * pbIsParent)
{
	SG_vhash* pvhParentsRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PARENTS, &pvhParentsRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvhParentsRef, pszParentHidRef, pbIsParent)  );
}

void SG_history_result__get_parent(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 parentIndex, const char** ppszParentHidRef, SG_uint32* piRevno)
{
	SG_vhash* pvhParentsRef;
	SG_vhash* pvhRef;
	const SG_variant* pvRef;
	SG_uint64 i64;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvhRef, HISTORY_RESULT_KEY__PARENTS, &pvhParentsRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvhParentsRef, parentIndex, ppszParentHidRef, &pvRef)  );

	if (piRevno)
	{
		SG_ERR_CHECK_RETURN(  SG_variant__get__uint64(pCtx, pvRef, &i64)  );
		*piRevno = (SG_uint32)i64;
	}
}

void SG_history_result__get_tag__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piTagCount)
{
	SG_varray* pvaTagsRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__TAGS, &pvaTagsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pvaTagsRef, piTagCount)  );
}

void SG_history_result__get_tag__text(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 tagIndex, const char** ppszTagRef)
{
	SG_varray* pvaTagsRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__TAGS, &pvaTagsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__get__sz(pCtx, pvaTagsRef, tagIndex, ppszTagRef)  );
}

void SG_history_result__get_stamp__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piStampCount)
{
	SG_varray* pvaStampsRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__STAMPS, &pvaStampsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pvaStampsRef, piStampCount)  );
}

void SG_history_result__get_stamp__text(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 stampIndex, const char** ppszStampRef)
{
	SG_varray* pvaStampsRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__STAMPS, &pvaStampsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__get__sz(pCtx, pvaStampsRef, stampIndex, ppszStampRef)  );
}

void SG_history_result__get_audit__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piAuditCount)
{
	SG_varray* pvaAuditsRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__AUDITS, &pvaAuditsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pvaAuditsRef, piAuditCount)  );
}

void SG_history_result__get_audit__who(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 auditIndex, const char** ppszWho)
{
	SG_varray* pvaAuditsRef;
	SG_vhash* pvhAuditRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__AUDITS, &pvaAuditsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__get__vhash(pCtx, pvaAuditsRef, auditIndex, &pvhAuditRef)  );

    SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvhAuditRef, SG_AUDIT__USERNAME, ppszWho)  );
    if (!*ppszWho)
    {
		SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvhAuditRef, SG_AUDIT__USERID, ppszWho)  );
    }
}

void SG_history_result__get_audit__when(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 auditIndex, SG_int64* piWhen)
{
	SG_varray* pvaAuditsRef;
	SG_vhash* pvhAuditRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__AUDITS, &pvaAuditsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__get__vhash(pCtx, pvaAuditsRef, auditIndex, &pvhAuditRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64_or_double(pCtx, pvhAuditRef, SG_AUDIT__TIMESTAMP, piWhen)  );
}

void SG_history_result__get_comment__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piCommentCount)
{
	SG_varray* pvaCommentsRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__COMMENTS, &pvaCommentsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pvaCommentsRef, piCommentCount)  );
}

void SG_history_result__get_comment__text(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 commentIndex, const char** ppszComment)
{
	SG_varray* pvaCommentsRef;
	SG_vhash* pvhCommentRef;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__COMMENTS, &pvaCommentsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__get__vhash(pCtx, pvaCommentsRef, commentIndex, &pvhCommentRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvhCommentRef, HISTORY_RESULT_KEY__COMMENTS_TEXT, ppszComment)  );
}

void SG_history_result__get_first_comment__text(SG_context* pCtx, SG_history_result* pHistory, const char** ppszComment)
{
	SG_varray* pvaCommentsRef;
	SG_vhash* pvhCommentRef;
	SG_uint32 count;
	SG_vhash* pvhRef;

	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__COMMENTS, &pvaCommentsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pvaCommentsRef, &count)  );
	if (count)
	{
		/* On 8/13/2010 Ian said:
		 * It seems like count-1 should actually be zero, but all the old code did count-1, so I'm leaving it for now. 
		 * Did all the old code deliberately retrieve the LAST comment (not the first), and this method needs renaming?
		 * Or are they added in reverse so the last one in the varray is actually the first comment? */
		SG_ERR_CHECK_RETURN(  SG_varray__get__vhash(pCtx, pvaCommentsRef, count-1, &pvhCommentRef)  );
		SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvhCommentRef, HISTORY_RESULT_KEY__COMMENTS_TEXT, ppszComment)  );
	}
	else
	{
		SG_NULLARGCHECK_RETURN(ppszComment);
		*ppszComment = NULL;
	}
}

void SG_history_result__get_comment__history__ref(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 commentIndex, SG_varray ** ppvhCommentHistory)
{
	SG_varray* pvaCommentsRef;
	SG_vhash* pvhCommentRef;
	SG_vhash* pvhRef;


	SG_ERR_CHECK_RETURN(  _history_result__get_current_vhash(pCtx, pHistory, &pvhRef)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhRef, HISTORY_RESULT_KEY__COMMENTS, &pvaCommentsRef)  );
	SG_ERR_CHECK_RETURN(  SG_varray__get__vhash(pCtx, pvaCommentsRef, commentIndex, &pvhCommentRef)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvhCommentRef, HISTORY_RESULT_KEY__HISTORY, ppvhCommentHistory)  );
}

void SG_history_result__get_root(SG_context* pCtx, SG_history_result* pHistory, SG_varray** ppvaRef)
{
	_history_result* p = (_history_result*)pHistory;

	SG_NULLARGCHECK_RETURN(pHistory);
	SG_NULLARGCHECK_RETURN(ppvaRef);

	*ppvaRef = p->pva;
}

void SG_history_result__free(SG_context* pCtx, SG_history_result* pResult)
{
	SG_ERR_CHECK_RETURN(  _history_result_free(pCtx, (_history_result*)pResult)  );
}

#if defined(DEBUG)
void SG_history_debug__dump_result_to_console(SG_context* pCtx, SG_history_result* pHistory)
{
	_history_result* p = (_history_result*)pHistory;

	SG_NULLARGCHECK_RETURN(pHistory);

    SG_VARRAY_STDERR(p->pva);
	//SG_ERR_CHECK_RETURN(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, p->pva, "history")  );
}
#endif /* defined(DEBUG) */

void _sg_history__create_empty_result(SG_context * pCtx, SG_bool* pbHasResult, SG_history_result ** ppResult)
{
	_history_result * pResult = NULL;
	SG_varray * pVArrayResults = NULL;

	SG_VARRAY__ALLOC(pCtx, &pVArrayResults);
	SG_ERR_CHECK(  SG_alloc1(pCtx, pResult)  );
	pResult->pva = pVArrayResults;
	pResult->idx = 0;
	*ppResult = (SG_history_result*)pResult;

	if (pbHasResult)
	{
		*pbHasResult = SG_FALSE;
	}
fail:
	;
}

struct _reassemble_dag__check_ancestry__dagwalk_data
{
	SG_int32 nGenOfPossibleParent;
	const char * pszHidOfPossibleParent;
	SG_ihash * pih_known_parents;
	SG_bool bIsParent;
};

void _sg_history__reassemble_dag__check_ancestry__dagwalk_callback(
        SG_context * pCtx, 
        SG_repo * pRepo, 
        void * myData, 
        SG_dagnode * currentDagnode, 
        SG_rbtree * pDagnodeCache, 
        SG_dagwalker_continue * pContinue
        )
{
	SG_int32 nCurrentDagnodeGen = 0;
	struct _reassemble_dag__check_ancestry__dagwalk_data * pData = NULL;
	const char * pszHidOfCurrentChangeset = NULL;
	SG_bool bParentIsKnown = SG_FALSE;
	SG_UNUSED(pDagnodeCache);
	SG_UNUSED(pRepo);

	pData = (struct _reassemble_dag__check_ancestry__dagwalk_data*)myData;

	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, currentDagnode, &nCurrentDagnodeGen)  );

	//If we've passed the possible parent's generation, we need to stop.
	if (nCurrentDagnodeGen < pData->nGenOfPossibleParent)
	{
		pData->bIsParent = SG_FALSE;
		*pContinue = SG_DAGWALKER_CONTINUE__STOP_NOW;
		return;
	}


	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, currentDagnode, &pszHidOfCurrentChangeset)  );

	//If the current node is the one that we're looking for, report yes, and return immediately
	if (SG_strcmp__null(pszHidOfCurrentChangeset, pData->pszHidOfPossibleParent) == 0)
	{
		pData->bIsParent = SG_TRUE;
		*pContinue = SG_DAGWALKER_CONTINUE__STOP_NOW;
		return;
	}

	SG_ERR_CHECK(  SG_ihash__has(pCtx, pData->pih_known_parents, pszHidOfCurrentChangeset, &bParentIsKnown)  );
	
	//If this node is already in the known parents list, don't bother dagwalking it or its parents.
	if (bParentIsKnown == SG_TRUE)
	{
		*pContinue = SG_DAGWALKER_CONTINUE__EMPTY_QUEUE;
		return;
	}
fail:
	;
}

void _sg_history__reassemble_dag__check_for_parent(SG_context * pCtx, SG_repo * pRepo, const char * pszInitialNode, const char * pszPossibleParent, SG_ihash * pih_known_parents, SG_rbtree * prb_dagnode_cache, SG_repo_fetch_dagnodes_handle * pFetchDagnodesHandle_input, SG_bool * pbIsParent)
{
	struct _reassemble_dag__check_ancestry__dagwalk_data data;
	SG_int32 nPossibleParentGen = 0;
	SG_dagnode * pPossibleParentDagnode = NULL;
	SG_dagnode * pPossibleChildDagnode = NULL;
	SG_bool bInCache = SG_FALSE;
	SG_repo_fetch_dagnodes_handle * pFetchDagnodesHandle = NULL;
	SG_bool bIsDirectParent = SG_FALSE;

	SG_NULLARGCHECK(pbIsParent);
	
	if (pFetchDagnodesHandle_input == NULL)
	{
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &pFetchDagnodesHandle)  );
	}
	else
	{
		pFetchDagnodesHandle = pFetchDagnodesHandle_input;
	}

	//Check to see if the potential child is in the dagnode cache.  If not, load it and put it in the cache.
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_dagnode_cache, pszInitialNode, &bInCache, (void**)&pPossibleChildDagnode)  );

	if (bInCache == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_REPO__FETCH_DAGNODES__ONE(pCtx, pRepo, pFetchDagnodesHandle, pszInitialNode, &pPossibleChildDagnode)  );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_dagnode_cache, pszInitialNode, pPossibleChildDagnode)  );
	}
	SG_ERR_CHECK(  SG_dagnode__is_parent(pCtx, pPossibleChildDagnode, pszPossibleParent, &bIsDirectParent)  );

	if (bIsDirectParent == SG_FALSE)
	{
		//Check to see if the potential parent is in the dagnode cache.  If not, load it and put it in the cache.
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_dagnode_cache, pszPossibleParent, &bInCache, (void**)&pPossibleParentDagnode)  );
	
		if (bInCache == SG_FALSE)
		{
			SG_ERR_CHECK(  SG_REPO__FETCH_DAGNODES__ONE(pCtx, pRepo, pFetchDagnodesHandle, pszPossibleParent, &pPossibleParentDagnode)  );
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_dagnode_cache, pszPossibleParent, pPossibleParentDagnode)  );
		}

		SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pPossibleParentDagnode, &nPossibleParentGen)  );

		*pbIsParent  = data.bIsParent = SG_FALSE;
		data.nGenOfPossibleParent = nPossibleParentGen;
		data.pszHidOfPossibleParent = pszPossibleParent;
		data.pih_known_parents = pih_known_parents;
		SG_ERR_CHECK(  SG_dagwalker__walk_dag_single__keep_cache(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszInitialNode, _sg_history__reassemble_dag__check_ancestry__dagwalk_callback, &data, prb_dagnode_cache, pFetchDagnodesHandle)   );
		*pbIsParent = data.bIsParent;
	}
	else
		*pbIsParent = SG_TRUE;

fail:
	if (pFetchDagnodesHandle_input == NULL)
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pFetchDagnodesHandle)  );
}

void _sg_history__reassemble_dag(SG_context * pCtx, SG_repo * pRepo, SG_bool bNeedSort, SG_history_result **ppResult, SG_history_token * pvh_old_token, SG_history_token ** pvh_new_token)
{
	SG_int32 outerIndex = 0;
	SG_int32 innerIndex = 0;
	_history_result* p = NULL;
	SG_uint32 resultCount = 0;
	SG_uint32 oldListCount = 0;
	SG_uint32 cacheSize = 0;
	SG_rbtree * prb_KnownParentsList = NULL;
	SG_vhash * pvhTokenList = NULL;
	SG_vhash * pvhOldTokenList = NULL;
	SG_ihash * pihParentsForThisNode = NULL;
	SG_repo_fetch_dagnodes_handle* pFetchDagnodesHandle = NULL;
	SG_rbtree * prb_dagnode_cache = NULL;
	SG_bool bCleanupTokenList = SG_FALSE;

	if (pvh_old_token != NULL)
		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, (SG_vhash*)pvh_old_token, "dag_reassembly_nodes", &pvhOldTokenList)  );

	if (pvh_new_token != NULL && *pvh_new_token != NULL)
		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, (SG_vhash*)*pvh_new_token, "dag_reassembly_nodes", &pvhTokenList)  );
	else 
	{
		bCleanupTokenList = SG_TRUE;
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhTokenList)  );
	}

	//In order to reassemble the dag, the first thing that we need to do is sort the results
	if (bNeedSort)
		SG_ERR_CHECK(  SG_history_result__sort_by_generation(pCtx, *ppResult)  );

	if (pvhOldTokenList != NULL && pvhTokenList != NULL)
		SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pvhOldTokenList, pvhTokenList)  );

	//Now, we iterate over the results.  The results will already
	//be in order from the latest generation to the earliest generation.

	p = (_history_result*)*ppResult;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_dagnode_cache)  );

	SG_ERR_CHECK(  SG_history_result__count(pCtx, *ppResult, &resultCount)  );

	if (pvhOldTokenList)
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhOldTokenList, &oldListCount)  );

	cacheSize = oldListCount + resultCount;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_KnownParentsList)  );
	
	SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &pFetchDagnodesHandle)  );

	//If we need to deal with reassembling the dag, put all the hids into the new tokenlist.
	//We need to do this first, since the next loop goes through the results
	//BACKWARDS!!!!!
	if (pvhTokenList != NULL)
	{
		const char * psz_test_csid = NULL;
		for (outerIndex = 0; outerIndex < (SG_int32)resultCount; outerIndex++)
		{
			p->idx = outerIndex;
			SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, *ppResult, &psz_test_csid)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhTokenList, psz_test_csid, 0)  );
		}
	}
	//History results come in reversed, so ppResult[resultCount - 1] has the smallest generation
	for (outerIndex = (SG_int32)resultCount - 1; outerIndex >= 0 ; outerIndex--)
	{
		const char * psz_test_csid = NULL;
		p->idx = outerIndex;

		SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, *ppResult, &psz_test_csid)  );

		SG_ERR_CHECK(  SG_IHASH__ALLOC(pCtx, &pihParentsForThisNode)  );
		for (innerIndex = outerIndex + 1; innerIndex < (SG_int32)resultCount; innerIndex++)
		{
			const char * psz_potential_parent_csid = NULL;
			SG_bool bAlreadyInParentList = SG_FALSE;
			SG_bool bIsParent = SG_FALSE;
			SG_uint32 nParentRevNo = 0;

			//Get the CSID for the potential parent
			p->idx = innerIndex;
			SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, *ppResult, &psz_potential_parent_csid)  );
			SG_ERR_CHECK(  SG_history_result__get_revno(pCtx, *ppResult, &nParentRevNo)  );
			
			p->idx = outerIndex; //For the rest of this loop, we only need to check the outer changeset.

			SG_ERR_CHECK(  SG_history_result__check_for_parent(pCtx, *ppResult, psz_potential_parent_csid, &bIsParent)  );

			if (bIsParent == SG_TRUE)
			{
				SG_ihash * pihParentsForParentNode = NULL;
				SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_KnownParentsList, psz_potential_parent_csid, NULL, (void**)&pihParentsForParentNode)  );
				SG_ERR_CHECK(  SG_ihash__copy_items(pCtx, pihParentsForParentNode, pihParentsForThisNode, SG_TRUE)  );
				SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pihParentsForThisNode, psz_potential_parent_csid, 0)  );
				SG_ERR_CHECK(  _sg_history_result__add_pseudo_parent(pCtx, *ppResult, psz_potential_parent_csid, nParentRevNo)  );
				continue;
			}
			
			SG_ERR_CHECK(  SG_ihash__has(pCtx, pihParentsForThisNode, psz_potential_parent_csid, &bAlreadyInParentList)  );

			if (bAlreadyInParentList == SG_TRUE)
				continue;

			SG_ERR_CHECK(  _sg_history__reassemble_dag__check_for_parent(pCtx, pRepo, psz_test_csid, psz_potential_parent_csid, pihParentsForThisNode, prb_dagnode_cache, pFetchDagnodesHandle, &bIsParent)  );
			if (bIsParent == SG_TRUE)
			{
				//The two revisions are related.  Don't skip it!
				SG_ihash * pihParentsForParentNode = NULL;
				SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_KnownParentsList, psz_potential_parent_csid, NULL, (void**)&pihParentsForParentNode)  );
				{
					SG_uint32 nCountParentsForParentNode = 0;
					SG_ERR_CHECK(  SG_ihash__count(pCtx, pihParentsForParentNode, &nCountParentsForParentNode)  );
					if (nCountParentsForParentNode == 0)
					{
						nCountParentsForParentNode++;
					}
				}
				SG_ERR_CHECK(  SG_ihash__copy_items(pCtx, pihParentsForParentNode, pihParentsForThisNode, SG_TRUE)  );
				SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pihParentsForThisNode, psz_potential_parent_csid, 0)  );
				SG_ERR_CHECK(  _sg_history_result__add_pseudo_parent(pCtx, *ppResult, psz_potential_parent_csid, nParentRevNo)  );
			}
		}
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_KnownParentsList, psz_test_csid, pihParentsForThisNode)  );
		pihParentsForThisNode = NULL;
	}

	
	if (oldListCount > 0)
	{
		//We need to check for connections between the new data, and the old data.
		//Loop over the old data, which covers the region from 0 to oldListCount - 1
		for (outerIndex = oldListCount - 1; outerIndex >= 0; outerIndex--)
		{
			SG_bool bBreakOutEarly = SG_FALSE;
			const char * psz_current_node_in_old_data = NULL;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair__int32(pCtx, pvhTokenList, outerIndex, &psz_current_node_in_old_data, NULL)  );

			SG_ERR_CHECK(  SG_IHASH__ALLOC(pCtx, &pihParentsForThisNode)  );

			//Now loop over the nodes with a more recent generation, to check 
			//for pseudo-parent connections.
			for (innerIndex = outerIndex + 1; bBreakOutEarly == SG_FALSE && innerIndex < (SG_int32)cacheSize; innerIndex++)
			{
				const char * psz_potential_parent = NULL;
				SG_bool bIsParent = SG_FALSE;
				SG_bool bAlreadyInParentList = SG_FALSE;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair__int32(pCtx, pvhTokenList, innerIndex, &psz_potential_parent, NULL)  );

				//Break out early, if the node is already known to be a parent.
				SG_ERR_CHECK(  SG_ihash__has(pCtx, pihParentsForThisNode, psz_potential_parent, &bAlreadyInParentList)  );

				if (bAlreadyInParentList == SG_TRUE)
					continue;

				SG_ERR_CHECK(  _sg_history__reassemble_dag__check_for_parent(pCtx, pRepo, psz_current_node_in_old_data, psz_potential_parent, pihParentsForThisNode, prb_dagnode_cache, pFetchDagnodesHandle, &bIsParent)  );

				if (bIsParent)
				{
					//The two revisions are related.  Don't skip it!
					SG_uint32 nCountParents = 0;
					SG_ihash * pihParentsForParentNode = NULL;
					SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_KnownParentsList, psz_potential_parent, NULL, (void**)&pihParentsForParentNode)  );
					SG_ERR_CHECK(  SG_ihash__copy_items(pCtx, pihParentsForParentNode, pihParentsForThisNode, SG_TRUE)  );
					SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pihParentsForThisNode, psz_potential_parent, 0)  );
					SG_ERR_CHECK(  SG_ihash__count(pCtx, pihParentsForThisNode, &nCountParents)  );
					if (innerIndex >= (SG_int32)oldListCount)
					{
						p->idx = innerIndex - oldListCount;
						SG_ERR_CHECK(  _sg_history_result__add_pseudo_child(pCtx, *ppResult, psz_current_node_in_old_data, 0)  );
					}
					
					if (nCountParents == (cacheSize - innerIndex))
					{
						//We can break out early if this node has _all_ of the remaining nodes as its parent.
						bBreakOutEarly = SG_TRUE;
					}
					//
				}
			}
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_KnownParentsList, psz_current_node_in_old_data, pihParentsForThisNode)  );
			pihParentsForThisNode = NULL;
		}
	}

fail:
	SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pFetchDagnodesHandle)  );
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb_dagnode_cache, (SG_free_callback*) SG_dagnode__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb_KnownParentsList, (SG_free_callback*) SG_ihash__free);
	if (bCleanupTokenList)
		SG_VHASH_NULLFREE(pCtx, pvhTokenList);
	SG_IHASH_NULLFREE(pCtx, pihParentsForThisNode);
	if (p != NULL)
		p->idx = 0;
	return;
}

void SG_history__run(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_stringarray * pStringArrayGIDs,
	SG_stringarray * pStringArrayStartingChangesets, 
	SG_stringarray * pStringArrayStartingChangesets_single_revisions,
	const char* pszUser,
	const char* pszStamp,
	SG_uint32 nResultLimit,
	SG_bool bLeaves,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bRecommendDagWalk,
	SG_bool bReassembleDag,
	SG_bool* pbHasResult,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken)
{
	SG_uint32 nSingleRevCount = 0;
	SG_uint32 count_args = 0;
	SG_ihash * pIHCandidateChangesets = NULL;
	if (pStringArrayGIDs)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayGIDs, &count_args)  );
	if (bHideObjectMerges &&  count_args == 0)
		SG_ERR_THROW2(SG_ERR_INVALIDARG,
					  (pCtx, "In order to hide merges, a file or folder object must be supplied."));

	if (pStringArrayStartingChangesets_single_revisions)
	{
		SG_ERR_CHECK( SG_stringarray__count(pCtx, pStringArrayStartingChangesets_single_revisions, &nSingleRevCount) );
	}

	if (    nSingleRevCount != 0
			|| (
             pszUser != NULL 
             && pszUser[0] != '\0'
             ) 
            || (
                pszStamp != NULL 
                && pszStamp[0] != '\0'
                ) 
            || nFromDate != 0 
            || nToDate != SG_INT64_MAX
			|| count_args != 0
            )
	{
		SG_uint32 nCandidateCount = 0;
		SG_ERR_CHECK(  sg_history__get_filtered_candidates(pCtx, pRepo, pStringArrayStartingChangesets_single_revisions, pStringArrayGIDs, pszUser, pszStamp, nFromDate, nToDate, &pIHCandidateChangesets)  );
		
		//These checks affirm that we have more than zero candidates left.
		if (pIHCandidateChangesets == NULL)
		{
			SG_ERR_CHECK(  _sg_history__create_empty_result(pCtx, pbHasResult, ppResult) );
			return;
		}
		SG_ERR_CHECK(  SG_ihash__count(pCtx, pIHCandidateChangesets, &nCandidateCount)  );
		if (nCandidateCount == 0)
		{
			SG_ERR_CHECK(  _sg_history__create_empty_result(pCtx, pbHasResult, ppResult) );
			SG_IHASH_NULLFREE(pCtx, pIHCandidateChangesets);
			return;
		}
	}

	if (nSingleRevCount > 0)
	{
		// They've given us a list of revisions, and they want us to check it against
		// the rest of the filters to see how many match.
		SG_ERR_CHECK( _sg_history__check_provided_revisions(pCtx, pRepo,
														   pStringArrayStartingChangesets,
														   pIHCandidateChangesets,
														   pStringArrayGIDs,
														   bHideObjectMerges, pbHasResult,
														   ppResult) );
	}
	else if (bLeaves && bRecommendDagWalk == SG_FALSE)
	{
		// They aren't asking for anything complicated, just use Ian's rev numbers
		// to iterate over all changesets.  This is easier, because no dag walk is necessary.
		SG_ERR_CHECK( _sg_history__list(pCtx, pRepo, nResultLimit, SG_UINT32_MAX,
									   pIHCandidateChangesets,
									   pStringArrayGIDs,
									   bHideObjectMerges, pbHasResult,
									   ppResult, ppHistoryToken) );
	}
	else
	{
		SG_ERR_CHECK( _sg_history__query(pCtx, pRepo,
										pStringArrayStartingChangesets,
										pIHCandidateChangesets,
										pStringArrayGIDs,
										nResultLimit,
										bHideObjectMerges,
										pbHasResult, ppResult, ppHistoryToken) );
	}

	if (pIHCandidateChangesets)
	{
		SG_uint32 candidateCount = 0;
		SG_ERR_CHECK(  SG_ihash__count(pCtx, pIHCandidateChangesets, &candidateCount)  );
		if (candidateCount > 0)
		{
			//If they have asked us to reassemble the dag.
			if (bReassembleDag)
			{
				SG_ERR_CHECK(  _sg_history__reassemble_dag(pCtx, pRepo, nSingleRevCount > 0, ppResult, NULL, ppHistoryToken)  );
				if (ppHistoryToken != NULL && *ppHistoryToken != NULL)
					SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, (SG_vhash*)*ppHistoryToken, "reassemble_dag", SG_TRUE)  );
			}
		}
	}

fail:
	SG_IHASH_NULLFREE(pCtx, pIHCandidateChangesets);
	return;
}

void SG_history__fetch_more(SG_context * pCtx, SG_repo * pRepo, SG_history_token * pHistoryToken, SG_uint32 nResultLimit, SG_history_result ** ppResult,  SG_history_token **ppNewToken)
{
	struct _my_history_dagwalk_data myData;
	SG_varray * pVArrayResults = NULL;
    _history_result* pResult = NULL;
	SG_stringarray * pStringArrayStartingChangesets = NULL;
	SG_vhash * pvh_token = (SG_vhash *)pHistoryToken;
	SG_varray * pvarrayCandidates = NULL;
	SG_varray * pvarrayGIDs = NULL;
	SG_bool bHas = SG_FALSE;
	SG_varray * pvarrayQueue = NULL;
	SG_ihash * pih_candidates = NULL;
	SG_bool bIsListOperation = SG_FALSE;
	SG_stringarray * pStringArrayGIDs = NULL;
	SG_bool bHideObjectMerges = SG_FALSE;
	SG_bool bReassembleDag = SG_FALSE;

	memset(&myData, 0, sizeof(myData));

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_token, "candidates", &bHas)  );
	if (bHas)
	{
    	SG_uint32 nCandidateCount = 0;
    	SG_uint32 index = 0;
    	const char * pszCandidate = NULL;

    	SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_token,  "candidates", &pvarrayCandidates)  );
    	SG_ERR_CHECK(  SG_varray__count(pCtx, pvarrayCandidates, &nCandidateCount)  );
    	SG_ERR_CHECK(  SG_IHASH__ALLOC(pCtx, &pih_candidates)  );
    	for (index = 0; index < nCandidateCount; index++)
    	{
			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvarrayCandidates, index, &pszCandidate)  );
			SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_candidates, pszCandidate, 0)  );
    	}
	}
	else
    	pih_candidates = NULL;
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_token, "hide_merges", &bHas)  );
	if (bHas)
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_token, "hide_merges", &bHideObjectMerges)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_token, "reassemble_dag", &bHas)  );
	if (bHas)
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_token, "reassemble_dag", &bReassembleDag)  );
		
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_token, "resume_from", &bIsListOperation)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_token, "gids", &bHas)  );
	if (bHas)
	{
    	SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_token,  "gids", &pvarrayGIDs)  );
		SG_ERR_CHECK(  SG_varray__to_stringarray(pCtx, pvarrayGIDs, &pStringArrayGIDs)  );
	}
	else
    	pStringArrayGIDs = NULL;
	if (bIsListOperation)
	{
		SG_int64 iStartFrom = 0;
		SG_ERR_CHECK(  SG_vhash__get__int64_or_double(pCtx, pvh_token, "resume_from", &iStartFrom)  );
		
		SG_ERR_CHECK(  _sg_history__list__loop(pCtx, pRepo, nResultLimit, (SG_int32)iStartFrom, pih_candidates, pStringArrayGIDs, bHideObjectMerges, NULL, ppResult, ppNewToken)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pVArrayResults)  );

		//Load the details from the token.
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_token, "history_on_root", &myData.bHistoryOnRoot)  );
		myData.nResultLimit = nResultLimit;
		myData.pArrayReturnResults = pVArrayResults;
		myData.pih_candidate_changesets = pih_candidates;
		myData.pStringArrayGIDs = pStringArrayGIDs;
		pStringArrayGIDs = NULL;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_token, "queue", &bHas)  );
		if (bHas)
		{
			SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_token,  "queue", &pvarrayQueue)  );
			SG_ERR_CHECK(  SG_varray__to_stringarray(pCtx, pvarrayQueue, &pStringArrayStartingChangesets)  );
		}
		else
			pStringArrayStartingChangesets = NULL;
		SG_ERR_CHECK(  SG_dagwalker__walk_dag(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pStringArrayStartingChangesets, _my_history__dag_walk_callback, (void*)&myData, (SG_dagwalker_token **)ppNewToken)  );
		if (ppNewToken != NULL && *ppNewToken != NULL)
		{
			SG_ERR_CHECK(  _sg_history__put_my_data_into_token(pCtx, *ppNewToken, &myData)  );
		}

		SG_ERR_CHECK(  SG_history__fill_in_details(pCtx, pRepo, pVArrayResults)  );
		if (pVArrayResults)
		{
			SG_ERR_CHECK(  SG_alloc1(pCtx, pResult)  );
			pResult->pva = pVArrayResults;
			pResult->idx = 0;
			*ppResult = (SG_history_result*)pResult;
		}
	}

	if (pih_candidates)
	{
		SG_uint32 candidateCount = 0;
		SG_ERR_CHECK(  SG_ihash__count(pCtx, pih_candidates, &candidateCount)  );
		if (candidateCount > 0)
		{
			//If they have asked us to reassemble the dag.
			if (bReassembleDag)
			{
				SG_ERR_CHECK(  _sg_history__reassemble_dag(pCtx, pRepo, SG_FALSE, ppResult, pHistoryToken, ppNewToken)  );
				if (ppNewToken != NULL && *ppNewToken != NULL)
					SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, (SG_vhash*)*ppNewToken, "reassemble_dag", SG_TRUE)  );
			}
		}
	}

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, myData.pStringArrayGIDs);
	SG_IHASH_NULLFREE(pCtx, pih_candidates);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayGIDs);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayStartingChangesets);

}
