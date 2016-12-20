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

#ifndef H_SG_RESOLVE_CHOICE_PROTOTYPES_H
#define H_SG_RESOLVE_CHOICE_PROTOTYPES_H

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
 * Gets the item that contains a choice.
 */
void SG_resolve__choice__get_item(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to get the containing item for.
	SG_resolve__item**  ppItem   //< [out] The item that contains the choice.
	);

/**
 * Gets the item state that a choice is about.
 */
void SG_resolve__choice__get_state(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to get the state of.
	SG_resolve__state*  pState   //< [out] The item state that the choice is about.
	);

// TODO 2012/01/20 Remove this from here.  I only added
// TODO            it so that mongoose can still include <sg.h>
// TODO            and until I can pull the resolve code out of
// TODO            'ut' and into 'wc'.
typedef SG_uint32 HACK___SG_mrg_cset_entry_conflict_flags;

/**
 * Gets information about the cause(s) of the choice.
 */
void SG_resolve__choice__get_causes(
	SG_context*                       pCtx,          //< [in] [out] Error and context info.
	SG_resolve__choice*               pChoice,       //< [in] The choice to get the cause(s) of.
	HACK___SG_mrg_cset_entry_conflict_flags* pConflicts,    //< [out] Set of conflict flags that caused this choice.
	                                                 //<       May be NULL if unneeded.
	SG_bool*                          pCollision,    //< [out] Whether or not a collision caused this choice.
	                                                 //<       May be NULL if unneeded.
	SG_vhash**                        ppDescriptions //< [out] A hash of descriptions of the choice's causes.
	                                                 //<       Each key is a single name of a cause (i.e. "Divergent Edit").
	                                                 //<       Each value is a brief description of that cause.
	);

/**
 * Gets whether or not a choice has mergeable state values.
 * This is generally only the case with file content state values.
 */
void SG_resolve__choice__get_mergeable(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The choice to get the mergeable state of.
	SG_bool*            pMergeable //< [out] Whether or not the choice's state values are mergeable.
	);

/**
 * Gets whether or not a choice is considered resolved.
 *
 * A choice is considered resolved in two circumstances:
 * [1] The item-overall has been marked resolved (which
 *     implicitly resolves all choices (such as in a
 *     delete)).
 * [2] This particular choice has been marked resolved.
 *
 */
void SG_resolve__choice__get_resolved(
	SG_context*         pCtx,     //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,  //< [in] The choice to get the resolved state of.
	SG_bool * pbChoiceResolved
	);

/**
 * Iterates through the set of other items involved in the same collision.
 * The item that owns the choice will not be returned.
 * This function only returns data for choices caused by a collision.
 */
void SG_resolve__choice__foreach_colliding_item(
	SG_context*                 pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice*         pChoice,   //< [in] The choice to iterate colliding items in.
	SG_resolve__item__callback* fCallback, //< [in] The callback to supply colliding items to.
	void*                       pUserData  //< [in] User data to supply to the callback.
	);

void SG_resolve__choice__foreach_portability_colliding_item(
	SG_context*                 pCtx,
	SG_resolve__choice*         pChoice,
	SG_resolve__item__callback* fCallback,
	void*                       pUserData
	);

/**
 * Gets data specific to choices that are caused by path cycles.
 */
void SG_resolve__choice__get_cycle_info(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to retrieve cycle data from.
	const char**        ppPath   //< [out] The cycling pseudo-path that merge wanted to create.
	                             //<       i.e. dir1/dir2/dir3/dir1
	);

/**
 * Iterates through the set of other items involved in the same cycle.
 * The item that owns the choice will not be returned.
 * This function only returns data for choices caused by a path cycle.
 */
void SG_resolve__choice__foreach_cycling_item(
	SG_context*                 pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice*         pChoice,   //< [in] The choice to iterate cycling items in.
	SG_resolve__item__callback* fCallback, //< [in] The callback to supply cycling items to.
	void*                       pUserData  //< [in] User data to supply to the callback.
	);

/**
 * Gets a value from a choice by its label.
 */
void SG_resolve__choice__check_value__label(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to find a value in.
	const char*         szLabel, //< [in] The label of the value to retrieve.
	                             //<      Well-known value labels are defined by SG_RESOLVE__LABEL__*
	SG_int32            iIndex,  //< [in] Index to use for indexed labels (i.e. merge1, merge2, other3)
	                             //<      See well-known label definitions for which are indexed.
	                             //<      Ignored if 0, label indices are always 1-based.
	                             //<      Also ignored if 1, labels at index 1 don't actually have the suffix.
	                             //<      This allows iteration starting at 1 with no special case for the first label with no suffix.
	                             //<      A negative index starts from the last available index and works forward.
	                             //<      So -1 retrieves the value with the highest index, -2 the second highest, etc.
	SG_resolve__value** ppValue  //< [out] The requested value, or NULL if it wasn't found.
	);

/**
 * Gets a value from a choice by its label.
 * Throws SG_ERR_NOT_FOUND if the value isn't found.
 */
void SG_resolve__choice__get_value__label(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to find a value in.
	const char*         szLabel, //< [in] The label of the value to retrieve.
	                             //<      Well-known value labels are defined by SG_RESOLVE__LABEL__*
	SG_int32            iIndex,  //< [in] Index to use for indexed labels (i.e. merge1, merge2, other3)
	                             //<      See well-known label definitions for which are indexed.
	                             //<      Ignored if 0, label indices are always 1-based.
	                             //<      Also ignored if 1, labels at index 1 don't actually have the suffix.
	                             //<      This allows iteration starting at 1 with no special case for the first label with no suffix.
	                             //<      A negative index starts from the last available index and works forward.
	                             //<      So -1 retrieves the value with the highest index, -2 the second highest, etc.
	SG_resolve__value** ppValue  //< [out] The requested value.
	);

/**
 * Gets a value from a choice by its associated changeset's HID.
 *
 * This function will accept SG_RESOLVE__HID__WORKING and return the "live"
 * working value in that case, which is the one currently associated directly
 * with the working copy.  It is also possible to have working values which are
 * "saved", but those can only be retrieved via value iteration or with
 * SG_resolve__choice__get/check_value__changeset and specifying
 * SG_RESOLVE__LABEL__WORKING with an index.
 *
 * A choice can end up with "saved" working values if a state of the value in
 * the working copy at a specific point in time must be preserved.  This can
 * currently occur in two cases:
 *
 * 1) A live working value is merged with another value to create a new merge
 *    value.  To maintain historical integrity, the working value at the point
 *    of the merge must be preserved so that the new merge value always shows
 *    the same parent data, even if the working copy is later changed.
 * 2) A different value is accepted when the live working value has edits.  In
 *    this case the working value is saved before the working copy is
 *    overwritten by the accepted value, to make sure the user doesn't lose
 *    their edits.
 */
void SG_resolve__choice__check_value__changeset__mnemonic(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to find a value in.
	const char*         pszMnemonic,
	SG_resolve__value** ppValue  //< [out] The requested value, or NULL if it wasn't found.
	);

/**
 * Gets a value from a choice by its associated changeset's HID.
 * Throws SG_ERR_NOT_FOUND if the value isn't found.
 *
 * See SG_resolve__choice__check_value__changeset for caveats and information
 * about passing SG_RESOLVE__HID__WORKING to this function.
 */
void SG_resolve__choice__get_value__changeset__mnemonic(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to find a value in.
	const char*         pszMnemonic,
	SG_resolve__value** ppValue  //< [out] The requested value.
	);

/**
 * Gets the value that was accepted as the resolution of the choice.
 *
 * Note that this function can return NULL even if the choice is resolved.
 * This might occur if the choice was resolved using a method other than
 * accepting one of its values.  See SG_resolve__choice__get_resolved.
 */
void SG_resolve__choice__get_value__accepted(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to get the result from.
	SG_resolve__value** ppValue  //< [out] The value accepted as the choice's resolution.
	                             //<       NULL if none has been accepted.
	);

/**
 * Iterates through each value associated with a choice.
 */
void SG_resolve__choice__foreach_value(
	SG_context*                  pCtx,        //< [in] [out] Error and context info.
	SG_resolve__choice*          pChoice,     //< [in] The choice to iterate values in.
	SG_bool                      bOnlyLeaves, //< [in] If SG_TRUE, only iterate leaf values for mergeable choices.
	SG_resolve__value__callback* fCallback,   //< [in] The callback to supply values to.
	void*                        pUserData    //< [in] User data to supply to the callback.
	);

/**
 * An SG_resolve__choice__callback that counts how many times it is called.
 * This can effectively count the number of choices on an item.
 */
void SG_resolve__choice_callback__count(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The current choice.
	void*               pUserData, //< [in] An SG_uint32* to increment.
	SG_bool*            pContinue  //< [out] Always set to true.
	);

END_EXTERN_C;

#endif
