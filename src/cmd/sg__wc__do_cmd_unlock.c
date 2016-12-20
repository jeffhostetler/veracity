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
 * @file sg__wc__do_cmd_unlock.c
 *
 * @details Handle 'vv unlock'.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

void wc__do_cmd_unlock(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const SG_stringarray * psaArgs,
					   const char* psz_username,
					   const char* psz_password)
{
	SG_ERR_CHECK(  SG_wc__unlock(pCtx,
								 NULL,
								 psaArgs,
								 pOptSt->bForce,
								 psz_username,
								 psz_password,
								 pOptSt->psz_server)  );

fail:
	return;
}
