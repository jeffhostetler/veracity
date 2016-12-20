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

#ifndef H_SG_WC_TX__RESOLVE__TO_VHASH__PRIVATE_PROTOTYPES_H
#define H_SG_WC_TX__RESOLVE__TO_VHASH__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_resolve__to_vhash(SG_context * pCtx,
						  const SG_resolve * pResolve,
						  SG_vhash ** ppvhResolve);

#if defined(DEBUG)
void SG_resolve_debug__dump_to_console(SG_context * pCtx,
									   const SG_resolve * pResolve)
#endif

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__TO_VHASH__PRIVATE_PROTOTYPES_H
