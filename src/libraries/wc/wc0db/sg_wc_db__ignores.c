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
 * @file sg_wc_db__ignores.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * If necessary, load the ignores settings from config/.vvignores.
 * This is cached in the "pDb" for later use.
 *
 */
void sg_wc_db__ignores__load(SG_context * pCtx, sg_wc_db * pDb)
{
	if (!pDb->pFilespecIgnores)
	{
		SG_ERR_CHECK(  SG_FILE_SPEC__ALLOC__COPY(pCtx, gpBlankFilespec, &pDb->pFilespecIgnores)  );
		SG_ERR_CHECK(  SG_file_spec__load_ignores(pCtx,
												  pDb->pFilespecIgnores,
												  pDb->pRepo,
												  pDb->pPathWorkingDirectoryTop,
												  0u)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * This only tests whether the pathname matches
 * an IGNORE pattern.  It *DOES NOT* know if the
 * item is controlled or uncontrolled.
 *
 * Since ignore-patterns can include repo-paths and
 * absolute pathnames, we should be careful to always
 * evaluate the ignores using a "live (in-tx)" path
 * rather than a "reference (pre-tx)" path.
 *
 * The assumption is that if you are in multi-step
 * transaction (like an UPDATE), and we are asked to
 * compute the ignorability of an item, our answer
 * should reflect the changes QUEUED in this TX
 * (because with the "LiveView" stuff we are trying
 * to present the illusion of an immediate apply).
 *
 * *BUT* we only take a repo-path (which are not
 * qualified (live/reference)), so beware.
 *
 */
void sg_wc_db__ignores__matches_ignorable_pattern(SG_context * pCtx,
												  sg_wc_db * pDb,
												  const SG_string * pStringLiveRepoPath,
												  SG_bool * pbIgnorable)
{
	SG_file_spec__match_result eval;

	if (!pDb->pFilespecIgnores)
		SG_ERR_CHECK(  sg_wc_db__ignores__load(pCtx, pDb)  );
	
	SG_ERR_CHECK(  SG_file_spec__should_include(pCtx, pDb->pFilespecIgnores,
												SG_string__sz(pStringLiveRepoPath),
												(SG_FILE_SPEC__MATCH_ANYWHERE
												 |SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY
												 |SG_FILE_SPEC__MATCH_TRAILING_SLASH),
												&eval)  );
	*pbIgnorable = (eval == SG_FILE_SPEC__RESULT__IGNORED);
	
fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Return the set of IGNORES for this WD as a VARRAY of SZ.
 * This is mainly intended for information purposes (such as
 * printing the set of ignores on the console), you shouldn't
 * need to use this because the WC layer automatically takes
 * care of everything associated with ignores.
 *
 * You own the returned VARRAY.
 * 
 */
void sg_wc_db__ignores__get_varray(SG_context * pCtx,
								   sg_wc_db * pDb,
								   SG_varray ** ppva)
{
	SG_varray * pvaAllocated = NULL;

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaAllocated)  );

	if (!pDb->pFilespecIgnores)
		SG_ERR_CHECK(  sg_wc_db__ignores__load(pCtx, pDb)  );

	if (pDb->pFilespecIgnores)
	{
		SG_uint32 k, count;

		SG_ERR_CHECK(  SG_file_spec__count_patterns(pCtx, pDb->pFilespecIgnores, SG_FILE_SPEC__PATTERN__IGNORE, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;

			SG_ERR_CHECK(  SG_file_spec__get_nth_pattern(pCtx, pDb->pFilespecIgnores, SG_FILE_SPEC__PATTERN__IGNORE, k, &psz_k, NULL)  );
			SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pvaAllocated, psz_k)  );
		}
	}

	*ppva = pvaAllocated;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaAllocated);
}
