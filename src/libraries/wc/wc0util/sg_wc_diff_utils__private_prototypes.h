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

#ifndef H_SG_WC_DIFF_UTILS_PROTOTYPES_H
#define H_SG_WC_DIFF_UTILS_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_diff_utils__make_label(SG_context * pCtx,
								  const char * szRepoPath,
								  const char * szHid,
								  const char * szDate,
								  SG_string ** ppStringLabel);

void sg_wc_diff_utils__export_to_temp_file(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const char * szVersion,
										   const char * szGidObject,
										   const char * szHidBlob,
										   const char * pszNameForSuffix,
										   SG_pathname ** ppPathTempFile);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_DIFF_UTILS_PROTOTYPES_H
