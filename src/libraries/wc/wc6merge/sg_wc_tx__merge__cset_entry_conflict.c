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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry_conflict__alloc(SG_context * pCtx,
									   SG_mrg_cset * pMrgCSet,
									   SG_mrg_cset_entry * pMrgCSetEntry_Ancestor,
									   SG_mrg_cset_entry * pMrgCSetEntry_Baseline,
									   SG_mrg_cset_entry_conflict ** ppMrgCSetEntryConflict)
{
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = NULL;

	SG_NULLARGCHECK_RETURN(pMrgCSet);
	SG_NULLARGCHECK_RETURN(pMrgCSetEntry_Ancestor);
	// pMrgCSetEntry_Baseline may or may not be null
	SG_NULLARGCHECK_RETURN(ppMrgCSetEntryConflict);

	// we allocate and return this.  we DO NOT automatically add it to
	// the conflict-list in the cset.

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx,pMrgCSetEntryConflict)  );

	pMrgCSetEntryConflict->pMrgCSet = pMrgCSet;
	pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor = pMrgCSetEntry_Ancestor;
	pMrgCSetEntryConflict->pMrgCSetEntry_Baseline = pMrgCSetEntry_Baseline;
	// defer alloc of vectors until we need them.
	// defer alloc of rbtrees until we need them.

	pMrgCSetEntryConflict->pMergeTool = NULL;

	*ppMrgCSetEntryConflict = pMrgCSetEntryConflict;
}

void SG_mrg_cset_entry_conflict__free(SG_context * pCtx,
									  SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict)
{
	if (!pMrgCSetEntryConflict)
		return;

	SG_VECTOR_I64_NULLFREE(pCtx, pMrgCSetEntryConflict->pVec_MrgCSetEntryNeq_Changes);
	SG_VECTOR_NULLFREE(pCtx, pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes);	// we do not own the pointers within
	SG_VECTOR_NULLFREE(pCtx, pMrgCSetEntryConflict->pVec_MrgCSet_Deletes);		// we do not own the pointers within

	// for the collapsable rbUnique's we own the vectors in the rbtree-values, but not the pointers within the vector
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pMrgCSetEntryConflict->prbUnique_AttrBits,          (SG_free_callback *)SG_vector__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pMrgCSetEntryConflict->prbUnique_Entryname,         (SG_free_callback *)SG_vector__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pMrgCSetEntryConflict->prbUnique_GidParent,         (SG_free_callback *)SG_vector__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob,   (SG_free_callback *)SG_vector__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob, (SG_free_callback *)SG_vector__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pMrgCSetEntryConflict->prbUnique_File_HidBlob,      (SG_free_callback *)SG_vector__free);

	SG_VECTOR_NULLFREE(pCtx, pMrgCSetEntryConflict->pVec_MrgCSetEntry_OtherDirsInCycle);	// we do not own the pointers within
	SG_STRING_NULLFREE(pCtx, pMrgCSetEntryConflict->pStringPathCycleHint);

	SG_FILETOOL_NULLFREE(pCtx, pMrgCSetEntryConflict->pMergeTool);

	SG_PATHNAME_NULLFREE(pCtx, pMrgCSetEntryConflict->pPathTempDirForFile);
	SG_PATHNAME_NULLFREE(pCtx, pMrgCSetEntryConflict->pPathTempFile_Ancestor);
	SG_PATHNAME_NULLFREE(pCtx, pMrgCSetEntryConflict->pPathTempFile_Result);
	SG_PATHNAME_NULLFREE(pCtx, pMrgCSetEntryConflict->pPathTempFile_Baseline);
	SG_PATHNAME_NULLFREE(pCtx, pMrgCSetEntryConflict->pPathTempFile_Other);

	SG_NULLFREE(pCtx, pMrgCSetEntryConflict->pszHidDisposable);
	SG_NULLFREE(pCtx, pMrgCSetEntryConflict->pszHidGenerated);

	SG_NULLFREE(pCtx, pMrgCSetEntryConflict);
}

//////////////////////////////////////////////////////////////////

/**
 * The values for RENAME, MOVE, ATTRBITS, SYMLINKS, and SUBMODULES are collapsable.  (see below)
 * In the corresponding rbUnique's we only need to remember the set of unique values for the
 * field.  THESE ARE THE KEYS IN THE prbUnique.
 *
 * As a convenience, we associate a vector of entries with each key.  These form a many-to-one
 * thing so that we can report all of the entries that have this value.
 *
 * Here we carry-forward the values from a sub-merge to the outer-merge by coping the keys
 * in the source-rbtree and insert in the destination-rbtree.
 *
 * NOTE: the term sub-merge here refers to the steps within an n-way merge;
 * it DOES NOT refer to a submodule.
 */
static void _carry_forward_unique_values(SG_context * pCtx,
										 SG_rbtree * prbDest,
										 SG_rbtree * prbSrc)
{
	SG_rbtree_iterator * pIter = NULL;
	SG_vector * pVec_Allocated = NULL;
	const char * pszKey;
	SG_vector * pVec_Src;
	SG_vector * pVec_Dest;
	SG_uint32 j, nr;
	SG_bool bFound;


	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,&pIter,prbSrc,&bFound,&pszKey,(void **)&pVec_Src)  );
	while (bFound)
	{
		SG_ERR_CHECK(  SG_rbtree__find(pCtx,prbDest,pszKey,&bFound,(void **)&pVec_Dest)  );
		if (!bFound)
		{
			SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx,&pVec_Allocated,3)  );
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx,prbDest,pszKey,pVec_Allocated)  );
			pVec_Dest = pVec_Allocated;
			pVec_Allocated = NULL;			// rbtree owns this now
		}

		SG_ERR_CHECK(  SG_vector__length(pCtx,pVec_Src,&nr)  );
		for (j=0; j<nr; j++)
		{
			SG_mrg_cset_entry * pMrgCSetEntry_x;

			SG_ERR_CHECK(  SG_vector__get(pCtx,pVec_Src,j,(void **)&pMrgCSetEntry_x)  );
			SG_ERR_CHECK(  SG_vector__append(pCtx,pVec_Dest,pMrgCSetEntry_x,NULL)  );

#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,"_carry_forward_unique_value: [%s][%s]\n",
									   pszKey,
									   SG_string__sz(pMrgCSetEntry_x->pMrgCSet->pStringCSetLabel))  );
#endif
		}

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx,pIter,&bFound,&pszKey,NULL)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx,pIter);
}

//////////////////////////////////////////////////////////////////

/**
 * The values for RENAME, MOVE, ATTRBITS, SYMLINKS, and SUBMODULES are collapsable.  (see below)
 * In the corresponding rbUnique's we only need to remember the set of unique values for the
 * field.  THESE ARE THE KEYS IN THE prbUnique.
 *
 * As a convenience, we associate a vector of entries with each key.  These form a many-to-one
 * thing so that we can report all of the entries that have this value.
 *
 * TODO since we should only process a cset once, we should not get any
 * TODO duplicates in the vector, but it might be possible.  i'm not going
 * TODO to worry about it now.  if this becomes a problem, consider doing
 * TODO a unique-insert into the vector -or- making the vector a sub-rbtree.
 *
 */
static void _update_1_rbUnique(SG_context * pCtx, SG_rbtree * prbUnique, const char * pszKey, SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k)
{
	SG_vector * pVec_Allocated = NULL;
	SG_vector * pVec;
	SG_bool bFound;

	SG_ERR_CHECK(  SG_rbtree__find(pCtx,prbUnique,pszKey,&bFound,(void **)&pVec)  );
	if (!bFound)
	{
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx,&pVec_Allocated,3)  );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx,prbUnique,pszKey,pVec_Allocated)  );
		pVec = pVec_Allocated;
		pVec_Allocated = NULL;			// rbtree owns this now
	}

	SG_ERR_CHECK(  SG_vector__append(pCtx,pVec,pMrgCSetEntry_Leaf_k,NULL)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,"_update_1_rbUnique: [%s][%s]\n",
									   pszKey,
									   SG_string__sz(pMrgCSetEntry_Leaf_k->pMrgCSet->pStringCSetLabel))  );
#endif

	return;

fail:
	SG_VECTOR_NULLFREE(pCtx, pVec_Allocated);
}

void SG_mrg_cset_entry_conflict__append_change(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k,
											   SG_mrg_cset_entry_neq neq)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);
	SG_NULLARGCHECK_RETURN(pMrgCSetEntry_Leaf_k);

	if (!pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes)
		SG_ERR_CHECK_RETURN(  SG_vector__alloc(pCtx,&pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes,2)  );

	SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx,pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes,(void *)pMrgCSetEntry_Leaf_k,NULL)  );

	if (!pMrgCSetEntryConflict->pVec_MrgCSetEntryNeq_Changes)
		SG_ERR_CHECK_RETURN(  SG_vector_i64__alloc(pCtx,&pMrgCSetEntryConflict->pVec_MrgCSetEntryNeq_Changes,2)  );

	SG_ERR_CHECK_RETURN(  SG_vector_i64__append(pCtx,pMrgCSetEntryConflict->pVec_MrgCSetEntryNeq_Changes,(SG_int64)neq,NULL)  );

	//////////////////////////////////////////////////////////////////
	// add the value of the changed fields to the prbUnique_ rbtrees so that we can get a count of the unique new values.
	//
	//////////////////////////////////////////////////////////////////
	// the values for RENAME, MOVE, ATTRBITS, SYMLINKS, and SUBMODULES are collapsable.  that is, if we
	// have something like:
	//        A
	//       / \.
	//     L0   a0
	//         /  \.
	//        L1   L2
	//
	// and a rename in each Leaf, then we can either:
	// [a] prompt for them to choose L1 or L2's name and then
	//     prompt for them to choose L0 or the name from step 1.
	//
	// [b] prompt for them to choose L0, L1, or L2 in one question.
	//
	// unlike file-content-merging, the net-net is that we have 1 new value
	// that is one of the inputs (or maybe we let them pick a new onw), but
	// it is not a combination of them and so we don't need to display the
	// immediate ancestor in the prompt.
	//
	// so we carry-forward the unique values from the leaves for each of
	// these fields.  so the final merge-result may have more unique values
	// that it has direct parents.
	//////////////////////////////////////////////////////////////////

	if (neq & SG_MRG_CSET_ENTRY_NEQ__ATTRBITS)
	{
		SG_int_to_string_buffer buf;
		SG_int64_to_sz((SG_int64)pMrgCSetEntry_Leaf_k->attrBits, buf);

		if (!pMrgCSetEntryConflict->prbUnique_AttrBits)
			SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pMrgCSetEntryConflict->prbUnique_AttrBits)  );

		SG_ERR_CHECK_RETURN(  _update_1_rbUnique(pCtx,pMrgCSetEntryConflict->prbUnique_AttrBits,buf,pMrgCSetEntry_Leaf_k)  );

		if (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict && pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_AttrBits)
			SG_ERR_CHECK_RETURN(  _carry_forward_unique_values(pCtx,
															   pMrgCSetEntryConflict->prbUnique_AttrBits,
															   pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_AttrBits)  );
	}

	if (neq & SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME)
	{
		if (!pMrgCSetEntryConflict->prbUnique_Entryname)
			SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pMrgCSetEntryConflict->prbUnique_Entryname)  );

		SG_ERR_CHECK_RETURN(  _update_1_rbUnique(pCtx,
												 pMrgCSetEntryConflict->prbUnique_Entryname,
												 SG_string__sz(pMrgCSetEntry_Leaf_k->pStringEntryname),
												 pMrgCSetEntry_Leaf_k)  );

		if (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict && pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_Entryname)
			SG_ERR_CHECK_RETURN(  _carry_forward_unique_values(pCtx,
															   pMrgCSetEntryConflict->prbUnique_Entryname,
															   pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_Entryname)  );
	}

	if (neq & SG_MRG_CSET_ENTRY_NEQ__GID_PARENT)
	{
		if (!pMrgCSetEntryConflict->prbUnique_GidParent)
			SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pMrgCSetEntryConflict->prbUnique_GidParent)  );

		SG_ERR_CHECK_RETURN(  _update_1_rbUnique(pCtx,pMrgCSetEntryConflict->prbUnique_GidParent,pMrgCSetEntry_Leaf_k->bufGid_Parent,pMrgCSetEntry_Leaf_k)  );

		if (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict && pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_GidParent)
			SG_ERR_CHECK_RETURN(  _carry_forward_unique_values(pCtx,
															   pMrgCSetEntryConflict->prbUnique_GidParent,
															   pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_GidParent)  );
	}

	if (neq & SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB)
	{
		if (!pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob)
			SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob)  );

		SG_ERR_CHECK_RETURN(  _update_1_rbUnique(pCtx,pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob,pMrgCSetEntry_Leaf_k->bufHid_Blob,pMrgCSetEntry_Leaf_k)  );

		if (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict && pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob)
			SG_ERR_CHECK_RETURN(  _carry_forward_unique_values(pCtx,
															   pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob,
															   pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob)  );
	}

	if (neq & SG_MRG_CSET_ENTRY_NEQ__SUBMODULE_HID_BLOB)
	{
		if (!pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob)
			SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob)  );

		SG_ERR_CHECK_RETURN(  _update_1_rbUnique(pCtx,pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob,pMrgCSetEntry_Leaf_k->bufHid_Blob,pMrgCSetEntry_Leaf_k)  );

		if (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict && pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob)
			SG_ERR_CHECK_RETURN(  _carry_forward_unique_values(pCtx,
															   pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob,
															   pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob)  );
	}

	// 2010/09/13 Update: we now do the carry-forward on the set of
	//            unique HIDs for the various versions of the file
	//            content from each of the leaves.  This lets us
	//            completely flatten the sub-merges into one final
	//            result (with upto n values).
	//
	//            This means we won't be creating the auto-merge-plan
	//            at this point.
	//
	//            The problem with the auto-merge-plan as originally
	//            designed is that it was being driven based upon
	//            the overall topology of the DAG as a whole rather
	//            than the topology/history of the individual file.
	//            And by respecting the history of the individual
	//            file, I think we can get closer ancestors and better
	//            per-file merging and perhaps fewer criss-crosses
	//            and/or we push all of these issues to RESOLVE.

	if (neq & SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB)
	{
		if (!pMrgCSetEntryConflict->prbUnique_File_HidBlob)
			SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pMrgCSetEntryConflict->prbUnique_File_HidBlob)  );

		SG_ASSERT(  (pMrgCSetEntry_Leaf_k->bufHid_Blob[0])  );

		// TODO 2010/09/13 the code that sets __FILE_HID_BLOB probably cannot tell
		// TODO            whether this branch did not change the file content
		// TODO            relative to the LCA or whether it did change it back to
		// TODO            the original value (an UNDO of the edits).  I would argue
		// TODO            that we should not list the former as a change, but that
		// TODO            we SHOULD list the latter.  The fix doesn't belong here,
		// TODO            but this is just where I was typing when I thought of it.
		
		SG_ERR_CHECK_RETURN(  _update_1_rbUnique(pCtx,pMrgCSetEntryConflict->prbUnique_File_HidBlob,pMrgCSetEntry_Leaf_k->bufHid_Blob,pMrgCSetEntry_Leaf_k)  );

		if (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict && pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_File_HidBlob)
			SG_ERR_CHECK_RETURN(  _carry_forward_unique_values(pCtx,
															   pMrgCSetEntryConflict->prbUnique_File_HidBlob,
															   pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict->prbUnique_File_HidBlob)  );
	}
}

void SG_mrg_cset_entry_conflict__append_delete(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_mrg_cset * pMrgCSet_Leaf_k)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);
	SG_NULLARGCHECK_RETURN(pMrgCSet_Leaf_k);

	if (!pMrgCSetEntryConflict->pVec_MrgCSet_Deletes)
		SG_ERR_CHECK_RETURN(  SG_vector__alloc(pCtx,&pMrgCSetEntryConflict->pVec_MrgCSet_Deletes,2)  );

	SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx,pMrgCSetEntryConflict->pVec_MrgCSet_Deletes,(void *)pMrgCSet_Leaf_k,NULL)  );
}

//////////////////////////////////////////////////////////////////

/**
 * return the number of leaves that deleted the entry.
 */
void SG_mrg_cset_entry_conflict__count_deletes(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->pVec_MrgCSet_Deletes)
		SG_ERR_CHECK_RETURN(  SG_vector__length(pCtx,pMrgCSetEntryConflict->pVec_MrgCSet_Deletes,pCount)  );
	else
		*pCount = 0;
}

/**
 * return the number of leaves that changed the entry in some way.
 */
void SG_mrg_cset_entry_conflict__count_changes(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes)
		SG_ERR_CHECK_RETURN(  SG_vector__length(pCtx,pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes,pCount)  );
	else
		*pCount = 0;
}

//////////////////////////////////////////////////////////////////

/**
 * if more than one leaf changed the attrbits, return the number of unique new values that it was set to.
 */
void SG_mrg_cset_entry_conflict__count_unique_attrbits(SG_context * pCtx,
													   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
													   SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->prbUnique_AttrBits)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSetEntryConflict->prbUnique_AttrBits,pCount)  );
	else
		*pCount = 0;
}

/**
 * if more than one leaf renamed the entry, return the number of unique new values that it was set to.
 */
void SG_mrg_cset_entry_conflict__count_unique_entryname(SG_context * pCtx,
														SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
														SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->prbUnique_Entryname)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSetEntryConflict->prbUnique_Entryname,pCount)  );
	else
		*pCount = 0;
}

/**
 * if more than one leaf moved the entry (to another directory), return the number of unique new values that it was set to.
 */
void SG_mrg_cset_entry_conflict__count_unique_gid_parent(SG_context * pCtx,
														 SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
														 SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->prbUnique_GidParent)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSetEntryConflict->prbUnique_GidParent,pCount)  );
	else
		*pCount = 0;
}

/**
 * if more than one leaf changed the contents of the symlink, return the number of
 * unique new values that it was set to.
 */
void SG_mrg_cset_entry_conflict__count_unique_symlink_hid_blob(SG_context * pCtx,
															   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
															   SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob,pCount)  );
	else
		*pCount = 0;
}

/**
 * if more than one leaf changed the contents of the submodule, return the number of
 * unique new values that it was set to.
 */
void SG_mrg_cset_entry_conflict__count_unique_submodule_hid_blob(SG_context * pCtx,
																 SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
																 SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob,pCount)  );
	else
		*pCount = 0;
}

/**
 * if more than one leaf changed the contents of the file, return the number of
 * unique new values that it was set to.
 */
void SG_mrg_cset_entry_conflict__count_unique_file_hid_blob(SG_context * pCtx,
															SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
															SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntryConflict);

	if (pMrgCSetEntryConflict->prbUnique_File_HidBlob)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSetEntryConflict->prbUnique_File_HidBlob,pCount)  );
	else
		*pCount = 0;
}
