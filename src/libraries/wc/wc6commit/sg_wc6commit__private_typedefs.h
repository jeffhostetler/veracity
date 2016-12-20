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

#ifndef H_SG_WC6COMMIT__PRIVATE_TYPEDEFS_H
#define H_SG_WC6COMMIT__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct sg_commit_data
{
	const SG_wc_commit_args *	pCommitArgs;
	SG_wc_tx *					pWcTx;

	SG_int64			i64Date;
	SG_audit			auditRecord;
	char *				pszTiedBranchName;
	SG_varray *			pvaParents;
	SG_uint32			nrParents;

	SG_varray *			pvaStatusForPrompt;
	
	char *				pszTrimmedMessage;

	SG_varray *			pvaDescs;

	SG_bool				bIsPartial;

	SG_changeset *		pChangeset_VC;
	SG_dagnode *		pDagnode_VC;
	char *				pszHidChangeset_VC;
};

typedef struct sg_commit_data sg_commit_data;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC6COMMIT__PRIVATE_TYPEDEFS_H
