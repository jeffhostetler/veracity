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

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Return the GID of an item identified by the given "input".
 * 
 * YOU PROBABLY DO NOT NEED/WANT TO CALL THIS ROUTINE since
 * most layers above layer-7 don't need to speak in terms of
 * GIDs and actually try to hide them from the user.
 *
 * WARNING: UNCONTROLLED items DO NOT have a permanent GID and
 * will change with each call and/or TX.
 *
 */
void SG_wc__get_item_gid(SG_context * pCtx,
						 const SG_pathname* pPathWc,
						 const char * pszInput,
						 char ** ppszGid)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx,
											  pszInput,
											  ppszGid)  )  );
}

/**
 * Return a stringarray of GIDs for each of the "input".
 *
 */
void SG_wc__get_item_gid__stringarray(SG_context * pCtx,
									  const SG_pathname* pPathWc,
									  const SG_stringarray * psaInput,
									  SG_stringarray ** ppsaGid)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_item_gid__stringarray(pCtx, pWcTx,
														   psaInput,
														   ppsaGid)  )  );
}

