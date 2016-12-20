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

#define MARKER_UNHANDLED			0
#define MARKER_LCA_PASS				1
#define MARKER_SPCA_PASS			2
#define MARKER_ADDITION_PASS		3
#define MARKER_FIXUP_PASS			4

//////////////////////////////////////////////////////////////////

#if TRACE_WC_MERGE
void _sg_wc_tx__merge__trace_msg__v_input(SG_context * pCtx,
										  const char * pszLabel,
										  const char * pszCSetLabel,
										  SG_mrg_cset * pMrgCSet_Ancestor,
										  SG_vector * pVec_MrgCSet_Leaves)
{
	SG_mrg_cset * pMrgCSet_Leaf_k;
	SG_uint32 nrLeaves, kLeaf;

	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,("%s:\n"
												  "    Result:   [name    %s]\n"
												  "    Ancestor: [hidCSet %s] %s\n"),
							   pszLabel,
							   ((pszCSetLabel) ? pszCSetLabel : "(null)"),
							   pMrgCSet_Ancestor->bufHid_CSet,
							   SG_string__sz(pMrgCSet_Ancestor->pStringCSetLabel))  );

	SG_ERR_CHECK_RETURN(  SG_vector__length(pCtx,pVec_MrgCSet_Leaves,&nrLeaves)  );

	for (kLeaf=0; kLeaf<nrLeaves; kLeaf++)
	{
		SG_ERR_CHECK_RETURN(  SG_vector__get(pCtx,pVec_MrgCSet_Leaves,kLeaf,(void **)&pMrgCSet_Leaf_k)  );
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,("    Leaf[%02d]: [hidCSet %s] %s\n"),
								   kLeaf,pMrgCSet_Leaf_k->bufHid_CSet,
								   SG_string__sz(pMrgCSet_Leaf_k->pStringCSetLabel))  );
	}
}

void _sg_wc_tx__merge__trace_msg__v_ancestor(SG_context * pCtx,
											 const char * pszLabel,
											 SG_int32 marker_value,
											 SG_mrg_cset * pMrgCSet_Ancestor,
											 const char * pszGid_Entry)
{
	SG_string * pString_temp = NULL;

	// compute repo-path as it existed in ancestor.
	SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
															  pMrgCSet_Ancestor,
															  pszGid_Entry,
															  &pString_temp)  );
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "%s[%d]: evaluating [ancestor entry %s] [in %s/%s] [%s]\n",
							   pszLabel, marker_value, pszGid_Entry,
							   pMrgCSet_Ancestor->pszMnemonicName,
							   SG_string__sz(pMrgCSet_Ancestor->pStringCSetLabel),
							   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );

	SG_STRING_NULLFREE(pCtx,pString_temp);
}

void _sg_wc_tx__merge__trace_msg__v_leaf_eq_neq(SG_context * pCtx,
												const char * pszLabel,
												SG_int32 marker_value,
												SG_uint32 kLeaf,
												SG_mrg_cset * pMrgCSet_Leaf_k,
												const char * pszGid_Entry,
												SG_mrg_cset_entry_neq neq)
{
	SG_string * pString_temp = NULL;

	// compute repo-path as it existed in the leaf.
	SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
															 pMrgCSet_Leaf_k,
															 pszGid_Entry,
															 &pString_temp)  );
	if (neq == SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL)
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "%s[%d]: [entry %s] not changed in leaf[%02d] neq[%02x] [%s]\n",
								   pszLabel, marker_value, pszGid_Entry,kLeaf,neq,
								   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );
	else
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "%s[%d]: [entry %s]     changed in leaf[%02d] neq[%02x] [%s]\n",
								   pszLabel, marker_value, pszGid_Entry,kLeaf,neq,
								   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );

	SG_STRING_NULLFREE(pCtx, pString_temp);
}

void _sg_wc_tx__merge__trace_msg__v_leaf_deleted(SG_context * pCtx,
												 const char * pszLabel,
												 SG_int32 marker_value,
												 SG_mrg_cset * pMrgCSet_Ancestor,
												 SG_uint32 kLeaf,
												 const char * pszGid_Entry)
{
	SG_string * pString_temp = NULL;

	// compute repo-path as it existed in the ancestor (since the entry (and possibly
	// the parent dir) was deleted in the leaf).
	SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
															  pMrgCSet_Ancestor,
															  pszGid_Entry,
															  &pString_temp)  );
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "%s[%d]: [entry %s] deleted in leaf[%02d] [%s]\n",
							   pszLabel, marker_value, pszGid_Entry,kLeaf,
							   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );

	SG_STRING_NULLFREE(pCtx, pString_temp);
}

void _sg_wc_tx__merge__trace_msg__v_leaf_added(SG_context * pCtx,
											   const char * pszLabel,
											   SG_int32 marker_value,
											   SG_mrg_cset * pMrgCSet_Leaf_k,
											   SG_uint32 kLeaf,
											   SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k)
{
	SG_string * pString_temp = NULL;

	// an entry was added by the leaf.
	// compute repo-path as it existed in the leaf.
	SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
															  pMrgCSet_Leaf_k,
															  pMrgCSetEntry_Leaf_k->bufGid_Entry,
															  &pString_temp)  );
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "%s[%d]: [entry %s] added in leaf[%02d] [parent gid %s] [%s]\n",
							   pszLabel, marker_value, pMrgCSetEntry_Leaf_k->bufGid_Entry,kLeaf,
							   pMrgCSetEntry_Leaf_k->bufGid_Parent,
							   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );

	SG_STRING_NULLFREE(pCtx, pString_temp);
}

void _sg_wc_tx__merge__trace_msg__v_leaf_peer(SG_context * pCtx,
											  const char * pszLabel,
											  SG_int32 marker_value,
											  SG_mrg_cset * pMrgCSet_Leaf_j,
											  SG_uint32 jLeaf,
											  SG_mrg_cset_entry * pMrgCSetEntry_Leaf_j)
{
	SG_string * pString_temp = NULL;

	// compute repo-path as it existed in the leaf.
	SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
															  pMrgCSet_Leaf_j,
															  pMrgCSetEntry_Leaf_j->bufGid_Entry,
															  &pString_temp)  );
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "%s[%d]: [entry %s] peer  in leaf[%02d] [parent gid %s] [%s]\n",
							   pszLabel, marker_value, pMrgCSetEntry_Leaf_j->bufGid_Entry,jLeaf,
							   pMrgCSetEntry_Leaf_j->bufGid_Parent,
							   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );

	SG_STRING_NULLFREE(pCtx, pString_temp);
}

static SG_rbtree_foreach_callback _sg_wc_tx__merge__trace_msg__dump_list_member__mrg_cset_entry;

static void _sg_wc_tx__merge__trace_msg__dump_list_member__mrg_cset_entry(SG_context * pCtx,
																		  const char * pszKey_Gid_Entry,
																		  void * pVoidAssocData_MrgCSetEntry,
																		  SG_UNUSED_PARAM(void * pVoid_Data))
{
	SG_mrg_cset_entry * pMrgCSetEntry = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry;
	SG_string * pString_temp = NULL;

	SG_UNUSED(pVoid_Data);

	SG_ASSERT(  (strcmp(pszKey_Gid_Entry,pMrgCSetEntry->bufGid_Entry) == 0)  );

	// compute the repo-path as it existed in the version of the tree that this entry belongs to.
	SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
															  pMrgCSetEntry->pMrgCSet,
															  pMrgCSetEntry->bufGid_Entry,
															  &pString_temp)  );
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "    [gid %s] %s\n",
							   pszKey_Gid_Entry,
							   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );

	SG_STRING_NULLFREE(pCtx, pString_temp);
}
	
static void _sg_wc_tx__merge__trace_msg__dump_delete_list(SG_context * pCtx,
														  const char * pszLabel,
														  SG_mrg_cset * pMrgCSet_Result)
{
	SG_uint32 nr = 0;

	if (pMrgCSet_Result->prbDeletes)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSet_Result->prbDeletes,&nr)  );

	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "%s: [result-cset %s] delete list: [count %d]\n",
							   pszLabel,SG_string__sz(pMrgCSet_Result->pStringCSetLabel),nr)  );
	if (nr > 0)
		SG_ERR_IGNORE(  SG_rbtree__foreach(pCtx,pMrgCSet_Result->prbDeletes,_sg_wc_tx__merge__trace_msg__dump_list_member__mrg_cset_entry,NULL)  );
}

//////////////////////////////////////////////////////////////////

struct _sg_mrg_cset_entry_conflict_flag_msgs
{
	SG_mrg_cset_entry_conflict_flags		bit;
	const char *							pszMsg;
};

typedef struct _sg_mrg_cset_entry_conflict_flag_msgs SG_mrg_cset_entry_conflict_flags_msg;

static SG_mrg_cset_entry_conflict_flags_msg aFlagMsgs[] =
{ 
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE,				"REMOVE-vs-MOVE" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_RENAME,				"REMOVE-vs-RENAME" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_ATTRBITS,			"REMOVE-vs-ATTRBITS_CHANGE" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT,		"REMOVE-vs-SYMLINK_EDIT" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT,			"REMOVE-vs-FILE_EDIT" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SUBMODULE_EDIT,		"REMOVE-vs-SUBMODULE_EDIT" },

	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN,			"REMOVE-CAUSED-ORPHAN" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE,		"MOVES-CAUSED-PATH_CYCLE" },

	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE,				"DIVERGENT-MOVE" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME,				"DIVERGENT-RENAME" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_ATTRBITS,			"DIVERGENT-ATTRBITS_CHANGE" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT,		"DIVERGENT-SYMLINK_EDIT" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SUBMODULE_EDIT,		"DIVERGENT-SUBMODULE_EDIT" },

	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD,				"DIVERGENT-FILE_EDIT_TBD" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__NO_RULE,			"DIVERGENT-FILE_EDIT_NO_RULE" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT,	"DIVERGENT-FILE_EDIT_AUTO_CONFLICT" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_ERROR,		"DIVERGENT-FILE_EDIT_AUTO_ERROR" },
	{	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK,			"DIVERGENT-FILE_EDIT_AUTO_OK" },
};

static SG_uint32 nrFlagMsgs = SG_NrElements(aFlagMsgs);

void _sg_wc_tx__merge__trace_msg__dump_entry_conflict_flags(SG_context * pCtx,
															SG_mrg_cset_entry_conflict_flags flags,
															SG_uint32 indent)
{
	SG_uint32 j;
	SG_mrg_cset_entry_conflict_flags flagsLocal = flags;

	for (j=0; j<nrFlagMsgs; j++)
	{
		if (flagsLocal & aFlagMsgs[j].bit)
		{
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,"%*c%s\n",indent,' ',aFlagMsgs[j].pszMsg)  );
			flagsLocal &= ~aFlagMsgs[j].bit;
		}
	}

	SG_ASSERT( (flagsLocal == 0) );		// make sure that table is complete.
}

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback _sg_wc_tx__merge__trace_msg__dump_list_member__mrg_cset_entry_conflict;

static void _sg_wc_tx__merge__trace_msg__dump_list_member__mrg_cset_entry_conflict(SG_context * pCtx,
																				   const char * pszKey_Gid_Entry,
																				   void * pVoidAssocData_MrgCSetEntryConflict,
																				   SG_UNUSED_PARAM(void * pVoid_Data))
{
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = (SG_mrg_cset_entry_conflict *)pVoidAssocData_MrgCSetEntryConflict;
	SG_string * pString_temp = NULL;
	SG_uint32 j;
	SG_uint32 nrChanges = 0;
	SG_uint32 nrDeletes = 0;

	SG_UNUSED(pVoid_Data);

	SG_ASSERT(  (strcmp(pszKey_Gid_Entry,pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufGid_Entry) == 0)  );

	if (pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes)
		SG_ERR_CHECK(  SG_vector__length(pCtx,pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes,&nrChanges)  );
	if (pMrgCSetEntryConflict->pVec_MrgCSet_Deletes)
		SG_ERR_CHECK(  SG_vector__length(pCtx,pMrgCSetEntryConflict->pVec_MrgCSet_Deletes,&nrDeletes)  );

	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "    conflict: [gid %s] [flags 0x%08x] [nr deletes %d] [nr changes %d]\n",
							   pszKey_Gid_Entry,pMrgCSetEntryConflict->flags,nrDeletes,nrChanges)  );

	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dump_entry_conflict_flags(pCtx,pMrgCSetEntryConflict->flags,8)  );

	// compute the repo-path of entry as it existed in the ANCESTOR version of the tree.
	SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
															  pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->pMrgCSet,
															  pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufGid_Entry,
															  &pString_temp)  );
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "        Ancestor: %s\n",
							   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );
	SG_STRING_NULLFREE(pCtx, pString_temp);

	// dump the branches/leaves where there was a change relative to the ancestor
	for (j=0; j<nrChanges; j++)
	{
		const SG_mrg_cset_entry * pMrgCSetEntry_Leaf_j;
		SG_int64 i64;
		SG_mrg_cset_entry_neq neq_Leaf_j;

		SG_ERR_CHECK(  SG_vector__get(pCtx,pMrgCSetEntryConflict->pVec_MrgCSetEntry_Changes,j,(void **)&pMrgCSetEntry_Leaf_j)  );
		SG_ERR_CHECK(  SG_vector_i64__get(pCtx,pMrgCSetEntryConflict->pVec_MrgCSetEntryNeq_Changes,j,&i64)  );
		neq_Leaf_j = (SG_mrg_cset_entry_neq)i64;

		// compute the repo-path of the entry as it existed in the branch/leaf version of the tree.
		SG_ERR_IGNORE(  _sg_mrg_cset__compute_repo_path_for_entry(pCtx,
																  pMrgCSetEntry_Leaf_j->pMrgCSet,
																  pMrgCSetEntry_Leaf_j->bufGid_Entry,
																  &pString_temp)  );
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "        changed: neq[%02x] [leaf hidCSet %s] [%s]\n",
								   neq_Leaf_j,
								   pMrgCSetEntry_Leaf_j->pMrgCSet->bufHid_CSet,
								   ((pString_temp) ? SG_string__sz(pString_temp) : "(unknown)"))  );

		if (neq_Leaf_j & SG_MRG_CSET_ENTRY_NEQ__ATTRBITS)
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "            attrbits: 0x%0x --> 0x%0x\n",
									   ((SG_uint32)pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->attrBits),
									   ((SG_uint32)pMrgCSetEntry_Leaf_j->attrBits))  );
		if (neq_Leaf_j & SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME)
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "            renamed: %s --> %s\n",
									   SG_string__sz(pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->pStringEntryname),
									   SG_string__sz(pMrgCSetEntry_Leaf_j->pStringEntryname))  );
		if (neq_Leaf_j & SG_MRG_CSET_ENTRY_NEQ__GID_PARENT)
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "            moved: %s --> %s\n",
									   pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufGid_Parent,
									   pMrgCSetEntry_Leaf_j->bufGid_Parent)  );
		if (neq_Leaf_j & SG_MRG_CSET_ENTRY_NEQ__DELETED)
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "            removed:\n")  );
		if (neq_Leaf_j & SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB)
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "           modified symlink: hid[ %s --> %s] content[TODO]\n",
									   pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufHid_Blob,
									   pMrgCSetEntry_Leaf_j->bufHid_Blob)  );
		if (neq_Leaf_j & SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB)
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "           modified file: hid[%s --> %s]\n",
									   pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufHid_Blob,
									   pMrgCSetEntry_Leaf_j->bufHid_Blob)  );
		if (neq_Leaf_j & SG_MRG_CSET_ENTRY_NEQ__SUBMODULE_HID_BLOB)
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "           modified submodule: hid[ %s --> %s] content[TODO]\n",
									   pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufHid_Blob,
									   pMrgCSetEntry_Leaf_j->bufHid_Blob)  );

		SG_STRING_NULLFREE(pCtx,pString_temp);
	}

	// dump the branches/leaves where there was a delete

	for (j=0; j<nrDeletes; j++)
	{
		const SG_mrg_cset * pMrgCSet_Leaf_j;

		SG_ERR_CHECK(  SG_vector__get(pCtx,pMrgCSetEntryConflict->pVec_MrgCSet_Deletes,j,(void **)&pMrgCSet_Leaf_j)  );
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "        deleted: [leaf hidCSet %s]\n",
								   pMrgCSet_Leaf_j->bufHid_CSet)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx,pString_temp);
}

static void _sg_wc_tx__merge__trace_msg__dump_conflict_list(SG_context * pCtx,
															const char * pszLabel,
															SG_mrg_cset * pMrgCSet_Result)
{
	SG_uint32 nr = 0;

	if (pMrgCSet_Result->prbConflicts)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,pMrgCSet_Result->prbConflicts,&nr)  );

	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "%s: [result-cset %s] conflict list: [count %d]\n",
							   pszLabel,SG_string__sz(pMrgCSet_Result->pStringCSetLabel),nr)  );
	if (nr > 0)
		SG_ERR_IGNORE(  SG_rbtree__foreach(pCtx,pMrgCSet_Result->prbConflicts,_sg_wc_tx__merge__trace_msg__dump_list_member__mrg_cset_entry_conflict,NULL)  );
}
#endif

//////////////////////////////////////////////////////////////////

/**
 * set all the marker values on all of the leaf CSETs to a known value
 * so that after we run the main loop, we can identify the entries that
 * we did not touch.
 */
static void _v_set_all_markers(SG_context * pCtx,
							   SG_vector * pVec_MrgCSet_Leaves,
							   SG_int64 newValue)
{
	SG_mrg_cset * pMrgCSet_Leaf_k;
	SG_uint32 nrLeaves, kLeaf;

	SG_ERR_CHECK_RETURN(  SG_vector__length(pCtx,pVec_MrgCSet_Leaves,&nrLeaves)  );
	
	for (kLeaf=0; kLeaf<nrLeaves; kLeaf++)
	{
		SG_ERR_CHECK_RETURN(  SG_vector__get(pCtx,pVec_MrgCSet_Leaves,kLeaf,(void **)&pMrgCSet_Leaf_k)  );
		SG_ERR_CHECK_RETURN(  SG_mrg_cset__set_all_markers(pCtx,pMrgCSet_Leaf_k,newValue)  );
	}
}

//////////////////////////////////////////////////////////////////

/**
 * context data for sg_rbtree_foreach_callback for:
 *     _v_process_ancestor_entry
 *     _v_process_leaf_k_entry_addition
 */
struct _v_data
{
	SG_mrg *					pMrg;							// we do not own this
	SG_vector *					pVec_MrgCSet_Leaves;			// we do not own this
	SG_mrg_cset *				pMrgCSet_Result;				// we own this

	// used by _v_process_ancestor_entry
	SG_mrg_cset *				pMrgCSet_Ancestor;				// we do not own this

	// used by _v_process_leaf_k_entry_addition
	SG_mrg_cset *				pMrgCSet_Leaf_k;				// we do not own this
	SG_uint32					kLeaf;

	// used by _v_find_orphans, _v_add_orphan_parents
	SG_rbtree *					prbMrgCSetEntry_OrphanParents;	// rbtree[gid-entry --> SG_mrg_cset_entry * pMrgCSetEntry_Ancestor]   orphan-to-add-list   we own rbtree, but do not own values

	// used by _v_find_path_cycles, _v_add_path_cycle_conflicts
	SG_rbtree *					prbMrgCSetEntry_DirectoryCycles;	// rbtree[gid-entry --> SG_mrg_cset_entry * pMrgCSetEntry_Result]	dirs involved in a cycle   we own rbtree, but do not own values

	SG_int32					use_marker_value;
};

typedef struct _v_data _v_data;

//////////////////////////////////////////////////////////////////

/**
 * Look at the corresponding entry in each of the k leaves and compare them
 * with the ancestor entry.  If we have an unchanged entry or an entry that
 * was only changed in one branch, handle it without a lot of fuss (or allocs).
 *
 * We do this because most of the entries in a tree won't change (or will only
 * be changed in one branch).
 */
static void _v_process_ancestor_entry__quick_scan(SG_context * pCtx,
												  const char * pszKey_Gid_Entry,
												  SG_mrg_cset_entry * pMrgCSetEntry_Ancestor,
												  _v_data * pData,
												  SG_bool * pbHandled)
{
	SG_uint32 nrLeaves, kLeaf;
	SG_uint32 kLeaf_Last_Change = SG_UINT32_MAX;
	SG_mrg_cset_entry * pMrgCSetEntry_Last_Change = NULL;
	SG_mrg_cset_entry_neq neq_Last_Change = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;

	SG_ERR_CHECK(  SG_vector__length(pCtx,pData->pVec_MrgCSet_Leaves,&nrLeaves)  );
	for (kLeaf=0; kLeaf<nrLeaves; kLeaf++)
	{
		SG_mrg_cset * pMrgCSet_Leaf_k = NULL;
		SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k = NULL;
		SG_mrg_cset_entry_neq neq = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;
		SG_bool bFound_in_Leaf = SG_FALSE;

		SG_ERR_CHECK(  SG_vector__get(pCtx,pData->pVec_MrgCSet_Leaves,kLeaf,(void **)&pMrgCSet_Leaf_k)  );

		SG_ERR_CHECK(  SG_rbtree__find(pCtx,pMrgCSet_Leaf_k->prbEntries,pszKey_Gid_Entry,
									   &bFound_in_Leaf,(void **)&pMrgCSetEntry_Leaf_k)  );
		if (bFound_in_Leaf)
		{
			if (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict)	// if any leaf already has a conflict (from sub-merge)
				goto not_handled;								// we are forced to do it the hard-way.

			pMrgCSetEntry_Leaf_k->markerValue = pData->use_marker_value;

			SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx,pMrgCSetEntry_Ancestor,pMrgCSetEntry_Leaf_k,&neq)  );
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_leaf_eq_neq(pCtx,"_v_process_ancestor_entry__quick_scan",
													  pData->use_marker_value,
													  kLeaf,pMrgCSet_Leaf_k,pszKey_Gid_Entry,neq)  );
#endif
		}
		else
		{
			neq = SG_MRG_CSET_ENTRY_NEQ__DELETED;
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_leaf_deleted(pCtx,"_v_process_ancestor_entry__quick_scan",
													   pData->use_marker_value,
													   pData->pMrgCSet_Ancestor,kLeaf,pszKey_Gid_Entry)  );
#endif
		}
		
		if (neq != SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL)
		{
			if (kLeaf_Last_Change != SG_UINT32_MAX)		// we found a second leaf/branch that changed the entry
				goto not_handled;

			kLeaf_Last_Change = kLeaf;
			pMrgCSetEntry_Last_Change = pMrgCSetEntry_Leaf_k;
			neq_Last_Change = neq;
		}
	}

	if (kLeaf_Last_Change == SG_UINT32_MAX)
	{
		// the entry was not changed in any branch.
		// clone the ancestor entry and add it to the result-cset.

#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "_v_process_ancestor_entry__quick_scan[%d]: cloning ancestor [entry %s]\n",
								   pData->use_marker_value,
								   pszKey_Gid_Entry)  );
#endif
		SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx,
																   pData->pMrg,
																   pMrgCSetEntry_Ancestor,
																   pData->pMrgCSet_Result,
																   NULL)  );
		goto handled;
	}
	else
	{
		if (neq_Last_Change & SG_MRG_CSET_ENTRY_NEQ__DELETED)
		{
			// the only change was a delete.
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "_v_process_ancestor_entry__quick_scan[%d]: omitting leaf[%d] [entry %s]\n",
									   pData->use_marker_value,
									   kLeaf_Last_Change,pszKey_Gid_Entry)  );
#endif
			SG_ERR_CHECK(  SG_mrg_cset__register_delete(pCtx,pData->pMrgCSet_Result,pMrgCSetEntry_Ancestor)  );
			goto handled;
		}
		else
		{
			// the only change was a proper change.  it may have one or more NEQ
			// bits set, but we don't care because we are going to take the whole
			// change as is.

#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "_v_process_ancestor_entry__quick_scan[%d]: cloning leaf[%d] [entry %s]\n",
									   pData->use_marker_value,
									   kLeaf_Last_Change,pszKey_Gid_Entry)  );
#endif
			SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx,
																	   pData->pMrg,
																	   pMrgCSetEntry_Last_Change,
																	   pData->pMrgCSet_Result,
																	   NULL)  );
			goto handled;
		}
	}

	/*NOTREACHED*/

handled:
	*pbHandled = SG_TRUE;
	return;

not_handled:
	*pbHandled = SG_FALSE;
	return;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * look at all of the peer entries in each cset for the given gid-entry
 * and compare them.  assume that we will have a conflict of some kind
 * and allocate an entry-conflict structure and populate it with the
 * entries from the leaves that changed it relative to the ancestor.
 * 
 * we want to identify:
 * [] the csets that changed it;
 * [] the values that they changed it to;
 * [] the unique values that they changed it to (removing duplicates on
 *    a field-by-field basis).
 *
 * you own the returned entry-conflict structure.  (we DO NOT automatically
 * add it to the conflict-list in the result-cset.)
 *
 * note that we are given a vector of leaves; these are the branches
 * descended from the given ancestor.  when we are an outer-merge
 * (containing a sub-merge) one or more of our "leaves" will actually
 * be the result-entry for the sub-merge (rather than an actual leaf
 * from the point of view of the DAGLCA graph).  so, for example, the
 * result-entry might not have a file-hid-blob because we haven't
 * done the auto-merge yet.
 */
static void _v_process_ancestor_entry__build_candidate_conflict(SG_context * pCtx,
																const char * pszKey_Gid_Entry,
																SG_mrg_cset_entry * pMrgCSetEntry_Ancestor,
																SG_mrg_cset_entry * pMrgCSetEntry_Baseline,
																_v_data * pData,
																SG_mrg_cset_entry_conflict ** ppMrgCSetEntryConflict)
{
	SG_uint32 nrLeaves, kLeaf;
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = NULL;

	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__alloc(pCtx,pData->pMrgCSet_Result,
													 pMrgCSetEntry_Ancestor,
													 pMrgCSetEntry_Baseline,
													 &pMrgCSetEntryConflict)  );

	SG_ERR_CHECK(  SG_vector__length(pCtx,pData->pVec_MrgCSet_Leaves,&nrLeaves)  );
	for (kLeaf=0; kLeaf<nrLeaves; kLeaf++)
	{
		SG_mrg_cset * pMrgCSet_Leaf_k = NULL;
		SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k = NULL;
		SG_mrg_cset_entry_neq neq = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;
		SG_bool bFound_in_Leaf = SG_FALSE;

		SG_ERR_CHECK(  SG_vector__get(pCtx,pData->pVec_MrgCSet_Leaves,kLeaf,(void **)&pMrgCSet_Leaf_k)  );

		SG_ERR_CHECK(  SG_rbtree__find(pCtx,pMrgCSet_Leaf_k->prbEntries,pszKey_Gid_Entry,
									   &bFound_in_Leaf,(void **)&pMrgCSetEntry_Leaf_k)  );
		if (bFound_in_Leaf)
		{
			pMrgCSetEntry_Leaf_k->markerValue = pData->use_marker_value;

			SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx,pMrgCSetEntry_Ancestor,pMrgCSetEntry_Leaf_k,&neq)  );
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_leaf_eq_neq(pCtx,"_v_process_ancestor_entry__build_candidate_conflict",
													  pData->use_marker_value,
													  kLeaf,pMrgCSet_Leaf_k,pszKey_Gid_Entry,neq)  );
#endif
			if ((neq != SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL) || (pMrgCSetEntry_Leaf_k->pMrgCSetEntryConflict))
				SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__append_change(pCtx,pMrgCSetEntryConflict,pMrgCSetEntry_Leaf_k,neq)  );
		}
		else
		{
			neq = SG_MRG_CSET_ENTRY_NEQ__DELETED;
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_leaf_deleted(pCtx,"_v_process_ancestor_entry__build_candidate_conflict",
													   pData->use_marker_value,
													   pData->pMrgCSet_Ancestor,kLeaf,pszKey_Gid_Entry)  );
#endif
			SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__append_delete(pCtx,pMrgCSetEntryConflict,pMrgCSet_Leaf_k)  );
		}
	}

	*ppMrgCSetEntryConflict = pMrgCSetEntryConflict;
	return;

fail:
	SG_MRG_CSET_ENTRY_CONFLICT_NULLFREE(pCtx, pMrgCSetEntryConflict);
}

/**
 * Look at the corresponding entry in each of the k leaves and compare them
 * with the ancestor entry.  We know/expect that 2 or more of them have
 * changed the entry some how.
 *
 * Build a SG_mrg_cset_entry_conflict with the details.  If we can internally
 * resolve it, we do so.  If not, we add the conflict to the CSET.
 *
 * Things we can handle quietly include things like a rename in one branch
 * and a chmod in another or 2 renames to the same new name.
 *
 * Things we can't handle include things like 2 renames to different new names,
 * for example.
 */
static void _v_process_ancestor_entry__hard_way(SG_context * pCtx,
												const char * pszKey_Gid_Entry,
												SG_mrg_cset_entry * pMrgCSetEntry_Ancestor,
												_v_data * pData)
{
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry_Composite = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry_Baseline = NULL;		// we don't own this
	SG_uint32 nrDeletes, nrChanges;
	SG_uint32 nrUniqueAttrBitsChanges;
	SG_uint32 nrUniqueEntrynameChanges;
	SG_uint32 nrUniqueGidParentChanges;
	SG_uint32 nrUniqueSymlinkHidBlobChanges;
	SG_uint32 nrUniqueFileHidBlobChanges;
	SG_uint32 nrUniqueSubmoduleHidBlobChanges;
	SG_mrg_cset_entry_conflict_flags flags = SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO;
	const char * psz_temp;
	SG_bool bFound;

	// see if there is a corresponding pMrgCSetEntry for this entry in the baseline cset.
	// this may or may not exist.

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->pMrg->pMrgCSet_Baseline->prbEntries, pszKey_Gid_Entry,
								   &bFound, (void **)&pMrgCSetEntry_Baseline)  );

	// assume the worst and allocate a conflict-structure to hold info
	// on the branches/leaves that changed the entry.  if we later determine
	// that we can actually handle the change, we delete the conflict-structure
	// rather than adding it to the conflict-list in the resut-cset.

	SG_ERR_CHECK(  _v_process_ancestor_entry__build_candidate_conflict(pCtx,
																	   pszKey_Gid_Entry,
																	   pMrgCSetEntry_Ancestor,
																	   pMrgCSetEntry_Baseline,
																	   pData,
																	   &pMrgCSetEntryConflict)  );

	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_deletes(pCtx,pMrgCSetEntryConflict,&nrDeletes)  );	// nr leaves that deleted it
	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_changes(pCtx,pMrgCSetEntryConflict,&nrChanges)  );	// nr leaves that changed it somehow
	SG_ASSERT(  ((nrDeletes + nrChanges) > 0)  );

	if ((nrDeletes > 0) && (nrChanges == 0))			// a clean delete (by 1 or more leaves (and no other leaf made any changes to the entry))
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "_v_process_ancestor_entry__hard_way..: omitting leaf [deleted in %d leaves] [entry %s]\n",
								   nrDeletes, pszKey_Gid_Entry)  );
#endif
		SG_ERR_CHECK(  SG_mrg_cset__register_delete(pCtx,pData->pMrgCSet_Result,pMrgCSetEntry_Ancestor)  );
		goto handled_entry__eat_conflict;
	}

	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_unique_attrbits(pCtx,pMrgCSetEntryConflict,&nrUniqueAttrBitsChanges)  );
	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_unique_entryname(pCtx,pMrgCSetEntryConflict,&nrUniqueEntrynameChanges)  );
	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_unique_gid_parent(pCtx,pMrgCSetEntryConflict,&nrUniqueGidParentChanges)  );
	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_unique_symlink_hid_blob(pCtx,pMrgCSetEntryConflict,&nrUniqueSymlinkHidBlobChanges)  );
	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_unique_file_hid_blob(pCtx,pMrgCSetEntryConflict,&nrUniqueFileHidBlobChanges)  );
	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__count_unique_submodule_hid_blob(pCtx,pMrgCSetEntryConflict,&nrUniqueSubmoduleHidBlobChanges)  );
	
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_process_ancestor_entry__hard_way..[%d]: [deleted in %d leaves] [changed in %d leaves] [entry %s]\n",
							   pData->use_marker_value,
							   nrDeletes, nrChanges, pszKey_Gid_Entry)  );
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "                                           nr unique: [attrbits %d][rename %d][move %d][symlink blob %d][file blob %d][submodule blog %d]\n",
							   nrUniqueAttrBitsChanges,
							   nrUniqueEntrynameChanges,
							   nrUniqueGidParentChanges,
							   nrUniqueSymlinkHidBlobChanges,
							   nrUniqueFileHidBlobChanges,
							   nrUniqueSubmoduleHidBlobChanges)  );
#endif

	// we create a new instance of the entry for the result-cset.  for things which
	// were either not changed or only changed in one leaf, we know the right answer.
	// for things that were changed in multiple leaves, we have a hard/arbitrary choice.
	// I can see 2 ways of doing this:
	// [a] use the ancestor version and list all of the leaf values as candidates for
	//     the user to choose from.
	// [b] use the baseline version (if present) and list the remaining values as
	//     alternatives.
	// I originally thought that [a] would be best.  But now I'm thinking that [b]
	// would be better because it'll better reflect the merge as a series of deltas
	// to the current baseline.
	//
	// create the clone.  try to use the baseline.  fallback to the ancestor.
	// then overwrite various fields.

	if (pMrgCSetEntry_Baseline)
		SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx,
																   pData->pMrg,
																   pMrgCSetEntry_Baseline,
																   pData->pMrgCSet_Result,
																   &pMrgCSetEntry_Composite)  );
	else
		SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx,
																   pData->pMrg,
																   pMrgCSetEntry_Ancestor,
																   pData->pMrgCSet_Result,
																   &pMrgCSetEntry_Composite)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "_v_process_ancestor_entry__hard_way..[%d]: [present in baseline %d]\n",
							   pData->use_marker_value,
							   (pMrgCSetEntry_Baseline != NULL))  );
#endif

	// if a field was only changed in one leaf (or in the same way by many leaves),
	// then we take the change without complaining.
	// 
	// if a field was changed in divergent ways, we need to think about it a little.
	//
	// [1] the values for RENAME, MOVE, ATTRBITS, SYMLINKS, and SUBMODULES are collapsable.
	//     (meaning in a 5-way merge with 5 diverent renames, the user should only have to
	//     pick 1 final value for the result; they should not have to vote for a new value
	//     for each pair-wise sub-merge.)
	//     so that if one of the leaves had a conflict on a field, we added all of possible
	//     values to our rbUnique, so our set of potential new values grows as we crawl out
	//     of the sub-merges.  then the preview/resolver can simply show the potential set
	//     without having to dive into the graph.
	//
	// [2] if there are multiple values for the file-hid-blob, then we need to build an
	//     auto-merge-plan and let it deal with it at the right time.
	//     (see SG_mrg_cset_entry_conflict__append_change())
	//
	//     we ZERO the file-hid-blob in the result-cset as an indicator that we need
	//     to wait for auto-merge.

	switch (nrUniqueGidParentChanges)
	{
	case 0:			// not moved in any leaf
		break;		// keep gid-parent from baseline/ancestor (since baseline value matches ancestor value).

	case 1:			// any/all moves moved it to one unique target directory.
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,NULL,pMrgCSetEntryConflict->prbUnique_GidParent,&bFound,&psz_temp,NULL)  );
		SG_ASSERT(  (bFound && psz_temp && *psz_temp)  );
		SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
												pMrgCSetEntry_Composite->bufGid_Parent, SG_NrElements(pMrgCSetEntry_Composite->bufGid_Parent),
												psz_temp)  );
		if (nrDeletes > 0)
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE;
		break;
			
	default:		// divergent move to 2 or more target directories.  keep it in baseline/ancestor, but flag it.
		// TODO putting it in the directory that it was in in the baseline/ancestor is a little bit questionable.
		// TODO we might want to put it in one of the target directories since we have the conflict-rename-trick.
		// TODO this might cause the entry to become an orphan....  BUT DON'T CHANGE THIS WITHOUT LOOKING AT THE
		// TODO CYCLE-PATH STUFF.
		if (nrDeletes > 0)
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE;
		flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE;
		break;
	}

	switch (nrUniqueEntrynameChanges)
	{
	case 0:			// not renamed in any leaf.
		break;		// keep entryname from baseline/ancestor.

	case 1:			// any/all renames renamed it to one unique target name.
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,NULL,pMrgCSetEntryConflict->prbUnique_Entryname,&bFound,&psz_temp,NULL)  );
		SG_ASSERT(  (bFound && psz_temp && *psz_temp)  );
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pMrgCSetEntry_Composite->pStringEntryname, psz_temp)  );
		if (nrDeletes > 0)
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_RENAME;
		break;

	default:		// divergent rename to 2 or more target entrynames.  keep value as it was in baseline/ancestor, but flag it.
		if (nrDeletes > 0)
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_RENAME;
		flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME;
		break;
	}

	switch (nrUniqueAttrBitsChanges)
	{
	case 0:			// no changes from baseline/ancestor.
		break;

	case 1:			// 1 unique new value
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,NULL,pMrgCSetEntryConflict->prbUnique_AttrBits,&bFound,&psz_temp,NULL)  );
		SG_ASSERT(  (bFound && psz_temp && *psz_temp)  );
		SG_ERR_CHECK(  SG_int64__parse(pCtx,&pMrgCSetEntry_Composite->attrBits,psz_temp)  );
		if (nrDeletes > 0)
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_ATTRBITS;
		break;

	default:		// multiple new values; use value from baseline/ancestor and complain and let them pick a new value.
		if (nrDeletes > 0)
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_ATTRBITS;
		flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_ATTRBITS;
		break;
	}

	switch (pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->tneType)
	{
	default:	// quiets compiler
		SG_ASSERT( 0 );
		SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:			// cannot run diff3 on the pathname fragment in the symlinks

		switch (nrUniqueSymlinkHidBlobChanges)
		{
		case 0:
			break;

		case 1:			// 1 new unique value, just take it
			SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,NULL,pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob,&bFound,&psz_temp,NULL)  );
			SG_ASSERT(  (bFound && psz_temp && *psz_temp)  );
			SG_ERR_CHECK(  SG_strcpy(pCtx,
									 pMrgCSetEntry_Composite->bufHid_Blob, SG_NrElements(pMrgCSetEntry_Composite->bufHid_Blob),
									 psz_temp)  );
			if (nrDeletes > 0)
				flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT;
			break;
			
		default:		// more than 1 new unique value, keep value from baseline/ancestor and complain since we can't auto-merge the symlink paths
			if (nrDeletes > 0)
				flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT;
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT;
			break;
		}
		break;

	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:	// need to run diff3 or something on the content of the files.

		switch (nrUniqueFileHidBlobChanges)
		{
		case 0:
			break;

		case 1:			// 1 new unique value, just take it
			SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,NULL,pMrgCSetEntryConflict->prbUnique_File_HidBlob,&bFound,&psz_temp,NULL)  );
			SG_ASSERT(  (bFound && psz_temp && *psz_temp)  );
			SG_ERR_CHECK(  SG_strcpy(pCtx,
									 pMrgCSetEntry_Composite->bufHid_Blob, SG_NrElements(pMrgCSetEntry_Composite->bufHid_Blob),
									 psz_temp)  );
			if (nrDeletes > 0)
				flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT;
			break;
			
		default:		// more than 1 new unique value, need to auto-merge it
			if (nrDeletes > 0)
				flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT;
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD;

			// zero bufHid_Blob in result-entry because there is more than one
			// possible value for it.  also forget that the inheritance for it.
			SG_ERR_CHECK(  SG_mrg_cset_entry__forget_inherited_hid_blob(pCtx,pMrgCSetEntry_Composite)  );
			
			break;
		}
		break;

	case SG_TREENODEENTRY_TYPE_SUBMODULE:

		switch (nrUniqueSubmoduleHidBlobChanges)
		{
		case 0:
			break;

		case 1:			// 1 new unique value -- all of our branches/leaves refer to the same cset within the submodule, just take it
			SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,NULL,pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob,&bFound,&psz_temp,NULL)  );
			SG_ASSERT(  (bFound && psz_temp && *psz_temp)  );
			SG_ERR_CHECK(  SG_strcpy(pCtx,
									 pMrgCSetEntry_Composite->bufHid_Blob, SG_NrElements(pMrgCSetEntry_Composite->bufHid_Blob),
									 psz_temp)  );
			if (nrDeletes > 0)	// one branch/leaf deleted the submodule from the outer cset *and* another branch/leaf updated it to a different cset
				flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SUBMODULE_EDIT;
			break;
			
		default:		// more than 1 new unique value, keep value from baseline/ancestor and complain
			if (nrDeletes > 0)
				flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SUBMODULE_EDIT;
			flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SUBMODULE_EDIT;
			break;
		}
		break;
	}

	if (flags == SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO)
	{
		// all of the structural changes are consistent.
		// no divergent changes.
		// no deletes-with-other-changes.
		// no need to merge file contents.

#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "_v_process_ancestor_entry__hard_way..[%d]: composite1 [flags 0x%08x] [parent gid %s] [attr 0x%0x] [blob %s] [entry %s] %s\n",
								   pData->use_marker_value,
								   flags,
								   pMrgCSetEntry_Composite->bufGid_Parent,
								   ((SG_uint32)pMrgCSetEntry_Composite->attrBits),
								   pMrgCSetEntry_Composite->bufHid_Blob,
								   pMrgCSetEntry_Composite->bufGid_Entry,
								   SG_string__sz(pMrgCSetEntry_Composite->pStringEntryname))  );
#endif
		goto handled_entry__eat_conflict;
	}
	else
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "_v_process_ancestor_entry__hard_way..[%d]: composite2 [flags 0x%08x] [parent gid %s] [attr 0x%0x] [blob ?] [entry %s] %s\n",
								   pData->use_marker_value,
								   flags,
								   pMrgCSetEntry_Composite->bufGid_Parent,
								   ((SG_uint32)pMrgCSetEntry_Composite->attrBits),
								   pMrgCSetEntry_Composite->bufGid_Entry,
								   SG_string__sz(pMrgCSetEntry_Composite->pStringEntryname))  );
#endif
		goto record_conflict;
	}
	
	/*NOTREACHED*/

record_conflict:
	pMrgCSetEntry_Composite->pMrgCSetEntryConflict = pMrgCSetEntryConflict;
	pMrgCSetEntryConflict->pMrgCSetEntry_Composite = pMrgCSetEntry_Composite;
	pMrgCSetEntryConflict->flags = flags;
	SG_ERR_CHECK(  SG_mrg_cset__register_conflict(pCtx,pData->pMrgCSet_Result,pMrgCSetEntryConflict)  );
	pMrgCSetEntryConflict = NULL;					// the result-cset owns it now
	return;

handled_entry__eat_conflict:
	// we were able to resolve the (potential) conflict and completely deal with the entry
	// so we don't need to add the conflict-struct to the result-cset.
	SG_MRG_CSET_ENTRY_CONFLICT_NULLFREE(pCtx, pMrgCSetEntryConflict);
	return;

fail:
	SG_MRG_CSET_ENTRY_CONFLICT_NULLFREE(pCtx, pMrgCSetEntryConflict);
	return;
}

static SG_rbtree_foreach_callback _v_process_ancestor_entry;

/**
 * we get called once for each entry (file/symlink/directory) present in the
 * ancestor version of the version control tree.
 *
 * look at each of the k leaves and see what was done to it in that branch.
 *
 * compose/clone a new SG_mrg_cset_entry to represent the merger and add it
 * to the result-cset.  if there are conflicts, also include enough info to
 * let the caller ask the user what to do.
 */
static void _v_process_ancestor_entry(SG_context * pCtx,
									  const char * pszKey_Gid_Entry,
									  void * pVoidAssocData_MrgCSetEntry_Ancestor,
									  void * pVoid_Data)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Ancestor = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Ancestor;
	_v_data * pData = (_v_data *)pVoid_Data;
	SG_bool bHandled = SG_FALSE;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_ancestor(pCtx,"_v_process_ancestor_entry",
										   pData->use_marker_value,
										   pData->pMrgCSet_Ancestor,
										   pszKey_Gid_Entry)  );
#endif

	// try to do a trivial clone for unchanged entries and entries only changed in one leaf/branch.
	SG_ERR_CHECK(  _v_process_ancestor_entry__quick_scan(pCtx,pszKey_Gid_Entry,pMrgCSetEntry_Ancestor,pData,&bHandled)  );
	if (bHandled)
		return;

	// if that fails, we have some kid of conflict or potential conflict.  do it the
	// hard way and assume the worst.

	SG_ERR_CHECK(  _v_process_ancestor_entry__hard_way(pCtx,pszKey_Gid_Entry,pMrgCSetEntry_Ancestor,pData)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _find_peer_in_other_leaf(SG_context * pCtx,
									 _v_data * pData,
									 const char * pszKey_Gid_Entry,
									 SG_uint32 * p_jLeaf,
									 SG_mrg_cset ** ppMrgCSet_Leaf_j,
									 SG_mrg_cset_entry ** ppMrgCSetEntry_Leaf_j)
{
	SG_uint32 nrLeaves;
	SG_uint32 jLeaf;

	SG_ERR_CHECK(  SG_vector__length(pCtx, pData->pVec_MrgCSet_Leaves, &nrLeaves)  );
	SG_ASSERT(  (nrLeaves == 2)  );

	for (jLeaf=0; jLeaf<nrLeaves; jLeaf++)
	{
		SG_mrg_cset * pMrgCSet_Leaf_j = NULL;
		SG_mrg_cset_entry * pMrgCSetEntry_Leaf_j;
		SG_bool bFound_in_j;

		if (jLeaf != pData->kLeaf)
		{
			SG_ERR_CHECK(  SG_vector__get(pCtx, pData->pVec_MrgCSet_Leaves, jLeaf, (void **)&pMrgCSet_Leaf_j)  );
			SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrgCSet_Leaf_j->prbEntries, pszKey_Gid_Entry,
										   &bFound_in_j, (void **)&pMrgCSetEntry_Leaf_j)  );
			if (bFound_in_j)
			{
				*p_jLeaf = jLeaf;
				*ppMrgCSet_Leaf_j = pMrgCSet_Leaf_j;
				*ppMrgCSetEntry_Leaf_j = pMrgCSetEntry_Leaf_j;
				return;
			}
		}
	}

	*p_jLeaf = nrLeaves;	// bogus
	*ppMrgCSet_Leaf_j = NULL;
	*ppMrgCSetEntry_Leaf_j = NULL;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

struct _my_history_data
{
	char * pszGid;
	char * pszHidCSet_k;
	char * pszHidCSet_j;
	SG_history_result * pHistoryResult_k;
	SG_history_result * pHistoryResult_j;
	SG_rbtree * prbIntersection;
};

static void _my_history_data__free(SG_context * pCtx,
								   struct _my_history_data * p)
{
	if (!p)
		return;

	SG_NULLFREE(pCtx, p->pszGid);
	SG_NULLFREE(pCtx, p->pszHidCSet_k);
	SG_NULLFREE(pCtx, p->pszHidCSet_j);
	SG_HISTORY_RESULT_NULLFREE(pCtx, p->pHistoryResult_k);
	SG_HISTORY_RESULT_NULLFREE(pCtx, p->pHistoryResult_j);
	SG_RBTREE_NULLFREE(pCtx, p->prbIntersection);
	SG_NULLFREE(pCtx, p);
}

#define SG_MY_HISTORY_DATA__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,_my_history_data__free)

static void _intersect_histories(SG_context * pCtx,
								 _v_data * pData,
								 const char * pszKey_Gid_Entry,
								 const char * pszHidCSet_k,
								 const char * pszHidCSet_j,
								 struct _my_history_data ** ppMyHistoryData)
{
	struct _my_history_data * pMyHistoryData = NULL;
	SG_stringarray * psaGids = NULL;
	SG_stringarray * psaStarting_k = NULL;
	SG_stringarray * psaStarting_j = NULL;
	SG_rbtree * prbWork = NULL;
	SG_uint32 nrResults_k = 0;
	SG_uint32 nrResults_j = 0;
#if TRACE_WC_MERGE
	SG_uint32 count = 0;
#endif
	SG_bool bMore;

	const char* pszUser = NULL;
	const char* pszStamp = NULL;
	SG_stringarray * psaSingle = NULL;
	SG_uint32 nResultLimit = SG_UINT32_MAX;
	SG_bool bLeaves = SG_FALSE;
	SG_bool bHideObjectMerges = SG_FALSE;
	SG_int64 nFromDate = 0;
	SG_int64 nToDate = SG_INT64_MAX;
	SG_bool bRecommendDagWalk = SG_TRUE;
	SG_bool bReassembleDag = SG_TRUE;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pMyHistoryData)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszKey_Gid_Entry, &pMyHistoryData->pszGid)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHidCSet_k, &pMyHistoryData->pszHidCSet_k)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHidCSet_j, &pMyHistoryData->pszHidCSet_j)  );

	// Get the history of each item from the point of view of each leaf.
	// That is, for each leaf we get an independent list of the CSets
	// (on the subset of the graph between the root and that leaf) where
	// the item was changed somehow.
	// 
	// We can't use SG_vv2__history() because we already have the wc.db
	// locked and we already know the GID and CSet HIDs.

	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaGids, 1)  );
	SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaGids, pMyHistoryData->pszGid)  );

	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaStarting_k, 1)  );
	SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaStarting_k, pMyHistoryData->pszHidCSet_k)  );
	
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaStarting_j, 1)  );
	SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaStarting_j, pMyHistoryData->pszHidCSet_j)  );

	SG_ERR_CHECK(  SG_history__run(pCtx, pData->pMrg->pWcTx->pDb->pRepo, psaGids, psaStarting_k,
								   psaSingle, pszUser, pszStamp, nResultLimit, bLeaves, bHideObjectMerges,
								   nFromDate, nToDate, bRecommendDagWalk, bReassembleDag,
								   NULL, &pMyHistoryData->pHistoryResult_k, NULL)  );
	SG_ERR_CHECK(  SG_history_result__count(pCtx, pMyHistoryData->pHistoryResult_k, &nrResults_k)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "================================================================\n")  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "History [gid=%s] [cset=%s] [count=%d]\n",
							   pMyHistoryData->pszGid, pMyHistoryData->pszHidCSet_k, nrResults_k)  );
	SG_ERR_IGNORE(  SG_history_debug__dump_result_to_console(pCtx, pMyHistoryData->pHistoryResult_k)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "================================================================\n")  );
#endif
	
	SG_ERR_CHECK(  SG_history__run(pCtx, pData->pMrg->pWcTx->pDb->pRepo, psaGids, psaStarting_j,
								   psaSingle, pszUser, pszStamp, nResultLimit, bLeaves, bHideObjectMerges,
								   nFromDate, nToDate, bRecommendDagWalk, bReassembleDag,
								   NULL, &pMyHistoryData->pHistoryResult_j, NULL)  );
	SG_ERR_CHECK(  SG_history_result__count(pCtx, pMyHistoryData->pHistoryResult_j, &nrResults_j)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "================================================================\n")  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "History [gid=%s] [cset=%s] [count=%d]\n",
							   pMyHistoryData->pszGid, pMyHistoryData->pszHidCSet_j, nrResults_j)  );
	SG_ERR_IGNORE(  SG_history_debug__dump_result_to_console(pCtx, pMyHistoryData->pHistoryResult_j)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "================================================================\n")  );
#endif

	// Intersect the 2 histories.  This will give us a list of the CSets
	// where the item changed and that are ancestors of both leaves.

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbWork)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS2(pCtx, &pMyHistoryData->prbIntersection, 0, NULL,
											 SG_rbtree__compare_function__reverse_strcmp)  );
	if (nrResults_k > 0)
	{
		bMore = SG_TRUE;
		while (bMore)
		{
			const char * pszHidCSet;

			SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, pMyHistoryData->pHistoryResult_k, &pszHidCSet)  );
			SG_ERR_CHECK(  SG_rbtree__add(pCtx, prbWork, pszHidCSet)  );

			SG_ERR_CHECK(  SG_history_result__next(pCtx, pMyHistoryData->pHistoryResult_k, &bMore)  );
		}
	}

	if (nrResults_j > 0)
	{
		bMore = SG_TRUE;
		while (bMore)
		{
			const char * pszHidCSet;
			SG_bool bFound;

			SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, pMyHistoryData->pHistoryResult_j, &pszHidCSet)  );
			SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbWork, pszHidCSet, &bFound, NULL)  );
			if (bFound)
				SG_ERR_CHECK(  SG_rbtree__add(pCtx, pMyHistoryData->prbIntersection, pszHidCSet)  );
		
			SG_ERR_CHECK(  SG_history_result__next(pCtx, pMyHistoryData->pHistoryResult_j, &bMore)  );
		}
	}

#if TRACE_WC_MERGE
	SG_ERR_CHECK(  SG_rbtree__count(pCtx, pMyHistoryData->prbIntersection, &count)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "IntersectHistories: [gid=%s] [count=%d]:\n",
							   pMyHistoryData->pszGid, count)  );
	SG_ERR_IGNORE(  SG_rbtree_debug__dump_keys_to_console(pCtx, pMyHistoryData->prbIntersection)  );
#endif	

	*ppMyHistoryData = pMyHistoryData;
	pMyHistoryData = NULL;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaGids);
	SG_STRINGARRAY_NULLFREE(pCtx, psaStarting_k);
	SG_STRINGARRAY_NULLFREE(pCtx, psaStarting_j);
	SG_RBTREE_NULLFREE(pCtx, prbWork);
	SG_MY_HISTORY_DATA__NULLFREE(pCtx, pMyHistoryData);
}

static void _remove_candidates_from_intersection(SG_context * pCtx,
												 SG_rbtree * prbIntersection,
												 const SG_stringarray * psaCandidates)
{
	SG_uint32 nrCandidates;
	SG_uint32 c;
	
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaCandidates, &nrCandidates)  );
	for (c=0; c<nrCandidates; c++)
	{
		const char * pszHid_c;
		SG_bool bFound;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaCandidates, c, &pszHid_c)  );
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "RemoveCandidateFromIntersection: %s\n", pszHid_c)  );
#endif
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbIntersection, pszHid_c, &bFound, NULL)  );
		if (bFound)
			SG_ERR_CHECK(  SG_rbtree__remove(pCtx, prbIntersection, pszHid_c)  );
	}

fail:
	return;
}

static void _is_item_present_in_aux_ancestor(SG_context * pCtx,
											 _v_data * pData,
											 const char * pszKey_Gid_Entry,
											 const char * pszHid_per_item_ancestor_cset,
											 const SG_mrg_cset_entry ** ppMrgCSetEntry_Aux)
{
	SG_mrg_cset * pMrgCSet_Aux = NULL;				// we do not own this
	SG_mrg_cset_entry * pMrgCSetEntry_Aux = NULL;	// we do not own this
	SG_bool bFound;

	// Load the CSet into our Aux-cache and see if the GID is present in it.
	SG_ERR_CHECK(  sg_mrg_cset__load_aux__using_repo(pCtx, pData->pMrg, pszHid_per_item_ancestor_cset,
													 &pMrgCSet_Aux)  );
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrgCSet_Aux->prbEntries, pszKey_Gid_Entry,
								   &bFound, (void **)&pMrgCSetEntry_Aux)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "IsItemPresentInAuxAncestor: is [gid %s] present in [cset %s] [found %d]\n",
							   pszKey_Gid_Entry, pszHid_per_item_ancestor_cset, bFound)  );
#endif

	*ppMrgCSetEntry_Aux = pMrgCSetEntry_Aux;

fail:
	return;
}

static void _use_per_item_ancestor(SG_context * pCtx,
								   _v_data * pData,
								   const char * pszKey_Gid_Entry,
								   SG_mrg_cset * pMrgCSet_Leaf_k, SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k,
								   SG_mrg_cset * pMrgCSet_Leaf_j, SG_mrg_cset_entry * pMrgCSetEntry_Leaf_j,
								   const char * pszHid_per_item_ancestor_cset)
{
	SG_mrg_cset * pMrgCSet_AncestorBackup = NULL;	// we do not own this
	SG_mrg_cset * pMrgCSet_Aux = NULL;				// we do not own this
	SG_mrg_cset_entry * pMrgCSetEntry_Aux = NULL;	// we do not own this
	SG_bool bFound;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "UsePerItemAncestor: [cset %s] for [gid %s]\n",
							   pszHid_per_item_ancestor_cset, pszKey_Gid_Entry)  );
#endif

	SG_ERR_CHECK(  sg_mrg_cset__load_aux__using_repo(pCtx, pData->pMrg, pszHid_per_item_ancestor_cset,
													 &pMrgCSet_Aux)  );
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrgCSet_Aux->prbEntries, pszKey_Gid_Entry,
								   &bFound, (void **)&pMrgCSetEntry_Aux)  );
	if (!bFound)
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "UsePerItemAncestor: [gid %s] not found in [cset %s]",
						 pszKey_Gid_Entry, pszHid_per_item_ancestor_cset)  );

	// HACK 2012/09/21 Swap out the official ancestor cset temporarily
	// HACK            with the per-file aux ancestor.
	pMrgCSet_AncestorBackup = pData->pMrgCSet_Ancestor;
	pData->pMrgCSet_Ancestor = pMrgCSet_Aux;
	SG_ERR_CHECK(  _v_process_ancestor_entry(pCtx, pszKey_Gid_Entry, pMrgCSetEntry_Aux, pData)  );
	pData->pMrgCSet_Ancestor = pMrgCSet_AncestorBackup;
	
	SG_UNUSED( pData );
	SG_UNUSED( pMrgCSet_Leaf_k );
	SG_UNUSED( pMrgCSetEntry_Leaf_k );
	SG_UNUSED( pMrgCSet_Leaf_j );
	SG_UNUSED( pMrgCSetEntry_Leaf_j );
fail:
	return;
}

static void _dive_pseudo_dag(SG_context * pCtx,
							 struct _my_history_data * pMyHistoryData,
							 SG_vhash * pvhHistory,
							 SG_uint32 indent,
							 const char * pszHid_y,
							 SG_bool bAncestorOfIntersection)
{
	SG_vhash * pvh_y = NULL;		// we don't own this
	SG_uint32 idx;
	SG_uint32 nrParents;
	SG_uint32 z;
	SG_bool bIsAncestoryAlreadyKnown;
	SG_bool bIsInIntersection;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhHistory, pszHid_y, &pvh_y)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_y, "is_ancestor_of_intersection", &bIsAncestoryAlreadyKnown)  );
	if (bIsAncestoryAlreadyKnown)
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "%*cDivePseudoDag: taking short-cut for '%s'\n", indent,' ', pszHid_y)  );
#endif
		return;
	}

	if (bAncestorOfIntersection)
	{
#if TRACE_WC_MEREG
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "%*cDivePseudoDag: marking '%s'\n", indent,' ', pszHid_y)  );
#endif
		SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_y, "is_ancestor_of_intersection", SG_TRUE)  );
	}

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_y, "is_in_intersection", &bIsInIntersection)  );
	SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh_y, "idx", &idx)  );
	
	SG_ERR_CHECK(  SG_history_result__set_index(pCtx, pMyHistoryData->pHistoryResult_k, idx)  );
	SG_ERR_CHECK(  SG_history_result__get_pseudo_parent__count(pCtx, pMyHistoryData->pHistoryResult_k, &nrParents)  );
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "%*cDivePseudoDag: diving on %d parents of '%s' with '%d':\n", indent,' ',
							   nrParents, pszHid_y, (bAncestorOfIntersection | bIsInIntersection))  );
#endif
	for (z=0; z<nrParents; z++)
	{
		const char * pszHid_z;

		SG_ERR_CHECK(  SG_history_result__set_index(pCtx, pMyHistoryData->pHistoryResult_k, idx)  );
		SG_ERR_CHECK(  SG_history_result__get_pseudo_parent(pCtx, pMyHistoryData->pHistoryResult_k, z, &pszHid_z, NULL)  );
		SG_ERR_CHECK(  _dive_pseudo_dag(pCtx, pMyHistoryData, pvhHistory, indent+2,
										pszHid_z, (bAncestorOfIntersection | bIsInIntersection))  );
	}

fail:
	return;
}

static void _find_per_item_ancestor_candidates(SG_context * pCtx,
											   _v_data * pData,
											   struct _my_history_data * pMyHistoryData,
											   SG_stringarray ** ppsaCandidates)
{
	SG_uint32 nrInIntersection;
	SG_vhash * pvhHistory = NULL;
	SG_stringarray * psaCandidates = NULL;
	SG_string * pStringRepoPath = NULL;

	SG_ERR_CHECK(  SG_rbtree__count(pCtx, pMyHistoryData->prbIntersection, &nrInIntersection)  );
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "FindPerItemAncestorCandidates: Top of Loop for [gid %s][nrInIntersection %d]\n",
							   pMyHistoryData->pszGid, nrInIntersection)  );
#endif
	if (nrInIntersection == 0)
	{
		switch (pData->pMrg->eFakeMergeMode)
		{
		//case SG_FAKE_MERGE_MODE__MERGE:
		//case SG_FAKE_MERGE_MODE__REVERT:
		default:
			// TODO 2012/09/21 Not sure that this could actually happen.
			// TODO            If nothing else, the intersection SHOULD
			// TODO            always have the CSet where this item was
			// TODO            placed under version control initially.
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "Empty intersection for [gid %s] between [kLeaf %s][jLeaf %s].",
							 pMyHistoryData->pszGid,
							 pMyHistoryData->pszHidCSet_k,
							 pMyHistoryData->pszHidCSet_j)  );

		case SG_FAKE_MERGE_MODE__UPDATE:
			{
				// 2013/01/14 Turns out this case can happen during an UPDATE.
				// Suppose you have a WC with a dirty/edited file, update to a changeset
				// where the file doesn't exist (yielding an "Added (Update)" status),
				// and *THEN* UPDATE AGAIN to a changeset where the file does exist.
				// This second update will see it as a file (with the same GID) in both
				// leaves (WC and the target cset); we will search for a common ancestor
				// (from the point of view of the baseline at the end of the first update
				// and which by construction doesn't have the file); and crawling back
				// through the history won't help.
				//
				// TODO 2013/01/14 It is tempting to suggest that we can simply substitute
				// TODO            one of the leaf values for the content HID in for the
				// TODO            ancestor, but it is not clear which one without leading
				// TODO            criss-cross-type problems.
				// TODO
				// TODO            I think it is better to just complain and let them fix
				// TODO            the dirt before doing the second update.

				sg_wc_liveview_item * pLVI = NULL;	// we do not own this
				SG_uint64 uiAliasGid;
				SG_bool bKnown;
				// TODO 2013/01/14 The WC-based version of the pMrgCSetEntry should have this field already.
				SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pData->pMrg->pWcTx->pDb,
																 pMyHistoryData->pszGid, &uiAliasGid)  );
				SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pData->pMrg->pWcTx, uiAliasGid,
																	 &bKnown, &pLVI)  );
				if (pLVI)
					SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pData->pMrg->pWcTx, pLVI,
																			  &pStringRepoPath)  );
				SG_ERR_THROW2(  SG_ERR_UPDATE_CONFLICT,
								(pCtx, "'%s'", ((pStringRepoPath) ? SG_string__sz(pStringRepoPath) : ""))  );
			}
		}
	}
	else if (nrInIntersection == 1)
	{
		const char * pszHid_0 = NULL;
		SG_bool bFound = SG_FALSE;

		// If the intersection has 1 item, then we have to use it.
		// This is the CSet that contains the last modification
		// that both branches share.
		//
		// We don't really care where this CSet is in the DAG relative
		// to the LCA and/or any SPCAs.
		//
		// (Each branch may have modified the file 100 times and in
		// different ways, but this is the last/only time that they were
		// sync'd.  It is important that we DID NOT request "hide-merges"
		// when we computed the histories.)
		//
		// Odds are this is the CSet that initially ADDED the item
		// (because common changes between the initial ADD and the fork
		// in the DAG should also appear in the intersection).
		//
		// Note that nothing here implies that this CSet is anywhere
		// near the actual fork in the DAG.

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, NULL, pMyHistoryData->prbIntersection,
												  &bFound, &pszHid_0, NULL)  );
#if TRACE_WC_MERGE
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
										   "FindPerItemAncestorCandidates[0]: %s\n", pszHid_0)  );
#endif
		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaCandidates, 1)  );
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaCandidates, pszHid_0)  );
	}
	else
	{
		// Walk the reassembled-pseudo-dag (the history-result) from 1 leaf
		// (it doesn't matter which) and build a vhash (keyed by CSet HID)
		// of the nodes.  (We need this because the history result has a
		// built-in index and we can't have multiple iterators on it at the
		// same time.  And because we need to hang more info from each CSet.)
		// 
		// Mark the nodes that are in the intersection.
		//
		// Mark the nodes that are ancestors of a node in the intersection
		// (by walking back the pseudo-parent edges).
		//
		// Nodes that are in the interesection *BUT NOT* the ancestor of an
		// intersection node are the candidate(s) we want to consider for
		// the item.  They are "CLOSEST" to the 2 branches being merged.
		// Where "closest" is defined as "most recent common change(s)
		// to this item along all unique paths"; this DOES NOT look at the
		// generation numbers nor the number of CSets where the item didn't
		// change.
		//
		// I'm going to call these candidate(s) the "leaves of the intersection".
		// That is, if we were to re-distill the reassembled-pseudo-dag another
		// time and only include the nodes in the intersection, these candidate(s)
		// would be the leaves in THAT dag; and the remaining intersection nodes
		// would be ancestors of those leaves.
		//
		// Normally, we would expect to get a single candidate.  But if there
		// are criss-crosses (or if we need to reject a candidate and re-run
		// the algorithm with the rejected candidate removed from the intersection
		// set), we can get more than one candidate.
		//

		SG_uint32 count;
		SG_uint32 y;
		SG_uint32 nrLeaves;
		SG_bool bMore;

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhHistory)  );
		SG_ERR_CHECK(  SG_history_result__set_index(pCtx, pMyHistoryData->pHistoryResult_k, 0)  );
		bMore = SG_TRUE;
		while (bMore)
		{
			const char * pszHidCSet;	// we do not own this
			SG_vhash * pvh_x = NULL;	// we do not own this
			SG_uint32 x;
			SG_bool bIsInIntersection;

			SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, pMyHistoryData->pHistoryResult_k, &pszHidCSet)  );
			SG_ERR_CHECK(  SG_history_result__get_index(pCtx, pMyHistoryData->pHistoryResult_k, &x)  );
			SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhHistory, pszHidCSet, &pvh_x)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_x, "idx", (SG_int64)x)  );
			SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMyHistoryData->prbIntersection, pszHidCSet, &bIsInIntersection, NULL)  );
			if (bIsInIntersection)
				SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_x, "is_in_intersection", bIsInIntersection)  );
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "StartingMap[%d] %s [IsInIntersection %d]\n",
									   x, pszHidCSet, bIsInIntersection)  );
#endif

			SG_ERR_CHECK(  SG_history_result__next(pCtx, pMyHistoryData->pHistoryResult_k, &bMore)  );
		}
		
		SG_ERR_CHECK(  SG_history_result__count(pCtx, pMyHistoryData->pHistoryResult_k, &count)  );
		for (y=0; y<count; y++)
		{
			const char * pszHid_y;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvhHistory, y, &pszHid_y, NULL)  );
			SG_ERR_CHECK(  _dive_pseudo_dag(pCtx, pMyHistoryData, pvhHistory, 2,
											pszHid_y, SG_FALSE)  );
		}
		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaCandidates, 1)  );
		nrLeaves = 0;
		for (y=0; y<count; y++)
		{
			const char * pszHid_y;
			SG_vhash * pvh_y;
			SG_bool bIsInIntersection;
			SG_bool bIsAncestorOfIntersection;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvhHistory, y, &pszHid_y, &pvh_y)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_y, "is_in_intersection", &bIsInIntersection)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_y, "is_ancestor_of_intersection", &bIsAncestorOfIntersection)  );
			if (bIsInIntersection && !bIsAncestorOfIntersection)
			{
#if TRACE_WC_MERGE
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
										   "FindPerItemAncestorCandidates[%d]: %s\n",
										   nrLeaves, pszHid_y)  );
#endif
				nrLeaves++;
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaCandidates, pszHid_y)  );
			}
		}
		SG_ASSERT_RELEASE_FAIL( (nrLeaves > 0) );
	}
	
	*ppsaCandidates = psaCandidates;
	psaCandidates = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhHistory);
	SG_STRINGARRAY_NULLFREE(pCtx, psaCandidates);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}
												   
//////////////////////////////////////////////////////////////////

/**
 * we get called once for each entry (file/symlink/directory) present
 * in a leaf version of the version control tree.
 *
 * if the entry was not "marked", then it was not referenced when we
 * walked the ancestor cset.  this means that it is not present in the
 * ancestor.  normally, this means that the entry was ADDED in this
 * branch, BUT WE HAVE TO CONFIRM THAT.
 *
 * since ADDS are unique (with a unique GID), it should not be
 * possible for this entry to appear in 2 leaves without having
 * appeared in the ancestor unless we have something like:
 *
 *****************************************************************
 * [Case 1] See W4060: 
 *                0
 *               / \.
 *              1   2
 *             / \ / \.
 *            3   4   5
 *             \ / \ /
 *              6   7
 *
 * or:
 * 
 *                0
 *               / \.
 *              1   2
 *             / \ / \.
 *            3   4a  5
 *            |   |   |
 *            |   4b  |
 *             \ / \ /
 *              6   7
 *
 * [] start with a file that exists in 0, 1, and 2.
 * [] modify the file in 3 and 5.
 * [] delete the file during the merge creating 4 (or
 *    in either 4a or 4b).
 * [] accept the modified (rather than deleted) version
 *    of the file when creating merges 6 and 7.
 *
 *****************************************************************
 * [Case 2] when there are SPCAs present, we have to allow
 * for the entry to have been created between the LCA and
 * the leaves.
 *
 *                 4
 *                / \.
 *               x   \.
 *               |    |
 *              S0    S1
 *               |\  /|
 *               | \/ |
 *               | /\ |
 *               |/  \|
 *               6    7
 * 
 * [] file created in x (so it does not exist in LCA 4).
 * [] file modified in 6 and 7.
 * 
 *****************************************************************
 * In both cases, when we try to merge 6 and 7, we consider this
 * portion of the DAG.
 *
 *                4
 *               / \.
 *              6   7
 *               \ /
 *                8
 *
 * or
 *
 *               4
 *              /  \.
 *            S0    S1
 *             |\  /|
 *             | \/ |
 *             | /\ |
 *             |/  \|
 *             6    7
 *              \  /
 *               8
 *  
 *
 * The file exists in both 6 and 7 (with the same GID),
 * but it does not exist in 4.  So there is no ancestor
 * version available for the 3-way merge (using just the LCA).
 * 
 *****************************************************************
 *
 * [] if it is a clean ADD (in this cset and without a peer
 *    in the other leaf), we can do a "plain add" -- we need
 *    to clone a new SG_mrg_cset_entry and add it to the result-cset.
 * [] if it has a peer in the other cset and both are equal,
 *    we don't really need the ancestor (since there's nothing
 *    to compare/merge), so we can do a "plain add" (marking
 *    the item processed in both leaves).
 * [] if the peers are different, search hard for the "best"
 *    ancestor.
 *
 */
static SG_rbtree_foreach_callback _v_process_leaf_k_entry_addition;

static void _v_process_leaf_k_entry_addition(SG_context * pCtx,
											 const char * pszKey_Gid_Entry,
											 void * pVoidAssocData_MrgCSetEntry_Leaf_k,
											 void * pVoid_Data)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Leaf_k;
	_v_data * pData = (_v_data *)pVoid_Data;
	struct _my_history_data * pMyHistoryData = NULL;
	SG_mrg_cset * pMrgCSet_Leaf_j = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry_Leaf_j = NULL;
	SG_uint32 nrLeaves, jLeaf;
	SG_stringarray * psaCandidates = NULL;

	if (pMrgCSetEntry_Leaf_k->markerValue != MARKER_UNHANDLED)
		return;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_leaf_added(
						pCtx,"_v_process_leaf_additions",
						pData->use_marker_value,
						pData->pMrgCSet_Leaf_k,pData->kLeaf,
						pMrgCSetEntry_Leaf_k)  );
#endif

	// See if "added" item is present in other leaves
	// before we simply assume it is an ADD.

	SG_ERR_CHECK(  SG_vector__length(pCtx, pData->pVec_MrgCSet_Leaves, &nrLeaves)  );
	if (nrLeaves != 2)
	{
		// TODO 2012/09/20 For now, to solve the W4060 problem I'm just going to
		// TODO            complain.  We don't really need this until we decide to
		// TODO            actually support n-way merges.
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}
		
	SG_ERR_CHECK(  _find_peer_in_other_leaf(pCtx, pData, pszKey_Gid_Entry,
											&jLeaf, &pMrgCSet_Leaf_j, &pMrgCSetEntry_Leaf_j)  );
	if (!pMrgCSetEntry_Leaf_j)
	{
		// no peer in the other leaf
		// This is a regular ADDED.
		pMrgCSetEntry_Leaf_k->markerValue = pData->use_marker_value;
		SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx, pData->pMrg,
																   pMrgCSetEntry_Leaf_k,
																   pData->pMrgCSet_Result,
																   NULL)  );
	}
	else
	{
		SG_mrg_cset_entry_neq neq = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_leaf_peer(
							pCtx,"_v_process_leaf_additions",
							pData->use_marker_value,
							pMrgCSet_Leaf_j, jLeaf,
							pMrgCSetEntry_Leaf_j)  );
#endif
		SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx, pMrgCSetEntry_Leaf_k, pMrgCSetEntry_Leaf_j, &neq)  );
		if (neq == SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL)
		{
			// item is identical in both leaves, just add like normal
			// since there isn't a conflict.
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Peer is identical [%s]\n", pszKey_Gid_Entry)  );
#endif
			pMrgCSetEntry_Leaf_j->markerValue = pData->use_marker_value;
			pMrgCSetEntry_Leaf_k->markerValue = pData->use_marker_value;
			SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx, pData->pMrg,
																	   pMrgCSetEntry_Leaf_k,
																	   pData->pMrgCSet_Result,
																	   NULL)  );
		}
		else
		{
			// item is different between the 2 leaves.  we need to work
			// a little harder to find an ancestor so we can decide what
			// the resulting item should be.
			//
			// Get the CSets where the item changed and those changes are
			// ancestors of both leaves.  These are the candidates dagnodes
			// that we want to consider.  Note that this set has almost
			// nothing to do with the set of CSets identified in the DAGLCA
			// computation (since files get edited all the time and not just
			// at just the merge/branch points).  Also note that we have to
			// compute this here (and not in an outer loop) because this
			// is a per-item history and thus is specific to this item.

			const char * pszHidBestCandidate = NULL;
			const SG_mrg_cset_entry * pMrgCSetEntry_BestCandidate = NULL;

			SG_ERR_CHECK(  _intersect_histories(pCtx, pData,
												pszKey_Gid_Entry,
												pData->pMrgCSet_Leaf_k->bufHid_CSet,
												pMrgCSet_Leaf_j->bufHid_CSet,
												&pMyHistoryData)  );
			while (1)
			{
				SG_uint32 nrCandidates = 0;
				SG_uint32 c;

				SG_ERR_CHECK(  _find_per_item_ancestor_candidates(pCtx, pData, pMyHistoryData, &psaCandidates)  );

				SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaCandidates, &nrCandidates)  );
				for (c=0; c<nrCandidates; c++)
				{
					const char * pszHid_c;
					const SG_mrg_cset_entry * pMrgCSetEntry_c = NULL;

					SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaCandidates, c, &pszHid_c)  );
					if (strcmp(pszHid_c, pData->pMrgCSet_Ancestor->bufHid_CSet) == 0)
					{
						// We already know that this item is not present in the LCA
						// so don't bother looking.  Treat this case as if we looked
						// and didn't find it.  That is, let the loop continue and
						// look at any peers in this tier.
#if TRACE_WC_MERGE
						SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
												   "SkippingCandidate[%d/%d]: for [gid %s] [cset %s] is LCA.\n",
												   c, nrCandidates, pszKey_Gid_Entry, pszHid_c)  );
#endif
					}
					else
					{
						SG_ERR_CHECK(  _is_item_present_in_aux_ancestor(pCtx, pData, pszKey_Gid_Entry, pszHid_c, &pMrgCSetEntry_c)  );
						if (pMrgCSetEntry_c)
						{
							// The GID is present in this CSet, so we *could* use it.
							// 
							// If it is present in more than one candidate, then we have
							// a criss-cross edit.  Picking one candidate over the other
							// is probably wrong, unless the 2 candidates made the same
							// changes.

							if (pszHidBestCandidate)
							{
								SG_mrg_cset_entry_neq neq = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;
								SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx, pMrgCSetEntry_c, pMrgCSetEntry_BestCandidate, &neq)  );
								if (neq == SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL)
								{
									// both SPCAs made the same change.
									// use the first one we found.
#if TRACE_WC_MERGE
									SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
															   "Candidates: '%s' and '%s' made same change.\n",
															   pszHidBestCandidate, pszHid_c)  );
#endif
								}
								else
								{
									// the SPCAs made different changes.
									// discard this set of candidates and look at
									// their parents (in the redistilled pseudo-dag).
#if TRACE_WC_MERGE
									SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
															   "Disqualifying candidates: '%s' and '%s' (and any peers)\n",
															   pszHidBestCandidate, pszHid_c)  );
#endif
									pszHidBestCandidate = NULL;		// reject previously qualified node.
									pMrgCSetEntry_BestCandidate = NULL;
									break;
								}
							}
							else
							{
								// The first candidate CSet we have seen that actually has
								// the item/GID in it.  We say that this CSet is "qualified".
								pszHidBestCandidate = pszHid_c;
								pMrgCSetEntry_BestCandidate = pMrgCSetEntry_c;
							}
						}
						else
						{
							// The CSet does not have the item/GID in it.  This should mean
							// that the item was deleted *IN THIS CSET*.  We cannot use this
							// candidate (because we are looking for the last (non-deleted)
							// version of the item to be the center panel.
							//
							// Keep looking at the other candidates and/or don't complain if
							// we already have a qualified candidate.  (If a DELETE and an
							// EDIT are peers in this redistilled pseudo-dag, we can want to
							// take the edited one.)
						}
					}
				}

				if (pszHidBestCandidate)
				{
					// If we have a single qualified CSet, use it.
					SG_ERR_CHECK(  _use_per_item_ancestor(pCtx, pData, pszKey_Gid_Entry,
														  pData->pMrgCSet_Leaf_k, pMrgCSetEntry_Leaf_k,
														  pMrgCSet_Leaf_j,        pMrgCSetEntry_Leaf_j,
														  pszHidBestCandidate)  );
					break;	// while(1)
				}
				else
				{
					// We eithed did not find a qualified candidate or we rejected them.
					// Remove the set of candidates from the intersection and redistill
					// the pseudo-dag so that we get the next "tier" of candidates.
					SG_ERR_CHECK(  _remove_candidates_from_intersection(pCtx, pMyHistoryData->prbIntersection, psaCandidates)  );
					SG_STRINGARRAY_NULLFREE(pCtx, psaCandidates);
					continue;	// while(1) -- try again with next tier
				}
			}
				
#if 1
			// TODO... verify that we handled everything
			SG_ASSERT_RELEASE_FAIL( (pMrgCSetEntry_Leaf_j->markerValue == pData->use_marker_value) );
			SG_ASSERT_RELEASE_FAIL( (pMrgCSetEntry_Leaf_k->markerValue == pData->use_marker_value) );
#endif			
		}
	}

fail:
	SG_MY_HISTORY_DATA__NULLFREE(pCtx, pMyHistoryData);
	SG_STRINGARRAY_NULLFREE(pCtx, psaCandidates);
}

//////////////////////////////////////////////////////////////////

/**
 * we get called once for each entry present in the result-cset.
 * we check to see if we are an orphan.  that is, don't have a parent
 * directory.  This can happen if one branch moves us to a directory
 * that another branch deleted.
 *
 * If that happens, we cancel the delete for the parent directory
 * and give it a delete-conflict.  If we do cancel the delete, we
 * need to recursively visit the parent because it too may now be
 * an orphan.
 *
 * Because we are iterating on the entry-list, we cache the entries
 * in a orphan-to-add-list and then add them the entry-list after we have
 * finished the walk.
 *
 * TODO We may get some false-positives here if the entry had a DIVERGENT_MOVE,
 * TODO because picked an arbitrary directory to put the entry in.  Perhaps,
 * TODO if we made a better choice, we could have avoided creating an orphan
 * TODO in the first place.
 */
static SG_rbtree_foreach_callback _v_find_orphans;

static void _v_find_orphans(SG_context * pCtx,
							SG_UNUSED_PARAM(const char * pszKey_Gid_Entry),
							void * pVoidAssocData_MrgCSetEntry_Result,
							void * pVoid_Data)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Result = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Result;
	_v_data * pData = (_v_data *)pVoid_Data;

	SG_mrg_cset_entry * pMrgCSetEntry_Result_Parent;		// this entry's parent in the result-cset
	SG_mrg_cset_entry * pMrgCSetEntry_Parent_Ancestor;		// this entry's parent in the ancestor-cset
	SG_bool bFound;
	SG_bool bKnown;
	sg_wc_liveview_item * pLVI;
	SG_string * pStringOrphanRepoPath = NULL;

	SG_UNUSED(pszKey_Gid_Entry);

	if (pMrgCSetEntry_Result->bufGid_Parent[0] == 0)		// the root node
		return;

	// look in the result-cset and see if the parent entry is present in the entry-list.

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx,
										  pData->pMrgCSet_Result->prbEntries,
										  pMrgCSetEntry_Result->bufGid_Parent,
										  &bFound,
										  (void **)&pMrgCSetEntry_Result_Parent)  );
	if (bFound)
		return;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_find_orphans: orphan [entry %s] [%s] needs [parent entry %s]\n",
							   pMrgCSetEntry_Result->bufGid_Entry,
							   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname),
							   pMrgCSetEntry_Result->bufGid_Parent)  );
#endif

	// 2012/03/15 If the orphan is an UNCONTROLLED (FOUND/IGNORED) item
	// (remember we lied to the MERGE code when we used __using_wc() to
	// load the "baseline" CSET) -- then let's just complain and stop
	// the music.  I'm not sure I want to undelete the parent directory
	// for trash that happened to be in the WD.  I think I'd rather let
	// them remove the trash and retry the merge.

	SG_ERR_CHECK_RETURN(  sg_wc_tx__liveview__fetch_random_item(pCtx, pData->pMrg->pWcTx,
																pMrgCSetEntry_Result->uiAliasGid,
																&bKnown, &pLVI)  );
	if (bKnown && SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pData->pMrg->pWcTx, pLVI,
																  &pStringOrphanRepoPath)  );
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "The item '%s' would become orphaned.",
						 SG_string__sz(pStringOrphanRepoPath))  );
	}

	if (!pData->prbMrgCSetEntry_OrphanParents)
		SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pData->prbMrgCSetEntry_OrphanParents)  );

	// since the parent entry is not in the result-cset's entry-list, it will be
	// in the result-cset's deleted-list and therefore there are NO versions of
	// the entry in any of the leaves/branches.  the only version of the entry is
	// the ancestor one.

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx,
										  pData->pMrgCSet_Result->prbDeletes,
										  pMrgCSetEntry_Result->bufGid_Parent,
										  &bFound,
										  (void **)&pMrgCSetEntry_Parent_Ancestor)  );
	if (bFound)
	{
		// we are the first orphan to need this parent directory.
		// add it to the orphan-to-add-list and remove it from the delete-list.

		SG_ERR_CHECK_RETURN(  SG_rbtree__update__with_assoc(pCtx,pData->prbMrgCSetEntry_OrphanParents,
															pMrgCSetEntry_Parent_Ancestor->bufGid_Entry,
															pMrgCSetEntry_Parent_Ancestor,NULL)  );
		SG_ERR_CHECK_RETURN(  SG_rbtree__remove(pCtx,pData->pMrgCSet_Result->prbDeletes,pMrgCSetEntry_Result->bufGid_Parent)  );
		// see if parent directory is an orphan, too.

		SG_ERR_CHECK_RETURN(  _v_find_orphans(pCtx,
											  pMrgCSetEntry_Parent_Ancestor->bufGid_Entry,
											  pMrgCSetEntry_Parent_Ancestor,
											  pData)  );
	}
	else
	{
		// another peer-orphan has already taken care of adding our parent to the orphan-to-add-list.

#if defined(DEBUG)
		SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx,
											  pData->prbMrgCSetEntry_OrphanParents,
											  pMrgCSetEntry_Result->bufGid_Parent,
											  &bFound,
											  (void **)&pMrgCSetEntry_Parent_Ancestor)  );
		SG_ASSERT(  (bFound)  );
#endif
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringOrphanRepoPath);
}

/**
 * we get called once for each parent entry that we need to un-delete
 * so that an orphaned entry will have a parent.
 */
static SG_rbtree_foreach_callback _v_add_orphan_parents;

static void _v_add_orphan_parents(SG_context * pCtx,
								  const char * pszKey_Gid_Entry,
								  void * pVoidAssocData_MrgCSetEntry_Parent_Ancestor,
								  void * pVoid_Data)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Ancestor = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Parent_Ancestor;
	_v_data * pData = (_v_data *)pVoid_Data;

	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry_Baseline = NULL;			// we do not own this
	SG_mrg_cset_entry * pMrgCSetEntry_Composite = NULL;			// we do not own this
	SG_uint32 nrLeaves, kLeaf;
	SG_bool bFound;

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->pMrg->pMrgCSet_Baseline->prbEntries, pszKey_Gid_Entry,
								   &bFound, (void **)&pMrgCSetEntry_Baseline)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_add_orphan_parents: [entry %s] [ancestor entryname %s][baseline entryname %s]\n",
							   pszKey_Gid_Entry,
							   SG_string__sz(pMrgCSetEntry_Ancestor->pStringEntryname),
							   ((pMrgCSetEntry_Baseline) ? SG_string__sz(pMrgCSetEntry_Baseline->pStringEntryname) : "(null)"))  );
#endif

	SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__alloc(pCtx,
													 pData->pMrgCSet_Result,
													 pMrgCSetEntry_Ancestor,
													 pMrgCSetEntry_Baseline,
													 &pMrgCSetEntryConflict)  );

	// by definition, the entry was deleted in all branches/leaves.

	SG_ERR_CHECK(  SG_vector__length(pCtx,pData->pVec_MrgCSet_Leaves,&nrLeaves)  );
	for (kLeaf=0; kLeaf<nrLeaves; kLeaf++)
	{
		SG_mrg_cset * pMrgCSet_Leaf_k = NULL;

		SG_ERR_CHECK(  SG_vector__get(pCtx,pData->pVec_MrgCSet_Leaves,kLeaf,(void **)&pMrgCSet_Leaf_k)  );
		SG_ERR_CHECK(  SG_mrg_cset_entry_conflict__append_delete(pCtx,pMrgCSetEntryConflict,pMrgCSet_Leaf_k)  );
	}

	if (pMrgCSetEntry_Baseline)
		SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx,
																   pData->pMrg,
																   pMrgCSetEntry_Baseline,
																   pData->pMrgCSet_Result,
																   &pMrgCSetEntry_Composite)  );
	else
		SG_ERR_CHECK(  SG_mrg_cset_entry__clone_and_insert_in_cset(pCtx,
																   pData->pMrg,
																   pMrgCSetEntry_Ancestor,
																   pData->pMrgCSet_Result,
																   &pMrgCSetEntry_Composite)  );

	pMrgCSetEntry_Composite->pMrgCSetEntryConflict = pMrgCSetEntryConflict;
	pMrgCSetEntryConflict->pMrgCSetEntry_Composite = pMrgCSetEntry_Composite;
	pMrgCSetEntryConflict->flags = SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN;

	SG_ERR_CHECK(  SG_mrg_cset__register_conflict(pCtx,pData->pMrgCSet_Result,pMrgCSetEntryConflict)  );
	pMrgCSetEntryConflict = NULL;					// the result-cset owns it now
	return;

fail:
	SG_MRG_CSET_ENTRY_CONFLICT_NULLFREE(pCtx, pMrgCSetEntryConflict);
}

//////////////////////////////////////////////////////////////////

/**
 * we get called once for each entry present in the result-cset.
 * we check to see if we are a moved directory that is in a pathname cycle.
 * 
 * this can happen when dir_a is moved into dir_b in one branch and dir_b is moved into
 * dir_a in another branch, so that if we populated the WD, we'd have .../dir_a/dir_b/dir_a/dir_b/...
 * that is the essense of the problem; there may be other intermediate directories involved,
 * such as .../dir_a/.../dir_b/.../dir_a/.../dir_b/...
 *
 * TODO since this can only happen when dir_a and dir_b are moved (because we assume that
 * TODO the tree was well formed in the ancestor version of the tree), i think that it is
 * TODO safe to say that we can mark each of the participating directory-moves as a conflict
 * TODO (and move them back to where they were in the baseline/ancestor version of the tree (as we
 * TODO do for divergent moves)) and let the user decide what to do.  this should let us
 * TODO populate the WD at least.
 *
 * this is kind of obscure, but if we have to un-move a directory, it could now be an
 * orphan (if the parent directory in the baseline/ancestor version was also deleted in a branch).
 * so when we do the un-move, we do the orphan check and add the parent to the orphan-to-add-list.
 */
static SG_rbtree_foreach_callback _v_find_path_cycles;

static void _v_find_path_cycles(SG_context * pCtx,
								SG_UNUSED_PARAM(const char * pszKey_Gid_Entry),
								void * pVoidAssocData_MrgCSetEntry_Result,
								void * pVoid_Data)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Result = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Result;
	_v_data * pData = (_v_data *)pVoid_Data;
	SG_rbtree * prbVisited = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry_Ancestor;
	SG_mrg_cset_entry * pMrgCSetEntry_Result_TreeAncestor;
	SG_bool bFound;
	SG_bool bVisited;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_find_path_cycles: top [entry %s/%s] [%s]\n",
							   ((pMrgCSetEntry_Result->bufGid_Parent[0]) ? pMrgCSetEntry_Result->bufGid_Parent : "(null)"),
							   pMrgCSetEntry_Result->bufGid_Entry,
							   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );
#endif

	SG_UNUSED(pszKey_Gid_Entry);

	if (pMrgCSetEntry_Result->bufGid_Parent[0] == 0)		// the root node
		return;

	if (pMrgCSetEntry_Result->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		return;

	if (pMrgCSetEntry_Result->pMrgCSetEntryConflict)
	{
		if (pMrgCSetEntry_Result->pMrgCSetEntryConflict->flags
			& (SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE | SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE))
		{
			// we already have a MOVE conflict.
			if (pMrgCSetEntry_Result->pMrgCSetEntryConflict->pMrgCSetEntry_Baseline)
			{
				// we arbitrarily set the new target directory to the where it was
				// in the baseline.  this lets the user see it where it where they
				// last saw it before the merge.
				pMrgCSetEntry_Ancestor = pMrgCSetEntry_Result->pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor;
			}
			else
			{
				// the entry was not present in baseline, so we (arbitrarily) set
				// the target directory to where it was in the ancestor.  so technically,
				// this entry doesn't look like it moved (at least until the user decides
				// how to resolve the conflict).
				return;
			}
		}
		else
		{
			// we already have a link to what this entry looked like in the ancestor version of the tree.
			pMrgCSetEntry_Ancestor = pMrgCSetEntry_Result->pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor;
		}
	}
	else
	{
		// find the ancestor version of this entry.
		SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx,pData->pMrgCSet_Ancestor->prbEntries,
											  pMrgCSetEntry_Result->bufGid_Entry,
											  &bFound,
											  (void **)&pMrgCSetEntry_Ancestor)  );
		if (!bFound)	// this directory was not present in the ancestor version of the tree,
			return;		// so it cannot be a move.
	}

	if (strcmp(pMrgCSetEntry_Result->bufGid_Parent,pMrgCSetEntry_Ancestor->bufGid_Parent) == 0)
		return;			// not a move.

	// we have a moved-directory.  walk up the tree (in the result-cset) and see
	// whether we reach the root directory or if we visit this node again.

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx,pData->pMrgCSet_Result->prbEntries,
										  pMrgCSetEntry_Result->bufGid_Parent,
										  &bFound,(void **)&pMrgCSetEntry_Result_TreeAncestor)  );
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_find_path_cycles: loop-start [entry %s] [%s] -->\n",
							   pMrgCSetEntry_Result->bufGid_Entry,
							   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );
#endif
	while (bFound)
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "_v_find_path_cycles: loop-body          --> [anc %s] [%s]\n",
								   pMrgCSetEntry_Result_TreeAncestor->bufGid_Entry,
								   SG_string__sz(pMrgCSetEntry_Result_TreeAncestor->pStringEntryname))  );
#endif

		if (strcmp(pMrgCSetEntry_Result->bufGid_Entry,pMrgCSetEntry_Result_TreeAncestor->bufGid_Entry) == 0)
		{
			// Oops, we are our own tree-ancestor in the new result-cset.
			//
			// we found a repo-path-cycle, so this directory is an ancestor of itself.
			// add it to the cycle-to-add-list.  we defer making the actual change to this
			// entry because we want to get the other participants marked too.

#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "_v_find_path_cycles: moved directory in path cycle [entry %s] [%s]\n",
									   pMrgCSetEntry_Result->bufGid_Entry,
									   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );	// cannot call __compute_repo_path_for_entry (obviously)
#endif

			if (!pData->prbMrgCSetEntry_DirectoryCycles)
				SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx,&pData->prbMrgCSetEntry_DirectoryCycles)  );

			SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx,pData->prbMrgCSetEntry_DirectoryCycles,
															 pMrgCSetEntry_Result->bufGid_Entry,
															 pMrgCSetEntry_Result)  );
			goto done;
		}

		if (pMrgCSetEntry_Result_TreeAncestor->bufGid_Parent[0] == 0)
		{
			// we reached the root node so we are not in a cycle
			goto done;
		}

		// see if there is a cycle above us that we are not a part of,
		// but prevents us from walking up the tree to the root.  for
		// example, if we are starting from 'z' and run into something
		// like:
		//        ..../a/c/a/c/a/c/a/x/y/z
		// i'm going to say that 'z' is not in a cycle, just a victim
		// of the 'a' and 'c' cycle.
		if (!prbVisited)
			SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbVisited)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbVisited, pMrgCSetEntry_Result_TreeAncestor->bufGid_Entry, &bVisited, NULL)  );
		if (bVisited)
		{
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
									   "_v_find_path_cycles: loop-stop already visited [anc %s] [%s]\n",
									   pMrgCSetEntry_Result_TreeAncestor->bufGid_Entry,
									   SG_string__sz(pMrgCSetEntry_Result_TreeAncestor->pStringEntryname))  );
#endif
			goto done;
		}
		SG_ERR_CHECK(  SG_rbtree__add(pCtx, prbVisited, pMrgCSetEntry_Result_TreeAncestor->bufGid_Entry)  );

		SG_ERR_CHECK(  SG_rbtree__find(pCtx,pData->pMrgCSet_Result->prbEntries,
									   pMrgCSetEntry_Result_TreeAncestor->bufGid_Parent,
									   &bFound,(void **)&pMrgCSetEntry_Result_TreeAncestor)  );
	}

done:
	;
fail:
	SG_RBTREE_NULLFREE(pCtx, prbVisited);
}

/**
 * we get called once for each entry we put in the cycle-to-add-list.
 *
 * convert the entry to a conflict (if it isn't already) and un-move it
 * back to the parent that it was in ancestor version of the tree.
 *
 * This un-move may cause us to become an orphan (if the parent was also
 * deleted by a branch/leaf), so we need to fix-up that too.
 */
static SG_rbtree_foreach_callback _v_add_path_cycle_conflicts_1;
static SG_rbtree_foreach_callback _v_add_path_cycle_conflicts_2;
static SG_rbtree_foreach_callback _v_add_path_cycle_conflicts_3;

static void _v_add_path_cycle_conflicts_1(SG_context * pCtx,
										  const char * pszKey_Gid_Entry,
										  void * pVoidAssocData_MrgCSetEntry_Result,
										  void * pVoid_Data)
{
	//////////////////////////////////////////////////////////////////
	// part 1: get CONFLICT structure setup and linked.
	//////////////////////////////////////////////////////////////////

	SG_mrg_cset_entry * pMrgCSetEntry_Result = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Result;
	_v_data * pData = (_v_data *)pVoid_Data;

	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict_Allocated = NULL;
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict;
	SG_mrg_cset_entry * pMrgCSetEntry_Ancestor;
	SG_mrg_cset_entry * pMrgCSetEntry_Baseline;
	SG_bool bFound;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_add_path_cycle_1: top [entry %s] [%s]\n",
							   pMrgCSetEntry_Result->bufGid_Entry,
							   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );
#endif

	if (pMrgCSetEntry_Result->pMrgCSetEntryConflict)
	{
		// a conflict of some kind (which may or may not include directory moves)

		pMrgCSetEntryConflict = pMrgCSetEntry_Result->pMrgCSetEntryConflict;
		pMrgCSetEntry_Ancestor = pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor;
		pMrgCSetEntry_Baseline = pMrgCSetEntryConflict->pMrgCSetEntry_Baseline;

		if (pMrgCSetEntry_Baseline)
		{
			// if entry was present in the baseline, we arbitrarily chose the
			// parent directory where the entry was in the baseline version of
			// the tree.
			SG_ASSERT(  ((pMrgCSetEntryConflict->flags & (SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE
														  | SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE)) != 0)  );
		}
		else
		{
			// if the entry was not present in the baseline, we arbitrarily
			// chose the parent directory where the entry was in the ancestor
			// version of the tree.
			// 
			// since we used the ancestor version we didn't add this entry to
			// the path-cycle list.  so we should never see these types of conflicts.
			SG_ASSERT(  (pMrgCSetEntryConflict->flags & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE) == 0  );
			SG_ASSERT(  (pMrgCSetEntryConflict->flags & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE) == 0  );
		}

		SG_ASSERT(  (pMrgCSetEntry_Ancestor)  );
		SG_ASSERT(  (pMrgCSetEntryConflict->pMrgCSetEntry_Composite == pMrgCSetEntry_Result)  );
	}
	else
	{
		// convert non-conflict into a conflict.

		// find the baseline version (if present) and the ancestor version of this entry.

		SG_ERR_CHECK(  SG_rbtree__find(pCtx,
									   pData->pMrg->pMrgCSet_Baseline->prbEntries,
									   pszKey_Gid_Entry,
									   &bFound,
									   (void **)&pMrgCSetEntry_Baseline)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx,
									   pData->pMrgCSet_Ancestor->prbEntries,
									   pszKey_Gid_Entry,
									   &bFound,
									   (void **)&pMrgCSetEntry_Ancestor)  );
		SG_ASSERT(  bFound  );

		SG_ERR_CHECK(  _v_process_ancestor_entry__build_candidate_conflict(pCtx,
																		   pszKey_Gid_Entry,
																		   pMrgCSetEntry_Ancestor,
																		   pMrgCSetEntry_Baseline,
																		   pData,
																		   &pMrgCSetEntryConflict_Allocated)  );
		SG_ERR_CHECK(  SG_mrg_cset__register_conflict(pCtx,pData->pMrgCSet_Result,pMrgCSetEntryConflict_Allocated)  );
		pMrgCSetEntryConflict = pMrgCSetEntryConflict_Allocated;	// the result-cset owns it now
		pMrgCSetEntryConflict_Allocated = NULL;
		
		pMrgCSetEntry_Result->pMrgCSetEntryConflict = pMrgCSetEntryConflict;
		pMrgCSetEntryConflict->pMrgCSetEntry_Composite = pMrgCSetEntry_Result;
	}

	pMrgCSetEntryConflict->flags |= SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE;

fail:
	SG_MRG_CSET_ENTRY_CONFLICT_NULLFREE(pCtx, pMrgCSetEntryConflict_Allocated);
	return;
}

static void _v_cycle_hint(SG_context * pCtx,
						  _v_data * pData,
						  SG_mrg_cset_entry * pMrgCSetEntry_Result,		// the dir we are building hints for
						  const char * pszGid_TreeAncestor)				// the parent/grandparent dir we are visiting in the recursion
{
	SG_mrg_cset_entry * pMrgCSetEntry_Result_TreeAncestor = NULL;	// we do not own this.
	SG_bool bFound;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_cycle_hint: top [entry %s] [%s]\n",
							   pMrgCSetEntry_Result->bufGid_Entry,
							   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );
#endif

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->pMrgCSet_Result->prbEntries,
								   pszGid_TreeAncestor,
								   &bFound, (void **)&pMrgCSetEntry_Result_TreeAncestor)  );
	SG_ASSERT(  (bFound)  );	// since the root dir can't be moved, we won't ever see this.
	
	if (strcmp(pMrgCSetEntry_Result->bufGid_Entry,
			   pMrgCSetEntry_Result_TreeAncestor->bufGid_Entry) == 0)
	{
		// our recursive walk up the tree reached the node we started with.
		// begin the hint with our entryname.

		SG_ERR_CHECK(  SG_string__append__sz(pCtx,
											 pMrgCSetEntry_Result->pMrgCSetEntryConflict->pStringPathCycleHint,
											 "...")  );
	}
	else
	{
		SG_ERR_CHECK(  _v_cycle_hint(pCtx, pData, pMrgCSetEntry_Result,
									 pMrgCSetEntry_Result_TreeAncestor->bufGid_Parent)  );

		// see if this tree-ancestor is significant (in the cycle directory) and add it
		// to this dir's list.  (since we want this dir's list to only include the dirs
		// that were moved and could possibly have participated in the cycle; we don't
		// want any of the noise dirs that just happen to be between the significant ones.)

		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prbMrgCSetEntry_DirectoryCycles,
									   pMrgCSetEntry_Result_TreeAncestor->bufGid_Entry,
									   &bFound, NULL)  );
		if (bFound)
			SG_ERR_CHECK(  SG_vector__append(pCtx,
											 pMrgCSetEntry_Result->pMrgCSetEntryConflict->pVec_MrgCSetEntry_OtherDirsInCycle,
											 pMrgCSetEntry_Result_TreeAncestor,
											 NULL)  );
	}

	SG_ERR_CHECK(  SG_string__append__sz(pCtx,
										 pMrgCSetEntry_Result->pMrgCSetEntryConflict->pStringPathCycleHint,
										 "/")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx,
										 pMrgCSetEntry_Result->pMrgCSetEntryConflict->pStringPathCycleHint,
										 SG_string__sz(pMrgCSetEntry_Result_TreeAncestor->pStringEntryname))  );
fail:
	return;
}

static void _v_add_path_cycle_conflicts_2(SG_context * pCtx,
										  const char * pszKey_Gid_Entry,
										  void * pVoidAssocData_MrgCSetEntry_Result,
										  void * pVoid_Data)
{
	//////////////////////////////////////////////////////////////////
	// part 2: build a table of the other moved directories that were
	// complicit to produce the cycle for this directory.  this is to
	// help RESOLVE be able to print useful info.  note that this table
	// should be thought of as a "merge wanted to mutually nest these
	// but couldn't".  and printing their repo-paths in "wanted-result-space"
	// isn't really possible.  and perhaps offer a hint as to what the
	// result wanted to be.
	//
	// we do this on all involved directories in part 2 *before* part 3
	// which will un-move all of them.
	//////////////////////////////////////////////////////////////////

	SG_mrg_cset_entry * pMrgCSetEntry_Result = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Result;
	_v_data * pData = (_v_data *)pVoid_Data;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_add_path_cycle_2: top [entry %s] [%s]\n",
							   pMrgCSetEntry_Result->bufGid_Entry,
							   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );
#endif

	SG_UNUSED( pszKey_Gid_Entry );

	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pMrgCSetEntry_Result->pMrgCSetEntryConflict->pVec_MrgCSetEntry_OtherDirsInCycle, 2)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pMrgCSetEntry_Result->pMrgCSetEntryConflict->pStringPathCycleHint)  );

	// walk up the directory tree (in the wanted-result-cset) and build
	// a list of the other dirs involved in the cycle and a hint string.

	SG_ERR_CHECK(  _v_cycle_hint(pCtx, pData, pMrgCSetEntry_Result, pMrgCSetEntry_Result->bufGid_Parent)  );
	
	SG_ERR_CHECK(  SG_string__append__sz(pCtx,
										 pMrgCSetEntry_Result->pMrgCSetEntryConflict->pStringPathCycleHint,
										 "/")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx,
										 pMrgCSetEntry_Result->pMrgCSetEntryConflict->pStringPathCycleHint,
										 SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "_v_cycle_hint: %s\n",
							   SG_string__sz(pMrgCSetEntry_Result->pMrgCSetEntryConflict->pStringPathCycleHint))  );
#endif

fail:
	return;
}

static void _v_add_path_cycle_conflicts_3(SG_context * pCtx,
										  const char * pszKey_Gid_Entry,
										  void * pVoidAssocData_MrgCSetEntry_Result,
										  void * pVoid_Data)
{
	//////////////////////////////////////////////////////////////////
	// part 3: un-move the directory back to some sane place.
	//////////////////////////////////////////////////////////////////

	SG_mrg_cset_entry * pMrgCSetEntry_Result = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry_Result;
	_v_data * pData = (_v_data *)pVoid_Data;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "_v_add_path_cycle_3: top [entry %s] [%s]\n",
							   pMrgCSetEntry_Result->bufGid_Entry,
							   SG_string__sz(pMrgCSetEntry_Result->pStringEntryname))  );
#endif

	SG_UNUSED( pszKey_Gid_Entry );

	// un-move the result-cset version of the entry to parent directory
	// where it was in either the baseline or the ancestor version of the tree.

	if (pMrgCSetEntry_Result->pMrgCSetEntryConflict->pMrgCSetEntry_Baseline)
		SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
												pMrgCSetEntry_Result->bufGid_Parent,SG_NrElements(pMrgCSetEntry_Result->bufGid_Parent),
												pMrgCSetEntry_Result->pMrgCSetEntryConflict->pMrgCSetEntry_Baseline->bufGid_Parent)  );
	else
		SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
												pMrgCSetEntry_Result->bufGid_Parent,SG_NrElements(pMrgCSetEntry_Result->bufGid_Parent),
												pMrgCSetEntry_Result->pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufGid_Parent)  );


	// see if the un-moved entry is now an orphan.  if so, put the parent on the orphan-to-add-list.
	SG_ERR_CHECK(  _v_find_orphans(pCtx,pMrgCSetEntry_Result->bufGid_Entry,pMrgCSetEntry_Result,pData)  );

fail:
	return;
}


//////////////////////////////////////////////////////////////////

static void _sg_mrg__compute_simpleV(SG_context * pCtx,
									 SG_mrg * pMrg,
									 const char * pszMnemonicName,
									 const char * pszCSetLabel,
									 const char * pszAcceptLabel,
									 SG_mrg_cset * pMrgCSet_Ancestor,
									 SG_vector * pVec_MrgCSet_Leaves,
									 SG_mrg_cset ** ppMrgCSet_Result)
{
	_v_data data;
	SG_uint32 kLeaf, nrLeaves;

	data.pMrg = NULL;
	data.pVec_MrgCSet_Leaves = NULL;
	data.pMrgCSet_Result = NULL;
	data.pMrgCSet_Ancestor = NULL;
	data.pMrgCSet_Leaf_k = NULL;
	data.kLeaf = 0;
	data.prbMrgCSetEntry_OrphanParents = NULL;
	data.prbMrgCSetEntry_DirectoryCycles = NULL;

	SG_NULLARGCHECK_RETURN(pMrg);
	// pszCSetLabel is optional
	SG_NULLARGCHECK_RETURN(pMrgCSet_Ancestor);
	SG_NULLARGCHECK_RETURN(pVec_MrgCSet_Leaves);
	SG_NULLARGCHECK_RETURN(ppMrgCSet_Result);

	SG_ERR_CHECK_RETURN(  SG_vector__length(pCtx,pVec_MrgCSet_Leaves,&nrLeaves)  );
	SG_ARGCHECK_RETURN(  (nrLeaves >= 2), nrLeaves  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__v_input(pCtx,"_sg_mrg__compute_simpleV",pszCSetLabel,pMrgCSet_Ancestor,pVec_MrgCSet_Leaves)  );
#endif

	// reset all markers on all entries in all of the leaf csets.  (later we can
	// tell which entries we did not examine (e.g. for added entries))
	// this is probably only necessary if we are in a complex merge and one of
	// these leaves have already been used in a partial merge.
	// TODO 2012/09/20 The above comment refers to a sub-merge in an planned
	// TODO            but not yet supported n-way merge.  We can probably get
	// TODO            rid of this step since we aren't going to try to do that
	// TODO            any more.
	SG_ERR_CHECK(  _v_set_all_markers(pCtx,pVec_MrgCSet_Leaves, MARKER_UNHANDLED)  );

	data.pMrg = pMrg;
	data.pVec_MrgCSet_Leaves = pVec_MrgCSet_Leaves;
	data.pMrgCSet_Ancestor = pMrgCSet_Ancestor;

	// create a CSET-like structure to contain the candidate merge.
	// this will be just like a regular SG_mrg_cset only it will be
	// completely synthesized in memory rather than reflecting a
	// tree on disk.
	//
	// note that bufHid_CSet will be empty.  cset-hid's are only
	// computed after a commit and MERGE *NEVER* AUTO-COMMITS.
	// (and besides, this merge may be part of a larger merge that
	// we don't know about.)
	//
	// also note that since we are synthesizing this, we don't yet
	// know anything about the root entries (pMrgCSetEntry_Root and
	// bufGid_Root), so they will be empty until we're done with
	// the scan.

	SG_ERR_CHECK(  SG_mrg_cset__alloc(pCtx,
									  pszMnemonicName,
									  pszCSetLabel,
									  pszAcceptLabel,
									  SG_MRG_CSET_ORIGIN__VIRTUAL,
									  SG_DAGLCA_NODE_TYPE__NOBODY,	// virtual nodes are not part of the DAGLCA
									  &data.pMrgCSet_Result)  );

	// take each entry in the ancestor and see if has been changed in any of the leaves
	// and create the appropriate entry in the result-cset.
	//
	// note that this will only get things that were present in the ancestor.  they may
	// or may not still be present in the leaves; they may or may not have been changed
	// in one or more branches.  IT WILL NOT GET entries that were added in one or more
	// branches.

	data.use_marker_value = MARKER_LCA_PASS;
	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,pMrgCSet_Ancestor->prbEntries,_v_process_ancestor_entry,(void **)&data)  );

	// look at the unmarked entries in each leaf.  these must be new entries added to the
	// tree within that branch and after the ancestor was created.  add them to the result-cset.

	data.use_marker_value = MARKER_ADDITION_PASS;
	for (kLeaf=0; kLeaf<nrLeaves; kLeaf++)
	{
		data.kLeaf = kLeaf;
		SG_ERR_CHECK(  SG_vector__get(pCtx,pVec_MrgCSet_Leaves,kLeaf,(void **)&data.pMrgCSet_Leaf_k)  );

		SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.pMrgCSet_Leaf_k->prbEntries,_v_process_leaf_k_entry_addition,(void **)&data)  );
	}

	data.use_marker_value = MARKER_FIXUP_PASS;

	// check for orphans.  this can happen when one leaf adds an entry to a directory or
	// moves an existing entry into a directory that was deleted by another leaf.  if we
	// have an orphan, we cancel the delete on the parent; this needs to add the parent
	// into the prbEntries, but we don't want to change the prbEntries while we are walking
	// it, so we put them in a orphan-to-add-list.

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.pMrgCSet_Result->prbEntries,_v_find_orphans,(void **)&data)  );
	if (data.prbMrgCSetEntry_OrphanParents)
	{
		SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.prbMrgCSetEntry_OrphanParents,_v_add_orphan_parents,(void **)&data)  );
		SG_RBTREE_NULLFREE(pCtx, data.prbMrgCSetEntry_OrphanParents);
	}

	// check for path cycles.  that is, directories that are their own ancestor in the tree.

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.pMrgCSet_Result->prbEntries,_v_find_path_cycles,(void **)&data)  );
	if (data.prbMrgCSetEntry_DirectoryCycles)
	{
		SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.prbMrgCSetEntry_DirectoryCycles,_v_add_path_cycle_conflicts_1,(void **)&data)  );
		SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.prbMrgCSetEntry_DirectoryCycles,_v_add_path_cycle_conflicts_2,(void **)&data)  );
		SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.prbMrgCSetEntry_DirectoryCycles,_v_add_path_cycle_conflicts_3,(void **)&data)  );
		SG_RBTREE_NULLFREE(pCtx, data.prbMrgCSetEntry_DirectoryCycles);

		// this is kind of obscure, but if we had to un-move dir_a or dir_b, they could now be
		// orphans (if the parent directory in the ancestor version was deleted in a branch).

		if (data.prbMrgCSetEntry_OrphanParents)
		{
			SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,data.prbMrgCSetEntry_OrphanParents,_v_add_orphan_parents,(void **)&data)  );
			SG_RBTREE_NULLFREE(pCtx, data.prbMrgCSetEntry_OrphanParents);
		}
	}

	// TODO check for collisions and type 12345 issues.
	// TODO rewrite type 12345 notes in terms of new sg_mrg__ routines.

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dump_delete_list(pCtx,"SG_mrg__compute_simpleV",data.pMrgCSet_Result)  );
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dump_conflict_list(pCtx,"SG_mrg__compute_simpleV",data.pMrgCSet_Result)  );
#endif

	// we *DO NOT* set pMrg->pMrgCSet_FinalResult 
	// because we don't know whether we are an internal sub-merge or the top-level
	// final merge, so we just return it.  (This distinction is only significant
	// when we allow more than 2-way merges.)

	*ppMrgCSet_Result = data.pMrgCSet_Result;
	return;

fail:
	SG_MRG_CSET_NULLFREE(pCtx, (data.pMrgCSet_Result) );
	SG_RBTREE_NULLFREE(pCtx, data.prbMrgCSetEntry_OrphanParents);
	SG_RBTREE_NULLFREE(pCtx, data.prbMrgCSetEntry_DirectoryCycles);
}
//////////////////////////////////////////////////////////////////

static void _sg_mrg__compute_simple2(SG_context * pCtx,
									 SG_mrg * pMrg,
									 const char * pszMnemonicName,
									 const char * pszCSetLabel,
									 const char * pszAcceptLabel,
									 SG_mrg_cset * pMrgCSet_Ancestor,
									 SG_mrg_cset * pMrgCSet_Leaf_0,
									 SG_mrg_cset * pMrgCSet_Leaf_1,
									 SG_mrg_cset ** ppMrgCSet_Result)
{

	SG_vector * pVec = NULL;

	SG_NULLARGCHECK_RETURN(pMrg);
	SG_NULLARGCHECK_RETURN(pszMnemonicName);
	SG_NULLARGCHECK_RETURN(pszCSetLabel);
	SG_NULLARGCHECK_RETURN(pszAcceptLabel);
	SG_NULLARGCHECK_RETURN(pMrgCSet_Ancestor);
	SG_NULLARGCHECK_RETURN(pMrgCSet_Leaf_0);
	SG_NULLARGCHECK_RETURN(pMrgCSet_Leaf_1);
	SG_NULLARGCHECK_RETURN(ppMrgCSet_Result);

	SG_ERR_CHECK_RETURN(  SG_vector__alloc(pCtx,&pVec,2)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx,pVec,pMrgCSet_Leaf_0,NULL)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx,pVec,pMrgCSet_Leaf_1,NULL)  );

	SG_ERR_CHECK(  _sg_mrg__compute_simpleV(pCtx,
											pMrg,
											pszMnemonicName,
											pszCSetLabel,
											pszAcceptLabel,
											pMrgCSet_Ancestor,
											pVec,
											ppMrgCSet_Result)  );

fail:
	SG_VECTOR_NULLFREE(pCtx,pVec);
}

//////////////////////////////////////////////////////////////////

/**
 * Using all of the pre-loaded CSET info, attempt a
 * "simple2" merge.  The term "simple2" comes from
 * the previous PendingTree-based implementation and
 * means "2-leaves".  That is, a traditional 2-way
 * (ancestor, baseline, other) merge.  And is in
 * contrast to a more general n-way merge that was
 * planned (because there's nothing simple about merge).
 *
 * We compute the MERGE using the 3 in-memory
 * pMrgCSets for the ancestor, baseline, and
 * other CSets.  We create a composite in-memory
 * pMrgCSet representing the potential result.
 *
 */
void sg_wc_tx__merge__compute_simple2(SG_context * pCtx,
									  SG_mrg * pMrg)
{
	// Compute a candidate merge 'M':
	//
	//                  A
	//                 / \.
	//               L0   L1
	//                 \ /
	//                  M

	SG_ERR_CHECK(  _sg_mrg__compute_simple2(pCtx,pMrg,
											SG_MRG_MNEMONIC__M,
											SG_WC__STATUS_SUBSECTION__M,
											SG_MRG_ACCEPT_LABEL__M,
											pMrg->pMrgCSet_LCA,
											pMrg->pMrgCSet_Baseline,
											pMrg->pMrgCSet_Other,
											&pMrg->pMrgCSet_FinalResult)  );

fail:
	return;
}
