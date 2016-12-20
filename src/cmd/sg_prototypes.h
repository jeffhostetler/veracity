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
 * @file sg_prototypes.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_PROTOTYPES_H
#define H_SG_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_report_journal(SG_context * pCtx, const SG_varray * pva);

//////////////////////////////////////////////////////////////////

/**
 * vv2__ prefix indicates that I have converted it to use/respect the "vv2"
 * version of "vv_verbs" and/or the WC re-write of PendingTree and/or that
 * the routine doesn't properly belong in the 'wc' API directly.
 *
 * wc__ prefix indicates that I have converted it to use the
 * WC re-write of PendingTree *and* the routine logically
 * belongs in the 'wc' API.
 *
 */

//////////////////////////////////////////////////////////////////

void vv2__do_cmd_cat(SG_context * pCtx,
					 const SG_option_state * pOptSt,
					 const SG_stringarray * psaArgs);

void vv2__do_cmd_comment(SG_context * pCtx,
						 const SG_option_state * pOptSt);

void vv2__do_cmd_init_new_repo(SG_context * pCtx,
							   const char * pszRepoName,
							   const char * pszFolderName,
							   const char * pszStorage,
							   const char * pszHashMethod,
							   const char * psz_shared_users);

void vv2__do_cmd_export(SG_context * pCtx,
						const char * pszRepoName,
						const char * pszFolder,
						SG_rev_spec* pRevSpec,
						const SG_varray * pvaSparse);

void vv2__do_cmd_leaves(SG_context * pCtx, 
						SG_option_state * pOptSt);

void vv2__do_cmd_locks(SG_context * pCtx, 
					   SG_option_state * pOptSt,
					   const char * psz_username_for_pull,
					   const char * psz_password_for_pull);

void vv2__do_cmd_stamp__subcommand(SG_context * pCtx,
								   const SG_option_state * pOptSt,
								   SG_uint32 argc,
								   const char ** argv);

void vv2__do_cmd_stamps(SG_context * pCtx,
						const SG_option_state * pOptSt);

void vv2__do_cmd_tags(SG_context * pCtx,
					  const SG_option_state * pOptSt);

void vv2__do_cmd_tag__subcommand(SG_context * pCtx,
								 const SG_option_state * pOptSt,
								 SG_uint32 argc,
								 const char ** argv);

//////////////////////////////////////////////////////////////////

void vvwc__do_cmd_status(SG_context * pCtx,
						 const SG_option_state * pOptSt,
						 const SG_stringarray * psaArgs);

void vvwc__do_cmd_mstatus(SG_context * pCtx,
						  const SG_option_state * pOptSt);

void vvwc__do_cmd_diff(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const SG_stringarray * psaArgs,
					   SG_uint32 * pNrErrors);

//////////////////////////////////////////////////////////////////

void vv2__do_cmd_history(SG_context * pCtx, 
						 const SG_option_state * pOptSt,
						 const SG_stringarray * psaArgs,
						 SG_bool b_graph);

//////////////////////////////////////////////////////////////////

void do_cmd_fast_import(SG_context * pCtx,
    SG_option_state * pOptSt,
	const char * psz_fi,
	const char * psz_repo
	);

void do_cmd_fast_export(SG_context * pCtx,
    SG_option_state * pOptSt,
	const char * psz_repo,
	const char * psz_fi
	);

void do_cmd_merge_preview(SG_context * pCtx, SG_option_state * pOptSt);

void do_cmd_resolve2(SG_context * pCtx,
					SG_option_state * pOptSt,
					SG_uint32 count_args,
					const char ** paszArgs);

void do_cmd_dump_lca(SG_context * pCtx,
					 SG_option_state * pOptSt);

void do_cmd_serve(SG_context * pCtx, 
				  SG_option_state* pOptSt,
				  int * pExitStatus);

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

/**
 * Traditionally, we have used a -N flag to indicate that
 * a command should act non-recursive rather than the default
 * of acting recursively.
 *
 * In the WC re-write of PendingTree, I made this a "depth"
 * argment.  Depth 0 means operate on the item only.  Depth
 * 1 means the item and if it is a directory the immediate
 * children.  And so on.
 *
 * This routine is to hide the mapping from flag to depth.
 * (I'd like to change the CL syntax to take it instead,
 * but not today.)
 *
 */
#define WC__GET_DEPTH(pOptSt)	(((pOptSt)->bRecursive) ? SG_INT32_MAX : 0)

/**
 * Historically, the value of the --no-ignores flag has been
 * hidden inside the pOptSt->pFilespec as part of the other
 * --include/--exclude args.
 *
 * As part of the WC re-write of PendingTree, I'm removing
 * the --include/--exclude args and I'm moving the no-ignores
 * flag to be a plain bool inside pOptSt.
 *
 * This routine is to hide that until the conversion is
 * completed.
 * 
 */
#define WC__GET_NO_IGNORES(pOptSt)		((pOptSt)->bNoIgnores)

//////////////////////////////////////////////////////////////////

void wc__do_cmd_add(SG_context * pCtx,
					const SG_option_state * pOptSt,
					const SG_stringarray * psaArgs);

void wc__do_cmd_addremove(SG_context * pCtx,
						  const SG_option_state * pOptSt,
						  const SG_stringarray * psaArgs);

void wc__do_cmd_checkout(SG_context * pCtx,
						 const char * pszRepoName,
						 const char * pszPath,
						 /*const*/ SG_rev_spec * pRevSpec,
						 const char * pszAttach,
						 const char * pszAttachNew,
						 const SG_varray * pvaSparse);

void wc__do_cmd_commit(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const SG_stringarray * psaArgs);

void wc__do_cmd_diffmerge(SG_context * pCtx,
						  SG_option_state * pOptSt);

void wc__do_cmd_lock(SG_context * pCtx,
					 const SG_option_state * pOptSt,
					 const SG_stringarray * psaArgs,
					 const char* psz_username,
					 const char* psz_password);

void wc__do_cmd_merge(SG_context * pCtx,
					  const SG_option_state * pOptSt);

void wc__do_cmd_move(SG_context * pCtx,
					 const SG_option_state * pOptSt,
					 const SG_stringarray * psaItemsToMove,
					 const char* pszPathMoveTo);

void wc__do_cmd_parents(SG_context * pCtx,
						const SG_option_state * pOptSt);

void wc__do_cmd_remove(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const SG_stringarray * psaArgs);

void wc__do_cmd_rename(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const char* pszPath, const char* pszNewName);

void wc__do_cmd_revert_all(SG_context * pCtx,
						   const SG_option_state * pOptSt,
						   const SG_stringarray * psaArgs);

void wc__do_cmd_revert_items(SG_context * pCtx,
							 const SG_option_state * pOptSt,
							 const SG_stringarray * psaArgs);

void wc__do_cmd_scan(SG_context * pCtx,
					 const SG_option_state * pOptSt);

void wc__do_cmd_unlock(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const SG_stringarray * psaArgs,
					   const char* psz_username,
					   const char* psz_password);

void wc__do_cmd_update(SG_context * pCtx,
					   const SG_option_state * pOptSt);

void wc__do_cmd_update__after_pull(SG_context * pCtx,
								   SG_rev_spec * pRevSpec);

//////////////////////////////////////////////////////////////////

void _get_zero_one_or_two_revision_specifiers(SG_context* pCtx,
											  const char* pszRepoName,
											  SG_rev_spec* pRevSpec,
											  char** ppszHid1,
											  char** ppszHid2);

//////////////////////////////////////////////////////////////////

void my_wrap_text(
	SG_context* pCtx,
	SG_string* pstr,
	SG_uint32 width,
	SG_string** ppstr
	);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_PROTOTYPES_H
