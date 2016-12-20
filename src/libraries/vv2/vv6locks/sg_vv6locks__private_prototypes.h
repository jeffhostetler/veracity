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

#ifndef H_SG_VV6LOCKS__PRIVATE_PROTOTYPES_H
#define H_SG_VV6LOCKS__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_vv2__locks__main(
	SG_context * pCtx,
	const char * pszRepoName,
	const char * pszBranchName,
	const char * psz_server,
	const char * psz_username,
	const char * psz_password,
	SG_bool b_pull,
	SG_bool bIncludeCompletedLocks,		// aka Verbose
	char ** ppsz_tied_branch_name,
	SG_vhash** ppvh_locks_by_branch,
	SG_vhash** ppvh_violated_locks,
	SG_vhash** ppvh_duplicate_locks
	);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV6LOCKS__PRIVATE_PROTOTYPES_H
