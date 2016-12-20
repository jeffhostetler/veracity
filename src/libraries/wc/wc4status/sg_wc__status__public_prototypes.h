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

#ifndef H_SG_WC__STATUS__PUBLIC_PROTOTYPES_H
#define H_SG_WC__STATUS__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_wc__status__tne_type_to_flags(SG_context * pCtx,
									  SG_treenode_entry_type tneType,
									  SG_wc_status_flags * pStatusFlags);

void SG_wc__status__flags_to_properties(SG_context * pCtx,
										SG_wc_status_flags statusFlags,
										SG_vhash ** ppvhProperties);

//////////////////////////////////////////////////////////////////

void SG_wc__status__sort_by_repopath(SG_context * pCtx,
									 SG_varray * pvaStatus);

//////////////////////////////////////////////////////////////////

void SG_wc__status__classic_format(SG_context * pCtx,
								   const SG_varray * pvaStatus,
								   SG_bool bVerbose,
								   SG_string ** ppStringResult);

void SG_wc__status__classic_format2(SG_context * pCtx,
									const SG_varray * pvaStatus,
									SG_bool bVerbose,
									const char * pszLinePrefix,
									const char * pszLineEOL,
									SG_string ** ppStringResult);

void SG_wc__status__classic_format2__item(SG_context * pCtx,
										  const SG_vhash * pvhItem,
										  const char * pszLinePrefix,
										  const char * pszLineEOL,
										  SG_string ** ppStringResult);

//////////////////////////////////////////////////////////////////

void SG_wc__status__summary(SG_context * pCtx,
							const SG_varray * pvaStatus,
							SG_wc_status_flags * pStatusFlagsSummary);

//////////////////////////////////////////////////////////////////

void sg_wc__status__find_status__single_item(SG_context * pCtx,
									  const SG_varray * pva_statuses,
									  const char * psz_repo_path,
									  SG_vhash ** ppvh_status_result);

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_wc_debug__status__dump_flags(SG_context * pCtx,
									 SG_wc_status_flags statusFlags,
									 const char * pszLabel,
									 SG_string ** ppString);

void SG_wc_debug__status__dump_flags_to_console(SG_context * pCtx,
												SG_wc_status_flags statusFlags,
												const char * pszLabel);
#endif

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC__STATUS__PUBLIC_PROTOTYPES_H
