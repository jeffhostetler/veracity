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
 * @file sg_wc_db__state.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_db__state_structural__free(SG_context * pCtx, sg_wc_db__state_structural * p_s)
{
	if (!p_s)
		return;
	
	SG_NULLFREE(pCtx, p_s->pszEntryname);

	SG_NULLFREE(pCtx, p_s);
}

void sg_wc_db__state_structural__alloc(SG_context * pCtx, sg_wc_db__state_structural ** pp_s)
{
	sg_wc_db__state_structural * p_s = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, p_s)  );

	*pp_s = p_s;
}

void sg_wc_db__state_structural__alloc__copy(SG_context * pCtx,
											 sg_wc_db__state_structural ** pp_s,
											 const sg_wc_db__state_structural * p_s_src)
{
	sg_wc_db__state_structural * p_s = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, p_s)  );

	p_s->uiAliasGid = p_s_src->uiAliasGid;
	p_s->uiAliasGidParent = p_s_src->uiAliasGidParent;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, p_s_src->pszEntryname, &p_s->pszEntryname)  );
	p_s->tneType = p_s_src->tneType;

	*pp_s = p_s;
	return;

fail:
	SG_WC_DB__STATE_STRUCTURAL__NULLFREE(pCtx, p_s);
}

void sg_wc_db__state_structural__alloc__fields(SG_context * pCtx,
											   sg_wc_db__state_structural ** pp_s,
											   SG_uint64 uiAliasGid,
											   SG_uint64 uiAliasGidParent,
											   const char * pszEntryname,
											   SG_treenode_entry_type tneType)
{
	sg_wc_db__state_structural * p_s = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, p_s)  );

	p_s->uiAliasGid = uiAliasGid;
	p_s->uiAliasGidParent = uiAliasGidParent;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszEntryname, &p_s->pszEntryname)  );
	p_s->tneType = tneType;

	*pp_s = p_s;
	return;

fail:
	SG_WC_DB__STATE_STRUCTURAL__NULLFREE(pCtx, p_s);
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__state_dynamic__free(SG_context * pCtx, sg_wc_db__state_dynamic * p_d)
{
	if (!p_d)
		return;

	SG_NULLFREE(pCtx, p_d->pszHid);
	SG_NULLFREE(pCtx, p_d);
}

void sg_wc_db__state_dynamic__alloc(SG_context * pCtx, sg_wc_db__state_dynamic ** pp_d)
{
	sg_wc_db__state_dynamic * p_d = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, p_d)  );

	*pp_d = p_d;
}

void sg_wc_db__state_dynamic__alloc__copy(SG_context * pCtx,
										  sg_wc_db__state_dynamic ** pp_d,
										  const sg_wc_db__state_dynamic * p_d_src)
{
	sg_wc_db__state_dynamic * p_d = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, p_d)  );
	p_d->attrbits = p_d_src->attrbits;
	if (p_d_src->pszHid && *p_d_src->pszHid)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, p_d_src->pszHid, &p_d->pszHid)  );

	*pp_d = p_d;
	return;

fail:
	SG_WC_DB__STATE_DYNAMIC__NULLFREE(pCtx, p_d);
}

//////////////////////////////////////////////////////////////////
