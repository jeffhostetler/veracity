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
 * @file sg_wc_tx__resolve__state__inline2.h
 *
 * @details SG_resolve__state__ routines that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__STATE__INLINE2_H
#define H_SG_WC_TX__RESOLVE__STATE__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_resolve__state__get_info(
	SG_context*       pCtx,
	SG_resolve__state eState,
	const char**      ppName,
	const char**      ppProblem,
	const char**      ppCommand
	)
{
	SG_ARGCHECK(eState < SG_RESOLVE__STATE__COUNT, eState);

	if (ppName != NULL)
	{
		*ppName = gaStateData[eState].szName;
	}
	if (ppProblem != NULL)
	{
		*ppProblem = gaStateData[eState].szProblem;
	}
	if (ppCommand != NULL)
	{
		*ppCommand = gaStateData[eState].szCommand;
	}

fail:
	return;
}

void SG_resolve__state__check_name(
	SG_context*        pCtx,
	const char*        szName,
	SG_resolve__state* pState
	)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pState);

	for (uIndex = 0u; uIndex < SG_RESOLVE__STATE__COUNT; ++uIndex)
	{
		if (SG_stricmp(szName, gaStateData[uIndex].szName) == 0)
		{
			break;
		}
	}

	*pState = (SG_resolve__state)uIndex;

fail:
	return;
}

void SG_resolve__state__parse_name(
	SG_context*        pCtx,
	const char*        szName,
	SG_resolve__state* pState
	)
{
	SG_resolve__state eState = SG_RESOLVE__STATE__COUNT;

	SG_NULLARGCHECK(pState);

	SG_ERR_CHECK(  SG_resolve__state__check_name(pCtx, szName, &eState)  );
	if (eState == SG_RESOLVE__STATE__COUNT)
	{
		SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "conflict state by name: %s", szName));
	}

	*pState = eState;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__STATE__INLINE2_H
