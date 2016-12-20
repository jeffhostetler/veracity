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
 * @file sg_typedefs.h
 */

//////////////////////////////////////////////////////////////////

#include <sg_getopt.h>
#include <sg_revision_specifier_typedefs.h>
#include <sg_wc__public_typedefs.h>

#ifndef H_SG_TYPEDEFS_H
#define H_SG_TYPEDEFS_H

#define	WHATSMYNAME	"vv"

#define HAVE_SUBREPOS

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

// enum for options that only have a "long" form
// options with a short form will be identified by the single char
typedef enum
{
	opt_cert = SG_GETOPT__FIRST_LONG_OPT,
	opt_area,
	opt_confirm,
	opt_dest,
	opt_server,
	opt_verbose,
	opt_status,
	opt_pull,
	opt_restore_backup,
	opt_from,
	opt_when,
	opt_global,
	opt_hide_merges,
	opt_leaves,
	opt_length,
	opt_list,
	opt_list_all,
	opt_max,
	opt_no_auto_merge,
	opt_no_ff_merge,
	opt_no_ignores,
	opt_no_plain,
	opt_no_prompt,
	opt_port,
	opt_remember,
	opt_remove,
	opt_repo,
	opt_reverse,
	opt_show_all,
	opt_sport,
	opt_src,
	opt_src_comment,
	opt_stamp,
	opt_startline,
	opt_tag,
	opt_to,
	opt_tool,
	opt_trace,
	opt_all_full,
	opt_all_zlib,
	opt_all,
	opt_all_but,
	opt_create,
	opt_subgroups,
	opt_pack,
	opt_path,
	opt_detached,
	opt_attach,
	opt_attach_new,
	opt_storage,
	opt_hash,
    opt_shared_users,
	opt_no_backups,
	opt_label,
	opt_overwrite,
	opt_quiet,
	opt_sparse,
	opt_list_unchanged,
	opt_allow_lost,
	opt_no_allow_after_the_fact,
	opt_no_fallback,
	opt_allow_dirty,

#if defined(DEBUG)
	opt_dagnum,			// debug
#endif

	opt_revert_files,
	opt_revert_symlinks,
	opt_revert_directories,
	opt_revert_lost,
	opt_revert_added,
	opt_revert_removed,
	opt_revert_renamed,
	opt_revert_moved,
	opt_revert_merge_added,
	opt_revert_update_added,
	opt_revert_attributes,
	opt_revert_content,

	OPT_REV_SPEC, // Special case revision specifier option. Can be satisfied by -r/--rev, -t/--tag, or -b/--branch
				  // Do not put this option on a command that also has a rev, tag, or branch option.

	OPT_MAX
} sg_cl__long_option;

#define OPT_REV_SPEC_DESCRIPTION "Revision (-r/--rev), Tag (--tag), or Branch (-b/--branch)"

//////////////////////////////////////////////////////////////////////////

struct _sg_option_state
{
	SG_int32* paRecvdOpts;
	SG_uint32 count_options;

	SG_uint32 optCountArray[OPT_MAX]; // Array in which we track the number of times each option is specified.

	SG_int32 iPort;
	SG_int32 iSPort;
	SG_int32 iMaxHistoryResults;
	SG_int32 iStartLine;
	SG_int32 iLength;

	char* psz_cert;
	char* psz_comment;
	SG_vhash* pvh_dags;
	SG_vhash* pvh_area_names;
	char* psz_dest_repo;
	char* psz_message;
	char* psz_output_file; // -o/--output
	char* psz_src_repo;
	char* psz_server;
	char* psz_repo;
	char* psz_restore_backup;
	char* psz_attach;
	char* psz_attach_new;
	char* psz_username;
	char* psz_when;
	char* psz_from_date;
	char* psz_to_date;
	SG_stringarray* psa_stamps;
	char* psz_tool;
	char* psz_storage;
	char* psz_hash;
	char* psz_shared_users;
	char* psz_label;
	char* psz_path;

	SG_stringarray* psa_branch_names;
	SG_stringarray* psa_revs;
	SG_stringarray* psa_tags;

	SG_rev_spec* pRevSpec; // Arguments that satisfy OPT_REV_SPEC (e.g. rev/tag/branch) are stored here.

	SG_varray * pvaSparse;

	SG_bool bAll;
	SG_bool bAllBut;
	SG_bool b_all_full;
	SG_bool b_all_zlib;
	SG_bool bClean;
	SG_bool bConfirm;
	SG_bool bForce;
	SG_bool bGlobal;
	SG_bool bList;
	SG_bool bListAll;
	SG_bool bListUnchanged;
	SG_bool bNoAutoMerge;
	SG_bool bNoPlain;
	SG_bool bNoPrompt;
	SG_bool bPromptForDescription;
	SG_bool bPublic;
	SG_bool bRecursive;
	SG_bool bRemember;
	SG_bool bRemove;
	SG_bool bCreate;
    SG_bool bSubgroups;
	SG_bool bShowAll;
	SG_bool bPack;
	SG_bool bTest;
	SG_bool bTrace;
	SG_bool bUpdate;
	SG_bool bVerbose;
	SG_bool bQuiet;
	SG_bool bStatus;
	SG_bool bPull;
	SG_bool bLeaves;
	SG_bool bHideMerges;
	SG_bool bReverseOutput;
    SG_bool bNew;
    SG_bool bAllowDetached;
	SG_bool bAttachCurrent;	// use currently attached branch or detached state when updating away from a head. L2121.
	SG_bool bNoBackups;
	SG_bool bOverwrite;
	SG_bool bSrcComment;
	SG_bool bNoFFMerge;
	SG_bool bNoIgnores;		// TODO 2011/10/06 Pulling this field from file_spec.
	SG_bool bKeep;
	SG_bool bAllowLost;
	SG_bool bNoAllowAfterTheFact;
	SG_bool bNoFallback;
	SG_bool bInteractive;
	SG_bool bAllowWhenDirty;
	SG_wc_status_flags RevertItems_flagMask_type;
	SG_wc_status_flags RevertItems_flagMask_dirt;
	
#if defined(DEBUG)
	SG_bool bAllowDebugRemoteShutdown;

	SG_bool bGaveDagNum;
	SG_uint64 ui64DagNum;
#endif

	SG_stringarray* psa_assocs;
};

typedef struct _sg_option_state SG_option_state;

//////////////////////////////////////////////////////////////////

#define CMD_FUNC_PARAMETERS SG_context * pCtx,\
	const char* pszAppName,\
	const char* pszCommandName,\
	SG_getopt* pGetopt,\
	SG_option_state* pOptSt,\
	SG_bool *pbUsageError,\
	int * pExitStatus

typedef void cmdfunc(CMD_FUNC_PARAMETERS);

#define DECLARE_CMD_FUNC(name) void cmd_##name(CMD_FUNC_PARAMETERS)

//////////////////////////////////////////////////////////////////

/**
 * An arbitrary value.  No individual command should define more arg flags than this.
 * This only covers how many unique flags can be defined for a command; it does not
 * refer to any limits on the number of flags that may be typed on a command line
 * (because some flags (like --include) can be repeated without limit).
 */
#define SG_CMDINFO_OPTION_LIMIT		50

struct cmdopt
{
	SG_int32 option;
	SG_uint32 min; // The minimum number of times the option can be specified or 0 to disable checking.
	SG_uint32 max; // The maximum number of times the option can be specified or 0 to disable checking.
};
typedef struct cmdopt cmdopt;

struct cmdinfo
{
	const char* name;
	const char* alias1;
	const char* alias2;
	const char* alias3;
	cmdfunc* fn;
	const char* desc;
	const char *extraHelp; /**< additional text to display after "usage: vv cmdname " */
	const char **extendedHelp; // NULL if unneeded, built via markdown source

	SG_bool advanced; /* command shows up only in --all command lists */

	/* TODO maybe this is the place to put full usage info? */

	cmdopt valid_options[SG_CMDINFO_OPTION_LIMIT];

	/** A list of option help descriptions, keyed by the option unique enum
	* (the 2nd field in sg_getopt_option), which override the generic
	* descriptions given in an sg_getopt_option on a per-command basis.
	*/
	struct { int optch; const char *desc; } desc_overrides[SG_CMDINFO_OPTION_LIMIT];
};

typedef struct cmdinfo cmdinfo;

struct subcmdinfo
{
	const char *cmdname;  /**< must match cmdinfo.name */
	const char *name; /**< subcmd name */
	const char *desc; /**< also used in main command help */
	const char *extraHelp; /**< used in vv help cmd subcmd */
	const char **extendedHelp; /**< ditto; optional, generated from markdown */
	cmdopt valid_options[SG_CMDINFO_OPTION_LIMIT];
	/** A list of option help descriptions, keyed by the option unique enum
	* (the 2nd field in sg_getopt_option), which override the generic
	* descriptions given in an sg_getopt_option and any given in cmdinfo.valid_options
	*/
	struct { int optch; const char *desc; } desc_overrides[SG_CMDINFO_OPTION_LIMIT];
};
typedef struct subcmdinfo subcmdinfo;

struct cmdhelpinfo
{
	const char *name;
	const char *desc;
	const char **helptext;
};

typedef struct cmdhelpinfo cmdhelpinfo;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_TYPEDEFS_H
