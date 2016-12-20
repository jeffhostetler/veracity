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
 * @file sg_wc_sparse.c
 *
 * @details Routines to help with sparse working directories
 * and exports.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Convert the list of user-specified args of things to exclude
 * from the CHECKOUT or EXPORT
 *
 */
void SG_wc_sparse__build_pattern_list(SG_context * pCtx,
									  SG_repo * pRepo,
									  const char * pszHidCSet,
									  const SG_varray * pvaSparse,
									  SG_file_spec ** ppFilespec)
{
	SG_file_spec * pFilespec = NULL;
	SG_pathname * pPath_k = NULL;
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaSparse, &count)  );
	for (k=0; k<count; k++)
	{
		const char * psz_k;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaSparse, k, &psz_k)  );
		if (psz_k[0] == '@')
		{
			if (psz_k[1] == '@')
			{
				// I have reserved "@@" (technically "@@/") as a prefix
				// to indicate the (pRepo, pszHidCSet, repo-path) of a
				// file that contains a list of patterns to exclude.
				// W0769.
				// 
				// TODO 2011/10/20 slip inside the cset and get the contents
				// TODO            of the file with repo-path &psz_k[1] and
				// TODO            load that as-if it were an already existing
				// TODO            file on disk.
				SG_UNUSED( pRepo );
				SG_UNUSED( pszHidCSet );

				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "The '@@' repo-path prefix is reserved for future use.")  );
			}
			else if (psz_k[1] == '/')
			{
				// They gave a regular repo-path.  We define this to mean a
				// pattern to omit during the checkout.  Anything matching
				// it (and below) will be marked sparse (not populated).
				SG_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec,
															 SG_FILE_SPEC__PATTERN__EXCLUDE,
															 psz_k,
															 0)  );
			}
			else
			{
				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "Extended-prefix repo-paths are not supported here: %s", psz_k)  );
			}
		}
		else
		{
			// Anything else we assume is relative or absolute pathname
			// to a FILE OF PATTERNS TO OMIT.  See W5458 for more info.
			SG_fsobj_type fsType = SG_FSOBJ_TYPE__UNSPECIFIED;
			SG_bool bExists = SG_FALSE;

			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath_k, psz_k)  );
			SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_k, &bExists, &fsType, NULL)  );
			if (!bExists || (fsType != SG_FSOBJ_TYPE__REGULAR))
				SG_ERR_THROW2(  SG_ERR_NOTAFILE,
								(pCtx, "Sparse pattern file '%s' not found or not a file.",
								 SG_pathname__sz(pPath_k))  );

			SG_ERR_CHECK(  SG_file_spec__add_patterns__file(pCtx, pFilespec,
															SG_FILE_SPEC__PATTERN__EXCLUDE,
															pPath_k,
															0)  );
			SG_PATHNAME_NULLFREE(pCtx, pPath_k);
		}
	}

	*ppFilespec = pFilespec;
	return;

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_PATHNAME_NULLFREE(pCtx, pPath_k);
}

