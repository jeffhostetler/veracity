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

#ifndef H_SG_WC_PORT__PUBLIC_PROTOTYPES_H
#define H_SG_WC_PORT__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_wc_port__free(SG_context * pCtx, SG_wc_port * pDir);

#define SG_WC_PORT_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_wc_port__free)

/**
 * Allocate a SG_wc_port structure to allow us to check for
 * collisions between a yet-to-be-given list of entrynames.
 * 
 * You must provide a MASK and CHARSET.
 *
 * Once you allocate this, call __add_item() to add the individual
 * items to the collection.  Wait until all items have been added
 * before inspecting any collision bits.
 *
 * *NOTE* You probably DO NOT want to call this directly.
 * *NOTE* See sg_wc_db__create_port().
 *
 * TODO 2012/01/20 Consider making this routine PRIVATE.
 * 
 */
void SG_wc_port__alloc__with_charset(SG_context * pCtx,
									 SG_wc_port_flags portMask,
									 const char * pszCharSetName,       // optional
									 SG_wc_port ** ppDir);

/**
 * Allocate a SG_wc_port with the default CHARSET for this
 * repo/wd, but use the given MASK.
 *
 * *NOTE* You probably DO NOT want to call this directly.
 * *NOTE* See sg_wc_db__create_port().
 *
 */
void SG_wc_port__alloc(SG_context * pCtx,
					   SG_repo * pRepo,
					   const SG_pathname * pPathWorkingDirectoryTop,
					   SG_wc_port_flags portMask,
					   SG_wc_port ** ppDir);

//////////////////////////////////////////////////////////////////

/**
 * Inspect the given file/directory entryname and see if it has any problematic
 * chars and add it to our container so that we can check for collisions.
 *
 * If we find a problem, we update the pItem->portWarnings.
 *
 * We try to be as verbose as possible and completely list warnings
 * rather than stopping after the first one.
 *
 * If this entryname is an exact-match with an existing entry already
 * in the collider, we DO NOT insert it and set *pbIsDuplicate.
 *
 * TODO 2011/08/16 See if we are still using pszGid_Entry for anything
 * TODO            and/or remove it.
 */
void SG_wc_port__add_item(SG_context * pCtx,
						  SG_wc_port * pDir,
						  const char * pszGid_Entry,		// optional, for logging.
						  const char * szEntryName,
						  SG_treenode_entry_type tneType,
						  SG_bool * pbIsDuplicate);

/**
 * Version of __add_item() where we store an optional assoc-data with
 * the entry.
 *
 * If this entryname is an exact-match with an existing entry already
 * in the collider, we DO NOT insert it and set *pbIsDuplicate and
 * optionally return the assoc-data for the one we matched.
 */
void SG_wc_port__add_item__with_assoc(SG_context * pCtx,
									  SG_wc_port * pDir,
									  const char * pszGid_Entry,	// optional, for logging.
									  const char * szEntryName,
									  SG_treenode_entry_type tneType,
									  void * pVoidAssocData,
									  SG_bool * pbIsDuplicate,
									  void ** ppVoidAssocData_Original);

//////////////////////////////////////////////////////////////////

/**
 * Fetch the flags and complete log of any problems observed
 * limited by the mask.
 */
void SG_wc_port__get_results(SG_context * pCtx,
							 SG_wc_port * pDir,
							 SG_wc_port_flags * pFlagsUnion,
							 const SG_string ** ppStringLog);

/**
 * Fetch the flags and log for the item with this original entryname.
 * This name corresponds to the entryname used in __add_item().
 *
 * WARNING: the complete set of collision flags cannot be computed until
 * after all the individual files/directories are added to the container.
 * Individual __INDIV__ bits are available immediately, but the __COLLISION__
 * bits are not.
 */
void SG_wc_port__get_item_result_flags(SG_context * pCtx,
									   SG_wc_port * pDir,
									   const char * szEntryName,
									   SG_wc_port_flags * pFlagsObserved,
									   const SG_string ** ppStringItemLog);

/**
 * the __with_assoc() version also returns the optional user-data given
 * if the __add_item__with_assoc() version was used.
 */
void SG_wc_port__get_item_result_flags__with_assoc(SG_context * pCtx,
												   SG_wc_port * pDir,
												   const char * szEntryName,
												   SG_wc_port_flags * pFlagsObserved,
												   const SG_string ** ppStringItemLog,
												   void ** ppVoidAssocData);

//////////////////////////////////////////////////////////////////

/**
 * Call the given callback for each flagged item (after removing
 * masked-off bits).
 */
void SG_wc_port__foreach_with_issues(SG_context * pCtx,
									 SG_wc_port * pDir,
									 SG_wc_port__foreach_callback * pfn,
									 void * pVoidCallerData);

//////////////////////////////////////////////////////////////////

/**
 * TODO 2011/08/03 This is a placeholder.  We need a way
 * TODO            to set config values at repo-scope that
 * TODO            can be pushed/pulled/versioned and/or to
 * TODO            load them from machine-scope or whatever.
 * TODO
 * TODO            Update this function (API and whatnot)
 * TODO            to get the portability-mask for this repo.
 * TODO            For example, a Linux-only should should be
 * TODO            able to turn off __FLAGS__COLLISION__CASE
 * TODO            and friends.
 * TODO
 * TODO            For now we assume all bits are turned on.
 *
 */
void SG_wc_port__get_mask_from_config(SG_context * pCtx,
									  SG_repo * pRepo,
									  const SG_pathname * pPathWorkingDirectoryTop,
									  SG_wc_port_flags * pMask);

/**
 * TODO 2011/08/03 This is a placeholder.  We need a way
 * TODO            to set a config value for the user's
 * TODO            LOCALE in repo-scope that can be pushed/
 * TODO            pulled/whatever.  This is only needed
 * TODO            if users want to have non-utf-8 filenames
 * TODO            on Linux -- and use high-bit characters
 * TODO            in the filesystem to represent ISO-8859-x
 * TODO            characters (for x > 1).
 * TODO
 * TODO            By default we assume that everyone is
 * TODO            using UTF-8.
 *
 * This returns the name of the character set that can
 * be passed to the character-converter-constructor.
 * It returns NULL if we should assume UTF-8.
 * 
 */
void SG_wc_port__get_charset_from_config(SG_context * pCtx,
										 SG_repo * pRepo,
										 const SG_pathname * pPathWorkingDirectoryTop,
										 char ** ppszCharSet);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_PORT__PUBLIC_PROTOTYPES_H
