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
 * @file sg_wc_tx__resolve__globals.h
 *
 * @details This contains the various GLOBAL variables originally
 * defined in the PendingTree version of sg_resolve.c.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__GLOBALS__INLINE0_H
#define H_SG_WC_TX__RESOLVE__GLOBALS__INLINE0_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/*
**
** Globals
**
*/

/**
 * Maps state values to info about them.
 *
 * Note: The order of this array must match the order of the SG_resolve__state enumeration.
 */
static const struct
{
	const char* szName;    //< name of the state
	const char* szProblem; //< a short summary of the problem that generated a choice on this state
	const char* szCommand; //< a short summary of what the user must do to resolve a choice on this state
}
gaStateData[] =
{
	{
		"existence",
		"Merge couldn't decide if the item should exist.",
		"You must choose if the item should exist.",
	},
	{
		"name",
		"Merge couldn't choose a name for the item.",
		"You must choose a name for the item.",
	},
	{
		"location",
		"Merge couldn't choose a location for the item.",
		"You must choose a location/parent for the item.",
	},
	{
		"attrs",
		"Merge couldn't choose file system attributes for the item.",
		"You must choose file system attributes for the item.",
	},
	{
		"contents",
		"Merge couldn't generate the item's contents.",
		"You must choose/generate the item's contents.",
	},
	{
		"invalid",
		"This is an invalid type of conflict.",
		"This is an invalid type of conflict.",
	},
};

/**
 * Maps the causes of conflicts/choices to information about them.
 */
static const struct
{
	SG_mrg_cset_entry_conflict_flags eConflictCauses;
	SG_bool                          bCollisionCause;
	SG_bool                          bPortabilityCause;	// I'm not going to bother with _port_flags here.
	const char*                      szName;
	const char*                      szDescription;
}
gaCauseData[] =
{
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE,            SG_FALSE, SG_FALSE, "Delete/Move",   "Item deleted in one branch and moved in another." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_RENAME,          SG_FALSE, SG_FALSE, "Delete/Rename", "Item deleted in one branch and renamed in another." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_ATTRBITS,        SG_FALSE, SG_FALSE, "Delete/Attr",   "Item deleted in one branch and attributes changed in another." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT,    SG_FALSE, SG_FALSE, "Delete/Edit",   "Symlink deleted in one branch and changed in another." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SUBMODULE_EDIT,  SG_FALSE, SG_FALSE, "Delete/Edit",   "Submodule deleted in one branch and changed in another." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT,       SG_FALSE, SG_FALSE, "Delete/Edit",   "File deleted in one branch and changed in another." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE,            SG_FALSE, SG_FALSE, "Move/Move",     "Item moved to different places in different branches." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME,          SG_FALSE, SG_FALSE, "Rename/Rename", "Item renamed to different names in different branches." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_ATTRBITS,        SG_FALSE, SG_FALSE, "Attr/Attr",     "Changes to item's attributes in different branches conflict." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT,    SG_FALSE, SG_FALSE, "Edit/Edit",     "Changes to symlink's target in different branches conflict." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SUBMODULE_EDIT,  SG_FALSE, SG_FALSE, "Edit/Edit",     "Changes to submodule's target in different branches conflict." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK, SG_FALSE, SG_FALSE, "Edit/Edit",     "Changes to item's contents in different branches conflict." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN,      SG_FALSE, SG_FALSE, "Orphaned Item", "Item added to a folder that was deleted in another branch." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE,   SG_FALSE, SG_FALSE, "Path Cycle",    "Item moved to a folder that is its own child in another branch." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO,                      SG_TRUE,  SG_FALSE, "Collision",     "Item's name matches a different item's name in another branch." },
	{ SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO,                      SG_FALSE, SG_TRUE,  "Potential Collision", "Item's name potentially matches a different item's name in another branch." },
};

/**
 * Maps SG_resolve__state values to the conflict flags associated with them.
 */
static const SG_uint32 gaConflictFlagMasks[SG_RESOLVE__STATE__COUNT] =
{
	// SG_RESOLVE__STATE__EXISTENCE
	  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_RENAME
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_ATTRBITS
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SUBMODULE_EDIT
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN,

	// SG_RESOLVE__STATE__NAME
	  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME,

	// SG_RESOLVE__STATE__LOCATION
	  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE,

	// SG_RESOLVE__STATE__ATTRIBUTES
	  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_ATTRBITS,

	// SG_RESOLVE__STATE__CONTENTS
	  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT
	| SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SUBMODULE_EDIT
};

/**
 * Maps SG_resolve__state values to the portability flags associated with them.
 * 
 * Note 2012/08/20 As a first order effort to get RESOLVE to recognize
 * Note            portability problems, I'm going to associate all of
 * Note            portaiblity bits with a NAME choice under the theory
 * Note            that all identified "potential problems" are basically
 * Note            the same as a hard/real name collision or error.
 * 
 * TODO 2012/08/21 Should these take just __MASK_COLLISION ?
 */
static const SG_wc_port_flags gaPortFlagMasks[SG_RESOLVE__STATE__COUNT] =
{
	// SG_RESOLVE__STATE__EXISTENCE
	SG_WC_PORT_FLAGS__ALL,

	// SG_RESOLVE__STATE__NAME
	SG_WC_PORT_FLAGS__ALL,

	// SG_RESOLVE__STATE__LOCATION
	SG_WC_PORT_FLAGS__ALL,

	// SG_RESOLVE__STATE__ATTRIBUTES
	SG_WC_PORT_FLAGS__NONE,

	// SG_RESOLVE__STATE__CONTENTS
	SG_WC_PORT_FLAGS__NONE,
};

/**
 * Maps SG_resolve__state values to the collision flags associated with them.
 */
static const SG_bool gaCollisionMasks[SG_RESOLVE__STATE__COUNT] =
{
	// SG_RESOLVE__STATE__EXISTENCE
	SG_TRUE,

	// SG_RESOLVE__STATE__NAME
	SG_TRUE,

	// SG_RESOLVE__STATE__LOCATION
	SG_TRUE,

	// SG_RESOLVE__STATE__ATTRIBUTES
	SG_FALSE,

	// SG_RESOLVE__STATE__CONTENTS
	SG_FALSE,
};

/**
 * Maps SG_treenode_entry_type values to the conflict flags denoting
 * conflicts that are mergeable for that type.
 */
static const SG_uint32 gaMergeableMasks[SG_RESOLVE__STATE__COUNT] =
{
	// none
	0,

	// SG_TREENODEENTRY_TYPE_REGULAR_FILE
	SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK,

	// SG_TREENODEENTRY_TYPE_DIRECTORY
	0,

	// SG_TREENODEENTRY_TYPE_SYMLINK
	0,

	// SG_TREENODEENTRY_TYPE_SUBMODULE
	0,
};

/**
 * Maps SG_resolve__state values to the values we use to store them on disk.
 *
 * This allows the enumeration values to change without invalidating any data
 * that is stored in wd.json files, as long as this array is updated to match
 * the new values.
 */
static const char* gaSerializedStateValues[SG_RESOLVE__STATE__COUNT] =
{
	"existence",
	"name",
	"location",
	"attributes",
	"contents",
};

/**
 * Map each type of choice into an individual RESOLVED or
 * UNRESOLVED bit in a statusFlags.
 *
 */
struct _state_2_wc_flags
{
	SG_wc_status_flags xr, xu;
};
static const struct _state_2_wc_flags gaState2wcflags[SG_RESOLVE__STATE__COUNT] =
{	{ SG_WC_STATUS_FLAGS__XR__EXISTENCE,   SG_WC_STATUS_FLAGS__XU__EXISTENCE   },
	{ SG_WC_STATUS_FLAGS__XR__NAME,        SG_WC_STATUS_FLAGS__XU__NAME        },
	{ SG_WC_STATUS_FLAGS__XR__LOCATION,    SG_WC_STATUS_FLAGS__XU__LOCATION    },
	{ SG_WC_STATUS_FLAGS__XR__ATTRIBUTES,  SG_WC_STATUS_FLAGS__XU__ATTRIBUTES  },
	{ SG_WC_STATUS_FLAGS__XR__CONTENTS,    SG_WC_STATUS_FLAGS__XU__CONTENTS    }
};

/*
 * Well known value labels.
 *
 * Note: Change these with care, since they're stored on disk in wd.json files.
 *       A new version loading a wd.json with old values will cause it to not
 *       be able to find values.
 */
const char* SG_RESOLVE__LABEL__ANCESTOR  = "ancestor";
const char* SG_RESOLVE__LABEL__BASELINE  = "baseline";
const char* SG_RESOLVE__LABEL__OTHER     = "other";
const char* SG_RESOLVE__LABEL__WORKING   = "working";
const char* SG_RESOLVE__LABEL__AUTOMERGE = "automerge";
const char* SG_RESOLVE__LABEL__MERGE     = "merge";

/**
 * String used as the HID of the working directory / pendingtree.
 */
const char* SG_RESOLVE__HID__WORKING = "";

/**
 * Format string used to create actual labels from base strings and a counter.
 */
static const char* gszIndexedLabelFormat = "%s%d";

/**
 * Format strings for creating temporary filenames for values.
 */
static const char* gszFilenameFormat_Merge   = "%s~M%u~";
static const char* gszFilenameFormat_Working = "%s~W%u~";

/*
 * Keys used in wd.json to store merge information.
 */
static const char* gszIssueKey_Gid                       = "gid";
static const char* gszIssueKey_Type                      = "tne_type";
static const char* gszIssueKey_ConflictFlags             = "conflict_flags";
static const char* gszIssueKey_Collision                 = "collision_flags";
static const char* gszIssueKey_PortabilityFlags          = "portability_flags";
// ? conflict_deletes  -- if we don't need this, merge could skip creating it.
static const char* gszIssueKey_Cycle_Hint                = "conflict_path_cycle_hint";
static const char* gszIssueKey_Cycle_Items               = "conflict_path_cycle_others";
// ? conflict_divergent_moves ?
// ? conflict_divergent_renames ?
// ? conflict_divergent_attrbits ?
// ? conflict_divergent_hid_symlink ?
// ? conflict_divergent_hid_submodule ?
static const char* gszIssueKey_TempPath                  = "repopath_tempdir";
static const char* gszIssueKey_File_AutomergeTool        = "automerge_mergetool";
// ? automerge_generated_hid ?
// ? automerge_disposable_hid ?
static const char* gszIssueKey_Collision_Items           = "collisions";
// portability_log
static const char* gszIssueKey_PortabilityCollision_Items = "portability_collisions";

static const char* gszIssueKey_Inputs                    = "input";
static const char* gszIssueKey_Inputs_Name               = "entryname";
static const char* gszIssueKey_Inputs_Location           = "gid_parent";
static const char* gszIssueKey_Inputs_Attributes         = "attrbits";
static const char* gszIssueKey_Inputs_Contents           = "hid";
// static const char* gszIssueKey_Inputs_AcceptLabel     = "accept_label";
// hid_cset
static const char* gszIssueKey_Inputs_TempRepoPath       = "tempfile";

static const char* gszIssueKey_Output                    = "output";
static const char* gszIssueKey_Output_Name               = "entryname";
//static const char* gszIssueKey_Output_Attributes         = "attrbits";
//static const char* gszIssueKey_Output_Contents           = "hid";
static const char* gszIssueKey_Output_TempRepoPath       = "tempfile";

/*
 * Keys used in wd.json to store resolve information.
 */
static const char* gszChoiceKey_Result    = "result";
static const char* gszChoiceKey_Values    = "values";
static const char* gszValueKey_Label      = "label";
static const char* gszValueKey_Value      = "value";
static const char* gszValueKey_ValueFile  = "valuefile";
static const char* gszValueKey_Mnemonic   = "mnemonic";
//static const char* gszValueKey_Changeset  = "changeset";
static const char* gszValueKey_Baseline   = "baseline";
static const char* gszValueKey_Other      = "other";
static const char* gszValueKey_Tool       = "tool";
static const char* gszValueKey_ToolResult = "tool_result";


// TODO 2012/05/23 rename the variables to be more abstract
// TODO            having ancestor/baseline/other works for
// TODO            a regular MERGE, but is odd for UPDATE.
// TODO            See 
static const char* gszMnemonic_Ancestor = SG_MRG_MNEMONIC__A;
static const char* gszMnemonic_Baseline = SG_MRG_MNEMONIC__B;
static const char* gszMnemonic_Other    = SG_MRG_MNEMONIC__C;
static const char* gszMnemonic_Working  = "";


//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__GLOBALS__INLINE0_H
