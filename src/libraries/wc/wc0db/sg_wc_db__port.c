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
 * @file sg_wc_db__port.c
 *
 * @details Convenience wrapper for the PORTABILITY and DB code.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Allocate and return an empty portability collider
 * (using the config settings appropriate for the
 * associated repo/WD).
 * 
 * It is NOT associated with any particular directory.
 *
 */
void sg_wc_db__create_port(SG_context * pCtx,
						   const sg_wc_db * pDb,
						   SG_wc_port ** ppPort)
{
	SG_ERR_CHECK_RETURN(  SG_wc_port__alloc(pCtx,
											pDb->pRepo,
											pDb->pPathWorkingDirectoryTop,
											pDb->portMask,
											ppPort)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Allocate and return an empty portability collider
 * (using the config settings appropriate for the
 * associated repo/WD).
 *
 * BUT TURN OFF ALL _INDIV_ BITS.
 * 
 * It is NOT associated with any particular directory.
 *
 */
void sg_wc_db__create_port__no_indiv(SG_context * pCtx,
									 const sg_wc_db * pDb,
									 SG_wc_port ** ppPort)
{
	SG_ERR_CHECK_RETURN(  SG_wc_port__alloc(pCtx,
											pDb->pRepo,
											pDb->pPathWorkingDirectoryTop,
											pDb->portMask & ~SG_WC_PORT_FLAGS__MASK_INDIV,
											ppPort)  );
}

