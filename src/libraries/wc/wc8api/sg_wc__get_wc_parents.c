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
 * @file sg_wc__get_wc_parents.c
 *
 * @details Wrapper for PARENTS.
 *
 * NOTE: sg.wc.parents() returns the array of parents of the WD.
 * After I wrote this, I created sg.wc.get_wc_info() that returns
 * a VHASH of stuff and contains the parents array within it.
 * So if you need more info (user, path, repo_name, etc) in
 * addition to the parents, you might consider using it instead.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_wc__get_wc_parents__varray(SG_context * pCtx,
								   const SG_pathname* pPathWc,
								   SG_varray ** ppvaParents)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx, pWcTx, ppvaParents)  )  );
}

//////////////////////////////////////////////////////////////////

void SG_wc__get_wc_parents__stringarray(SG_context * pCtx,
										const SG_pathname* pPathWc,
										SG_stringarray ** ppsaParents)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__stringarray(pCtx, pWcTx, ppsaParents)  )  );
}

//////////////////////////////////////////////////////////////////

void SG_wc__get_wc_baseline(SG_context * pCtx,
							const SG_pathname* pPathWc,
							char ** ppszHidBaseline,
							SG_bool * pbHasMerge)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_wc_baseline(pCtx, pWcTx, ppszHidBaseline, pbHasMerge)  )  );
}
