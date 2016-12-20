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
 * @file sg_vv_utils__folder_arg_to_path.h
 *
 * @details Routine to take whatever relative or absolute path (or null)
 * that the user gave us and create a full path.  This will default to
 * the CWD.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_UTILS__FOLDER_ARG_TO_PATH_H
#define H_SG_VV_UTILS__FOLDER_ARG_TO_PATH_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_vv_utils__folder_arg_to_path(SG_context * pCtx,
									 const char * pszFolder,	//< [in] whatever the user gave us, may be relative, may be blank/null
									 SG_pathname ** ppPath)		//< [out] proper pathname of the new candidate working directory
{
	SG_pathname * pPathAllocated = NULL;

	// pszFolder is optional
	SG_NULLARGCHECK_RETURN( ppPath );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathAllocated)  );
	if (pszFolder && *pszFolder)
		SG_ERR_CHECK(  SG_pathname__set__from_sz(pCtx, pPathAllocated, pszFolder)  );
	else
		SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathAllocated)  );

	*ppPath = pPathAllocated;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathAllocated);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV_UTILS__FOLDER_ARG_TO_PATH_H
