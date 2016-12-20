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
 * @file sg__vv2__do_cmd_tag.c
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

void vv2__do_cmd_tags(SG_context * pCtx,
					  const SG_option_state * pOptSt)
{
	SG_varray * pvaTags = NULL;
	SG_uint32 countRevSpecs = 0;
	SG_uint32 k, countTags;

#if 1 // Need W6606
	// TODO 2012/07/02 Until W6606 is fixed we manually cross-check for
	// TODO            invalid options from other subcommands.
	// TODO            vv tag <subcommand> takes: --repo --rev --branch --tag
	// TODO            vv tag list         takes: --repo
	// TODO            vv tags             takes: --repo

	if (pOptSt->pRevSpec != NULL)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
	if (countRevSpecs > 0)
		SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Do not specify a revision when listing all tags."));
#endif

	SG_ERR_CHECK(  SG_vv2__tags(pCtx, pOptSt->psz_repo, SG_TRUE, &pvaTags)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaTags, &countTags)  );
	for (k=0; k<countTags; k++)
	{
		SG_vhash * pvh_k;
		const char * psz_tag_k;
		const char * psz_csid_k;
		SG_uint32 revno_k;
		SG_bool bPresentInLocalRepo;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaTags, k, &pvh_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(    pCtx, pvh_k, "tag",   &psz_tag_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(    pCtx, pvh_k, "csid",  &psz_csid_k)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_k, "revno", &bPresentInLocalRepo)  );
		if (bPresentInLocalRepo)
		{
			SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh_k, "revno", &revno_k)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%20s: %6u:%s\n", psz_tag_k, revno_k, psz_csid_k)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%20s: %6s:%s (not present in repository)\n", psz_tag_k, "?", psz_csid_k)  );
		}
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaTags);
}

//////////////////////////////////////////////////////////////////

void vv2__do_cmd_tag__subcommand(SG_context * pCtx,
								 const SG_option_state * pOptSt,
								 SG_uint32 argc,
								 const char ** argv)
{
	SG_uint32 countRevSpecs = 0;

	if (strcmp(argv[0],"add") == 0)
	{
#if 1 // Need W6606
		// TODO            vv tag <subcommand> takes: --repo --rev --branch --tag
		// TODO            vv tag move         takes: --repo --rev --branch --tag
#endif

		// argv[0] is subcommand name
		// argv[1] is tag name

		if (argc != 2)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		SG_ERR_CHECK(  SG_vv2__tag__add(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec, argv[1], SG_FALSE)  );
	}
	else if (strcmp(argv[0],"move") == 0)
	{
#if 1 // Need W6606
		// TODO            vv tag <subcommand> takes: --repo --rev --branch --tag
		// TODO            vv tag move         takes: --repo --rev --branch --tag
#endif

		// argv[0] is subcommand name
		// argv[1] is tag name

		if (argc != 2)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		SG_ERR_CHECK(  SG_vv2__tag__move(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec, argv[1])  );
	}
	else if (strcmp(argv[0],"remove") == 0)
	{

#if 1 // Need W6606
		// TODO 2012/07/02 Until W6606 is fixed we manually cross-check for
		// TODO            invalid options from other subcommands.
		// TODO            vv tag <subcommand> takes: --repo --rev --branch --tag
		// TODO            vv tag remove       takes: --repo
		if (pOptSt->pRevSpec != NULL)
			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
		if (countRevSpecs > 0)
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Do not specify a revision when removing a tag."));
#endif

		// argv[0] is subcommand name
		// argv[1] is tag name

		if (argc != 2)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		SG_ERR_CHECK(  SG_vv2__tag__remove(pCtx, pOptSt->psz_repo, argv[1])  );
	}
	else if (strcmp(argv[0],"list") == 0)
	{
		// argv[0] is subcommand name

		if (argc != 1)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		SG_ERR_CHECK(  vv2__do_cmd_tags(pCtx, pOptSt)  );
	}
	else
	{
		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Unknown subcommand '%s'.", argv[0])  );
	}

fail:
	return;
}
