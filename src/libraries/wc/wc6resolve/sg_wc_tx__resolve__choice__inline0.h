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

#ifndef H_SG_WC_TX__RESOLVE__CHOICE__INLINE0_H
#define H_SG_WC_TX__RESOLVE__CHOICE__INLINE0_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Data about a single choice on an item.
 * Each choice regards one particular state on the item.
 * So the combination of an item and state uniquely identify a choice.
 */
struct _sg_resolve__choice
{
	SG_resolve__item*                pItem;           //< The item that owns this choice.
	SG_resolve__state                eState;          //< The conflicting state that the choice is about on the item.
	SG_mrg_cset_entry_conflict_flags eConflictCauses; //< Conflict flag(s) that caused this choice to be created.
	SG_bool                          bCollisionCause; //< Whether or not a collision helped cause this choice.
	SG_wc_port_flags                 ePortCauses;     //<Whether or not a portability problem helped cause this choice.
	SG_pathname*                     pTempPath;       //< Absolute path where temporary files related to the choice are stored.
	                                                  //< Might be NULL for choices that don't require temporary files.
	SG_vector*                       pValues;         //< The set of values associated with the choice.
	                                                  //< This is ordered such that a value's parents always come BEFORE it in the list.
	                                                  //< Vector contains SG_resolve__value* values.
	SG_vector*                       pLeafValues;     //< List of the current leaf values after considering completed merges.
	                                                  //< NULL if the choice is non-mergeable (because it will always equal pValues).
	                                                  //< Vector contains SG_resolve__value* values.
	                                                  //< Points into pValues.
	SG_resolve__value*               pMergeableResult; //< The resolved value for a file-content choice.
	                                                  //< NULL if the choice is unresolved or of different type.
	                                                  //< Points into pValues.

	// only used when bCollisionCause == SG_TRUE
	// NULL otherwise
	SG_vector*                       pCollisionItems; //< List of other items that are involved in the collision causing this choice.
	                                                  //< Vector contains SG_resolve__item* values.
	                                                  //< Points to other items in the same resolve.

	// only used when eConflictCauses includes SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE
	// NULL otherwise
	SG_vector*                       pCycleItems;     //< List of other items that are involved in the cycle causing this choice.
	                                                  //< Vector contains SG_resolve__item* values.
	                                                  //< Points to other items in the same resolve.
	char*                            szCycleHint;     //< A hint provided by merge about the choice's cycle.
	                                                  //< Currently this is a cycling repo path that merge wanted to create, but couldn't.

	// only used when ePortCauses != 0
	// NULL otherwise
	SG_vector*                       pPortabilityCollisionItems; //< List of other items that are involved in the potential collision causing this choice.
	                                                  //< Vector contains SG_resolve__item* values.
	                                                  //< Points to other items in the same resolve.

};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__CHOICE__INLINE0_H
