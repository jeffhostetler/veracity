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

#ifndef H_SG_SVN_LOAD_PROTOTYPES_H
#define H_SG_SVN_LOAD_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Imports data from an SVN dump into a Veracity repo.
 */
void SG_svn_load__import(
	SG_context*    pCtx,     //< [in] [out] Error and context info.
	SG_repo*       pRepo,    //< [in] The repo to import into.
	SG_readstream* pStream,  //< [in] Stream to read the SVN dump from.
	const char*    szParent, //< [in] HID of the changeset to be the parent of the first imported one.
	const char*    szBranch, //< [in] Veracity branch to make imported changesets part of.
	                         //<      If NULL, imported changesets will be anonymous (not part of a branch).
	const char*    szPath,   //< [in] SVN path to import files/nodes from.
	                         //<      If NULL, all files/nodes are imported.
	const char*    szStamp,  //< [in] Stamp to apply to all imported changesets.
	                         //<      NULL to not apply any stamp.
	SG_bool        bComment  //< [in] If true apply an extra comment to all imported changesets.
	                         //<      The comment contains the SVN revision the changeset was imported from.
	);

END_EXTERN_C;

#endif
