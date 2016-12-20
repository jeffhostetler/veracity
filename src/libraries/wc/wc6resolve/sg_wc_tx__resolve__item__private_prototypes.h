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

#ifndef H_SG_RESOLVE_ITEM_PROTOTYPES_H
#define H_SG_RESOLVE_ITEM_PROTOTYPES_H

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
 * Gets the resolve structure that contains an item.
 */
void SG_resolve__item__get_resolve(
	SG_context*       pCtx,     //< [in] [out] Error and context info.
	SG_resolve__item* pItem,    //< [in] The item to get the containing resolve for.
	SG_resolve**      ppResolve //< [out] The resolve that contains the item.
	);

/**
 * Gets the GID of an item.
 */
void SG_resolve__item__get_gid(
	SG_context*       pCtx,  //< [in] [out] Error and context info.
	SG_resolve__item* pItem, //< [in] The item to get the GID of.
	const char**      ppGid  //< [out] The GID of the given item.
	);

/**
 * Gets the type of an item.
 */
void SG_resolve__item__get_type(
	SG_context*             pCtx,  //< [in] [out] Error and context info.
	SG_resolve__item*       pItem, //< [in] The item to get the type of.
	SG_treenode_entry_type* pType  //< [out] The type of the item.
	);

/**
 * Gets the repo path of an item in the working directory.
 */
void SG_resolve__item__get_repo_path__working(
	SG_context*       pCtx,          //< [in] [out] Error and context info.
	const SG_resolve__item* pItem,   //< [in] The item to get the repo path of.
	SG_string ** ppStringRepoPath,   //< [out] You own this.
	SG_bool * pbShowInParens         //< [out] Guidance for printing/display (set when deleted)
	);

/**
 * Gets the resolved status of an item.
 * See the definition of the __X__, __XR__, and __XU__ bits.
 *
 */
void SG_resolve__item__get_resolved(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The item to get the resolved status of.
	SG_wc_status_flags * pStatusFlags_x_xr_xu     //< [out]
	);

/**
 * Gets a choice from the item by state.
 */
void SG_resolve__item__check_choice__state(
	SG_context*          pCtx,    //< [in] [out] Error and context info.
	SG_resolve__item*    pItem,   //< [in] The item to get a choice from.
	SG_resolve__state    eState,  //< [in] The state of the choice to retrieve.
	SG_resolve__choice** ppChoice //< [out] The requested choice.
	                              //<       NULL if the item has no choice for that state.
	);

/**
 * Gets a choice from the item by state.
 * Throws SG_ERR_NOT_FOUND if there is no choice for that state on the item.
 */
void SG_resolve__item__get_choice__state(
	SG_context*          pCtx,    //< [in] [out] Error and context info.
	SG_resolve__item*    pItem,   //< [in] The item to get a choice from.
	SG_resolve__state    eState,  //< [in] The state of the choice to retrieve.
	SG_resolve__choice** ppChoice //< [out] The requested choice.
	);

/**
 * Iterates through choices on a single item involved in the resolve.
 * Choices are returned in order of preferred resolution.
 */
void SG_resolve__item__foreach_choice(
	SG_context*                   pCtx,            //< [in] [out] Error and context info.
	SG_resolve__item*             pItem,           //< [in] The item to iterate the choices on.
	SG_bool                       bOnlyUnresolved, //< [in] If SG_TRUE, only iterate unresolved choices.
	SG_resolve__choice__callback* fCallback,       //< [in] Callback to supply each choice to.
	void*                         pUserData        //< [in] User data to supply to the callback.
	);

/**
 * An SG_resolve__item__callback that counts how many times it is called.
 * This can effectively count the number of items in the resolve.
 */
void SG_resolve__item_callback__count(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current item.
	void*             pUserData, //< [in] An SG_uint32* to increment.
	SG_bool*          pContinue  //< [out] Always set to true.
	);

END_EXTERN_C;

#endif
