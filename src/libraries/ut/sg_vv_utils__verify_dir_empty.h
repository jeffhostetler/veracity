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
 * @file sg_vv_utils__verify_dir_empty.h
 *
 * @details Routine to verify that the given directory path either
 * does not exist or is an empty directory.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_UTILS__VERIFY_DIR_EMPTY_H
#define H_SG_VV_UTILS__VERIFY_DIR_EMPTY_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

static SG_dir_foreach_callback _vv_utils__count_dir;

static void _vv_utils__count_dir(SG_context * pCtx,
								 const SG_string * pStringEntryName,
								 SG_fsobj_stat * pfsStat,
								 void * pVoidData)
{
	SG_uint32 * pSum = (SG_uint32 *)pVoidData;

	SG_UNUSED( pCtx );
	SG_UNUSED( pStringEntryName );
	SG_UNUSED( pfsStat );

	*pSum = *pSum + 1;
}

void SG_vv_utils__verify_dir_empty(SG_context * pCtx,
								   const SG_pathname * pPath)
{
	SG_bool bExists;
	SG_fsobj_stat stat;
	SG_uint32 sum = 0;

	SG_ERR_CHECK_RETURN(  SG_fsobj__exists_stat__pathname(pCtx, pPath, &bExists, &stat)  );
	if (!bExists)
		return;
	
	if ((stat.type & SG_FSOBJ_TYPE__DIRECTORY) == 0)
		SG_ERR_THROW2_RETURN(  SG_ERR_NOT_A_DIRECTORY,
							   (pCtx, "The path '%s' is not a directory.",
								SG_pathname__sz(pPath))  );

	SG_ERR_CHECK_RETURN(  SG_dir__foreach(pCtx, pPath, SG_DIR__FOREACH__SKIP_OS, _vv_utils__count_dir, (void *)&sum)  );
	if (sum > 0)
		SG_ERR_THROW2_RETURN(  SG_ERR_DIR_NOT_EMPTY,
							   (pCtx, "The directory '%s' is not empty.",
								SG_pathname__sz(pPath))  );
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV_UTILS__VERIFY_DIR_EMPTY_H
