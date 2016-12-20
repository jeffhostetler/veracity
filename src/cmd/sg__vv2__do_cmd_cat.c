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
 * @file sg__vv2__do_cmd_cat.c
 *
 * @details 
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

void vv2__do_cmd_cat(SG_context * pCtx,
					 const SG_option_state * pOptSt,
					 const SG_stringarray * psaArgs)
{
	SG_pathname* pPathOutputFile = NULL;

	if (pOptSt->psz_output_file)
	{
		SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPathOutputFile, pOptSt->psz_output_file)  );
		SG_ERR_CHECK(  SG_vv2__cat__to_pathname(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec, psaArgs, pPathOutputFile)  );
	}
	else
	{
		// TODO 2012/12/21 For now, just splat the contents of each file
		// TODO            to the console without anything fancy.
		// TODO
		// TODO            We support multiple files in one command, but
		// TODO            only without bells or whistles.
		// TODO            Do we need to print headers or breaks between
		// TODO            them or should we just behave like '/bin/cat'?
		// TODO
		// TODO            Do we want to have a '/bin/cat -v' like mode?
		SG_ERR_CHECK(  SG_vv2__cat__to_console(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec, psaArgs)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathOutputFile);
}
