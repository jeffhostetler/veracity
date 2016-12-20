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

#ifndef H_SG_RESOLVE_VALUE_PROTOTYPES_H
#define H_SG_RESOLVE_VALUE_PROTOTYPES_H

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
 * Gets the choice that contains a value.
 */
void SG_resolve__value__get_choice(
	SG_context*          pCtx,    //< [in] [out] Error and context info.
	SG_resolve__value*   pValue,  //< [in] The value to get the containing choice for.
	SG_resolve__choice** ppChoice //< [out] The choice that contains the value.
	);

/**
 * Gets the label that identifies a value.
 */
void SG_resolve__value__get_label(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to get data about.
	const char**       ppLabel //< [out] The value's label.
	);

/**
 * Gets a value structure's actual data value.
 * Use SG_resolve__value__get_summary if you're looking for something more suitable for display.
 *
 * Throws SG_ERR_USAGE if the data value isn't a bool.
 * Values for the following choice types have boolean data:
 * - EXISTENCE: Whether or not the item exists.
 */
void SG_resolve__value__get_data__bool(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to get the data of.
	SG_bool*           pData   //< [out] The value structure's data as a bool.
	);

/**
 * Gets a value structure's actual data value.
 * Use SG_resolve__value__get_summary if you're looking for something more suitable for display.
 *
 * Throws SG_ERR_USAGE if the data value isn't an integer.
 * Values for the following choice types have integral data:
 * - ATTRIBUTES: The item's file system attributes.
 */
void SG_resolve__value__get_data__int64(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to get the data of.
	SG_int64*          pData   //< [out] The value structure's data as an integer.
	);

/**
 * Gets a value structure's actual data value.
 * Use SG_resolve__value__get_summary if you're looking for something more suitable for display.
 *
 * Throws SG_ERR_USAGE if the data value isn't a string.
 * Values for the following choice types have textual data:
 * - NAME:        The item's name.
 * - LOCATION:    The GID of the item's parent directory.
 * - CONTENTS:    The HID of the blob containing the item's contents.
 */
void SG_resolve__value__get_data__sz(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to get the data of.
	const char**       ppData  //< [out] The value structure's data as a string.
	);

/**
 * Gets a summary of the value that is suitable for display.
 * In many cases, this is the actual value in a human-readable form.
 * In other cases, this is a shorter and/or more readable version of the actual value.
 * Some values have no good way to summarize, and so return NULL.
 */
void SG_resolve__value__get_summary(
	SG_context*        pCtx,     //< [in] [out] Error and context info.
	SG_resolve__value* pValue,   //< [in] The value to get a summary of.
	SG_string**        ppSummary //< [out] A summary of the value suitable for display.
	                             //<       NULL if the value cannot be well summarized.
	);

/**
 * Gets the size of the value's data.
 *
 * This has different meanings depending on the type of choice:
 * - EXISTENCE:          sizeof(SG_bool)
 * - NAME:               Length of the item's name.
 * - LOCATION:           Length of the item's parent's repo path.
 * - ATTRIBUTES:         sizeof(SG_int64)
 * - CONTENTS/FILE:      Size of the file's contents.
 * - CONTENTS/SYMLINK:   Length of the symlink's target path.
 * - CONTENTS/SUBMODULE: Length of the HID of the blob containing the submodule's data.
 */
void SG_resolve__value__get_size(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to get the size of.
	SG_uint64*         pSize   //< [out] The size of the value.
	);

/**
 * Gets a set of flags that describe the value.
 */
void SG_resolve__value__get_flags(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to get flags for.
	SG_uint32*         pFlags  //< [out] The flags for the value.
	);

/**
 * Checks whether or not a value has a given flag.
 * If multiple flags are passed, it checks if the value has ALL of them.
 */
void SG_resolve__value__has_flag(
	SG_context*        pCtx,    //< [in] [out] Error and context info.
	SG_resolve__value* pValue,  //< [in] The value to check for a flag on.
	SG_uint32          uFlag,   //< [in] The flag(s) to check for.
	SG_bool*           pHasFlag //< [out] True if the value has the given flag(s), false otherwise.
	);

/**
 * Gets data about the value's association with a changeset.
 */
void SG_resolve__value__get_changeset__mnemonic(
	SG_context*         pCtx,          //< [in] [out] Error and context info.
	SG_resolve__value*  pValue,        //< [in] The value to get info about.
	SG_bool*            pHasChangeset, //< [out] Whether or not the value is associated with a changeset.
	                                   //<       This output is required.
	const char**        ppszMnemonic   //< [out] The MNEMONIC of the changeset that the value is associated with.
	                                   //<       This output is optional, and meaningless/ignored if pHasChangeset is false.
	);

/**
 * Gets additional data specific to working values from mergeable choices.
 */
void SG_resolve__value__get_mergeable_working(
	SG_context*         pCtx,                //< [in] [out] Error and context info.
	SG_resolve__value*  pValue,              //< [in] The value to get info about.
	SG_bool*            pIsMergeableWorking, //< [out] Whether or not the value is a working value from a mergeable choice.
	                                         //<       This output is required.
	SG_resolve__value** ppParent,            //< [out] The value that this working value is based on.
	                                         //<       This output is optional, and meaningless/ignored if pIsWorking is false.
	SG_bool*            pModified            //< [out] Whether or not the value's data is different from its parent's.
	                                         //<       This output is optional, and meaningless/ignored if pIsWorking is false.
	);

/**
 * Gets data about the value's creation via merge.
 */
void SG_resolve__value__get_merge(
	SG_context*         pCtx,        //< [in] [out] Error and context info.
	SG_resolve__value*  pValue,      //< [in] The value to get info about.
	SG_bool*            pWasMerged,  //< [out] Whether or not the value was created via merging.
	                                 //<       This output is required.
	SG_resolve__value** ppBaseline,  //< [out] The value's "baseline" parent.
	                                 //<       This output is optional, and meaningless/ignored if pWasMerged is false.
	SG_resolve__value** ppOther,     //< [out] The value's "other" parent.
	                                 //<       This output is optional, and meaningless/ignored if pWasMerged is false.
	SG_bool*            pAutomatic,  //< [out] Whether the merge was automatic (done by the merge system)
	                                 //<       or manual (done by the user with the resolve system).
	                                 //<       This output is optional, and meaningless/ignored if pWasMerged is false.
	const char**        ppTool,      //< [out] The name of the merge tool used to create the value.
	                                 //<       This output is optional, and meaningless/ignored if pWasMerged is false.
	SG_int32*           pToolResult, //< [out] The result of running ppTool to create the value.
	                                 //<       One of the SG_MERGETOOL__STATUS__* values.
	                                 //<       This output is optional, and meaningless/ignored if pWasMerged is false.
	SG_int64*           pMergeTime   //< [out] The date/time that the merge value was created.
	);

/**
 * An SG_resolve__value__callback that counts how many times it is called.
 * This can effectively count the number of values in a choice.
 */
void SG_resolve__value_callback__count(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The current value.
	void*              pUserData, //< [in] An SG_uint32* to increment.
	SG_bool*           pContinue  //< [out] Always set to true.
	);

END_EXTERN_C;

#endif
