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
 * @file sg_wc__status__tne_type_to_flags.c
 *
 * @details Routine to convert a tneType into one of the _T_
 * type status flags.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Since we now have __T_ file-type bits in a SG_wc_status_flags
 * which are strictly based on the file-type and nothing else, we
 * can isolate this code so that SG_vv2__status_() can use it.
 *
 */
void SG_wc__status__tne_type_to_flags(SG_context * pCtx,
									  SG_treenode_entry_type tneType,
									  SG_wc_status_flags * pStatusFlags)
{
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;

	SG_NULLARGCHECK_RETURN( pStatusFlags );

	switch (tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		statusFlags = SG_WC_STATUS_FLAGS__T__FILE;
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		statusFlags = SG_WC_STATUS_FLAGS__T__DIRECTORY;
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		statusFlags = SG_WC_STATUS_FLAGS__T__SYMLINK;
		break;

	case SG_TREENODEENTRY_TYPE_SUBMODULE:
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED );

	case SG_TREENODEENTRY_TYPE__DEVICE:
		statusFlags = SG_WC_STATUS_FLAGS__T__DEVICE;
		break;

	default:
		statusFlags = SG_WC_STATUS_FLAGS__T__BOGUS;
		break;
	}

	*pStatusFlags = statusFlags;

}

