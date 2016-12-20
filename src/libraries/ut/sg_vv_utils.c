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
 * @file sg_vv_utils.c
 *
 * @details Utility routines to help SG_vv_verbs.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

#include "sg_vv_utils__folder_arg_to_path.h"
#include "sg_vv_utils__lookup_wd_info.h"
#include "sg_vv_utils__verify_dir_empty.h"

//////////////////////////////////////////////////////////////////

void SG_util__convert_argv_into_stringarray(SG_context * pCtx,
											SG_uint32 argc, const char ** argv,
											SG_stringarray ** ppsa)
{
	SG_stringarray * psa = NULL;
	SG_uint32 nrKept = 0;
	SG_uint32 k;

	if (argc && argv)
	{
		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, argc)  );
		for (k=0; k<argc; k++)
		{
			if (!argv[k])	// argv ends at first null pointer regardless of what argc says.
				break;

			if (argv[k][0])		// skip over empty "" strings.
			{
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, argv[k])  );
				nrKept++;
			}
		}

		if (nrKept == 0)
			SG_STRINGARRAY_NULLFREE(pCtx, psa);
	}
	
	*ppsa = psa;
	return;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa);
}
