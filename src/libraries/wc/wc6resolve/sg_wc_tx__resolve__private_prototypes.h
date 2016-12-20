/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License"	);
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
 * @file sg_resolve_prototypes.h
 *
 * @details A module that helps the user resolve issues that merge could not solve automatically.
 *
 * Resolve works by considering each item (file, directory, symlink, submodule)
 * as a collection of state values.  The possible types of state values are
 * defined by the SG_resolve__state enumeration.  Each item has a value for each
 * of these states, and conflicts arise when merge is unable to determine the
 * correct value for one or more states on an item.
 *
 * Resolve starts by generating a set of choices.  Each choice regards a single
 * conflicting state on a single item.  A choice contains data that is useful
 * to the user when deciding what the final value for that state on that item
 * should be.  Most notably, the choice contains the various values assigned
 * to that state by changesets involved in the merge (the parent revisions, the
 * common ancestor revision, and the current working directory, usually).
 * Resolving the choice just means telling the choice which of these values
 * the item should use.
 *
 * Some choices are "mergeable", meaning that the type of state they're
 * associated with has values that can be merged, rather than being mutually
 * exclusive (this is most often the case with file contents).  In this case,
 * any already existing values (i.e. from parent revisions) can be merged
 * together to create new values using a merge tool (from SG_mergetool).
 * The newly created values can be accepted or merged further, just like any
 * of the original values, or when there is only a single leaf value remaining,
 * it is automatically accepted as the resolution.
 */

#ifndef H_SG_RESOLVE_PROTOTYPES_H
#define H_SG_RESOLVE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// TODO 2012/03/29 The contents of this file were moved out of
// TODO            src/libraries/include/sg_resolve_prototypes.h
// TODO            during the conversion from PendingTree to WC.
// TODO
// TODO            The symbols were declared PUBLIC from the point
// TODO            of view of sglib.
// TODO
// TODO            I think all of these routines should be PRIVATE
// TODO            to WC so I've moved them into header files marked
// TODO            WC-private, but I haven't taken time to rename
// TODO            with "sg_" or "_sg_" prefixes.
// TODO
//////////////////////////////////////////////////////////////////

/**
 * Allocates the RESOLVE data structure and populates it with CHOICES based
 * on the set of ISSUES identified by MERGE and any CHOICES already decided
 * by previous invocations of RESOLVE in the WD associated with the given WC TX.
 *
 * The pWcTX controls the scope of the WC TX; we DO NOT own it.
 * Rather **IT** will own us.  Any changes that RESOLVE wants to
 * make to the WD ****MUST**** be handed to WC-proper via the wc7txapi
 * and/or wc5queue APIs.
 *
 * THAT IS, YOU DO NOT OWN THE RETURNED pResolve, THE TX DOES.
 * 
 */
void SG_resolve__alloc(
	SG_context*     pCtx,         //< [in] [out] Error and context info.
	SG_wc_tx *      pWcTx,        //< [in]
	SG_resolve**    ppResolve     //< [out] The allocated resolve structure.
	);

#if defined(DEBUG)
#define SG_RESOLVE__ALLOC(pCtx, pWcTx, ppResolve)                              \
	SG_STATEMENT(                                                              \
		SG_resolve* _pResolve = NULL;                                          \
		SG_resolve__alloc(pCtx, pWcTx, &_pResolve);                            \
		_sg_mem__set_caller_data(_pResolve, __FILE__, __LINE__, "SG_resolve"); \
		*(ppResolve) = _pResolve;                                              \
	);
#else
#define SG_RESOLVE__ALLOC(pCtx, pWcTx, ppResolve) \
	SG_resolve__alloc(pCtx, pWcTx, ppResolve);
#endif

/**
 * Frees a resolve data structure.
 */
void SG_resolve__free(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_resolve* ppResolve //< [in] [out] The resolve structure to free.
	);

/**
 * NULLFREE macro for SG_resolve__free.
 */
#define SG_RESOLVE_NULLFREE(pCtx, pResolve) _sg_generic_nullfree(pCtx, pResolve, SG_resolve__free)

/**
 * Gets a count of how many choices are currently unresolved.
 * This is meant to be an easy way to check if there are unresolved conflicts,
 * for instance when attempting to commit.
 *
 * If you have an SG_resolve*, pass it.  If you don't and won't need one for
 * anything else, you can supply a pendingtree instead and this function will
 * handle the SG_resolve for you.  It is an error to pass both.
 */
void SG_resolve__get_unresolved_count(
	SG_context*       pCtx,     //< [in] [out] Error and context info.
	/*const*/ SG_resolve* pResolve, //< [in] Resolve instance to count unresolved choices in.
	SG_uint32*        pCount    //< [out] Number of unresolved choices/conflicts remaining.
	                            //<       If non-zero, things like commits should not be allowed.
	);

/**
 * Runs a viewer tool on a single value of a choice.
 * This is not implemented yet because we lack a system for viewer/editor tools.
 *//*
void SG_resolve__view_value(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to view.
	const char*        szTool, //< [in] Name of the viewer tool to use.
	                           //<      If NULL, a default tool is chosen.
	char**             ppTool  //< [out] Name of the viewer tool used.
	                           //<       Caller owns this.
	                           //<       May be NULL if unneeded.
	);
*/
/**
 * Runs a diff tool on two values.
 */
void SG_resolve__diff_values(
	SG_context*        pCtx,          //< [in] [out] Error and context info.
	SG_resolve__value* pLeft,         //< [in] The value to treat as the left value in the diff tool.
	SG_resolve__value* pRight,        //< [in] The value to treat as the right value in the diff tool.
	const char*        szToolContext, //< [in] The difftool context to invoke the diff tool from.
	                                  //<      One of SG_DIFFTOOL__CONTEXT__*.
	const char*        szTool,        //< [in] Name of the diff tool to use.
	                                  //<      If NULL, a default tool is chosen.
	char**             ppTool         //< [out] Name of the diff tool used.
	                                  //<       Caller owns this.
	                                  //<       May be NULL if unneeded.
	);

/**
 * Runs a merge tool on two values in a mergeable choice.
 */
void SG_resolve__merge_values(
	SG_context*         pCtx,          //< [in] [out] Error and context info.
	SG_resolve__value*  pBaseline,     //< [in] The value to treat as the baseline value in the merge tool.
	SG_resolve__value*  pOther,        //< [in] The value to treat as the other value in the merge tool.
	const char*         szTool,        //< [in] Name of the merge tool to use.
	                                   //<      If NULL, a default tool is chosen.
	const char*         szLabel,       //< [in] A label to assign to the result value, or NULL to use a default.
	                                   //<      If a value already exists with this label, then:
	                                   //<      1) If it's an automatically generated value, it is an error.
	                                   //<      2) If it's a manually generated value, it is overwritten.
	SG_bool             bAccept,       //< [in] If the merge succeeds and the choice is unresolved, then this
	                                   //<      flag specifies whether or not the new merge value should also
	                                   //<      be accepted automatically.  This flag is unused otherwise.
	SG_int32*           pResultCode,   //< [out] The result code returned by the merge tool.
	                                   //<       Either SG_MERGETOOL__RESULT__* or SG_FILETOOL__RESULT__*.
	SG_resolve__value** ppMergeableResultValue, //< [out] The value resulting from the merge.  May be NULL if unneeded.
	                                   //<       Set to NULL if the merge tool produced no output
	                                   //<       or if pResultCode is SG_MERGETOOL__RESULT__CANCEL.
	char**              ppTool         //< [out] Name of the diff tool used.
	                                   //<       Caller owns this.
	                                   //<       May be NULL if unneeded.
	);

/**
 * Resolves a choice by accepting a given value.
 * This will overwrite any already accepted value on the choice.
 */
void SG_resolve__accept_value(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	SG_resolve__value* pValue //< [in] The value to accept for the choice.
	);

/**
 * Finds an item in a resolve by GID.
 */
void SG_resolve__check_item__gid(
	SG_context*        pCtx,     //< [in] [out] Error and context info.
	SG_resolve*        pResolve, //< [in] The resolve structure to find an item in.
	const char*        szGid,    //< [in] The GID of the item to find.
	SG_resolve__item** ppItem    //< [out] The requested item.
	                             //<       NULL if the item wasn't found in the resolve.
	);

/**
 * Finds an item in a resolve by GID.
 * Throws SG_ERR_NOT_FOUND if the item isn't found.
 */
void SG_resolve__get_item__gid(
	SG_context*        pCtx,     //< [in] [out] Error and context info.
	SG_resolve*        pResolve, //< [in] The resolve structure to find an item in.
	const char*        szGid,    //< [in] The GID of the item to find.
	SG_resolve__item** ppItem    //< [out] The requested item.
	                             //<       NULL if the item wasn't found in the resolve.
	);

/**
 * Iterates through items involved in the resolve.
 * Items are returned in order of preferred resolution.
 */
void SG_resolve__foreach_item(
	SG_context*                 pCtx,            //< [in] [out] Error and context info.
	SG_resolve*                 pResolve,        //< [in] The resolve structure to iterate items in.
	SG_bool                     bOnlyUnresolved, //< [in] If SG_TRUE, only iterate unresolved items.
	SG_resolve__item__callback* fCallback,       //< [in] Callback to supply each item to.
	void*                       pUserData        //< [in] User data to supply to the callback.
	);

/**
 * Iterates through choices on items involved in the resolve.
 * This is a shortcut for a nested loop using SG_resolve__foreach_item
 * and then SG_resolve__item__foreach_choice on each item.
 */
void SG_resolve__foreach_choice(
	SG_context*                   pCtx,            //< [in] [out] Error and context info.
	SG_resolve*                   pResolve,        //< [in] The resolve structure to iterate choices in.
	SG_bool                       bOnlyUnresolved, //< [in] If SG_TRUE, only iterate unresolved items/choices.
	SG_bool                       bGroupByState,   //< [in] If SG_TRUE, returned choices are grouped by state.
	                                               //<      If SG_FALSE, returned choices are grouped by filename.
	SG_resolve__choice__callback* fCallback,       //< [in] Callback to supply each choice to.
	void*                         pUserData        //< [in] User data to supply to the callback.
	);

/**
 *
 *
 */
void SG_resolve__hack__get_repo(SG_context * pCtx,
								SG_resolve * pResolve,
								SG_repo ** ppRepo);

END_EXTERN_C;

#endif
