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

void sg_wc_tx__apply__resolve_issue__s(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   const SG_vhash * pvh)
{
#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__resolve_issue__s:\n"))  );
#else
	SG_UNUSED( pCtx );
#endif

	SG_UNUSED( pWcTx );
	SG_UNUSED( pvh );

	// we don't actually have anything here.
	// the journal record was more for the verbose log.
	// the actual work of updating the SQL will be done
	// in the parallel journal-stmt.
}

void sg_wc_tx__apply__resolve_issue__sr(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh)
{
#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__resolve_issue__sr:\n"))  );
#else
	SG_UNUSED( pCtx );
#endif

	SG_UNUSED( pWcTx );
	SG_UNUSED( pvh );

	// we don't actually have anything here.
	// the journal record was more for the verbose log.
	// the actual work of updating the SQL will be done
	// in the parallel journal-stmt.
}
