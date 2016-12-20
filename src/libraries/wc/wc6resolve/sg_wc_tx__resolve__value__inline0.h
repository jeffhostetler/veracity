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

#ifndef H_SG_WC_TX__RESOLVE__VALUE__INLINE0_H
#define H_SG_WC_TX__RESOLVE__VALUE__INLINE0_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * A single state value associated with a choice.
 * Values are generally created from the changesets involved in the merge.
 * Values are also created from the WD state value if necessary.
 * For states whose values are mergeable, values can also be created by merging two other values.
 * Values that are the result of the merge may be invalid (have no value), if the merge failed for some reason.
 */
struct _sg_resolve__value
{
	// identification
	SG_resolve__choice* pChoice;          //< The choice that owns this value.
	char*               szLabel;          //< The label/name for this value.

	// value
	// If all of these are NULL, see the "changeset" and "merge" sections for what it means in each case.
	// See the comment below this struct for detailed information about these values in various cases.
	SG_variant*         pVariant;         //< The actual data value.
	                                      //< Type of the variant depends on the state of the choice that owns the value.
	                                      //< For mergeable values, this is the HID of a blob.
	                                      //< NULL for values that don't have a variant/value (failed merges).

	SG_string*          pVariantFile;     //< repo-path to the file.  This might be used to describe
	                                      //< the inputs to a 'diff' or 'merge' sub-command (in which case they
	                                      //< will refer to a TEMP file).  It can also refer to the working
	                                      //< value (when they say 'accept working', for example), and refer
	                                      //< to something in the working directory (that should have existed
	                                      //< prior to the beginning of the TX).  When we allow RESOLVE to
	                                      //< do handle multiple steps in one TX, it is also possible that this
	                                      //< could refer to a file that has a QUEUEd OVERWRITE from a previous
	                                      //< step.
	                                      //<
	                                      //< Since the WC layer is free to *LIE* about
	                                      //< the location/name/contents of a file (because
	                                      //< of already QUEUED operations that have not yet
	                                      //< been APPLIED) we can set this to a GID-domain
	                                      //< repo-path so we don't have to worry about in-tx
	                                      //< vs pre-tx values for 'working' values.
	                                      //<
	                                      //<
	                                      //< ***Always use _wc__variantFile_to_absolute()
	                                      //< ***if you need to get a pathname from this.
	                                      //< 
	                                      //< This field is only used for mergeable values.
	                                      //< NULL for non-mergeable values.


	char*               szVariantSummary; //< More human-readable summary of the data stored in pVariant.
	                                      //< Used for values where the actual data is not easily comprehensible or available in a file.
	                                      //< NULL for values that don't require it.
	SG_uint64           uVariantSize;     //< Size in bytes of the data value.
	                                      //< For mergeable values, this is the size of the file indicated by pVariantFile.
	                                      //< For non-mergeable values, this is generally the size of the value in pVariant.

	// miscellaneous
	SG_uint32           uChildren;        //< Number of other values that list this one as a parent.

	char * pszMnemonic;

	// merge
	// These are only used for values created by merging other values.
	// (EXCEPTION: pBaselineParent is used for working values of a mergeable choice.)
	// If a merge value has all NULL variant data, then it's the result of a failed merge.
	// Use iMergeToolResult to get more detail about the nature of the failure.
	SG_resolve__value*  pBaselineParent;  //< The baseline parent value that was merged into this one.
	                                      //< Points at one of the other values in the same choice.
	                                      //< For working values of a mergeable choice, this indicates the value's base/parent value,
	                                      //< which is the one that was copied into the working copy to generate the working value.
	SG_resolve__value*  pOtherParent;     //< The other parent value that was merged into this one.
	                                      //< Points at one of the other values in the same choice.
	SG_bool             bAutoMerge;       //< If SG_TRUE, this value was generated automatically by merge.
	                                      //< If SG_FALSE, it was generated manually by the user with resolve.
	char*               szMergeTool;      //< The name of the merge tool used to create this node from its parents.
	SG_int32            iMergeToolResult; //< The status result when szMergeTool was run.
	                                      //< One of SG_MERGETOOL__STATUS__*
	SG_int64            iMergeTime;       //< The date/time that the merge value was created.
	                                      //< Corresponds to the timestamp of the value's temporary file on disk.
};
/*
 * More about value data:
 *
 * EXISTENCE
 * - pVariant:         (bool) True if the item exists, false if it doesn't.
 * - pVariantFile:     NULL
 * - szVariantSummary: NULL
 * - uVariantSize:     sizeof(SG_bool)
 * NAME
 * - pVariant:         (string) The name of the item.
 * - pVariantFile:     NULL
 * - szVariantSummary: NULL
 * - uVariantSize:     The length of the item's name.
 * LOCATION
 * - pVariant:         (string) The GID of the item's parent folder.
 * - pVariantFile:     NULL
 * - szVariantSummary: The parent's repo path.
 * - uVariantSize:     The length of the parent's repo path.
 * ATTRIBUTES
 * - pVariant:         (int) The item's attribute bits.
 * - pVariantFile:     NULL
 * - szVariantSummary: NULL
 * - uVariantSize:     sizeof(SG_int64)
 * CONTENTS/FILE
 * - pVariant:         (string) The HID of the blob containing the item's contents.
 * - pVariantFile:      ***SEE NOTE [2]***  repo-path to a file containing the item's/blob's contents.
 * - szVariantSummary: NULL
 * - uVariantSize:     The size of the file indicated by pVariantFile.
 * CONTENTS/DIRECTORY (not possible)
 * CONTENTS/SYMLINK
 * - pVariant:         (string) The target of the symlink item.  (***NOT THE HID***)
 * - pVariantFile:     NULL
 * - szVariantSummary: NULL
 * - uVariantSize:     The length of the symlink's target path.
 * CONTENTS/SUBMODULE
 * - pVariant:         (string) The HID of the blob containing the submodule's data.
 * - pVariantFile:     NULL
 * - szVariantSummary: A summary string retrieved from the submodule.
 * - uVariantSize:     The length of the blob's HID.
 */

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__VALUE__INLINE0_H
