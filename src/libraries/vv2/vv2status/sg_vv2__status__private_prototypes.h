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

#ifndef H_SG_VV2__STATUS__PRIVATE_PROTOTYPES_H
#define H_SG_VV2__STATUS__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_vv2__status__free(SG_context * pCtx, sg_vv2status * pST);

#define SG_VV2__STATUS__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_vv2__status__free)

void sg_vv2__status__alloc(SG_context * pCtx,
						   SG_repo * pRepo,
						   SG_rbtree * prbTreenodesCache,
						   const char chDomain_0,
						   const char chDomain_1,
						   const char * pszLabel_0,
						   const char * pszLabel_1,
						   const char * pszWasLabel_0,
						   const char * pszWasLabel_1,
						   sg_vv2status ** ppST);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__compare_cset_vs_cset(SG_context * pCtx,
										  sg_vv2status * pST,
										  const char * pszHidCSet_0,
										  const char * pszHidCSet_1);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__compute_flags(SG_context * pCtx,
								   sg_vv2status_od * pOD);

void sg_vv2__status__compute_flags2(SG_context * pCtx,
									const SG_treenode_entry * pTNE_orig,
									const char * pszParentGid_orig,
									const SG_treenode_entry * pTNE_dest,
									const char * pszParentGid_dest,
									SG_wc_status_flags * pStatusFlags);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__compute_repo_path(SG_context * pCtx,
									   sg_vv2status * pST,
									   sg_vv2status_od * pOD);

void sg_vv2__status__compute_ref_repo_path(SG_context * pCtx,
										   sg_vv2status * pST,
										   sg_vv2status_od * pOD);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__od__free(SG_context * pCtx, sg_vv2status_od * pOD);

#define SG_VV2__STATUS__OD__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_vv2__status__od__free)

void sg_vv2__status__od__alloc(SG_context * pCtx, const char * szGidObject, sg_vv2status_od ** ppNewOD);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__odi__free(SG_context * pCtx, sg_vv2status_odi * pInst);

#define SG_VV2__STATUS__ODI__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_vv2__status__odi__free)

void sg_vv2__status__odi__alloc(SG_context * pCtx, sg_vv2status_odi ** ppInst);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__summarize(SG_context * pCtx,
							   sg_vv2status * pST,
							   SG_varray ** ppvaStatus);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__add_to_work_queue(SG_context * pCtx, sg_vv2status * pST, sg_vv2status_od * pOD);

void sg_vv2__status__can_short_circuit_from_work_queue(SG_UNUSED_PARAM( SG_context * pCtx ),
													   const sg_vv2status_od * pOD,
													   SG_bool * pbCanShortCircuit);

void sg_vv2__status__remove_from_work_queue(SG_context * pCtx,
											sg_vv2status * pST,
											sg_vv2status_od * pOD,
											SG_uint32 depthInQueue);

void sg_vv2__status__assert_in_work_queue(SG_context * pCtx,
										  sg_vv2status * pST,
										  sg_vv2status_od * pOD,
										  SG_uint32 depthInQueue);

void sg_vv2__status__remove_first_from_work_queue(SG_context * pCtx,
												  sg_vv2status * pST,
												  SG_bool * pbFound,
												  sg_vv2status_od ** ppOD);

//////////////////////////////////////////////////////////////////

void sg_vv2__status__alloc_item(SG_context * pCtx,
								SG_repo * pRepo,
								const char * pszLabel_0,
								const char * pszLabel_1,
								const char * pszWasLabel_0,
								const char * pszWasLabel_1,
								SG_wc_status_flags statusFlags,
								const char * pszGid,
								const SG_string * pStringRefRepoPath,
								const SG_treenode_entry * ptne_0,
								const char * pszParentGid_0,
								const SG_string * pStringCurRepoPath,
								const SG_treenode_entry * ptne_1,
								const char * pszParentGid_1,
								SG_vhash ** ppvhItem);

void sg_vv2__status__main(SG_context * pCtx,
						  SG_repo * pRepo,
						  SG_rbtree * prbTreenodeCache,
						  const char * pszHidCSet_0,
						  const char * pszHidCSet_1,
						  const char chDomain_0,
						  const char chDomain_1,
						  const char * pszLabel_0,
						  const char * pszLabel_1,
						  const char * pszWasLabel_0,
						  const char * pszWasLabel_1,
						  SG_bool bNoSort,
						  SG_varray ** pvaStatus,
						  SG_vhash ** ppvhLegend);

void sg_vv2__filtered_status(SG_context * pCtx,
							 SG_repo * pRepo,
							 SG_rbtree * prbTreenodeCache_Shared,	// optional
							 const char * pszHidCSet_0,
							 const char * pszHidCSet_1,
							 SG_bool b_allow_wd_inputs,
							 const SG_stringarray * psaInputs,
							 SG_uint32 depth,
							 const char chDomain_0,
							 const char chDomain_1,
							 const char * pszLabel_0,
							 const char * pszLabel_1,
							 const char * pszWasLabel_0,
							 const char * pszWasLabel_1,
							 SG_bool bNoSort,
							 SG_varray ** ppvaStatusFiltered,
							 SG_vhash ** ppvhLegend);

void sg_vv2__status__make_legend(SG_context * pCtx,
								 SG_vhash ** ppvhLegend,
								 const char * pszHid_0,   const char * pszHid_1,
								 const char * pszLabel_0, const char * pszLabel_1);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV2__STATUS__PRIVATE_PROTOTYPES_H
