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
 *
 * @file sg_resolve_typedefs.h
 *
 * @details See sg_resolve_prototypes.h for a description of the resolve module.
 *
 */

#ifndef H_SG_RESOLVE_TYPEDEFS_H
#define H_SG_RESOLVE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// TODO 2012/03/29 The contents of this file were moved out of
// TODO            src/libraries/include/sg_resolve_typedefs.h
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
 * All of the resolve data about a single working directory.
 * This is the basic data structure used by the resolve system.
 * All other resolve data is stored within it somewhere.
 */
// TODO 2012/04/10 I promoted this to sg_wc7txapi__public_typedefs.h
// TODO            for a test.
// typedef struct _sg_resolve SG_resolve;

/**
 * Data about a single item with choices on it.
 * The item is resolved if all of its choices have been resolved.
 */
typedef struct _sg_resolve__item SG_resolve__item;

/**
 * Type of callback used when iterating items in a resolve.
 */
typedef void SG_resolve__item__callback(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current item.
	void*             pUserData, //< [in] User data supplied to the calling function.
	SG_bool*          pContinue  //< [out] If set to SG_FALSE, iteration will be stopped.
	);

/**
 * A list of individual item states that can be in conflict.
 * These are listed in the order that we prefer to resolve them.
 *
 * Note: If these are re-ordered, there are several constant array globals
 *       in sg_resolve.c which use them as indices that will need to be
 *       re-ordered to match.
 */
typedef enum _sg_resolve__state
{
	SG_RESOLVE__STATE__EXISTENCE,   //< Whether or not the item exists.
	SG_RESOLVE__STATE__NAME,        //< An item's filename relative to its parent.
	SG_RESOLVE__STATE__LOCATION,    //< An item's parent directory's GID.
	SG_RESOLVE__STATE__ATTRIBUTES,  //< The item's chmod attribute bits.
	SG_RESOLVE__STATE__CONTENTS,    //< The item's contents.
	SG_RESOLVE__STATE__COUNT,       //< Number of items in the enumeration, for iteration.
}
SG_resolve__state;

/**
 * A single choice that must be made on an item.
 * A choice is uniquely identified by the item it's on and the STATE it regards.
 */
typedef struct _sg_resolve__choice SG_resolve__choice;

/**
 * Type of callback used when iterating choices on an item.
 */
typedef void SG_resolve__choice__callback(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The current choice.
	void*               pUserData, //< [in] User data supplied to the calling function.
	SG_bool*            pContinue  //< [out] If set to SG_FALSE, iteration will be stopped.
);

/**
 * A single state value associated with a choice.
 * Each choice contains a number of state values
 * Each state value contains a single potential value for that state of the choice's item.
 * State values are referenced by a label which is unique among values on the choice.
 * Each state value is one of the following:
 * 1) Associated with a changeset (the merge's ancestor or one of its parents)
 *    in which case it contains the value that the item had in that changeset.
 * 2) Associated with the working directory (a special case of (1))
 *    in which case it contains the value that the item has in the working directory.
 * 3) A value that used to be associated with the working directory, but was saved
 *    at a particular point in time.  It contains a value that was in the working
 *    directory at one time during the merge/resolve process.  This can currently
 *    occur for the following reasons:
 *    a) A working value was used as the parent of a merge.  For the merge to continue
 *       to make sense historically, its parent values must not change and are therefore
 *       saved if they are a live working value.
 *    b) A working value containing edits was overwritten by accepting a different
 *       value.  The working value is saved in this case to preserve the edits that
 *       were made manually in the working directory.
 * 3) An intermediate value generated by merging/combining two other values
 *    in which case it contains a value that is a combination of two other values.
 */
typedef struct _sg_resolve__value SG_resolve__value;

/*
 * The labels used for various well-known merge nodes.
 */
extern const char* SG_RESOLVE__LABEL__ANCESTOR;  // full name
extern const char* SG_RESOLVE__LABEL__BASELINE;  // full name
extern const char* SG_RESOLVE__LABEL__OTHER;     // base name, usable with an index (indices only possible in not-fully-implemented N-way cases)
extern const char* SG_RESOLVE__LABEL__WORKING;   // base name, usable with an index
extern const char* SG_RESOLVE__LABEL__AUTOMERGE; // base name, usable with an index (indices only possible in not-fully-implemented N-way cases)
extern const char* SG_RESOLVE__LABEL__MERGE;     // base name, usable with an index

/**
 * The HID used to identify values that are associated with the working directory.
 */
extern const char* SG_RESOLVE__HID__WORKING;

/**
 * Flags used to describe a value.
 */
typedef enum _sg_resolve__value__flags
{
	SG_RESOLVE__VALUE__CHANGESET    = 1 << 0, //< The value is associated with a changeset.
	                                          //< Exclusive with MERGE.
	SG_RESOLVE__VALUE__MERGE        = 1 << 1, //< The value is the result of merging two other values.
	                                          //< Exclusive with CHANGESET.
	SG_RESOLVE__VALUE__AUTOMATIC    = 1 << 2, //< The value was generated automatically by merge (as opposed to manually by resolve).
	                                          //< Always set with CHANGESET, sometimes with MERGE.
	SG_RESOLVE__VALUE__RESULT       = 1 << 3, //< The value is the current resolution of its choice.
	SG_RESOLVE__VALUE__OVERWRITABLE = 1 << 4, //< The value can be overwritten with new data.
	                                          //< Set if AUTOMATIC and RESULT are unset.
	SG_RESOLVE__VALUE__LEAF         = 1 << 5, //< The value is a merge leaf.
	                                          //< A leaf value is one that still needs to be merged with other leaf values
	                                          //< to arrive at a final merged result.  When a merge leaves only a single leaf,
	                                          //< it can be automatically accepted as the choice's resolution.
	SG_RESOLVE__VALUE__LIVE         = 1 << 6, //< The value is associated with the live working copy.
	                                          //< This is NOT set for saved working values (this flag is how you tell the difference).
}
SG_resolve__value__flags;

/**
 * Type of callback used when iterating values on a choice.
 */
typedef void SG_resolve__value__callback(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The current value.
	void*              pUserData, //< [in] User data supplied to the calling function.
	SG_bool*           pContinue  //< [out] If set to SG_FALSE, iteration will be stopped.
);

END_EXTERN_C;

#endif
