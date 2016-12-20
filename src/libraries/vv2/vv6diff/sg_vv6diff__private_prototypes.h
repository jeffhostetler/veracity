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

#ifndef H_SG_VV6DIFF__PRIVATE_PROTOTYPES_H
#define H_SG_VV6DIFF__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_vv2__diff__diff_to_stream(SG_context * pCtx,
								  const char * pszRepoName,
								  const SG_varray * pvaStatus,
								  SG_bool bInteractive,
								  const char * pszTool,
								  SG_vhash ** ppvhResultCodes);

void sg_vv2__diff__directory(SG_context * pCtx,
							 sg_vv6diff__diff_to_stream_data * pData,
							 const SG_vhash * pvhItem,
							 SG_wc_status_flags statusFlags);

void sg_vv2__diff__file(SG_context * pCtx,
						sg_vv6diff__diff_to_stream_data * pData,
						const SG_vhash * pvhItem,
						SG_wc_status_flags statusFlags);

void sg_vv2__diff__header(SG_context * pCtx,
						  const SG_vhash * pvhItem,
						  SG_string ** ppStringHeader);

void sg_vv2__diff__symlink(SG_context * pCtx,
						   sg_vv6diff__diff_to_stream_data * pData,
						   const SG_vhash * pvhItem,
						   SG_wc_status_flags statusFlags);

void sg_vv2__diff__create_session_temp_dir(SG_context * pCtx,
										   sg_vv6diff__diff_to_stream_data * pData);

void sg_vv2__diff__export_to_temp_file(SG_context * pCtx,
									   sg_vv6diff__diff_to_stream_data * pData,
									   const char * szVersion,	// an index (like 0 or 1, _older_ _newer_) or maybe cset HID
									   const char * szGidObject,
									   const char * szHidBlob,
									   const char * pszNameForSuffix,
									   SG_pathname ** ppPathTempFile);

void sg_vv2__diff__create_temp_file(SG_context * pCtx,
									sg_vv6diff__diff_to_stream_data * pData,
									const char * szVersion,	// an index (like 0 or 1, _older_ _newer_) or maybe cset HID
									const char * szGidObject,
									const char * pszNameForSuffix,
									SG_pathname ** ppPathTempFile);

void sg_vv2__diff__print_header_on_console(SG_context * pCtx,
										   sg_vv6diff__diff_to_stream_data * pData,
										   const SG_string * pStringHeader);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV6DIFF__PRIVATE_PROTOTYPES_H
