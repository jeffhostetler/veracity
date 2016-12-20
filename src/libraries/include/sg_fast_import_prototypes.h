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

#ifndef H_SG_FAST_IMPORT_PROTOTYPES_H
#define H_SG_FAST_IMPORT_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Imports a repo from a file in the "git fast-import" format.
 */
void SG_fast_import__import(
	SG_context * pCtx,            //< [in] [out] Error and context info.
	const char * psz_fi,          //< [in] File to import from.
	const char * psz_repo,        //< [in] Name of the repo to create and import into.
	const char * psz_storage,     //< [in] Storage driver to use for the new repo.
	                              //<      If NULL, the default is used.
	const char * psz_hash,        //< [in] Hash algorithm to use for the new repo.
	                              //<      If NULL, the default is used.
	const char * psz_shared_users //< [in] The repo to share users with.
	                              //<      If NULL, the new repo doesn't share users with any other repo.
	);

END_EXTERN_C;

#endif
