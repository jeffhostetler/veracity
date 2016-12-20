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
 * @file sg_wc__jsglue.c
 *
 * @details Make all the various 'wc' routines available from JavaScript.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Public routine to install all of the various APIs/Classes/Methods
 * into JavaScript.
 *
 */
void SG_wc__jsglue__install_scripting_api(SG_context * pCtx,
										  JSContext * cx,
										  JSObject * glob,
										  JSObject * pjsobj_sg)
{
	// install 'wc' into 'sg' (giving 'sg.wc.*').
	SG_ERR_CHECK_RETURN(  sg_wc__jsglue__install_scripting_api__wc   (pCtx, cx, glob, pjsobj_sg)  );

	// install 'sg_wc_tx' into global scope (giving 'new sg_wx_tx()').
	SG_ERR_CHECK_RETURN(  sg_wc__jsglue__install_scripting_api__wc_tx(pCtx, cx, glob, pjsobj_sg)  );
}


