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
 * @file sg_vv_utils__lookup_wd_info.h
 *
 * @details Routine to take a directory path anywhere within a working directory
 * and return official info for the root of the working directory.  That is,
 * find the directory associated with "@/" from any point within it.  For the
 * root directory, it returns itself.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_UTILS__LOOKUP_WD_INFO_H
#define H_SG_VV_UTILS__LOOKUP_WD_INFO_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_vv_utils__throw_NOTAREPOSITORY_for_wd_association(SG_context * pCtx,
                                                          const char * szRepoName,
                                                          SG_pathname * pWdRoot)
{
	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(szRepoName!=NULL);
	if(pWdRoot)
	{
		SG_ERR_THROW2_RETURN(SG_ERR_NOTAREPOSITORY,
							 (pCtx,
							  (SG_CR_STR // Special trick to hide the "Not a repository:" prefix.
							   "The path\n"
							   "    '%s'\n"
							   "is a working copy for the '%s' repository, which does not exist,\n"
							   "or has been deleted and recreated.\n"
							   "\n"
							   "This error can happen for various reasons, but most likely the\n"
							   "repository has been renamed or deleted.\n"
							   "\n"
							   "To remove the working copy's association with this repository,\n"
							   "remove the \".sgdrawer\" folder from the root of the working copy."),
							  SG_pathname__sz(pWdRoot), szRepoName)  );
	}
	else
	{
		SG_ERR_THROW2_RETURN(SG_ERR_NOTAREPOSITORY,
							 (pCtx,
							  (SG_CR_STR // Special trick to hide the "Not a repository:" prefix.
							   "This path is in a working copy for the '%s' repository,\n"
							   "which does not exist, or has been deleted and recreated.\n"
							   "\n"
							   "This error can happen for various reasons, but most likely\n"
							   "the repository has been renamed or deleted."),
							  szRepoName)  );
	}
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV_UTILS__LOOKUP_WD_INFO_H
