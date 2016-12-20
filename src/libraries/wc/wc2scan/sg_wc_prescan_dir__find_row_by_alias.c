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
 * @file sg_wc_prescan_dir__find_row_by_alias.c
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
 * Use the given alias to try to find the corresponding row.
 *
 * You DO NOT own the returned row.
 *
 */
void sg_wc_prescan_dir__find_row_by_alias(SG_context * pCtx,
										  const sg_wc_prescan_dir * pPrescanDir,
										  SG_uint64 uiAliasGid,
										  SG_bool * pbFound,
										  sg_wc_prescan_row ** ppPrescanRow)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__find(pCtx,
											   pPrescanDir->prb64PrescanRows,
											   uiAliasGid,
											   pbFound,
											   (void **)ppPrescanRow)  );
}
