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
 * @file sg_wc_attrbits.c
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
 * Fetch the sets of defined ATTRBITS bit masks.
 * This probably doesn't need to ever move to a repo-config.
 *
 */
void SG_wc_attrbits__get_masks_from_config(SG_context * pCtx,
										   SG_repo * pRepo,
										   const SG_pathname * pPathWorkingDirectoryTop,
										   SG_wc_attrbits_data ** ppAttrbitsData)
{
	SG_wc_attrbits_data * pData = NULL;

	SG_UNUSED( pRepo );
	SG_UNUSED( pPathWorkingDirectoryTop );

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pData)  );
	pData->mask_defined = SG_WC_ATTRBITS__MASK;

	pData->mask_defined_platform = SG_WC_ATTRBITS__ZERO;
#if defined(WINDOWS)
	// Windows does not support 'chmod +x'.
#else
	pData->mask_defined_platform |= SG_WC_ATTRBITS__FILE_X;
#endif

	*ppAttrbitsData = pData;
}

//////////////////////////////////////////////////////////////////

void SG_wc_attrbits_data__free(SG_context * pCtx,
							   SG_wc_attrbits_data * pAttrbitsData)
{
	if (!pAttrbitsData)
		return;

	SG_NULLFREE(pCtx, pAttrbitsData);
}

//////////////////////////////////////////////////////////////////

/**
 * Compute ATTRBITS *based strictly on the given PERMS*
 * and as is appropriate on this platform for this type
 * of item.
 *
 */
void sg_wc_attrbits__compute_from_perms(SG_context * pCtx,
										const SG_wc_attrbits_data * pAttrbitsData,
										SG_treenode_entry_type tneType,
										SG_fsobj_perms perms,
										SG_uint64 * pAttrbits)
{
	SG_uint64 attrbits = SG_WC_ATTRBITS__ZERO;

	SG_UNUSED( pCtx );

	if (pAttrbitsData->mask_defined_platform & SG_WC_ATTRBITS__FILE_X)
	{
		// we respect the '__file_x' bit on this platform.

		if (tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		{
			// we only define the '__file_x' bit for files.

			if (perms & 0100)	// The 'u+x' bit is '100 from '777 mask (aka S_IXUSR)
				attrbits |= SG_WC_ATTRBITS__FILE_X;
			else
				attrbits &= ~SG_WC_ATTRBITS__FILE_X;
		}
	}

	*pAttrbits = attrbits;
}

//////////////////////////////////////////////////////////////////

/**
 * Using the platform-attribute-mask and given attrbit values,
 * compute the PERMS to use when creating a NEW file.
 *
 * TODO 2011/10/19 Think about having a repo-config setting for
 * TODO            the default mode bits/umask for new files.
 * TODO            The 0644 could just as well be 0600.
 * 
 */
void SG_wc_attrbits__compute_perms_for_new_file_from_attrbits(SG_context * pCtx,
															  const SG_wc_attrbits_data * pAttrbitsData,
															  SG_uint64 attrbits,
															  SG_fsobj_perms * pPerms)
{
	SG_fsobj_perms perms = 0644;
	
	SG_UNUSED( pCtx );

	if (attrbits & SG_WC_ATTRBITS__FILE_X)
	{
		if (pAttrbitsData->mask_defined_platform & SG_WC_ATTRBITS__FILE_X)
		{
			perms |= 0100;		// set 'u+x' bit (aks S_IXUSR)
		}
	}

	*pPerms = perms;
}

//////////////////////////////////////////////////////////////////

/**
 * Combine the baseline and observed ATTRBITS based upon
 * the rule that we should inherit the bits not defined
 * for this platform from the baseline and get the value
 * of the bits defined for this platform.
 *
 * That is, if a Linux user did a 'chmod +x' on a file
 * and we did a checkout on Windows and do a STATUS,
 * we won't *actually* see a 'u+x' on the file when we
 * stat() it (because Windows can't), but we want to
 * see the effective bits include it -- and be able to
 * say the 'x' bit hasn't changed -- it's not the case
 * that the user turned it off, rather we couldn't set
 * it.
 *
 */
void sg_wc_attrbits__compute_effective_attrbits(SG_context * pCtx,
												const SG_wc_attrbits_data * pAttrbitsData,
												SG_uint64 attrbitsBaseline,
												SG_uint64 attrbitsObserved,
												SG_uint64 * pAttrbitsEffective)
{
	SG_UNUSED( pCtx );

	*pAttrbitsEffective = (attrbitsBaseline & ~pAttrbitsData->mask_defined_platform) | attrbitsObserved;
}
