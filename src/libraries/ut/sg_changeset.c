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
 * @file sg_changeset.c
 *
 * @details Routines for manipulating the contents of a Changeset.
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#	define TRACE_CHANGESET		0
#else
#	define TRACE_CHANGESET		0
#endif

/**
 * SG_changeset is a structure containing information on a single
 * changeset (a commit).  This includes all of the parent changesets
 * that it depends on (that it was a change from) and handles to
 * the various data structures that we need to represent this state
 * of the user's project and a list of blobs that are considered
 * new with respect to the parents.
 *
 * Changesets are converted to JSON and stored in Changeset-type
 * Blobs in the Repository.
 */
struct SG_changeset
{
    /**
     * A changeset struct can be in one of two states: constructing
     * or frozen.  During the constructing phase, data is accumulated
     * in members used only during that phase, in a substruct.  Then,
     * when committing to the repo, the changeset is frozen, the data
     * converted to a vhash.
     *
     * When a changeset is read from the repo, it is frozen, and the
     * constructing substruct is not used.
     */

    struct
    {
        SG_changeset_version ver;
        SG_uint64 dagnum;
        SG_int32 generation;
        SG_rbtree* prb_parents;

        struct
        {
            char* psz_hid_tree_root;
            SG_rbtree* prb_treenode_cache;
            SG_vhash* pvh_gid_changes;
        } tree;

        struct
        {
            char* psz_db_template;
            SG_varray* pva_all_templates;
            SG_vhash* pvh_changes;
        } db;

    } constructing;

    struct
    {
        SG_vhash* pvh;
        char* psz_hid;
    } frozen;
};

//////////////////////////////////////////////////////////////////

/*
 * We store the following keys/values in the hash.  Some keys are
 * always present and some may be version-dependent.  Since we
 * don't know what future keys/values may be required, we leave this
 * an open-ended list.
 */

/* Always present. */
#define KEY_VERSION                 "ver"

/* Present in Version 1 changeset (may also be used by future versions). */
#define KEY_DAGNUM                  "dagnum"
#define KEY_PARENTS                 "parents"
#define KEY_GENERATION              "generation"

#define KEY_TREE                    "tree"
#define KEY_TREE__ROOT              "root"
#define KEY_TREE__CHANGES           "changes"

#define KEY_DB                      "db"
#define KEY_DB__CHANGES             "changes"
#define KEY_DB__TEMPLATE            "template"
#define KEY_DB__ALL_TEMPLATES       "all_templates"
#define KEY_DB__CHANGES__ADD        "add"
#define KEY_DB__CHANGES__REMOVE     "remove"

//////////////////////////////////////////////////////////////////

#define FAIL_IF_FROZEN(pChangeset)                                               \
	SG_STATEMENT(                                                                \
		SG_bool b = SG_TRUE;                                                     \
		SG_ERR_CHECK_RETURN(  SG_changeset__is_frozen(pCtx,(pChangeset),&b)  );  \
		if (b)                                                                   \
			SG_ERR_THROW_RETURN( SG_ERR_INVALID_WHILE_FROZEN );                  )

#define FAIL_IF_NOT_FROZEN(pChangeset)                                           \
	SG_STATEMENT(                                                                \
		SG_bool b = SG_FALSE;                                                    \
		SG_ERR_CHECK_RETURN(  SG_changeset__is_frozen(pCtx,(pChangeset),&b)  );  \
		if (!b)                                                                  \
			SG_ERR_THROW_RETURN( SG_ERR_INVALID_UNLESS_FROZEN );                 )

#define FAIL_IF_DB_DAG(pChangeset)                                               \
	SG_STATEMENT(                                                                \
		if (SG_DAGNUM__IS_DB(pChangeset->constructing.dagnum))                  \
			SG_ERR_THROW_RETURN( SG_ERR_WRONG_DAG_TYPE );                        )

#define FAIL_IF_NOT_DB_DAG(pChangeset)                                               \
	SG_STATEMENT(                                                                \
		if (!SG_DAGNUM__IS_DB(pChangeset->constructing.dagnum))                  \
			SG_ERR_THROW_RETURN( SG_ERR_WRONG_DAG_TYPE );                        )

//////////////////////////////////////////////////////////////////

void SG_changeset__alloc__committing(SG_context * pCtx, SG_uint64 dagnum, SG_changeset ** ppNew)
{
	SG_changeset* pChangeset = NULL;

	SG_NULLARGCHECK_RETURN(ppNew);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pChangeset)  );

    pChangeset->constructing.dagnum = dagnum;

    if (SG_DAGNUM__IS_DB(pChangeset->constructing.dagnum))
    {
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pChangeset->constructing.db.pvh_changes)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pChangeset->constructing.tree.prb_treenode_cache)  );
    }

	*ppNew = pChangeset;
    pChangeset = NULL;

fail:
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
}

//////////////////////////////////////////////////////////////////

void SG_changeset__free(SG_context * pCtx, SG_changeset * pChangeset)
{
	if (!pChangeset)
    {
		return;
    }

	SG_VHASH_NULLFREE(pCtx, pChangeset->frozen.pvh);
	SG_VHASH_NULLFREE(pCtx, pChangeset->constructing.tree.pvh_gid_changes);
	SG_VHASH_NULLFREE(pCtx, pChangeset->constructing.db.pvh_changes);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pChangeset->constructing.tree.prb_treenode_cache, (SG_free_callback*) SG_treenode__free);
	SG_RBTREE_NULLFREE(pCtx, pChangeset->constructing.prb_parents);
	SG_NULLFREE(pCtx, pChangeset->frozen.psz_hid);
	SG_NULLFREE(pCtx, pChangeset->constructing.tree.psz_hid_tree_root);
	SG_NULLFREE(pCtx, pChangeset->constructing.db.psz_db_template);
    SG_VARRAY_NULLFREE(pCtx, pChangeset->constructing.db.pva_all_templates);

	SG_NULLFREE(pCtx, pChangeset);
}

static void _sg_changeset__serialize_parents(
	SG_context * pCtx,
    SG_changeset* pcs,
	SG_vhash* pvh
	)
{
	// convert the given idset into a sorted varray and add it to
	// or update the value of the n/v pair with the given key in
	// the top-level vhash.
	//
	// if the idset is null, make sure that the corresponding key
	// in the top-level vhash is set correctly.

	SG_varray * pva = NULL;
	SG_uint32 count;
    SG_bool b_has = SG_FALSE;

	if (!pcs->constructing.prb_parents)
		goto NoIds;

	SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx, pcs->constructing.prb_parents, &count)  );

	if (count == 0)
		goto NoIds;

	SG_ERR_CHECK_RETURN(  SG_rbtree__to_varray__keys_only(pCtx, pcs->constructing.prb_parents, &pva)  );

	SG_ERR_CHECK( SG_vhash__add__varray(pCtx, pvh, KEY_PARENTS, &pva) );

    goto done;

NoIds:
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh, KEY_PARENTS, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh, KEY_PARENTS)  );
    }

done:
fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void _sg_changeset__create_the_vhash(SG_context * pCtx, SG_changeset * pChangeset)
{
    SG_rbtree_iterator* pIterator = NULL;
    SG_vhash* pvh = NULL;
	char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

	SG_NULLARGCHECK_RETURN(pChangeset);

	FAIL_IF_FROZEN(pChangeset);

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, KEY_GENERATION, (SG_int64)(pChangeset->constructing.generation))  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, pChangeset->constructing.dagnum, buf_dagnum, sizeof(buf_dagnum))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, KEY_DAGNUM, buf_dagnum)  );

	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, KEY_VERSION, (SG_int64)(pChangeset->constructing.ver))  );


	SG_ERR_CHECK(  _sg_changeset__serialize_parents(pCtx, pChangeset, pvh)  );

    if (SG_DAGNUM__IS_DB(pChangeset->constructing.dagnum))
    {
        SG_vhash* pvh_db = NULL;

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, KEY_DB, &pvh_db)  );

        if (pChangeset->constructing.db.pva_all_templates)
        {
            SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_db, KEY_DB__ALL_TEMPLATES, &pChangeset->constructing.db.pva_all_templates)  );
        }

        if (pChangeset->constructing.db.psz_db_template)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_db, KEY_DB__TEMPLATE, pChangeset->constructing.db.psz_db_template)  );
        }

        if (pChangeset->constructing.db.pvh_changes)
        {
            SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_db, KEY_DB__CHANGES, &pChangeset->constructing.db.pvh_changes)  );
        }
    }
    else
    {
        SG_vhash* pvh_tree = NULL;

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, KEY_TREE, &pvh_tree)  );
        if (pChangeset->constructing.tree.psz_hid_tree_root)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_tree, KEY_TREE__ROOT, pChangeset->constructing.tree.psz_hid_tree_root)  );
        }

        if (pChangeset->constructing.tree.pvh_gid_changes)
        {
            SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_tree, KEY_TREE__CHANGES, &pChangeset->constructing.tree.pvh_gid_changes)  );
        }
    }

    pChangeset->frozen.pvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
}

void SG_changeset__set_version(SG_context * pCtx, SG_changeset * pChangeset, SG_changeset_version ver)
{
	SG_NULLARGCHECK_RETURN(pChangeset);

	FAIL_IF_FROZEN(pChangeset);

    pChangeset->constructing.ver = ver;
}

void SG_changeset__add_parent(SG_context * pCtx, SG_changeset * pChangeset, const char* pszidHidParent)
{
	SG_NULLARGCHECK_RETURN(pChangeset);

	FAIL_IF_FROZEN(pChangeset);

	if (!pszidHidParent)
		return;

	// create rbtree if necessary (we deferred allocation until needed)

	if (!pChangeset->constructing.prb_parents)
	{
		SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC__PARAMS(pCtx, &pChangeset->constructing.prb_parents, 4, NULL)  );
	}

	// finally, actually add HID to idset.

	SG_ERR_CHECK_RETURN(  SG_rbtree__update(pCtx, pChangeset->constructing.prb_parents, pszidHidParent)  );
}

void SG_changeset__tree__set_root(SG_context * pCtx, SG_changeset * pChangeset, const char* pszidHid)
{
	SG_NULLARGCHECK_RETURN(pChangeset);

	FAIL_IF_FROZEN(pChangeset);
	FAIL_IF_DB_DAG(pChangeset);

    SG_ASSERT(!pChangeset->constructing.tree.psz_hid_tree_root);

    if (pszidHid)
    {
        SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx, pszidHid, &pChangeset->constructing.tree.psz_hid_tree_root)  );
    }
}

static void _get_generation_from_repo(SG_context * pCtx, SG_repo * pRepo, SG_uint64 dagnum, const char* pszidHid, SG_int32 * pGen)
{
	// load the dagnode for the given HID into memory and
	// extract the generation.

	SG_dagnode* pdn = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pszidHid);
	SG_NULLARGCHECK_RETURN(pGen);

	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, pszidHid, &pdn)  );
	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, pGen)  );

	SG_DAGNODE_NULLFREE(pCtx, pdn);

	return;
fail:
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}

static void sg_changeset__tree__load_treenode(
        SG_context* pCtx, 
        SG_changeset* pcs,
        SG_repo* pRepo,
        const char* psz_hid_treenode,
        SG_treenode** pptn
        )
{
    SG_bool b_already = SG_FALSE;
    SG_treenode* ptn = NULL;

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pcs->constructing.tree.prb_treenode_cache, psz_hid_treenode, &b_already, (void**) &ptn)  );
    if (!ptn)
    {
        SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, psz_hid_treenode, &ptn)  );
        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pcs->constructing.tree.prb_treenode_cache, psz_hid_treenode, ptn)  );
    }

    *pptn = ptn;
    ptn = NULL;

fail:
    SG_TREENODE_NULLFREE(pCtx, ptn);
}

static void sg_changeset__tree__get_pairs(
        SG_context* pCtx, 
        SG_changeset* pcs,
        SG_repo* pRepo,
        const char* psz_gid_treenode,
        const char* psz_hid_treenode,
        SG_vhash* pvh_pairs
        )
{
    SG_uint32 count_entries = 0;
	SG_treenode* ptn = NULL;
    SG_uint32 i_entry = 0;

	SG_ERR_CHECK(  sg_changeset__tree__load_treenode(pCtx, pcs, pRepo, psz_hid_treenode, &ptn)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, ptn, &count_entries)  );

	for (i_entry=0; i_entry<count_entries; i_entry++)
	{
		const char * psz_gid = NULL;
		const SG_treenode_entry* pEntry = NULL;
        const char* psz_hid = NULL;
        SG_treenode_entry_type type = 0;
        const char* psz_name = NULL;
        SG_vhash* pvh_info = NULL;
        SG_int64 bits = 0;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, ptn, i_entry, &psz_gid, &pEntry)  );
        SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &psz_hid)  );
        SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &type)  );
        SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &psz_name)  );
        SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pEntry, &bits)  );

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_pairs, psz_gid, &pvh_info)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__HID, psz_hid)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__NAME, psz_name)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__DIR, psz_gid_treenode ? psz_gid_treenode : "")  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__BITS, bits)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__TYPE, type)  );

        if ( type == SG_TREENODEENTRY_TYPE_DIRECTORY)
        {
            SG_ERR_CHECK(  sg_changeset__tree__get_pairs(pCtx, pcs, pRepo, psz_gid, psz_hid, pvh_pairs)  );
        }
	}

fail:
    ;
}

static void sg_changeset__tree__compare_two_gid_infos(
        SG_context* pCtx, 
        const char* psz_gid,
        SG_vhash* pvh_info__me,
        SG_vhash* pvh_info__parent,
        SG_vhash* pvh_gid_changes
        )
{
    const char* psz_hid__me = NULL;
    const char* psz_dir__me = NULL;
    const char* psz_name__me = NULL;
    SG_int64 type__me = 0;
    SG_int64 bits__me = 0;

    SG_bool b_diff__type = SG_FALSE;
    SG_bool b_diff__bits = SG_FALSE;
    SG_bool b_diff__hid = SG_FALSE;
    SG_bool b_diff__name = SG_FALSE;
    SG_bool b_diff__dir = SG_FALSE;

    SG_vhash* pvh_info__new = NULL;

    // me
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_info__me, SG_CHANGEESET__TREE__CHANGES__HID, &psz_hid__me)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_info__me, SG_CHANGEESET__TREE__CHANGES__NAME, &psz_name__me)  );
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_info__me, SG_CHANGEESET__TREE__CHANGES__TYPE, &type__me)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_info__me, SG_CHANGEESET__TREE__CHANGES__DIR, &psz_dir__me)  );
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_info__me, SG_CHANGEESET__TREE__CHANGES__BITS, &bits__me)  );

    if (pvh_info__parent)
    {
        const char* psz_hid__parent = NULL;
        const char* psz_dir__parent = NULL;
        const char* psz_name__parent = NULL;
        SG_int64 type__parent = 0;
        SG_int64 bits__parent = 0;

        // parent
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_info__parent, SG_CHANGEESET__TREE__CHANGES__HID, &psz_hid__parent)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_info__parent, SG_CHANGEESET__TREE__CHANGES__NAME, &psz_name__parent)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_info__parent, SG_CHANGEESET__TREE__CHANGES__TYPE, &type__parent)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_info__parent, SG_CHANGEESET__TREE__CHANGES__BITS, &bits__parent)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_info__parent, SG_CHANGEESET__TREE__CHANGES__DIR, &psz_dir__parent)  );

        b_diff__type = (type__me != type__parent);
        b_diff__bits = (bits__me != bits__parent);
        b_diff__hid = (0 != strcmp(psz_hid__me, psz_hid__parent));
        b_diff__name = (0 != strcmp(psz_name__me, psz_name__parent));
        b_diff__dir = (0 != strcmp(psz_dir__me, psz_dir__parent));

        if (
                !b_diff__type
                && !b_diff__bits
                && !b_diff__hid
                && !b_diff__dir
                && !b_diff__name
           )
        {
            return;
        }
    }
    else
    {
        b_diff__type = SG_TRUE;
        b_diff__hid = SG_TRUE;
        b_diff__name = SG_TRUE;
        b_diff__dir = (psz_dir__me != NULL);
        b_diff__bits = (bits__me != 0);
    }

    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_gid_changes, psz_gid, &pvh_info__new)  );
    if (b_diff__type)
    {
        SG_ERR_CHECK(  SG_vhash__update__int64(pCtx, pvh_info__new, SG_CHANGEESET__TREE__CHANGES__TYPE, type__me)  );
    }
    if (b_diff__hid)
    {
        SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_info__new, SG_CHANGEESET__TREE__CHANGES__HID, psz_hid__me)  );
    }
    if (b_diff__name)
    {
        SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_info__new, SG_CHANGEESET__TREE__CHANGES__NAME, psz_name__me)  );
    }
    if (b_diff__dir)
    {
        SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_info__new, SG_CHANGEESET__TREE__CHANGES__DIR, psz_dir__me)  );
    }
    if (b_diff__bits)
    {
        SG_ERR_CHECK(  SG_vhash__update__int64(pCtx, pvh_info__new, SG_CHANGEESET__TREE__CHANGES__BITS, bits__me)  );
    }

fail:
    ;
}

static void sg_changeset__tree__gid_changes__two(
        SG_context* pCtx, 
        SG_vhash* pvh_pairs__me,
        SG_vhash* pvh_pairs__parent,
        SG_vhash* pvh_gid_changes
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    // TODO change this to sort both lists and walk them simultaneously

    // first walk through __me and find anything which is
    // (1) not in __parent
    // (2) different in __parent
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_pairs__me, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_gid = NULL;
        SG_vhash* pvh_info__me = NULL;
        SG_vhash* pvh_info__parent = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_pairs__me, i, &psz_gid, &pvh_info__me)  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_pairs__parent, psz_gid, &pvh_info__parent)  );
        SG_ERR_CHECK(  sg_changeset__tree__compare_two_gid_infos(pCtx, psz_gid, pvh_info__me, pvh_info__parent, pvh_gid_changes)  );
    }

    // now walk through __parent and find anything which is
    // not in __me
    //
    // it could be argued that we do not need this.  anything which has been 
    // deleted is already represented in the delta by its absence in its parent 
    // treenode which got a new hid.  we decide to keep this code to maintain
    // consistency across versions and to avoid any unknown risks associated
    // with a change.
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_pairs__parent, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_gid = NULL;
        SG_vhash* pvh_info__parent = NULL;
        SG_bool b = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_pairs__parent, i, &psz_gid, &pvh_info__parent)  );

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pairs__me, psz_gid, &b)  );
        if (!b)
        {
            SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_gid_changes, psz_gid)  );
        }
    }

fail:
    ;
}

static void sg_changeset__db__calc_all_templates(
        SG_context* pCtx, 
        SG_changeset* pcs, 
        SG_repo* pRepo
        )
{
    SG_vhash* pvh_all_templates = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_changeset* pcs_parent = NULL;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_all_templates)  );
    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_all_templates, pcs->constructing.db.psz_db_template)  );
	if (pcs->constructing.prb_parents)
    {
        SG_bool b = SG_FALSE;
        const char* psz_csid_parent = NULL;
        SG_uint32 count_parents = 0;

        SG_ERR_CHECK(  SG_rbtree__count(pCtx, pcs->constructing.prb_parents, &count_parents)  );
        if (count_parents > 0)
        {
            SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pcs->constructing.prb_parents, &b, &psz_csid_parent, NULL)  );
            while (b)
            {
                SG_varray* pva_parent_all_templates = NULL;

                SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, psz_csid_parent, &pcs_parent)  );
                SG_ERR_CHECK(  SG_changeset__db__get_all_templates(pCtx, pcs_parent, &pva_parent_all_templates)  );
                if (pva_parent_all_templates)
                {
                    SG_uint32 count = 0;
                    SG_uint32 i = 0;

                    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parent_all_templates, &count)  );

                    for (i=0; i<count; i++)
                    {
                        const char* psz_hid = NULL;

                        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parent_all_templates, i, &psz_hid)  );
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_all_templates, psz_hid)  );

                    }
                }

                SG_CHANGESET_NULLFREE(pCtx, pcs_parent);

                SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_csid_parent, NULL)  );
            }
            SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

        }
    }
    SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_all_templates, SG_FALSE, SG_vhash_sort_callback__increasing)  );
    SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvh_all_templates, &pcs->constructing.db.pva_all_templates)  );

fail:
    SG_CHANGESET_NULLFREE(pCtx, pcs_parent);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_VHASH_NULLFREE(pCtx, pvh_all_templates);
}

static void sg_changeset__tree__calc_gid_change_list(
        SG_context* pCtx, 
        SG_changeset* pcs, 
        SG_repo* pRepo
        )
{
    SG_vhash* pvh_pairs__me = NULL;
    SG_vhash* pvh_gid_changes = NULL;
    SG_vhash* pvh_pairs__parent = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_uint32 count_changes = 0;
	char* psz_hid_treenode_parent = NULL;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_pairs__me)  );
    SG_ERR_CHECK(  sg_changeset__tree__get_pairs(pCtx, pcs, pRepo, NULL, pcs->constructing.tree.psz_hid_tree_root, pvh_pairs__me)  );

	if (pcs->constructing.prb_parents)
    {
        SG_bool b = SG_FALSE;
        const char* psz_csid_parent = NULL;
        SG_uint32 count_parents = 0;

        SG_ERR_CHECK(  SG_rbtree__count(pCtx, pcs->constructing.prb_parents, &count_parents)  );
        if (0 == count_parents)
        {
            goto no_parents;
        }

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_gid_changes)  );
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pcs->constructing.prb_parents, &b, &psz_csid_parent, NULL)  );
        while (b)
        {
            SG_vhash* pvh_gid_changes_this_parent = NULL;

			SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, psz_csid_parent, &psz_hid_treenode_parent)  );
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_pairs__parent)  );
            SG_ERR_CHECK(  sg_changeset__tree__get_pairs(pCtx, pcs, pRepo, NULL, psz_hid_treenode_parent, pvh_pairs__parent)  );

            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_gid_changes, psz_csid_parent, &pvh_gid_changes_this_parent)  );
            SG_ERR_CHECK(  sg_changeset__tree__gid_changes__two(pCtx, pvh_pairs__me, pvh_pairs__parent, pvh_gid_changes_this_parent)  );

            SG_NULLFREE(pCtx, psz_hid_treenode_parent);
            SG_VHASH_NULLFREE(pCtx, pvh_pairs__parent);

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_csid_parent, NULL)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

        pcs->constructing.tree.pvh_gid_changes = pvh_gid_changes;
        pvh_gid_changes = NULL;
    }
    else
    {
no_parents:
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_gid_changes)  );
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_gid_changes, "", &pvh_pairs__me)  );
        pcs->constructing.tree.pvh_gid_changes = pvh_gid_changes;
        pvh_gid_changes = NULL;
    }

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pcs->constructing.tree.pvh_gid_changes, &count_changes)  );
    if (0 == count_changes)
    {
        SG_VHASH_NULLFREE(pCtx, pcs->constructing.tree.pvh_gid_changes);
    }

fail:
    SG_NULLFREE(pCtx, psz_hid_treenode_parent);
    SG_VHASH_NULLFREE(pCtx, pvh_gid_changes);
    SG_VHASH_NULLFREE(pCtx, pvh_pairs__me);
    SG_VHASH_NULLFREE(pCtx, pvh_pairs__parent);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

static void _sg_changeset__compute_generation(SG_context * pCtx, SG_changeset * pChangeset, SG_repo * pRepo)
{
	// compute the GENERATION of this CHANGESET and stuff the value into the vhash.
	//
	// the generation is defined as  1+max(generation(parents))
	//
	// the generation is the length of the longest dag path from this
	// changeset/commit/dagnode to the null/*root* node.  the initial
	// checkin has g==1. a checkin based on it has g==2.  and so on.
	// for merges (where there are more than one parent), we take the
	// maximum g over the set of parents.
	// this gives us the length of the longest path.
	//
	// to compute this, we must fetch each parent and extract
	// its generation value.  for performance, we fetch the
	// parent dagnode instead of the parent changeset itself.
	// the generation is stored in both places, and the dagnode
	// is smaller.

	SG_int32 gen = 0, gen_k = 0;
    const char* psz_hid_parent = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(pRepo);

	FAIL_IF_FROZEN(pChangeset);

	// fetch the generation for each of the parents.

	gen = 0;

	if (pChangeset->constructing.prb_parents)
    {
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pChangeset->constructing.prb_parents, &b, &psz_hid_parent, NULL)  );
        while (b)
        {
            SG_ERR_CHECK(  _get_generation_from_repo(pCtx, pRepo, pChangeset->constructing.dagnum, psz_hid_parent, &gen_k)  );
            gen = SG_MAX(gen, gen_k);

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_parent, NULL)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

	// set our generation in the vhash.

    pChangeset->constructing.generation = gen+1;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

//////////////////////////////////////////////////////////////////

static void _sg_changeset_v1__validate(SG_context * pCtx, const SG_changeset * pChangeset)
{
	// validate the fields in a VERSION 1 CSET.
	//
	// most of these fields are in the top-level vhash.
	//
	// we assume that the blob list and parents list have been
	// imported into their idsets, since we do not reference the varrays
	// in the _getters_.

	SG_int32 gen = 0;
	SG_uint64 dagnum = 0;
    SG_varray* pva = NULL;

	// parent list is optional -- but make sure that if it is
	// present that it is of the right form.

	SG_changeset__get_parents(pCtx, pChangeset, &pva);
	if (SG_CONTEXT__HAS_ERR(pCtx))
		SG_ERR_RESET_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);

	dagnum = 0;
	SG_changeset__get_dagnum(pCtx, pChangeset, &dagnum);
	if (SG_CONTEXT__HAS_ERR(pCtx))
		SG_ERR_RESET_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);
	if (!dagnum)
		SG_ERR_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);

	// verify that we have a generation value.  we don't bother
	// checking the math -- just verify that we have one.

	SG_changeset__get_generation(pCtx, pChangeset,&gen);
	if (SG_CONTEXT__HAS_ERR(pCtx))
		SG_ERR_RESET_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);
	if (gen <= 0)
		SG_ERR_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);

	// TODO 2010/10/19 Shouldn't we verify that
	// TODO            cset.blobs.{treenode,treeuserfile,...}
	// TODO            are present?
}

static void _sg_changeset__validate(SG_context * pCtx, const SG_changeset * pChangeset, SG_changeset_version * pver)
{
	// validate the given vhash and make sure that it is a well-formed representation
	// of a Changeset.  This is used after parsing a JSON script of incoming
	// information.  The JSON parser only knows JSON syntax; it doesn't know anything
	// about what fields we have or need and the data types (and value ranges) they have.
	//
	// We return the version number that we found on the Changeset and do our
	// validation to that version -- to make sure that it is a well-formed instance
	// of a Changeset of that version.
	//
	// If we get a newer version of a Changeset than this instance of the software
	// knows about, we return an error.
	//
	// we silently map all parsing/inspection errors to a generic
	// SG_ERR_CHANGESET_VALIDATION_FAILED error code.   the caller doesn't need to
	// see our dirty laundry.

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(pver);

	SG_changeset__get_version(pCtx, pChangeset, pver);
	if (SG_CONTEXT__HAS_ERR(pCtx))
		SG_ERR_RESET_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);

	switch (*pver)
	{
	default:
		SG_ERR_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);

	case SG_CSET_VERSION_1:
		_sg_changeset_v1__validate(pCtx, pChangeset);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RESET_THROW_RETURN(SG_ERR_CHANGESET_VALIDATION_FAILED);
		return;
	}
}

static void _sg_changeset__freeze(SG_context * pCtx, SG_changeset * pChangeset, char** ppszidHid)
{
	// this is static because we don't trust anyone to just hand us
	// any old HID and say it matches the value that ***would be***
	// computed from the ***current*** contents of the vhash.
	//
	// we take ownership of the given HID -- you no longer own it.

	SG_ASSERT(pChangeset);
	SG_ASSERT(ppszidHid);
	SG_ASSERT(*ppszidHid);

	if (pChangeset->frozen.psz_hid)
    {
		SG_NULLFREE(pCtx, pChangeset->frozen.psz_hid);
    }

	pChangeset->frozen.psz_hid = *ppszidHid;
    *ppszidHid = NULL;
}

/**
 * Allocate a Changeset structure and populate with the contents of the
 * given JSON stream.  We assume that this was generated by our
 * SG_changeset__to_json() routine.
 */
static void my_parse_and_freeze(SG_context * pCtx, const char * szString, SG_uint32 len, const char* psz_hid, SG_changeset ** ppNew)
{
	SG_changeset * pChangeset;
	SG_changeset_version ver = SG_CSET_VERSION__INVALID;
    char* psz_copy = NULL;

	SG_NULLARGCHECK_RETURN(ppNew);
	SG_NONEMPTYCHECK_RETURN(szString);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pChangeset)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__BUFLEN(pCtx, &pChangeset->frozen.pvh, szString, len)  );

	// make a copy of the HID so that we can stuff it into the Changeset and freeze it.
	// we freeze it because the caller should not be able to modify the Changeset by
	// accident -- since we are a representation of something on disk already.

	SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid, &psz_copy)  );
	_sg_changeset__freeze(pCtx, pChangeset,&psz_copy);

	// validate the fields in the changeset and verify that it is well-formed
	// and defines a version that we understand.

	SG_ERR_CHECK(  _sg_changeset__validate(pCtx, pChangeset, &ver)  );

	*ppNew = pChangeset;
    pChangeset = NULL;

fail:
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
}

//////////////////////////////////////////////////////////////////

static void sg_changeset__estimate_json_size(SG_context * pCtx, SG_changeset * pcs, SG_uint32* piResult)
{
    SG_uint32 hid_length = 40;
    SG_uint32 total = 0;
    SG_varray* pva_parents = NULL;

    total += 32; // dagnum
    total += 8; // ver
    total += 16; // generation

    SG_NULLARGCHECK_RETURN(pcs);

    SG_ERR_CHECK_RETURN(  SG_vhash__check__varray(pCtx, pcs->frozen.pvh, "parents", &pva_parents)  );
    if (pva_parents)
    {
        SG_uint32 count_parents = 0;

        total += 16;
        SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pva_parents, &count_parents)  );
        total += (count_parents * (4 + hid_length));
    }

    if (SG_DAGNUM__IS_TREE(pcs->constructing.dagnum))
    {
        SG_vhash* pvh_tree = NULL;

        total += 16; // keys "tree" and "changes" etc
        total += (16 + hid_length); // root

        SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pcs->frozen.pvh, KEY_TREE, &pvh_tree)  );
        if (pvh_tree)
        {
            SG_vhash* pvh_changes = NULL;

            SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_tree, KEY_TREE__CHANGES, &pvh_changes)  );
            if (pvh_changes)
            {
                SG_uint32 count_parents = 0;
                SG_uint32 i_parent = 0;

                SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
                for (i_parent=0; i_parent<count_parents; i_parent++)
                {
                    SG_vhash* pvh_chg = NULL;
                    const char* psz_csid_parent = NULL;
                    SG_uint32 count_changes = 0;

                    total += (16 + hid_length); // hid key, etc.

                    SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_chg)  );

                    SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_chg, &count_changes)  );
                    total += (count_changes * 
                            (
                             64   // gid for the key
                             + 64 // gid for the dir
                             + 32 // filename
                             + 32 // misc
                             + 72 // punc and keys bits, dir, hid, name, type
                             + hid_length // the hid of the entry
                             ));
                }
            }
        }
    }
    else
    {
        SG_vhash* pvh_db = NULL;

        SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pcs->frozen.pvh, KEY_DB, &pvh_db)  );
        total += 16; // keys "db" and "changes" etc

        if (pvh_db)
        {
            SG_vhash* pvh_changes = NULL;
            SG_varray* pva_all_templates = NULL;
            const char* psz_template = NULL;

            SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_db, KEY_DB__CHANGES, &pvh_changes)  );
            if (pvh_changes)
            {
                SG_uint32 count_parents = 0;
                SG_uint32 i_parent = 0;

                SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
                total += (count_parents * (8 + hid_length)); // hash keys
                total += (count_parents * (24)); // hash keys "add" "remove" etc

                for (i_parent=0; i_parent<count_parents; i_parent++)
                {
                    SG_vhash* pvh_chg = NULL;
                    const char* psz_csid_parent = NULL;
                    SG_vhash* pvh_add = NULL;
                    SG_vhash* pvh_remove = NULL;

                    SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_chg)  );
                    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_chg, "add", &pvh_add)  );
                    if (pvh_add)
                    {
                        SG_uint32 count_add = 0;

                        SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_add, &count_add)  );
                        total += (count_add * (8 + hid_length));
                    }
                    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_chg, "remove", &pvh_remove)  );
                    if (pvh_remove)
                    {
                        SG_uint32 count_remove = 0;

                        SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_remove, &count_remove)  );
                        total += (count_remove * (8 + hid_length));
                    }
                }
            }

            SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvh_db, KEY_DB__TEMPLATE, &psz_template)  );
            if (psz_template)
            {
                total += (16 + hid_length);
            }

            SG_ERR_CHECK_RETURN(  SG_vhash__check__varray(pCtx, pvh_db, KEY_DB__ALL_TEMPLATES, &pva_all_templates)  );
            if (pva_all_templates)
            {
                SG_uint32 count_all_templates = 0;

                SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx, pva_all_templates, &count_all_templates)  );
                total += (count_all_templates * (8 + hid_length));
            }
        }
    }

    *piResult = total;
}

/**
 * Convert the given Changeset to a JSON stream in the given string.
 * The string should be empty (but we do not check or enforce this).
 *
 * We sort various fields within the Changeset before exporting.
 * (That is why we do not make the Changeset 'const'.)
 *
 * Our caller should freeze us ASAP.
 */
static void _sg_changeset__to_json(SG_context * pCtx, SG_changeset * pChangeset, SG_string * pStr)
{
	SG_uint32 estimate = 0;

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(pStr);

    FAIL_IF_FROZEN(pChangeset);

	SG_ERR_CHECK_RETURN(  _sg_changeset__create_the_vhash(pCtx, pChangeset)  );

	//////////////////////////////////////////////////////////////////
	// This must be the last step before we convert to JSON.
	//
	// sort the vhash in increasing key order.  this is necessary
	// so that when we create the HID from the JSON stream we get a
	// consistent value (independent of the order in which the keys
	// were created).
	//
	// WARNING: SG_vhash__sort (even with bRecursive==TRUE) only sorts
	// WARNING: the vhash and the sub vhashes within it -- it DOES NOT
	// WARNING: sort the varrays contained within.  We must sort them
	// WARNING: if we want/need them sorted.  The idea is that a varray
	// WARNING: may contain ordered information.  In our case (we're
	// WARNING: storing 2 lists of IDs), we want them sorted because
	// WARNING: order doesn't matter and we want our HID to be generated
	// WARNING: consistently.  So we sorted them during __create_the_vhash

	SG_ERR_CHECK_RETURN(  SG_vhash__sort(pCtx, pChangeset->frozen.pvh, SG_TRUE, SG_vhash_sort_callback__increasing)  );

	SG_ERR_CHECK(  sg_changeset__estimate_json_size(pCtx, pChangeset, &estimate)  );
	SG_ERR_CHECK(  SG_string__make_space(pCtx, pStr, estimate)  );

	// export the vhash to a JSON stream.  the vhash becomes the top-level
	// object in the JSON stream.

	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pChangeset->frozen.pvh, pStr)  );

#if 0
    fprintf(stderr, "ACCURACY %d -- (%d vs est %d)\n", SG_string__length_in_bytes(pStr) * 100 / estimate, (int) SG_string__length_in_bytes(pStr), (int) estimate);
#endif

    // now this object has transitioned to frozen.  free/invalidate
    // all the struct members except for the vhash and the hid

	SG_RBTREE_NULLFREE(pCtx, pChangeset->constructing.prb_parents);
	SG_NULLFREE(pCtx, pChangeset->constructing.tree.psz_hid_tree_root);
    // TODO free more stuff in constructing here.

    // fall thru

fail:
    ;
}

void SG_changeset__is_frozen(SG_context * pCtx, const SG_changeset * pChangeset, SG_bool * pbFrozen)
{
	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(pbFrozen);

	*pbFrozen = (pChangeset->frozen.psz_hid != NULL);
}

void SG_changeset__get_id_ref(SG_context * pCtx, const SG_changeset * pChangeset, const char** ppszidHidChangeset)
{
	// the caller wants to know our HID -- this is only valid when we are frozen.
	//
	// we give them a const pointer to our actual value.  they ***MUST NOT*** free this.

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(ppszidHidChangeset);

	FAIL_IF_NOT_FROZEN(pChangeset);

	*ppszidHidChangeset = pChangeset->frozen.psz_hid;
}

void SG_changeset__save_to_repo(
	SG_context * pCtx,
	SG_changeset * pChangeset,
	SG_repo * pRepo,
	SG_repo_tx_handle* pRepoTx
	)
{
	SG_string * pString = NULL;
	char* pszHidComputed = NULL;

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(pRepo);

	FAIL_IF_FROZEN(pChangeset);

	// compute the changeset's generation using the set of parent changesets.
	// this is expensive and we only do it right before we freeze/save the
	// changeset.  this computes the generation and stuffs it into the vhash.

	SG_ERR_CHECK(  _sg_changeset__compute_generation(pCtx, pChangeset, pRepo)  );

    if (SG_DAGNUM__IS_TREE(pChangeset->constructing.dagnum))
    {
        if (pChangeset->constructing.tree.psz_hid_tree_root)
        {
            SG_ERR_CHECK(  sg_changeset__tree__calc_gid_change_list(pCtx, pChangeset, pRepo)  );
        }
    }
    else
    {
        if (pChangeset->constructing.db.psz_db_template)
        {
            SG_ERR_CHECK(  sg_changeset__db__calc_all_templates(pCtx, pChangeset, pRepo)  );
        }
    }

	// serialize changeset into JSON string.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  _sg_changeset__to_json(pCtx, pChangeset, pString)  );

#if TRACE_CHANGESET
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,"SG_changeset__save_to_repo:\n%s\n",SG_string__sz(pString))  );
#endif

    // create a blob in the repository using the JSON string.  this computes
    // the HID and returns it.

	SG_ERR_CHECK(  SG_repo__store_blob_from_memory(
		pCtx,
		pRepo, pRepoTx, 
        SG_FALSE, // alwaysfull
		(const SG_byte *)SG_string__sz(pString),
		SG_string__length_in_bytes(pString),
		&pszHidComputed)  );

#if 0
    fprintf(stderr, "changeset %s (%d bytes):\n%s\n", pszHidComputed, (int) SG_string__length_in_bytes(pString), SG_string__sz(pString));
    fflush(stderr);
#endif

	// freeze the changeset memory-object and effectively make it read-only
	// from this point forward.  we give up our ownership of the HID.

	SG_ASSERT(  pszHidComputed  );
	SG_ERR_CHECK(  _sg_changeset__freeze(pCtx, pChangeset, &pszHidComputed)  );

	SG_STRING_NULLFREE(pCtx, pString);
	
	return;  

fail:
	SG_NULLFREE(pCtx, pszHidComputed);
	SG_STRING_NULLFREE(pCtx, pString);
}

void SG_changeset__tree__find_root_hid(
	SG_context * pCtx,
	SG_repo * pRepo,
	const char* pszidHidBlob,
	char** ppszidTreeRootHid
	)
{
	SG_changeset * pChangeset = NULL;
	const char * pszRootHid = NULL;
	SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, pszidHidBlob, &pChangeset)  );
	SG_ERR_CHECK(  SG_changeset__tree__get_root(pCtx, pChangeset, &pszRootHid)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszRootHid, ppszidTreeRootHid)  );

fail:
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
}
void SG_changeset__load_from_repo(
	SG_context * pCtx,
	SG_repo * pRepo,
	const char* pszidHidBlob,
	SG_changeset ** ppChangesetReturned
	)
{
	// fetch contents of a Changeset-type blob and convert to a Changeset object.

	SG_changeset * pChangeset = NULL;
	char* pszidHidCopy = NULL;
    SG_blob* pblob = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pszidHidBlob);
	SG_NULLARGCHECK_RETURN(ppChangesetReturned);

	*ppChangesetReturned = NULL;

	// fetch the Blob for the given HID and convert from a JSON stream into an
	// allocated Changeset object.

	SG_ERR_CHECK(  SG_repo__get_blob(pCtx, pRepo, pszidHidBlob, &pblob)  );
	SG_ERR_CHECK(  my_parse_and_freeze(pCtx, (const char *)pblob->data, (SG_uint32) pblob->length, pszidHidBlob, &pChangeset)  );
	SG_ERR_CHECK(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );

	// make a copy of the HID so that we can stuff it into the Changeset and freeze it.
	// we freeze it because the caller should not be able to modify the Changeset by
	// accident -- since we are a representation of something on disk already.

	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszidHidBlob, &pszidHidCopy)  );
	_sg_changeset__freeze(pCtx, pChangeset,&pszidHidCopy);

	*ppChangesetReturned = pChangeset;
    pChangeset = NULL;

fail:
	SG_ERR_REPLACE(SG_ERR_BLOB_NOT_FOUND, SG_ERR_CHANGESET_BLOB_NOT_FOUND);
	SG_ERR_IGNORE(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
	SG_NULLFREE(pCtx, pszidHidCopy);
}

void SG_changeset__load_from_staging(SG_context* pCtx,
									 const char* pszidHidBlob,
									 SG_staging_blob_handle* pStagingBlobHandle,
									 SG_changeset** ppChangesetReturned)
{
	// fetch contents of a Changeset-type blob and convert to a Changeset object.

	SG_byte * pbuf = NULL;
	SG_changeset * pChangeset = NULL;
	char* pszidHidCopy = NULL;
    SG_uint32 len = 0;

	SG_NULLARGCHECK_RETURN(ppChangesetReturned);

	*ppChangesetReturned = NULL;

	// fetch the Blob for the given HID and convert from a JSON stream into an
	// allocated Changeset object.

	SG_ERR_CHECK(  SG_staging__fetch_blob_into_memory(pCtx, pStagingBlobHandle, &pbuf, &len)  );
	SG_ERR_CHECK(  my_parse_and_freeze(pCtx, (const char *)pbuf, (SG_uint32) len, pszidHidBlob, &pChangeset)  );
	SG_NULLFREE(pCtx, pbuf);

	// make a copy of the HID so that we can stuff it into the Changeset and freeze it.
	// we freeze it because the caller should not be able to modify the Changeset by
	// accident -- since we are a representation of something on disk already.

	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszidHidBlob, &pszidHidCopy)  );
	_sg_changeset__freeze(pCtx, pChangeset,&pszidHidCopy);

	*ppChangesetReturned = pChangeset;
    pChangeset = NULL;

fail:
	SG_NULLFREE(pCtx, pbuf);
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
	SG_NULLFREE(pCtx, pszidHidCopy);
}

//////////////////////////////////////////////////////////////////

void SG_changeset__create_dagnode(
	SG_context * pCtx,
	const SG_changeset * pChangeset,
	SG_dagnode ** ppNewDagnode
	)
{
	SG_dagnode * pdn = NULL;
	SG_int32 gen = 0;
    SG_varray* pva = NULL;

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(ppNewDagnode);

	FAIL_IF_NOT_FROZEN(pChangeset);

	SG_ERR_CHECK( SG_changeset__get_generation(pCtx, pChangeset, &gen) );

	// allocate a new DAGNODE using the CHANGESET's HID (as the child).
	// set the GENERATION on the dagnode to match the CHANGESET.

	// TODO why is the dagnum in the next line 0?  BECAUSE it's not a dagnum, it's a revno
	SG_ERR_CHECK( SG_dagnode__alloc(pCtx, &pdn, pChangeset->frozen.psz_hid, gen, 0)  );

	// add each of the CHANGESET's parents to the set of parents in the DAGNODE.

	SG_ERR_CHECK(  SG_changeset__get_parents(pCtx, pChangeset, &pva)  );
    if (pva)
    {
        SG_ERR_CHECK( SG_dagnode__set_parents__varray(pCtx, pdn, pva) );
    }

	// now freeze the dagnode so that no other parents can be added.

	SG_ERR_CHECK( SG_dagnode__freeze(pCtx, pdn) );

	*ppNewDagnode = pdn;
    pdn = NULL;

fail:
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}

void SG_changeset__get_version(SG_context * pCtx, const SG_changeset * pChangeset, SG_changeset_version * pver)
{
	SG_int64 v = 0;

	SG_NULLARGCHECK_RETURN(pChangeset);
	FAIL_IF_NOT_FROZEN(pChangeset);
	SG_NULLARGCHECK_RETURN(pver);

	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pChangeset->frozen.pvh,KEY_VERSION,&v)  );

    *pver = v;
}

void SG_changeset__get_dagnum(SG_context * pCtx, const SG_changeset * pChangeset, SG_uint64 * piDagNum)
{
    const char* psz_dagnum = NULL;

	SG_NULLARGCHECK_RETURN(pChangeset);
	FAIL_IF_NOT_FROZEN(pChangeset);
	SG_NULLARGCHECK_RETURN(piDagNum);

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pChangeset->frozen.pvh, KEY_DAGNUM, &psz_dagnum)  );
    SG_ERR_CHECK_RETURN(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, piDagNum)  );
}

void SG_changeset__tree__get_root(SG_context * pCtx, const SG_changeset * pChangeset, const char** ppszidHid)
{
    SG_vhash* pvh_tree = NULL;

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(ppszidHid);

	FAIL_IF_NOT_FROZEN(pChangeset);
	FAIL_IF_DB_DAG(pChangeset);

    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pChangeset->frozen.pvh, KEY_TREE, &pvh_tree)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvh_tree, KEY_TREE__ROOT, ppszidHid)  );
}

void SG_changeset__get_generation(SG_context * pCtx, const SG_changeset * pChangeset, SG_int32 * pGen)
{
	SG_int64 gen64;

	SG_NULLARGCHECK_RETURN(pChangeset);
    FAIL_IF_NOT_FROZEN(pChangeset);
	SG_NULLARGCHECK_RETURN(pGen);

	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pChangeset->frozen.pvh,KEY_GENERATION,&gen64)  );

	*pGen = (SG_int32)gen64;
}

void SG_changeset__get_parents(
	SG_context * pCtx,
	const SG_changeset * pChangeset,
    SG_varray** ppva
	)
{
    SG_bool b = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pChangeset);
	FAIL_IF_NOT_FROZEN(pChangeset);
	SG_NULLARGCHECK_RETURN(ppva);

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pChangeset->frozen.pvh, KEY_PARENTS, &b)  );
    if (b)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pChangeset->frozen.pvh, KEY_PARENTS, ppva)  );
    }
    else
    {
        *ppva = NULL;
    }
}

void SG_changeset__tree__get_changes(
	SG_context * pCtx,
	const SG_changeset * pChangeset,
    SG_vhash** ppvh
	)
{
    SG_vhash* pvh_tree = NULL;

	SG_NULLARGCHECK_RETURN(pChangeset);
	SG_NULLARGCHECK_RETURN(ppvh);

	FAIL_IF_NOT_FROZEN(pChangeset);
	FAIL_IF_DB_DAG(pChangeset);

    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pChangeset->frozen.pvh, KEY_TREE, &pvh_tree)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_tree, KEY_TREE__CHANGES, ppvh)  );
}

void SG_changeset__db__get_changes(
	SG_context * pCtx,
	const SG_changeset* pcs,
    SG_vhash** ppvh
    )
{
    SG_vhash* pvh_db = NULL;

	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(ppvh);

	FAIL_IF_NOT_FROZEN(pcs);

    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pcs->frozen.pvh, KEY_DB, &pvh_db)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_db, KEY_DB__CHANGES, ppvh)  );
}

void SG_changeset__db__get_template(
	SG_context * pCtx,
	const SG_changeset* pcs,
    const char** ppsz
    )
{
    SG_vhash* pvh_db = NULL;

	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(ppsz);

	FAIL_IF_NOT_FROZEN(pcs);

    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pcs->frozen.pvh, KEY_DB, &pvh_db)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvh_db, KEY_DB__TEMPLATE, ppsz)  );
}

void SG_changeset__db__set_template(
    SG_context* pCtx,
    SG_changeset* pcs,
    const char* psz_template
    )
{
	SG_NULLARGCHECK_RETURN(pcs);

	FAIL_IF_FROZEN(pcs);
	FAIL_IF_NOT_DB_DAG(pcs);

    SG_ASSERT(!pcs->constructing.db.psz_db_template);

    if (psz_template)
    {
        SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx, psz_template, &pcs->constructing.db.psz_db_template)  );
    }
}

static void remove_sub_if_empty(
    SG_context* pCtx,
    SG_vhash* pvh,
    const char* psz
    )
{
    SG_vhash* pvh_sub = NULL;
    SG_uint32 count = 0;

    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh, psz, &pvh_sub)  );
    if (pvh_sub)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_sub, &count)  );
        if (0 == count)
        {
            SG_ERR_CHECK_RETURN(  SG_vhash__remove(pCtx, pvh, psz)  );
        }
    }
}

void SG_changeset__db__set_parent_delta(
    SG_context* pCtx,
    SG_changeset* pcs,
    const char* psz_csid_parent,
    SG_vhash** ppvh_delta
    )
{
	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(ppvh_delta);
	SG_NULLARGCHECK_RETURN(*ppvh_delta);
	SG_NULLARGCHECK_RETURN(psz_csid_parent);

	FAIL_IF_FROZEN(pcs);
	FAIL_IF_NOT_DB_DAG(pcs);

    SG_ERR_CHECK_RETURN(  remove_sub_if_empty(pCtx, *ppvh_delta, "add")  );
    SG_ERR_CHECK_RETURN(  remove_sub_if_empty(pCtx, *ppvh_delta, "remove")  );
    SG_ERR_CHECK_RETURN(  remove_sub_if_empty(pCtx, *ppvh_delta, "attach_add")  );

    SG_ERR_CHECK_RETURN(  SG_vhash__add__vhash(pCtx, pcs->constructing.db.pvh_changes, psz_csid_parent, ppvh_delta)  );
}

void SG_changeset__tree__add_treenode(
    SG_context * pCtx,
    SG_changeset* pcs,
    const char* psz_hid_treenode,
    SG_treenode** ppTreenode
    )
{
    SG_treenode* ptn_already = NULL;

    SG_NULLARGCHECK_RETURN(pcs);
    SG_NULLARGCHECK_RETURN(ppTreenode);
    SG_NULLARGCHECK_RETURN(*ppTreenode);

    SG_ERR_CHECK_RETURN(  SG_rbtree__update__with_assoc(pCtx, pcs->constructing.tree.prb_treenode_cache, psz_hid_treenode, *ppTreenode, (void**) &ptn_already)  );

    if (ptn_already)
    {
        if (ptn_already != *ppTreenode)
        {
            SG_TREENODE_NULLFREE(pCtx, ptn_already);
        }
    }

    *ppTreenode = NULL;
}

// TODO do we need this routine anymore?
void SG_changeset__check_gid_change(
	SG_context * pCtx,
	const SG_changeset* pcs,
    const char* psz_gid,
	SG_bool bHideMerges,
    SG_bool* pb
    )
{
    SG_vhash* pvh_tree = NULL;
    SG_vhash* pvh_changes = NULL;

	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(pb);

	FAIL_IF_NOT_FROZEN(pcs);

    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pcs->frozen.pvh, KEY_TREE, &pvh_tree)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_tree, KEY_TREE__CHANGES, &pvh_changes)  );
    if (pvh_changes)
    {
        SG_uint32 count_parents = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
        for (i=0; i<count_parents; i++)
        {
            SG_vhash* pvh_chg = NULL;
            SG_bool b = SG_FALSE;
            const char* psz_csid_parent = NULL;

            SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i, &psz_csid_parent, &pvh_chg)  );
            SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_chg, psz_gid, &b)  );
            if (b)
            {
				if (bHideMerges == SG_FALSE)
				{
					*pb = SG_TRUE;
					return;
				}
            }
			else
			{
				//When you are hiding merges, if the gid has not changed
				//relative to one of the parents, then you should skip it.
				if (bHideMerges == SG_TRUE)
				{
					*pb = SG_FALSE;
					return;
				}
			}
        }
		if (bHideMerges == SG_TRUE)
		{
			*pb = SG_TRUE;
			return;
		}
    }
    *pb = SG_FALSE;
}

void SG_changeset__db__get_all_templates(
	SG_context * pCtx,
	const SG_changeset* pcs,
    SG_varray** ppva
    )
{
    SG_vhash* pvh_db = NULL;

	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(ppva);

	FAIL_IF_NOT_FROZEN(pcs);

    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pcs->frozen.pvh, KEY_DB, &pvh_db)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__check__varray(pCtx, pvh_db, KEY_DB__ALL_TEMPLATES, ppva)  );
}

void SG_changeset__get_vhash_ref(
	SG_context * pCtx,
	const SG_changeset* pcs,
    SG_vhash** ppvh
    )
{
	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(ppvh);

	FAIL_IF_NOT_FROZEN(pcs);

    *ppvh = pcs->frozen.pvh;
}


