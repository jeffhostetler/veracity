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
 * @file sg_wc_tx__update__determine_target_cset.c
 *
 * @details Determine the target cset of the UPDATE using
 * the various input args and the state of the current WD.
 *
 * This replaces SG_vv_util__rev_spec_to_csid__wd().
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _validate_basic_args(SG_context * pCtx,
								 sg_update_data * pUpdateData)
{
	SG_uint32 nrArgs = 0;

	// Allow at most one of --detach, --attach, --attach-new, --attach-current.

	if (pUpdateData->pUpdateArgs->bDetached)
		nrArgs++;

	if (pUpdateData->pUpdateArgs->bAttachCurrent)
		nrArgs++;

	if (pUpdateData->pUpdateArgs->pszAttach && *pUpdateData->pUpdateArgs->pszAttach)
		nrArgs++;

	if (pUpdateData->pUpdateArgs->pszAttachNew && *pUpdateData->pUpdateArgs->pszAttachNew)
		nrArgs++;

	if (nrArgs > 1)
		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "The 'attach', 'attach-new', 'attach-current', and 'detach' options cannot be combined.")  );

	// Allow at most one of --rev, --tag, or --branch.

	if (pUpdateData->pUpdateArgs->pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pUpdateData->pUpdateArgs->pRevSpec, &nrArgs)  );
		if (nrArgs > 1)
			SG_ERR_THROW2(  SG_ERR_USAGE,
							(pCtx, "The 'rev', 'tag', and 'branch' options cannot be combined.")  );

		// if it OK if (nrArgs == 0).
	}

fail:
	return;
}

static void _get_starting_conditions(SG_context * pCtx,
									 sg_update_data * pUpdateData)
{
	SG_varray * pvaParents = NULL;
	const char * pszBaseline;
	SG_uint32 nrParents;

	// Get the HID of the current baseline.
	// Disallow UPDATE if we have a pending MERGE.

	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx,
													pUpdateData->pWcTx,
													&pvaParents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &nrParents)  );
	if (nrParents > 1)
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE,
						(pCtx, "Cannot UPDATE with uncommitted MERGE.")  );

	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, 0, &pszBaseline)  );
	SG_ERR_CHECK(  SG_strdup(pCtx, pszBaseline, &pUpdateData->pszHid_StartingBaseline)  );

	// See if the WD is currently attached/tied to a branch.
	// We DO NOT care one way or another at this point.
	// Later as we are parsing the args we can supply
	// the current branch as the default as necessary.

	SG_ERR_CHECK(  SG_wc_tx__branch__get(pCtx, pUpdateData->pWcTx,
										&pUpdateData->pszBranchName_Starting)  );

#if TRACE_WC_UPDATE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Update: baseline (%s, %s).\n",
							   pUpdateData->pszHid_StartingBaseline,
							   ((pUpdateData->pszBranchName_Starting) ? pUpdateData->pszBranchName_Starting : ""))  );
#endif

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
}

static void _compute_target_hid_from_rev_spec(SG_context * pCtx,
											  sg_update_data * pUpdateData,
											  const SG_rev_spec * pRevSpec)
{
	SG_NULLARGCHECK_RETURN( pUpdateData );
	SG_NULLARGCHECK_RETURN( pRevSpec );

	// Parse the contents of the *GIVEN* rev-spec (don't assume the one
	// in pUpdateArgs).
	// 
	// [] if we have '--rev prefix', expand the prefix to a full/unique HID.
	//    returns (HID, null).
	//    throws if ambiguous.
	//
	// [] if we have '--tag tag', lookup the tag and get the implied HID.
	//    returns (HID, null).
	//    throws if not found.
	//
	// [] if we have '--branch name', lookup the HID of the unique head.
	//    returns (HID, NAME).
	//    throws if branch not found.
	//    throws if branch has multiple heads.
	//    throws if head refers to a changset that isn't present.
	//
	// TODO 2012/01/10 In the 'branch has multiple heads' case, consider
	// TODO            trying to choose one of them.  For example, if
	// TODO            only one is a descendant of the current baseline
	// TODO            it might be a good choice (we would still need to
	// TODO            warn them about the branch needing merging).

	SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx,
											  pUpdateData->pWcTx->pDb->pRepo,
											  pRevSpec,
											  SG_TRUE,
											  &pUpdateData->pszHidTarget,
											  &pUpdateData->pszBranchNameTarget)  );
	if (pUpdateData->pszBranchNameTarget)
	{
		// they used '-b'.
		// pUpdateData->pszHidTarget is the unique head of the branch.
		pUpdateData->bRequestedByBranch = SG_TRUE;
	}
	else
	{
		// they used '-r' or '-t'.
		// pUpdateData->pszHidTarget MAY OR MAY NOT be a head.
		pUpdateData->bRequestedByRev = SG_TRUE;
	}
		
	// Regardless of what they gave us in pRevSpec,
	// we don't know anything about the relationship
	// between the current baseline and the requested
	// target changeset.  defer computing it until we
	// need it.
	pUpdateData->dqRel = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;

fail:
	return;
}

static void _compute_target_hid(SG_context * pCtx,
								sg_update_data * pUpdateData)
{
	SG_rev_spec * pRevSpecPrivate = NULL;
	SG_uint32 nrArgs = 0;
	SG_dagquery_find_head_status dqfhs;
	char bufHidGoal[SG_HID_MAX_BUFFER_LENGTH];

	if (pUpdateData->pUpdateArgs->pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pUpdateData->pUpdateArgs->pRevSpec, &nrArgs)  );
		if (nrArgs > 0)
		{
			SG_ERR_CHECK(  _compute_target_hid_from_rev_spec(pCtx,
															 pUpdateData,
															 pUpdateData->pUpdateArgs->pRevSpec)  );
			goto done;
		}
	}

	// no -r, -t, or -b

	if (pUpdateData->pszBranchName_Starting)
	{
		// Since the WD is tied/attached to a branch,
		// try to find a unique head on this branch.
		//
		// I'm going to be lazy here and create a
		// private rev-spec with the current branch
		// and let the same code handle it as if
		// they gave us '--branch <current_branch>'
		// as input.

		SG_ERR_CHECK(  SG_rev_spec__alloc(pCtx, &pRevSpecPrivate)  );
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx,
											   pRevSpecPrivate,
											   pUpdateData->pszBranchName_Starting)  );
		_compute_target_hid_from_rev_spec(pCtx, pUpdateData, pRevSpecPrivate);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			if (SG_context__err_equals(pCtx, SG_ERR_BRANCH_NOT_FOUND))
			{
				// Y8393. complain if they did:
				//     vv branch new foo
				//     vv update (with no args)
				// because a new branch doesn't yet have any commits, so
				// the branch doesn't have any targets -- granted, the WD
				// has a changeset and we could try to update as if they
				// weren't attached, but this is all a bit ambiguous.
				SG_context__err_reset(pCtx);
				SG_ERR_THROW(  SG_ERR_UPDATE_FROM_NEW_BRANCH  );
			}
			SG_ERR_RETHROW;
		}

		goto done;
	}

	// no rev-spec given and we are untied.
	// 
	// see if there is a single/unique descendent
	// head from the current baseline.

	SG_ERR_CHECK(  SG_dagquery__find_single_descendant_head(pCtx,
															pUpdateData->pWcTx->pDb->pRepo,
															SG_DAGNUM__VERSION_CONTROL,
															pUpdateData->pszHid_StartingBaseline,
															&dqfhs,
															bufHidGoal, (sizeof(bufHidGoal)))  );
	switch (dqfhs)
	{
	case SG_DAGQUERY_FIND_HEAD_STATUS__IS_LEAF:
		// the current baseline *is* a leaf (and therefore has no descendants).
		// our caller will deal with this trivial case.
		pUpdateData->dqRel = SG_DAGQUERY_RELATIONSHIP__SAME;
		break;

	case SG_DAGQUERY_FIND_HEAD_STATUS__UNIQUE:
		// the baseline has a single descendant.
		pUpdateData->dqRel = SG_DAGQUERY_RELATIONSHIP__DESCENDANT;
		break;

	case SG_DAGQUERY_FIND_HEAD_STATUS__MULTIPLE:
		// _MULTIPLE_HEADS_FROM_DAGNODE and _BRANCH_NEEDS_MERGE
		// are basically equivalent errors.
		SG_ERR_THROW2(  SG_ERR_MULTIPLE_HEADS_FROM_DAGNODE,
						(pCtx, "Cannot automatically assume goal changeset for update")  );
		break;

	default:
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Unknown dagquery status [dqfhs %d]", dqfhs)  );
		break;
	}

	// duplicate the essense of _compute_target_hid_from_rev_spec()
	// because we already know all of the answers.
	//
	// pretend like they said '--rev hid_goal'.
	// we know that the target is a leaf, but not necessarily a head.

	SG_ERR_CHECK(  SG_STRDUP(pCtx, bufHidGoal, &pUpdateData->pszHidTarget)  );
	pUpdateData->bRequestedByRev = SG_TRUE;

done:
	;
fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpecPrivate);
}

static void _check_attach_name(SG_context * pCtx,
							   sg_update_data * pUpdateData)
{
	if (pUpdateData->pUpdateArgs->pszAttach)
	{
		SG_ERR_CHECK(  SG_vc_branches__check_attach_name(pCtx,
														 pUpdateData->pWcTx->pDb->pRepo,
														 pUpdateData->pUpdateArgs->pszAttach,
														 SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_EXIST,
														 SG_FALSE, // do not validate existing name
														 &pUpdateData->pszNormalizedAttach)  );
	}
	else if (pUpdateData->pUpdateArgs->pszAttachNew)
	{
		SG_ERR_CHECK(  SG_vc_branches__check_attach_name(pCtx,
														 pUpdateData->pWcTx->pDb->pRepo,
														 pUpdateData->pUpdateArgs->pszAttachNew,
														 SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_NOT_EXIST,
														 SG_TRUE,  // validate new name
														 &pUpdateData->pszNormalizedAttachNew)  );
	}
	
fail:
	return;
}

static void _choose_attach_detach_for_r_or_t(SG_context * pCtx,
											 sg_update_data * pUpdateData)
{
	// They requested a specific changeset by either -r or -t
	// so we don't know the relationship between the changeset
	// and any branch.
	//
	// Our task here is to decide what branch (if any) the WD
	// should be attached to after we finish the update.

	if (pUpdateData->pUpdateArgs->bDetached)
	{
		// we don't know anything about whether the chosen
		// changeset is a head, but we don't care because
		// the user wants to go rogue.

		pUpdateData->pszAttachChosen = NULL;
		pUpdateData->bAttachNewChosen = SG_FALSE;
		goto done;
	}
	else if (pUpdateData->pUpdateArgs->bAttachCurrent)
	{
		if (pUpdateData->pszBranchName_Starting && *pUpdateData->pszBranchName_Starting)
			SG_ERR_CHECK(  SG_STRDUP(pCtx,
									 pUpdateData->pszBranchName_Starting,
									 &pUpdateData->pszAttachChosen)  );
		else
			pUpdateData->pszAttachChosen = NULL;
		pUpdateData->bAttachNewChosen = SG_FALSE;
		goto done;
	}
	else if (pUpdateData->pszNormalizedAttach)
	{
		// we don't know anything about whether the chosen
		// changeset is a head, but we don't care because
		// the user wants to force the attach to a specific
		// EXISTING branch.

		SG_ERR_CHECK(  SG_STRDUP(pCtx,
								 pUpdateData->pszNormalizedAttach,
								 &pUpdateData->pszAttachChosen)  );
		pUpdateData->bAttachNewChosen = SG_FALSE;
		goto done;
	}
	else if (pUpdateData->pszNormalizedAttachNew)
	{
		// we don't know anything about whether the chosen
		// changeset is a head, but we don't care because
		// the user wants to force the attach to a specific
		// NEW branch.

		SG_ERR_CHECK(  SG_STRDUP(pCtx,
								 pUpdateData->pszNormalizedAttachNew,
								 &pUpdateData->pszAttachChosen)  );
		pUpdateData->bAttachNewChosen = SG_TRUE;
		goto done;
	}
	else
	{
		// We were not given any attach/detach instructions.
		// Try to infer them from what we know about the
		// target changeset and the WD.

		SG_vhash * pvhValues;		// we do not own this
		SG_vhash * pvhValuesHid;		// we do not own this
		SG_uint32 nrValuesHidNames;
		SG_bool bHasValuesHid;
		SG_bool bHasValuesHidName;

		// See if this changeset is a head.  Note that a
		// changeset can be the head of more than one branch.
		// 
		// If the target changeset IS a head, WE WANT TO
		// AUTOMATICALLY/IMPLICITLY ATTACH THE WD TO IT.
		// 
		// If the changeset is the head of multiple branches,
		// try to default to the currently attached branch.
		//
		// The pile looks something like this:
		// 
		// var r = sg.open_local_repo();
		// var b = r.named_branches();
		//
		// b.branches.<name>.records.<recid> == <hid>
		// b.branches.<name>.values.<hid> == <recid>
		// b.values.<hid>.<name>.<recid> == null
		// b.closed....
		//
		// We only care about the existence of 'b.values.<hid>'
		// and 'b.values.<hid>.<name>' and how many <names>
		// that 'b.values.<hid>' contains.

		if (!pUpdateData->pvhPile)
			SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx,
												   pUpdateData->pWcTx->pDb->pRepo,
												   &pUpdateData->pvhPile)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pUpdateData->pvhPile, "values", &pvhValues)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhValues, pUpdateData->pszHidChosen, &bHasValuesHid)  );
		if (bHasValuesHid)	// b.values.<hid> exists: so the target cset is the head of one or more branches.
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhValues, pUpdateData->pszHidChosen, &pvhValuesHid)  );
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhValuesHid, &nrValuesHidNames)  );	// how many <names> are in b.values.<hid>
			if (nrValuesHidNames > 1)						// The changeset is the head of multiple branches.
			{
				if (pUpdateData->pszBranchName_Starting) 	// If the current branch is one of them, prefer it.
				{
					SG_ERR_CHECK(  SG_vhash__has(pCtx,
												 pvhValuesHid,
												 pUpdateData->pszBranchName_Starting,
												 &bHasValuesHidName)  );
					if (bHasValuesHidName)					// Preserve the attachment to the current branch.
					{
						SG_ERR_CHECK(  SG_STRDUP(pCtx,
												 pUpdateData->pszBranchName_Starting,
												 &pUpdateData->pszAttachChosen)  );
						pUpdateData->bAttachNewChosen = SG_FALSE;
						goto done;
					}
					else
					{
						SG_ERR_THROW2(  SG_ERR_MUST_ATTACH_OR_DETACH,
										(pCtx, "The target changeset '%s' is the head of multiple branches (unrelated to the current branch '%s').",
										 pUpdateData->pszHidChosen,
										 pUpdateData->pszBranchName_Starting)  );
					}
				}
				else
				{
					SG_ERR_THROW2(  SG_ERR_MUST_ATTACH_OR_DETACH,
									(pCtx, "The target changeset '%s' is the head of multiple branches.",
									 pUpdateData->pszHidChosen)  );
				}
			}
			else // (nrValuesHidNames == 1)		// The target changeset is the head of exactly one branch.
			{
				const char * pszBranch_0;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhValuesHid, 0, &pszBranch_0, NULL)  );
				SG_ERR_CHECK(  SG_STRDUP(pCtx, pszBranch_0, &pUpdateData->pszAttachChosen)  );
				pUpdateData->bAttachNewChosen = SG_FALSE;
				goto done;
			}
		}
		else				// The target changeset is NOT a head.  We want the WD to be detached.
		{
			if (pUpdateData->pszBranchName_Starting)
			{
				// Detaching a tied WD is not automatic.  (We want them to confirm this.)
				// 
				// We want to be detached here because a future checkin relative to this
				// target changeset may or may not have anything to do with the starting
				// branch that *just happened* to be associated with the WD.

				SG_ERR_THROW2(  SG_ERR_MUST_ATTACH_OR_DETACH,
								(pCtx, "The target changeset '%s' is not a head; you must explicitly attach/detach the working copy.",
								 pUpdateData->pszHidChosen)  );
			}
			else
			{
				// If the WD is already detached, we can continue detached.

				pUpdateData->pszAttachChosen = NULL;
				pUpdateData->bAttachNewChosen = SG_FALSE;
				goto done;
			}
		}
	}

done:
	;
fail:
	return;
}

static void _choose_attach_detach_for_b(SG_context * pCtx,
										sg_update_data * pUpdateData)
{
	// They requested an update to a specific branch.
	// And we have determined that that branch has a
	// single/unique head.
	//
	// Our task here is to decide what branch (if any)
	// the WD should be attached to after we finish the
	// update.
	//
	// That is, the '--branch name' helped us choose the
	// target changeset, but that doesn't necessarily
	// mean that they want the WD attached to it.

	if (pUpdateData->pUpdateArgs->bDetached)
	{
		pUpdateData->pszAttachChosen = NULL;
		pUpdateData->bAttachNewChosen = SG_FALSE;
		goto done;
	}
	else if (pUpdateData->pUpdateArgs->bAttachCurrent)
	{
		if (pUpdateData->pszBranchName_Starting && *pUpdateData->pszBranchName_Starting)
			SG_ERR_CHECK(  SG_STRDUP(pCtx,
									 pUpdateData->pszBranchName_Starting,
									 &pUpdateData->pszAttachChosen)  );
		else
			pUpdateData->pszAttachChosen = NULL;
		pUpdateData->bAttachNewChosen = SG_FALSE;
		goto done;
	}
	else if (pUpdateData->pszNormalizedAttach)
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx,
								 pUpdateData->pszNormalizedAttach,
								 &pUpdateData->pszAttachChosen)  );
		pUpdateData->bAttachNewChosen = SG_FALSE;
		goto done;
	}
	else if (pUpdateData->pszNormalizedAttachNew)
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx,
								 pUpdateData->pszNormalizedAttachNew,
								 &pUpdateData->pszAttachChosen)  );
		pUpdateData->bAttachNewChosen = SG_TRUE;
		goto done;
	}
	else // otherwise, assume the value from '--branch name'
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pUpdateData->pszBranchNameTarget, &pUpdateData->pszAttachChosen)  );
		pUpdateData->bAttachNewChosen = SG_FALSE;
		goto done;
	}

done:
	;
fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Use the (optional) arguments given by the user and/or the
 * current state of the WD and/or branch info to figure out
 * what the target cset should be and/or what branch we should
 * optionally attach to afterwards.
 *
 * We collect a lot of intermediate info, but the final values
 * are all named "...Chosen".
 *
 */
void sg_wc_tx__update__determine_target_cset(SG_context * pCtx,
											 sg_update_data * pUpdateData)
{
	SG_ERR_CHECK(  _get_starting_conditions(pCtx, pUpdateData)  );

	SG_ERR_CHECK(  _validate_basic_args(pCtx, pUpdateData)  );
	SG_ERR_CHECK(  _check_attach_name(pCtx, pUpdateData)  );
	SG_ERR_CHECK(  _compute_target_hid(pCtx, pUpdateData)  );

	if (pUpdateData->bRequestedByRev)
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pUpdateData->pszHidTarget, &pUpdateData->pszHidChosen)  );
		SG_ERR_CHECK(  _choose_attach_detach_for_r_or_t(pCtx, pUpdateData)  );
	}
	else if (pUpdateData->bRequestedByBranch)
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pUpdateData->pszHidTarget, &pUpdateData->pszHidChosen)  );
		SG_ERR_CHECK(  _choose_attach_detach_for_b(pCtx, pUpdateData)  );
	}
	else 
	{
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pUpdateData->pRevSpecChosen)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pUpdateData->pRevSpecChosen, pUpdateData->pszHidChosen)  );

#if TRACE_WC_UPDATE
	if (pUpdateData->pszAttachChosen)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Update: target   (%s, %s, %s).\n",
								   pUpdateData->pszHidChosen,
								   pUpdateData->pszAttachChosen,
								   ((pUpdateData->bAttachNewChosen) ? "attach-new" : "attach"))  );
	else
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Update: target   (%s, detached).\n",
								   pUpdateData->pszHidChosen)  );
#endif

fail:
	return;
}
