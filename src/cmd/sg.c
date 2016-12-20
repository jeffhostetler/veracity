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

/*
  vv

  The Veracity command-line application
*/

#if defined(WINDOWS)
#include <io.h>
#endif
#include <signal.h>
#include <sg.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_wc__public_typedefs.h>

#include <sg_vv2__public_prototypes.h>
#include <sg_wc__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"
#include "sg_help_source.h"

//////////////////////////////////////////////////////////////////
// The --sparse option on CHECKOUT technically works as of 2.5
// (and I want to be able to use it in testing)
// but we want to disable/hide it for now in production releases
// because we need some other stuff to round out the feature --
// such as a way to later back-fill (populate) a WD with items
// that were originally omitted.  See also sg.wc.checkout({})
// in src/libraries/wc/wc9js/sg_wc__jsglue__sg_wc.c
// 
#if defined(DEBUG)
#  define FEATURE_SPARSE_CHECKOUT 1
#else
#  define FEATURE_SPARSE_CHECKOUT 0
#endif
//////////////////////////////////////////////////////////////////

/* For every command, provide a forward declaration of the cmd func
 * here, so that its function pointer can be included in the lookup
 * table. */

DECLARE_CMD_FUNC(add);
DECLARE_CMD_FUNC(addremove);
DECLARE_CMD_FUNC(lock);
DECLARE_CMD_FUNC(unlock);
DECLARE_CMD_FUNC(locks);
DECLARE_CMD_FUNC(branch);
DECLARE_CMD_FUNC(cat);
DECLARE_CMD_FUNC(checkout);
DECLARE_CMD_FUNC(export);
DECLARE_CMD_FUNC(fast_import);
DECLARE_CMD_FUNC(fast_export);
DECLARE_CMD_FUNC(svn_load);
DECLARE_CMD_FUNC(zip);
DECLARE_CMD_FUNC(clone);
DECLARE_CMD_FUNC(user);
#if 0
DECLARE_CMD_FUNC(creategroup);
DECLARE_CMD_FUNC(groups);
DECLARE_CMD_FUNC(group);
#endif
DECLARE_CMD_FUNC(log);
DECLARE_CMD_FUNC(comment);
DECLARE_CMD_FUNC(commit);
DECLARE_CMD_FUNC(diff);
DECLARE_CMD_FUNC(diffmerge);
DECLARE_CMD_FUNC(forget_passwords);
DECLARE_CMD_FUNC(heads);
DECLARE_CMD_FUNC(leaves);
DECLARE_CMD_FUNC(help);
DECLARE_CMD_FUNC(history);
DECLARE_CMD_FUNC(incoming);
DECLARE_CMD_FUNC(localsetting);
DECLARE_CMD_FUNC(localsettings);
DECLARE_CMD_FUNC(merge);
DECLARE_CMD_FUNC(merge_preview);
DECLARE_CMD_FUNC(move);
DECLARE_CMD_FUNC(init_new_repo);
DECLARE_CMD_FUNC(outgoing);
DECLARE_CMD_FUNC(parents);
DECLARE_CMD_FUNC(pull);
DECLARE_CMD_FUNC(push);
DECLARE_CMD_FUNC(reindex);
DECLARE_CMD_FUNC(remove);
DECLARE_CMD_FUNC(rename);
DECLARE_CMD_FUNC(repo);
DECLARE_CMD_FUNC(resolve2);
DECLARE_CMD_FUNC(revert);
DECLARE_CMD_FUNC(scan);
DECLARE_CMD_FUNC(serve);
DECLARE_CMD_FUNC(stamp);
DECLARE_CMD_FUNC(status);
DECLARE_CMD_FUNC(mstatus);
DECLARE_CMD_FUNC(tag);
DECLARE_CMD_FUNC(update);
DECLARE_CMD_FUNC(upgrade);
DECLARE_CMD_FUNC(version);
DECLARE_CMD_FUNC(whoami);

//advanced commands
DECLARE_CMD_FUNC(linehistory);
DECLARE_CMD_FUNC(zingmerge);
DECLARE_CMD_FUNC(blobcount);
DECLARE_CMD_FUNC(dump_json);
DECLARE_CMD_FUNC(graph_history);
DECLARE_CMD_FUNC(dump_lca);
#ifdef DEBUG
DECLARE_CMD_FUNC(dump_treenode);
#endif
DECLARE_CMD_FUNC(hid);
DECLARE_CMD_FUNC(vcdiff);
DECLARE_CMD_FUNC(check_integrity);

static void _sg_option_state__free(SG_context* pCtx, SG_option_state* pThis);

#if 1 && defined(DEBUG)
#define HAVE_TEST_CRASH
DECLARE_CMD_FUNC(crash);
#endif

struct sg_getopt_option sg_cl_options[] =
{                                                                                           //break point for 80-column display
	// word-wrapping is done automatically now for command and option descriptions, so hard tabs and newlines should be avoided.
	{"all",          opt_all,      0, "Include all files and folders recursively"},
	{"all-but",      opt_all_but,  0, "Include all items except for those listed"},
	{"all-commands", opt_show_all, 0, "Include normally hidden commands used primarily for debugging"},
	{"all-full",     opt_all_full, 0, "Change all blobs to full encoding"},
	{"all-zlib",     opt_all_zlib, 0, "Change all blobs to zlib encoding"},
	{"area",         opt_area,     1, "Specify which area of repo data to push or pull"},
	{"assoc",        'a',          1, "Associate work item"},
	{"attach",       opt_attach,   1, "Attach the working copy"},
	{"attach-new",   opt_attach_new, 1, "Attach the working copy to a newly created branch"},
	{"attach-current",  'c',       0, "Use currently attached branch or detached state when updating away from a head"},
	{"branch",       'b',          1, "Use the changeset indicated by the head of this branch"},
	{"cert",         opt_cert,     1, "Full or relative path of certificate (.PEM) file to use in SSL encryption. Defaults to ./ssl_cert.pem"},
	{"clean",        'C',          0, "Replace the working copy with the baseline repository version. "},
	{"confirm",      opt_confirm,  0, "Yes, I'm sure, go ahead and do it"},
	{"create",       opt_create,   0, "Add the user if it does not exist"},
	{"dest",         opt_dest,     1, "Pull into the specified repository"},
	{"detached",     opt_detached, 0, "<<generic force message not defined>>"},
	{"force",        'F',          0, "<<generic force message not defined>>"},
	{"from",         opt_from,     1, "Earliest date included in the history query"},
	{"global",       opt_global,   0, "Apply change to the Machine Default of the setting (in addition to the current working copy, if applicable)"},
	{"hash",         opt_hash,     1, "Hash algorithm to use for the new repository; valid values are: SHA1/160 (default), SHA2/256, SHA2/384, SHA2/512, SKEIN/256, and SKEIN/512"},
	{"hide-merges",  opt_hide_merges,   0, "Do not include merge changesets where a file or folder is identical to the version in one of the parent changesets; requires a file or folder argument"},
	{"interactive" , 'i',          0, "<<generic interactive message not defined>>"},
	{"keep",         'k',          0, "<<generic keep message not defiend>>"},
	{"label",        opt_label,    1, "Use the specified label"},
	{"leaves",       opt_leaves,   0, "Include history starting with all leaf nodes in the DAG"},
	{"length",       opt_length,   1, "The number of lines to consider for line history.  Length must be greater than zero"},
	{"list",         opt_list,     0, "List one or more issues or all unresolved issues"},
	{"list-all",     opt_list_all, 0, "<<generic list-all message not defined>>"},
	{"list-unchanged", opt_list_unchanged, 0, "<<generic list-unchanged message not defined>>"},
	{"max",          opt_max,      1, "Maximum number of results to include in the history query"},
	{"message",      'm',          1, "Add changeset comment"},
	{"no-auto-merge",opt_no_auto_merge, 0, "Do not auto-merge file content"},
	{"no-backups",   opt_no_backups, 0, "<<generic no-backups message not defined>>"},
	{"no-fallback",  opt_no_fallback, 0, "<<generic no-fallback message not defined>>"},
	{"no-ff",        opt_no_ff_merge, 0, "Do not attempt fast-forward merges"},
	{"no-ignores",   opt_no_ignores, 0, "Include files that would otherwise be ignored"},
	{"no-plain",     opt_no_plain, 0, "Serve encrypted content only"},
	{"no-prompt",    opt_no_prompt,0, "Bypass confirmation prompt"},
	{"nonrecursive", 'N',          0, "Do not recurse subdirectories"},
	{"output",       'o',          1, "Write output to the given file"},
	{"overwrite",    opt_overwrite, 0, "Overwrite something that already exists"},
	{"pack",         opt_pack,     0, "Compress the repository using vcdiff and zlib where appropriate"},
	{"path",         opt_path,     1, "The path to use"},
	{"port",         opt_port,     1, "Port to use for unencrypted content; defaults to 8080"},
	{"public",       'P',          0, "Allow remote connections"},
	{"quiet",        opt_quiet,    0, "Show less detailed output" },
	{"pull",         opt_pull,     0, "Refresh the list of locks from the upstream repository instance"},
	{"remember",    opt_remember,  0, "Remember the password (on supported platforms)"},
	{"remove",       opt_remove,   0, "Remove, rather than add"},
	{"repo",         opt_repo,     1, "The repository descriptor name"},
	{"restore-backup",         opt_restore_backup,     1, "The path to the backup closet"},
	{"rev",          'r',          1, "Use the changeset indicated by this revision identifier"},
	{"reverse",		 opt_reverse,  0, "Reverse the order of the history output (this does not change which revisions are returned, only the order in which they are displayed)"},
	{"server",       opt_server,   1, "Specify the repository instance to use when synchronizing locks"},
	{"shared-users", opt_shared_users, 1, "New repository should share user accounts with the given existing repository instance"},
	{"sparse",       opt_sparse,   1, "<<generic sparse message not defined>>"},
	{"sport",        opt_sport,    1, "The port to use for encrypted content. Must not be the same as the implicit or explicit value of --port"},
	{"src",          opt_src,      1, "Push from the specified repository"},
	{"src-comment",  opt_src_comment, 0, "Add a comment containing the changeset's source." },
	{"stamp",        opt_stamp,    1, "List only changesets indicated by the given stamp"},
	{"startline",       opt_startline,1, "The first line to include in line history"},
	{"status",       opt_status,   0, "<<generic status message not defined>>"},
	{"storage",      opt_storage,  1, "The storage technology to use for the new repository" },
	{"subgroups",    opt_subgroups,0, "The members to be added are names of subgroups rather than names of users"},
	{"tag",          opt_tag,      1, "Use the changeset indicated by this tag"},
	{"test",         'T',          0, "Do not pend any actions; display details about actions that would be pended"},
	{"to",           opt_to,       1, "Latest date included in the history query"},
	{"tool",         opt_tool,     1, "<<generic tool message not defined>>"},
	{"trace",        opt_trace,    0, "Print verboser-than-verbose output for debugging and diagnostics. (debug)"},
	{"update",       'u',          0, "Update the working copy after the pull"},
	{"user",         'U',          1, "List only changesets committed by this user"},
	{"verbose",      opt_verbose,  0, "Include full text of multi-line comments"},
	{"when",         opt_when,     1, "Specify when these changes happened"},
	{"allow-lost",   opt_allow_lost, 0, "Allow commit to try to cope with lost items"},
	{"no-allow-after",  opt_no_allow_after_the_fact, 0, "Disallow after-the-fact moves/renames"},
	{"allow-dirty",  opt_allow_dirty, 0, "<<generic status message not defined>>"},

#if defined(DEBUG)
	{"dagnum",       opt_dagnum,   1, "Request a specific DAG (debug)"},
#endif

	{"revert-files",        opt_revert_files,        0, "Limit revert to files"},
	{"revert-symlinks",     opt_revert_symlinks,     0, "Limit revert to symlinks"},
	{"revert-directories",  opt_revert_directories,  0, "Limit revert to directories"},
	{"revert-lost",         opt_revert_lost,         0, "Limit revert to restoring lost items"},
	{"revert-added",        opt_revert_added,        0, "Limit revert to undo adds"},
	{"revert-removed",      opt_revert_removed,      0, "Limit revert to undo removes"},
	{"revert-renamed",      opt_revert_renamed,      0, "Limit revert to undo renames"},
	{"revert-moved",        opt_revert_moved,        0, "Limit revert to undo moves"},
	{"revert-merge-added",  opt_revert_merge_added,  0, "Limit revert to items added by merge"},
	{"revert-update-added", opt_revert_update_added, 0, "Limit revert to items added by update"},
	{"revert-attributes",   opt_revert_attributes,   0, "Limit revert to undo attribute changes"},
	{"revert-content",      opt_revert_content,      0, "Limit revert to undo edits to files/symlinks"},

	{0,               0, 0, 0},
};

/* Now the lookup table. */

struct cmdinfo commands[] =
{
	{
		"add", NULL, NULL, NULL, cmd_add,	// requires 1 or more file-or-folder args
		"Pend the addition of a file or directory to the current repository", "file-or-folder [file-or-folder ...]", NULL,
		SG_FALSE, 
		{
			{'N',0,0},
			{'T',0,0},
			{opt_no_ignores,0,0},
			{opt_verbose,0,0}
		}, 
		{
			{'T', "Determine whether the 'add' could be performed, but do not actually do it"},
			{opt_verbose, "List items that were or would be added"}
		}
	},

	{
		"addremove", NULL, NULL, NULL, cmd_addremove,	// does not require any file-or-folder args
		"Pend additions for all Found files and removals for all Lost files", "[file-or-folder ...]", NULL,
		SG_FALSE, 
		{
			{'N',0,0},				// supported by plain "ADD"
			{'T',0,0},				// supported by both "ADD" and "REMOVE"
			{opt_no_ignores,0,0},	// supported by plain "ADD"
			{opt_verbose,0,0}		// supported by both "ADD" and "REMOVE"
			// since we only remove lost items, we don't need:
			// --force, --no-backups, --keep
		}, 
		{
			{'T', "Do not pend any additions or removals; display details about what would be pended"},
			{opt_verbose, "List the items were or would be added or removed"}
		}
	},

	{
		"blobcount", NULL, NULL, NULL, cmd_blobcount,
		"Show the number of blobs in a repo", "repo_name", NULL,
		SG_TRUE, 
		{ {0,0,0} }, 
		{ {0, ""} }
	},

	{
		"branch", "branches", NULL, NULL, cmd_branch,
		"Create, delete, manipulate or list named branches", 
		"[subcommand [branchname]] [OPTIONS] ", &vv_help_branch,
		SG_FALSE, { {opt_all,0,1}, {OPT_REV_SPEC,0,0}, {opt_repo,0,1} }, 
		{
			{opt_all, "Include all heads when moving; include closed branches when listing"},
			{'b', "Use the changeset(s) indicated by the head(s) of this branch"}
		}
	},

	{
		"cat", "type", NULL, NULL, cmd_cat,
		"Unix-style cat on a repository file",
		"[--repo=REPOSITORY] [--rev=CHANGESETID | --tag=TAG | --branch=BRANCH] file [...]", &vv_help_cat,
		SG_FALSE, 
		{
			{opt_repo,0,1}, {OPT_REV_SPEC,0,1},
			{'o',0,1}
		}, 
		{ 
			{'b', "Use the changeset indicated by the head of this branch; it is an error if the branch has multiple heads"},
			{'o', "Write output to the given file. On Windows, output redirection is unsupported. Use this instead."}
		}
	},

	{
		"verify", "check_integrity", NULL, NULL, cmd_check_integrity,
		"Check repository integrity", 
		"subcommand [OPTIONS] ", &vv_help_verify,
		SG_TRUE, 
		{ {opt_repo,1,1}, {OPT_REV_SPEC,0,1} },
		{ 
			{'b', "For verify tree_deltas, verify the head changeset of this branch"},
			{'r', "For verify tree_deltas, verify the specified changeset"},
			{opt_tag, "For verify tree_deltas, verify the changeset indicated by this tag"}
		}
	},

	{
		"checkout", NULL, NULL, NULL, cmd_checkout,
		"Create a new working copy of a repository version", "OPTIONS REPOSITORY [path]", NULL,
		SG_FALSE, 
		{ {OPT_REV_SPEC,0,1},
		  {opt_attach,0,1},
		  {opt_attach_new,0,1}
#if FEATURE_SPARSE_CHECKOUT
		  ,{opt_sparse,0,0}
#endif
		}, 
		{
			{opt_attach, "Attach the working copy to the specified branch"},
			{opt_attach_new, "Attach the working copy to a new branch"}
#if FEATURE_SPARSE_CHECKOUT
			,{opt_sparse, "Do sparse checkout; omit this item from working directory (may be repeated)"}
#endif
		}
	},

	{
		"fast-import", NULL, NULL, NULL, cmd_fast_import,
		"Import from a fast-import stream", "OPTIONS import_file.fi REPOSITORY", NULL,
		SG_FALSE, 
		{ {opt_shared_users,0,1}, {opt_hash,0,1} },
		{ {0, ""} }
	},

	{
		"fast-export", NULL, NULL, NULL, cmd_fast_export,
		"Export a repository as a fast-import stream", "REPOSITORY export_file.fi", NULL,
		SG_FALSE, 
		{ {0,0,0} }, 
		{ {0, ""} }
	},

	{
		"export", NULL, NULL, NULL, cmd_export,
		"Export a repository version to a non-version-controlled directory", "OPTIONS REPOSITORY [path]", NULL,
		SG_FALSE, 
		{ {OPT_REV_SPEC,0,1},
		  {opt_sparse,0,0}
		},
		{ {opt_sparse, "Do sparse export; omit this item from the result (may be repeated)"} 
		}
	},

	{
		"clone", NULL, NULL, NULL, cmd_clone,
		"Create a new instance of an existing repository", "existing_repo_name new_repo_name", NULL,
		SG_FALSE, 
		{
			{opt_all_full,0,0}, {opt_all_zlib,0,0}, {opt_pack,0,0}, {opt_remember,0,0}
		},
		{ {0, ""} }
	},

	{
		"config", "localsettings", "localsetting", NULL, cmd_localsettings,
		"View and manipulate configuration settings", "[subcommand [...]]", &vv_help_config,
		SG_FALSE, 
		{ {'F',0,0}, {opt_verbose,0,0} },
		{
			{'F', "Allow specification of settings in unknown scopes"},
			{opt_verbose, "Include factory default settings in output"}
		}
	},

	{
		"comment", NULL, NULL, NULL, cmd_comment,
		"Add a comment to a changeset", "(--rev | --tag | --branch) [--message]", NULL,
		SG_FALSE, 
		{
			{opt_repo,0,1},
			{OPT_REV_SPEC,1,1},
			{'m',0,1},
			{'U',0,1},
			{opt_when,0,1}
		}, 
		{
			{'U', "The user for this comment; required with --repo"}
		}
	},

	{
		"commit", NULL, NULL, NULL, cmd_commit,	// does not require any file-or-folder args
		"Commit the pending changeset to the repository", "[file-or-folder ...]", &vv_help_commit,
		SG_FALSE, 
		{ 
			{'m',0,1},
			{opt_detached,0,0},
			{opt_stamp,0,0},
			{'a',0,0},
			{'N',0,0},
			{'U',0,1},
			{opt_when,0,1},
			{'T',0,0},
			{opt_verbose,0,0},
			{opt_allow_lost,0,0}
		},
		{
			{'m', "Changeset comment.  If omitted, an editor will be started to let you enter one."},
			{opt_detached, "Allow the commit even though the working copy is not attached to a branch"},
			{opt_stamp, "Stamp(s) to be added"},
			//'N' default message is ok
			{'U', "Override the current user setting for this commit"},
			//opt_when default message is ok
			{'T', "Do not commit; display details about what would be committed"},
			{opt_verbose, "List the items that were or would be committed"}
			//opt_allow_lost default message is ok
		}
	},

	{
		"diff", NULL, NULL, NULL, cmd_diff,
		// TODO 2012/04/30 Move the big FROM/TO stuff to extended help.
		"Show differences between versions of files or folders",
		"[OPTIONS] [file_or_folder [...]] [FROM_REVSPEC [TO_REVSPEC]]", NULL,
		SG_FALSE, 
		{
			{opt_repo,0,1},
			{OPT_REV_SPEC,0,2},
			{'i',0,0},
			{opt_tool,0,1},
			{'N',0,0}
		},
		{
			{'i', "Use default interactive/gui diff-tool for each file pair"},
			{opt_tool, "Use this diff-tool for each file pair"}
		}
	},

	{
		// TODO 2011/11/01 Move the big FROM/TO stuff to extended help.
		"diffmerge", "sgdm", "dm", NULL, cmd_diffmerge,
		"Diff from root using SourceGear DiffMerge",
		"[(--rev FROM | --tag FROM | --branch FROM) [(--rev TO | --tag TO | --branch TO)]]", NULL,
		SG_FALSE, 
		{
			{OPT_REV_SPEC,0,2},
			{opt_repo,0,1}
		},
		{ {0, ""} }
	},

	{
		"dump_json", NULL, NULL, NULL, cmd_dump_json,
		"Write a repository blob to stdout in JSON format", "blobid", NULL,
		SG_TRUE, 
		{ {opt_repo,0,1} },
		{ {0, ""} }
	},

	{
		"zingmerge", NULL, NULL, NULL, cmd_zingmerge,
		"Attempt automatic merge of all zing dags", "[--repo=REPOSITORY]", NULL,
		SG_TRUE, 
		{ {opt_repo,0,1} }, 
		{ {0, ""} }
	},

	{
		"reindex", NULL, NULL, NULL, cmd_reindex,
		"Rebuild indexes", "[--repo=REPOSITORY] [--all]", NULL,
		SG_TRUE, 
		{ {opt_repo,0,1}, {opt_all,0,0} }, 
		{
			{opt_all, "Reindex all repositories in the closet"},
		}
	},

	{
		"dump_lca", NULL, NULL, NULL, cmd_dump_lca,
		"Write the DAG LCA to stdout in graphviz dot format or optionally as a simple list of nodes", "", NULL,
		SG_TRUE, 
		{ {OPT_REV_SPEC,0,0}, {opt_list,0,0}, {opt_verbose,0,0}, {opt_repo,0,1}
#if defined(DEBUG)
				  , {opt_dagnum,0,1}
#endif
		},
		{ {opt_list, "Output a simple list of nodes rather than using graphviz format"} }
	},

#ifdef DEBUG
	{
		"dump_treenode", NULL, NULL, NULL, cmd_dump_treenode,
		"Dump the contents of the treenode for the given changeset", "[changesetid]", NULL,
		SG_TRUE, 
		{ {opt_repo,0,1} },
		{ {0, ""} }
	},
#endif

#if 0
	{
		"creategroup", NULL, NULL, NULL, cmd_creategroup,
		"Create a new user group", "[--repo=REPOSITORY] group_name", NULL,
		SG_FALSE, 
		{ {opt_repo,0,1} },
		{ {0, ""} }
	},

	{
		"group", NULL, NULL, NULL, cmd_group,
		"Manage membership of a group", "group_name [ addusers|addsubgroups|removeusers|removesubgroups|list|listdeep] member_name+ ]", NULL,
		SG_FALSE,
		{ {opt_repo,0,1} },
		{ {0, ""} }
	},

	{
		"groups", NULL, NULL, NULL, cmd_groups,
		"List groups", "[--repo=REPOSITORY]", NULL,
		SG_FALSE, 
		{ {opt_repo,0,1} }, 
		{ {0, ""} }
	},
#endif

#if defined(WINDOWS)
	{
		"forget-passwords", NULL, NULL, NULL, cmd_forget_passwords,
			"Delete all saved Veracity passwords", "[--no-prompt]", NULL,
			SG_TRUE, 
		{ { opt_no_prompt, 0, 1 } }, 
		{ {0, ""} }
	},
#endif

	{
		"heads", "tips", NULL, NULL, cmd_heads,
		"List the heads of all active named branches", "[--repo DESCRIPTORNAME]", NULL,
		SG_FALSE, 
		{ {opt_repo,0,1}, {opt_all,0,0}, {'b',0,0}, {opt_verbose,0,1} },
		{
			{opt_all, "Include closed branch heads"},
			{'b', "Show heads for only the specified branch(es)"},
			{opt_verbose, "Include full text of multi-line comments"},
			{0, ""}
		}
	},

	{
		"help", NULL, NULL, NULL, cmd_help,
		"List supported commands", "[command name]", NULL,
		SG_FALSE, 
		{ {opt_show_all,0,0}, {opt_quiet,0,0} }, 
		{ {0, ""} }
	},

	{
		"hid", NULL, NULL, NULL, cmd_hid,
		"Print the hash of a file", "filename", NULL,
		SG_TRUE , 
		{ {opt_repo,0,1} }, 
		{ {0, ""} }
	},

	{
		"history", "log", NULL, NULL, cmd_history,
		"Show the history of a repository, directory, or file", "[file-or-folder]", NULL,
		SG_FALSE, 
		{
			{opt_max, 0, 1},
			{opt_repo, 0, 1}, 
			{OPT_REV_SPEC, 0, 0}, 
			{'U',0,1},
			{opt_from,0,1},
			{opt_to,0,1},
			{opt_stamp,0,0},		// TODO 2012/07/05 Shouldn't the upper limit be 1 since we don't allow multiple stamps?
			{opt_leaves,0,0},
			{opt_reverse,0,0},
			{opt_hide_merges,0,0},
			{opt_list_all,0,1},
			{opt_verbose, 0, 1}
		},
		{
			{opt_list_all, "Output full history for each rev/tag"}
		}
	},

	{
		"incoming", NULL, NULL, NULL, cmd_incoming,
		"List changesets that would be pulled", "[--dest DESCRIPTORNAME] [SOURCEREPO]", NULL,
		SG_FALSE,
		{
			{opt_dest,0,1},
			{opt_remember,0,0}
#if defined(DEBUG)
			,{opt_trace,0,0}
#endif
			,{opt_verbose,0,0}
		}, 
		{ 
			{opt_dest, "Print results for a pull into the specified repository"}
		}
	},

	{
		"graph_history", NULL, NULL, NULL, cmd_graph_history,
		"Same as the history command, except output in graphviz dot format", "", NULL,
		SG_TRUE, 
		{
			{opt_max,0,1},
			{opt_repo,0,1},
			{OPT_REV_SPEC, 0, 0}, 
			{'U',0,1},
			{opt_from,0,1}, 
			{opt_to,0,1},
			{opt_stamp,0,0},
			{opt_leaves,0,0},
			{opt_reverse,0,0},
			{opt_hide_merges,0,0},
			{opt_list_all,0,1}
		}, 
		{
			{opt_stamp, "List only changesets indicated by this stamp"},
			{opt_list_all, "Output full history for each rev/tag"}
		}
	},

	{
		"leaves", NULL, NULL, NULL, cmd_leaves,
		"List all leaves of a repository", "[--repo DESCRIPTORNAME]", NULL,
		SG_FALSE,
		{
			{opt_repo,0,1}
#if defined(DEBUG)
			, {opt_dagnum,0,1}
#endif
			, {opt_verbose, 0, 1}
		},
		{ {0, ""} }
	},

	{
		"linehistory", "line_history", NULL, NULL, cmd_linehistory,
		"Search for the last changes to a subsection of a file", "[--repo DESCRIPTORNAME] --startline LINENUMBER --length LENGTH ", NULL,
		SG_TRUE,
		{
			{opt_repo,0,1}
			, {opt_startline,0,1}
			, {opt_length,0,1}
		},
		{ {0, ""} }
	},

	{
		"lock", NULL, NULL, NULL, cmd_lock,
		"Lock a file", "filename [filename [...]] [--server REPOINSTANCE]", NULL,
		SG_FALSE, 
		{ {opt_server,0,1} }, 
		{ {0, ""} }
	},

	{
		"unlock", NULL, NULL, NULL, cmd_unlock,
		"Unlock a file", "filename", NULL,
		SG_FALSE, 
		{ {opt_server,0,1}, {'F',0,0} }, 
		{ {0, ""} }
	},

	{
		"locks", NULL, NULL, NULL, cmd_locks,
		"Show the status of all locks on the current branch", " [--pull] [--server REPOINSTANCE]", NULL,
		SG_FALSE, 
		{
			{opt_repo,0,1},
			{'b',0,1},
			{opt_pull,0,0},
			{opt_server,0,1}
#if defined(DEBUG)
			, {opt_verbose,0,0}
#endif
		},
		{
			{'b', "Show locks for this branch"}
		}
	},

	{
		"merge", NULL, NULL, NULL, cmd_merge,
		"Merge branches", "", NULL,
		SG_FALSE,
		{
			{OPT_REV_SPEC,0,1},
			{'T',0,0},
			{opt_verbose,0,0}, 
			{opt_status,0,0},
			{opt_no_ff_merge,0,0},
			{opt_no_auto_merge,0,0},
			{opt_allow_dirty,0,0},
			// TODO 2012/01/16 deal with any keep/backup/no-backup args.
		},
		{
			{'T', "Determine whether the 'merge' could be performed, but do not actually do it"},
			{opt_status, "Show expected status (most useful with -T)"},
			{opt_verbose, "Show low-level details"},
			{opt_allow_dirty, "Allow dirty merges (debug/experimental)"}
		}
	},

	{
		"merge-preview", "preview-merge", NULL, NULL, cmd_merge_preview,
		"Show which changesets would be brought in by a merge", "[[BASELINE-REVSPEC] OTHER-REVSPEC]", &vv_help_mergepreview,
		SG_FALSE,
		{
			{opt_repo,0,1},
			{OPT_REV_SPEC,0,2},
		},
		{
			{0, ""}
		}
	},

	{
		"move", "mv", NULL, NULL, cmd_move,
		"Pend the moving of a file or directory", "item_to_be_moved ... new_directory", NULL,
		SG_FALSE, 
		{
			{opt_no_allow_after_the_fact,0,0},
			{'T',0,0},
			{opt_verbose,0,0}
		},
		{
			{'T', "Determine whether the 'move' could be performed, but do not actually do it"},
			{opt_verbose, "List items that were or would be moved"}
		}
	},

	{
		"init", "init_new_repo", NULL, NULL, cmd_init_new_repo,
		"Create a new repository and initialize a working copy", "OPTIONS repo_name folder", NULL,
		SG_FALSE,
		{
			// NOTE: opt_storage has been removed.  It should be added back when
			//       we have more than one storage technology available.
			// NOTE: opt_no_wc has been removed.  The public 'vv init' always
			//       creates a WD.  'vv repo new' is new way to create a repo
			//       without a WD.

			{opt_shared_users,0,1},
			{opt_hash,0,1}
		},
		{ {0, ""} }
	},

	{
		"outgoing", NULL, NULL, NULL, cmd_outgoing,
		"List changesets that would be pushed", "[--src DESCRIPTORNAME] [DESTREPO]", NULL,
		SG_FALSE,
		{ 
			{opt_src,0,1},
			{opt_remember,0,0}
#if defined(DEBUG)
			,{opt_trace,0,1}
#endif
			, {opt_verbose, 0, 1}
		}, 
		{ 
			{opt_src, "Print results for a push from the specified repository"}
		}
	},

	{
		"parents", NULL, NULL, NULL, cmd_parents,
		"List the parent changesets of the working copy", "", NULL,
		SG_FALSE,
		{ {opt_verbose,0,1} },
		{ {0, ""} }
	},

	{
		"pull", NULL, NULL, NULL, cmd_pull,
		"Pull committed changes from another repository instance", "[--update] [--rev CHANGESETID] [--tag TAGID]\n"
		"               [--area AREARG] [--dest DESCRIPTORNAME] [SOURCEREPO]", NULL,
		SG_FALSE,
		{ 
			{'u',0,0}, {OPT_REV_SPEC,0,0}, {opt_dest,0,1}, {opt_area,0,0}, {opt_remember,0,0}
#if defined(DEBUG)
			,{opt_trace,0,0}
#endif
		},
		{
			{'r', "Pull only the specified changeset and its ancestors; may be used more than once"},
			{opt_tag, "Pull only the tagged changeset and its ancestors; may be used more than once"},
			{'b',"Pull only the changeset(s) that are part of this branch"},
			{opt_dest, "Pull into the specified repository"},
			{opt_area, "Pull from an area of repository data; may be specified more than once"}
		}
	},

	{
		"push", NULL, NULL, NULL, cmd_push,
		"Push committed changes to another repository instance", "[--force] [--rev CHANGESETID] [--tag TAGID]\n"
		"               [--area AREARG] [--src DESCRIPTORNAME] [DESTREPO]", NULL,
		SG_FALSE, 
		{ 
			{'F',0,0}, {OPT_REV_SPEC,0,0}, {opt_src,0,1}, {opt_area,0,0}, {opt_remember,0,0}
#if defined(DEBUG)
			,{opt_trace,0,0}
#endif
		},
		{
			{'F', "Perform the push even if a branch needs to be merged"},
			{'r', "Push only the specified changeset and its ancestors; may be used more than once"},
			{opt_tag, "Push only the tagged changeset and its ancestors; may be used more than once"},
			{'b',"Push only the changeset(s) that are part of this branch"},
			{opt_src, "Push from the specified repository"},
			{opt_area, "Push from an area of repository data; may be specified more than once"}
		}
	},

	{
		"remove", "rm", "delete", "del", cmd_remove,	// requires 1 or more file-or-folder args
		"Pend the removal of an item from the current repository", "file-or-folder [file-or-folder ...]", &vv_help_remove,
		SG_FALSE, 
		{
			{'T',0,0},				// --test
			{'N',0,0},				// --nonrecursive
			{'F',0,0},				// --force
			{opt_no_backups,0,0},	// --no-backups
			{'k',0,0},				// --keep
			{opt_verbose,0,0}		// --verbose
		},
		{
			{'T', "Do not pend any removals; display details about removals that would be pended"},
			//'N' default message is ok
			{'F', "Force removal of modified items"},
			{opt_no_backups, "Do not create backups of forced items"},
			{'k', "Keep the items in the working copy; only remove them from version control"},
			{opt_verbose, "List items that were or would be removed"}
		}
	},

	{
		"rename", "ren", NULL, NULL, cmd_rename,
		"Pend the renaming of an item in the current repository", "item_to_be_renamed new_name", NULL,
		SG_FALSE,
		{
			{opt_no_allow_after_the_fact,0,0},
			{'T',0,0},
			{opt_verbose,0,0}
		},
		{
			{'T', "Determine whether the 'rename' could be performed, but do not actually do it"},
			{opt_verbose, "List items that were or would be renamed"}
		}
	},

	{
		"repo", "repos", "repository", "repositories", cmd_repo,
		"Create, list, delete, rename, or show details about repositories",
		"[SUBCOMMAND] [...]", &vv_help_repo,
		SG_FALSE,
		{
			{opt_all_but,0,1},
			{opt_no_prompt,0,1},
			{opt_verbose,0,1},
			{opt_list_all,0,1},
			{opt_shared_users,0,1},
			{opt_hash,0,1}
			// NOTE: opt_storage has been removed.  It should be added back when we have more than one storage technology available.
		}, 
		{
			{opt_no_prompt, "Do not prompt to confirm deletion (when using '--all-but') or rename"},
			{opt_verbose, "Show extra details and statistics"},
			{opt_list_all, "List all repositories, even those not ready for use, with their status"}
		}
	},

	{
		"resolve", NULL, NULL, NULL, cmd_resolve2,
		"Resolve merge conflicts", "[subcommand [...]]", &vv_help_resolve2,
		SG_FALSE,
		{
			{ opt_all,       0, 1 },
			{ opt_tool,      0, 1 },
			{ opt_verbose,   0, 1 },
			{ opt_overwrite, 0, 1 },
		},
		{
			{ opt_all,       "Also prompt for conflicts that are already resolved" },
			{ opt_tool,      "Default merge tool to use for interactive 'merge' commands" },
			{ opt_verbose,   "Used with 'list' subcommand to view detailed conflict information" },
			{ opt_overwrite, "Used with 'accept' subcommand to overwrite previous resolutions" },
		}
	},

	{
		"revert", NULL, NULL, NULL, cmd_revert,
		"Revert changes in the pending changeset", "[local paths of things to be reverted]", NULL,
		SG_FALSE, 
		{
			{'N',0,0},
			{'T',0,0},
			{opt_all,0,0},
			{opt_verbose,0,0},
			{opt_no_backups,0,0},
			{opt_revert_files,0,0},
			{opt_revert_symlinks,0,0},
			{opt_revert_directories,0,0},
			{opt_revert_lost,0,0},
			{opt_revert_added,0,0},
			{opt_revert_removed,0,0},
			{opt_revert_renamed,0,0},
			{opt_revert_moved,0,0},
			{opt_revert_merge_added,0,0},
			{opt_revert_update_added,0,0},
			{opt_revert_attributes,0,0},
			{opt_revert_content,0,0},
		},
		{
			{'T', "Do not revert pending changes; display details about changes that would be reverted"},
			{opt_verbose,"Show extra, low-level status details"},
			{opt_no_backups, "Do not create backups of dirty files"}
		}
	},

	{
		// TODO 2011/10/13 This command isn't necessary with the WC re-write of PendingTree.
		// TODO            Before it was an 'advanced' command to flush the timestamp cache
		// TODO            and/or rebuild the wd.json (primarily the latter).
		//
		// I'm going to keep it for now as a way to let the user flush and re-populate the
		// timestamp cache.

		"scan", NULL, NULL, NULL, cmd_scan,
		"Scan the working copy for changed files", "", NULL,
		SG_TRUE,
		{
			{opt_verbose,0,0},				// These args are for
			{opt_no_ignores,0,0},			// the status that we
		},
		{ 
			{opt_verbose,"Show extra, low-level status details"},
		}
	},

	{
		"serve", "server", NULL, NULL, cmd_serve,
		"Launch Veracity in server mode", "", NULL,
		SG_FALSE, 
		{ {'P',0,0}, {opt_port,0,1}/*, {opt_sport,0,1}, {opt_no_plain,0,0}, {opt_cert,0,1}*/ },
		{ {0, ""} }
	},

	{
		"stamp", "stamps", NULL, NULL, cmd_stamp,
		"Stamp a changeset with a user-specified label", 
		"SUBCOMMAND [STAMPNAME] [--all | {--REVSPEC [...]}] [--repo=REPOSITORY]", &vv_help_stamp,
		SG_FALSE, { {opt_all,0,1}, {opt_repo, 0, 1}, {OPT_REV_SPEC,0,0} }, 
		{
			{opt_all, "Used to remove a stamp from all changesets where it is applied"},
			{'r', "Changeset to which stamp should apply"},
			{'b', "Apply stamp to the head(s) of this branch"},
			{opt_tag, "Existing tag identifying a changeset to which stamp should apply"}
		}
	},

	{
		"status", "st", NULL, NULL, cmd_status,
		// TODO 2012/02/13 reword the synopsys.....
		"Show the status of the working copy",
		"[OPTIONS] [file_or_folder [...]] [FROM_REVSPEC [TO_REVSPEC]]", &vv_help_status,
		SG_FALSE, 
		{
			{opt_repo,0,1},
			{OPT_REV_SPEC,0,2}, 
			{'N',0,0},
			{opt_verbose,0,0},
			{opt_no_ignores,0,0},
			{opt_list_unchanged,0,0},
		}, 
		{
			{opt_verbose, "Show extra, low-level status details"},
			{opt_list_unchanged, "List controlled but unchanged items"},
		}
	},

	{
		"mstatus", "", NULL, NULL, cmd_mstatus,
		"Show extended status following a merge",
		"[OPTIONS] [REVSPEC]", &vv_help_mstatus,
		SG_FALSE,
		{
			{opt_repo,0,1},		// historical only
			{OPT_REV_SPEC,0,1},	// historical only
			{opt_verbose,0,0},
			{opt_no_ignores,0,0},
			{opt_no_fallback,0,0}
		},
		{
			{opt_verbose, "Show extra, low-level status details"},
			{opt_no_fallback, "If not in a merge, do not fallback to regular status"}, 
		}
	},

	{
		"svn-load", NULL, NULL, NULL,
		cmd_svn_load,
		"Import from a Subversion dump stream",
		"[--repo=REPOSITORY --REVSPEC [--attach BRANCH | --detached]] [--path PATH] [--stamp STAMP] [--src-comment] < DUMPFILE",
		&vv_help_svnload,
		SG_FALSE,
		{
			{ opt_repo,        0, 1 },
			{ OPT_REV_SPEC,    0, 1 },
			{ opt_attach,      0, 1 },
			{ opt_detached,    0, 1 },
			{ opt_path,        0, 1 },
			{ opt_stamp,       0, 1 },
			{ opt_src_comment, 0, 1 },
			{ opt_confirm,     0, 1 },
		},
		{
			{ opt_repo,        "Repository to import into." },
			{ 'r',             "Use this changeset as the initial parent." },
			{ 'b',             "Use the head of this branch as the initial parent.  Include imported changesets in this branch unless --attach or --detached specified." },
			{ opt_tag,         "Use the tagged changeset as the initial parent." },
			{ opt_attach,      "Include all imported changesets in this branch." },
			{ opt_detached,    "Don't include imported changesets in any branch." },
			{ opt_path,        "Only import data from within this SVN path." },
			{ opt_stamp,       "Apply this stamp to each imported changeset." },
			{ opt_src_comment, "Add a comment to each imported changeset containing the SVN revision that it came from." },
			{ opt_confirm,     "Actually run the import, rather than just printing a warning." },
		}
	},

	{
		"tag", "tags", "label", "labels", cmd_tag,
		"Tag the specified changeset with a unique, user-specified label",
		"SUBCOMMAND [TAGNAME] [--REVSPEC] [--repo=REPOSITORY]", &vv_help_tag,
		SG_FALSE, 
		{ {opt_repo,0,1}, {OPT_REV_SPEC,0,1} }, 
		{
			{opt_tag, "Existing tag identifying a changeset to which the new tag will apply"},
			{'r', "Changeset to which this tag should apply"},
			{'b', "Apply tag to the head of the specified branch"}
		}
	},

	{
		"upgrade", NULL, NULL, NULL, cmd_upgrade,
		"Upgrade repository format to latest version", "", NULL,
		SG_TRUE, 
		{ 
            {opt_confirm,0,0}, 
            {opt_restore_backup,0,0} 
        },
		{
			{opt_confirm, "Instead of explaining what will happen, just do it"}
		}
	},

	{
		"update", NULL, NULL, NULL, cmd_update,
		"Update the working copy to a different (typically newer) changeset", "", NULL,
		SG_FALSE, 
		{
			{OPT_REV_SPEC,0,1},
			{opt_detached,0,0},
			{opt_attach,0,1},
			{opt_attach_new,0,1},
			{'c',0,0},
			{'T',0,0},
			{opt_status,0,0},
			{opt_verbose,0,0}
			// TODO 2012/01/16 deal with any clean/keep/force/backup/no-backup args.
		},
		{
			{opt_detached, "Update working copy and detach from any branch"},
			{opt_attach, "Update working copy and attach to this branch"},
			{opt_attach_new, "Update working copy and attach it to a newly created branch"},
			// default attach-current message is ok
			{'T', "Determine whether the 'update' could be performed, but do not actually do it"},
			{opt_status, "Show expected status (most useful with -T)"},
			{opt_verbose, "Show low-level details"}
		}
	},

	{
		"user", "users", NULL, NULL, cmd_user,
		"Manage user accounts and the current user", "SUBCOMMAND [...] [--repo=REPOSITORY]", &vv_help_user,
		SG_FALSE,
		{ {opt_repo,0,1}, {opt_list_all,0,1} },
		{ {opt_repo, "Manage user accounts belonging to this repository"}, {opt_list_all, "Include inactive users in the user list"} }
	},

	{
		"vcdiff", NULL, NULL, NULL, cmd_vcdiff,
		"Encode or decode binary file deltas", "deltify|undeltify basefile targetfile deltafile", NULL,
		SG_TRUE,
		{ {0,0,0} },
		{ {0, ""} }
	},

	{
		"version", NULL, NULL, NULL, cmd_version,
		"Report the current version", "[server_url]", NULL,
		SG_FALSE,
		{ {0,0,0} },
		{ {0, ""} }
	},

	{
		"whoami", NULL, NULL, NULL, cmd_whoami,
		"Set or print current user account", "[--repo=REPOSITORY] [--create] username", NULL,
		SG_FALSE,
		{ {opt_repo,0,1}, {opt_create, 0, 0} },
		{ {0, ""} }
	},

	{
		"zip", NULL, NULL, NULL, cmd_zip,
		"Export a repository version to a zip file", 
		"filename.zip", NULL,
		SG_FALSE,
		{ {opt_repo,0,1}, {OPT_REV_SPEC,0,0} },
		{ {0, ""} }
	},

#if defined(HAVE_TEST_CRASH)
	{
		"crash", NULL, NULL, NULL, cmd_crash,
		"Cause a crash", "", NULL,
		SG_TRUE,
		{ {0,0,0} },
		{ {0, ""} }
	},
#endif

};

const SG_uint32 VERBCOUNT = (sizeof(commands) / sizeof(struct cmdinfo));


subcmdinfo subcommands[] =
{
#if 0
	{
		"verify", "tree_deltas",
		"Check integrity of tree deltas within version control changesets",
		"",
		NULL,
		{ 
            {opt_repo,0,1},
            {OPT_REV_SPEC,0,0} 
        },
		{
			{0, NULL}
		}
	},
#endif
	{
		"branch", "new",
		"Attach the working copy to a new branch and begin working in it",
		"BRANCHNAME",
		&vv_help_branch_new,
		{ {0,0,0} }, 
		{ {0, NULL} }
	},
	{
		"branch", "attach",
		"Attach the working copy to an existing branch",
		"BRANCHNAME",
		&vv_help_branch_attach,
		{ {0,0,0} }, 
		{ {0, NULL} }
	},
	{
		"branch", "detach",
		"Detach the working copy from any currently attached branch",
		"BRANCHNAME [--REVSPEC [...]] [--repo REPOSITORY]",
		&vv_help_branch_detach,
		{ {0,0,0} }, 
		{ {0, NULL} }
	},
	{
		"branch", "add-head",
		"Add a new BRANCHNAME head at REVSPEC",
		"BRANCHNAME --REVSPEC [--repo REPOSITORY]",
		&vv_help_branch_add_head,
		{ {OPT_REV_SPEC,0,1}, {opt_repo,0,1} }, 
		{ {0, NULL} }
	},
	{
		"branch", "remove-head",
		"Remove one or more BRANCHNAME heads",
		"BRANCHNAME (--all | --REVSPEC [...]) [--repo REPOSITORY]",
		&vv_help_branch_remove_head,
		{ {opt_all,0,1}, {OPT_REV_SPEC,0,2}, {opt_repo,0,1} }, 
		{
			{opt_all, "Remove all heads of BRANCHNAME"}
		}
	},
	{
		"branch", "move-head",
		"Move one or all heads of BRANCHNAME to another changeset",
		"BRANCHNAME (--all | [--REV_SOURCE]) --REV_TARGET [--repo REPO]",
		&vv_help_branch_move_head,
		{ {opt_all,0,1}, {OPT_REV_SPEC,1,2}, {opt_repo,0,1} }, 
		{
			{opt_all, "Consolidate all heads of BRANCHNAME"}
		}
	},
	{
		"branch", "close",
		"Close (de-activate) the branch referred to by BRANCHNAME",
		"BRANCHNAME [--repo REPOSITORY]",
		&vv_help_branch_close,
		{ {opt_repo,0,1} }, 
		{ {0, NULL} }
	},
	{
		"branch", "reopen",
		"Reopen (re-activate) the branch referred to by BRANCHNAME",
		"BRANCHNAME [--repo REPOSITORY]",
		&vv_help_branch_reopen,
		{ {opt_repo,0,1} }, 
		{ {0, NULL} }
	},
	{
		"branch", "list",
		"List all open branches, indicating those that need to be merged",
		"[--all] [--repo REPOSITORY]",
		&vv_help_branch_list,
		{ {opt_all,0,1}, {opt_repo,0,1} }, 
		{
			{opt_all, "Include and indicate closed branches when listing"}
		}
	},
	{
		"config", "export",
		"Export Veracity configuration as a JSON object",
		"[filename]",
		NULL,
		{ {opt_verbose,0,0} },
		{
			{0, NULL}
		}
	},
	{
		"config", "import",
		"Import Veracity configuration from a JSON document",
		"[filename]",
		NULL,
		{ {0,0,0} },
		{ {0, NULL} }
	},

	{
		"config", "set", 
		"Set a configuration value",
		"(setting) (value)",
		NULL,
		{ {'F',0,0}, {0,0,0} },
		{ {0, NULL} }
	},
	{
		"config", "reset",
		"Reset a configuration setting to its default value",
		"(setting)",
		NULL,
		{ {'F',0,0}, {0,0,0} },
		{ {0, NULL} }
	},
	{
		"config", "empty",
		"Remove the contents of a configuration setting",
		"(setting)",
		NULL,
		{ {'F',0,0}, {0,0,0} },
		{ {0, NULL} }
	},
	{
		"config", "add-to",
		"Add a value to a config setting list",
		"(setting) (value) (value) ...",
		&vv_help_config_add_to,
		{ {'F',0,0}, {0,0,0} },
		{ {0, NULL} }
	},
	{
		"config", "remove-from",
		"Remove a value from a config setting list",
		"(setting) (value) (value) ...",
		NULL,
		{ {'F',0,0}, {0,0,0} },
		{ {0, NULL} }
	},
	{
		"config", "show",
		"Print all configuration values at and below the named setting, or all values if setting is omitted",
		"[setting]",
		NULL,
		{ {0,0,0} },
		{ {0, NULL} }
	},
	{
		"config", "defaults",
		"Print default configuration values",
		"",
		NULL,
		{ {0,0,0} },
		{ {0, NULL} }
	},
	{
		"config", "list-ignores",
		"Print the effective list of ignores",
		"",
		NULL,
		{ {0,0,0} },
		{ {0, NULL} }
	},
	{
		"stamp", "add",
		"Add a stamp to a changeset",
		"STAMPNAME [--REVSPEC [...]] [--repo REPOSITORY]",
		&vv_help_stamp_add,
		{ {opt_repo, 0, 1}, {OPT_REV_SPEC,0,0} }, 
		{
			{'r', "Changeset to which this stamp should be added"},
			{'b', "Add the stamp to the head(s) of this branch"},
			{opt_tag, "Existing tag identifying a changeset to which this stamp should be added"}
		}
	},
	{
		"stamp", "remove",
		"Remove a stamp from a changeset",
		"STAMPNAME [--all | {--REVSPEC [...]}] [--repo REPOSITORY]",
		&vv_help_stamp_remove,
		{ {opt_all,0,1}, {opt_repo, 0, 1}, {OPT_REV_SPEC,0,0} }, 
		{
			{opt_all, "Remove stamp from all changesets where it has been applied"},
			{'r', "Changeset from which this stamp should be removed"},
			{'b', "Remove the stamp from the heads(s) of this branch"},
			{opt_tag, "Existing tag identifying a changeset from which this stamp should be removed"}
		}
	},
	{
		"stamp", "list",
		"Print a summary of stamp usage",
		"[STAMPNAME] [--repo REPOSITORY]",
		&vv_help_stamp_list,
		{ {opt_repo, 0, 1} }, 
		{ {0, NULL} }
	},
	{
		"tag", "add",
		"Add a tag to a changeset",
		"NEWTAG [--REVSPEC] [--repo REPOSITORY]",
		&vv_help_tag_add,
		{ {opt_repo, 0, 1}, {OPT_REV_SPEC,0,0} }, 
		{
			{'r', "Changeset to which this tag should be added"},
			{'b', "Add tag to the head of this branch"},
			{opt_tag, "Existing tag identifying a changeset to which this tag should be added"}
		}
	},
	{
		"tag", "move",
		"Move a tag to a different changeset",
		"EXISTINGTAG [--REVSPEC] [--repo REPOSITORY]",
		&vv_help_tag_move,
		{ {opt_repo, 0, 1}, {OPT_REV_SPEC,0,0} }, 
		{
			{'r', "Changeset to which this tag should be moved"},
			{'b', "Move tag to the head of this branch"},
			{opt_tag, "Existing tagged changeset to which this tag should be moved"}
		}
	},
	{
		"tag", "remove",
		"Remove a tag from its associated changeset",
		"EXISTINGTAG [--repo REPOSITORY]",
		&vv_help_tag_remove,
		{ {opt_repo, 0, 1} }, 
		{ {0, NULL} }
	},
	{
		"tag", "list",
		"Print a list of tags and their associated changesets",
		"[--repo REPOSITORY]",
		&vv_help_tag_list,
		{ {opt_repo, 0, 1} }, 
		{ {0, NULL} }
	},
	{
		"resolve", "list",
		"List conflicts in the working copy",
		"[FILENAME]",
		&vv_help_resolve2_list,
		{
			{ opt_all,     0, 1 },
			{ opt_verbose, 0, 1 },
		},
		{
			{ opt_all,     "Include conflicts that are already resolved in the list" },
			{ opt_verbose, "Show detailed information about each conflict" }
		}
	},
	{
		"resolve", "next",
		"List the next unresolved conflict in the working copy",
		"[FILENAME]",
		&vv_help_resolve2_next,
		{ { 0, 0, 0 } },
		{ { 0, NULL } }
	},
	{
		"resolve", "accept",
		"Resolve a conflict by accepting a potential resolution value",
		"VALUE [FILENAME] [CONFLICT]",
		&vv_help_resolve2_accept,
		{
			{ opt_all,       0, 1 },
			{ opt_overwrite, 0, 1 },
		},
		{
			{ opt_all,       "Accept the resolution value on all conflicts that meet any provided criteria" },
			{ opt_overwrite, "Allow resolved conflicts to be re-resolved with a different resolution value" },
		}
	},
/*	{
		"resolve", "view",
		"View a resolution value from a conflict",
		"VALUE FILENAME [CONFLICT]",
		&vv_help_resolve2_view,
		{
			{ opt_tool, 0, 1 },
		},
		{
			{ opt_tool, "Use a specific viewer tool" },
		}
	},
*/	{
		"resolve", "diff",
		"Diff two resolution values from a conflict",
		"VALUE1 VALUE2 FILENAME [CONFLICT]",
		&vv_help_resolve2_diff,
		{
			{ opt_tool, 0, 1 },
		},
		{
			{ opt_tool, "Use a specific diff tool" },
		}
	},
	{
		"resolve", "merge",
		"Create a new merged resolution value on a conflict",
		"FILENAME [CONFLICT]",
		&vv_help_resolve2_merge,
		{
			{ opt_tool, 0, 1 },
		},
		{
			{ opt_tool, "Use a specific merge tool" },
		}
	},
	{
		"resolve", "FILENAME",
		"Prompt to resolve each conflict on the specified file",
		"",
		&vv_help_resolve2_FILENAME,
		{
			{ opt_tool, 0, 1 },
		},
		{ { 0, NULL } }
	},
	{
		"repo", "new",
		"Create a new, empty repository (without associating any directory as a\n"
		"        working copy)",
		"NAME [--shared-users=REPOSITORY | --hash=HASHMETHOD]",
		NULL,
		{
			{opt_shared_users,0,1},
			{opt_hash,0,1},
			// NOTE: opt_storage has been removed.  It should be added back when we have more than one storage technology available.
			{ 0, 0, 0 }
		},
		{
			{ 0, NULL }
		}
	},
	{
		"repo", "delete",
		"Delete one or more repositories",
		"[--all-but [--no-prompt]] REPOSITORY [REPOSITORY2 ...]",
		NULL,
		{
			{ opt_all_but, 0, 1 },
			{ opt_no_prompt, 0, 1 },
			{ 0, 0, 0 }
		},
		{
			{opt_no_prompt, "Do not prompt to confirm deletion when using '--all-but'"}
		}
	},
	{
		"repo", "rename",
		"Rename a repository",
		"OLDNAME NEWNAME",
		NULL,
		{
			{ opt_no_prompt, 0, 1 },
			{ 0, 0, 0 }
		},
		{
			{opt_no_prompt, "Do not prompt to confirm rename"}
		}
	},
	{
		"repo", "info",
		"Show information about a repository",
		"[REPONAME | DISKPATH]",
		NULL,
		{
			{ opt_verbose, 0, 1 },
			{ 0, 0, 0 }
		},
		{
			{opt_verbose, "Show extra details and statistics"}
		}
	},
	{
		"repo", "list",
		"List available Veracity repositories",
		"[--list-all]",
		NULL,
		{
			{ opt_list_all, 0, 1 },
			{ 0, 0, 0 }
		},
		{
			{opt_list_all, "List all repositories, even those not ready for use, with their status"}
		}
	},
	{
		"user", "create",
		"Create a new user account",
		"USERNAME [--repo REPOSITORY]",
		NULL,
		{
			{ 0, 0, 0 }
		},
		{
			{ 0, NULL }
		}
	},
	{
		"user", "set",
		"Set the current user account",
		"USERNAME [--repo REPOSITORY]",
		NULL,
		{
			{ 0, 0, 0 }
		},
		{
			{ 0, NULL }
		}
	},
	{
		"user", "rename",
		"Rename an existing user account",
		"OLDUSERNAME NEWUSERNAME [--repo REPOSITORY]",
		NULL,
		{
			{ 0, 0, 0 }
		},
		{
			{ 0, NULL }
		}
	},
	{
		"user", "list",
		"Print a list of existing user accounts",
		"[--repo REPOSITORY] [--list-all]",
		NULL,
		{
			{opt_list_all, 0, 1}, { 0, 0, 0 }
		},
		{
			{ 0, NULL }
		}
	},
	{
		"user", "set-active",
		"Set a user's account status to 'active'",
		"USERNAME [--repo REPOSITORY]",
		NULL,
		{
			{ 0, 0, 0 }
		},
		{
			{ 0, NULL }
		}
	},
	{
		"user", "set-inactive",
		"Set a user's account status to 'inactive'",
		"USERNAME [--repo REPOSITORY]",
		NULL,
		{
			{ 0, 0, 0 }
		},
		{
			{ 0, NULL }
		}
	}

};

const SG_uint32 SUBCMDCOUNT = (sizeof(subcommands) / sizeof(struct subcmdinfo));


static cmdhelpinfo vvBonusHelp[] = 
{
	{ "filetool", "Specify tools used to compare and merge different file types", &vv_help_filetool },
	{ "repo-paths", "Repository path prefixes", &vv_help_repopaths }
};

const SG_uint32 BONUSHELPCOUNT = (sizeof(vvBonusHelp) / sizeof(cmdhelpinfo));


/* ----------------*/
/* Helper routines */
/* ----------------*/

static void _get_canonical_command(const char *cmd_name,
								   const cmdinfo** pCmdInfo)
{
	SG_uint32 i = 0;
	const cmdinfo *ci = NULL;
	SG_uint32 len = 0;

	if (cmd_name == NULL)
		return;

	for ( i = 0; i < VERBCOUNT; ++i )
	{
		if ((strcmp(cmd_name, commands[i].name) == 0) ||
			(commands[i].alias1 && (strcmp(cmd_name, commands[i].alias1) == 0)) ||
			(commands[i].alias2 && (strcmp(cmd_name, commands[i].alias2) == 0)) ||
			(commands[i].alias3 && (strcmp(cmd_name, commands[i].alias3) == 0))
		   )
		{
			*pCmdInfo = commands + i;
			return;
		}
	}

	len = SG_STRLEN(cmd_name);

	if (len == 0)
		return;

	// look for *exactly one* substring match
	for ( i = 0; i < VERBCOUNT; ++i )
	{
		if ((strncmp(cmd_name, commands[i].name, len) == 0) ||
			(commands[i].alias1 && (strncmp(cmd_name, commands[i].alias1, len) == 0)) ||
			(commands[i].alias2 && (strncmp(cmd_name, commands[i].alias2, len) == 0)) ||
			(commands[i].alias3 && (strncmp(cmd_name, commands[i].alias3, len) == 0))
		   )
		{
			if (ci != NULL)   // multiple matches, bail
				return;

			ci = commands + i;
		}
	}

	if (ci)
		*pCmdInfo = ci;

	// TODO Review: so what happens here?  We return nothing or NULL?
	// TODO         is this case even possible? (have we already gone
	// TODO         a parser that did validate the name?)

  /* If we get here, there was no matching command name or alias. */
	return;
}

static void _parse_and_count_args(
	SG_context *pCtx,
	SG_getopt* os,
	const char*** ppaszArgs,
	SG_uint32 expectedCount,
	SG_bool *pbUsageError)
{
	SG_uint32	count_args = 0;

	SG_ERR_CHECK_RETURN(  SG_getopt__parse_all_args(pCtx, os, ppaszArgs, &count_args)  );

	if (count_args != expectedCount)
		*pbUsageError = SG_TRUE;
}

// callback for SG_qsort
int _command_compare(SG_context * pCtx,
					const void * pVoid_pIndex1,
					const void * pVoid_pIndex2,
					SG_UNUSED_PARAM(void * pVoidCallerData))
{
	const SG_uint32* pIndex1 = (const SG_uint32 *) pVoid_pIndex1;
	const SG_uint32* pIndex2 = (const SG_uint32 *) pVoid_pIndex2;
	SG_UNUSED(pCtx);
	SG_UNUSED(pVoidCallerData);

	return strcmp( commands[*pIndex1].name, commands[*pIndex2].name );
}

static void do_cmd_help__list_bonus_help(SG_context *pCtx, SG_console_stream cs, const char *topic, SG_bool *pbListed)
{
	SG_uint32 i;

	SG_NULLARGCHECK_RETURN(pbListed);
	SG_NULLARGCHECK_RETURN(topic);

	*pbListed = SG_FALSE;

	for ( i = 0; i < BONUSHELPCOUNT; ++i )
	{
		cmdhelpinfo *help = &vvBonusHelp[i];

		if (strcmp(help->name, topic) == 0)
		{
			SG_ERR_IGNORE(  SG_console(pCtx, cs, "%s\n\n%s", help->desc, *help->helptext)  );

			*pbListed = SG_TRUE;
			break;
		}
	}
}


static void do_cmd_help__list_all_commands(SG_context * pCtx, SG_console_stream cs, const char *pszAppName, SG_option_state * pOptSt)
{
	SG_uint32 commandIndex[(sizeof(commands) / sizeof(struct cmdinfo))];
	SG_uint32 i;
	SG_uint32 commandWidth = 0;
	SG_uint32 consoleWidth;
	SG_bool bVerbose = SG_FALSE;
	SG_bool bQuiet = SG_FALSE;
	
	consoleWidth = SG_console__get_width();
	
	if (pOptSt)
	{
		bVerbose = pOptSt->bShowAll;
		bQuiet = pOptSt->bQuiet;
	}
	
	if (!bQuiet)
		SG_ERR_IGNORE(  SG_console(pCtx, cs, "Usage: %s command [options]\n\nCommands:\n\n", pszAppName)  );
	for (i=0; i<VERBCOUNT; i++)
	{
		commandIndex[i] = i;
		if ((bVerbose || (! commands[i].advanced)) && (SG_STRLEN(commands[i].name) > commandWidth))
		{
			commandWidth = SG_STRLEN(commands[i].name);
		}
	}

	if (bVerbose)
		for ( i = 0; i < BONUSHELPCOUNT; ++i )
			if (SG_STRLEN(vvBonusHelp[i].name) > commandWidth)
				commandWidth = SG_STRLEN(vvBonusHelp[i].name);

	SG_qsort(pCtx, commandIndex, VERBCOUNT, sizeof(commandIndex[0]),
			 _command_compare, NULL);

	for (i=0; i<VERBCOUNT; i++)
	{
		if (bVerbose || (! commands[commandIndex[i]].advanced))
		{
			SG_getopt__pretty_print(pCtx, cs, commands[commandIndex[i]].name, " ", commands[commandIndex[i]].desc, commandWidth, consoleWidth);
		}
	}

	if (bVerbose)
	{
		if (!bQuiet)
			SG_getopt__pretty_print(pCtx, cs, "", "", "\nAdditional Help topics:\n", 0, consoleWidth);

		for ( i = 0; i < BONUSHELPCOUNT; ++i )
			SG_getopt__pretty_print(pCtx, cs, vvBonusHelp[i].name, " ", vvBonusHelp[i].desc, commandWidth, consoleWidth);
	}
	
	if (!bQuiet)
	{
		SG_getopt__pretty_print(pCtx, cs, "", "", "\ninput \"" WHATSMYNAME " help command\" for details on a particular command or topic", 0, consoleWidth);

		if (!bVerbose)
			SG_getopt__pretty_print(pCtx, cs, "", "", "input \"" WHATSMYNAME " help --all-commands\" for the full list of help topics\n", 0, consoleWidth);
	}
}

static void _print_option_help(SG_context* pCtx, const cmdinfo* thiscmd, SG_int32 optch, SG_console_stream cs) 
{
	const char* dest_override = NULL;
	SG_uint32 idx;
	SG_uint32 k;

	for (k = 0; k<(sizeof(sg_cl_options) / sizeof(struct sg_getopt_option)); k++)
	{
		if (optch == sg_cl_options[k].optch)
		{
			SG_getopt_option* pOpt = &sg_cl_options[k];
			for (idx = 0; (idx < SG_CMDINFO_OPTION_LIMIT) && (thiscmd->desc_overrides[idx]).optch; idx++)
			{
				if ((thiscmd->desc_overrides[idx]).optch == pOpt->optch)
				{
					dest_override = (thiscmd->desc_overrides[idx]).desc;
					break;
				}
			}
			SG_ERR_IGNORE(  SG_getopt__print_option(pCtx, cs, pOpt, dest_override)  );
			break;
		}
	}
}


static void _print_suboption_help(SG_context* pCtx, const cmdinfo* thiscmd, const subcmdinfo *thissub, SG_int32 optch, SG_console_stream cs) 
{
	const char* dest_override = NULL;
	SG_uint32 idx;
	SG_uint32 k;

	for (k = 0; k<(sizeof(sg_cl_options) / sizeof(struct sg_getopt_option)); k++)
	{
		if (optch == sg_cl_options[k].optch)
		{
			SG_getopt_option* pOpt = &sg_cl_options[k];
			for (idx = 0; (idx < SG_CMDINFO_OPTION_LIMIT) && (thissub->desc_overrides[idx]).optch; idx++)
			{
				if ((thissub->desc_overrides[idx]).optch == pOpt->optch)
				{
					dest_override = (thissub->desc_overrides[idx]).desc;
					break;
				}
			}

			if (dest_override)
				SG_ERR_IGNORE(  SG_getopt__print_option(pCtx, cs, pOpt, dest_override)  );
			else
				SG_ERR_IGNORE(  _print_option_help(pCtx, thiscmd, optch, cs)  );

			break;
		}
	}
}


static SG_bool _anySubcommands(const cmdinfo *cmd, const char *subname)
{
	SG_uint32 i;

	for ( i = 0; i < SUBCMDCOUNT; ++i )
		if (strcmp(cmd->name, subcommands[i].cmdname) == 0)
			if ((! subname) || (strcmp(subcommands[i].name, subname) == 0))
				return(SG_TRUE);

	return(SG_FALSE);
}

static void _list_subcommands(
	SG_context *pCtx, 
	SG_console_stream cs,
	const cmdinfo *cmd)
{
	if (_anySubcommands(cmd, NULL))
	{
		SG_uint32 i;

		SG_ERR_IGNORE(  SG_console(pCtx, cs, "\nSubcommands:\n------------\n")  );

		for ( i = 0; i < SUBCMDCOUNT; ++i )
			if (strcmp(cmd->name, subcommands[i].cmdname) == 0)
			{
				const subcmdinfo *sci = &subcommands[i];

				SG_ERR_IGNORE(  SG_console(pCtx, cs, "    %s %s\n        %s\n", sci->name, sci->extraHelp, sci->desc)  );
			}
	}
}

static void do_cmd_help__list_subcommand_help(
	SG_context * pCtx, 
	SG_console_stream cs,
	const char * szAppName,
	const cmdinfo *cmd,
	const char * szSubcommandName)
{
	SG_uint32 i = 0;
	SG_uint32 consoleWidth;
	const subcmdinfo *thissub = NULL;

	consoleWidth = SG_console__get_width();

	for ( i = 0; i < SUBCMDCOUNT; ++i )
		if ((strcmp(cmd->name, subcommands[i].cmdname) == 0) && (strcmp(subcommands[i].name, szSubcommandName) == 0))
		{
			thissub = &subcommands[i];
			break;
		}

	if (thissub)
	{
		SG_uint32 j = 0;

		SG_ERR_IGNORE(  SG_console(pCtx, cs, "\n")  );

		SG_getopt__pretty_print(pCtx, cs, "", "", thissub->desc, 0, consoleWidth);

		SG_ERR_IGNORE(  SG_console(pCtx, cs, "\n")  );

		SG_ERR_IGNORE(  SG_console(pCtx, cs, "Usage: %s %s %s %s\n", szAppName, cmd->name, thissub->name, thissub->extraHelp)  );

		if (thissub->valid_options[0].option)
			SG_ERR_IGNORE(  SG_console(pCtx, cs, "\nPossible options:\n")  );
		while(thissub->valid_options[j].option)
		{
			if (thissub->valid_options[j].option == OPT_REV_SPEC)
			{
				SG_ERR_IGNORE(  _print_suboption_help(pCtx, cmd, thissub, 'r', cs)  );
				SG_ERR_IGNORE(  _print_suboption_help(pCtx, cmd, thissub, opt_tag, cs)  );
				SG_ERR_IGNORE(  _print_suboption_help(pCtx, cmd, thissub, 'b', cs)  );
			}
			SG_ERR_IGNORE(  _print_suboption_help(pCtx, cmd, thissub, thissub->valid_options[j].option, cs)  );
			j++;
		}

		if (thissub->extendedHelp && *(thissub->extendedHelp))
			SG_ERR_IGNORE(  SG_console(pCtx, cs, "\n%s", *(thissub->extendedHelp))  );
	}
	else
	{
		SG_ERR_IGNORE(  SG_console(pCtx, cs, "Unknown subcommand: %s %s\n\n", cmd->name, szSubcommandName)  );
		SG_ERR_THROW_RETURN(  SG_ERR_USAGE  );
	}
}


static void do_cmd_help__list_command_help(
	SG_context * pCtx, 
	SG_console_stream cs,
	const char * szAppName, 
	const char * szCommandName, 
	const char * szSubcommandName,
	SG_bool showCommandName)
{
	SG_uint32 j = 0;
	const cmdinfo *thiscmd = NULL;
	SG_uint32 consoleWidth;

	_get_canonical_command(szCommandName, &thiscmd);

	if (szSubcommandName && _anySubcommands(thiscmd, szSubcommandName))
	{
		do_cmd_help__list_subcommand_help(pCtx, cs, szAppName, thiscmd, szSubcommandName);
		return;
	}

	consoleWidth = SG_console__get_width();

	if (thiscmd)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, cs, "\n")  );

		if (showCommandName)
		{
			SG_getopt__pretty_print(pCtx, cs, thiscmd->name, ": ", thiscmd->desc, 0, consoleWidth);
		}
		else
		{
			SG_getopt__pretty_print(pCtx, cs, "", "", thiscmd->desc, 0, consoleWidth);
		}
		SG_ERR_IGNORE(  SG_console(pCtx, cs, "\n")  );

		SG_ERR_IGNORE(  SG_console(pCtx, cs, "Usage: %s %s %s\n", szAppName, thiscmd->name, thiscmd->extraHelp)  );

		if (thiscmd->valid_options[0].option)
			SG_ERR_IGNORE(  SG_console(pCtx, cs, "\nPossible options:\n")  );
		while(thiscmd->valid_options[j].option)
		{
			if (thiscmd->valid_options[j].option == OPT_REV_SPEC)
			{
				SG_ERR_IGNORE(  _print_option_help(pCtx, thiscmd, 'r', cs)  );
				SG_ERR_IGNORE(  _print_option_help(pCtx, thiscmd, opt_tag, cs)  );
				SG_ERR_IGNORE(  _print_option_help(pCtx, thiscmd, 'b', cs)  );
			}
			SG_ERR_IGNORE(  _print_option_help(pCtx, thiscmd, thiscmd->valid_options[j].option, cs)  );
			j++;
		}

		SG_ERR_IGNORE(  _list_subcommands(pCtx, cs, thiscmd)  );

		if (thiscmd->extendedHelp && *(thiscmd->extendedHelp))
			SG_ERR_IGNORE(  SG_console(pCtx, cs, "\n%s", *(thiscmd->extendedHelp))  );
	}
	else
	{
		SG_ERR_IGNORE(  SG_console(pCtx, cs, "Unknown command: %s\n\n", szCommandName)  );
		SG_ERR_THROW_RETURN(  SG_ERR_USAGE  );
	}
}

//////////////////////////////////////////////////////////////////

/**
 * Dump the "Journal" (formerly "ActionLog") to the console.
 * This is for things that requested "--verbose".
 *
 * TODO Pretty this up; dumping JSON to the terminal is stupid.
 */
void sg_report_journal(SG_context * pCtx, const SG_varray * pva)
{
    SG_vhash* pvh = NULL;				// we DO NOT own this
    SG_string* pstr = NULL;
    SG_uint32 count;
    SG_uint32 i;

	if (pva)
	{
		SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
		for (i=0; i<count; i++)
		{
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
			SG_ERR_CHECK(  SG_string__clear(pCtx, pstr)  );
			SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh, pstr)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT ,"%s\n", SG_string__sz(pstr))  );
		}
	}

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

//////////////////////////////////////////////////////////////////

/* TODO we should ask ourselves if we need portability warnings on tag strings.
 * For now, I'm going to assume no. */


static SG_bool _get_option_index(const cmdinfo* pCmdInfo, SG_int32 iOptionCode, SG_uint32* piIdx)
{
	SG_uint32 i;

	for (i = 0; i < SG_NrElements(pCmdInfo->valid_options); i++)
	{
		if (pCmdInfo->valid_options[i].option == iOptionCode)
		{
			if (piIdx)
				*piIdx = i;
			return SG_TRUE;
		}
		else if (pCmdInfo->valid_options[i].option == 0) // no more valid options
			break;
	}

	return SG_FALSE;
}

static void _print_invalid_option(SG_context* pCtx, const char* pszAppName, const char* pszCommandName, SG_int32 iInvalidOpt)
{
	// TODO fix this to print a nicer message.
	SG_console(pCtx, SG_CS_STDERR, "\nInvalid option for '%s %s': ", pszAppName, pszCommandName);
	SG_getopt__print_invalid_option(pCtx, iInvalidOpt, sg_cl_options);
	SG_console(pCtx, SG_CS_STDERR, "\n");
}

static void _print_too_many(SG_context* pCtx, SG_int32 iInvalidOpt, SG_uint32 min, SG_uint32 max)
{
	char* pszOptionAsProvided = NULL;
	char* pszRefOptionDesc = NULL;

	if (iInvalidOpt != OPT_REV_SPEC)
	{
		SG_ERR_CHECK(  SG_getopt__get_option_as_provided(pCtx, iInvalidOpt, sg_cl_options, &pszOptionAsProvided)  );
		pszRefOptionDesc = pszOptionAsProvided;
	}
	else
	{
		pszRefOptionDesc = OPT_REV_SPEC_DESCRIPTION;
	}

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "\n")  );

	if (max == 1)
	{
		if (min)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' must be specified.\n", pszRefOptionDesc)  );
		else
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' can only be specified once.\n", pszRefOptionDesc)  );
	}
	else
	{
		if (!min)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' can be specified up to %u times.\n", pszRefOptionDesc, max)  );
		else
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' must be specified between %u and %u times.\n", pszRefOptionDesc, min, max)  );
	}

	/* common cleanup */
fail:
	SG_NULLFREE(pCtx, pszOptionAsProvided);
}

static void _print_too_few(SG_context* pCtx, SG_int32 iInvalidOpt, SG_uint32 min, SG_uint32 max)
{
	char* pszOptionAsProvided = NULL;
	const char* pszRefOptionDesc = NULL;

	if (iInvalidOpt != OPT_REV_SPEC)
	{
		SG_ERR_CHECK(  SG_getopt__get_option_as_provided(pCtx, iInvalidOpt, sg_cl_options, &pszOptionAsProvided)  );
		pszRefOptionDesc = pszOptionAsProvided;
	}
	else
	{
		pszRefOptionDesc = OPT_REV_SPEC_DESCRIPTION;
	}

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "\n")  );

	if (max == 0)
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' must be specified at least %u times.\n", pszRefOptionDesc, min)  );
	else if (max == 1)
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' must be specified.\n", pszRefOptionDesc)  );
	else 
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' must be specified between %u and %u times.\n", pszRefOptionDesc, min, max)  );

	/* common cleanup */
fail:
	SG_NULLFREE(pCtx, pszOptionAsProvided);
}

/**
 * Look at all of the -f or --flags given on the command line
 * and verify each is defined for this command.
 *
 * Print error message for each invalid flag and return bUsageError.
 * (We don't throw this because we are only part of the parse and
 * validation.  We want the caller to be able to use the existing
 * INVOKE() macro and be consistent with the other CMD_FUNCs (at
 * least for now).
 *
 * TODO convert all CMD_FUNCs that have a loop like the one in this
 * TODO function to use this function instead.
 */
static void _inspect_all_options(SG_context * pCtx,
								 const char * pszAppName,
								 const char * pszCommandName,
								 SG_option_state * pOptSt,
								 SG_bool * pbUsageError)
{
	const cmdinfo * pThisCmd = NULL;
	SG_bool bUsageError = SG_FALSE;
	SG_uint32 k;
	SG_varray* pvaHandled = NULL;

	_get_canonical_command(pszCommandName, &pThisCmd);
	SG_ASSERT(  (pThisCmd)  );

	SG_VARRAY__ALLOC__PARAMS(pCtx, &pvaHandled, 10, NULL, NULL);

	/* Handle OPT_REV_SPEC, if this command takes one. */
	for (k=0; k < SG_CMDINFO_OPTION_LIMIT; k++) /* Looping over all options this command supports. */
	{
		cmdopt current_option = pThisCmd->valid_options[k];
		if (current_option.option == OPT_REV_SPEC)
		{
			/* If the command supports OPT_REV_SPEC, we handle rev/tag/branch arguments and prevent them from being checked below. */
			
			SG_uint32 numRevSpecsGiven = 0;

			if (pOptSt->psa_revs)
				SG_STRINGARRAY_NULLFREE(pCtx, pOptSt->psa_revs);
			if (pOptSt->psa_tags)
				SG_STRINGARRAY_NULLFREE(pCtx, pOptSt->psa_tags);
			if (pOptSt->psa_branch_names)
				SG_STRINGARRAY_NULLFREE(pCtx, pOptSt->psa_branch_names);

			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &numRevSpecsGiven)  );
			
			if (current_option.min && numRevSpecsGiven < current_option.min)
			{
				bUsageError = SG_TRUE;
				SG_ERR_IGNORE(  _print_too_few(pCtx, current_option.option, current_option.min, current_option.max)  );
			}
			if (current_option.max && numRevSpecsGiven > current_option.max)
			{
				bUsageError = SG_TRUE;
				SG_ERR_IGNORE(  _print_too_many(pCtx, current_option.option, current_option.min, current_option.max)  );
			}

			SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pvaHandled, OPT_REV_SPEC)  );
			SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pvaHandled, 'r')  );
			SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pvaHandled, opt_tag)  );
			SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pvaHandled, 'b')  );

			break;
		}
		if (current_option.option == 0)
			break;
	}

	/* Loop through all provided options */
	for (k=0; k < pOptSt->count_options; k++)
	{
		SG_uint32 idx;
		SG_int32 current_option = pOptSt->paRecvdOpts[k];
		SG_bool bHandled = SG_FALSE;

		SG_ERR_CHECK(  SG_varray__find__int64(pCtx, pvaHandled, current_option, &bHandled, NULL)  );

		if (bHandled)
			continue;

		/* If an option that's not valid for this command is specified, say so. */
		if ( !_get_option_index(pThisCmd, current_option, &idx) )
		{
			SG_ERR_IGNORE(  _print_invalid_option(pCtx, pszAppName, pszCommandName, current_option)  );
			bUsageError = SG_TRUE;
		}
		else
		{
			/* If a non-zero max is specified for the command, verify that the option wasn't specified more than that. */
			if (pThisCmd->valid_options[idx].max && (pOptSt->optCountArray[current_option] > pThisCmd->valid_options[idx].max))
			{
				SG_bool found = SG_FALSE;
				SG_ERR_CHECK(  SG_varray__find__int64(pCtx, pvaHandled, current_option, &found, NULL)  );
				if (!found)
				{
					SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pvaHandled, current_option)  );
					SG_ERR_IGNORE(  _print_too_many(pCtx, current_option, 
						pThisCmd->valid_options[idx].min, pThisCmd->valid_options[idx].max)  );
					bUsageError = SG_TRUE;
				}
			}
		}
	}

	/* Loop through valid options for this command */
	for (k=0; k < SG_CMDINFO_OPTION_LIMIT; k++)
	{
		cmdopt current_option = pThisCmd->valid_options[k];
		if (current_option.option == 0)
			break;

		/* If a non-zero max is specified for the command, verify that the option was specified at least that many times. */
		if ( current_option.min && (pOptSt->optCountArray[current_option.option] < current_option.min) )
		{
			SG_bool found = SG_FALSE;
			SG_ERR_CHECK(  SG_varray__find__int64(pCtx, pvaHandled, current_option.option, &found, NULL)  );
			if (!found)
			{
				SG_uint32 idx;
				SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pvaHandled, current_option.option)  );
				if ( _get_option_index(pThisCmd, current_option.option, &idx) )
				{
					SG_ERR_IGNORE(  _print_too_few(pCtx, current_option.option, 
						pThisCmd->valid_options[idx].min, pThisCmd->valid_options[idx].max)  );
				}
				else
				{
					SG_ASSERT(SG_FALSE);
				}
				bUsageError = SG_TRUE;
			}
		}
	}

	*pbUsageError = bUsageError;

	/* common cleanup */
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaHandled);
}

/**
 * Helper routine for verbs that are valid when zero, one, or two revisions are specified.
 */
void _get_zero_one_or_two_revision_specifiers(SG_context* pCtx,
											  const char* pszRepoName,
											  SG_rev_spec* pRevSpec,
											  char** ppszHid1,
											  char** ppszHid2)
{
	SG_stringarray* psaFullHids = NULL;

	SG_NULLARGCHECK_RETURN(ppszHid1);
	SG_NULLARGCHECK_RETURN(ppszHid2);

	if (pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_all(pCtx, pszRepoName, pRevSpec, SG_FALSE, &psaFullHids)  );
		if (psaFullHids)
		{
			SG_uint32 count, i;

			SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaFullHids, &count)  );
			for (i = 0; i < count; i++)
			{
				const char* pszRefHid;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaFullHids, i, &pszRefHid)  );
				if (i == 0)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszRefHid, ppszHid1)  );
				else if (i==1)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszRefHid, ppszHid2)  );
			}
		}
	}

	/* common cleanup */
fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaFullHids);
}

static void _get_zero_or_one_revision_specifiers(SG_context* pCtx, const char* pszRepoName, SG_rev_spec* pRevSpec, char** ppszHid1)
{
	SG_stringarray* psaFullHids = NULL;

	SG_NULLARGCHECK_RETURN(ppszHid1);

	if (pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_all(pCtx, pszRepoName, pRevSpec, SG_FALSE, &psaFullHids)  );
		if (psaFullHids)
		{
            SG_uint32 count = 0;
			SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaFullHids, &count)  );
            if (count)
            {
				const char* pszRefHid;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaFullHids, 0, &pszRefHid)  );
                SG_ERR_CHECK(  SG_STRDUP(pCtx, pszRefHid, ppszHid1)  );
            }
		}
	}

	/* common cleanup */
fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaFullHids);
}

void my_wrap_text(
        SG_context* pCtx,
        SG_string* pstr,
        SG_uint32 width,
        SG_string** ppstr
        )
{
	char** words = NULL;
    SG_uint32 count_words = 0;
    SG_string* pstr_result = NULL;
    SG_uint32 len_cur_line = 0;
    SG_uint32 i = 0;

	SG_NULLARGCHECK_RETURN(pstr);
	SG_ARGCHECK_RETURN(width > 8, width);

    // TODO 1000 for max substrings?
	SG_ERR_CHECK(  SG_string__split__asciichar(pCtx, pstr, ' ', 1000, &words, &count_words) );

    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_result)  );
    len_cur_line = 0;
    for (i=0; i<count_words; i++)
    {
        SG_uint32 len_cur_word = SG_STRLEN(words[i]);

        if ((len_cur_line + len_cur_word + 1) <= width)
        {
            if (len_cur_line)
            {
                SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_result, " %s", words[i])  );
            }
            else
            {
                SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_result, "%s", words[i])  );
            }
            len_cur_line += (1 + len_cur_word);
        }
        else
        {
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_result, "\n%s", words[i])  );
            len_cur_line = (1 + len_cur_word);
        }
    }

    *ppstr = pstr_result;
    pstr_result = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr_result);
	SG_STRINGLIST_NULLFREE(pCtx, words, count_words);
}


void _vv__delete_repo(SG_context * pCtx, const char * szRepoName)
{
	SG_ASSERT(pCtx!=NULL);
	SG_NONEMPTYCHECK_RETURN(szRepoName);

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Deleting %s...", szRepoName)  );

    SG_ERR_CHECK(  SG_repo__delete_repo_instance(pCtx, szRepoName)  );
    SG_ERR_CHECK(  SG_closet__descriptors__remove(pCtx, szRepoName)  );

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, " Done.\n")  );

	return;
fail:
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, " An error occurred deleting this repo.\n")  );
}

void do_cmd_blobcount(SG_context * pCtx, const char* psz_descriptor_name);

void do_cmd_repo(
	SG_context * pCtx,
	SG_bool plural,
	SG_uint32 count_args, const char ** paszArgs,
	SG_bool allBut, SG_bool noPrompt,
	SG_bool verbose,
	SG_bool listAll,
	const char * pszStorage, const char * pszHashMethod, const char * psz_shared_users,
	int * pExitStatus)
{
	SG_uint32 i;
	SG_stringarray * pHitList = NULL;
	SG_vhash * pDescriptors = NULL;
	SG_string * pStr = NULL;
	SG_string * pDescriptorName = NULL;
	SG_pathname * pPath = NULL;
	SG_pathname * pTopPath = NULL;

	SG_vhash* pvhDescriptor = NULL;
	SG_varray* pva = NULL;

	SG_repo * pRepo = NULL;
	char * sz = NULL;

	SG_ASSERT(pCtx!=NULL);

	if(count_args>=2 && strcmp(paszArgs[0],"delete")==0)
	{
		SG_bool proceed = SG_TRUE;

		if(verbose || listAll || pszStorage || pszHashMethod || psz_shared_users)
			SG_ERR_THROW(SG_ERR_USAGE);

		count_args-=1;
		paszArgs+=1;

		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pHitList, 1)  );

		// Validate all repo names
		for(i=0; i<count_args; ++i)
		{
			SG_bool bExists = SG_FALSE;
			SG_ERR_CHECK(  SG_closet__descriptors__exists(pCtx, paszArgs[i], &bExists)  );
			if (!bExists)
				SG_ERR_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", paszArgs[i]));
		}

		if(!allBut)
		{
			for(i=0; i<count_args; ++i)
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, pHitList, paszArgs[i])  );
		}
		else
		{
			SG_uint32 repo_count;
			SG_uint32 delete_count = 0;

			if(!noPrompt)
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Repositories:\n")  );

			SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pDescriptors)  );
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pDescriptors, &repo_count)  );
			for (i=0; i<repo_count; ++i) {
				SG_uint32 j;
				const char * szRepoName = NULL;
				SG_bool found_in_exception_list = SG_FALSE;
				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pDescriptors, i, &szRepoName, NULL)  );
				for (j=0; (j<count_args)&&(!found_in_exception_list); ++j)
					found_in_exception_list = (strcmp(szRepoName, paszArgs[j])==0);
				if(!found_in_exception_list)
				{
					SG_ERR_CHECK(  SG_stringarray__add(pCtx, pHitList, szRepoName)  );
					++delete_count;
					if(!noPrompt)
						SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "     %s\n", szRepoName)  );
				}
			}
			SG_VHASH_NULLFREE(pCtx, pDescriptors);

			if(!noPrompt && delete_count>0)
			{
				if(delete_count==1)
					SG_ERR_CHECK(  SG_console__ask_question__yn(pCtx, "Delete this repository?", SG_QA_DEFAULT_NO, &proceed)  );
				else
				{
					SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStr)  );
					SG_ERR_CHECK(  SG_string__append__format(pCtx, pStr, "Delete these %d repositories?", delete_count)  );
					SG_ERR_CHECK(  SG_console__ask_question__yn(pCtx, SG_string__sz(pStr), SG_QA_DEFAULT_NO, &proceed)  );
					SG_STRING_NULLFREE(pCtx, pStr);
				}
			}
			else if(!noPrompt)
			{
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "     (None)\n")  );
			}
		}

		if(proceed)
		{
			SG_uint32 count;
			const char * const * ppHitList = NULL;
			SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, pHitList, &ppHitList, &count)  );
			for (i=0; i<count; ++i)
			{
				_vv__delete_repo(pCtx, ppHitList[i]);
				if(SG_context__has_err(pCtx))
				{
					if(count>1)
					{
						SG_log__report_error__current_error(pCtx);
						SG_context__err_reset(pCtx);
					}
					else
						SG_ERR_RETHROW;
				}
			}
		}
		else
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Aborted. No repositories have been deleted.\n")  );
		}

		SG_STRINGARRAY_NULLFREE(pCtx, pHitList);
	}
	else if((count_args>=1 && strcmp(paszArgs[0],"list")==0) || (count_args==0 && plural))
	{
		SG_uint32 count = 0;

		if(count_args>1 || allBut || noPrompt || verbose || pszStorage || pszHashMethod || psz_shared_users)
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "'--list-all' is the only valid option for the 'list' subcommand."));

		if (listAll)
		{
			SG_uint32 maxlen = 10; // length of the header, "Repository"

			SG_ERR_CHECK(  SG_closet__descriptors__list__all(pCtx, &pDescriptors)  );

			SG_ERR_CHECK(  SG_vhash__count(pCtx, pDescriptors, &count)  );
			{
				const char* pszName = NULL;

				for (i=0; i<count; i++)
				{
					const char* pszName = NULL;
					SG_uint32 len;
					SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pDescriptors, i, &pszName, NULL)  );
					len = SG_STRLEN(pszName);
					if (len > maxlen)
						maxlen = len;
				}

				if (i)
				{
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%*s%20s\n", maxlen, "Repository", "Status")  );
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%*s%20s\n", maxlen, "----------", "------")  );
				}

				for (i=0; i<count; i++)
				{
					SG_vhash* pvhRefStatus = NULL;
					SG_int32 iStatus = -1;
					const char* pszRefStatus = "Unknown";

					SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pDescriptors, i, &pszName, &pvhRefStatus)  );
					SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvhRefStatus, SG_CLOSET_DESCRIPTOR_STATUS_KEY, &iStatus)  );
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhRefStatus, SG_CLOSET_DESCRIPTOR_STATUS_NAME_KEY, &pszRefStatus)  );
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%*s%20s\n", maxlen, pszName, pszRefStatus)  );
				}
			}

		}
		else
		{
			SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pDescriptors)  );

			SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pDescriptors, &pva)  );
			SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );

			for (i=0; i<count; i++)
			{
				const char* pRepoName = NULL;
				SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &pRepoName)  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", pRepoName)  );
			}

			SG_VARRAY_NULLFREE(pCtx, pva);
		}

		SG_VHASH_NULLFREE(pCtx, pDescriptors);
	}
	else if(count_args==2 && strcmp(paszArgs[0],"new")==0)
	{
		if(allBut || noPrompt || verbose || listAll)
			SG_ERR_THROW(SG_ERR_USAGE);

		if(pszHashMethod && psz_shared_users)
			SG_ERR_THROW(SG_ERR_USAGE);

		SG_ERR_CHECK(  SG_vv2__init_new_repo(pCtx, paszArgs[1], NULL, pszStorage, pszHashMethod, SG_TRUE, psz_shared_users, SG_FALSE, NULL, NULL)  );
	}
	else if((count_args>=1 && strcmp(paszArgs[0],"info")==0))
	{
		const char * pszDescriptorName = NULL;

		if(count_args>2 || allBut || noPrompt || listAll || pszStorage || pszHashMethod || psz_shared_users)
			SG_ERR_THROW(SG_ERR_USAGE);

		if(count_args<2)
		{
			SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPath);
			if(SG_context__err_equals(pCtx, SG_ERR_NOT_A_WORKING_COPY))
			{
				SG_context__err_reset(pCtx);
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "You are not currently inside a working copy.\n")  );
				*pExitStatus = 1;
				goto fail;
			}
			SG_ERR_CHECK_CURRENT;

			SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath, &pTopPath, &pDescriptorName, NULL)  );
			pszDescriptorName = SG_string__sz(pDescriptorName);

			SG_PATHNAME_NULLFREE(pCtx, pPath);

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Current repository:   %s\n", pszDescriptorName)  );
		}
		else
		{
			SG_bool isDescriptor=SG_FALSE;
			SG_ERR_CHECK(  SG_closet__descriptors__exists(pCtx, paszArgs[1], &isDescriptor)  );

			if(isDescriptor)
			{
				pszDescriptorName=paszArgs[1];
				SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDescriptorName, &pRepo)  );
			}
			else
			{
				SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, paszArgs[1])  );
				SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pPath);
				if(SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND) || SG_context__err_equals(pCtx, SG_ERR_NOT_A_DIRECTORY))
				{
					SG_context__err_reset(pCtx);
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "'%s' is not a valid repository name or directory path on disk.\n", paszArgs[1])  );
					*pExitStatus = 1;
					goto fail;
				}
				SG_ERR_CHECK_CURRENT;

				SG_workingdir__find_mapping(pCtx, pPath, &pTopPath, &pDescriptorName, NULL);
				if(SG_context__err_equals(pCtx, SG_ERR_NOT_A_WORKING_COPY))
				{
					SG_context__err_reset(pCtx);
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "The directory %s is not a working copy.\n", SG_pathname__sz(pPath))  );
					*pExitStatus = 1;
					goto fail;
				}
				SG_ERR_CHECK_CURRENT;

				pszDescriptorName = SG_string__sz(pDescriptorName);
				SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDescriptorName, &pRepo)  );

				SG_PATHNAME_NULLFREE(pCtx, pPath);
			}

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Repository:           %s\n", pszDescriptorName)  );
		}

		if(pTopPath!=NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Root of Working Copy: %s\n", SG_pathname__sz(pTopPath))  );
			SG_PATHNAME_NULLFREE(pCtx, pTopPath);
		}

		SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__PATHS_DEFAULT, pRepo, &sz, NULL);
		if(!SG_context__has_err(pCtx) && sz && *sz!='\0')
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Default push/pull:    %s\n", sz)  );
		}
		else
			SG_context__err_reset(pCtx);
		SG_NULLFREE(pCtx, sz);

		if(verbose)
		{
			SG_ERR_CHECK(  SG_closet__descriptors__get(pCtx, pszDescriptorName, NULL, &pvhDescriptor)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStr)  );
			SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvhDescriptor, pStr)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\nDescriptor:\n%s", SG_string__sz(pStr))  );
			SG_VHASH_NULLFREE(pCtx, pvhDescriptor);
			SG_STRING_NULLFREE(pCtx, pStr);

			SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pRepo, &sz)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\nHash method: %s\n", sz)  );
			SG_NULLFREE(pCtx, sz);

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\nBlob Count:\n")  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,   "==========================\n")  );
			SG_ERR_CHECK(  do_cmd_blobcount(pCtx, pszDescriptorName)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,   "==========================\n")  );
		}

		SG_STRING_NULLFREE(pCtx, pDescriptorName);
		SG_REPO_NULLFREE(pCtx, pRepo);
	}
	else if(count_args==3 && strcmp(paszArgs[0],"rename")==0)
	{
		SG_bool proceed = SG_TRUE;

		if(allBut || verbose || listAll || pszStorage || pszHashMethod || psz_shared_users)
			SG_ERR_THROW(SG_ERR_USAGE);

		if(!noPrompt)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, 
									  ("Renaming a repository breaks all associations with current working copies\n"
									   "of that repository.\n"
									   "\n"
									   "To create a working copy for the new name, perform a checkout.\n"
									   "\n"
									   "Additionally, no config settings (such as the default push/pull location)\n"
									   "will be migrated to the new name. Any desired repository-specific settings\n"
									   "must be set manually under the new name.\n"
									   "\n"
									   "To see the old settings, run:\n"
									   "    vv config show /instance/%s\n"
									   "\n"),
									  paszArgs[1])  );
			SG_ERR_CHECK(  SG_console__ask_question__yn(pCtx, "Are you sure you want to rename this repository?", SG_QA_DEFAULT_NO, &proceed)  );
		}

		if(proceed)
		{
			SG_ERR_CHECK(  SG_closet__descriptors__rename(pCtx, paszArgs[1], paszArgs[2])  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Rename complete.\n")  );
		}
		else
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Aborted. The repository was not renamed.\n")  );
		}

		SG_VHASH_NULLFREE(pCtx, pvhDescriptor);
	}
	else if(count_args==0)
	{
		const char * pszDescriptorName = NULL;
		
		SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPath);
		if(SG_context__err_equals(pCtx, SG_ERR_NOT_A_WORKING_COPY))
		{
			SG_context__err_reset(pCtx);
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "You are not currently inside a working copy.\n")  );
			*pExitStatus = 1;
			goto fail;
		}
		SG_ERR_CHECK_CURRENT;

		SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath, &pTopPath, &pDescriptorName, NULL)  );
		pszDescriptorName = SG_string__sz(pDescriptorName);

		SG_PATHNAME_NULLFREE(pCtx, pPath);

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", pszDescriptorName)  );
		
		if(pTopPath!=NULL)
		{
			SG_PATHNAME_NULLFREE(pCtx, pTopPath);
		}
		
		SG_STRING_NULLFREE(pCtx, pDescriptorName);
		SG_REPO_NULLFREE(pCtx, pRepo);
		
	}
	else
	{
		SG_ERR_THROW(SG_ERR_USAGE);
	}

	return;
fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pHitList);
	SG_VHASH_NULLFREE(pCtx, pDescriptors);
	SG_STRING_NULLFREE(pCtx, pStr);
	SG_STRING_NULLFREE(pCtx, pDescriptorName);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pTopPath);
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptor);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, sz);
}

// Forward declaration, because this and my_localsettings__join call each other.
static void my_localsettings__variant_to_string(
	SG_context* pCtx,
	const SG_variant* pValue,
	SG_int32 iArrayIndent,
	SG_string* pOutput
	);

/**
 * Structure used by my_localsettings__join as pCallerData.
 */
typedef struct
{
	SG_int32   iArrayIndent; //< Passed to my_localsettings__variant_to_string.
	SG_string* pGlue;        //< The glue string to join to join the values together.
	SG_string* pResult;      //< The joined result string.
}
my_localsettings__join__data;

/**
 * Joins all the values in an array together into one string, formatted for localsettings output.
 * Intended for use as a SG_varray_foreach_callback.
 */
static void my_localsettings__join(
	SG_context*       pCtx,        //< Error and context information.
	void*             pCallerData, //< An allocated instance of my_localsettings__join__data.
	                               //< Pass the same instance to all related calls to this function.
	const SG_varray*  pArray,      //< The array containing values being joined.
	SG_uint32         uIndex,      //< The index of the current value in the array.
	const SG_variant* pValue       //< The current value being joined to the others provided so far.
	)
{
	SG_string* pValueString = NULL;
	my_localsettings__join__data* pJoinData = NULL;

	SG_NULLARGCHECK(pCallerData);
	SG_UNUSED(pArray);
	SG_NULLARGCHECK(pValue);

	pJoinData = (my_localsettings__join__data*) pCallerData;

	if (uIndex > 0)
	{
		SG_ERR_CHECK(  SG_string__append__string(pCtx, pJoinData->pResult, pJoinData->pGlue)  );
	}

	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pValueString)  );
	SG_ERR_CHECK(  my_localsettings__variant_to_string(pCtx, pValue, pJoinData->iArrayIndent, pValueString)  );
	SG_ERR_CHECK(  SG_string__append__string(pCtx, pJoinData->pResult, pValueString)  );

fail:
	SG_STRING_NULLFREE(pCtx, pValueString);
}

/**
 * Validates and resolves an input setting name into a fully-qualified one.
 * Names that start with '/' are assumed to be fully-qualified already.
 * They will just be checked to ensure that they're in a valid scope.
 * Other names are placed in the MACHINE scope.
 */
static void my_localsettings__resolve_name(
	SG_context* pCtx,   //< Error and context information.
	const char* szName, //< The input name to resolve.
	SG_bool     bForce, //< True if names outside of the known scopes should be allowed.
	SG_string*  pOutput //< The resolved name string.
	)
{
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(pOutput);

	// check if the given name is global
	if (szName[0] == '/')
	{
		// only check that it's in a known scope if the force flag wasn't used
		if (!bForce)
		{
			SG_int32 iSlashCount = 0;
			SG_uint32 uCurrent = 0;

			// make sure the global name starts with a known scope
			if (0 == strncmp(szName, SG_LOCALSETTING__SCOPE__INSTANCE, strlen(SG_LOCALSETTING__SCOPE__INSTANCE)))
			{
				iSlashCount = 3;
			}
			else if (0 == strncmp(szName, SG_LOCALSETTING__SCOPE__REPO, strlen(SG_LOCALSETTING__SCOPE__REPO)))
			{
				iSlashCount = 3;
			}
			else if (0 == strncmp(szName, SG_LOCALSETTING__SCOPE__ADMIN, strlen(SG_LOCALSETTING__SCOPE__ADMIN)))
			{
				iSlashCount = 3;
			}
			else if (0 == strncmp(szName, SG_LOCALSETTING__SCOPE__MACHINE, strlen(SG_LOCALSETTING__SCOPE__MACHINE)))
			{
				iSlashCount = 2;
			}
			else
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Setting %s is in an unknown scope.", szName)  );
			}

			// make sure the name contains enough slashes for the scope
			// this ensures that "/instance/<name>" isn't used by itself
			while (szName[uCurrent] != '\0')
			{
				if (szName[uCurrent] == '/')
				{
					iSlashCount = iSlashCount - 1;
				}
				uCurrent = uCurrent + 1;
			}
			if (iSlashCount > 0)
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Setting %s isn't fully qualified.", szName)  );
			}
		}

		// the name looks good, copy it to the output
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, szName)  );
	}
	else
	{
		// prefix the local name with a default scope (machine, for now)
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, SG_LOCALSETTING__SCOPE__MACHINE)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pOutput, "/")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pOutput, szName)  );
	}

fail:
	return;
}

/**
 * Converts a settings variant value into a string suitable for display.
 */
static void my_localsettings__variant_to_string(
	SG_context*       pCtx,         //< Error and context information.
	const SG_variant* pValue,       //< The setting value to convert.
	                                //< Should not be a VHASH, recurse into them before calling this.
	SG_int32          iArrayIndent, //< Indicates how to handle the multiple values in an array.
	                                //< If non-negative, then values will be on their own line indented by this number of spaces.
	                                //< If negative, then values will just be separated with a comma.
	                                //< Nested arrays aren't really handled and haven't been tested.
	SG_string*        pOutput       //< The string to place the converted value into.
	)
{
	SG_string* pJoinGlue = NULL;

	SG_NULLARGCHECK(pOutput);

	if (pValue == NULL)
	{
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, "{UNSET}")  );
		return;
	}
	else
	{
		switch (pValue->type)
		{
			case SG_VARIANT_TYPE_NULL:
			{
				SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, "{NULL}")  );
				break;
			}
			case SG_VARIANT_TYPE_INT64:
			{
				SG_int64 value;
				SG_int_to_string_buffer szConvertBuffer;
				SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pValue, &value)  );
				SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, SG_int64_to_sz(value, szConvertBuffer))  );
				break;
			}
			case SG_VARIANT_TYPE_DOUBLE:
			{
				double value;
				SG_ERR_CHECK(  SG_variant__get__double(pCtx, pValue, &value)  );
				SG_ERR_CHECK(  SG_string__sprintf(pCtx, pOutput, "%g", value)  );
				break;
			}
			case SG_VARIANT_TYPE_BOOL:
			{
				SG_bool value;
				SG_ERR_CHECK(  SG_variant__get__bool(pCtx, pValue, &value)  );
				SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, value ? "True" : "False")  );
				break;
			}
			case SG_VARIANT_TYPE_SZ:
			{
				SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, pValue->v.val_sz)  );
				break;
			}
			case SG_VARIANT_TYPE_VARRAY:
			{
				my_localsettings__join__data JoinData;
				SG_uint32 uCount = 0u;

				// check the length of the array
				SG_ERR_CHECK(  SG_varray__count(pCtx, pValue->v.val_varray, &uCount)  );
				if (uCount == 0u)
				{
					// empty, that's easy
					SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, "{EMPTY LIST}")  );
				}
				else
				{
					// build the glue to join all the values together
					if (iArrayIndent < 0)
					{
						// the glue should just be a comma
						SG_ERR_CHECK(  SG_string__set__sz(pCtx, pJoinGlue, ", ")  );

						// use the next array indent if it goes recursive (this array contains arrays)
						JoinData.iArrayIndent = iArrayIndent - 1;
					}
					else
					{
						SG_int32 i = 0;

						// the glue should be a newline and the given indent
						SG_ERR_CHECK(  SG_string__alloc(pCtx, &pJoinGlue)  );
						SG_ERR_CHECK(  SG_string__set__sz(pCtx, pJoinGlue, "\n")  );
						for (i=0; i<iArrayIndent; ++i)
						{
							SG_ERR_CHECK(  SG_string__append__sz(pCtx, pJoinGlue, " ")  );
						}

						// use the next array indent if it goes recursive (this array contains arrays)
						JoinData.iArrayIndent = iArrayIndent + 4;
					}

					// join the array values together using the glue to give us our final display value
					JoinData.pGlue = pJoinGlue;
					JoinData.pResult = pOutput;
					SG_ERR_CHECK(  SG_varray__foreach(pCtx, pValue->v.val_varray, my_localsettings__join, &JoinData)  );
				}

				break;
			}
			case SG_VARIANT_TYPE_VHASH:
			{
				SG_uint32 uCount = 0u;

				// we should only ever end up seeing empty hash values
				// non-empty hashes should be recursed into instead of provided directly
				SG_ERR_CHECK(  SG_vhash__count(pCtx, pValue->v.val_vhash, &uCount)  );
				if (uCount > 0u)
				{
					SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, "{ERROR: Non-empty vhash}")  );
				}
				else
				{
					SG_ERR_CHECK(  SG_string__set__sz(pCtx, pOutput, "{EMPTY HASH}")  );
				}
				break;
			}
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, pJoinGlue);
}

/**
 * Structure used by my_localsettings__print_value as pCallerData.
 */
typedef struct
{
	SG_string* pLastScope; //< Stores the name of the last scope seen by the callback.
}
my_localsettings__print_value__data;

/**
 * Prints a single setting value.
 * Generally intended for use as a SG_localsettings_foreach_callback.
 */
static void my_localsettings__print_value(
	SG_context*       pCtx,       //< Error and context information.
	const char*       szFullName, //< The fully qualified name of the setting to print.
	const char*       szScope,    //< Just the scope of the setting being printed (including admin/repo ID or descriptor name, where relevant).
	const char*       szName,     //< The name of the setting without the scope.
	const SG_variant* pValue,     //< The value of the setting, or NULL if it has no value.
	void*             pCallerData //< NULL or an allocated instance of my_localsettings__print_value__data.
	                              //< If an instance is given and reused across several calls,
	                              //< then back-to-back settings in the same scope will be grouped together.
	)
{
	static const SG_uint32 iIndent = 4;
	SG_string* pValueString = NULL;
	my_localsettings__print_value__data* pData = NULL;
	SG_uint32 i = 0u;

	SG_UNUSED(szFullName);
	SG_NULLARGCHECK(szScope);
	SG_NULLARGCHECK(szName);

	pData = (my_localsettings__print_value__data*) pCallerData;
	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pValueString)  );

	// convert the value to a string
	// the extra +2 indent is for the colon and space in the output
	SG_ERR_CHECK(  my_localsettings__variant_to_string(pCtx, pValue, SG_STRLEN(szName) + iIndent + 2, pValueString)  );

	// if the name is empty, then the scope IS the full name
	// this is a weird forced setting where the top-level key doesn't have a hash value
	// but we still have to deal with it somehow
	if (szName[0] == '\0')
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s: %s\n", szScope, SG_string__sz(pValueString))  );
	}
	else
	{
		// if this is a different scope than last time, print it out and then remember it for next time
		if (pData == NULL || strcmp(SG_string__sz(pData->pLastScope), szScope) != 0)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s/\n", szScope)  );
			SG_ERR_CHECK(  SG_string__set__sz(pCtx, pData->pLastScope, szScope)  );
		}

		// print out the indent
		for (i=0u; i<iIndent; ++i)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, " ")  );
		}

		// print out the value
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s: %s\n", szName, SG_string__sz(pValueString))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pValueString);
}

/**
 * Prints the values of all settings whose names match a pattern.
 */
static void my_localsettings__print_values(
	SG_context* pCtx,            //< Error and context information.
	const char* szPattern,       //< The pattern to match setting names against, or NULL to not restrict by pattern.
	                             //< Patterns that start with "/" will only match the beginning of fully qualified names.
	                             //< Other patterns will match anywhere in non-qualified names.
	SG_bool     bIncludeDefaults //< If true, then factory default values will also be included.
	)
{
	my_localsettings__print_value__data PrintValueData = {NULL};

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &(PrintValueData.pLastScope))  );
	SG_ERR_CHECK(  SG_localsettings__foreach(pCtx, szPattern, bIncludeDefaults, my_localsettings__print_value, &PrintValueData)  );

fail:
	SG_STRING_NULLFREE(pCtx, PrintValueData.pLastScope);
}

/**
 * Prints the old and new values of a single setting.
 * Use this to display the results of manipulating a setting.
 */
static void my_localsettings__print_changed_value(
	SG_context* pCtx,       //< Error and context information.
	const char* szFullName, //< The fully qualified name of the changed setting.
	SG_variant* pOldValue,  //< The setting's value before it was changed.
	SG_variant* pNewValue   //< The setting's value after it was changed.
	)
{
	SG_string* pScopeName = NULL;
	SG_string* pSettingName = NULL;
	SG_string* pName = NULL;
	SG_uint32 uLength = 0u;
	SG_uint32 i = 0u;
	my_localsettings__print_value__data PrintValueData = {NULL};

	// allocate data
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &(PrintValueData.pLastScope))  );

	// split the full name into scope and setting names
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pScopeName)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pSettingName)  );
	SG_ERR_CHECK(  SG_localsettings__split_full_name(pCtx, szFullName, NULL, pScopeName, pSettingName)  );

	// print the old value
	SG_ERR_CHECK(  my_localsettings__print_value(pCtx, szFullName, SG_string__sz(pScopeName), SG_string__sz(pSettingName), pOldValue, &PrintValueData)  );

	// clear the setting name and build a new one that's just a right-aligned arrow: ->
	// if the pSettingName is empty, then the pScopeName is the full name we're displaying (and replacing)
	// (it's a weird forced edged case in localsettings where a top-level key doesn't have a vhash value)
	pName = pSettingName;
	uLength = SG_string__length_in_bytes(pName);
	if (uLength == 0u)
	{
		pName = pScopeName;
		uLength = SG_string__length_in_bytes(pName);
	}
	if (uLength >= 2u)
	{
		uLength = uLength - 2;
	}
	else if (uLength == 1u)
	{
		uLength = uLength - 1;
	}
	SG_ERR_CHECK(  SG_string__clear(pCtx, pName)  );
	for (i=0u; i<uLength; ++i)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pName, " ")  );
	}
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pName, "->")  );

	// print the new value
	SG_ERR_CHECK(  my_localsettings__print_value(pCtx, szFullName, SG_string__sz(pScopeName), SG_string__sz(pSettingName), pNewValue, &PrintValueData)  );

fail:
	SG_STRING_NULLFREE(pCtx, PrintValueData.pLastScope);
	SG_STRING_NULLFREE(pCtx, pScopeName);
	SG_STRING_NULLFREE(pCtx, pSettingName);
}

/**
 * Imports localsettings from a JSON file, replacing all existing settings.
 */
static void my_localsettings__import(
	SG_context* pCtx,     //< Error and context information.
	SG_bool     bVerbose, //< True if imported settings should be printed out.
	const char* psz_path  //< The path/filename to read the settings from, or NULL to read them from STDIN.
	)
{
	SG_uint32 len32 = 0;
	SG_uint64 len64 = 0;
	SG_vhash* pvh = NULL;
	SG_pathname* pPath = NULL;
	SG_file* pFile = NULL;
	SG_byte* p = NULL;
	SG_string* s = NULL;

	// check where to read the settings from
	if (psz_path == NULL) {
		// read from stdin
		SG_ERR_CHECK(  SG_string__alloc(pCtx, &s)  );
		SG_ERR_CHECK(  SG_console__read_stdin(pCtx, &s)  );
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &s, &p, NULL)  );
	} else {
		// read from the specified file

		// determine file length
		SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPath, psz_path)  );
		SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &len64, NULL)  );
		if (!SG_uint64__fits_in_uint32(len64))
		{
			SG_ERR_THROW(  SG_ERR_UNSPECIFIED  ); // TODO
		}
		len32 = (SG_uint32)len64;

		// read the file into a buffer
		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
		SG_ERR_CHECK(  SG_alloc(pCtx, 1,len32+1,&p)  );
		SG_ERR_CHECK(  SG_file__read(pCtx, pFile, len32, p, NULL)  );
		p[len32] = 0;
	}

	// parse the json data in the buffer to a vhash
	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvh, (const char*) p)  );
	SG_NULLFREE(pCtx, p);

	// import settings from the vhash
	SG_ERR_CHECK(  SG_localsettings__import(pCtx, pvh)  );

	// print out the results, if requested
	if (bVerbose == SG_TRUE)
	{
		SG_ERR_CHECK(  my_localsettings__print_values(pCtx, NULL, SG_FALSE)  );
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, p);
	SG_STRING_NULLFREE(pCtx, s);
}

/**
 * Exports localsettings to a JSON file.
 */
static void my_localsettings__export(
	SG_context* pCtx,            //< Error and context information.
	const char* psz_path,        //< The path/filename to write the settings to, or NULL to write them to STDOUT.
	SG_bool     bIncludeDefaults //< Whether or not default settings should be included in the export.
	)
{
	SG_vhash* pvh = NULL;
	const SG_vhash* pDefaults = NULL;
	SG_string* pstr = NULL;

	// dump settings to a vhash and then to a json string
	SG_ERR_CHECK(  SG_localsettings__list__vhash(pCtx,  &pvh )  );
	if (bIncludeDefaults == SG_TRUE)
	{
		// TODO 2012/12/21 Is this really necessary?  Should we add
		// TODO            an arg to SG_localsettings__list__vhash()
		// TODO            and let it take care of this?
		SG_ERR_CHECK(  SG_localsettings__factory__list__vhash(pCtx, &pDefaults)  );
		SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvh, SG_LOCALSETTING__SCOPE__DEFAULT + 1, pDefaults)  );
	}
	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr)  );
	SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh, pstr)  );
	SG_VHASH_NULLFREE(pCtx, pvh );

	// check where to dump the string
	if (psz_path == NULL) {
		// dump it to stdout
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", SG_string__sz(pstr))  );
	} else {
		// dump it to the specified file
		SG_ERR_CHECK(  SG_file__text__write(pCtx, psz_path, SG_string__sz(pstr))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_VHASH_NULLFREE(pCtx, pvh );
}

/**
 * print details for "list-ignores".
 *
 */
static void my_localsettings__list_ignores(SG_context * pCtx)
{
	SG_vhash * pvhInfo = NULL;
	SG_varray * pvaIgnores = NULL;		// we do not own this

	SG_ERR_CHECK(  SG_wc__get_wc_info(pCtx, NULL, &pvhInfo)  );
	SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pvhInfo, "ignores", &pvaIgnores)  );
	if (pvaIgnores)
	{
		SG_uint32 k, count;

		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaIgnores, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;

			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaIgnores, k, &psz_k)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", psz_k)  );
		}
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhInfo);
}

void do_cmd_linehistory(SG_context * pCtx,
					const SG_option_state * pOptSt,
					const SG_stringarray * psaArgs)
{
	SG_varray * pvaResults = NULL;
	SG_uint32 count = 0;
	const char * pszPath = NULL;

	//assert(psaArgs && psaArgs->count>0 && (psaArgs[k] for all k))
	SG_NULLARGCHECK(psaArgs);
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaArgs, &count)  );
	SG_ARGCHECK( count > 0, "line history needs a file argument");
	SG_ARGCHECK( count <= 1, "line history can only be called on one file at a time");
	
	SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaArgs, 0, &pszPath)  );
	SG_ERR_CHECK(  SG_wc__line_history(pCtx, NULL, pszPath, pOptSt->iStartLine, pOptSt->iLength, &pvaResults )  );
	if (pvaResults)
	{
		SG_ERR_CHECK(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, pvaResults, "results")  );
	}
		

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
}

/**
 * Handles the "localsettings" command.
 */
void do_cmd_localsettings(
	SG_context*  pCtx,      //< Error and context information.
	SG_bool      bVerbose,  //< Whether or not the global verbose flag was used.
	SG_bool      bForce,    //< Whether or not the global force flag was used.
	SG_uint32    uArgCount, //< The number of arguments to the command.
	const char** pszArgs    //< The arguments to the command.
	)
{
	SG_string* pName = NULL;
	SG_variant* pOldValue = NULL;
	SG_variant* pNewValue = NULL;

	// if we have any arguments, then pszArgs[0] is the subcommand.
	if (uArgCount == 0u)
	{
		SG_ERR_CHECK(  my_localsettings__print_values(pCtx, NULL, bVerbose)  );
	}
	else if (0 == strcmp(pszArgs[0], "export"))
	{
		if (uArgCount > 2)
		{
			SG_ERR_THROW(  SG_ERR_USAGE  );
		}
		SG_ERR_CHECK(  my_localsettings__export(pCtx, uArgCount == 1 ? NULL : pszArgs[1], bVerbose)  );
	}
	else if (0 == strcmp(pszArgs[0], "import"))
	{
		if (uArgCount > 2)
		{
			SG_ERR_THROW(  SG_ERR_USAGE  );
		}
		SG_ERR_CHECK(  my_localsettings__import(pCtx, bVerbose, uArgCount == 1 ? NULL : pszArgs[1])  );
	}
	else if (0 == strcmp(pszArgs[0], "defaults"))
	{
		if (uArgCount > 1)
		{
			SG_ERR_THROW(  SG_ERR_USAGE  );
		}
		SG_ERR_CHECK(  my_localsettings__print_values(pCtx, SG_LOCALSETTING__SCOPE__DEFAULT, SG_TRUE)  );
	}
	else if (0 == strcmp(pszArgs[0], "show"))
	{
		if (uArgCount > 2)
		{
			SG_ERR_THROW(  SG_ERR_USAGE  );
		}
		SG_ERR_CHECK(  my_localsettings__print_values(pCtx, uArgCount > 1 ? pszArgs[1] : NULL, bVerbose)  );
	}
	else if (0 == strcmp(pszArgs[0], "list-ignores"))
	{
		// TODO 2011/07/16 I'm not sure that this command belongs here
		// TODO            now that I've added the .sgignores stuff.
		// TODO            It was originally here to show the net result
		// TODO            from a SG_localsettings__get__collapsed_varray()
		// TODO            but I removed that function.

		if (uArgCount > 2)
		{
			SG_ERR_THROW(  SG_ERR_USAGE  );
		}

		SG_ERR_CHECK(  my_localsettings__list_ignores(pCtx)  );
	}
	else
	{
		SG_int64	intValue	= 0;
		// for all of the other valid subcommands
		// paszArgs[1] must be a setting path that we're going to manipulate
		if (uArgCount < 2 || ((0 != strcmp(pszArgs[0], "set"))  
							  && (0 != strcmp(pszArgs[0], "reset"))  
							  && (0 != strcmp(pszArgs[0], "empty"))  
							  && (0 != strcmp(pszArgs[0], "add-to"))
							  && (0 != strcmp(pszArgs[0], "remove-from"))  )  )
		{
			SG_ERR_THROW(  SG_ERR_USAGE  );
		}


		// resolve the setting path to something that we know is usable
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pName)  );
		SG_ERR_CHECK(  my_localsettings__resolve_name(pCtx, pszArgs[1], bForce, pName)  );

		// retrieve the setting's current value
		SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, SG_string__sz(pName), NULL, &pOldValue, NULL)  );

		// manipulate the setting according to the command used
		if (0 == strcmp(pszArgs[0], "set"))
		{
			if (uArgCount != 3)
			{
				SG_ERR_THROW(  SG_ERR_USAGE  );
			}
			if (pOldValue != NULL  && pOldValue->type == SG_VARIANT_TYPE_SZ)
			{
				SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, SG_string__sz(pName), pszArgs[2])  );
			}
			else if (pOldValue != NULL  && pOldValue->type == SG_VARIANT_TYPE_INT64)
			{
				SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &intValue, pszArgs[2])  );
				SG_ERR_CHECK(  SG_localsettings__update__int64(pCtx, SG_string__sz(pName), intValue)  );
			}
			else if (pOldValue == NULL)
			{
				//assume string
				SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, SG_string__sz(pName), pszArgs[2])  );
			}
			else
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Setting type is not a string or an integer")  );
			}
			
		}
		else if (0 == strcmp(pszArgs[0], "reset")) // TODO delete?  remove?
		{
			if (uArgCount != 2)
			{
				SG_ERR_THROW(  SG_ERR_USAGE  );
			}
			SG_ERR_CHECK(  SG_localsettings__reset(pCtx, SG_string__sz(pName))  );
		}
		else if (0 == strcmp(pszArgs[0], "empty"))
		{
			if (uArgCount != 2)
			{
				SG_ERR_THROW(  SG_ERR_USAGE  );
			}
			if (pOldValue != NULL && pOldValue->type != SG_VARIANT_TYPE_VARRAY)
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Setting type is not an array")  );
			}
			SG_ERR_CHECK(  SG_localsettings__varray__empty(pCtx, SG_string__sz(pName))  );
		}
		else if (0 == strcmp(pszArgs[0], "add-to"))
		{
			SG_uint32 i;
			if (uArgCount < 3)
			{
				SG_ERR_THROW(  SG_ERR_USAGE  );
			}
			if (pOldValue != NULL && pOldValue->type != SG_VARIANT_TYPE_VARRAY)
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Setting type is not an array")  );
			}
			for(i=2;i<uArgCount;++i)
			{
				SG_ERR_CHECK(  SG_localsettings__varray__append(pCtx, SG_string__sz(pName), pszArgs[i])  );
			}
		}
		else if (0 == strcmp(pszArgs[0], "remove-from"))
		{
			SG_uint32 i;
			if (uArgCount < 3)
			{
				SG_ERR_THROW(  SG_ERR_USAGE  );
			}
			if (pOldValue != NULL && pOldValue->type != SG_VARIANT_TYPE_VARRAY)
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Setting type is not an array")  );
			}
			for(i=2;i<uArgCount;++i)
			{
				SG_ERR_CHECK(  SG_localsettings__varray__remove_first_match(pCtx, SG_string__sz(pName), pszArgs[i])  );
			}
		}
		else
		{
			SG_ERR_THROW(  SG_ERR_USAGE  );
		}

		// get the setting's new value
		SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, SG_string__sz(pName), NULL, &pNewValue, NULL)  );

		// print out the old and new values to show the results
		SG_ERR_CHECK(  my_localsettings__print_changed_value(pCtx, SG_string__sz(pName), pOldValue, pNewValue)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pName);
	SG_VARIANT_NULLFREE(pCtx, pOldValue);
	SG_VARIANT_NULLFREE(pCtx, pNewValue);
}



#if 0
void do_cmd_groups(SG_context * pCtx, const char* psz_descriptor_name)
{
    SG_repo* pRepo = NULL;
	SG_pathname* pPathCwd = NULL;
    SG_varray* pva_groups = NULL;
    SG_uint32 i = 0;
    SG_uint32 count = 0;

    if (psz_descriptor_name)
    {
        SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );
    }

    SG_ERR_CHECK(  SG_group__list(pCtx, pRepo, &pva_groups)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_groups, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_name = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_groups, i, &pvh)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "name", &psz_name)  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", psz_name)  );
    }

    /* fall through */

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_groups);
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_REPO_NULLFREE(pCtx, pRepo);
}

void do_cmd_creategroup(SG_context * pCtx, const char* psz_name, const char* psz_descriptor_name)
{
    SG_repo* pRepo = NULL;
	SG_pathname* pPathCwd = NULL;

    if (psz_descriptor_name)
    {
        SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );
    }

    SG_ERR_CHECK(  SG_group__create(pCtx, pRepo, psz_name)  );

    /* fall through */

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_REPO_NULLFREE(pCtx, pRepo);
}

#define MY_CMD_GROUP_OP__LIST_SHALLOW      (1)
#define MY_CMD_GROUP_OP__LIST_DEEP         (2)
#define MY_CMD_GROUP_OP__ADD_USERS         (3)
#define MY_CMD_GROUP_OP__REMOVE_USERS      (4)
#define MY_CMD_GROUP_OP__ADD_SUBGROUPS     (5)
#define MY_CMD_GROUP_OP__REMOVE_SUBGROUPS  (6)

void do_cmd_group(SG_context * pCtx, const char* psz_group_name, const char* psz_descriptor_name, SG_uint32 op, const char** paszNames, SG_uint32 count_names)
{
    SG_repo* pRepo = NULL;
	SG_pathname* pPathCwd = NULL;
    SG_varray* pva = NULL;
    SG_vhash* pvh = NULL;
    SG_vhash* pvh_group = NULL;

    if (psz_descriptor_name)
    {
        SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );
    }

    switch (op)
    {
        case MY_CMD_GROUP_OP__LIST_DEEP:
            {
                SG_uint32 count_members = 0;
                SG_uint32 i = 0;

                if (count_names)
                {
                    // TODO error
                }

                SG_ERR_CHECK(  SG_group__list_all_users(pCtx, pRepo, psz_group_name, &pvh)  );
                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count_members)  );
                for (i=0; i<count_members; i++)
                {
                    const char* psz_userid = NULL;
                    SG_vhash* pvh_user = NULL;
                    const SG_variant* pv = NULL;
                    const char* psz_username = NULL;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &psz_userid, &pv)  );
                    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_user)  );

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, "name", &psz_username)  );
                    SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", psz_username)  );
                }
            }
            break;

        case MY_CMD_GROUP_OP__LIST_SHALLOW:
            {
                SG_uint32 count_members = 0;
                SG_uint32 i = 0;

                if (count_names)
                {
                    // TODO error
                }

                SG_ERR_CHECK(  SG_group__get_by_name(pCtx, pRepo, psz_group_name, &pvh_group)  );
                if (pvh_group)
                {
                    SG_varray* pva_members = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_group, "users", &pva_members)  );
                    if (pva_members)
                    {
                        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_members, &count_members)  );
                        for (i=0; i<count_members; i++)
                        {
                            SG_vhash* pvh_member = NULL;
                            const char* psz_member_name = NULL;

                            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_members, i, &pvh_member)  );
                            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_member, "name", &psz_member_name)  );
                            SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s: %s\n", "user", psz_member_name)  );
                        }
                    }

                    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_group, "subgroups", &pva_members)  );
                    if (pva_members)
                    {
                        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_members, &count_members)  );
                        for (i=0; i<count_members; i++)
                        {
                            SG_vhash* pvh_member = NULL;
                            const char* psz_member_name = NULL;

                            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_members, i, &pvh_member)  );
                            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_member, "name", &psz_member_name)  );
                            SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s: %s\n", "subgroup", psz_member_name)  );
                        }
                    }
                    SG_VHASH_NULLFREE(pCtx, pvh_group);
                }
                else
                {
                }
            }
            break;

        case MY_CMD_GROUP_OP__ADD_USERS:
            {
                SG_ERR_CHECK(  SG_group__add_users(pCtx, pRepo, psz_group_name, paszNames, count_names)  );
            }
            break;

        case MY_CMD_GROUP_OP__REMOVE_USERS:
            {
                SG_ERR_CHECK(  SG_group__remove_users(pCtx, pRepo, psz_group_name, paszNames, count_names)  );
            }
            break;

        case MY_CMD_GROUP_OP__ADD_SUBGROUPS:
            {
                SG_ERR_CHECK(  SG_group__add_subgroups(pCtx, pRepo, psz_group_name, paszNames, count_names)  );
            }
            break;

        case MY_CMD_GROUP_OP__REMOVE_SUBGROUPS:
            {
                SG_ERR_CHECK(  SG_group__remove_subgroups(pCtx, pRepo, psz_group_name, paszNames, count_names)  );
            }
            break;

        default:
            {
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            break;
    }

    /* fall through */

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_group);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_REPO_NULLFREE(pCtx, pRepo);
}
#endif

static void do_cmd_set_whoami(SG_context * pCtx, const char* psz_username, const char* psz_descriptor_name, SG_bool bCreate)
{
    SG_ERR_CHECK(  SG_vv2__whoami(pCtx, psz_descriptor_name, psz_username, bCreate, NULL)  );
    /* fall through */

fail:
	;
}

static void do_cmd_show_whoami(SG_context * pCtx, const char* psz_descriptor_name)
{
	char * psz_username = NULL;

	SG_ERR_CHECK(  SG_vv2__whoami(pCtx, psz_descriptor_name, NULL, SG_FALSE, &psz_username)  );
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", psz_username)  );

fail:
	SG_NULLFREE(pCtx, psz_username);
}

void do_cmd_user(SG_context * pCtx, SG_bool plural, const char* psz_descriptor_name, SG_bool listInactive, SG_uint32 argc, const char ** argv)
{
	SG_repo* pRepo = NULL;
	SG_pathname* pPathCwd = NULL;
	SG_varray* pva_users = NULL;
	SG_uint32 i = 0;
	SG_uint32 count = 0;
	SG_vhash * pvh_user = NULL;

	if(argc==2 && !strcmp(argv[0],"create"))
	{
		if (psz_descriptor_name)
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
		else
			SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );

        SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, argv[1], &pvh_user)  );
        if (pvh_user)
        {
            SG_VHASH_NULLFREE(pCtx, pvh_user);
            SG_ERR_THROW(SG_ERR_USER_ALREADY_EXISTS);
        }
		SG_ERR_CHECK(  SG_user__create(pCtx, pRepo, argv[1], NULL)  );
	}
	else if(argc==3 && !strcmp(argv[0],"rename"))
	{
		const char * szUserid = NULL;
		const char * szOldUsername = NULL;

		if (psz_descriptor_name)
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
		else
			SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );

		SG_user__lookup_by_userid(pCtx, pRepo, NULL, argv[1], &pvh_user);
		if(!SG_CONTEXT__HAS_ERR(pCtx) && pvh_user!=NULL)
		{
			szUserid = argv[1];
		}
		else
		{
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_USERNOTFOUND);
			SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, argv[1], &pvh_user)  );
			if(pvh_user==NULL){SG_ERR_THROW2(SG_ERR_USERNOTFOUND,(pCtx,"%s",argv[1]));}
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, "recid", &szUserid)  );
		}
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, "name", &szOldUsername)  );

		if(strcmp(szOldUsername, argv[2]))
			SG_ERR_CHECK(  SG_user__rename(pCtx, pRepo, szUserid, argv[2])  );
	}
	else if((argc==1 && !strcmp(argv[0],"list")) || (argc==0 && plural))
	{
		if (psz_descriptor_name)
		{
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );
		}

		SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pva_users)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pva_users, &count)  );
		for (i=0; i<count; i++)
		{
			SG_vhash* pvh = NULL;
			const char* psz = NULL;
			SG_bool inactive = SG_FALSE;
			SG_bool has = SG_FALSE;
			const char *extra = "";
			const char* psz_recid = NULL;

			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_users, i, &pvh)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "recid", &psz_recid)  );

			if (! strcmp(psz_recid,SG_NOBODY__USERID))
				continue;

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh, "inactive", &has)  );

			if (has)
				SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh, "inactive", &inactive)  );

			if (inactive)
				extra = " (inactive)";

			if (listInactive || ! inactive)
			{
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "name", &psz)  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s%s\n", psz, extra)  );

				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "recid", &psz)  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    %s\n", psz)  );
			}
		}
	}
	else if(argc==2 && ((!strcmp(argv[0],"set-active")) || (!strcmp(argv[0],"set-inactive"))))
	{
		const char *userid = NULL;
		SG_bool active = (strcmp(argv[0], "set-inactive") ? SG_TRUE : SG_FALSE);

		if (psz_descriptor_name)
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
		else
			SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );

        SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, argv[1], &pvh_user)  );
        if (! pvh_user)
        {
            SG_ERR_THROW(SG_ERR_USERNOTFOUND);
        }

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, SG_ZING_FIELD__RECID, &userid)  );

		SG_ERR_CHECK(  SG_user__set_active(pCtx, pRepo, userid, active)  );
	}
	else if (argc == 0)
	{
		SG_ERR_CHECK(  do_cmd_show_whoami(pCtx, psz_descriptor_name)  );
	}
	else if(argc == 2 && strcmp(argv[0], "set") == 0)
	{
		SG_ERR_CHECK(  do_cmd_set_whoami(pCtx, argv[1], psz_descriptor_name, SG_FALSE)  );
	}
	else
	{
		SG_ERR_THROW(SG_ERR_USAGE);
	}

	/* fall through */

fail:
	SG_VARRAY_NULLFREE(pCtx, pva_users);
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvh_user);
}

void do_cmd_version__remote(SG_context * pCtx, const char * szUrl)
{
	SG_string * pUrl = NULL;
	SG_curl* pCurl = NULL;
	SG_string* pstrResponse = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pUrl, szUrl)  );

	if(SG_string__sz(pUrl)[SG_string__length_in_bytes(pUrl)-1]!='/')
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pUrl, "/")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pUrl, "version.txt")  );

	SG_ERR_CHECK_RETURN(  SG_curl__alloc(pCtx, &pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_URL, SG_string__sz(pUrl))  );

	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pCurl, pstrResponse)  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pCurl)  );

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s - %s\n", szUrl, SG_string__sz(pstrResponse))  );
	
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	SG_CURL_NULLFREE(pCtx, pCurl);
	SG_STRING_NULLFREE(pCtx, pUrl);

	return;
fail:
	SG_STRING_NULLFREE(pCtx, pUrl);
	SG_CURL_NULLFREE(pCtx, pCurl);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
}

void do_cmd_blobcount(SG_context * pCtx, const char* psz_descriptor_name)
{
    SG_repo* pRepo = NULL;

    SG_uint32 count_blobs__all = 0;
    SG_uint64 len_encoded__all = 0;
    SG_uint64 len_full__all = 0;
    SG_uint64 max_len_encoded__all = 0;
    SG_uint64 max_len_full__all = 0;

    SG_uint32 count_blobs__full = 0;
    SG_uint64 len_encoded__full = 0;
    SG_uint64 len_full__full = 0;
    SG_uint64 max_len_encoded__full = 0;
    SG_uint64 max_len_full__full = 0;

    SG_uint32 count_blobs__alwaysfull = 0;
    SG_uint64 len_encoded__alwaysfull = 0;
    SG_uint64 len_full__alwaysfull = 0;
    SG_uint64 max_len_encoded__alwaysfull = 0;
    SG_uint64 max_len_full__alwaysfull = 0;

    SG_uint32 count_blobs__zlib = 0;
    SG_uint64 len_encoded__zlib = 0;
    SG_uint64 len_full__zlib = 0;
    SG_uint64 max_len_encoded__zlib = 0;
    SG_uint64 max_len_full__zlib = 0;

    SG_uint32 count_blobs__vcdiff = 0;
    SG_uint64 len_encoded__vcdiff = 0;
    SG_uint64 len_full__vcdiff = 0;
    SG_uint64 max_len_encoded__vcdiff = 0;
    SG_uint64 max_len_full__vcdiff = 0;

    SG_int_to_string_buffer buf;
    SG_blobset* pbs = NULL;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepo, 0, 0, 0, &pbs)  );

    SG_ERR_CHECK(  SG_blobset__get_stats(
                pCtx,
                pbs,
                0,
                &count_blobs__all,
                &len_encoded__all,
                &len_full__all,
                &max_len_encoded__all,
                &max_len_full__all
                )  );

    SG_ERR_CHECK(  SG_blobset__get_stats(
                pCtx,
                pbs,
                SG_BLOBENCODING__FULL,
                &count_blobs__full,
                &len_encoded__full,
                &len_full__full,
                &max_len_encoded__full,
                &max_len_full__full
                )  );

    SG_ERR_CHECK(  SG_blobset__get_stats(
                pCtx,
                pbs,
                SG_BLOBENCODING__ALWAYSFULL,
                &count_blobs__alwaysfull,
                &len_encoded__alwaysfull,
                &len_full__alwaysfull,
                &max_len_encoded__alwaysfull,
                &max_len_full__alwaysfull
                )  );

    SG_ERR_CHECK(  SG_blobset__get_stats(
                pCtx,
                pbs,
                SG_BLOBENCODING__ZLIB,
                &count_blobs__zlib,
                &len_encoded__zlib,
                &len_full__zlib,
                &max_len_encoded__zlib,
                &max_len_full__zlib
                )  );

    SG_ERR_CHECK(  SG_blobset__get_stats(
                pCtx,
                pbs,
                SG_BLOBENCODING__VCDIFF,
                &count_blobs__vcdiff,
                &len_encoded__vcdiff,
                &len_full__vcdiff,
                &max_len_encoded__vcdiff,
                &max_len_full__vcdiff
                )  );

    SG_BLOBSET_NULLFREE(pCtx, pbs);

    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "full\n")  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d\n", "count", count_blobs__full)  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "total", SG_int64_to_sz(len_encoded__full, buf))  );

    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "alwaysfull\n")  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d\n", "count", count_blobs__alwaysfull)  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "total", SG_int64_to_sz(len_encoded__alwaysfull, buf))  );

    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "zlib\n")  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d\n", "count", count_blobs__zlib)  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "full", SG_int64_to_sz(len_full__zlib,buf))  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "encoded", SG_int64_to_sz(len_encoded__zlib,buf))  );
    if (len_full__zlib)
    {
      SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d%%\n", "saved", (int) ((len_full__zlib - len_encoded__zlib) / (double) len_full__zlib * 100.0))  );
    }

    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "vcdiff\n")  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d\n", "count", count_blobs__vcdiff)  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "full", SG_int64_to_sz(len_full__vcdiff,buf))  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "encoded", SG_int64_to_sz(len_encoded__vcdiff,buf))  );
    if (len_full__vcdiff)
    {
      SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d%%\n", "saved", (int) ((len_full__vcdiff - len_encoded__vcdiff) / (double) len_full__vcdiff * 100.0))  );
    }

    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "all\n")  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d\n", "count", count_blobs__all)  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "full", SG_int64_to_sz(len_full__all,buf))  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %12s\n", "encoded", SG_int64_to_sz(len_encoded__all,buf))  );
    SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%12s  %d%%\n", "saved", (int) ((len_full__all - len_encoded__all) / (double) len_full__all * 100.0))  );

    /* fall through */

fail:
    SG_REPO_NULLFREE(pCtx, pRepo);
}

void do_cmd_zip(SG_context * pCtx, SG_option_state* pOptSt, const char* psz_path)
{
	SG_repo* pRepo = NULL;
	SG_pathname* pPathCwd = NULL;
	char* pszHidCs = NULL;
	SG_uint32 countRevSpecs = 0;

	SG_ERR_CHECK(  SG_cmd_util__get_repo_from_options_or_cwd(pCtx, pOptSt, &pRepo, &pPathCwd)  );
	if (pOptSt->psz_repo)
	{
		SG_ASSERT( (pPathCwd == NULL) );
	}

	if (pOptSt->pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
	}
	
	if (pOptSt->psz_repo && (countRevSpecs == 0))
		SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "You must specify a revision to zip when using --repo.")  );
	
	if (countRevSpecs)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_TRUE, &pszHidCs, NULL)  );
	}
	else
	{
		SG_bool bHasMerge = SG_FALSE;
		SG_ERR_CHECK(  SG_wc__get_wc_baseline(pCtx, NULL, &pszHidCs, &bHasMerge)  );		// assumes CWD-based pRepo

		if (bHasMerge)
			SG_ERR_THROW2(  SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE,
							(pCtx, "You must specify a revision to zip when you have an uncommitted merge.")  );
	}

    SG_ERR_CHECK_RETURN(  SG_repo__zip(pCtx, pRepo, pszHidCs, psz_path)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_NULLFREE(pCtx, pszHidCs);
}

void do_cmd_clone(
        SG_context * pCtx, 
        const char* psz_existing, 
        const char* psz_new,
        SG_option_state * pOptSt,
        const char* psz_username,
        const char* psz_password,
		SG_bool* pbNewIsRemote
        )
{
	SG_bool bNewIsRemote = SG_FALSE;
    struct SG_clone__params__all_to_something ats;
    struct SG_clone__demands demands;
	SG_vhash* pvhCloneResult = NULL;

    memset(&ats, 0, sizeof(ats));
    ats.low_pass = 0;
    ats.high_pass = 0;

    SG_ERR_CHECK(  SG_clone__init_demands(pCtx, &demands)  );
    demands.zlib_savings_over_full = -1;
    demands.vcdiff_savings_over_full = -1;
    demands.vcdiff_savings_over_zlib = -1;

	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, psz_new, &bNewIsRemote)  );
    if (bNewIsRemote)
    {
        SG_ERR_CHECK(  SG_clone__to_remote(
                pCtx, 
                psz_existing, 
                psz_username, 
                psz_password, 
                psz_new,
				&pvhCloneResult
                )  );
    }
    else
    {
        if (pOptSt->b_all_full)
        {
            ats.new_encoding = SG_BLOBENCODING__KEEPFULLFORNOW;
            SG_ERR_CHECK(  SG_clone__to_local(
                        pCtx, 
                        psz_existing, 
                        psz_username,
                        psz_password,
                        psz_new, 
                        &ats,
                        NULL,
                        &demands
                        )  );
        }
        else if (pOptSt->b_all_zlib)
        {
            ats.new_encoding = SG_BLOBENCODING__ZLIB;
            SG_ERR_CHECK(  SG_clone__to_local(
                        pCtx, 
                        psz_existing, 
                        psz_username,
                        psz_password,
                        psz_new, 
                        &ats,
                        NULL,
                        &demands
                        )  );
        }
        else if (pOptSt->bPack)
        {
            struct SG_clone__params__pack params_pack;

            params_pack.nKeyframeDensity = SG_PACK_DEFAULT__KEYFRAMEDENSITY; 
            params_pack.nRevisionsToLeaveFull = SG_PACK_DEFAULT__REVISIONSTOLEAVEFULL; 
            params_pack.nMinRevisions = SG_PACK_DEFAULT__MINREVISIONS;
            params_pack.low_pass = 0;
            params_pack.high_pass = 0;

            SG_ERR_CHECK(  SG_clone__to_local(
                        pCtx, 
                        psz_existing, 
                        psz_username,
                        psz_password,
                        psz_new, 
                        NULL,
                        &params_pack,
                        &demands
                        )  );
        }
        else
        {
            SG_ERR_CHECK(  SG_clone__to_local(
                        pCtx, 
                        psz_existing, 
                        psz_username, 
                        psz_password, 
                        psz_new,
                        NULL,
                        NULL,
                        NULL
                        )  );
        }
    }

	// Save default push/pull location for the new repository instance.
	if (!bNewIsRemote)
	{
		SG_ERR_CHECK(  SG_localsettings__descriptor__update__sz(pCtx, psz_new, SG_LOCALSETTING__PATHS_DEFAULT, psz_existing)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Use 'vv checkout %s <path>' to get a working copy.\n", psz_new)  );
		if(psz_username)
			SG_ERR_CHECK(  do_cmd_set_whoami(pCtx, psz_username, psz_new, SG_FALSE)  );
	}

	if (pvhCloneResult)
	{
		SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
		SG_ERR_CHECK(  SG_vhash__check__int32(pCtx, pvhCloneResult, SG_SYNC_CLONE_TO_REMOTE_KEY__STATUS, (SG_int32*)&status)  );
		if (SG_REPO_STATUS__NEED_USER_MAP == status)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n"
				"NOTE: The clone was successful, but you must import\n"
				"      the new repository before it can be used.\n"
				"      Click 'map users' on the repositories page.\n\n")  );
		}
	}
	
	if (pbNewIsRemote)
		*pbNewIsRemote = bNewIsRemote;

	/* fall through*/
fail:
	SG_VHASH_NULLFREE(pCtx, pvhCloneResult);
}


void do_zingmerge(SG_context * pCtx, const char* psz_descriptor_name)
{
    SG_repo* pRepo = NULL;
	SG_pathname* pPathCwd = NULL;
    char* psz_admin_id = NULL;
    SG_varray* pva_log = NULL;

    if (psz_descriptor_name)
    {
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );
    }

    SG_ERR_CHECK(  SG_zing__auto_merge_all_dags(pCtx, pRepo)  );

    /* fall through */

fail:
    SG_NULLFREE(pCtx, psz_admin_id);
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VARRAY_NULLFREE(pCtx, pva_log);
}

void do_reindex_all(SG_context * pCtx)
{
    SG_repo* pRepo = NULL;
    SG_vhash* pvh_descriptors = NULL;
    SG_uint32 repo_count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pvh_descriptors)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_descriptors, &repo_count)  );
    for (i=0; i<repo_count; ++i) 
    {
        const char * psz_descriptor_name = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_descriptors, i, &psz_descriptor_name, NULL)  );
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
        SG_ERR_CHECK(  SG_repo__rebuild_indexes(pCtx, pRepo)  );
        SG_REPO_NULLFREE(pCtx, pRepo);
    }

    /* fall through */

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_descriptors);
    SG_REPO_NULLFREE(pCtx, pRepo);
}

void do_reindex(SG_context * pCtx, const char* psz_descriptor_name)
{
    SG_repo* pRepo = NULL;
	SG_pathname* pPathCwd = NULL;

    if (psz_descriptor_name)
    {
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_descriptor_name, &pRepo)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );
    }

    SG_ERR_CHECK(  SG_repo__rebuild_indexes(pCtx, pRepo)  );

    /* fall through */

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_REPO_NULLFREE(pCtx, pRepo);
}

void do_dump_json(SG_context * pCtx, const char* psz_repo, const char* psz_spec_hid)
{
    SG_repo* pRepo = NULL;
    SG_vhash* pvh = NULL;
    SG_string* pstr = NULL;
    char* psz_hid = NULL;

    /* open the repo */
	if (psz_repo)
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );
	else
		SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );

    /* lookup the blob id */
    SG_ERR_CHECK(  SG_repo__hidlookup__blob(pCtx, pRepo, psz_spec_hid, &psz_hid)  );

    /* fetch the blob */
    SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_hid, &pvh)  );
    SG_REPO_NULLFREE(pCtx, pRepo);

    /* print it out.  fix the CRLF */
    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh, pstr)  );
    SG_VHASH_NULLFREE(pCtx, pvh);

    SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, pstr,
                                          (SG_byte *)"\r\n",2,
                                          (SG_byte *)"\n",1,
                                          SG_UINT32_MAX,SG_TRUE,NULL)  );

    SG_console(pCtx, SG_CS_STDOUT, "%s", SG_string__sz(pstr));

    SG_NULLFREE(pCtx, psz_hid);
    SG_STRING_NULLFREE(pCtx, pstr);

	return;

fail:
    SG_NULLFREE(pCtx, psz_hid);
    SG_REPO_NULLFREE(pCtx, pRepo);
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void do_cmd_dump_json(SG_context * pCtx, const char* psz_repo, SG_uint32 count_args, const char** paszArgs)
{
    SG_uint32 i;

    for (i=0; i<count_args; i++)
    {
        SG_ERR_CHECK_RETURN(  do_dump_json(pCtx, psz_repo, paszArgs[i])  );
    }
}


#ifdef DEBUG
void do_cmd_dump_treenode(SG_context * pCtx, const char * psz_repo, const char * psz_spec_cs)
{
	SG_pathname * pPathCwd = NULL;
    SG_repo* pRepo = NULL;
	char * psz_hid_cs = NULL;
	char * szHidSuperRoot = NULL;
	SG_string * pStrDump = NULL;

	if (psz_repo)
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );
	else
		SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, &pPathCwd)  );

	if (psz_spec_cs && *psz_spec_cs)
	{
		SG_ERR_CHECK(  SG_repo__hidlookup__dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, psz_spec_cs, &psz_hid_cs)  );
	}
	else
	{
		// assume the current baseline.  NOTE that our dump will not show the dirt in the WD.
		//
		// when we have an uncomitted merge, we will have more than one parent.
		// what does this command mean then?  It feels like we we should throw
		// an error and say that you have to commit first.

		SG_bool bHasMerge = SG_FALSE;
		SG_ERR_CHECK(  SG_wc__get_wc_baseline(pCtx, NULL, &psz_hid_cs, &bHasMerge)  );		// assumes CWD-based pRepo

		if (bHasMerge)
			SG_ERR_THROW(  SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE  );
	}

	// load the contents of the desired CSET and get the super-root treenode.

    SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, psz_hid_cs, &szHidSuperRoot)  );

	// let the treenode dump routine populate our string.
	// TODO we may want to crack this open and dump the user's actual root.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrDump)  );
	SG_ERR_CHECK(  SG_treenode_debug__dump__by_hid(pCtx,szHidSuperRoot,pRepo,4,SG_TRUE,pStrDump)  );

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "CSet[%s]:\n%s\n", psz_hid_cs, SG_string__sz(pStrDump))  );

    /* fall through to common cleanup */

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_REPO_NULLFREE(pCtx, pRepo);
    SG_NULLFREE(pCtx, psz_hid_cs);
	SG_NULLFREE(pCtx, szHidSuperRoot);
	SG_STRING_NULLFREE(pCtx, pStrDump);
}
#endif


void vv2__do_cmd_hid(SG_context * pCtx,
					 const SG_option_state * pOptSt,
					 const char* pszPath)
{
    SG_pathname* pPath = NULL;
    char* pszHid = NULL;

    SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, pszPath);
	SG_ERR_CHECK(  SG_vv2__compute_file_hid(pCtx, pOptSt->psz_repo, pPath, &pszHid)  );
    SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", pszHid)  );

fail:
    SG_NULLFREE(pCtx, pszHid);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}



#if defined(WINDOWS)
void do_cmd_forget_passwords(
	SG_context* pCtx,
	SG_bool bNoPrompt)
{
	SG_bool bConfirmed = SG_FALSE;

	if (!bNoPrompt)
		SG_ERR_CHECK_RETURN(  SG_console__ask_question__yn(pCtx, "Are you sure you want delete all saved passwords?", SG_QA_DEFAULT_NO, &bConfirmed)  );
	else
		bConfirmed = SG_TRUE;

	if (bConfirmed)
		SG_ERR_CHECK_RETURN(  SG_password__forget_all(pCtx)  );
}
#endif

void do_cmd_heads(
        SG_context * pCtx, 
        const char* psz_repo,
		const SG_stringarray* psaRequestedBranchNames,
		SG_bool bAll,
		SG_bool bVerbose
        )
{
	SG_rbtree* prbLeafHidsToOutput = NULL;
	SG_rbtree* prbNonLeafHidsToOutput = NULL;
	SG_rbtree_iterator* pit = NULL;
	SG_repo * pRepo = NULL;
	SG_vhash* pvhPileBranches = NULL;

	if (psz_repo)
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );
	else
		SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhPileBranches)  );

	SG_ERR_CHECK(  SG_vv_verbs__heads__repo(pCtx, pRepo, psaRequestedBranchNames, bAll, &prbLeafHidsToOutput, &prbNonLeafHidsToOutput)  );

	/* Output the results. */
	{
		SG_bool bOk = SG_FALSE;
		const char* pszRefHid;

		/* Output leaves at the top of the list. */
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbLeafHidsToOutput, &bOk, &pszRefHid, NULL)  );
		while (bOk)
		{
			SG_ERR_CHECK(  SG_cmd_util__dump_log(pCtx, SG_CS_STDOUT, pRepo, pszRefHid, pvhPileBranches, !bAll, bVerbose)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &bOk, &pszRefHid, NULL)  );
		}
		SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

		/* Then the non-leaves */
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbNonLeafHidsToOutput, &bOk, &pszRefHid, NULL)  );
		while (bOk)
		{
			SG_ERR_CHECK(  SG_cmd_util__dump_log(pCtx, SG_CS_STDOUT, pRepo, pszRefHid, pvhPileBranches, !bAll, bVerbose)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &bOk, &pszRefHid, NULL)  );
		}
	}

	// fall thru to common cleanup.

fail:
	SG_RBTREE_NULLFREE(pCtx, prbNonLeafHidsToOutput);
	SG_RBTREE_NULLFREE(pCtx, prbLeafHidsToOutput);
	SG_VHASH_NULLFREE(pCtx, pvhPileBranches);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}



static void do_cmd_branch(
        SG_context* pCtx,
        SG_bool plural,
        SG_uint32 count_args, 
        const char** paszArgs,
        SG_option_state * pOptSt
        )
{
    SG_repo* pRepo = NULL;
    SG_vhash* pvh_pile = NULL;
    char* psz_tied_branch_name = NULL;
    char* psz_rev_1 = NULL;
    char* psz_rev_2 = NULL;
	SG_stringarray* psaFullHids = NULL;

	// TODO 2012/07/17 Per W6502 I'd like to refactor this and call VV2 and WC API
	// TODO            routines to do most of the dirty work.

    if (
        (0 == count_args && !plural)
        || (0 < count_args && 0 == strcmp(paszArgs[0], "new"))
        || (0 < count_args && 0 == strcmp(paszArgs[0], "attach"))
        || (0 < count_args && 0 == strcmp(paszArgs[0], "detach"))
       )
    {
        if (0 == count_args)
        {
			SG_ERR_CHECK(  SG_wc__branch__get(pCtx, NULL, &psz_tied_branch_name)  );
            if (psz_tied_branch_name)
            {
                SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", psz_tied_branch_name)  );
            }
            else
            {
                // TODO print there is no current named branch
            }
        }
        else if (0 == strcmp(paszArgs[0], "new"))
        {
            if (count_args != 2)
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

			SG_ERR_CHECK(  SG_wc__branch__attach_new(pCtx, NULL, paszArgs[1])  );

            SG_console(pCtx, SG_CS_STDOUT, "Working copy attached to %s. A new head will be created with the next commit.\n", paszArgs[1]);
        }
        else if (0 == strcmp(paszArgs[0], "attach"))
        {
            if (count_args != 2)
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

			SG_ERR_TRY(  SG_wc__branch__attach(pCtx, NULL, paszArgs[1])  );
			SG_ERR_CATCH(  SG_ERR_BRANCH_NOT_FOUND  )
			{
				SG_ERR_RETHROW2(  (pCtx, "Branch %s does not exist. Use 'vv branch new' to create it.", paszArgs[1])  );
			}
			SG_ERR_CATCH_END;

            SG_console(pCtx, SG_CS_STDOUT, "Working copy attached to %s. A new head will be created with the next commit.\n", paszArgs[1]);
        }
        else if (0 == strcmp(paszArgs[0], "detach"))
        {
			SG_ERR_CHECK(  SG_wc__branch__get(pCtx, NULL, &psz_tied_branch_name)  );
            if (psz_tied_branch_name)
            {
				SG_ERR_CHECK(  SG_wc__branch__detach(pCtx, NULL)  );
                SG_console(pCtx, SG_CS_STDOUT, "Working copy detached from any branch.  The next commit will start an anonymous branch and will require the '--detached' option.\n");
            }
            else
            {
                SG_console(pCtx, SG_CS_STDOUT, "Working copy was already detached.\n");
            }
        }
    }
    else
    {
        const char * LIST = "list";
        if(0 == count_args)
        {
            paszArgs=&LIST;
            count_args=1;
        }

        if (pOptSt->psz_repo)
        {
            SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pOptSt->psz_repo, &pRepo)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );
        }

        if (0 == strcmp(paszArgs[0], "add-head"))
        {
            SG_audit q;
            SG_vhash* pvh_pile_branches = NULL;
            SG_vhash* pvh_branch_info = NULL;
            SG_vhash* pvh_branch_values = NULL;
            SG_bool b_has = SG_FALSE;
			SG_uint32 countRevSpecs = 0;

            if (count_args != 2)
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

            SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_pile_branches, paszArgs[1], &pvh_branch_info)  );
            if (pvh_branch_info)
            {
                SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
            }

			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
            if (1 != countRevSpecs)
            {
                SG_console(pCtx, SG_CS_STDERR, "%s accepts one revision, which may be specified with --rev, --tag or --branch\n", "add-head");

                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

			SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_FALSE, &psz_rev_1, NULL)  );

            if (pvh_branch_values)
            {
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branch_values, psz_rev_1, &b_has)  );
                if (b_has)
                {
                    SG_console(pCtx, SG_CS_STDERR, "%s is already a head of branch %s\n", psz_rev_1, paszArgs[1]);
                    SG_ERR_THROW(  SG_ERR_USAGE  );
                }
            }

            // TODO check for attempts to add ancestor/descendant

            SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

            SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx, pRepo, paszArgs[1], psz_rev_1, NULL, SG_TRUE, &q)  );
            if (pvh_branch_info)
            {
                SG_console(pCtx, SG_CS_STDOUT, "%s is now a head of branch %s\n", psz_rev_1, paszArgs[1]);
            }
            else
            {
                SG_console(pCtx, SG_CS_STDOUT, "%s is now a head of newly created branch %s\n", psz_rev_1, paszArgs[1]);
            }
        }
        else if (0 == strcmp(paszArgs[0], "remove-head"))
        {
            SG_audit q;
            SG_uint32 i = 0;
			SG_uint32 countRevSpecs;

            if (count_args != 2)
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

            if (pOptSt->bAll)
            {
                SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );
                SG_ERR_CHECK(  SG_vc_branches__remove_head(pCtx, pRepo, paszArgs[1], NULL, &q)  );
                SG_console(pCtx, SG_CS_STDOUT, "Branch %s no longer exists.\n", paszArgs[1]);
            }
            else
            {
				SG_uint32 countResolvedHids = 0;
                SG_vhash* pvh_pile_branches = NULL;
                SG_vhash* pvh_branch_info = NULL;

				SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );

                if (0 == countRevSpecs)
                {
                    SG_console(pCtx, SG_CS_STDERR, "%s requires at least one revision, which may be specified by with --rev, --tag or --branch\n", "remove-head");

                    SG_ERR_THROW(  SG_ERR_USAGE  );
                }

                SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );
				
				SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_TRUE, &psaFullHids, NULL)  );
				SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaFullHids, &countResolvedHids)  );
                for (i = 0; i < countResolvedHids; i++)
                {
					const char* pszRefHid;
					SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaFullHids, i, &pszRefHid)  );
                    SG_ERR_CHECK(  SG_vc_branches__remove_head(pCtx, pRepo, paszArgs[1], pszRefHid, &q)  );
                    SG_console(pCtx, SG_CS_STDOUT, "%s is no longer a head of %s\n", pszRefHid, paszArgs[1]);
                }

                // get pile and see if the branch exists, and if not, say so
                SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
                SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
                SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_pile_branches, paszArgs[1], &pvh_branch_info)  );
                if (!pvh_branch_info)
                {
                    SG_console(pCtx, SG_CS_STDOUT, "Branch %s no longer exists.\n", paszArgs[1]);
                }
            }
        }
        else if (0 == strcmp(paszArgs[0], "move-head"))
        {
            SG_vhash* pvh_pile_branches = NULL;
            SG_vhash* pvh_branch_info = NULL;
            SG_vhash* pvh_branch_values = NULL;
            SG_uint32 count_branch_values = 0;
			SG_uint32 countRevSpecs = 0;

            if (count_args != 2)
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

            SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_pile_branches, paszArgs[1], &pvh_branch_info)  );
            if (!pvh_branch_info)
            {
                SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
            }
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_branch_values)  );

			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );

            if (1 == countRevSpecs)
            {
				SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_FALSE, &psz_rev_1, NULL)  );

                if (pOptSt->bAll)
                {
                    SG_audit q;

                    SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

                    SG_ERR_CHECK(  SG_vc_branches__move_head__all(pCtx, pRepo, paszArgs[1], psz_rev_1, &q)  );
                    SG_console(pCtx, SG_CS_STDOUT, "%s is now the only head of %s\n", psz_rev_1, paszArgs[1]);
                }
                else if (1 == count_branch_values)
                {
                    const char* psz_cur_rev = NULL;
                    SG_audit q;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_values, 0, &psz_cur_rev, NULL)  );
                    SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

                    SG_ERR_CHECK(  SG_vc_branches__move_head(pCtx, pRepo, paszArgs[1], psz_cur_rev, psz_rev_1, &q)  );
                }
                else
                {
                    SG_console(pCtx, SG_CS_STDERR, "%s has more than one head.  Unable to determine the source changeset.\n", paszArgs[1]);
                    SG_ERR_THROW(  SG_ERR_USAGE  );
                }
            }
            else if (2 == countRevSpecs)
            {
				const char* psz_from = NULL;
				const char* psz_to = NULL;
                SG_audit q;

				SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_FALSE, &psaFullHids, NULL)  );
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaFullHids, 0, &psz_from)  );
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaFullHids, 1, &psz_to)  );

				SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );
				SG_ERR_CHECK(  SG_vc_branches__move_head(pCtx, pRepo, paszArgs[1], psz_from, psz_to, &q)  );
            }
            else
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }
        }
        else if (0 == strcmp(paszArgs[0], "close"))
        {
            SG_audit q;

            if (count_args != 2)
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

            SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

            SG_ERR_CHECK(  SG_vc_branches__close(pCtx, pRepo, paszArgs[1], &q)  );
            SG_console(pCtx, SG_CS_STDOUT, "Branch %s is now closed.\n", paszArgs[1]);
        }
        else if (0 == strcmp(paszArgs[0], "reopen"))
        {
            SG_audit q;

            if (count_args != 2)
            {
                SG_ERR_THROW(  SG_ERR_USAGE  );
            }

            SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

            SG_ERR_CHECK(  SG_vc_branches__reopen(pCtx, pRepo, paszArgs[1], &q)  );
            SG_console(pCtx, SG_CS_STDOUT, "Branch %s has been reopened.\n", paszArgs[1]);
        }
        else if (0 == strcmp(paszArgs[0], "list"))
        {
            SG_uint32 count = 0;
            SG_uint32 i = 0;
            SG_vhash* pvh_pile_branches = NULL;
            SG_vhash* pvh_pile_closed = NULL;

            SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvh_pile_closed)  );
            SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_pile_branches, SG_FALSE, SG_vhash_sort_callback__increasing_insensitive)  );

            // TODO deal with closed and --all

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_pile_branches, &count)  );

            for (i=0; i<count; i++)
            {
                SG_bool b_include_this_one = SG_FALSE;
                SG_bool b_closed = SG_FALSE;
                const SG_variant* pv = NULL;
                const char* psz_name = NULL;
                SG_vhash* pvh_branch_info = NULL;
                SG_vhash* pvh_branch_values = NULL;
                SG_uint32 count_branch_values = 0;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_pile_branches, i, &psz_name, &pv)  );
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_closed, psz_name, &b_closed)  );
                if (pOptSt->bAll)
                {
                    b_include_this_one = SG_TRUE;
                }
                else
                {
                    if (b_closed)
                    {
                        b_include_this_one = SG_FALSE;
                    }
                    else
                    {
                        b_include_this_one = SG_TRUE;
                    }
                }

                if (b_include_this_one)
                {
					SG_uint32 nrPresent = 0;
					SG_uint32 nrMissing = 0;
					SG_uint32 k;

                    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_branch_info)  );
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_branch_values)  );
					for (k=0; k<count_branch_values; k++)
					{
						const char * pszHid_k = NULL;
						SG_bool bExists_k = SG_FALSE;
						SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_values, k, &pszHid_k, NULL)  );
						SG_ERR_CHECK(  SG_repo__does_blob_exist(pCtx, pRepo, pszHid_k, &bExists_k)  );
						if (bExists_k)
							nrPresent++;
						else
							nrMissing++;
					}

                    if (nrPresent > 1)
                    {
                        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s (needs merge)", psz_name)  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s", psz_name)  );
                    }

					if (pOptSt->bAll && (nrMissing > 0))
					{
						SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, " (%d of %d not present in repository)",
												  nrMissing, nrMissing+nrPresent)  );
					}

                    if (b_closed)
                    {
                        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, " (closed)\n")  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
                    }
                }
            }
        }
        else
        {
            SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Unknown subcommand: %s\n", paszArgs[0])  );
            SG_ERR_THROW(  SG_ERR_USAGE  );
        }
    }

fail:
	SG_NULLFREE(pCtx, psz_rev_1);
	SG_NULLFREE(pCtx, psz_rev_2);
    SG_NULLFREE(pCtx, psz_tied_branch_name);
    SG_REPO_NULLFREE(pCtx, pRepo);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
	SG_STRINGARRAY_NULLFREE(pCtx, psaFullHids);
}

void do_cmd_vcdiff(SG_context * pCtx, SG_uint32 count_args, const char** paszArgs)
{
	SG_pathname* pPath1 = NULL;
	SG_pathname* pPath2 = NULL;
	SG_pathname* pPath3 = NULL;

	if (
		(count_args == 4)
		&& (0 == strcmp(paszArgs[0], "deltify"))
		)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, paszArgs[1])  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath2, paszArgs[2])  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath3, paszArgs[3])  );
		SG_ERR_CHECK(  SG_vcdiff__deltify__files(pCtx, pPath1, pPath2, pPath3)  );
	}
	else if (
		(count_args == 4)
		&& (0 == strcmp(paszArgs[0], "undeltify"))
		)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, paszArgs[1])  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath2, paszArgs[2])  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath3, paszArgs[3])  );
		SG_ERR_CHECK(  SG_vcdiff__undeltify__files(pCtx, pPath1, pPath2, pPath3)  );
	}
	else
	{
		SG_ERR_THROW_RETURN(  SG_ERR_USAGE  );
	}

	SG_PATHNAME_NULLFREE(pCtx, pPath1);
	SG_PATHNAME_NULLFREE(pCtx, pPath2);
	SG_PATHNAME_NULLFREE(pCtx, pPath3);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath1);
	SG_PATHNAME_NULLFREE(pCtx, pPath2);
	SG_PATHNAME_NULLFREE(pCtx, pPath3);
}


void do_cmd_upgrade(SG_context * pCtx, SG_uint32 count_args, const char ** paszArgs, SG_option_state* pOptSt)
{
    SG_vhash* pvh_old_repos = NULL;
    SG_uint32 count_repos = 0;
    SG_uint32 i = 0;
    SG_pathname* pPath_closet_backup = NULL;
    SG_pathname* pPath_new_backup = NULL;

    SG_NULLARGCHECK_RETURN(pOptSt);
    SG_UNUSED(count_args);
    SG_UNUSED(paszArgs);

    if (pOptSt->psz_restore_backup)
    {
		SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPath_closet_backup, pOptSt->psz_restore_backup)  );
        SG_ERR_CHECK(  SG_closet__restore(pCtx, pPath_closet_backup, &pPath_new_backup)  );
        if (pPath_new_backup)
        {
            SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "An existing closet was moved to %s\n", SG_pathname__sz(pPath_new_backup))  );
        }
        goto done;
    }

    if (!pOptSt->bConfirm)
    {
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "--confirm not given.  No changes will be made.\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
    }

    SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Checking to see which repositories need to be upgraded...")  );
    SG_ERR_CHECK(  SG_vv_verbs__upgrade__find_old_repos(pCtx, &pvh_old_repos)  );
    if (pvh_old_repos)
    {
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_old_repos, &count_repos)  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "done (%d found)\n", (int) count_repos)  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
    }
    else
    {
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "done (none found)\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
        goto done;
    }

    if (pOptSt->bConfirm)
    {
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Creating a backup...")  );
        SG_ERR_CHECK(  SG_closet__backup(pCtx, &pPath_closet_backup)  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "done\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
#if defined(WINDOWS)
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "To restore this backup:\n    vv upgrade --restore-backup \"%s\"\n", SG_pathname__sz(pPath_closet_backup))  );
#else
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "To restore this backup:\n    vv upgrade --restore-backup %s\n", SG_pathname__sz(pPath_closet_backup))  );
#endif

		SG_ERR_CHECK(  SG_vv_verbs__upgrade_repos(pCtx, pvh_old_repos)  );
    }
    else
    {
        SG_uint64 len_closet = 0;

        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "If you re-run this command with the --confirm flag,\nthe following things will happen:\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "1.  A complete backup of your repositories will be made.\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );

        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "2.  The following repositories will be upgraded:\n")  );
        for (i=0; i<count_repos; i++)
        {
            const char* psz_repo_name = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_old_repos, i, &psz_repo_name, NULL)  );

            SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    %s\n", psz_repo_name)  );
        }
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );

        SG_ERR_CHECK(  SG_closet__get_size(pCtx, &len_closet)  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "You should have at least %d MB of available disk space before you begin.\n", (int) ((len_closet*2) / (1024*1024)))  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );

        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "You should ensure that no one is accessing any repository\non this machine during the upgrade.\n")  );
        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );

        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "To perform this upgrade, do 'vv upgrade --confirm'\n")  );

        // explain that this can be done manually if you prefer
    }

done:
fail:
    SG_VHASH_NULLFREE(pCtx, pvh_old_repos);
    SG_PATHNAME_NULLFREE(pCtx, pPath_closet_backup);
    SG_PATHNAME_NULLFREE(pCtx, pPath_new_backup);
}


//////////////////////////////////////////////////////////////////

#define REPLACEINARG(_replaceMe_, _replaceWith_) SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, pCurrentArg, (const SG_byte*)_replaceMe_, SG_STRLEN(_replaceMe_), (const SG_byte*)_replaceWith_, SG_STRLEN(_replaceWith_), 1, SG_TRUE, NULL)  );


void do_cmd_incoming(
	SG_context* pCtx,
	const char* pszSrcRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	const char* pszDestRepoDescriptorName,
	SG_bool bVerbose,
	SG_bool bTrace)
{
	SG_history_result* pInfo = NULL;
	SG_vhash* pvhStats = NULL;
	
	SG_ERR_CHECK(  SG_vv_verbs__incoming(pCtx, pszSrcRepoSpec, pszUsername, pszPassword, pszDestRepoDescriptorName, &pInfo, bTrace ? &pvhStats : NULL)  );
	
	if (pInfo)
		SG_ERR_CHECK(  SG_cmd_util__dump_history_results(pCtx, SG_CS_STDOUT, pInfo, NULL, SG_FALSE, bVerbose, SG_TRUE)  );
	else
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "No incoming changes.\n")  );

#if defined(DEBUG)
	if (pvhStats)
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "Incoming stats")  );
#endif

fail:
	SG_HISTORY_RESULT_NULLFREE(pCtx, pInfo);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
}

void do_cmd_pull(
	SG_context * pCtx,
	SG_option_state * pOptSt,
	const char* pszSrcRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	const char* pszDestRepoDescriptorName)
{
	SG_rev_spec* pMyRevSpec = NULL;
	SG_vhash* pvhStats = NULL;
	SG_repo* pRepoDest = NULL;
	SG_uint32 count_revspec = 0;
	SG_vhash* pvhAcctInfo = NULL;

	SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &count_revspec)  );

	if (pOptSt->bUpdate && pOptSt->pRevSpec)
	{
		/* Pull will steal the rev spec. If we're going to use it again for update below,
		 * make our own copy. */
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC__COPY(pCtx, pOptSt->pRevSpec, &pMyRevSpec)  );
	}

	SG_vv_verbs__pull(pCtx, pszSrcRepoSpec, pszUsername, pszPassword, pszDestRepoDescriptorName, 
		pOptSt->pvh_area_names, &pOptSt->pRevSpec, &pvhAcctInfo,
		pOptSt->bTrace ? &pvhStats : NULL);
	if (SG_context__err_equals(pCtx, SG_ERR_REPO_ID_MISMATCH))
	{
		SG_uint32 count_areas = 0;

		SG_context__err_reset(pCtx);

		if (pOptSt->pvh_area_names)
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pOptSt->pvh_area_names, &count_areas)  );

		if (!count_revspec && !count_areas)
		{
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDestRepoDescriptorName, &pRepoDest)  );

			SG_pull__admin(pCtx, pRepoDest, pszSrcRepoSpec, pszUsername, pszPassword, NULL, pOptSt->bTrace ? &pvhStats : NULL);
			if (SG_context__err_equals(pCtx, SG_ERR_ADMIN_ID_MISMATCH))
			{
				SG_context__err_reset(pCtx);
				SG_ERR_THROW(SG_ERR_REPO_ID_MISMATCH);
			}
			else
				SG_ERR_CHECK_CURRENT;

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n"
				"WARNING: The repositories are not clones of one another,\n"
				"         but they belong to the same administrative group.\n"
				"         Only the user database was pulled.\n\n")  );
		}
		else
			SG_ERR_THROW(SG_ERR_REPO_ID_MISMATCH);
	}
	else
		SG_ERR_CHECK_CURRENT;

	// TODO: friendly error message when repo's not found (local or remote).

	if (pvhAcctInfo)
	{
		const char* pszAcctMsg = NULL;
		SG_ERR_CHECK(  SG_sync__get_account_info__details(pCtx, pvhAcctInfo, &pszAcctMsg, NULL)  );
		if (pszAcctMsg)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n%s\n", pszAcctMsg)  );
	}

#if defined(DEBUG)
	if (pvhStats)
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "Pull stats")  );
#endif

	// Note: I'm moving the UPDATE until after pull is completely finished.
	// Update can fail for various reasons (either multiple/ambiguous heads
	// or dirt in the WD or etc) and none of that should prevent the PULL
	// from completing.
	
	if (pOptSt->bUpdate)
	{
		// update based on the rev spec provided to pull
		SG_ERR_CHECK(  wc__do_cmd_update__after_pull(pCtx, pMyRevSpec)  );
	}

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pMyRevSpec);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_VHASH_NULLFREE(pCtx, pvhAcctInfo);
}

void do_cmd_outgoing(
	SG_context* pCtx,
	const char* pszDestRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	const char* pszSrcRepoDescriptorName,
	SG_bool bVerbose,
	SG_bool bTrace)
{
	SG_history_result* pInfo = NULL;
	SG_vhash* pvhStats = NULL;

	SG_ERR_CHECK(  SG_vv_verbs__outgoing(pCtx, pszDestRepoSpec, pszUsername, pszPassword, pszSrcRepoDescriptorName, &pInfo, 
		bTrace ? &pvhStats : NULL)  );
	if (pInfo)
		SG_ERR_CHECK(  SG_cmd_util__dump_history_results(pCtx, SG_CS_STDOUT, pInfo, NULL, SG_FALSE, bVerbose, SG_FALSE)  );
	else
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "No outgoing changes.\n")  );

#if defined(DEBUG)
	if (pvhStats)
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "Outgoing stats")  );
#endif

fail:
	SG_HISTORY_RESULT_NULLFREE(pCtx, pInfo);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
}

void do_cmd_push(
	SG_context * pCtx,
	SG_option_state * pOptSt,
	const char* pszDestRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	const char* pszSrcRepoDescriptorName,
	int * pExitStatus)
{
	SG_vhash* pvhStats = NULL;
	SG_repo* pRepo = NULL;
	SG_vhash* pvhAcctInfo = NULL;

	SG_ASSERT(pExitStatus!=NULL);

	SG_vv_verbs__push(pCtx, pszDestRepoSpec, pszUsername, pszPassword, pszSrcRepoDescriptorName, 
		pOptSt->pvh_area_names, pOptSt->pRevSpec, pOptSt->bForce, &pvhAcctInfo, pOptSt->bTrace ? &pvhStats : NULL);
	if(SG_context__err_equals(pCtx, SG_ERR_SERVER_DOESNT_ACCEPT_PUSHES))
	{
		SG_context__err_reset(pCtx);
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "The server does not accept pushes.\n")  );
		*pExitStatus = 1;
	}
	else if (SG_context__err_equals(pCtx, SG_ERR_REPO_ID_MISMATCH))
	{
		SG_uint32 count_revspec = 0;
		SG_uint32 count_areas = 0;

		SG_context__err_reset(pCtx);

		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &count_revspec)  );
		if (pOptSt->pvh_area_names)
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pOptSt->pvh_area_names, &count_areas)  );

		if (!count_revspec && !count_areas)
		{
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszSrcRepoDescriptorName, &pRepo)  );

			SG_push__admin(pCtx, pRepo, pszDestRepoSpec, pszUsername, pszPassword, &pvhAcctInfo, pOptSt->bTrace ? &pvhStats : NULL);
			if (SG_context__err_equals(pCtx, SG_ERR_ADMIN_ID_MISMATCH))
			{
				SG_context__err_reset(pCtx);
				SG_ERR_THROW(SG_ERR_REPO_ID_MISMATCH);
			}
			else
				SG_ERR_CHECK_CURRENT;

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n"
				"WARNING: The repositories are not clones of one another,\n"
				"         but they belong to the same administrative group.\n"
				"         Only the user database was pushed.\n\n")  );

			SG_REPO_NULLFREE(pCtx, pRepo);
		}
		else
			SG_ERR_THROW(SG_ERR_REPO_ID_MISMATCH);
	}
	else
		SG_ERR_CHECK_CURRENT;

	if (pvhStats)
	{
#ifdef DEBUG
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "Push stats")  );
#endif
		SG_VHASH_NULLFREE(pCtx, pvhStats);
	}

	if (pvhAcctInfo)
	{
		const char* pszAcctMsg = NULL;
		SG_ERR_CHECK(  SG_sync__get_account_info__details(pCtx, pvhAcctInfo, &pszAcctMsg, NULL)  );
		if (pszAcctMsg)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n%s\n", pszAcctMsg)  );
		SG_VHASH_NULLFREE(pCtx, pvhAcctInfo);
	}

	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvhAcctInfo);
}

/* ---------------------------------------------------------------- */
/* All the cmd funcs are below */

/*
 * If you are implementing a new cmd func, pick one of the existing
 * ones and copy it as a starting point.  All cmd funcs should follow
 * a similar structure:
 *
 * 1.  Check the flags.  Set bUsageError if anything is wrong.
 *
 * 2.  Check the number of args, set bUsageError if necessary.
 *
 * 3.  If no problems with flags/args, try to do the command.
 *     This will either succeed, throw a hard error, or throw a
 *     usage-error (when it is too complex to fully check here).
 *
 * 4.  if we have a usage-error, print some help and throw.
 */

/**
 * A private macro to run the command and handle hard- and usage-errors.
 * Invoke the command handler using pCtx as an argument.
 *
 * We Assume arg/flag checking code do a THROW for hard errors
 * and *only* set bUsageError when a bad arg/flag was seen.
 *
 * We do the THROW if bUsageError set.  (This allows *ALL*
 * flags/args to reported by the flag/arg checker, rather
 * than just the first.)
 *
 * Try to run the actual command.
 *
 * If it threw an error, we rethrow it.  If something within
 * the command threw Usage, we set our flag to be consistent.
 *
 * If something within the command-block set our flag (but
 * didn't throw), then we take care of it.
 */
#define INVOKE(_c_)	SG_STATEMENT(						\
	SG_ASSERT( (SG_context__has_err(pCtx)==SG_FALSE) );	\
	if (*pbUsageError)									\
		SG_ERR_THROW(  SG_ERR_USAGE  );					\
	_c_;												\
	if (SG_context__has_err(pCtx))						\
	{													\
		if (SG_context__err_equals(pCtx,SG_ERR_USAGE))	\
			*pbUsageError = SG_TRUE;						\
		SG_ERR_RETHROW;									\
	}													\
	else if (*pbUsageError)								\
	{													\
		SG_ERR_THROW(  SG_ERR_USAGE  );					\
	}													)


/* ---------------------------------------------------------------- */

DECLARE_CMD_FUNC(hid)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 1, pbUsageError)  );

	INVOKE(  vv2__do_cmd_hid(pCtx, pOptSt, paszArgs[0])  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(dump_json)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	INVOKE(  do_cmd_dump_json(pCtx, pOptSt->psz_repo, count_args, paszArgs)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

void do_cmd_check_integrity(
        SG_context * pCtx,
        SG_uint32 count_args, 
        const char** paszArgs,
        SG_option_state * pOptSt,
        const char* psz_rev
        );

DECLARE_CMD_FUNC(check_integrity)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;
    char* psz_rev = NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if (count_args && (0 != strcmp(paszArgs[0], "tree_deltas")))
	{
		SG_uint32 count_revspec = 0;
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &count_revspec)  );
		if (count_revspec)
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "A revision specifier is valid only with verify tree_deltas"));
	}
	else
		SG_ERR_CHECK(  _get_zero_or_one_revision_specifiers(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec, &psz_rev)  );

	if (*pbUsageError)
	{
		goto fail;
	}

	INVOKE(  do_cmd_check_integrity(pCtx, count_args, paszArgs, pOptSt, psz_rev)  );

fail:
	SG_NULLFREE(pCtx, psz_rev);
	SG_NULLFREE(pCtx, paszArgs);
}



DECLARE_CMD_FUNC(repo)
{
	const char** paszArgs= NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	INVOKE(  do_cmd_repo(
		pCtx,
		!strcmp(pszCommandName, "repos") || !strcmp(pszCommandName, "repositories"),
		count_args, paszArgs,
		pOptSt->bAllBut, pOptSt->bNoPrompt,
		pOptSt->bVerbose,
		pOptSt->bListAll,
		pOptSt->psz_storage, pOptSt->psz_hash, pOptSt->psz_shared_users,
		pExitStatus)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(serve)
{
	const char** paszArgs= NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if(count_args>0)
		SG_ERR_THROW(SG_ERR_USAGE);

	INVOKE(  do_cmd_serve(pCtx, pOptSt, pExitStatus)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(graph_history)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(  vv2__do_cmd_history(pCtx, pOptSt, psaArgs, SG_TRUE);  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(linehistory)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(  do_cmd_linehistory(pCtx, pOptSt, psaArgs);  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}


DECLARE_CMD_FUNC(history)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(  vv2__do_cmd_history(pCtx, pOptSt, psaArgs, SG_FALSE);  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(localsettings)
{
	const char** paszArgs= NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	INVOKE(  do_cmd_localsettings(pCtx, pOptSt->bVerbose, pOptSt->bForce, count_args, paszArgs)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(dump_lca)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
	if (count_args)
		*pbUsageError = SG_TRUE;

#if defined(DEBUG)
	if (pOptSt->bGaveDagNum)
	{
		SG_uint32 tagCount = 0;
		SG_ERR_CHECK(  SG_rev_spec__count_tags(pCtx, pOptSt->pRevSpec, &tagCount)  );
		if (tagCount > 0)
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "tags cannot be specified in combination with dagnums\n")  );
			*pbUsageError = SG_TRUE;
		}
	}
#endif

	if (pOptSt->bList && pOptSt->bVerbose)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Cannot use both --list and --verbose.\n")  );
		*pbUsageError = SG_TRUE;
	}

	// allow "vv dump_lca [--list | --verbose] [--rev=r1 | --tag=t1 [--rev=r2 | --tag=t2 ...]] [repo_desc_name dag_num]"
	
	INVOKE(  do_cmd_dump_lca(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

#ifdef DEBUG
DECLARE_CMD_FUNC(dump_treenode)
{
	const char** paszArgs = NULL;
	const char* pszChangesetId = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if (count_args > 0 )
		pszChangesetId = paszArgs[0];

	INVOKE(  do_cmd_dump_treenode(pCtx, pOptSt->psz_repo, pszChangesetId)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}
#endif

DECLARE_CMD_FUNC(commit)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(  wc__do_cmd_commit(pCtx, pOptSt, psaArgs);  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(revert)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(
		if (pOptSt->bAll)
			wc__do_cmd_revert_all(pCtx, pOptSt, psaArgs);
		else
			wc__do_cmd_revert_items(pCtx, pOptSt, psaArgs);
		);

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(lock)
{
	SG_stringarray * psaArgs = NULL;
	char* szWhoami = NULL;
	SG_string* pUsername = NULL;
	SG_string* pPassword = NULL;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pOptSt);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );
	if (!psaArgs)		// we require 1 or more args
		*pbUsageError = SG_TRUE;

	INVOKE(

		wc__do_cmd_lock(pCtx, pOptSt, psaArgs, NULL, NULL);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			SG_cmd_util__get_username_for_repo(pCtx, NULL, &szWhoami);
			do
			{
				SG_context__err_reset(pCtx);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_TRUE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);
				if(!SG_context__has_err(pCtx))
					wc__do_cmd_lock(pCtx, pOptSt, psaArgs, SG_string__sz(pUsername), SG_string__sz(pPassword));

				SG_context__push_level(pCtx);
				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
				SG_context__pop_level(pCtx);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

fail:
	SG_NULLFREE(pCtx, szWhoami);
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(unlock)
{
	SG_stringarray * psaArgs = NULL;
	char* szWhoami = NULL;
	SG_string* pUsername = NULL;
	SG_string* pPassword = NULL;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pOptSt);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(

		wc__do_cmd_unlock(pCtx, pOptSt, psaArgs, NULL, NULL);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			SG_cmd_util__get_username_for_repo(pCtx, NULL, &szWhoami);
			do
			{
				SG_context__err_reset(pCtx);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_TRUE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);
				if(!SG_context__has_err(pCtx))
					wc__do_cmd_unlock(pCtx, pOptSt, psaArgs, SG_string__sz(pUsername), SG_string__sz(pPassword));

				SG_context__push_level(pCtx);
				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
				SG_context__pop_level(pCtx);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(remove)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );
	if (!psaArgs)		// we require 1 or more args
		*pbUsageError = SG_TRUE;

	INVOKE(  wc__do_cmd_remove(pCtx, pOptSt, psaArgs)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(add)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );
	if (!psaArgs)		// we require 1 or more args
		*pbUsageError = SG_TRUE;

	INVOKE(  wc__do_cmd_add(pCtx, pOptSt, psaArgs)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(help)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if (count_args > 2)
	{
		*pbUsageError = SG_TRUE;
	}

	INVOKE(
		if (0 == count_args)
			do_cmd_help__list_all_commands(pCtx, SG_CS_STDOUT, pszAppName, pOptSt);
		else
		{
			const char *cmd = paszArgs[0];
			const char *subcmd = (1 == count_args) ? NULL : paszArgs[1];
			SG_bool	listed = SG_FALSE;

			SG_ERR_CHECK(  do_cmd_help__list_bonus_help(pCtx, SG_CS_STDOUT, cmd, &listed) );

			if (! listed)
			{
				do_cmd_help__list_command_help(pCtx, SG_CS_STDOUT, pszAppName, cmd, subcmd, SG_FALSE);

				if (SG_context__err_equals(pCtx, SG_ERR_USAGE))
				{
					do_cmd_help__list_all_commands(pCtx, SG_CS_STDERR, pszAppName, pOptSt);
					SG_context__err_reset(pCtx);
				}
			}
		}
		);

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

static void x_get_users(
        SG_context * pCtx,
        SG_varray* pva,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_users = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_users)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_username = NULL;
        const char* psz_userid = NULL;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "name", &psz_username)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_FIELD__RECID, &psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_users, psz_userid, &b_already)  );
        if (!b_already)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_users, psz_userid, psz_username)  );
        }
    }

    *ppvh = pvh_users;
    pvh_users = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_users);
}

static void x_get_audits(
        SG_context * pCtx,
        SG_varray* pva,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_audits = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_audits)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_csid = NULL;
        const char* psz_userid = NULL;
        SG_int64 timestamp = -1;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "csid", &psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "userid", &psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "timestamp", &timestamp)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_audits, psz_csid, &b_already)  );
        if (!b_already)
        {
            SG_vhash* pvh_a = NULL;

            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_audits, psz_csid, &pvh_a)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_a, "userid", psz_userid)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_a, "timestamp", timestamp)  );
        }
    }

    *ppvh = pvh_audits;
    pvh_audits = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_audits);
}

static void dag_integrity_dump(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum
        )
{
    SG_ihash* pih_csids = NULL;
    char* psz_repo_id = NULL;
    char* psz_admin_id = NULL;
    SG_changeset* pcs = NULL;
    SG_uint32 count_nodes = 0;
    SG_uint32 i_node = 0;
    SG_varray* pva_all_audits = NULL;
    SG_vhash* pvh_audits = NULL;
    SG_varray* pva_all_users = NULL;
    SG_vhash* pvh_users = NULL;
    SG_vhash* pvh_add = NULL;
    SG_bool b_pop = SG_FALSE;

    SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pva_all_users)  );
    SG_ERR_CHECK(  SG_repo__query_audits(pCtx, pRepo, dagnum, -1, -1, NULL, &pva_all_audits)  );

    SG_ERR_CHECK(  x_get_users(pCtx, pva_all_users, &pvh_users)  );
    SG_ERR_CHECK(  x_get_audits(pCtx, pva_all_audits, &pvh_audits)  );

    SG_ERR_CHECK(  SG_repo__fetch_dagnode_ids(pCtx, pRepo, dagnum, -1, -1, &pih_csids)  );
    SG_ERR_CHECK(  SG_ihash__count(pCtx, pih_csids, &count_nodes)  );
    
    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Verifying", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_nodes, "nodes")  );

    for (i_node = 0; i_node < count_nodes; i_node++)
    {
        SG_vhash* pvh_cs = NULL;
        const char* pszHid = NULL;

        SG_ERR_CHECK(  SG_ihash__get_nth_pair(pCtx, pih_csids, i_node, &pszHid, NULL)  );

        SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, pszHid, &pcs)  );

        SG_ERR_CHECK(  SG_changeset__get_vhash_ref(pCtx, pcs, &pvh_cs)  );
        //SG_VHASH_STDOUT(pvh_cs);

        SG_CHANGESET_NULLFREE(pCtx, pcs);

        SG_repo__db__calc_delta_from_root(pCtx, pRepo, dagnum, pszHid, 0, &pvh_add);
        if (SG_context__has_err(pCtx))
        {
            if (SG_context__err_equals(pCtx, SG_ERR_DB_DELTA))
            {
                SG_context__err_to_console(pCtx, SG_CS_STDERR);
                SG_ERR_CHECK(  SG_context__err_reset(pCtx)  );
            }
            else
            {
                SG_ERR_CHECK_CURRENT;
            }
        }
        SG_VHASH_NULLFREE(pCtx, pvh_add);

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

fail:
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
    SG_IHASH_NULLFREE(pCtx, pih_csids);
    SG_VARRAY_NULLFREE(pCtx, pva_all_audits);
    SG_VARRAY_NULLFREE(pCtx, pva_all_users);
    SG_VHASH_NULLFREE(pCtx, pvh_audits);
    SG_VHASH_NULLFREE(pCtx, pvh_users);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_NULLFREE(pCtx, psz_repo_id);
    SG_NULLFREE(pCtx, psz_admin_id);
}

void SG_verify__blob_hashes(
        SG_context * pCtx,
        const char* psz_repo
        )
{
    SG_repo* pRepo = NULL;
    SG_blobset* pbs = NULL;
    SG_vhash* pvh = NULL;
    SG_uint32 count_blobs = 0;
    SG_uint32 i = 0;
    SG_bool b_pop = SG_FALSE;
	SG_bool b_failure = SG_FALSE;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepo, 0, 0, 0, &pbs)  );
    SG_ERR_CHECK(  SG_blobset__get(pCtx, pbs, 0, 0, &pvh)  );
    SG_BLOBSET_NULLFREE(pCtx, pbs);
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count_blobs)  );

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Verifying", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_blobs, "blobs")  );

    for (i=0; i<count_blobs; i++)
    {
		const char* psz_hid = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &psz_hid, NULL)  );

        SG_repo__verify_blob(pCtx, pRepo, psz_hid, NULL);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "\nFailed to verify blob with HID %s: ", psz_hid)  );
			SG_context__err_to_console(pCtx, SG_CS_STDOUT);
			SG_ERR_DISCARD;
			b_failure = SG_TRUE;
		}

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

	if (b_failure)
		SG_ERR_THROW(SG_ERR_BLOBVERIFYFAILED);

fail:
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_BLOBSET_NULLFREE(pCtx, pbs);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_verify__db_deltas(
        SG_context * pCtx,
        const char* psz_repo
        )
{
	SG_repo* pRepo = NULL;
	SG_uint32  i = 0;
	SG_uint32  count_dagnums = 0;
	SG_uint64* paDagNums     = NULL;
    SG_bool b_pop = SG_FALSE;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count_dagnums, &paDagNums)  );

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Verifying", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_dagnums, "dags")  );

	for (i=0; i<count_dagnums; i++)
	{
        if (SG_DAGNUM__IS_DB(paDagNums[i]))
        {
            SG_bool b_do = SG_TRUE;

            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(paDagNums[i]))
            {
                SG_zingtemplate* pzt = NULL;

                SG_zing__get_cached_template__static_dagnum(pCtx, paDagNums[i], &pzt);
                if (
                        SG_context__has_err(pCtx)
                        && (SG_context__err_equals(pCtx, SG_ERR_ZING_TEMPLATE_NOT_FOUND))
                        )
                {
                    SG_context__err_reset(pCtx);
                    b_do = SG_FALSE;
                }
            }

            if (b_do)
            {
                SG_ERR_CHECK(  dag_integrity_dump(pCtx, pRepo, paDagNums[i])  );
            }
        }
        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
	}

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

	/* fall through */
fail:
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
	SG_NULLFREE(pCtx, paDagNums);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_verify__dag_consistency(
        SG_context * pCtx,
        const char* psz_repo
        )
{
	SG_repo* pRepo = NULL;
	SG_uint32  i = 0;
	SG_uint32  count_dagnums = 0;
	SG_uint64* paDagNums     = NULL;
    SG_bool b_pop = SG_FALSE;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count_dagnums, &paDagNums)  );

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Verifying", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_dagnums, "dags")  );

	for (i=0; i<count_dagnums; i++)
	{
        SG_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pRepo, paDagNums[i], NULL)  );

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
	}

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

	/* fall through */
fail:
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
	SG_NULLFREE(pCtx, paDagNums);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_verify__dbndx_states(
        SG_context * pCtx,
        const char* psz_repo
        )
{
	SG_repo* pRepo = NULL;
	SG_uint32  i = 0;
	SG_uint32  count_dagnums = 0;
	SG_uint64* paDagNums     = NULL;
    SG_bool b_pop = SG_FALSE;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count_dagnums, &paDagNums)  );

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Verifying", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_dagnums, "dags")  );

	for (i=0; i<count_dagnums; i++)
	{
        SG_ERR_CHECK(  SG_repo__verify__dbndx_states(pCtx, pRepo, paDagNums[i], NULL)  );

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
	}

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

	/* fall through */
fail:
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
	SG_NULLFREE(pCtx, paDagNums);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void do_cmd_check_integrity(
        SG_context * pCtx,
        SG_uint32 count_args, 
        const char** paszArgs,
        SG_option_state * pOptSt,
        const char* psz_rev
        )
{
    if (0 == count_args)
        SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "A subcommand is required."));

    if (0 == strcmp(paszArgs[0], "tree_deltas"))
    {
        if (psz_rev)
        {
            SG_ERR_CHECK(  SG_verify__tree_deltas__one(pCtx, pOptSt->psz_repo, psz_rev)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_verify__tree_deltas(pCtx, pOptSt->psz_repo)  );
        }
    }
    else if (0 == strcmp(paszArgs[0], "blob_hashes"))
    {
        SG_ERR_CHECK(  SG_verify__blob_hashes(pCtx, pOptSt->psz_repo)  );
    }
    else if (0 == strcmp(paszArgs[0], "dbndx_states"))
    {
        SG_ERR_CHECK(  SG_verify__dbndx_states(pCtx, pOptSt->psz_repo)  );
    }
    else if (0 == strcmp(paszArgs[0], "dag_consistency"))
    {
        SG_ERR_CHECK(  SG_verify__dag_consistency(pCtx, pOptSt->psz_repo)  );
    }
    else if (0 == strcmp(paszArgs[0], "db_deltas"))
    {
        SG_ERR_CHECK(  SG_verify__db_deltas(pCtx, pOptSt->psz_repo)  );
    }
    else if (0 == strcmp(paszArgs[0], "blobs_present"))
    {
        // TODO make sure every blob referenced in every changeset is present.
    }
    else
    {
        SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Unknown subcommand: %s", paszArgs[0]));
    }

fail:
    ;
}

DECLARE_CMD_FUNC(version)
{

	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;
	
	const char * szVersion= NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
	INVOKE(
		if(count_args==0)
		{
			SG_lib__version(pCtx, &szVersion);
			if(!SG_context__has_err(pCtx))
				SG_console(pCtx, SG_CS_STDOUT, "%s\n", szVersion);
		}
		else if(count_args==1)
		{
			do_cmd_version__remote(pCtx, paszArgs[0]);
		}
		else
			*pbUsageError = SG_TRUE;
	);

	// fall through
fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(blobcount)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 1, pbUsageError)  );

    INVOKE(  do_cmd_blobcount(pCtx, paszArgs[0])  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

#if 0
DECLARE_CMD_FUNC(group)
{
	const char** paszArgs = NULL;
    SG_uint32 count_args = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

    if (count_args < 1)
    {
        SG_ERR_THROW2(  SG_ERR_INVALIDARG,
                       (pCtx, "Must include group name")  );
    }
    else if (count_args == 1)
    {
        INVOKE(  do_cmd_group(pCtx, paszArgs[0], pOptSt->psz_repo, MY_CMD_GROUP_OP__LIST_SHALLOW, NULL, 0)  );
    }
    else
    {
        if (0 == strcmp(paszArgs[1], "addusers"))
        {
            INVOKE(  do_cmd_group(pCtx, paszArgs[0], pOptSt->psz_repo, MY_CMD_GROUP_OP__ADD_USERS, paszArgs+2, count_args-2)  );
        }
        else if (0 == strcmp(paszArgs[1], "addsubgroups"))
        {
            INVOKE(  do_cmd_group(pCtx, paszArgs[0], pOptSt->psz_repo, MY_CMD_GROUP_OP__ADD_SUBGROUPS, paszArgs+2, count_args-2)  );
        }
        else if (0 == strcmp(paszArgs[1], "removeusers"))
        {
            INVOKE(  do_cmd_group(pCtx, paszArgs[0], pOptSt->psz_repo, MY_CMD_GROUP_OP__REMOVE_USERS, paszArgs+2, count_args-2)  );
        }
        else if (0 == strcmp(paszArgs[1], "removesubgroups"))
        {
            INVOKE(  do_cmd_group(pCtx, paszArgs[0], pOptSt->psz_repo, MY_CMD_GROUP_OP__REMOVE_SUBGROUPS, paszArgs+2, count_args-2)  );
        }
        else if (0 == strcmp(paszArgs[1], "list"))
        {
            INVOKE(  do_cmd_group(pCtx, paszArgs[0], pOptSt->psz_repo, MY_CMD_GROUP_OP__LIST_SHALLOW, paszArgs+2, count_args-2)  );
        }
        else if (0 == strcmp(paszArgs[1], "listdeep"))
        {
            INVOKE(  do_cmd_group(pCtx, paszArgs[0], pOptSt->psz_repo, MY_CMD_GROUP_OP__LIST_DEEP, paszArgs+2, count_args-2)  );
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_INVALIDARG,
                           (pCtx, "Invalid subcommand: %s", paszArgs[1])  );
        }
    }

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(creategroup)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 1, pbUsageError)  );

    INVOKE(  do_cmd_creategroup(pCtx, paszArgs[0], pOptSt->psz_repo)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(groups)
{
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pGetopt);
	SG_UNUSED(pExitStatus);

    INVOKE(  do_cmd_groups(pCtx, pOptSt->psz_repo)  );

fail:
    ;
}

#endif

DECLARE_CMD_FUNC(zingmerge)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

    INVOKE(  do_zingmerge(pCtx, pOptSt->psz_repo)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(reindex)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

    if (pOptSt->bAll)
    {
        INVOKE(  do_reindex_all(pCtx)  );
    }
    else
    {
        INVOKE(  do_reindex(pCtx, pOptSt->psz_repo)  );
    }

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(whoami)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

    // TODO what about scope of whoami information?

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
    if (1 == count_args)
    {
		INVOKE(  do_cmd_set_whoami(pCtx, paszArgs[0], pOptSt->psz_repo, pOptSt->bCreate)  );
    }
    else
    {
		INVOKE(  do_cmd_show_whoami(pCtx, pOptSt->psz_repo)  );
    }

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(locks)
{
	char* szWhoami = NULL;
	SG_string* pUsername = NULL;
	SG_string* pPassword = NULL;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pGetopt);
	SG_UNUSED(pExitStatus);

	INVOKE(

		// TODO 2012/07/05 This loop doesn't use the keyring if available.
		// TODO            See the loop in cmd_pull().  See W0912.

		vv2__do_cmd_locks(pCtx, pOptSt, NULL, NULL);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			SG_cmd_util__get_username_for_repo(pCtx, NULL, &szWhoami);
			do
			{
				SG_context__err_reset(pCtx);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_TRUE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);
				if(!SG_context__has_err(pCtx))
					vv2__do_cmd_locks(pCtx, pOptSt, SG_string__sz(pUsername), SG_string__sz(pPassword));

				SG_context__push_level(pCtx);
				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
				SG_context__pop_level(pCtx);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

fail:
	SG_NULLFREE(pCtx, szWhoami);
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
}

DECLARE_CMD_FUNC(user)
{
	const char** paszArgs= NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pGetopt);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	INVOKE(  do_cmd_user(pCtx, !strcmp(pszCommandName,"users"), pOptSt->psz_repo, pOptSt->bListAll, count_args, paszArgs)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(zip)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 1, pbUsageError)  );

    INVOKE(  do_cmd_zip(pCtx, pOptSt, paszArgs[0])  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(clone)
{
	const char** paszArgs = NULL;
    SG_uint32 count_mutually_exclusive_options = 0;

	char *szWhoami = NULL;
	const char* szPass = NULL;
	
	SG_string *pUsername = NULL;
	SG_string *pPassword = NULL;

	SG_bool bNewIsRemote = SG_FALSE;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 2, pbUsageError)  );

    if (*pbUsageError)
    {
        SG_ERR_THROW(SG_ERR_USAGE);
    }

    if (pOptSt->bPack)
    {
        count_mutually_exclusive_options++;
    }
    if (pOptSt->b_all_full)
    {
        count_mutually_exclusive_options++;
    }
    if (pOptSt->b_all_zlib)
    {
        count_mutually_exclusive_options++;
    }

    if (count_mutually_exclusive_options > 1)
    {
        SG_console(pCtx, SG_CS_STDERR, "clone can accept only one of --pack, --all-full, or --all-zlib\n");
        SG_ERR_THROW(SG_ERR_USAGE);
    }

	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, paszArgs[1], &bNewIsRemote)  );
	if (bNewIsRemote)
	{
		SG_ERR_CHECK(  SG_cmd_util__get_username_for_repo(pCtx, paszArgs[0], &szWhoami)  );
		if (szWhoami)
		{
			SG_password__get(pCtx, paszArgs[1], szWhoami, &pPassword);
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTIMPLEMENTED);
			if (pPassword)
			{
				szPass = SG_string__sz(pPassword);
				bHasSavedCredentials = SG_TRUE;
			}
		}
	}

    INVOKE(

		do_cmd_clone(pCtx, paszArgs[0], paszArgs[1], pOptSt, szWhoami, szPass, NULL);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			do
			{
				SG_context__err_reset(pCtx);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_FALSE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);

				if(!SG_context__has_err(pCtx))
					do_cmd_clone(pCtx, paszArgs[0], paszArgs[1], pOptSt, SG_string__sz(pUsername), SG_string__sz(pPassword), &bNewIsRemote);

				if ( (pOptSt->bRemember) && (!SG_context__has_err(pCtx)) )
				{
					if (bNewIsRemote)
						SG_cmd_util__set_password(pCtx, paszArgs[1], pUsername, pPassword);
					else
						SG_cmd_util__set_password(pCtx, paszArgs[0], pUsername, pPassword);
				}

				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

fail:
	SG_NULLFREE(pCtx, paszArgs);

	SG_NULLFREE(pCtx, szWhoami);
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
}

DECLARE_CMD_FUNC(checkout)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	// we have "vv checkout [<options>] <repository> [<pathname>]"
	// so we should have 1 or 2 non-option args.
	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
	if ((count_args < 1) || (count_args > 2))
	{
		*pbUsageError = SG_TRUE;
		goto fail;
	}
	
	INVOKE(  wc__do_cmd_checkout(pCtx,
								 paszArgs[0],
								 ((count_args > 1) ? paszArgs[1] : NULL),
								 pOptSt->pRevSpec,
								 pOptSt->psz_attach,
								 pOptSt->psz_attach_new,
								 pOptSt->pvaSparse)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(fast_import)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 2, pbUsageError)  );

	INVOKE(  SG_fast_import__import(pCtx, paszArgs[0], paszArgs[1], pOptSt->psz_storage, pOptSt->psz_hash, pOptSt->psz_shared_users)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(fast_export)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 2, pbUsageError)  );

	INVOKE(  SG_fast_export__export(pCtx, paszArgs[0], paszArgs[1])  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(svn_load)
{
	const char**     aArgs        = NULL;
	SG_pathname*     pCwd         = NULL;
	SG_bool          bRoot        = SG_TRUE;
	SG_pathname*     pRoot        = NULL;
	SG_string*       sRepo        = NULL;
	const char*      szRepo       = NULL;
	SG_repo*         pRepo        = NULL;
	char*            szMyParent   = NULL;
	char*            szMyBranch   = NULL;
	const char*      szBranch     = NULL;
	const char*      szPath       = NULL;
	const char*      szStamp      = NULL;
	SG_bool          bSrcComment  = SG_FALSE;
	SG_readstream*   pStdin       = NULL;
	SG_bool          bHasMerge    = SG_FALSE;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	// parse arguments
	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &aArgs, 0u, pbUsageError)  );

	// check if the CWD is in a working copy
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pCwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pCwd)  );
	SG_ERR_TRY(  SG_workingdir__find_mapping(pCtx, pCwd, &pRoot, &sRepo, NULL)  );
	SG_ERR_CATCH_SET(SG_ERR_NOT_A_WORKING_COPY, bRoot, SG_FALSE);
	SG_ERR_CATCH_END;

	// find the required data
	if (bRoot != SG_FALSE)
	{
		// we're in a working copy, all the required data is implied by it

		// make sure they didn't try to explicitly specify anything
		if (pOptSt->psz_repo != NULL)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Cannot specify a repository when inside a working copy."));
		}
		if (SG_rev_spec__is_empty(pCtx, pOptSt->pRevSpec) == SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Cannot specify an initial parent when inside a working copy."));
		}
		if (pOptSt->psz_attach != NULL)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Cannot specify a branch attachment when inside a working copy."));
		}
		if (pOptSt->bAllowDetached != SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Cannot specify a branch attachment when inside a working copy."));
		}

		// repo
		szRepo = SG_string__sz(sRepo);
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, szRepo, &pRepo)  );

		// parent
		SG_ERR_CHECK(  SG_wc__get_wc_baseline(pCtx, pRoot, &szMyParent, &bHasMerge)  );
		// TODO 2012/10/25 Andy, Should we throw if bHasMerge?  (That is, if
		// TODO            the WD has more than one parent.)

		// branch
		SG_ERR_CHECK(  SG_wc__branch__get(pCtx, pRoot, &szMyBranch)  );
		szBranch = szMyBranch;
	}
	else
	{
		// we're not in a working copy, they must specify the required data

		// repo
		if (pOptSt->psz_repo == NULL)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "No repository specified."));
		}
		else
		{
			szRepo = pOptSt->psz_repo;
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, szRepo, &pRepo)  );
		}

		// parent
		if (SG_rev_spec__is_empty(pCtx, pOptSt->pRevSpec) != SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "No initial parent specified."));
		}
		SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_FALSE, &szMyParent, &szMyBranch)  );

		// branch
		if (pOptSt->psz_attach != NULL && pOptSt->bAllowDetached != SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Cannot use --attach and --detached together."));
		}
		if (pOptSt->psz_attach != NULL)
		{
			szBranch = pOptSt->psz_attach;
		}
		else if (pOptSt->bAllowDetached != SG_FALSE)
		{
			szBranch = NULL;
		}
		else if (szMyBranch != NULL)
		{
			szBranch = szMyBranch;
		}
		else
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "No branch attachment specified."));
		}
	}

	// find optional data
	if (pOptSt->psz_path != NULL)
	{
		szPath = pOptSt->psz_path;
	}
	if (pOptSt->psa_stamps != NULL)
	{
		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pOptSt->psa_stamps, 0u, &szStamp)  );
	}
	if (pOptSt->bSrcComment != SG_FALSE)
	{
		bSrcComment = SG_TRUE;
	}

	// check if they confirmed the operation or not
	if (pOptSt->bConfirm == SG_FALSE)
	{
		// not confirmed, print out what will happen and advise a backup
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "This command would import a Subversion dump stream into an existing Veracity\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "repository.\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "The following parameters would be used while importing:\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\tTarget Repository:          %s\n", szRepo)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\tInitial Parent Changeset:   %s\n", szMyParent)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\tTarget Branch:              %s\n", szBranch == NULL ? "(detached)" : szBranch)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\tSource Subversion Path:     %s\n", szPath == NULL ? "(all)" : szPath)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\tImported Changeset Stamp:   %s\n", szStamp == NULL ? "(none)" : szStamp)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\tImported Changeset Comment: %s\n", bSrcComment == SG_FALSE ? "no" : "yes")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Because not all errors can be predicted, and because any already-imported\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "changesets remain in the Veracity repository even if an error occurs, it is\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "recommended that you backup the target repository prior to importing any data.\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "After the repository has been backed up, re-run this command with the --confirm\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "flag to actually process and import the stream.\n")  );
	}
	else
	{
		// confirmed, run the import
		SG_ERR_CHECK(  SG_readstream__alloc__for_stdin(pCtx, &pStdin, SG_TRUE)  );
		INVOKE(  SG_svn_load__import(pCtx, pRepo, pStdin, szMyParent, szBranch, szPath, szStamp, bSrcComment)  );
	}

fail:
	SG_NULLFREE(pCtx, aArgs);
	SG_PATHNAME_NULLFREE(pCtx, pCwd);
	SG_PATHNAME_NULLFREE(pCtx, pRoot);
	SG_STRING_NULLFREE(pCtx, sRepo);
	SG_NULLFREE(pCtx, szMyParent);
	SG_NULLFREE(pCtx, szMyBranch);
	SG_READSTREAM_NULLCLOSE(pCtx, pStdin);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

DECLARE_CMD_FUNC(export)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	// we have "vv export [<options>] <repository> [<pathname>]"
	// so we should have 1 or 2 non-option args.
	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
	if ((count_args < 1) || (count_args > 2))
	{
		*pbUsageError = SG_TRUE;
		goto fail;
	}

	INVOKE(  vv2__do_cmd_export(pCtx,
								paszArgs[0],
								((count_args > 1) ? paszArgs[1] : NULL),
								pOptSt->pRevSpec,
								pOptSt->pvaSparse)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(rename)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);
	SG_UNUSED(pOptSt);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 2, pbUsageError)  );

	INVOKE(  wc__do_cmd_rename(pCtx, pOptSt, paszArgs[0], paszArgs[1])  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(resolve2)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	INVOKE(  do_cmd_resolve2(pCtx, pOptSt, count_args, paszArgs)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(move)
{
	SG_stringarray * psaArgs = NULL;
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);
	SG_UNUSED(pOptSt);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if (count_args < 2)
	{
		*pbUsageError = SG_TRUE;
	}
	else
	{
		const char * pszDest = paszArgs[count_args-1];

		SG_ERR_CHECK(  SG_util__convert_argv_into_stringarray(pCtx, count_args-1, paszArgs, &psaArgs)  );
		if (!psaArgs || !*pszDest)
			*pbUsageError = SG_TRUE;

		INVOKE(  wc__do_cmd_move(pCtx, pOptSt, psaArgs, pszDest)  );
	}

fail:
	SG_NULLFREE(pCtx, paszArgs);
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(init_new_repo)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	const char * pszRepoName = NULL;
	const char * pszFolder = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if ((count_args != 2))
	{
		*pbUsageError = SG_TRUE;				// we only allow: repo_name folder
	}
	else
	{
		pszRepoName = paszArgs[0];
		pszFolder = paszArgs[1];
	}

	INVOKE(  vv2__do_cmd_init_new_repo(pCtx,
									   pszRepoName,
									   pszFolder,
									   pOptSt->psz_storage,
									   pOptSt->psz_hash,
									   pOptSt->psz_shared_users)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(parents)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 0, pbUsageError)  );

	INVOKE(  wc__do_cmd_parents(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}


DECLARE_CMD_FUNC(scan)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 0, pbUsageError)  );

	INVOKE(  wc__do_cmd_scan(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(addremove)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	if (!psaArgs)	// a complete ADDREMOVE starting from root.
	{
		if (!pOptSt->bRecursive)		// doesn't make any sense to do -N on root since
			*pbUsageError = SG_TRUE;	// the root dir itself can't be added/removed.
	}

	INVOKE(  wc__do_cmd_addremove(pCtx, pOptSt, psaArgs)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

//////////////////////////////////////////////////////////////////

DECLARE_CMD_FUNC(status)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(  vvwc__do_cmd_status(pCtx, pOptSt, psaArgs)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(mstatus)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 0, pbUsageError)  );

	INVOKE(  vvwc__do_cmd_mstatus(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

//////////////////////////////////////////////////////////////////

DECLARE_CMD_FUNC(stamp)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if (strcmp(pszCommandName, "stamps") == 0)
	{
		if (count_args != 0)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		INVOKE(  vv2__do_cmd_stamps(pCtx, pOptSt)  );
	}
	else
	{
		if (count_args == 0)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		INVOKE(  vv2__do_cmd_stamp__subcommand(pCtx, pOptSt, count_args, paszArgs)  );
	}

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(comment)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if (count_args > 0)
	{
		*pbUsageError = SG_TRUE;
	}
	else
	{
		INVOKE(  vv2__do_cmd_comment(pCtx, pOptSt)  );
	}

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(tag)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if ((strcmp(pszCommandName, "tags") == 0) || (strcmp(pszCommandName, "labels") == 0))
	{
		if (count_args != 0)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		INVOKE(  vv2__do_cmd_tags(pCtx, pOptSt)  );
	}
	else
	{
		if (count_args == 0)
			SG_ERR_THROW(  SG_ERR_USAGE  );

		INVOKE(  vv2__do_cmd_tag__subcommand(pCtx, pOptSt, count_args, paszArgs)  );
	}

fail:
	SG_NULLFREE(pCtx, paszArgs);
}


DECLARE_CMD_FUNC(cat)
{
	SG_stringarray * psaArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );
	if (!psaArgs)
		SG_ERR_THROW(  SG_ERR_USAGE  );

	INVOKE(  vv2__do_cmd_cat(pCtx, pOptSt, psaArgs)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(upgrade)
{
	SG_uint32 count_args = 0;
	const char** paszArgs = NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	INVOKE(  do_cmd_upgrade(pCtx, count_args, paszArgs, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}


DECLARE_CMD_FUNC(vcdiff)
{
	SG_uint32 count_args = 0;
	const char** paszArgs = NULL;

	SG_UNUSED(pOptSt);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	INVOKE(  do_cmd_vcdiff(pCtx, count_args, paszArgs)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}


DECLARE_CMD_FUNC(diff)
{
	SG_stringarray * psaArgs = NULL;
	SG_uint32 nrErrors = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);

	SG_ERR_CHECK(  SG_getopt__parse_all_args__stringarray(pCtx, pGetopt, &psaArgs)  );

	INVOKE(  vvwc__do_cmd_diff(pCtx, pOptSt, psaArgs, &nrErrors)  );
	// Exit with 2 if any difftools had a problem.
	// We don't have an error in pCtx because vvwc__do_cmd_diff()
	// already printed the details.
	*pExitStatus = ((nrErrors > 0) ? 2 : 0);

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaArgs);
}

DECLARE_CMD_FUNC(diffmerge)
{
	SG_uint32 count_args = 0;
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if(count_args>0)
	{
		*pbUsageError = SG_TRUE;
	}
	else
	{
		INVOKE(  wc__do_cmd_diffmerge(pCtx, pOptSt)  );
	}
	
fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(merge)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	// Verify zero args.
	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 0, pbUsageError)  );

	INVOKE(  wc__do_cmd_merge(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(merge_preview)
{
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 0, pbUsageError)  );

	INVOKE(  do_cmd_merge_preview(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(branch)
{
	const char** paszArgs = NULL;
	SG_uint32 count_args = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	if (*pbUsageError)
	{
		goto fail;
	}

	INVOKE(  do_cmd_branch(pCtx, !strcmp(pszCommandName,"branches"), count_args, paszArgs, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(leaves)
{
	SG_uint32 count_args = 0;
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
	if (count_args)
		*pbUsageError = SG_TRUE;

	INVOKE(  vv2__do_cmd_leaves(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

#if defined(WINDOWS)
DECLARE_CMD_FUNC(forget_passwords)
{
	SG_uint32 count_args = 0;
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
	if (*pbUsageError)
		goto fail;

	INVOKE(  do_cmd_forget_passwords(pCtx, pOptSt->bNoPrompt)  );

fail:
	;
}
#endif

DECLARE_CMD_FUNC(heads)
{
	SG_uint32 count_args = 0;
	const char** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );
	if (count_args)
		*pbUsageError = SG_TRUE;

	INVOKE(  do_cmd_heads(pCtx, pOptSt->psz_repo, pOptSt->psa_branch_names, pOptSt->bAll, pOptSt->bVerbose)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

DECLARE_CMD_FUNC(update)
{
	const char ** paszArgs = NULL;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	// Verify zero args.
	SG_ERR_CHECK(  _parse_and_count_args(pCtx, pGetopt, &paszArgs, 0, pbUsageError)  );

	INVOKE(  wc__do_cmd_update(pCtx, pOptSt)  );

fail:
	SG_NULLFREE(pCtx, paszArgs);
}

static void _parse_pull_args(
	SG_context* pCtx, 
	SG_option_state* pOptSt,
	SG_getopt* pGetopt,
	SG_pathname** ppPathCwd,
	const char** ppszDestRepoDescriptorName,
	const char** ppszSrcRepoSpec)
{
	const char ** paszArgs = NULL;
	SG_uint32 count_args;
	SG_pathname* pPathCwd = NULL;
	SG_string* pstrCwdDescriptorName = NULL;
	char* pszDestRepoDescriptorName = NULL;
	char* pszSrcRepoSpec = NULL;
	const char* pszRefSrcRepoSpec;
	SG_string* pstrSavedPath = NULL;

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	// Determine destination repo
	if (pOptSt->psz_dest_repo)
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pOptSt->psz_dest_repo, &pszDestRepoDescriptorName)  );
		if (pOptSt->bUpdate)
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\nThe update and dest flags cannot be combined.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}
	}
	else
	{
		SG_ERR_CHECK(  SG_cmd_util__get_descriptor_name_from_cwd(pCtx, &pstrCwdDescriptorName, &pPathCwd)  );
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrCwdDescriptorName, (SG_byte**)&pszDestRepoDescriptorName, NULL)  );
	}

	// Determine source repo
	if (0 == count_args)
	{
		if (!pszDestRepoDescriptorName)
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\nNot in a working copy and no SOURCEREPO specified.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}

		SG_localsettings__descriptor__get__sz(pCtx, pszDestRepoDescriptorName, "paths/default", &pszSrcRepoSpec);
		if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
		{
			SG_ERR_DISCARD;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\nNo default path and no SOURCEREPO specified.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}
		if (SG_context__err_equals(pCtx, SG_ERR_JSONDB_INVALID_PATH))
		{
			SG_ERR_DISCARD;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\ndest must be a local repository name.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}
		else
			SG_ERR_CHECK_CURRENT;
		SG_ERR_CHECK_CURRENT;

		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszSrcRepoSpec, (char**)&pszRefSrcRepoSpec)  );
	}
	else if (1 == count_args)
	{
		// A source repo argument was provided.

		SG_bool bExists = SG_FALSE;

		// Is it an http url?
		if (strlen(paszArgs[0]) > 6)
		{
			if (SG_strnicmp("http://", paszArgs[0], 7) == 0)
				bExists = SG_TRUE; // remote repo, existence is determined elsewhere
			else if (SG_strnicmp("https://", paszArgs[0], 8) == 0)
				bExists = SG_TRUE; // remote repo, existence is determined elsewhere
		}

		// Is it a valid descriptor name?
		if (!bExists)
			SG_ERR_CHECK(  SG_closet__descriptors__exists(pCtx, paszArgs[0], &bExists)  );

		if (bExists)
		{
		  SG_ERR_CHECK(  SG_STRDUP(pCtx, paszArgs[0], (char**)&pszRefSrcRepoSpec)  );
		}
		else // Is it a saved alias?
		{
			if (!pszDestRepoDescriptorName)
			{
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
					"\nThere is no local repository instance matching %s.\n\n",
					paszArgs[0])  );
				SG_ERR_THROW(SG_ERR_USAGE);
			}

			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrSavedPath)  );
			SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstrSavedPath, "paths/%s", paszArgs[0])  );

			SG_localsettings__descriptor__get__sz(pCtx, pszDestRepoDescriptorName,
				SG_string__sz(pstrSavedPath), &pszSrcRepoSpec);
			if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			{
				SG_ERR_DISCARD;
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
					"\nThere is no local repository instance or saved alias matching %s.\n\n",
					paszArgs[0])  );
				SG_ERR_THROW(SG_ERR_USAGE);
			}
			SG_ERR_CHECK_CURRENT;

			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszSrcRepoSpec, (char**)&pszRefSrcRepoSpec)  );
		}
	}
	else
	{
		SG_ERR_THROW(SG_ERR_USAGE);
	}

	SG_RETURN_AND_NULL(pPathCwd, ppPathCwd);
	SG_RETURN_AND_NULL(pszRefSrcRepoSpec, ppszSrcRepoSpec);
	SG_RETURN_AND_NULL(pszDestRepoDescriptorName, ppszDestRepoDescriptorName);

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_STRING_NULLFREE(pCtx, pstrCwdDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRepoSpec);
	SG_NULLFREE(pCtx, paszArgs);
	SG_STRING_NULLFREE(pCtx, pstrSavedPath);
	SG_NULLFREE(pCtx, pszDestRepoDescriptorName);
}

DECLARE_CMD_FUNC(incoming)
{
	const char* pszDestRepoDescriptorName = NULL;
	const char* pszSrcRepoSpec = NULL;

	char* szWhoami = NULL;
	SG_string* pUsername = NULL;
	SG_string* pPassword = NULL;
	const char *pp = NULL;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_pull_args(pCtx, pOptSt, pGetopt, NULL, &pszDestRepoDescriptorName, &pszSrcRepoSpec)  );
	INVOKE(
		SG_cmd_util__get_username_for_repo(pCtx, pszDestRepoDescriptorName, &szWhoami);

		if (szWhoami)
		{
			SG_password__get(pCtx, pszSrcRepoSpec, szWhoami, &pPassword);
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTIMPLEMENTED);

			if (pPassword)
			{
				pp = SG_string__sz(pPassword);
				bHasSavedCredentials = SG_TRUE;
			}
		}

		do_cmd_incoming(pCtx, pszSrcRepoSpec, szWhoami, pp, pszDestRepoDescriptorName, pOptSt->bVerbose, pOptSt->bTrace);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			do
			{
				SG_context__err_reset(pCtx);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_TRUE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);
				if(!SG_context__has_err(pCtx))
					do_cmd_incoming(pCtx, pszSrcRepoSpec, SG_string__sz(pUsername), SG_string__sz(pPassword), pszDestRepoDescriptorName, pOptSt->bVerbose, pOptSt->bTrace);

				if ( (pOptSt->bRemember) && (!SG_context__has_err(pCtx)) )
				{
					SG_cmd_util__set_password(pCtx, pszSrcRepoSpec, pUsername, pPassword);
				}

				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

	/* fall through */
fail:
	if (SG_context__has_err(pCtx))
		*pbUsageError = SG_TRUE;
	SG_NULLFREE(pCtx, pszDestRepoDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRepoSpec);

	SG_NULLFREE(pCtx, szWhoami);
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
}

DECLARE_CMD_FUNC(pull)
{
	const char* pszDestRepoDescriptorName = NULL;
	const char* pszSrcRepoSpec = NULL;

	char* szWhoami = NULL;
	SG_string* pUsername = NULL;
	SG_string* pPassword = NULL;
	const char *pp = NULL;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_pull_args(pCtx, pOptSt, pGetopt, NULL, &pszDestRepoDescriptorName, &pszSrcRepoSpec)  );

	INVOKE(

		SG_cmd_util__get_username_for_repo(pCtx, pszDestRepoDescriptorName, &szWhoami);

		if (szWhoami)
		{
			SG_password__get(pCtx, pszSrcRepoSpec, szWhoami, &pPassword);
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTIMPLEMENTED);

			if (pPassword)
			{
				pp = SG_string__sz(pPassword);
				bHasSavedCredentials = SG_TRUE;
			}
		}

		do_cmd_pull(pCtx, pOptSt, pszSrcRepoSpec, szWhoami, pp, pszDestRepoDescriptorName);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			if (!szWhoami)
				SG_cmd_util__get_username_for_repo(pCtx, pszDestRepoDescriptorName, &szWhoami);
			do
			{
				SG_context__err_reset(pCtx);

				SG_STRING_NULLFREE(pCtx, pPassword);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_TRUE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);

				if(!SG_context__has_err(pCtx))
					do_cmd_pull(pCtx, pOptSt, pszSrcRepoSpec, SG_string__sz(pUsername), SG_string__sz(pPassword), pszDestRepoDescriptorName);

				if ( (pOptSt->bRemember) && (!SG_context__has_err(pCtx)) )
				{
					SG_cmd_util__set_password(pCtx, pszSrcRepoSpec, pUsername, pPassword);
				}

				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

	/* fall through */
fail:
	if (SG_context__has_err(pCtx))
		*pbUsageError = SG_TRUE;
	SG_NULLFREE(pCtx, pszDestRepoDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRepoSpec);

	SG_NULLFREE(pCtx, szWhoami);
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
}

static void _parse_push_args(
	SG_context* pCtx, 
	SG_option_state* pOptSt,
	SG_getopt* pGetopt,
	const char** ppszSrcRepoDescriptorName,
	const char** ppszDestRepoSpec)
{
	const char ** paszArgs = NULL;
	SG_uint32 count_args;
	SG_string* pstrCwdDescriptorName = NULL;
	const char* pszSrcRepoDescriptorName = NULL;
	char* pszDestRepoSpec = NULL;
	const char* pszRefDestRepoSpec;
	SG_string* pstrSavedPath = NULL;

	SG_ERR_CHECK(  SG_getopt__parse_all_args(pCtx, pGetopt, &paszArgs, &count_args)  );

	// Determine source repo
	if (pOptSt->psz_src_repo)
	  SG_ERR_CHECK(  SG_STRDUP(pCtx, pOptSt->psz_src_repo, (char**)&pszSrcRepoDescriptorName)  );
	else
	{
		SG_ERR_CHECK(  SG_cmd_util__get_descriptor_name_from_cwd(pCtx, &pstrCwdDescriptorName, NULL)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pstrCwdDescriptorName), (char**)&pszSrcRepoDescriptorName)  );
	}

	// Determine dest repo
	if (0 == count_args)
	{
		if (!pszSrcRepoDescriptorName)
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\nNot in a working copy and no DESTREPO specified.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}
		SG_ERR_CHECK_CURRENT;

		SG_localsettings__descriptor__get__sz(pCtx, pszSrcRepoDescriptorName, "paths/default", &pszDestRepoSpec);
		if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
		{
			SG_ERR_DISCARD;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\nNo default path and no DESTREPO specified.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}
		if (SG_context__err_equals(pCtx, SG_ERR_JSONDB_INVALID_PATH))
		{
			SG_ERR_DISCARD;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\nsrc must be a local repository name.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}
		else
			SG_ERR_CHECK_CURRENT;

		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszDestRepoSpec, (char**)&pszRefDestRepoSpec)  );
	}
	else if (1 == count_args)
	{
		// A dest repo argument was provided.

		SG_bool bExists = SG_FALSE;

		// Is it an http url?
		if (strlen(paszArgs[0]) > 6)
		{
			if (SG_strnicmp("http://", paszArgs[0], 7) == 0)
				bExists = SG_TRUE; // remote repo, existence is determined elsewhere
			else if (SG_strnicmp("https://", paszArgs[0], 8) == 0)
				bExists = SG_TRUE; // remote repo, existence is determined elsewhere
		}

		// Is it a valid descriptor name?
		if (!bExists)
			SG_ERR_CHECK(  SG_closet__descriptors__exists(pCtx, paszArgs[0], &bExists)  );

		if (bExists)
		{
		  SG_ERR_CHECK(  SG_STRDUP(pCtx, paszArgs[0], (char**)&pszRefDestRepoSpec)  );
		}
		else // Is it a saved alias?
		{
			if (!pszSrcRepoDescriptorName)
			{
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
					"\nThere is no local repository instance matching %s.\n\n",
					paszArgs[0])  );
				SG_ERR_THROW(SG_ERR_USAGE);
			}

			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrSavedPath)  );
			SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstrSavedPath, "paths/%s", paszArgs[0])  );

			SG_localsettings__descriptor__get__sz(pCtx, pszSrcRepoDescriptorName,
				SG_string__sz(pstrSavedPath), &pszDestRepoSpec);
			if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			{
				SG_ERR_DISCARD;
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
					"\nThere is no local repository instance or saved alias matching %s.\n\n",
					paszArgs[0])  );
				SG_ERR_THROW(SG_ERR_USAGE);
			}
			SG_ERR_CHECK_CURRENT;

			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszDestRepoSpec, (char**)&pszRefDestRepoSpec)  );
		}
	}
	else
	{
		SG_ERR_THROW(SG_ERR_USAGE);
	}

	SG_RETURN_AND_NULL(pszRefDestRepoSpec, ppszDestRepoSpec);
	SG_RETURN_AND_NULL(pszSrcRepoDescriptorName, ppszSrcRepoDescriptorName);

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pstrCwdDescriptorName);
	SG_NULLFREE(pCtx, pszDestRepoSpec);
	SG_NULLFREE(pCtx, pszSrcRepoDescriptorName);
	SG_NULLFREE(pCtx, paszArgs);
	SG_STRING_NULLFREE(pCtx, pstrSavedPath);
}

DECLARE_CMD_FUNC(outgoing)
{
	const char* pszSrcRepoDescriptorName = NULL;
	const char* pszDestRepoSpec = NULL;

	char* szWhoami = NULL;
	SG_string* pUsername = NULL;
	SG_string* pPassword = NULL;
	const char *pp = NULL;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszAppName);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pExitStatus);

	SG_ERR_CHECK(  _parse_push_args(pCtx, pOptSt, pGetopt, &pszSrcRepoDescriptorName, &pszDestRepoSpec)  );
	INVOKE(
		SG_cmd_util__get_username_for_repo(pCtx, pszSrcRepoDescriptorName, &szWhoami);

		if (szWhoami)
		{
			SG_password__get(pCtx, pszDestRepoSpec, szWhoami, &pPassword);
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTIMPLEMENTED);

			if (pPassword)
			{
				pp = SG_string__sz(pPassword);
				bHasSavedCredentials = SG_TRUE;
			}
		}

		do_cmd_outgoing(pCtx, pszDestRepoSpec, szWhoami, pp, pszSrcRepoDescriptorName, pOptSt->bVerbose, pOptSt->bTrace);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			do
			{
				SG_context__err_reset(pCtx);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_TRUE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);
				if(!SG_context__has_err(pCtx))
					do_cmd_outgoing(pCtx, pszDestRepoSpec, SG_string__sz(pUsername), SG_string__sz(pPassword), pszSrcRepoDescriptorName, pOptSt->bVerbose, pOptSt->bTrace);
				if ( (pOptSt->bRemember) && (!SG_context__has_err(pCtx)) )
				{
					SG_cmd_util__set_password(pCtx, pszDestRepoSpec, pUsername, pPassword);
				}

				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

	/* fall through */
fail:
	if (SG_context__has_err(pCtx))
		*pbUsageError = SG_TRUE;
	SG_NULLFREE(pCtx, pszSrcRepoDescriptorName);
	SG_NULLFREE(pCtx, pszDestRepoSpec);

	SG_NULLFREE(pCtx, szWhoami);
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
}

DECLARE_CMD_FUNC(push)
{
	const char* pszSrcRepoDescriptorName = NULL;
	const char* pszDestRepoSpec = NULL;

	char* szWhoami = NULL;
	SG_string* pUsername = NULL;
	SG_string* pPassword = NULL;
	const char *pp = NULL;
	SG_bool bHasSavedCredentials = SG_FALSE;
	SG_uint32 kAttempt = 0;

	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);

	SG_ERR_CHECK(  _parse_push_args(pCtx, pOptSt, pGetopt, &pszSrcRepoDescriptorName, &pszDestRepoSpec)  );
	INVOKE(
		SG_cmd_util__get_username_for_repo(pCtx, pszSrcRepoDescriptorName, &szWhoami);

		if (szWhoami)
		{
			SG_password__get(pCtx, pszDestRepoSpec, szWhoami, &pPassword);
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTIMPLEMENTED);

			if (pPassword)
			{
				pp = SG_string__sz(pPassword);
				bHasSavedCredentials = SG_TRUE;
			}
		}

		do_cmd_push(pCtx, pOptSt, pszDestRepoSpec, szWhoami, pp, pszSrcRepoDescriptorName, pExitStatus);

		if(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_context__err_reset(pCtx);
			if (!szWhoami)
				SG_cmd_util__get_username_for_repo(pCtx, pszSrcRepoDescriptorName, &szWhoami);
			do
			{
				SG_context__err_reset(pCtx);
				SG_STRING_NULLFREE(pCtx, pPassword);
				SG_cmd_util__get_username_and_password(pCtx, szWhoami, SG_TRUE, bHasSavedCredentials, kAttempt++, &pUsername, &pPassword);
				if(!SG_context__has_err(pCtx))
					do_cmd_push(pCtx, pOptSt, pszDestRepoSpec, SG_string__sz(pUsername), SG_string__sz(pPassword), pszSrcRepoDescriptorName, pExitStatus);

				if ( (pOptSt->bRemember) && (!SG_context__has_err(pCtx)) )
				{
					SG_cmd_util__set_password(pCtx, pszDestRepoSpec, pUsername, pPassword);
				}

				SG_STRING_NULLFREE(pCtx, pUsername);
				SG_STRING_NULLFREE(pCtx, pPassword);
			}
			while(SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED));
		}

	);

	/* fall through */
fail:
	if (SG_context__has_err(pCtx))
		*pbUsageError = SG_TRUE;
	SG_NULLFREE(pCtx, pszSrcRepoDescriptorName);
	SG_NULLFREE(pCtx, pszDestRepoSpec);

	SG_NULLFREE(pCtx, szWhoami);
	SG_STRING_NULLFREE(pCtx, pUsername);
	SG_STRING_NULLFREE(pCtx, pPassword);
}

#if defined(HAVE_TEST_CRASH)
DECLARE_CMD_FUNC(crash)
{
	SG_uint32 * p = NULL;

	SG_UNUSED(pCtx);
	SG_UNUSED(pGetopt);
	SG_UNUSED(pOptSt);
	SG_UNUSED(pbUsageError);
	SG_UNUSED(pszCommandName);
	SG_UNUSED(pszAppName);
	SG_UNUSED(pExitStatus);

	// Time to bite the dust...

	*p = 1;

}
#endif

//////////////////////////////////////////////////////////////////

/**
 * convert (argc,argv) into a SG_getopt.  This takes care of converting
 * platform strings into UTF-8.
 *
 */

static void _intern_argv(SG_context * pCtx, int argc, ARGV_CHAR_T ** argv, SG_getopt ** ppGetopt)
{
	SG_ASSERT_RELEASE_RETURN(argc > 0);

	SG_ERR_CHECK_RETURN(  SG_getopt__alloc(pCtx, argc, argv, ppGetopt)  );

	(*ppGetopt)->interleave = 1;
}

static void _parse_option_port(SG_context *pCtx, const char *portname, const char *optstr, SG_int32 *pval)
{
	SG_int64 val;

	SG_ASSERT(SG_INT32_MAX >= 65535);
	SG_NULLARGCHECK_RETURN(portname);
	SG_NULLARGCHECK_RETURN(optstr);
	SG_NULLARGCHECK_RETURN(pval);

	SG_ERR_CHECK(  SG_int64__parse(pCtx, &val, optstr)  );

	if ((val < 1) || (val > 65535))
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "invalid value for --%s: %li -- must be an integer from 1 to 65535", portname, (long)val)  );
		SG_context__err(pCtx, SG_ERR_USAGE, __FILE__, __LINE__, "invalid port value");
	}
	else
	{
		*pval = (SG_int32)val;
	}

fail:
	;
}

static void _parse_options(SG_context * pCtx, SG_getopt ** ppGetopt, SG_option_state ** ppOptSt)
{
	SG_option_state* pOptSt = *ppOptSt;
	const char* pszOptArg = NULL;
	SG_uint32 iOptId;
	SG_bool bNoMoreOptions = SG_FALSE;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pOptSt)  );
	SG_ERR_CHECK(  SG_alloc(pCtx, (*ppGetopt)->count_args, sizeof(SG_int32), &pOptSt->paRecvdOpts)  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pOptSt->pRevSpec)  );

	// init default values -- NOTE only need to set things that are non-zero (because of above calloc())
	pOptSt->bRecursive = SG_TRUE;
	pOptSt->bReverseOutput = SG_FALSE;
	pOptSt->bPromptForDescription = SG_TRUE;
	pOptSt->iPort = 8080;
	pOptSt->iSPort = 0;
	pOptSt->iMaxHistoryResults = SG_UINT32_MAX;
	pOptSt->iStartLine = 0;
	pOptSt->iLength = 0;

	while (1)
	{
		SG_getopt__long(pCtx, *ppGetopt, sg_cl_options, &iOptId, &pszOptArg, &bNoMoreOptions);

		if (bNoMoreOptions)
		{
			SG_context__err_reset(pCtx);
			break;
		}
		else if (SG_context__has_err(pCtx))
		{
			const char* pszErr;
			SG_context__err_get_description(pCtx, &pszErr);
			SG_context__push_level(pCtx);
			SG_console(pCtx, SG_CS_STDERR, "\n");
			SG_console__raw(pCtx, SG_CS_STDERR, pszErr);
			SG_console(pCtx, SG_CS_STDERR, "\n");
			SG_context__pop_level(pCtx);
			SG_ERR_RETHROW;
		}

		pOptSt->optCountArray[iOptId]++;

		pOptSt->paRecvdOpts[pOptSt->count_options] = iOptId;
		pOptSt->count_options++;

		// TODO:  we should probably do some checking per option here
		// i.e. do the strings being passed in look like they should
		// for things like opt_changeset

		switch(iOptId)
		{
		case opt_all:
			{
				pOptSt->bAll = SG_TRUE;
			}
			break;
		case opt_all_full:
			{
				pOptSt->b_all_full = SG_TRUE;
			}
			break;
		case opt_all_zlib:
			{
				pOptSt->b_all_zlib = SG_TRUE;
			}
			break;
		case opt_all_but:
			{
				pOptSt->bAllBut = SG_TRUE;
			}
			break;
		case opt_show_all:
			{
				pOptSt->bShowAll = SG_TRUE;
			}
			break;
		case 'a':
			{
				// if not alloc'ed, alloc
				// add assoc to psa_assocs/increment counter at the end
				if (!pOptSt->psa_assocs)
				{
					SG_ERR_CHECK( SG_STRINGARRAY__ALLOC(pCtx, &pOptSt->psa_assocs, pOptSt->count_options) );
				}
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, pOptSt->psa_assocs, pszOptArg)  );
			}
			break;
		case opt_cert:
			{
				if (!pOptSt->psz_cert)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_cert)  );
			}
			break;
		case 'C':
			{
				pOptSt->bClean = SG_TRUE;
			}
			break;
        case opt_confirm:
            {
				pOptSt->bConfirm = SG_TRUE;
            }
            break;
		case opt_dest:
			{
				if (!pOptSt->psz_dest_repo)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_dest_repo)  );
			}
			break;
		case opt_src:
			{
				if (!pOptSt->psz_src_repo)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_src_repo)  );
			}
			break;
		case opt_src_comment:
			{
				pOptSt->bSrcComment = SG_TRUE;
			}
			break;
		case opt_server:
			{
				if (!pOptSt->psz_server)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_server)  );
			}
			break;
		case opt_verbose:
			{
				if (pOptSt->bQuiet)
					pOptSt->bQuiet = SG_FALSE;
				pOptSt->bVerbose = SG_TRUE;
			}
			break;
		case opt_status:
			{
				pOptSt->bStatus = SG_TRUE;
			}
			break;
		case opt_quiet:
			{
				if (pOptSt->bVerbose)
					pOptSt->bVerbose = SG_FALSE;
				pOptSt->bQuiet = SG_TRUE;
			}
		case opt_pull:
			{
				pOptSt->bPull = SG_TRUE;
			}
			break;
		case 'F':
			{
				pOptSt->bForce = SG_TRUE;
			}
			break;
		case opt_global:
			{
				pOptSt->bGlobal = SG_TRUE;
			}
			break;
		case opt_no_ignores:
			{
				pOptSt->bNoIgnores = SG_TRUE;
			}
			break;

		case 'm':
			{
				if (!pOptSt->psz_message)
				{
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_message)  );
					pOptSt->bPromptForDescription = SG_FALSE;
				}
			}
			break;
		case 'N':
			{
				pOptSt->bRecursive = SG_FALSE;
			}
			break;
		case 'o':
			{
				if (!pOptSt->psz_output_file)
				{
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_output_file)  );
				}
			}
			break;
		case opt_no_plain:
			{
				pOptSt->bNoPlain = SG_TRUE;
			}
			break;
		case opt_no_prompt:
			{
				pOptSt->bNoPrompt = SG_TRUE;
			}
			break;
		case 'P':
			{
				pOptSt->bPublic = SG_TRUE;
			}
			break;
		case opt_port:
			{
				SG_ERR_CHECK(  _parse_option_port(pCtx, "port", pszOptArg, &pOptSt->iPort)  );
			}
			break;
		case opt_create:
			{
				pOptSt->bCreate = SG_TRUE;
			}
			break;
		case opt_subgroups:
			{
				pOptSt->bSubgroups = SG_TRUE;
			}
			break;
		case opt_remove:
			{
				pOptSt->bRemove = SG_TRUE;
			}
			break;
		case opt_detached:
			{
				pOptSt->bAllowDetached = SG_TRUE;
			}
			break;
		case 'c':
			{
				pOptSt->bAttachCurrent = SG_TRUE;
			}
			break;
		case opt_restore_backup:
			{
				if (pOptSt->psz_restore_backup == NULL)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_restore_backup)  );
			}
			break;
		case opt_path:
			{
				if (pOptSt->psz_path == NULL)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_path)  );
			}
			break;
		case opt_repo:
			{
				if (pOptSt->psz_repo == NULL)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_repo)  );
			}
			break;
		case 'b': // --branch
            {
				/* Add the option for both 'b' and OPT_REV_SPEC. 
				 * Later _inspect_all_options will make sure only the correct one is set. */

				if (pOptSt->psa_branch_names == NULL)
					SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pOptSt->psa_branch_names, 2)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, pOptSt->psa_branch_names, pszOptArg)  );

				SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pOptSt->pRevSpec, pszOptArg)  );
			}
			break;
		case opt_attach:
            {
				if (pOptSt->psz_attach == NULL)
                {
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_attach)  );
                }
            }
			break;
		case opt_attach_new:
            {
				if (pOptSt->psz_attach_new == NULL)
                {
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_attach_new)  );
                }
            }
			break;
		case 'r':
			{
				/* Add the option for both 'r' and OPT_REV_SPEC. 
				 * Later _inspect_all_options will make sure only the correct one is set. */
				if (pOptSt->psa_revs == NULL)
					SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pOptSt->psa_revs, 2)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, pOptSt->psa_revs, pszOptArg)  );

				SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pOptSt->pRevSpec, pszOptArg)  );
			}
			break;
		case opt_area:
			{
				if (!pOptSt->pvh_area_names)
                {
					SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pOptSt->pvh_area_names)  );
                }
				SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pOptSt->pvh_area_names, pszOptArg)  );
			}
			break;
		case opt_sport:
			{
				SG_ERR_CHECK(  _parse_option_port(pCtx, "sport", pszOptArg, &pOptSt->iSPort)  );
			}
			break;
		case opt_stamp:
			{
				// if not alloc'ed, alloc
				// add stamp to psa_stamps
				if (!pOptSt->psa_stamps)
				{
					SG_ERR_CHECK( SG_STRINGARRAY__ALLOC(pCtx, &pOptSt->psa_stamps, pOptSt->count_options) );
				}
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, pOptSt->psa_stamps, pszOptArg)  );
			}
			break;
		case opt_tag:
			{
				/* Add the option for both opt_tag and OPT_REV_SPEC. 
				 * Later _inspect_all_options will make sure only the correct one is set. */
				if (pOptSt->psa_tags == NULL)
					SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pOptSt->psa_tags, 2)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, pOptSt->psa_tags, pszOptArg)  );

				SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pOptSt->pRevSpec, pszOptArg)  );
			}
			break;
		case 'T':
			{
				pOptSt->bTest = SG_TRUE;
			}
			break;
		case 'U':
			{
				if (!pOptSt->psz_username)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_username)  );
			}
			break;
		case 'u':
			pOptSt->bUpdate = SG_TRUE;
			break;
		case opt_to:
			{
				if (!pOptSt->psz_to_date)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_to_date)  );
			}
			break;
		case opt_from:
			{
				if (!pOptSt->psz_from_date)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_from_date)  );
			}
			break;
		case opt_when:
			{
				if (!pOptSt->psz_when)
				SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_when)  );
			}
			break;
		case opt_leaves:
			{
				pOptSt->bLeaves = SG_TRUE;
			}
			break;
		case opt_hide_merges:
			{
				pOptSt->bHideMerges = SG_TRUE;
			}
			break;
		case opt_reverse:
			{
				pOptSt->bReverseOutput = SG_TRUE;
			}
			break;

		case opt_max:
			{
				SG_uint32 parsed = SG_UINT32_MAX;
				// not sure the best way to do this
				SG_ERR_CHECK(SG_uint32__parse__strict(pCtx, &parsed, pszOptArg));
				pOptSt->iMaxHistoryResults = parsed;
			}
			break;

		case opt_startline:
			{
				SG_uint32 parsed = 0;
				// not sure the best way to do this
				SG_ERR_CHECK(SG_uint32__parse__strict(pCtx, &parsed, pszOptArg));
				pOptSt->iStartLine = parsed;
			}
			break;

		case opt_length:
			{
				SG_uint32 parsed = 0;
				// not sure the best way to do this
				SG_ERR_CHECK(SG_uint32__parse__strict(pCtx, &parsed, pszOptArg));
				pOptSt->iLength = parsed;
			}
			break;

		case opt_list:
			{
				pOptSt->bList = SG_TRUE;
			}
			break;

		case opt_list_all:
			{
				pOptSt->bListAll = SG_TRUE;
			}
			break;

		case opt_remember:
			{
				pOptSt->bRemember = SG_TRUE;
			}
			break;

		case opt_tool:
			{
				if (!pOptSt->psz_tool)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_tool)  );
			}
			break;

		case opt_no_auto_merge:
			{
				pOptSt->bNoAutoMerge = SG_TRUE;
			}
			break;

		case opt_storage:
			{
				if (!pOptSt->psz_storage)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_storage)  );
			}
			break;

		case opt_shared_users:
			{
				if (!pOptSt->psz_shared_users)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_shared_users)  );
			}
			break;

		case opt_hash:
			{
				if (!pOptSt->psz_hash)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_hash)  );
			}
			break;

		case opt_pack:
			{
				pOptSt->bPack = SG_TRUE;
			}
			break;

		case opt_no_backups:
			{
				pOptSt->bNoBackups = SG_TRUE;
			}
			break;

		case 'k':
			{
				pOptSt->bKeep = SG_TRUE;
			}
			break;

		case opt_overwrite:
			{
				pOptSt->bOverwrite = SG_TRUE;
			}
			break;

		case opt_label:
			{
				if (!pOptSt->psz_label)
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszOptArg, &pOptSt->psz_label)  );
			}
			break;

		case opt_no_ff_merge:
			{
				pOptSt->bNoFFMerge = SG_TRUE;
			}
			break;

#if defined(DEBUG)
		case opt_dagnum:
			{
				SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, pszOptArg, &pOptSt->ui64DagNum)  );
				pOptSt->bGaveDagNum = SG_TRUE;
			}
			break;
#endif

		case opt_trace:
			{
				pOptSt->bTrace = SG_TRUE;
			}
			break;

		case opt_sparse:
			{
				if (!pOptSt->pvaSparse)
					SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pOptSt->pvaSparse)  );
				SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pOptSt->pvaSparse, pszOptArg)  );
			}

		case opt_list_unchanged:
			{
				pOptSt->bListUnchanged = SG_TRUE;
			}
			break;

		case opt_allow_lost:
			{
				pOptSt->bAllowLost = SG_TRUE;
			}
			break;

		case opt_no_allow_after_the_fact:
			{
				pOptSt->bNoAllowAfterTheFact = SG_TRUE;
			}
			break;

		case opt_allow_dirty:
			{
				pOptSt->bAllowWhenDirty = SG_TRUE;
			}
			break;

		case opt_no_fallback:
			{
				pOptSt->bNoFallback = SG_TRUE;
			}
			break;

		case 'i':
			{
				pOptSt->bInteractive = SG_TRUE;
			}
			break;

		case opt_revert_files:
			pOptSt->RevertItems_flagMask_type |= SG_WC_STATUS_FLAGS__T__FILE;
			break;
		case opt_revert_symlinks:
			pOptSt->RevertItems_flagMask_type |= SG_WC_STATUS_FLAGS__T__SYMLINK;
			break;
		case opt_revert_directories:
			pOptSt->RevertItems_flagMask_type |= SG_WC_STATUS_FLAGS__T__DIRECTORY;
			break;

		case opt_revert_lost:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__U__LOST;
			break;
		case opt_revert_added:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__S__ADDED;
			break;
		case opt_revert_removed:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__S__DELETED;
			break;
		case opt_revert_renamed:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__S__RENAMED;
			break;
		case opt_revert_moved:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__S__MOVED;
			break;
		case opt_revert_merge_added:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__S__MERGE_CREATED;
			break;
		case opt_revert_update_added:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__S__UPDATE_CREATED;
			break;
		case opt_revert_attributes:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED;
			break;
		case opt_revert_content:
			pOptSt->RevertItems_flagMask_dirt |= SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED;
			break;

		default:
			break;
		}

	}

	*ppOptSt = pOptSt;
	return;
fail:
	SG_ERR_IGNORE(  _sg_option_state__free(pCtx, pOptSt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Logs all of the command line arguments to the loggers.
 */
static void _log_cmdline(
	SG_context*      pCtx,   //< [in] [out] Error and context info.
	const SG_getopt* pGetopt //< [in] Getopt containing command line arguments.
	)
{
	SG_string* sCmdline = NULL;
	SG_uint32  uIndex   = 0u;

	SG_STRING__ALLOC(pCtx, &sCmdline);

	for (uIndex = 0u; uIndex < pGetopt->count_args; ++uIndex)
	{
		if (uIndex > 0u)
		{
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sCmdline, " ")  );
		}
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, sCmdline, pGetopt->paszArgs[uIndex])  );
	}

	SG_ERR_IGNORE(  SG_log__report_verbose(pCtx, "Performing command: %s", SG_string__sz(sCmdline))  );

fail:
	SG_STRING_NULLFREE(pCtx, sCmdline);
}

/**
 * Dispatch to appropriate command using the given SG_getopt.
 *
 */
static void _dispatch(SG_context * pCtx, const char *szCmd, const char *szAppName, const cmdinfo *pCmdInfo, SG_getopt * pGetopt, SG_option_state* pOptSt, SG_bool initialUsageError, int * pExitStatus)
{
	if (pCmdInfo)
	{
		SG_bool	bUsageError = initialUsageError;

		SG_ERR_IGNORE(  _log_cmdline(pCtx, pGetopt)  );

		// TODO consider just passing pArgv to the command functions.
		if (! bUsageError)
			SG_ERR_CHECK(  _inspect_all_options(pCtx, szAppName, szCmd, pOptSt, &bUsageError)  );

		if (! bUsageError)
		{
			SG_ERR_CHECK(  pCmdInfo->fn(
							   pCtx,
							   szAppName,
							   szCmd,
							   pGetopt,
							   pOptSt,
							   &bUsageError,
							   pExitStatus)
				);
		}

		if (bUsageError)
		{
			SG_ERR_THROW(SG_ERR_USAGE);
		}

		return;
	}

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "\nInvalid command: %s\n\n", szCmd));

	do_cmd_help__list_all_commands(pCtx, SG_CS_STDERR, szAppName, pOptSt);

	SG_ERR_THROW_RETURN(  SG_ERR_USAGE  );

fail:
	if (SG_context__err_equals(pCtx, SG_ERR_USAGE))
	{
		const char *szMessage = NULL;
		SG_error err;
		const char *szSubcommand = NULL;

		err = SG_context__err_get_description(pCtx, &szMessage);

		if (SG_IS_OK(err) && szMessage && *szMessage)
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s", szMessage)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\n")  );
		}

		// Figure out if they used a subcommand that we have specifc help text for:
		if(pGetopt && pGetopt->count_args>=3)
		{
			SG_uint32 j;
			const cmdinfo * pCmdCanonical = NULL;
			_get_canonical_command(szCmd, &pCmdCanonical);
			for(j=0;j<SUBCMDCOUNT && !szSubcommand;++j)
				if(!strcmp(pCmdCanonical->name, subcommands[j].cmdname) && !strcmp(pGetopt->paszArgs[2], subcommands[j].name))
					szSubcommand = pGetopt->paszArgs[2];
		}

		SG_ERR_IGNORE(  do_cmd_help__list_command_help(pCtx, SG_CS_STDERR, szAppName, szCmd, szSubcommand, SG_FALSE)  );
	}
}

//////////////////////////////////////////////////////////////////

/**
 * Print error message on STDERR for the given error and convert the
 * SG_ERR_ value into a proper ExitStatus for the platform.
 *
 * TODO investigate if (-1,errno) is appropriate for all 3 platforms.
 *
 * DO NOT WRAP THE CALL TO THIS FUNCTION WITH SG_ERR_IGNORE() (because we
 * need to be able to access the original error/status).
 */
static void _compute_exit_status_and_print_error_message(SG_context * pCtx, SG_getopt * pGetopt, int * pExitStatus)
{
	SG_error err;
	char bufErrorMessage[SG_ERROR_BUFFER_SIZE+1];

	SG_ASSERT(pExitStatus!=NULL);

	SG_context__get_err(pCtx,&err);

	if (err == SG_ERR_OK)
	{
		SG_int_to_string_buffer tmp;
		if(*pExitStatus==0)
			SG_ERR_IGNORE(  SG_log__report_verbose(pCtx, "Command '%s' completed normally."SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR, SG_getopt__get_command_name(pGetopt))  );
		else
			SG_ERR_IGNORE(  SG_log__report_error(pCtx, "Command '%s' completed with exit status %s."SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR, SG_getopt__get_command_name(pGetopt), SG_int64_to_sz(*pExitStatus, tmp))  );
	}
	else if (err == SG_ERR_USAGE)
	{
#if 1
		// TODO 2010/05/13 We need to define a set of exit codes
		// TODO            for all of the commands in vv.
		// TODO            Something like:
		// TODO            0 : ok
		// TODO            1 : fail
		// TODO            2 : usage or other error
		// TODO
		// TODO            Like how /bin/diff returns 0,1,2.
#endif
		// We special case SG_ERR_USAGE because the code that signalled that
		// error has already printed whatever context-specific usage info.

		*pExitStatus = -2;  //Don't return -1, as exec() assumes that -1 means the exe could not be found.
	}
	else
	{
		SG_string * pStrContextDump = NULL;

		SG_error__get_message(err, SG_TRUE, bufErrorMessage, SG_ERROR_BUFFER_SIZE);

		// try to write full stacktrace and everything from the context to the log.
		// if we can't get that, just write the short form.
		//
		// TODO consider having the log_debug level do the full dump and the
		// TODO terser levels just do the summary.

		SG_context__err_to_string(pCtx, SG_TRUE, &pStrContextDump);

		SG_ERR_IGNORE(  SG_log__report_error(pCtx,
			"Command '%s' completed with error:" SG_PLATFORM_NATIVE_EOL_STR "%s"SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR,
									 SG_getopt__get_command_name(pGetopt),
									 (  (pStrContextDump)
										? SG_string__sz(pStrContextDump)
										: bufErrorMessage))  );

#if defined(DEBUG)
		/* Print all the gory details to the console in debug builds. */
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s: %s\n",
								   SG_getopt__get_app_name(pGetopt),
								   (  (pStrContextDump)
										? SG_string__sz(pStrContextDump)
										: bufErrorMessage))  );
#else
		/* Print strictly the error message in release builds. The details are in the log. */
		SG_STRING_NULLFREE(pCtx, pStrContextDump);
		SG_context__err_to_string(pCtx, SG_FALSE, &pStrContextDump);
		if (pStrContextDump)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s\n", SG_string__sz(pStrContextDump))  );
		else
		{
			SG_error__get_message(err, SG_FALSE, bufErrorMessage, SG_ERROR_BUFFER_SIZE);
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s\n", bufErrorMessage)  );
		}
#endif

		SG_STRING_NULLFREE(pCtx, pStrContextDump);

		// the shell and command tools in general only expect ERRNO-type errors.

		if (SG_ERR_TYPE(err) == __SG_ERR__ERRNO__)
			*pExitStatus = SG_ERR_ERRNO_VALUE(err);
		else
			*pExitStatus = 1;  // this means we threw an error, there's no unix error code that will
							 // cover all the possible errors that could come out of vv, so this
							 // one will have to do (EPERM)
	}
}

/**
 * If we could not initialize things fully, we need to stop quickly
 * and WITHOUT trying to LOG anything (because we can't tell where
 * the error was and/or how much of the global-init was completed).
 *
 * So just spew to STDERR and return the exit code.
 *
 * We DO NOT take a pGetopt because the command line hasn't been parsed yet.
 *
 * TODO 2012/10/08 On Windows: consider also printing on the debug console.  
 *
 */
static void _print_error_message_for_global_init_failed(SG_context * pCtx, int * pExitStatus)
{
	SG_error err;
	char bufErrorMessage[SG_ERROR_BUFFER_SIZE+1];

	*pExitStatus = 0;

	SG_context__get_err(pCtx,&err);
	if (err != SG_ERR_OK)
	{
		SG_string * pStrContextDump = NULL;

		SG_error__get_message(err, SG_TRUE, bufErrorMessage, SG_ERROR_BUFFER_SIZE);

#if defined(DEBUG)
		// try to write full stacktrace and everything from the context to the log.
		// if we can't get that, just write the short form.
		SG_context__err_to_string(pCtx, SG_TRUE, &pStrContextDump);
#else
		// In production, we just dump the short form.
		SG_context__err_to_string(pCtx, SG_FALSE, &pStrContextDump);
#endif
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s\n",
								   (  (pStrContextDump)
										? SG_string__sz(pStrContextDump)
										: bufErrorMessage))  );
		SG_STRING_NULLFREE(pCtx, pStrContextDump);

		// the shell and command tools in general only expect ERRNO-type errors.

		if (SG_ERR_TYPE(err) == __SG_ERR__ERRNO__)
			*pExitStatus = SG_ERR_ERRNO_VALUE(err);
		else
			*pExitStatus = 1;  // this means we threw an error, there's no unix error code that will
							 // cover all the possible errors that could come out of vv, so this
							 // one will have to do (EPERM)
	}
}

//////////////////////////////////////////////////////////////////

static int _my_main(SG_context * pCtx, SG_getopt * pGetopt, SG_option_state* pOptSt, SG_bool bUsageError)
{
	int exitStatus = 0;

	SG_pathname*                          pLogPath           = NULL;
	char*                                 szLogPath          = NULL;
	char*                                 szLogLevel         = NULL;
	SG_log_console__data                  cLogStdData;
	SG_log_text__data                     cLogFileData;
	SG_log_text__writer__daily_path__data cLogFileWriterData;
	SG_uint32							  logFileFlags = SG_LOG__FLAG__NONE;
	const char *szCmd = NULL;
	const char *szAppName = NULL;
	const cmdinfo * pCmdInfo = NULL;
	SG_bool bIsServer = SG_FALSE;
	SG_string * pLogFileHeader = NULL;
	char * szSetting = NULL;

	SG_zero(cLogStdData);
	SG_zero(cLogFileData);
	SG_zero(cLogFileWriterData);

	// find the appropriate log path
	SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_PATH, NULL, &szLogPath, NULL);
	SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_UNSUPPORTED_CLOSET_VERSION);

	if (szLogPath != NULL)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pLogPath, szLogPath)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__LOG_DIRECTORY(pCtx, &pLogPath)  );
	}

	// get the configured log level
	SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_LEVEL, NULL, &szLogLevel, NULL);
	SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_UNSUPPORTED_CLOSET_VERSION);

	// register the stdout console logger
	SG_ERR_CHECK(  SG_log_console__set_defaults(pCtx, &cLogStdData)  );
	SG_ERR_CHECK(  SG_log_console__register(pCtx, &cLogStdData, NULL, SG_LOG__FLAG__HANDLER_TYPE__NORMAL)  );

	//Get the canonical command.  We do this to check for vv serve before we configure the logger.
	szCmd = SG_getopt__get_command_name(pGetopt);
	szAppName = SG_getopt__get_app_name(pGetopt);

	_get_canonical_command(szCmd, &pCmdInfo);

	if (pCmdInfo && strcmp(pCmdInfo->name, "serve") == 0)
	{
		bIsServer = SG_TRUE;
	}
	// register the file logger
	SG_ERR_CHECK(  SG_log_text__set_defaults(pCtx, &cLogFileData)  );
	cLogFileData.fWriter             = SG_log_text__writer__daily_path;
	cLogFileData.pWriterData         = &cLogFileWriterData;
	cLogFileData.szRegisterMessage   = NULL;
	cLogFileData.szUnregisterMessage = NULL;
	if (szLogLevel != NULL)
	{
		if (SG_stricmp(szLogLevel, "quiet") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__QUIET;
			cLogFileData.bLogVerboseOperations = SG_FALSE;
			cLogFileData.bLogVerboseValues     = SG_FALSE;
		}
		else if (SG_stricmp(szLogLevel, "normal") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__NORMAL;
			cLogFileData.bLogVerboseOperations = SG_FALSE;
			cLogFileData.bLogVerboseValues     = SG_FALSE;
		}
		else if (SG_stricmp(szLogLevel, "verbose") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;
			cLogFileData.szRegisterMessage   = "---- " WHATSMYNAME " started logging ----";
			cLogFileData.szUnregisterMessage = "---- " WHATSMYNAME " stopped logging ----";
		}
	}
	logFileFlags |= SG_LOG__FLAG__DETAILED_MESSAGES;

	SG_ERR_CHECK(  SG_log_text__writer__daily_path__set_defaults(pCtx, &cLogFileWriterData)  );
	cLogFileWriterData.bReopen          = SG_FALSE;
	cLogFileWriterData.ePermissions     = 0666;
	cLogFileWriterData.pBasePath        = pLogPath;
	if (bIsServer != SG_TRUE)
		cLogFileWriterData.szFilenameFormat = "vv-%d-%02d-%02d.log";
	else
	{
		const char * szVersion = NULL;
		const char * settings[] = {
			SG_LOCALSETTING__SERVER_FILES,
			"server/cache",
			"server/session_storage",
			SG_LOCALSETTING__SERVER_READONLY,
			SG_LOCALSETTING__SERVER_HOSTNAME,
			SG_LOCALSETTING__SERVER_SCHEME,
			SG_LOCALSETTING__SERVER_DEBUG_DELAY,
			SG_LOCALSETTING__SERVER_ENABLE_DIAGNOSTICS,
			SG_LOCALSETTING__SERVER_SSJS_MUTABLE};
		SG_uint32 i;

		cLogFileWriterData.szFilenameFormat = "vv-serve-%d-%02d-%02d.log";

		SG_ERR_CHECK(  SG_lib__version(pCtx, &szVersion)  );
		SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pLogFileHeader,
			"SourceGear Veracity" SG_PLATFORM_NATIVE_EOL_STR
			"Version %s" SG_PLATFORM_NATIVE_EOL_STR,
			szVersion)  );
		for(i=0; i<SG_NrElements(settings); ++i)
		{
			SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, settings[i], NULL, &szSetting, NULL)  );
			if(szSetting!=NULL)
				SG_ERR_CHECK(  SG_string__append__format(pCtx, pLogFileHeader, "%s = \"%s\"" SG_PLATFORM_NATIVE_EOL_STR, settings[i], szSetting)  );
			else if(i<4)
				SG_ERR_CHECK(  SG_string__append__format(pCtx, pLogFileHeader, "%s = <default>" SG_PLATFORM_NATIVE_EOL_STR, settings[i])  );
			SG_NULLFREE(pCtx, szSetting);
		}
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pLogFileHeader, SG_PLATFORM_NATIVE_EOL_STR)  );
		cLogFileWriterData.szHeader = SG_string__sz(pLogFileHeader);
	}

	SG_ERR_CHECK(  SG_log_text__register(pCtx, &cLogFileData, NULL, logFileFlags)  );

	// dispatch the command
	SG_ERR_CHECK(  _dispatch(pCtx, szCmd, szAppName, pCmdInfo, pGetopt, pOptSt, bUsageError, &exitStatus)  );

fail:
	_compute_exit_status_and_print_error_message(pCtx, pGetopt, &exitStatus); // DO NOT WRAP THIS WITH SG_ERR_IGNORE
	SG_ERR_IGNORE(  SG_log_text__unregister(pCtx, &cLogFileData)  );
	SG_ERR_IGNORE(  SG_log_console__unregister(pCtx, &cLogStdData)  );
	SG_NULLFREE(pCtx, szLogLevel);
	SG_NULLFREE(pCtx, szLogPath);
	SG_PATHNAME_NULLFREE(pCtx, pLogPath); // must come after cLogFileData is unregistered, because it uses pLogPath
	SG_STRING_NULLFREE(pCtx, pLogFileHeader);
	SG_NULLFREE(pCtx, szSetting);
	return exitStatus;
}

static void _sg_option_state__free(SG_context* pCtx, SG_option_state* pThis)
{
	if (!pThis)
		return;

	SG_NULLFREE(pCtx, pThis->psz_restore_backup);
	SG_NULLFREE(pCtx, pThis->psz_path);
	SG_NULLFREE(pCtx, pThis->psz_repo);
	SG_NULLFREE(pCtx, pThis->psz_cert);
	SG_NULLFREE(pCtx, pThis->psz_comment);
	SG_NULLFREE(pCtx, pThis->psz_dest_repo);
	SG_NULLFREE(pCtx, pThis->psz_src_repo);
	SG_NULLFREE(pCtx, pThis->psz_server);
	SG_NULLFREE(pCtx, pThis->psz_message);
	SG_NULLFREE(pCtx, pThis->psz_output_file);
	SG_NULLFREE(pCtx, pThis->psz_username);
	SG_NULLFREE(pCtx, pThis->psz_when);
	SG_NULLFREE(pCtx, pThis->psz_from_date);
	SG_NULLFREE(pCtx, pThis->psz_to_date);
	SG_NULLFREE(pCtx, pThis->psz_tool);
	SG_NULLFREE(pCtx, pThis->psz_storage);
	SG_NULLFREE(pCtx, pThis->psz_hash);
	SG_NULLFREE(pCtx, pThis->psz_shared_users);
	SG_NULLFREE(pCtx, pThis->psz_attach);
	SG_NULLFREE(pCtx, pThis->psz_attach_new);
	SG_NULLFREE(pCtx, pThis->psz_label);

	SG_STRINGARRAY_NULLFREE(pCtx, pThis->psa_stamps);

	SG_VHASH_NULLFREE(pCtx, pThis->pvh_dags);
	SG_VHASH_NULLFREE(pCtx, pThis->pvh_area_names);

	SG_NULLFREE(pCtx, pThis->paRecvdOpts);

	SG_STRINGARRAY_NULLFREE(pCtx, pThis->psa_revs);
	SG_STRINGARRAY_NULLFREE(pCtx, pThis->psa_tags);
	SG_STRINGARRAY_NULLFREE(pCtx, pThis->psa_branch_names);

	SG_REV_SPEC_NULLFREE(pCtx, pThis->pRevSpec);

	SG_VARRAY_NULLFREE(pCtx, pThis->pvaSparse);

    SG_STRINGARRAY_NULLFREE(pCtx, pThis->psa_assocs);

	SG_NULLFREE(pCtx, pThis);
}

//////////////////////////////////////////////////////////////////

static SG_context* g_pCtx = NULL;

static void set_sigint_handler(void);
static void sigint_handler(int sig)
{
	/* Handling SIGINT this way has several theoretical flaws, but in practice this works quite
	 * well. And this is *so* much better than not handling it at all. So we're moving ahead. For a
	 * description of the theoretical flaws that may need addressing in the future, see bug K2522. */
	if (SIGINT == sig)
	{
		SG_context* pCtx = g_pCtx;
		set_sigint_handler();
		if (pCtx)
		{
			(void)SG_context__err__SIGINT(pCtx, __FILE__, __LINE__);
		}
	}
}

static void set_sigint_handler()
{
#if defined(WINDOWS)
	(void)signal(SIGINT, sigint_handler);
#else
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = sigint_handler;
	sigaction(SIGINT, &action, NULL);
#endif
}

#if defined(WINDOWS)
int wmain(int argc, wchar_t ** argv)
#else
int main(int argc, char ** argv)
#endif
{
	SG_context * pCtx = NULL;
	SG_getopt * pGetopt = NULL;
	SG_option_state * pOptSt = NULL;
	int exitStatus = 0;
	SG_bool bUsageError = SG_FALSE;

	SG_error err = SG_context__alloc(&pCtx);
	if (SG_IS_ERROR(err))
		return -2;

#if defined(WINDOWS)
	// W5511 Set STDOUT and STDERR to be UTF8 streams.
	// This must be done before we write to them.
	// Then fwprintf_s() (in SG_console()) has a better
	// chance of writing data to the console without
	// getting question mark ('?') substitutions.
	// This is mainly to allow localized error messages
	// to appear properly on non-latin systems.
	//
	// ***THE USER ALSO HAS TO HAVE A UNICODE FONT SELECTED
	// IN THEIR TERMINAL WINDOW.***   (Try "Lucida Console")
	(void)_setmode( _fileno(stdout), _O_U8TEXT );
	(void)_setmode( _fileno(stderr), _O_U8TEXT );
#endif

	g_pCtx = pCtx;
	set_sigint_handler();

	SG_lib__global_initialize(pCtx);
	if (SG_context__has_err(pCtx))
	{
		// We get here when getcwd() returns NULL and $SGCLOSET has
		// a relative path in it, for example.
		_print_error_message_for_global_init_failed(pCtx, &exitStatus);
		SG_ERR_RETHROW;
	}

	_intern_argv(pCtx, argc, argv, &pGetopt);
	if (SG_context__has_err(pCtx))
	{
		// We get here when getcwd() returns NULL and we try to
		// create an absolute path for argv[0] in __set_app_name(),
		// for example.
		_print_error_message_for_global_init_failed(pCtx, &exitStatus);
		SG_ERR_RETHROW;
	}

	_parse_options(pCtx, &pGetopt, &pOptSt);

	if (SG_context__has_err(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_GETOPT_BAD_ARG))
		{
			bUsageError = SG_TRUE;
			SG_context__err_reset(pCtx);
		}
		else
		{
			_compute_exit_status_and_print_error_message(pCtx, pGetopt, &exitStatus); // DO NOT WRAP THIS WITH SG_ERR_IGNORE
			SG_ERR_RETHROW;
		}
	}

	SG_ERR_CHECK(  exitStatus = _my_main(pCtx, pGetopt, pOptSt, bUsageError)  );

fail:
	SG_GETOPT_NULLFREE(pCtx, pGetopt);
	SG_ERR_IGNORE(  _sg_option_state__free(pCtx, pOptSt)  );
	SG_ERR_IGNORE(  SG_lib__global_cleanup(pCtx)  );

	if (SG_context__err_equals(pCtx, SG_ERR_SIGINT))
	{
		SG_CONTEXT_NULLFREE(pCtx);

		/* So that a caller is properly notified we were killed by SIGINT.
		 * See http://www.cons.org/cracauer/sigint.html */
		(void)signal(SIGINT, SIG_DFL);
		(void)raise(SIGINT);
	}

	SG_CONTEXT_NULLFREE(pCtx);
	return exitStatus;
}

