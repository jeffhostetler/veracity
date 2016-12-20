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
 * @file sg_usermap_prototypes.h
 *
 */

#ifndef H_SG_USERMAP_PROTOTYPES_H
#define H_SG_USERMAP_PROTOTYPES_H

BEGIN_EXTERN_C

//////////////////////////////////////////////////////////////////////////

/**
 * Create a new usermap corresponding the the repository identified by pszDescriptorName.
 * Throws if a usermap matching that name already exists.
 * The caller is responsible for handling races and consistency with descriptor creation.
 */
void SG_usermap__create(
	SG_context* pCtx, 
	const char* pszDescriptorName, 
	const SG_pathname* pPathStaging
	);

/**
 * Delete an entire usermap (as opposed to an individual user mapping).
 * Does nothing if no usermap with name pszOld exists.
 * The caller is responsible for handling races and consistency with descriptor deletes.
 */
void SG_usermap__delete(
	SG_context* pCtx, 
	const char* pszDescriptorName
	);

//////////////////////////////////////////////////////////////////////////

void SG_usermap__users__get_all(
	SG_context* pCtx,
	const char* pszDescriptorName,
	SG_vhash** ppvhList
	);

void SG_usermap__users__get_one(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const char* pszSrcRecId,
	char** ppszDestRecId
	);

void SG_usermap__users__add_or_update(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const char* pszSrcRecId,
	const char* pszDestRecId
	);

void SG_usermap__users__remove_one(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const char* pszSrcRecId
	);

void SG_usermap__users__remove_all(
	SG_context* pCtx,
	const char* pszDescriptorName
	);

void SG_usermap__get_fragball_path(
	SG_context* pCtx,
	const char* pszDescriptorName,
    SG_pathname** pp
    );

//////////////////////////////////////////////////////////////////////////


END_EXTERN_C

#endif
