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

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

/**
 * report results of ADD, REMOVE, or ADDREMOVE to the console.
 *
 */
void sg_report_addremove_journal(SG_context * pCtx, const SG_varray * pva,
								 SG_bool bAdded, SG_bool bRemoved,
								 SG_bool bVerbose)
{
	SG_uint32 nrAdded = 0;
	SG_uint32 nrRemoved = 0;
	SG_uint32 k, count;
	SG_string * pStringVerbose = NULL;

	if (bVerbose)
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringVerbose)  );

	if (pva)
	{
		SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
		for (k=0; k<count; k++)
		{
			SG_vhash * pvh;
			const char * pszOp;
			const char * pszSrc;

			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, k, &pvh)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "op", &pszOp)  );
			if (strncmp(pszOp, "add", 3) == 0)
			{
				nrAdded++;
				if (bVerbose)
				{
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "src", &pszSrc)  );
					SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringVerbose,
															 "  Adding: %s\n", pszSrc)  );
				}
			}
			else if ((strncmp(pszOp, "remove", 6) == 0)
					 || (strncmp(pszOp, "undo_add", 8) == 0))
			{
				nrRemoved++;
				if (bVerbose)
				{
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "src", &pszSrc)  );
					SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringVerbose,
															 "Removing: %s\n", pszSrc)  );
				}
			}
			else
			{
				// TODO 2013/01/04 This is an error, but not severe enough to stop things.
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "src", &pszSrc)  );
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TODO ? %s %s\n", pszOp, pszSrc)  );
			}
		}
	}

	if (bAdded && bRemoved)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%d added, %d removed\n", nrAdded, nrRemoved)  );
	else if (bAdded)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%d added\n", nrAdded)  );
	else if (bRemoved)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%d removed\n", nrRemoved)  );

	if (bVerbose)
		SG_ERR_IGNORE(  SG_console__raw(pCtx, SG_CS_STDOUT, SG_string__sz(pStringVerbose))  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringVerbose);
}

