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

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void sg_vv2__diff__header(SG_context * pCtx,
						  const SG_vhash * pvhItem,
						  SG_string ** ppStringHeader)
{
	// TODO 2012/05/08 We may want to make the header different for
	// TODO            historical diffs (legend info maybe).
	// TODO
	// TODO            But for now, just assume that the WC version
	// TODO            is sufficient.
	// TODO
	// TODO            Note that this requires that we use _subsection_
	// TODO            headers that it recognizes.
	// TODO            See sg_wc__status__classic_format.c:_format_primary_trailer()

	SG_ERR_CHECK_RETURN(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, ppStringHeader)  );
}

