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
 * @file sg__vv2__do_cmd_comment.c
 *
 * @details Handle details of a 'vv comment' command.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

void vv2__do_cmd_comment(SG_context * pCtx,
						 const SG_option_state * pOptSt)
{
	SG_ERR_CHECK(  SG_vv2__comment(pCtx,
								   pOptSt->psz_repo,
								   pOptSt->pRevSpec,
								   pOptSt->psz_username,
								   pOptSt->psz_when,
								   pOptSt->psz_message,
								   ((pOptSt->bPromptForDescription)
									? SG_cmd_util__get_comment_from_editor :
									NULL))  );

	// TODO 2011/11/07 Consider printing the affected CSET
	// TODO            like we do after a COMMIT so that they
	// TODO            can see the new comment in context with
	// TODO            the rest of the CSET.

fail:
	return;
}

						
