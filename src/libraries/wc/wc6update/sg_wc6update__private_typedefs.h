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

#ifndef H_SG_WC6UPDATE__PRIVATE_TYPEDEFS_H
#define H_SG_WC6UPDATE__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct sg_update_data
{
	const SG_wc_update_args *	pUpdateArgs;

	SG_wc_tx *					pWcTx;
	SG_vhash *					pvhPile;
	SG_varray *					pvaStatusVV2;
	SG_varray *					pvaStatusWC;
	SG_uint32					nrUncontrolledChangesInWC;
	SG_uint32					nrControlledChangesInWC;

	// details about the state of the WD before we started.
	char *						pszHid_StartingBaseline;
	char *						pszBranchName_Starting;

	// details about how they requested/named the target changeset.
	char *						pszHidTarget;
	char *						pszBranchNameTarget;
	char * 						pszNormalizedAttach;	// what we normalized pUpdateArgs->pszAttach
	char * 						pszNormalizedAttachNew; // what we normalized pUpdateArgs->pszAttachNew
	SG_bool						bRequestedByBranch;
	SG_bool						bRequestedByRev;

	// details about what we finally chose.
	char *						pszHidChosen;
	SG_rev_spec *				pRevSpecChosen;
	char *						pszAttachChosen;
	SG_bool						bAttachNewChosen;
	SG_dagquery_relationship	dqRel;
	


};

typedef struct sg_update_data sg_update_data;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC6UPDATE__PRIVATE_TYPEDEFS_H
