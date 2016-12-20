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

#ifndef VV_VERBS_H
#define VV_VERBS_H

#include "precompiled.h"
#include "vvFlags.h"
#include "vvHistoryObjects.h"
#include "vvNullable.h"
#include "vvRepoLocator.h"

/*
 * This is a wrapper around the actual sglib verb operations used by vvDialog.
 * It is higher level and takes care of a lot of sglib details so dialogs don't have to.
 * It also hides sglib-specific types and translates them to wxWidgets and Windows equivalents.
 * All functions that return a bool are indicating success/failure.
 *
 * This is a base class interface, which allows for several implementations.
 * This is helpful so that a debug or mock implementation can be easily swapped in for debugging/testing purposes.
 */
class vvVerbs
{
// types
public:
	/**
	 * Data about a user in a repository.
	 */
	struct UserData
	{
		wxString sGid;    //< Unique ID of the user.
		wxString sName;   //< Name of the user.
		wxString sPrefix; //< Prefix used to help generate unique IDs from this user.
		bool     bActive; //< Whether or not the user is marked as active.
		bool     bNobody; //< True if this record is the "nobody" user.
	};

	/**
	 * A list of user data.
	 */
	typedef std::list<UserData> stlUserDataList;

	/**
	 * Data about a stamp in a repository.
	 */
	struct StampData
	{
		wxString     sName;  //< Name of the stamp.
		unsigned int uCount; //< Number of times the stamp is used in the repo.
	};

	/**
	 * A list of stamp data.
	 */
	typedef std::list<StampData> stlStampDataList;

	
	/**
	 * Data about a tag in a repository.
	 */
	struct TagData
	{
		wxString     sName;  //< Name of the tag.
		wxString     sCSID;  //< revision that the tag is applied to.
	};

	/**
	 * A list of stamp data.
	 */
	typedef std::list<TagData> stlTagDataList;

	/**
	 * List of possible checkout types.
	 */
	enum CheckoutType
	{
		CHECKOUT_HEAD,      //< Checkout the head revision.
		CHECKOUT_BRANCH,    //< Checkout the head revision in a branch.
		CHECKOUT_CHANGESET, //< Checkout an explicit changeset ID.
		CHECKOUT_TAG,       //< Checkout by tag name.
		CHECKOUT_COUNT,     //< Number of values, for iteration.
	};

	struct LockEntry
	{
		wxString sUsername;           
		wxString sRepoPath;
		wxString sGID;
		bool bViolated;
		wxString sStartHid;
		wxString sEndCSID;
	};
	
	/**
	 * List of item diffs returned by Diff/Status.
	 */

	typedef std::vector<LockEntry> LockList;

	/**
	 * Data about changes made to a single version-controlled item.
	 */
	struct DiffEntry
	{
		/**
		 * List of possible types of items in a diff.
		 *
		 * Roughly corresponds to SG_treenode_entry_type in
		 * sg_treenode_entry_typedefs.h, but is not necessarily intended to mirror
		 * them.  They are intended to provide a potentially simpler UI-centric
		 * view of the sglib internal data.
		 */
		enum ItemType
		{
			ITEM_FILE,      //< Diff entry is for a file.
			ITEM_FOLDER,    //< Diff entry is for a folder.
			ITEM_SYMLINK,   //< Diff entry is for a symbolic link.
			ITEM_SUBMODULE, //< Diff entry is for a submodule.
			ITEM_COUNT,     //< Number of values, for iteration.
		};

		/**
		 * List of possible changes to an item in a diff.
		 *
		 * Roughly corresponds to SG_DIFFSTATUS_FLAGS__* in sg_treediff2_typedefs.h,
		 * but it is not necessarily intended to mirror them.  These flags are
		 * intended to provide a potentially simpler UI-centric view of the sglib
		 * internal data.
		 */
		enum ChangeFlags
		{
			CHANGE_NONE              = 0,
			CHANGE_ADDED             = 1 <<  0, //< Item was added to source control.
			CHANGE_DELETED           = 1 <<  1, //< Item was removed from source control and disk.
			CHANGE_MODIFIED          = 1 <<  2, //< Controlled item was changed.
			CHANGE_LOST              = 1 <<  3, //< Item is controlled but not on disk.
			CHANGE_FOUND             = 1 <<  4, //< Item is on disk but uncontrolled.
			CHANGE_RENAMED           = 1 <<  5, //< Controlled item was renamed within a directory.
			CHANGE_MOVED             = 1 <<  6, //< Controlled item was moved to a new directory.
			CHANGE_ATTRS             = 1 <<  7, //< Controlled item's basic attributes were changed.
			CHANGE_LOCKED            = 1 <<  8, //< Controlled item is locked by the user.
			CHANGE_LOCK_CONFLICT	 = 1 <<  9, //< Controlled item is locked by another user, but there is a modification in the working folder.
			CHANGE_LAST              = 1 <<  10, //< Last item in the enumeration, for iteration purposes.

			// bits in this mask are mutually exclusive
			CHANGE_MASK_EXCLUSIVE    = CHANGE_ADDED
			                         | CHANGE_DELETED
			                         | CHANGE_MODIFIED
			                         | CHANGE_LOST
			                         | CHANGE_FOUND,

			// bits in this mask may be combined with each other and other flags
			CHANGE_MASK_MODIFIER     = CHANGE_RENAMED
			                         | CHANGE_MOVED
			                         | CHANGE_ATTRS
									 | CHANGE_LOCKED
									 | CHANGE_LOCK_CONFLICT,
		};

		// data
		ItemType eItemType;           //< Type of item the entry is about.
		wxString sItemGid;            //< GID of the item that's different.
		wxString sItemPath;           //< Repo path of the item that's different.
		vvFlags  cItemChanges;        //< ChangeFlags describing what about the item changed.
		bool     bItemIgnored;        //< Whether or not the item is ignored by the include/exclude/ignore settings.
		wxString sContentHid;         //< HID of the item's content.
		wxString sOldContentHid;      //< HID of the item's old content.
		wxString sMoved_From;         //< Old path, for MOVED diffs.
		wxString sRenamed_From;       //< Old filename, for RENAMED diffs.
		wxString sSubmodule_Current;  //< Current version token, for SUBMODULE items.
		wxString sSubmodule_Original; //< Original/baseline version token, for SUBMODULE items.
		bool     bSubmodule_Dirty;    //< Whether or not a submodule's folder is dirty, for SUBMODULE items.
		LockList cLocks;	  //< The list of locks that apply to this file.
	};

	/**
	 * List of item diffs returned by Diff/Status.
	 */
	typedef std::list<DiffEntry> DiffData;

	/**
	 * List of flags used with the Commit function.
	 */
	enum CommitFlag
	{
		COMMIT_NO_SUBMODULES, //< Ignore submodules when committing.
		COMMIT_NO_RECURSE,    //< Old --non-recursive flag, I don't think this is used anymore (replaced by SG_file_spec).
		COMMIT_DETACHED,	  //< This is a detached commit.
	};

	enum RepoItemType
	{
		INVALID=-1,					/**< Invalid Value, must be -1. */

		REGULAR_FILE=1,				/**< A regular file. */
		DIRECTORY=2,					/**< A sub-directory. */
		SYMLINK=3,					/**< A symlink. */

		SUBMODULE=4					/**< A submodule directory */
	};
	
	/**
	 * Data about a single item inside the repo (like "@/blah/blah/blah.txt")
	 */
	struct RepoItem
	{
		RepoItemType eItemType;
		wxString sItemName;
	};

	/**
	 * List of conflicts returned by Get_Children.
	 */
	typedef std::vector<RepoItem> RepoItemList;

	/**
	 * Data about a single parent of a changeset
	 */
	struct RevisionEntry
	{
		unsigned int nRevno;            
		wxString sChangesetHID;           
	};

	typedef std::vector<RevisionEntry> RevisionList;

	/**
	 * Data about a single audit on a changeset or comment
	 */
	struct AuditEntry
	{
		wxString sWho;            
		wxDateTime nWhen;           
	};

	/**
	 * Data about a single comment on a changeset
	 */
	struct CommentEntry
	{
		std::vector<AuditEntry> cAudits;
		wxString sCommentText;           
	};
	
	/**
	 * Data about a single changeset
	 */
	struct HistoryEntry
	{
		// data
		unsigned int nRevno;            //< GID of the item that's different.
		wxString sChangesetHID;           //< Repo path of the item that's different.
		RevisionList cParents;
		wxArrayString cTags;
		wxArrayString cStamps;
		wxArrayString cBranches;
		bool bIsMissing;
		std::vector<AuditEntry> cAudits;
		std::vector<CommentEntry> cComments;
	};
	
	/**
	 * List of item diffs returned by Diff/Status.
	 */
	typedef std::vector<HistoryEntry> HistoryData;

	class BranchEntry
	{
	public:
		// data
		wxString sBranchName;           
		wxString sBranchNameForDisplay;           
		bool bClosed;
		bool operator<(BranchEntry other) const
		{
			return sBranchNameForDisplay.CompareTo(other.sBranchNameForDisplay, wxString::ignoreCase) < 0;
		}
	};
	
	/**
	 * List of item diffs returned by Diff/Status.
	 */
	typedef std::vector<BranchEntry> BranchList;
	
	/**
	 * Set of available merge tool contexts.
	 * Matches SG_MERGETOOL__CONTEXT constants.
	 */
	enum MergeToolContext
	{
		MERGETOOL_CONTEXT_NONE,    //< Context not known or unneeded.
		MERGETOOL_CONTEXT_MERGE,   //< Tool used during an automated merge.
		MERGETOOL_CONTEXT_RESOLVE, //< Tool used during an interactive resolve.
		MERGETOOL_CONTEXT_COUNT    //< Last item in the enumeration, for iteration.
	};

	/**
	 * Flags that can be associated with a ResolveValue.
	 * Matches SG_resolve__value__flags.
	 */
	enum ResolveValueFlags
	{
		RESOLVE_VALUE_CHANGESET    = 1 << 0, //< Value is associated with a changeset.
		RESOLVE_VALUE_MERGE        = 1 << 1, //< Value was created by merging two other values.
		RESOLVE_VALUE_AUTOMATIC    = 1 << 2, //< Merged value was created automatically by "merge" command.
		RESOLVE_VALUE_RESULT       = 1 << 3, //< Value is the currently accepted result for its choice.
		RESOLVE_VALUE_OVERWRITABLE = 1 << 4, //< Value can be overwritten with a new merge result.
		RESOLVE_VALUE_LEAF         = 1 << 5, //< Value is a leaf that should be merged to obtain a final result.
		RESOLVE_VALUE_LIVE         = 1 << 6, //< Value is currently associated with the live working copy.
		RESOLVE_VALUE_LAST         = 1 << 7, //< Last value in the enumeration, for iteration.
	};

	/**
	 * One possible value on a ResolveChoice.
	 * Matches SG_resolve__value.
	 */
	struct ResolveValue
	{
		wxString   sLabel;      //< Value's unique and user-friendly label.
		wxAny      cData;       //< Value's data, might be bool, int64, or string depending on choice type.
		wxString   sSummary;    //< Output-friendly string summary of cData.
		vvFlags    cFlags;      //< Combination of relevent ResolveValueFlags.
		wxUint64   uSize;       //< The value's size.  Meaning depends on the choice type.
		wxString   sChangeset;  //< HID of the changeset associated with the value.
		                        //< Empty if the value isn't associated with a changeset.
		                        //< Values with associated changesets have the CHANGESET flag.
		wxString   sMWParent;   //< Label of the value that is this working value's parent.
		                        //< Only meaningful on working values of mergeable (CONTENTS) choices.
		                        //< Working values have the CHANGESET flag and an empty sChangeset.
		                        //< "MW" stands for "Mergeable Working".
		bool       bMWModified; //< Whether or not this working value is modified relative to its parent.
		                        //< Only meaningful if sMWParent is meaningful.
		wxString   sBaseline;   //< Label of the first value that was merged to create this one.
		                        //< Empty if the value isn't the result of a merge.
		                        //< Values created via merge have the MERGE flag.
		wxString   sOther;      //< Label of the second value that was merged to create this one.
		                        //< Empty if the value isn't the result of a merge.
		                        //< Values created via merge have the MERGE flag.
		wxString   sTool;       //< The merge tool used to create this value.
		                        //< Empty if the value isn't the result of a merge.
		                        //< Values created via merge have the MERGE flag.
		int        iExit;       //< The exit code from sTool when this value was generated.
		                        //< Zero if the value isn't the result of a merge.
		                        //< Values created via merge have the MERGE flag.
		wxDateTime iTime;       //< Time date/time associated with the value.
		                        //< Zero if the value isn't the result of a merge.
	};

	/**
	 * Map of value labels to a data structure describing them.
	 */
	typedef std::map<wxString, ResolveValue> stlResolveValueMap;

	/*
	 * Well-known base value labels.
	 * All except the first two may also use a numeric index suffix.
	 */
	static const char* sszResolveLabel_Ancestor;
	static const char* sszResolveLabel_Baseline;
	static const char* sszResolveLabel_Other;
	static const char* sszResolveLabel_Working;
	static const char* sszResolveLabel_Automerge;
	static const char* sszResolveLabel_Merge;

	/**
	 * The possible item states that a choice might be about.
	 * Matches SG_resolve__state.
	 */
	enum ResolveChoiceState
	{
		RESOLVE_STATE_EXISTENCE,   //< The choice concerns whether or not the item should exist.
		RESOLVE_STATE_NAME,        //< The choice is between possible filenames for the item.
		RESOLVE_STATE_LOCATION,    //< The choice is between possible parent folders for the item.
		RESOLVE_STATE_ATTRIBUTES,  //< The choice is between possible chmod bits for the item.
		RESOLVE_STATE_CONTENTS,    //< The choice is between possible contents for the item.
		RESOLVE_STATE_COUNT,       //< Last value in the enumeration, for iteration.
	};
	enum BranchAttachmentAction
	{
		BRANCH_ACTION__NO_CHANGE,   //< Do not change the branch attachment
		BRANCH_ACTION__ATTACH,      //< Attach to a new branch
		BRANCH_ACTION__DETACH,		//< Detach from the branch
		BRANCH_ACTION__CREATE,		//< Create a new branch and attach to it.
		BRANCH_ACTION__REOPEN,		//< Reopen a branch that is closed.
		BRANCH_ACTION__CLOSE,		//< Close a branch.  The control will need to do this in order to restore a reopened branch to its closed state..
		BRANCH_ACTION__ERROR		//< The branch attachment control is in an error state.
	};

	/**
	 * A map of choice cause names to descriptions about them.
	 */
	typedef std::map<wxString, wxString> stlResolveCauseMap;

	/**
	 * Ways in which other items can be related to a conflict.
	 */
	enum ResolveRelation
	{
		RESOLVE_RELATION_COLLISION, //< The other item is colliding with the current one.
		RESOLVE_RELATION_CYCLE,     //< The other item is in a path cycle with the current one.
		RESOLVE_RELATION_COUNT      //< Last value in the enumeration, for iteration.
	};

	/**
	 * Details about an item that is related to a conflict.
	 */
	struct ResolveRelatedItem
	{
		ResolveRelation     eRelation;          //< Way in which the item is related.
		wxString            sGid;               //< GID of the related item.
		DiffEntry::ItemType eType;              //< Type of the related item.
		vvFlags             cStatus;            //< Current status of the related item.
		wxString            sRepoPath_Baseline; //< Repo path of the related item in the baseline parent.
		wxString            sRepoPath_Working;  //< Current repo path the related item.
	};

	/**
	 * A map of item GIDs to details about them and their relation to a conflict.
	 */
	typedef std::map<wxString, ResolveRelatedItem> stlResolveRelatedMap;

	/**
	 * One choice/conflict that must be made/resolved on an item.
	 * Matches SG_resolve__choice.
	 */
	struct ResolveChoice
	{
		ResolveChoiceState      eState;       //< The item state that the choice is about.
		wxString                sName;        //< A user/display friendly name for the choice's state.
		wxString                sProblem;     //< A brief summary of the problem that led to the choice.
		wxString                sCommand;     //< A brief summary of what the user must do to resolve the choice.
		stlResolveCauseMap      cCauses;      //< List of reasons the choice is necessary.
		                                      //< Maps cause names to descriptions, both user/output friendly.
		bool                    bMergeable;   //< Whether or not the choice's values can be merged.
		bool                    bResolved;    //< Whether or not the choice has been resolved.
		bool                    bDeleted;     //< If bResolved is true, this indicates whether or not the item was
		                                      //< resolved by deleting it (as opposed to accepting a value).
		wxString                sResolution;  //< Label of the value that is the choice's resolution.
		                                      //< Empty if the choice doesn't have an accepted value.
		                                      //< Might be empty even if bResolved is true (ex: the item is deleted).
		stlResolveValueMap      cValues;      //< List of values available to resolve the choice.
		                                      //< Maps value labels to data about the value.
		stlResolveRelatedMap    cColliding;   //< Information about other items involved in the same collision.
		                                      //< Empty for choices not caused by a collision.
		stlResolveRelatedMap    cCycling;     //< Information about other items involved in the same path cycle.
		                                      //< Empty for choices not caused by a path cycle.
		wxString                sCycle;       //< The portion of a path causing an infinite path cycle.
		                                      //< Empty for choices not caused by a path cycle.
	};

	/**
	 * A map of choice state to a data structure describing a choice on that state.
	 */
	typedef std::map<ResolveChoiceState, ResolveChoice> stlResolveChoiceMap;

	/**
	 * A single version-controlled item that has one or more conflicts/choices on it.
	 * Matches SG_resolve__item.
	 */
	struct ResolveItem
	{
		wxString               sGid;               //< GID of the item.
		DiffEntry::ItemType    eType;              //< Type of the item.
		vvFlags                cStatus;            //< Current status of the item.
		                                           //< Composed of DiffEntry::ChangeFlags.
		//wxString               sAncestor;          //< HID of the common ancestor changeset of conflicts on this item.
		wxString               sRepoPath_Ancestor; //< Repo path the item had in the common ancestor changeset.
		wxArrayString          sRepoPath_Parents;  //< Repo path the item had in the working copy's parent changesets.
		                                           //< Should be in the same order as would be returned by "parents".
		                                           //< An empty string indicates the item not existing in that parent.
		wxString               sRepoPath_Working;  //< Repo path the item has in the working copy.
		bool                   bResolved;          //< Whether or not all choices/conflicts on the item are resolved.
		bool                   bDeleted;           //< If bResolved is true, this indicates whether or not the item was
		                                           //< resolved by deleting it (as opposed to accepting values on its choices).
		stlResolveChoiceMap    cChoices;           //< List of choices/conflicts on the item, resolved or unresolved.

		/**
		 * Gets the best available repo path from the item.
		 * This is usually the item's current path in the working copy.
		 * However, in cases where the item has been deleted, it must instead
		 * be the item's repo path from one of the other involved changesets.
		 */
		wxString& FindBestRepoPath(
			bool* pNonExistent = NULL //< [out] True if the item doesn't exist in the working copy.
			                          //<       and therefore the returned path is from another changeset.
			);
		const wxString& FindBestRepoPath(
			bool* pNonExistent = NULL
			) const;
	};

	/**
	 * A list of all items with conflicts/choices.
	 */
	typedef std::list<ResolveItem> stlResolveItemList;

	/*
	* A wrapper around resolve item lists that also
	* include the base of the working folder.
	*/
	struct  WcResolveData
	{
		wxString sWorkingCopyRoot;
		stlResolveItemList cResolveItems;
	};

	struct  WcResolveChoiceData
	{
		wxString sWorkingCopyRoot;
		stlResolveChoiceMap cChoices;
	};

	struct  WcResolveRelatedData
	{
		wxString sWorkingCopyRoot;
		stlResolveRelatedMap cRelated;
	};
	/**
	 * Struct to hold defaults for the Serve Dialog
	 */
	struct ServeDefaults
	{
		bool					bPublic;			//The server should be public (accept all connections)
		int						nPortNumber;		//The port the server should listen
		bool					bServeAllRepos;     //Expose all repos
	};

	/**
	 * Different scopes that a single config setting can be stored at.
	 * Sorted according to the order scopes are checked for a setting.
	 * These are bit-flags to make it easy to store several scopes in one variable,
	 * usually to pass/return a set of scopes to/from a function.
	 */
	enum ConfigScope
	{
		CONFIG_SCOPE_INSTANCE = 1 << 0, //< Values apply only to a single repository instance/descriptor..
		CONFIG_SCOPE_REPO     = 1 << 1, //< Values apply to a single repository ID.
		CONFIG_SCOPE_ADMIN    = 1 << 2, //< Values apply to a single admin ID (group of repositories).
		CONFIG_SCOPE_MACHINE  = 1 << 3, //< Values apply to the current machine.
		CONFIG_SCOPE_DEFAULT  = 1 << 4, //< Values apply to anything and are built-in and read-only.
		CONFIG_SCOPE_LAST     = 1 << 5, //< Last value in the enum, for iteration.

		// a mask containing all scopes that require a repo to use
		CONFIG_SCOPE_REPO_REQUIRED = CONFIG_SCOPE_INSTANCE
		                           | CONFIG_SCOPE_REPO
		                           | CONFIG_SCOPE_ADMIN,

		// a mask containing all the defined scopes
		CONFIG_SCOPE_ALL      = CONFIG_SCOPE_LAST - 1
	};

	class WorkItemEntry
	{
	public:
		// data
		wxString sID;           
		wxString sRecID;
		wxString sStatus;
		int nStatus;
		wxString sTitle;
	};
	typedef std::vector<WorkItemEntry> WorkItemList;

// constants
public:
	/**
	 * The name of the directory that symbolizes a repository root: @
	 * Does not include a trailing slash.
	 */
	static const char* sszRepoRoot;

	/**
	 * The username of the "nobody" user.
	 */
	static const char* sszNobodyUsername;

	/**
	 * The name of the "main" branch that always exists.
	 */
	static const char* sszMainBranch;

// functionality
public:
	/**
	 * Initializes sglib globally.
	 * Most other functions will assert until this one is called successfully.
	 */
	bool Initialize(
		class vvContext& cContext //< [in] [out] Error and context info.
		);

	/**
	 * Shuts down sglib globally.
	 * Most other functions will assert after this one is called.
	 */
	void Shutdown(
		class vvContext& cContext //< [in] [out] Error and context info.
		);

	wxString GetLogFilePath(
		class vvContext&     pCtx,   //< [in] [out] Error and context info.
		const char * szFilenameFormat //< [in] The filename format for the log file.  For example, "vvDialog-%d-%02d-%02d.log"
		);

	/**
	 * Gets a list of repos on the local machine.
	 */
	bool GetLocalRepos(
		class vvContext& cContext,   //< [in] [out] Error and context info.
		wxArrayString&   cLocalRepos //< [out] Array to add local repos to.
		);

	/**
	 * Checks to see if a local repo with the given name exists.
	 * Returns true if the repo exists, false otherwise.
	 */
	bool LocalRepoExists(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sName     //< [in] The local repo name to check for.
		);

	/**
	 * Gets a list of remote repos that are known by the local machine.
	 */
	bool GetRemoteRepos(
		class vvContext& cContext,    //< [in] [out] Error and context info.
		const wxString& sRepoDescriptorName,//< [in] The repo descriptor name.  Leave this null or empty to get all paths for all repos.
		wxArrayString&   cRemoteRepos,//< [out] Array to add remote repos to.
		wxArrayString&   cRemoteRepoAliases //< [out] Array of aliases.  It will be the same length as cRemoteRepos.  
		);

	/**
	 * Finds the working copy that contains a given path.
	 * Returns true if the given path is in a working copy, or false if it isn't.
	 */
	bool FindWorkingFolder(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sPath,    //< [in] The path to find a working copy from.
		wxString*        pFolder,  //< [out] The root of the found working copy.
		                           //<       Not modified if no working copy is found.
		                           //<       If set, this is suitable for passing to other vvVerbs functions.
		wxString*        pRepo,    //< [out] The local name of the repo that the found working copy is from.
		                           //<       Not modified if no working copy is found.
		                           //<       If set, this is suitable for passing to other vvVerbs functions.
		wxArrayString*        pBaselineHIDs = NULL     //< [out] The baseline HID of the working copy.
		                           //<       Not modified if no working copy is found.
		                           //<       If set, this is suitable for passing to other vvVerbs functions.
		);

	/**
	 * Finds the single working copy that contains a given set of paths.
	 * Returns true if the given paths are in a single working copy, or false if they aren't.
	 */
	bool FindWorkingFolder(
		class vvContext&     cContext, //< [in] [out] Error and context info.
		const wxArrayString& cPaths,   //< [in] The path to find a working copy from.
		wxString*            pFolder,  //< [out] The root of the found working copy.
		                               //<       Not modified if no working copy is found.
		                               //<       If set, this is suitable for passing to other vvVerbs functions.
		wxString*            pRepo,    //< [out] The local name of the repo that the found working copy is from.
		                               //<       Not modified if no working copy is found.
		                               //<       If set, this is suitable for passing to other vvVerbs functions.
		wxArrayString*        pBaselineHIDs   = NULL   //< [out] The baseline HID of the working copy.
		                           //<       Not modified if no working copy is found.
		                           //<       If set, this is suitable for passing to other vvVerbs functions.
		);

	/**
	 * Converts an absolute path into a repo path in a given working copy.
	 */
	bool GetRepoPath(
		class vvContext& cContext,       //< [in] [out] Error and context info.
		const wxString&  sAbsolutePath,  //< [in] The absolute paths to convert.
		const wxString&  sWorkingFolder, //< [in] The working copy to convert relative to.
		wxString&        sRepoPath       //< [out] The converted repo path.
		);

	/**
	 * Converts a set of absolute paths into a set of repo paths in a given working copy.
	 */
	bool GetRepoPaths(
		class vvContext&     cContext,       //< [in] [out] Error and context info.
		const wxArrayString& cAbsolutePaths, //< [in] The absolute paths to convert.
		const wxString&      sWorkingFolder, //< [in] The working copy to convert relative to.
		wxArrayString&       cRepoPaths      //< [in] The converted repo paths.
		);

	/**
	 * Converts a repo path into a path that is relative to its working copy's root folder.
	 */
	bool GetRelativePath(
		class vvContext& cContext,     //< [in] [out] Error and context info.
		const wxString&  sRepoPath,    //< [in] The repo path to convert.
		wxString&        sRelativePath,//< [out] The converted relative path.
		bool			 bLogError  = true //< [in] Log an error if it can't be found
		);

	/**
	 * Converts a set of repo paths into paths that are relative to their working copy's root folders.
	 */
	bool GetRelativePaths(
		class vvContext&     cContext,      //< [in] [out] Error and context info.
		const wxArrayString& cRepoPaths,    //< [in] The repo paths to convert.
		wxArrayString&       cRelativePaths //< [out] The converted relative paths.
		);

	/**
	 * Converts a relative or repo path into an absolute path based at a given working copy root.
	 */
	bool GetAbsolutePath(
		class vvContext& cContext,     //< [in] [out] Error and context info.
		const wxString&  sFolder,      //< [in] Absolute path of the working copy root.
		const wxString&  sPath,        //< [in] Relative or repo path to convert.
		wxString&        sAbsolutePath,//< [out] The converted absolute path.
		bool			 bLogError  = true //< [in] Log an error if it can't be found
		);
	
	/**
	 * Converts a set of relative or repo paths into absolute paths based at a given working copy root.
	 */
	bool GetAbsolutePaths(
		class vvContext&     cContext,      //< [in] [out] Error and context info.
		const wxString&      sFolder,       //< [in] Absolute path of the working copy root.
		const wxArrayString& cPaths,        //< [in] Relative or repo paths to convert.
		wxArrayString&       cAbsolutePaths, //< [out] The converted absolute paths.
		bool			 bLogError  = true //< [in] Log an error if it can't be found
		);

	/**
	 * Checks the specified repo to see if it 
	 */
	bool CheckForOldRepoFormat(
		class vvContext&     cContext,      //< [in] [out] Error and context info.
		const wxString& sRepoName);

	/**
	 * Gets the unique GID of an item at a given absolute path.
	 */
	bool GetGid(
		class vvContext& cContext,      //< [in] [out] Error and context info.
		const wxString&  sAbsolutePath, //< [in] Absolute path of the item to get the GID of.
		wxString&        sGid           //< [out] The GID of the item.
		);

	/*
	 * Return an array suitable for putting in a selection list or combo dropdown.
	*/
	wxArrayString GetBranchDisplayNames(vvVerbs::BranchList bl);

	bool BranchEntryComparer(const vvVerbs::BranchEntry& b1, const vvVerbs::BranchEntry& b2);

	/**
	 * Gets a list of branches in a repo.
	 */
	bool GetBranches(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] The name of the repo to get a list of branches from.
		bool bIncludeClosed,	   //< [in] Include branches that have been marked closed.
		BranchList&   cBranches //< [out] Array to add branches to.
		);

	/**
	 * Checks whether or not a branch exists in a repo.
	 */
	bool BranchExists(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] Name of rhe repo to look for the branch in.
		const wxString&  sBranch,  //< [in] Name of the branch to check for.
		bool&            bExists,  //< [out] True if the branch exists, false otherwise.
		bool&            nbOpen    //< [out] Return value of true if the branch is open
								   //<       Return value of false if the branch is closed, or it doesn't exist.
		);

	/**
	 * Gets a list of users in a repo.
	 */
	bool GetUsers(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] The name of the repo to get a list of users from.
		stlUserDataList& cUsers    //< [out] List to add users to.
		);

	/**
	 * Gets a list of users in a repo.
	 */
	bool GetUsers(
		class vvContext& cContext,          //< [in] [out] Error and context info.
		const wxString&  sRepo,             //< [in] The name of the repo to get a list of users from.
		wxArrayString&   cUsers,            //< [out] List to add user names to.
		vvNullable<bool> nbActive  = true,  //< [in] True to only get active users.
		                                    //<      False to only get inactive users.
		                                    //<      vvNULL to get both active and inactive users.
		bool             bNobody   = false  //< [in] Whether or not the "nobody" user should be included.
		);

	/**
	 * Looks up data about a user based on their ID.
	 */
	bool GetUserById(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] The name of the repo to lookup the user in.
		const wxString&  sUserId,  //< [in] The ID of the user to lookup.
		bool&            bExists,  //< [out] Whether or not the given user exists.
		UserData*        pUser     //< [out] Data about the requested user.
		                           //<       If bExists is false, this is untouched.
		);

	/**
	 * Looks up data about a user based on their name.
	 */
	bool GetUserByName(
		class vvContext& cContext,  //< [in] [out] Error and context info.
		const wxString&  sRepo,     //< [in] The name of the repo to lookup the user in.
		const wxString&  sUserName, //< [in] The name of the user to lookup.
		bool&            bExists,   //< [out] Whether or not the given user exists.
		UserData*        pUser      //< [out] Data about the requested user.
		                            //<       If bExists is false, this is untouched.
		);

	/**
	 * Gets the current user in a repo.
	 */
	bool GetCurrentUser(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] The name of the repo to get the current user from.
		wxString&        sUser     //< [out] The current user in the given repo, or wxEmptyString if none is set.
		);

	/**
	 * Sets the current user in a repo.
	 */
	bool SetCurrentUser(
		class vvContext& cContext,       //< [in] [out] Error and context info.
		const wxString&  sRepo,          //< [in] The name of the repo to set the current user in.
		const wxString&  sUser,          //< [in] The user to set as current in the given repo.
		bool             bCreate = false //< [in] Whether or not the specified user should be created if it doesn't exist.
		                                 //<      Passing true is equivalent to using the --create flag with whoami.
		);

	/**
	 * Creates a new user in a repo.
	 */
	bool CreateUser(
		class vvContext& cContext,  //< [in] [out] Error and context info.
		const wxString&  sRepo,     //< [in] The name of the repo to create a user in.
		const wxString&  sUsername, //< [in] The name of the user to create.
		wxString*        pId        //< [out] The ID of the new user.
		);

	/**
	 * Gets a list of stamps in a repo.
	 */
	bool GetStamps(
		class vvContext&  cContext, //< [in] [out] Error and context info.
		const wxString&   sRepo,    //< [in] The name of the repo to get a list of stamps from.
		stlStampDataList& cStamps   //< [out] List to add stamps to.
		);
	
	/**
	 * Gets a list of tags in a repo.
	 */
	bool GetTags(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] The name of the repo to get a list of tags from.
		stlTagDataList&  cTags     //< [out] List to add tags to.
		);

	/**
	 * Gets a list of tags in a repo.
	 */
	bool GetTags(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] The name of the repo to get a list of tags from.
		wxArrayString&   cTags     //< [out] List to add tag names to.
		);

	/**
	 * Checks if a tag exists in a repo.
	 */
	bool TagExists(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepo,    //< [in] Name of the repo to check for the tag in.
		const wxString&  sTag,     //< [in] Name of the tag to check for.
		bool&            bExists   //< [in] Whether or not the tag exists in the repo.
		);

	/**
	 * Checks if a revision exists in a repo.
	 * Can lookup partial HIDs and/or local-only IDs, and only considers them to
	 * exist if there is exactly one matching revision.
	 */
	bool RevisionExists(
		class vvContext& cContext,       //< [in] [out] Error and context info.
		const wxString&  sRepo,          //< [in] The repo to check for the revision in.
		const wxString&  sRevision,      //< [in] HID of the revision to check for.
		                                 //<      May be partial or a local-only ID.
		bool&            bExists,        //< [in] True if the revision exists, false otherwise.
		                                 //<      Also false if multiple existing revisions match sRevision.
		wxString*        pFullHid = NULL //< [out] The full HID of the found revision.
		                                 //<       Helpful if sRevision was only a prefix.
		);

	/**
	 * Gets the branch that a given working copy is tied to.
	 */
	bool GetBranch(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sFolder,  //< [in] The working copy to get the tied branch from.
		wxString&        sBranch   //< [out] The branch that the given working copy is tied to.
		                           //<       Set to an empty string if the folder is not tied.
		);
	
	bool GetSetting_Revert__Save_Backups(
		vvContext&     pCtx,						//< [in] [out] Error and context info.
		bool * pbSaveBackups						//< [out] The value of the user's revert backup setting.
		);


	bool vvVerbs::SetSetting_Revert__Save_Backups(
		vvContext&     pCtx, 						//< [in] [out] Error and context info.
		bool bSaveBackups							//< [in] The new value for the user's revert backup setting.
		);

	bool GetHistoryFilterDefaults(
		vvContext&     pCtx,						//< [in] [out] Error and context info.
		const wxString& sRepoName,					//< [in] The repo to load defaults for.  These are repo instance specific.
		vvHistoryFilterOptions& cHistoryDefaults	//< [out] The history filter defaults that are stored in localsettings.
		);

	/**
	 * Creates a new local repository.
	 */
	bool New(
		class vvContext& cContext,                       //< [in] [out] Error and context info.
		const wxString&  sName,                          //< [in] The unique (among local repos) name for the new repo.
		const wxString&  sFolder        = wxEmptyString, //< [in] A folder to checkout the new repo into.
		                                                 //<      Empty string indicates to not checkout the new repo.
		const wxString&  sSharedUsersRepo= wxEmptyString,//< [in] A local repo name that can be used to link the users list
		                                                 //<      Empty string indicates to not share users.
		const wxString&  sStorageDriver = wxEmptyString, //< [in] The storage driver to use for the new repo.
		                                                 //<      Empty string to use the default storage driver.
		const wxString&  sHashMethod    = wxEmptyString  //< [in] The hash method to use in the new repo.
		                                                //<      Empty string to use the default hash method.
		);

	/**
	 * Creates a new local repo that is a clone of an existing repo.
	 */
	bool Clone(
		class vvContext& cContext,      //< [in] [out] Error and context info.
		const wxString&  sExistingRepo, //< [in] The existing repo to clone.  Might be a local repo name or remote repo URL.
		const wxString&  sNewRepoName,   //< [in] The unique (among local repos) name to use for the new repo.
		const wxString&  sUsername,						//< [in] The username that will be provided to the remote server for authentication.
		const wxString&  sPassword						//< [in] The password that will be provided to the remote server for authentication.
		);

	/**
	 * Update a working copy to the given branch or revision.
	 */
	bool Update(
		vvContext&      pCtx,		//< [in] [out] Error and context info.
		const wxString& sFolder,	//< [in] the disk path to the working copy
		const wxString& sBranchAttachment, //< [in] the branch to attach to.
		const class vvRevSpec *cRevSpec,			//< [in] the revision/branch to update to.  Pass NULL to update to the latest head for the currently attached branch.
		const vvNullable<bool> bDetach			//< [in] detach the working folder.
		);
		
	/**
	 * Creates a working copy from a local repo.
	 * sRevision and sTag cannot both be specified.
	 * If neither is specified, the HEAD revision will be checked out.
	 */
	bool Checkout(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sName,    //< [in] The name of the local repo to checkout.
		const wxString&  sFolder,  //< [in] Absolute path of the folder to check the repo out into.
		CheckoutType     eType,    //< [in] The type of item to checkout from.
		const wxString&  sTarget   //< [in] The target to checkout (possible values depend on eType).
		);

	/**
	 * Computes the difference between a working copy and a changeset.
	 */
	bool Status(
		class vvContext& cContext,  //< [in] [out] Error and context info.
		const vvRepoLocator& cRepoLocator, //< [in] The repo to use.
		const wxString&  sRevision, //< [in] The revision to diff the working copy against.
		                            //<      If empty, the first parent of the working copy is used.
		const wxString&  sRevision2, //< [in] If two revisions are passed, we return the differences between two 
									 //<      historical versions.
		DiffData&        cDiff      //< [out] Data about the differences between the given folder and revision.
		);

	/**
	 * Rename an object on disk.
	 */
	bool Rename(
		class vvContext&      pCtx,					// [in] [out] Error and context info.
		const wxString& sFullPath_objectToRename,	// [in] The full disk path of the object to rename.  The object must exist on disk.
		const wxString& sNewName					// [in] The new name to give to the item.
		);

	/**
	 * Commits a working copy to a repo as a new changeset.
	 */
	bool Commit(
		class vvContext&     cContext,         //< [in] [out] Error and context info.
		const wxString&      sFolder,          //< [in] The working copy to commit.
		const wxString&      sComment,         //< [in] Comment to add to the new changeset.
		const wxArrayString* pPaths    = NULL, //< [in] Repo paths of individual dirty items to commit.
		                                       //<      If NULL, then all dirty items will be committed.
		const wxArrayString* pStamps   = NULL, //< [in] List of stamps to apply to the new changeset.
		                                       //<      Passing NULL the same as passing an empty list.
		const wxArrayString* pWorkItemAssociations = NULL, //< [in] The list of work items that should be associated with 
														   // the new changeset.
		const wxString*      pUser     = NULL, //< [in] The username to commit as.
		                                       //<      If NULL, then the current user is used.
		vvFlags              cFlags    = 0u,   //< [in] COMMIT_* flags to customize behavior.
		wxString*            pHid      = NULL  //< [out] The HID of the new changeset.
		);

	/**
	 * Add a comment to an existing changeset.
	 */
	bool AddComment(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepoName,    //< [in] The local repo name for the changeset.
		const wxString&  sChangesetHID,  //< [in] The changeset ID
		const wxString&  sNewComment	//< [in] The text of the new comment
		);

	bool AddStamp(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepoName,    //< [in] The local repo name for the changeset.
		const wxString&  sChangesetHID,  //< [in] The changeset ID
		const wxString&  sNewStamp	//< [in] The new stamp
		);

	bool AddTag(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sRepoName,    //< [in] The local repo name for the changeset.
		const wxString&  sChangesetHID,  //< [in] The changeset ID
		const wxString&  sNewTag	//< [in] The new tag
		);

	bool DeleteStamp(
		class vvContext& pCtx,			//< [in] [out] Error and context info.
		const wxString&  sRepoName,		//< [in] The local repo name for the changeset.
		const wxString&  sChangesetHID,	//< [in] The changeset ID
		const wxString&  sRemoveMe		//< [in] The stamp to remove
		);

	bool DeleteTag(
		class vvContext& pCtx,			//< [in] [out] Error and context info.
		const wxString&  sRepoName,		//< [in] The local repo name for the changeset.
		const wxString&  sRemoveMe		//< [in] The tag to remove
		);
	/**
	 * Move an object on disk.
	 */
	bool Move(
		class vvContext&      pCtx,					// [in] [out] Error and context info.
		const wxArrayString& sFullPath_objectToMove,		// [in] The full disk path of the object to move.  The object must exist on disk.
		const wxString& sNewParentPath				// [in] The new parent path.
		);
	
	/**
	 * Add an object on disk.
	 */
	bool Add(
		class vvContext&      pCtx,						// [in] [out] Error and context info.
		const wxArrayString& sFullPath_objectToAdd		// [in] The full disk path of the object to add.  The object must exist on disk.
		);

	/**
	 * Remove an object from the repository.
	 */
	bool Remove(
		class vvContext&      pCtx,						// [in] [out] Error and context info.
		const wxArrayString& sFullPath_objectToRemove	// [in] The full disk path of the object to remove.  The object must exist on disk.
		);

	/**
	 * Revert a change inside a working copy.
	 */
	bool Revert(
		class vvContext&      pCtx,						// [in] [out] Error and context info.
		wxString sFolder,								// [in] the disk path to the working copy
		const wxArrayString *sRepoPaths,				// [in] The array of repo paths to revert.  Send NULL to revert all changes in the working copy.
		bool bNoBackups									// [in] Controls the saving of backup files, if they are modified.
		);

	/**
	 * Pull changes from a remote server or from a local repo.
	 */
	bool Pull(
		class vvContext& cContext,						//< [in] [out] Error and context info.
		const wxString&  sSrcLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  Changes will be pulled from that repo.
		const wxString&  sDestLocalRepoName,			//< [in] The name of the local repo to pull changes into.
		const wxArrayString& saAreas,					//< [in] The area names of areas to update.
		const wxArrayString& saTags,					//< [in] The tags that apply to dagnodes to pull
		const wxArrayString& saDagnodes,				//< [in] The dagnode ids that will be pulled.
		const wxArrayString& saBranchNames,				//< [in] The names of branches that will be pulled.
		const wxString&  sUsername,						//< [in] The username that will be provided to the remote server for authentication.
		const wxString&  sPassword						//< [in] The password that will be provided to the remote server for authentication.
		);

	/**
	 * Pushes changes from a local repo to another repo.
	 */
	bool Push(
		class vvContext& cContext,						//< [in] [out] Error and context info.
		const wxString&  sDestLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  Changes will be pulled from that repo.
		const wxString&  sSrcLocalRepoName,				//< [in] The name of the local repo to push changes from.
		const wxArrayString& saAreas,					//< [in] The area names of areas to update
		const wxArrayString& saTags,					//< [in] The tags that apply to dagnodes to push
		const wxArrayString& saDagnodes,				//< [in] The dagnode ids that will be pushed.
		const wxArrayString& saBranchNames,				//< [in] The names of branches that will be pushed.
		const wxString&  sUsername,						//< [in] The username that will be provided to the remote server for authentication.
		const wxString&  sPassword						//< [in] The password that will be provided to the remote server for authentication.
		);
		

	/**
	 * Load the changesets which are different bettween a local and remote repos.
	 */
	bool Incoming(
		class vvContext& cContext,						//< [in] [out] Error and context info.
		const wxString&  sSrcLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  Changes will be pulled from that repo.
		const wxString&  sDestLocalRepoName,			//< [in] The name of the local repo to pull changes into.
		const wxString&  sUsername,						//< [in] The username that will be provided to the remote server for authentication.
		const wxString&  sPassword,						//< [in] The password that will be provided to the remote server for authentication.
		HistoryData& cHistoryData
		);

	/**
	 * Load the changesets which are different bettween a local and remote repos.
	 */
	bool Outgoing(
		class vvContext& cContext,						//< [in] [out] Error and context info.
		const wxString&  sDestLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  Changes will be pulled from that repo.
		const wxString&  sSrcLocalRepoName,			//< [in] The name of the local repo to pull changes into.
		const wxString&  sUsername,						//< [in] The username that will be provided to the remote server for authentication.
		const wxString&  sPassword,						//< [in] The password that will be provided to the remote server for authentication.
		HistoryData& cHistoryData
		);

	bool CheckOtherRepo_IsItRemote(
		class vvContext& cContext,							//< [in] [out] Error and context info.
		const wxString&  sOtherLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  
		bool& bIsRemote										//< [out] If true, then the repo is remote.
		);

	bool History(
		class vvContext& cContext,						//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		const vvHistoryFilterOptions filterOptions,		//< [in] History Options.
		HistoryData& cHistoryData,						//< [out] History Results
		void ** ppNewHistoryToken						//< [out] A token that can be used to fetch more data.
		);

	bool History__Fetch_More(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		unsigned int nNumResultsDesired,				//< [in] The number of additional results desired.
		void * pHistoryToken,							//< [in]  The token that was returned by a previous call to history.  This will be freed if the fetch is successful.
		HistoryData& cHistoryData,						//< [out] The new data.
		void ** ppNewHistoryToken						//< [out] A new token that can be used to fetch more data.
		);

	bool History__Free_Token(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		void * pHistoryToken);							//< [in] The token to free.

	bool Repo_Item_Get_Details(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		wxString sRepoName,								//< [in] The name of the local repo 
		wxString sChangesetHID,							//< [in] The changeset to browse
		wxString sItemPath,								//< [in] The full path of the item to look up.  This must be a repo path.
		RepoItem& cDetails);							//< [out] The details about the item in question.

	bool Repo_Item_Get_Children(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		wxString sRepoName,								//< [in] The name of the local repo 
		wxString sChangesetHID,							//< [in] The changeset to browse
		wxString sItemPath,								//< [in] The full path of the item to look up.  This must be a repo path.
		RepoItemList& cChildren);					//< [out]  The list of children.

	bool Get_Revision_Details(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		const wxString& sRevID,							//< [in] The revision to load
		HistoryEntry& cHistoryEntry,						//< [out] The details.
		vvVerbs::RevisionList& children						//< [out] A vector of the children of the revision.
		);

	bool Export_Revision(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		const wxString& sRevID,							//< [in] The revision to load
		wxString sLocalOutputPath						//< [in] The local path to output to.
		);

	bool Diff(
		vvContext&           pCtx,						//< [in] [out] Error and context info.
		const vvRepoLocator& cRepoLocator,				//< [in] The locator for the repo to diff aginst.
		const wxArrayString* pPaths,					//< [in] The paths to limit the diff to.
		bool bRecursive,								//< [in] Perform a recursive diff.
		wxString* sRev1,								//< [in] The first revision to diff against.  May be null.
		wxString* sRev2,								//< [in] The second revision to diff against.  May be null.
		wxString* sTool);							//< [in] The tool to use to diff.  May be null.

	bool GetBranchHeads(
		vvContext&           pCtx,						//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		const wxString& sBranchName,					//< [in] The branch to query
		wxArrayString& sBranchHeadHids					//< [out] The current heads of that branch.
		);

	/**
	 * Merges a new revision into a working copy, giving it another parent.
	 */
	bool Merge(
		vvContext&             pCtx,                    //< [in] [out] Error and context info.
		const wxString&        sFolder,                 //< [in] Working copy to merge a new revision into.
		const class vvRevSpec& cRevSpec,                //< [in] Revision to merge into the working copy.
		const bool bAllowFastForward,					//< [in] Allow merge to use a fast-forward merge.
		const wxString&        sTool    = wxEmptyString //< [in] Merge tool to use for automated merges.
		                                                //<      If empty, a default tool will be used.
		);

	/**
	 * Gets a list of available diff tools.
	 */
	bool GetDiffTools(
		vvContext&      cContext,         //< [in] [out] Error and context info.
		const wxString& sRepo,            //< [in] Name of the repo to get repo-specific tools from.
		                                  //<      If empty, only built-in and machine-wide tools are included.
		wxArrayString&  cTools,           //< [out] Filled with a list of diff tools.
		bool            bInternal = false //< [in] Whether or not "internal" tools should be included in the list.
		);

	/**
	 * Gets an appropriate diff tool to use on an item.
	 */
	bool GetDiffTool(
		vvContext&      cContext,    //< [in] [out] Error and context info.
		const wxString& sItem,       //< [in] Full path of the item to get a diff tool for.
		const wxString& sSuggestion, //< [in] Name of a suggested tool to use.
		                             //<      May not end up being chosen.
		                             //<      May be empty if no suggestion is available/appropriate.
		wxString&       sTool        //< [out] Name of the tool selected for the item.
		);

	/**
	 * Gets a list of available merge tools.
	 */
	bool GetMergeTools(
		vvContext&      cContext,         //< [in] [out] Error and context info.
		const wxString& sRepo,            //< [in] Name of the repo to get repo-specific tools from.
		                                  //<      If empty, only built-in and machine-wide tools are included.
		wxArrayString&  cTools,           //< [out] Filled with a list of merge tools.
		bool            bInternal = false //< [in] Whether or not "internal" tools should be included in the list.
		);

	/**
	 * Gets an appropriate merge tool to use on an item.
	 */
	bool GetMergeTool(
		vvContext&       cContext,    //< [in] [out] Error and context info.
		const wxString&  sItem,       //< [in] Full path of the item to get a merge tool for.
		MergeToolContext eContext,    //< [in] Context the tool is needed for.
		                              //<      Use NONE to not specify a context.
		const wxString&  sSuggestion, //< [in] Name of a suggested tool to use.
		                              //<      May not end up being chosen.
		                              //<      May be empty if no suggestion is available/appropriate.
		wxString&        sTool        //< [out] Name of the tool selected for the item.
		);

	/**
	 * Retrieves resolve data from a working copy.
	 *
	 * Note: The returned data is only valid for the current state of the specified working copy.
	 *       If anything changes in the working copy, the data should be considered obsolete.
	 *       Resolve_Merge and Resolve_Accept are two such functions that modify the working copy,
	 *       so existing resolve data should be discarded after calling it.
	 */
	bool Resolve_GetData(
		vvContext&          cContext, //< [in] [out] Error and context info.
		const wxString&     sFolder,  //< [in] The working folder to retrieve resolve data from.
		stlResolveItemList& cData     //< [out] The retrieved resolve data.
		);

	/**
	 * Retrieves resolve data from a working copy about a single item.
	 * See notes on other overload.
	 */
	bool Resolve_GetData(
		vvContext&      cContext, //< [in] [out] Error and context info.
		const wxString& sFolder,  //< [in] The working folder to retrieve resolve data from.
		const wxString& sGid,     //< [in] GID of the item to retrieve resolve data about.
		ResolveItem&    cData     //< [out] The retrieved resolve item data.
		);

	/**
	 * Displays a diff of two values on a single choice/conflict of an item.
	 */
	bool Resolve_Diff(
		vvContext&         cContext,             //< [in] [out] Error and context info.
		const wxString&    sFolder,              //< [in] The working folder to diff values in.
		const wxString&    sItemGid,             //< [in] GID of the item to diff values on.
		ResolveChoiceState eChoice,              //< [in] The choice on the item to diff values of.
		const wxString&    sLeftValue,           //< [in] The label of the first value to diff.
		const wxString&    sRightValue,          //< [in] The label of the second value to diff.
		const wxString&    sTool = wxEmptyString //< [in] Name of the diff tool to use to diff the values.
		                                         //<      If empty, the configured default tool is used.
		);

	/**
	 * Merges two values on a single mergeable choice/conflict to create a new value.
	 *
	 * Warning: This function invalidates any previously retrieved resolve data!
	 */
	bool Resolve_Merge(
		vvContext&         cContext,                     //< [in] [out] Error and context info.
		const wxString&    sFolder,                      //< [in] The working folder to merge values in.
		const wxString&    sItemGid,                     //< [in] GID of the item to merge values on.
		ResolveChoiceState eChoice,                      //< [in] The choice on the item to merge values of.
		const wxString&    sBaselineValue,               //< [in] The label of the value to use as the baseline parent.
		const wxString&    sOtherValue,                  //< [in] The label of the value to use as the other parent.
		const wxString&    sTool        = wxEmptyString, //< [in] Name of the merge tool to use to merge the values.
		                                                 //<      If empty, the configured default tool is used.
		const wxString&    sLabel       = wxEmptyString, //< [in] Label to assign to the newly created value.
		                                                 //<      If empty, a label is automatically generated.
		int*               pStatus      = NULL,          //< [out] Exit code returned from the merge tool.
		wxString*          pMergedValue = NULL           //< [out] Label of the newly created merged value.
		);

	/**
	 * Resolves a single choice/conflict on an item by accepting one of its
	 * values as the resolution.
	 *
	 * Warning: This function invalidates any previously retrieved resolve data!
	 */
	bool Resolve_Accept(
		vvContext&         cContext, //< [in] [out] Error and context info.
		const wxString&    sFolder,  //< [in] The working folder to accept a value in.
		const wxString&    sItemGid, //< [in] GID of the item to accept a value on.
		ResolveChoiceState eChoice,  //< [in] The choice on the item to accept a value for.
		const wxString&    sValue    //< [in] The label of the value to accept.
		);
	/**
	 * Create a new branch, attach to an existing branch, or detach from a branch.
	 */
	bool PerformBranchAttachmentAction(
		vvContext&             pCtx,                    //< [in] [out] Error and context info.
		const wxString&        sFolder,                 //< [in] Working copy to merge a new revision into.
		const vvVerbs::BranchAttachmentAction action,   //< [in] The action to perform.
		const wxString&        sBranchName				//< [in] If the action is ATTACH or CREATE, this is the branch name to use.
		);

	/**
	 * Check to see if a direct dag path exists between two revisions.
	 */
	bool CheckForDagConnection(
		vvContext&             pCtx,                    //< [in] [out] Error and context info.
		const wxString&        sRepoName,               //< [in] The repo that contains the change sets.
		const wxString&        sPossibleAncestor,		//< [in] The potential parent.
		const wxString&        sPossibleDescendant,		//< [in] The potential descendant.
		bool *				   bResultAreConnected		//< [out] The result of the check.
		);

	/*
		Delete an object on the disk, and send to the recycle bin.
	*/
	bool vvVerbs::DeleteToRecycleBin(
		vvContext&             pCtx,
		const wxString&        sDiskPath,
		bool				   bOkToShowGUI
		);

	/*
		List the areas in a local repo
	*/
	bool ListAreas(
		vvContext&             pCtx,
		const wxString&        sRepoName,
		wxArrayString&		   sAreaNames
		);

	/*
		Start the web server.
	*/
	bool StartWebServer(
		vvContext&             pCtx,
		struct mg_context **ctx,
		bool bPublic,
		wxString sWorkingFolderLocation,
		int nPortNumber
		);

	/*
		Stop the web server.
	*/
	bool StopWebServer(
		vvContext&             pCtx,
		struct mg_context *ctx
		);	
	/*
		Get the name of the version control area.
	*/

	wxString GetVersionControlAreaName();

	/**
	 * Gets a string name for a config scope.
	 */
	const char* GetConfigScopeName(
		ConfigScope eScope //< [in] The ConfigScope to get the name of.
		);

	/**
	 * Get the value of a config setting.
	 */
	bool GetConfigValue(
		vvContext&      cContext, //< [in] [out] Error and context info.
		vvFlags         cScope,   //< [in] The scope(s) to check for the setting.
		                          //<      If multiple scopes are specified, they are checked in order.
		                          //<      If zero, the scope is assumed to be embedded in the name.
		                          //<      Usually you want to specify CONFIG_SCOPE_ALL.
		const wxString& sName,    //< [in] Name of the config setting to retrieve.
		                          //<      Probably looks like a path, i.e. "server/files"
		                          //<      If cScope is zero, this should include the scope.
		const wxString& sRepo,    //< [in] Name of the repo to find the setting in.
		                          //<      If empty, then instance, repo, and admin scoped settings are ignored.
		wxString&       sValue,   //< [out] Value of the retrieved setting.
		const wxString& sDefault  //< [in] Value to return if the setting doesn't exist.
		);

	/**
	 * Get the value of a config setting.
	 */
	bool GetConfigValue(
		vvContext&      cContext, //< [in] [out] Error and context info.
		vvFlags         cScope,   //< [in] The scope(s) to check for the setting.
		                          //<      If multiple scopes are specified, they are checked in order.
		                          //<      If zero, the scope is assumed to be embedded in the name.
		                          //<      Usually you want to specify CONFIG_SCOPE_ALL.
		const wxString& sName,    //< [in] Name of the config setting to retrieve.
		                          //<      Probably looks like a path, i.e. "server/files"
		                          //<      If cScope is zero, this should include the scope.
		const wxString& sRepo,    //< [in] Name of the repo to find the setting in.
		                          //<      If empty, then instance, repo, and admin scoped settings are ignored.
		wxInt64&        iValue,   //< [out] Value of the retrieved setting.
		wxInt64         iDefault  //< [in] Value to return if the setting doesn't exist.
		);

	/**
	 * Get the value of a config setting.
	 */
	bool GetConfigValue(
		vvContext&      cContext, //< [in] [out] Error and context info.
		vvFlags         cScope,   //< [in] The scope(s) to check for the setting.
		                          //<      If multiple scopes are specified, they are checked in order.
		                          //<      If zero, the scope is assumed to be embedded in the name.
		                          //<      Usually you want to specify CONFIG_SCOPE_ALL.
		const wxString& sName,    //< [in] Name of the config setting to retrieve.
		                          //<      Probably looks like a path, i.e. "server/files"
		                          //<      If cScope is zero, this should include the scope.
		const wxString& sRepo,    //< [in] Name of the repo to find the setting in.
		                          //<      If empty, then instance, repo, and admin scoped settings are ignored.
		wxArrayString&  cValue    //< [out] Value of the retrieved setting.
		                          //<       If the setting doesn't exist, the value is untouched.
		);

	/**
	 * Sets the value of a config setting.
	 */
	bool SetConfigValue(
		vvContext&      cContext, //< [in] [out] Error and context info.
		vvFlags         cScope,   //< [in] The scope(s) to set the setting in.
		                          //<      If multiple scopes are specified, the setting is set in all of them.
		                          //<      If zero, the scope is assumed to be embedded in the name.
		const wxString& sName,    //< [in] Name of the config setting to retrieve.
		                          //<      Probably looks like a path, i.e. "server/files"
		                          //<      If cScope is zero, this should include the scope.
		const wxString& sRepo,    //< [in] Name of the repo to set the setting in.
		                          //<      If empty, then instance, repo, and admin settings cannot be set.
		const wxString& sValue    //< [in] Value to change the config setting to.
		);

	/**
	 * Sets the value of a config setting.
	 */
	bool SetConfigValue(
		vvContext&      cContext, //< [in] [out] Error and context info.
		vvFlags         cScope,   //< [in] The scope(s) to set the setting in.
		                          //<      If multiple scopes are specified, the setting is set in all of them.
		                          //<      If zero, the scope is assumed to be embedded in the name.
		const wxString& sName,    //< [in] Name of the config setting to retrieve.
		                          //<      Probably looks like a path, i.e. "server/files"
		                          //<      If cScope is zero, this should include the scope.
		const wxString& sRepo,    //< [in] Name of the repo to set the setting in.
		                          //<      If empty, then instance, repo, and admin settings cannot be set.
		wxInt64         iValue    //< [in] Value to change the config setting to.
		);

	/**
	 * Sets the value of a config setting.
	 */
	bool SetConfigValue(
		vvContext&           cContext, //< [in] [out] Error and context info.
		vvFlags              cScope,   //< [in] The scope(s) to set the setting in.
		                               //<      If multiple scopes are specified, the setting is set in all of them.
		                               //<      If zero, the scope is assumed to be embedded in the name.
		const wxString&      sName,    //< [in] Name of the config setting to retrieve.
		                               //<      Probably looks like a path, i.e. "server/files"
		                               //<      If cScope is zero, this should include the scope.
		const wxString&      sRepo,    //< [in] Name of the repo to set the setting in.
		                               //<      If empty, then instance, repo, and admin settings cannot be set.
		const wxArrayString& cValue    //< [in] Value to change the config setting to.
		);

	/**
	 * Unsets the value of a config setting.
	 */
	bool UnsetConfigValue(
		vvContext&           cContext, //< [in] [out] Error and context info.
		vvFlags              cScope,   //< [in] The scope(s) to set the setting in.
		                               //<      If multiple scopes are specified, the setting is set in all of them.
		                               //<      If zero, the scope is assumed to be embedded in the name.
		const wxString&      sName,    //< [in] Name of the config setting to retrieve.
		                               //<      Probably looks like a path, i.e. "server/files"
		                               //<      If cScope is zero, this should include the scope.
		const wxString&      sRepo     //< [in] Name of the repo to set the setting in.
		                               //<      If empty, then instance, repo, and admin settings cannot be set.
		);
	
	/**
	 * List all the repos which need to be upgraded.
	 */
	bool Upgrade__listOldRepos(
		vvContext&      pCtx,
		wxArrayString& aRepoNames
		);

	/**
	 * Upgrades all repos to the latest version
	 */
	bool Upgrade(
		vvContext&           cContext //< [in] [out] Error and context info.
		);

	bool Lock(
		vvContext&           pCtx,
		const vvRepoLocator& cRepoLocator,
		const wxString& sLockServer,
		const wxString& sUsername,
		const wxString& sPassword,
		const wxArrayString& pPaths
		);

	bool vvVerbs::Unlock(
		vvContext&           pCtx,
		const vvRepoLocator& cRepoLocator,
		const wxString& sLockServer,
		bool bForce,
		const wxString& sUsername,
		const wxString& sPassword,
		const wxArrayString& pPaths
		);

	bool CheckFileForLock(
		vvContext&           pCtx,
		const vvRepoLocator& cRepoLocator,
		const wxString& sLocalFile,
		vvVerbs::LockList& cLockData);

	bool GetSavedPassword(vvContext&           cContext, //< [in] [out] Error and context info.
		const wxString& sRemoteRepo,							//< [in] The remote repository 
		const wxString& sUserName,								//< [in] The username that we used to save the password
		wxString& sPassword);									//< [out] The saved password.

	bool vvVerbs::SetSavedPassword(vvContext&           pCtx, //< [in] [out] Error and context info.
		const wxString& sRemoteRepo,							//< [in] The remote repository 
		const wxString& sUserName,								//< [in] The username that we used to save the password
		const wxString& sPassword);								//< [in] The saved password.

	/**
	 * Deletes all of the user's saved Veracity passwords.
	 */
	bool DeleteSavedPasswords(
		vvContext& cContext //< [in] [out] Error and context info.
		);

	bool ListWorkItems(vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString& sRepoName,								//< [in] The repository that contains the work items
		const wxString& sSearchTerm,							//< [in] The search term
		WorkItemList& workItems);								//< [in] the results

	bool vvVerbs::RefreshWC(vvContext&           pCtx,			//< [in] [out] Error and context info.
		const wxString& sWC_Root,								//< [in] The local directory to refresh.
		const wxArrayString * sWC_Specific_Paths = NULL);		//< [in] An array of paths to WF item, that need to be specifically updated.

	bool vvVerbs::GetVersionString(vvContext&           pCtx,	//< [in] [out] Error and context info.
		wxString& sVersionNumber);								// [out] The version string for sglib
};


#endif
