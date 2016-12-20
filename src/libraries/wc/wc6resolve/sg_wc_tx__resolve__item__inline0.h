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

#ifndef H_SG_WC_TX__RESOLVE__ITEM__INLINE0_H
#define H_SG_WC_TX__RESOLVE__ITEM__INLINE0_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////


/**
 * Data about a single item with choices on it.
 * The item is resolved if all of its choices have been resolved.
 */
struct _sg_resolve__item
{
	SG_resolve*            pResolve;                           //< The resolve that owns this item.
	char*                  szGid;                              //< The item's unique GID.
	SG_string *            pStringGidDomainRepoPath;           //< GID-domain extended repo-path for this item.
                                                               //< We use this to talk to WC by GID so we don't have to track actual repo-paths.

	sg_wc_liveview_item * pLVI;		// we do not own this

	SG_treenode_entry_type eType;                              //< The item's type.

	char*                  szMergedName;                       //< Name of the item as it was left by merge.
//	char*                  szMergedContentHid;                 //< HID of the item's content as it was left by merge.
//	SG_int64               iMergedAttributes;                  //< Attributes of the item as it was left by merge.

	char*                  szRestoreOriginalName;              //< If the item was assigned a temporary name by merge,
	                                                           //< this is the unmangled name (probably with a bias towards the baseline).
	                                                           //< This is the name we'll implicitly revert it back to once it's resolved.
	                                                           //< Will be NULL if the item had different names in different parent changesets.
	                                                           //< In that case, reverting back to an original name will be unnecessary,
	                                                           //< because the user will have to choose a name to resolve the NAME choice that it will have.

	char*                  szReferenceOriginalName;            //< If the item was assigned a temporary name by merge,
	                                                           //< this is the unmangled name (probably with a bias towards the baseline).

	SG_resolve__choice*    aChoices[SG_RESOLVE__STATE__COUNT]; //< The set of choices on the item, indexed by the state in conflict.
	                                                           //< Indices for states that are not in conflict are NULL.
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__ITEM__INLINE0_H
