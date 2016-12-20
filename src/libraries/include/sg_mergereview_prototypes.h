/*
Copyright 2012-2013 SourceGear, LLC

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
 * @file sg_mergereview_prototypes.h
 *
 * @details
 *
 */

////////////////////////////////////////////////////////////////////////////////

#ifndef H_SG_MERGEREVIEW_PROTOTYPES_H
#define H_SG_MERGEREVIEW_PROTOTYPES_H

BEGIN_EXTERN_C;

////////////////////////////////////////////////////////////////////////////////


/**
 * Gets a view of VC history based on the "merged-in" order of changesets.
 * See comments at the top of sg_mergereview.c for a discussion of the topic.
 */
void SG_mergereview(

	// Output parameter for errors.
	SG_context * pCtx,

	// Input. Repo to perform merge review in.
	SG_repo * pRepo,

	// Input. VC head to walk back from.
	const char * szHidHead,

	// Input flag. When true the algorithm only walks back until it gets to
	// the "baseline" VC parent of the starting "head" changeset. Ie, the first
	// changeset other than the head that isn't indented.
	SG_bool singleMergeReview,

	// Input. Optional. A vhash where the keys are VC merge nodes (revno,
	// converted to string) and the value for each key is the parent VC node to
	// use as the "baseline" or "old" side of the merge (thus considering the
	// other(s) to be "new") (this value is also a revno, but kept as an int).
	SG_vhash * pMergeBaselines,

	// Input. Optional (use 0 or SG_UINT32_MAX to disable).
	SG_uint32 resultLimit,

	// Output. List of results. Each result is a vhash with the following
	// members (note that you can call SG_history__fill_in_details() on this
	// list for further information):
	// {
	//      "revno": (int),
	//      "changeset_id": (string),
	//      "parents": (changeset_id-to-revno map (vhash) of VC parents. This is
	//          included to match results returned by SG_history, since
	//           SG_history__fill_in_details() doesn't add it),
	//      "displayChildren": (varray of revno ints),
	//      "continuationToken": (varray. This member is only included for
	//          indented nodes with multiple display children and is useful for
	//          when "rotating" the node),
	//      "indent": (int>=0 representing indentation level),
	//      "displayParent": (revno int. ***Omitted for non-indented nodes.***),
	//      "indexInParent": (int. Also omitted for non-indented nodes.)
	// }
	SG_varray ** ppResults,

	// Output. Optional. Total number of results in the list. This is just a
	// convenience so that you don't have to call SG_varray__count() separately.
	SG_uint32 * pCountResults,

	// Output. Optional. Set to NULL if the root of the VC dag was reached,
	// otherwise set to a value that can be used to get the next chunk by calling
	// SG_mergereview__continue().
	SG_varray ** ppContinuationToken

	);

/**
 * Same as SG_mergereview, but starts from a "continuation token" rather than
 * from a single changeset.
 */
void SG_mergereview__continue(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_varray * pContinuationToken,
	SG_bool singleMergeReview,
	SG_vhash * pMergeBaselines,
	SG_uint32 resultLimit,
	SG_varray ** ppResults,
	SG_uint32 * pCountResults,
	SG_varray ** ppContinuationToken
	);


////////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif //H_SG_MERGEREVIEW_PROTOTYPES_H
