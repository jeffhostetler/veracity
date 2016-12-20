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
 * Return the GID-domain repo-path of an item identified by
 * the given "input".  This is basically an "@<gid>".  This
 * is a convenience routine (mainly for testing) to help us
 * make tests a little easier to write since these are constant
 * for an item and immune to various tree-structure-contortions
 * that we are testing.
 * 
 * This uses the same algorithm that all of layer-7 WC TX API
 * uses to map arbitrary user input into a specific item.
 *
 * YOU PROBABLY DO NOT NEED/WANT TO CALL THIS ROUTINE since
 * most layers above layer-7 don't need to speak in terms of
 * GIDs and actually try to hide them from the user.
 *
 * WARNING: for UNCONTROLLED items, the returned value will
 * be a TEMP GID -- good for the duration of this TX.
 * 
 */
void SG_wc_tx__get_item_gid_path(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const char * pszInput,
								 char ** ppszGidPath)
{
	SG_string * pString = NULL;
	char * pszGid = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );	// throw if omitted, do not assume "@/".
	SG_NULLARGCHECK_RETURN( ppszGidPath );

	SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx, pszInput, &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pString)  );
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pString, (SG_byte **)ppszGidPath, NULL)  );
	
fail:
	SG_NULLFREE(pCtx, pszGid);
	SG_STRING_NULLFREE(pCtx, pString);
}

