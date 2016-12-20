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

/**
 *
 * @file sg_vv_utils_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_UTILS_PROTOTYPES_H
#define H_SG_VV_UTILS_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_vv_utils__folder_arg_to_path(SG_context * pCtx,
									 const char * pszFolder,	//< [in] whatever the user gave us, may be relative, may be blank/null
									 SG_pathname ** ppPath);	//< [out] proper pathname of the new candidate working directory


void SG_vv_utils__throw_NOTAREPOSITORY_for_wd_association(SG_context * pCtx,       //< Must be non-NULL.
                                                          const char * szRepoName, //< Must be non-NULL.
                                                          SG_pathname * pWdRoot);  //< Optional.

void SG_vv_utils__verify_dir_empty(SG_context * pCtx,
								   const SG_pathname * pPath);

//////////////////////////////////////////////////////////////////

/**
 * Convert the given (argc,argv) into a stringarray.
 * Omit the empty args.
 * 
 * Return null if there are no args.
 * 
 * Use this on the result of SG_getopt__parse_all_args()
 * to get a stringarray of the remaining (non-option) args.
 *
 */
void SG_util__convert_argv_into_stringarray(SG_context * pCtx,
											SG_uint32 argc, const char ** argv,
											SG_stringarray ** ppsa);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV_UTILS_PROTOTYPES_H
