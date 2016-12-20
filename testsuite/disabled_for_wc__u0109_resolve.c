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
 * @file u0109_resolve.c
 *
 * @details Unit tests for sg_resolve.c.
 *
 */


/*
**
** Includes
**
*/

#include <sg.h>
#include "unittests.h"


/*
**
** Defines
**
*/

/**
 * Uncommenting this define causes only the TEST_SETUP portion of
 * each test to be run.  This doesn't actually cause the resolve
 * code to be tested, however it can be a very handy way to setup
 * a bunch of repos in various conflicting states for manual testing.
 */
//#define SETUP_ONLY

/**
 * Uncommenting this define while also using SETUP_ONLY causes each
 * test to have a resolve.bat/resolve.sh written within it that contains
 * commands equivalent to the test's resolve plan.  This allows you to
 * automate resolving each test with the command line client instead
 * of this app.  This is a pretty good (though not foolproof) means of
 * testing the resolve command line interface.  It still fails on
 * resolve plans that rely on keeping track of changing resolve state
 * data such as new values or renamed/moved items.  Examples of this are
 * new working values being added and conflicting items being moved/renamed
 * before they are resolved (such as the second conflict to be resolved
 * in a cycle).  Currently RESOLVE_MODIFY_* steps are not written to
 * the scripts because they are difficult to replicate with commands; a
 * comment is written instead.
 */
//#define DUMP_RESOLVE

/**
 * Uncommenting this define causes only the tests whose name match
 * the pattern to be executed.  This is most often handy when working
 * on a single test (just specify its full name), but can also be useful
 * to run a subset of tests with similar names like "divergent_rename__*".
 * Pattern matching is implemented with SG_file_spec__match_pattern, so
 * standard filename globbing wildcards work.
 */
//#define TEST_PATTERN "divergent_edit__automerge*"

/**
 * Uncommenting this define causes all SETUP_UPDATE steps to be implemented
 * by checking out to a new working copy rather than by updating the
 * existing working copy.  In theory no other parts of the test should care
 * if the working copy is the same before and after the SETUP_UPDATE step,
 * so the test should run just fine and have the same results.  This was
 * implemented so that the resolve tests can still be run in the absence
 * of a functioning update verb.
 */
//#define UPDATE_BY_CHECKOUT

/*
 * These defines control how many of various types of resolve
 * objects can be declared in each test.
 */
#define MAX_EXPECT_VALUES    15
#define MAX_EXPECT_CHOICES   SG_RESOLVE__STATE__COUNT
#define MAX_EXPECT_ITEMS     10
#define MAX_EXPECT_RELATIONS 10
#define MAX_SETUP_STEPS      25
#define MAX_RESOLVE_STEPS    10
#define MAX_VERIFY_STEPS     15

/*
 * If you're seeing platform-specific failures of this test, then read on for the
 * most likely cause!
 *
 * Testing that the timestamp associated with each resolve value is a bit tricky,
 * because the test can't know precisely what to expect the timestamp to be.
 * Instead, it gets the current time before and after the resolve call that creates
 * the value, and then tries to ensure that the timestamp associated with the value
 * falls within that range.
 *
 * This has a finicky problem - the resolve value's timestamp is associated with
 * a file on the file system, which means it's based on the resolution of the file
 * system's last modified time.  So if that resolution is low relative to the
 * resolution used to get the before/after time, then the value's timestamp will
 * be prior to the before time.  For example, if the file system only has one
 * second resolution and the before/after timer has 20 millisecond resolution, then
 * if the merge occurs at time 1234ms, the before/after times might be 1220 and 1240,
 * and the value's file-based time might come back as 1000 since the next possible
 * file-based time wouldn't be until 2000.
 *
 * On Windows, everything seems to work fine.  The file system time seems to have
 * the same resolution as the before/after timer.
 *
 * On Ubuntu, the file system time seems to have a slightly lower resolution.  Adding
 * a minor delay in between the before/after times makes it work fine.  Probably the
 * before/after time has a high resolution timer, and the file system uses a standard
 * ~15ms timer.
 *
 * On Mac, the file system time appears to only have one second resolution, while the
 * before/after timer is at least a standard ~15ms resolution, if not better.  Rather
 * than introduce enough delay to make it succeed (almost one second per test), these
 * checks are simply skipped on Mac.  If they work on the other platforms, then it's
 * pretty reasonable to assume that things are okay on the Mac as well.
 *
 * There is now a confirmed case of a Linux distro that fails here, behaving
 * basically like the Mac (file system timer only works in whole seconds).
 * http://veracity-scm.com/qa/questions/1665/multiple-test-failures-in-veracity-21-u0109_resolve
 * Given that, and the sheer variety of Linux distros and their customizability,
 * it seems pointless to try and distinguish Linux cases that work from ones
 * that don't.  Instead we'll just skip this check on Linux as well.
 *
 * Two defines control this check:
 * If TIMESTAMP_CHECK is defined, it means to do the check.  If it's not defined
 * then the check is skipped.  If TIMESTAMP_CHECK_DELAY is defined, then it
 * indicates the delay in milliseconds to add.
 */
#if defined(WINDOWS)
#define TIMESTAMP_CHECK
#endif
//#define TIMESTAMP_CHECK_DELAY 15


/*
**
** Types
**
*/

/**
 * Types of actions we can perform to setup a working directory.
 */
typedef enum u0109__test__setup_action
{
	SETUP_ACTION_ADD,      //< Add an item.
	                       //< szRepoPath: path of the new item
	                       //< eItemType: the type of item to add
	                       //< szName: name to assign to the new version of the item's content
	                       //< fModifyFunction: function to set the item's contents, for files/symlinks
	                       //< szValue: argument for fModifyFunction
	                       //< szOther: name to assign to the new item's GID
	SETUP_ACTION_DELETE,   //< Remove an item.
	                       //< szRepoPath: path of the item to delete
	SETUP_ACTION_MOVE,     //< Move an item to a new parent.
	                       //< szRepoPath: path of the item to move
	                       //< szValue: repo path of the item's new parent folder
	SETUP_ACTION_RENAME,   //< Rename an item.
	                       //< szRepoPath: path of the item to rename
	                       //< szValue: the item's new name
	SETUP_ACTION_MODIFY,   //< Modify the contents of an item.
	                       //< szRepoPath: path of the item to modify
	                       //< szName: name to assign to the new version of the item's content
	                       //< fModifyFunction: function to modify the item with
	                       //< szValue: argument for fModifyFunction
	SETUP_ACTION_REVERT,   //< Revert an item.
	                       //< szRepoPath: path of the item to revert (NULL to revert --all).
	SETUP_ACTION_COMMIT,   //< Commit the pending changes.
	                       //< szName: name to assign to the commit
	SETUP_ACTION_UPDATE,   //< Update the WD to a new baseline.
	                       //< szName: name of the commit to update to.
	SETUP_ACTION_MERGE,    //< Run a merge.
	                       //< szName: name of the commit to merge with (the "other" commit).
	                       //<         the current baseline will be used, so UPDATE to the right one first.
	SETUP_ACTION_CONTENTS, //< Runs a merge on previously named contents to generate a new content.
	                       //< szRepoPath: repo path to associate to the new merged content.
	                       //< szName: name to assign to the new merged content.
	                       //< szAncestor: name of the content to use as the common ancestor.
	                       //< szBaseline: name of the content to use as the baseline parent.
	                       //< szOther: name of the content to use as the other parent.
	SETUP_ACTION_COUNT     //< Number of elements in this enum.
}
u0109__test__setup_action;

/**
 * Type of a function used to generate/modify a file's contents.
 */
typedef void u0109__test__content_function(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	const SG_pathname* pPath,     //< [in] The full path of the file to generate/modify.
	const char*        szArgument //< [in] The argument specified by the caller, usage depends on each function.
	);

/**
 * A single step to take to setup a working directory.
 */
typedef struct u0109__test__setup_step
{
	// This is only used to check for valid instances in a "NULL-terminated" array.
	SG_bool                        bValid;

	// This data is used by all instances.
	u0109__test__setup_action      eAction;         //< The action to take.

	// This data's usage depends on eAction (see each action's comments).
	const char*                    szRepoPath;
	const char*                    szName;
	const char*                    szValue;
	SG_treenode_entry_type         eItemType;
	u0109__test__content_function* fContentFunction;
	const char*                    szAncestor;
	const char*                    szBaseline;
	const char*                    szOther;
}
u0109__test__setup_step;

/**
 * Types of resolve values that we might expect in a merged WD.
 */
typedef enum u0109__test__expect_value_type
{
	EXPECT_VALUE_CHANGESET, //< A value associated with a changeset.
	EXPECT_VALUE_MERGE,     //< A value created by merging two other values.
	EXPECT_VALUE_COUNT      //< Number of elements in this enum.
}
u0109__test__expect_value_type;

/**
 * Data about a single resolve value we expect to see in a WD's resolve data.
 *
 * Instances of this will be compared to actual resolve values read from the
 * WD's resolve data to verify that it meets expectations.
 */
typedef struct u0109__test__expect_value
{
	// This is only used to check for valid instances in a "NULL-terminated" array.
	SG_bool                        bValid;

	// This data is used by all instances.
	u0109__test__expect_value_type eType;                //< Type of the value.
	const char*                    szName;               //< Name assigned to the value by the test.
	SG_bool                        bLeaf;                //< Whether or not the value is expected to be a leaf.
	SG_bool                        bData;                //< The value's expected data, if boolean.
	SG_int64                       iData;                //< The value's expected data, if integral.
	const char*                    szData;               //< The value's expected data, if textual.

	// This data is only used for EXPECT_VALUE_CHANGESET instances.
	const char*                    szCommitName;         //< Name of the commit the value is associated with.
	                                                     //< NULL indicates the working copy.
	SG_bool                        bLive;                //< Whether or not the value is expected to be live.
	const char*                    szMWParentName;       //< Name of the value's expected parent value.
	                                                     //< Only used for working values on a mergeable choice.
	SG_bool                        bMWModified;          //< Whether or not the value is expected to be modified relative to its parent.
	                                                     //< Only used for working values on a mergeable choice.

	// This data is only used for EXPECT_VALUE_MERGE instances.
	const char*                    szMergeName;          //< Name of the merge that the value is associated with.
	const char*                    szBaselineParentName; //< Name of the value's expected baseline parent value.
	const char*                    szOtherParentName;    //< Name of the value's expected other parent value.
	SG_int32                       iMergeToolResult;     //< Expected result of running the merge tool.
	                                                     //< Should be SG_FILETOOL__RESULT__COUNT when not used.
	SG_bool                        bAutomatic;           //< Whether or not the value is expected to have been created automatically.
}
u0109__test__expect_value;

/**
 * Types of resolve values that we might expect in a merged WD.
 */
typedef enum u0109__test__expect_relation_type
{
	EXPECT_RELATION_COLLISION, //< The item is related because it is colliding with the current one.
	EXPECT_RELATION_CYCLE,     //< The item is related because it is involved in a cycle with the current one.
	EXPECT_RELATION_COUNT      //< Number of elements in this enum.
}
u0109__test__expect_relation_type;

/**
 * Data about a single relation that a choice has to another item.
 *
 * Instances of this will be compared to the actual list of related items
 * on a choice to verify that it meets expectations.
 */
typedef struct u0109__test__expect_relation
{
	// This is only used to check for valid instances in a "NULL-terminated" array.
	SG_bool                           bValid;

	// This data is used by all instances.
	u0109__test__expect_relation_type eType;  //< Type of the expected relationship.
	const char*                       szName; //< Name of the expected related item.
}
u0109__test__expect_relation;

/**
 * Data about a single resolve choice we expect to see in a WD's resolve data.
 *
 * Instances of this will be compared to actual resolve choices read from the
 * WD's resolve data to verify that it meets expectations.
 */
typedef struct u0109__test__expect_choice
{
	// This is only used to check for valid instances in a "NULL-terminated" array.
	SG_bool                          bValid;

	// This data is used by all instances.
	SG_resolve__state                eState;                              //< Expected state of the choice.
	SG_mrg_cset_entry_conflict_flags eConflictCauses;                     //< Expected conflict causes of the choice.
	SG_bool                          bCollisionCause;                     //< Expected collision cause of the choice.
	const char*                      szAcceptedValueLabel;                //< Label of the value expected to be the choice's accepted resolution.
	                                                                      //< NOTE: This is a LABEL, not a NAME.

	// This data is maintained by the test framework code during execution.
	SG_vhash*                        pValues;                             //< Maps value names to their labels.

	// This data is used by all instances.
	u0109__test__expect_value        aValues[MAX_EXPECT_VALUES+1u];       //< Values expected to be owned by the choice.
	                                                                      //< This is "NULL-terminated": the first instance with bValid == SG_FALSE is the end.
	                                                                      //< This works because static data that isn't explicitly initialized is set to 0 by the compiler.
	u0109__test__expect_relation     aRelations[MAX_EXPECT_RELATIONS+1u]; //< Names of expected items that are expected to be related to the choice.
	                                                                      //< This is "NULL-terminated": the first instance with bValid == SG_FALSE is the end.
	                                                                      //< This works because static data that isn't explicitly initialized is set to 0 by the compiler.
}
u0109__test__expect_choice;

/**
 * Data about a single resolve item we expect to see in a WD's resolve data.
 *
 * Instances of this will be compared to actual resolve items read from the
 * WD's resolve data to verify that it meets expectations.
 */
typedef struct u0109__test__expect_item
{
	// This is only used to check for valid instances in a "NULL-terminated" array.
	SG_bool                    bValid;

	// This data is used by all instances.
	const char*                szName;                          //< Name assigned to the item by the test.
	SG_treenode_entry_type     eType;                           //< Expected type of the item.
	const char*                szAncestorRepoPath;              //< Expected ancestor repo path of the item.
	const char*                szBaselineRepoPath;              //< Expected baseline repo path of the item.
	const char*                szOtherRepoPath;                 //< Expected other repo path of the item.
	SG_bool                    bResolved;                       //< Whether or not we expect the item to be resolved.

	// This data is maintained by the test framework code during execution.
	SG_bool                    bDeleted;                        //< Whether or not we expect the item's resolution to be deletion.

	// This data is used by all instances.
	u0109__test__expect_choice aChoices[MAX_EXPECT_CHOICES+1u]; //< Choices expected to be owned by the item.
	                                                            //< This is "NULL-terminated": the first instance with bValid == SG_FALSE is the end.
	                                                            //< This works because static data that isn't explicitly initialized is set to 0 by the compiler.
}
u0109__test__expect_item;

/**
 * Types of actions we can take to resolve conflicts during a test.
 */
typedef enum u0109__test__resolve_action
{
	RESOLVE_ACTION_ACCEPT, //< Accept a value.
	RESOLVE_ACTION_MERGE,  //< Merge two values to create a new value.
	RESOLVE_ACTION_DELETE, //< Delete a file from the working dir, similar to SETUP_ACTION_DELETE.
	RESOLVE_ACTION_MODIFY, //< Modify a file in the working dir, similar to SETUP_ACTION_MODIFY.
	RESOLVE_ACTION_COUNT   //< Number of elements in this enum.
}
u0109__test__resolve_action;

/**
 * A single step taken during a test to resolve conflicts.
 */
typedef struct u0109__test__resolve_step
{
	// This is only used to check for valid instances in a "NULL-terminated" array.
	SG_bool                        bValid;

	// This data is used by all instances.
	u0109__test__resolve_action    eAction;             //< Action to take.
	const char*                    szItemName;          //< Name of item to operate on.
	SG_resolve__state              eState;              //< ACCEPT/MERGE:  State of choice to operate on.
	                                                    //< MODIFY/DELETE: Not used.
	const char*                    szValueName;         //< ACCEPT/MODIFY: Name of the value to accept/modify.
	                                                    //< MERGE:         Name of the value to create.
	                                                    //< DELETE:        Repo path of the temporary file to delete, or NULL if deleting an item's file.
	const char*                    szContentName;       //< ACCEPT/MERGE:  Name of a new working value to create, or NULL to not create one.
	                                                    //< MODIFY:        Name to assign to the new content.
	                                                    //< DELETE:        Not used.
	const char*                    szContentData;       //< ACCEPT/MERGE:  Not used.
	                                                    //< MODIFY:        Data to pass to fContentFunction.
	                                                    //< DELETE:        Not used.

	// This data is only used by RESOLVE_ACTION_MERGE instances.
	const char*                    szBaselineValueName; //< Name of value to be the baseline parent.
	const char*                    szOtherValueName;    //< Name of value to be the other parent.
	const char*                    szMergeTool;         //< Name of the merge tool to use.
	SG_int32                       iMergeToolResult;    //< Expected result from the merge tool.
	const char*                    szMergedContents;    //< Name of the contents expected as the merge result.
	SG_bool                        bAccept;             //< Whether or not the merge should automatically accept a final leaf result.
	SG_bool                        bResolved;           //< Whether or not the merged value is expected to be accepted as the resolution.

	// This data is only used by RESOLVE_ACTION_MODIFY instances.
	u0109__test__content_function* fContentFunction;    //< Function to use to modify the file.

}
u0109__test__resolve_step;

/**
 * A single step taken to verify the state of the WD after resolution.
 */
typedef struct u0109__test__verify_step
{
	// This is only used to check for valid instances in a "NULL-terminated" array.
	SG_bool                bValid;

	// This data is used by all instances.
	const char*            szRepoPath;    //< Repo path of the item to verify.
	SG_bool                bExists;       //< True if the item must exist, false if it must not.
	SG_treenode_entry_type eType;         //< Type that the item must be.
	                                      //< If SG_TREENODEENTRY_TYPE__INVALID, type will not be verified.
	const char*            szContent;     //< Content that should appear in the item.
	                                      //< If NULL, the content will not be verified.
	                                      //< File: the name assigned to a content HID that is expected to be the file's content.
	                                      //< Symlink: the expected target of the symlink.
	const char*            szItemName;    //< Name of the item whose GID should be used to transform szRepoPath.
	                                      //< NULL if no transformation is necessary.
}
u0109__test__verify_step;

/**
 * Data used to execute a resolve test.
 */
typedef struct u0109__test
{
	// This data is used by all instances.
	const char*               szName;                              //< Name of the test.
	const char*               szStartName;                         //< Name of a prior test whose WD this test should start with.

	// This data is used by all instances.
	// These arrays are "NULL-terminated": the first instance with bValid == SG_FALSE is the end.
	// This works because static data that isn't explicitly initialized is set to 0 by the compiler.
	u0109__test__setup_step   aSetupPlan[MAX_SETUP_STEPS+1u];      //< Ordered array of steps used to setup a working directory.
	u0109__test__expect_item  aExpectedItems[MAX_EXPECT_ITEMS+1u]; //< Array of resolve items that are expected after WD setup.
	u0109__test__resolve_step aResolvePlan[MAX_RESOLVE_STEPS+1u];  //< Ordered array of steps used to resolve conflicts in the WD after setup.
	u0109__test__verify_step  aVerification[MAX_VERIFY_STEPS+1u];  //< Array of steps to take to verify the WD state after resolution.

	// This data is maintained by the test framework code during execution.
	const struct u0109__test* pStartTest;                          //< The test named by szStartName, if any.
	SG_string*                sRepo;                               //< Name of the repo used for the test.
	SG_pathname*              pRootPath;                           //< Root path that contains all the test's working copies.
	SG_vector*                pPaths;                              //< List of the test's working copies.
	SG_pathname*              pPath;                               //< The path of the test's current working copy.  Points into pPaths.
	SG_uint32                 uNextMergeName;                      //< Index of the next merge name to use in gaMergeNames.
	SG_vhash*                 pCommitsHids;                        //< Maps commit names to their HIDs.
	SG_vhash*                 pItemsGids;                          //< Maps item names to their GIDs.
	SG_vhash*                 pContentsHids;                       //< Maps content names to their HIDs.
	SG_vhash*                 pContentsRepoPaths;                  //< Maps content names to the repo paths they're associated with.
	SG_vhash*                 pContentsSizes;                      //< Maps content names to their sizes.
	SG_vhash*                 pMergesStartTimes;                   //< Maps merge names to their start times.
	SG_vhash*                 pMergesEndTimes;                     //< Maps merge names to their end times.
	SG_varray*                pStrings;                            //< Array of intermediate strings owned and used by the test or objects within it.
}
u0109__test;

/*
 * Macros that statically declare instances of u0109__test to run a single test.
 *
 * Used as follows: (must occur in the given order: SETUP, EXPECT, RESOLVE, VERIFY).
 *
 * TEST(name)
 *	TEST_SETUP
 *		up to MAX_SETUP_STEPS SETUP_* macros
 *	TEST_EXPECT
 *		up to MAX_EXPECT_ITEMS EXPECT_ITEM macros
 *	TEST_RESOLVE
 *		up to MAX_RESOLVE_STEPS RESOLVE_* macros
 *	TEST_VERIFY
 *		up to MAX_VERIFY_STEPS VERIFY_* macros
 * TEST_END
 */
#define TEST(szName) { szName, NULL,
#define TEST_SETUP {
#define TEST_EXPECT },{
#define TEST_RESOLVE },{
#define TEST_VERIFY },{
#define TEST_END }, NULL, NULL, NULL, NULL, NULL, 0u, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },

/**
 * Used instead of TEST(szName) to create a test that keeps going from
 * where a prior test ended.  This allows tests to be chained together
 * so that several merges in a row can be tested.
 *
 * szName      - The name for the test.
 * szStartName - The name of a previous test that the new test will start from.
 *               The new test will start with a copy of the previous test's WD
 *               and work in the same repo.  Make sure that the named test actually
 *               appears BEFORE this test in the list, to ensure that it's already
 *               been executed by the time this one starts.
 */
#define TEST_CONTINUE(szName, szStartName) { szName, szStartName,

/**
 * Statically declare no setup steps.
 * Use this if no setup is necesary.
 */
#define SETUP_NONE { SG_FALSE, SETUP_ACTION_COUNT, NULL, NULL, NULL, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL },

/**
 * Statically declares an instance of u0109__test__setup_step.
 * Use one of the other SETUP_* macros to correctly configure the instance for a particular task.
 */
#define SETUP_STEP(eAction, szRepoPath, szName, szValue, eItemType, fModifyFunction, szAncestor, szBaseline, szOther) { SG_TRUE, eAction, szRepoPath, szName, szValue, eItemType, fModifyFunction, szAncestor, szBaseline, szOther },

/**
 * Statically declare a setup step that adds an item to the WD.
 * Use one of the other SETUP_ADD_* macros to correctly configure the instance for a specific type of item.
 */
#define SETUP_ADD(eItemType, szRepoPath, szGidName, szHidName, fContentFunction, szContentFunctionArgument) SETUP_STEP(SETUP_ACTION_ADD, szRepoPath, szHidName, szContentFunctionArgument, eItemType, fContentFunction, NULL, NULL, szGidName)

/**
 * Statically declare a setup step that adds a file to the WD.
 *
 * szRepoPath                - The full repo path of the new file.
 * szItemName                - A name to use to refer to the file's GID later in the test.
 *                             This name can be used instead of a repo path in later SETUP_* steps.
 * szContentName             - A name to use to refer to the file's content HID later in the test.
 *                             May be NULL if the test won't refer back to the content.
 * fContentFunction          - A function to use to generate the file's contents.
 *                           - May be NULL to generate an empty file.
 *                           - See _content__* functions.
 * szContentFunctionArgument - A string to pass to fContentFunction as data.
 *                             Its usage depends on the exact function in question.
 */
#define SETUP_ADD_FILE(szRepoPath, szItemName, szContentName, fContentFunction, szContentFunctionArgument) SETUP_ADD(SG_TREENODEENTRY_TYPE_REGULAR_FILE, szRepoPath, szItemName, szContentName, fContentFunction, szContentFunctionArgument)

/**
 * Statically declare a setup step that adds a folder to the WD.
 *
 * szRepoPath - The full repo path of the new folder.
 * szItemName - A name to use to refer to the folder's GID later in the test.
 *              This name can be used instead of a repo path in later SETUP_* steps.
 */
#define SETUP_ADD_FOLDER(szRepoPath, szItemName) SETUP_ADD(SG_TREENODEENTRY_TYPE_DIRECTORY, szRepoPath, szItemName, NULL, NULL, NULL)

/**
 * Statically declare a setup step that adds a symlink to the WD.
 *
 * szRepoPath - The full repo path of the new symlink.
 * szItemName - A name to use to refer to the symlink's GID later in the test.
 *              This name can be used instead of a repo path in later SETUP_* steps.
 * szTarget   - The new symlink's target.
 */
#define SETUP_ADD_SYMLINK(szRepoPath, szItemName, szTarget) SETUP_ADD(SG_TREENODEENTRY_TYPE_SYMLINK, szRepoPath, szItemName, NULL, _content__symlink, szTarget)

/**
 * Statically declare a setup step that deletes an item from the WD.
 *
 * szRepoPath - The full repo path of the item to delete.
 */
#define SETUP_DELETE(szRepoPath) SETUP_STEP(SETUP_ACTION_DELETE, szRepoPath, NULL, NULL, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL)

/**
 * Statically declare a setup step that moves an item within the WD.
 *
 * szRepoPath          - The full repo path of the item to move.
 * szNewParentRepoPath - The repo path of the folder to move the item into.
 */
#define SETUP_MOVE(szRepoPath, szNewParentRepoPath) SETUP_STEP(SETUP_ACTION_MOVE, szRepoPath, NULL, szNewParentRepoPath, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL)

/**
 * Statically declare a setup step that renames an item in the WD.
 *
 * szRepoPath - The full repo path of the item to rename.
 * szNewName  - The new name to assign to the item.
 */
#define SETUP_RENAME(szRepoPath, szNewName) SETUP_STEP(SETUP_ACTION_RENAME, szRepoPath, NULL, szNewName, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL)

/**
 * Statically declare a setup step that modifies the contents of an item in the WD.
 * Use one of the other SETUP_MODIFY_* macros to correctly configure the instance for a specific type of item.
 */
#define SETUP_MODIFY(szRepoPath, szName, fModifyFunction, szModifyFunctionArgument) SETUP_STEP(SETUP_ACTION_MODIFY, szRepoPath, szName, szModifyFunctionArgument, SG_TREENODEENTRY_TYPE__INVALID, fModifyFunction, NULL, NULL, NULL)

/**
 * Statically declare a setup step that modifies a file in the WD.
 *
 * szRepoPath                - The full repo path of the file to modify.
 * szName                    - A name to use to refer to the file's new content HID later in the test.
 * fContentFunction          - A function to use to modify the file's contents.
 *                           - See _content__* functions.
 * szContentFunctionArgument - A string to pass to fContentFunction as data.
 *                             Its usage depends on the exact function in question.
 */
#define SETUP_MODIFY_FILE(szRepoPath, szName, fModifyFunction, szModifyFunctionArgument) SETUP_MODIFY(szRepoPath, szName, fModifyFunction, szModifyFunctionArgument)

/**
 * Statically declare a setup step that modifies a symlink in the WD.
 *
 * szRepoPath - The full repo path of the symlink to modify.
 * szTarget   - The symlink's new target.
 */
#define SETUP_MODIFY_SYMLINK(szRepoPath, szTarget) SETUP_MODIFY(szRepoPath, NULL, _content__symlink, szTarget)

/**
 * Statically declare a setup step that reverts an item in the WD.
 *
 * szRepoPath - The full repo path of the item to revert.
 *              If NULL then all items are reverted using --all.
 *              Using SETUP_REVERT_ALL is easier for this.
 */
#define SETUP_REVERT(szRepoPath) SETUP_STEP(SETUP_ACTION_REVERT, NULL, NULL, NULL, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL)

/**
 * Statically declare a setup step that reverts all pending items.
 */
#define SETUP_REVERT_ALL SETUP_REVERT(NULL)

/**
 * Statically declare a setup step that commits the WD.
 *
 * szName - A name to use to refer to the commit later in the test.
 *          Also used as the commit's message.
 */
#define SETUP_COMMIT(szName) SETUP_STEP(SETUP_ACTION_COMMIT, NULL, szName, NULL, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL)

/**
 * Statically declare a setup step that updates the WD to a new baseline commit.
 *
 * szName - The name of the commit to update to.
 */
#define SETUP_UPDATE(szName) SETUP_STEP(SETUP_ACTION_UPDATE, NULL, szName, NULL, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL)

/**
 * Statically declare a setup step that runs a merge in the WD.
 *
 * szName      - A name for the merge.
 * szOtherName - The name of the commit to merge with.
 *               The current baseline will be used as the baseline,
 *               so use SETUP_UPDATE to update to it first, if needed.
 */
#define SETUP_MERGE(szName, szOtherName) SETUP_STEP(SETUP_ACTION_MERGE, NULL, szName, szOtherName, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, NULL)

/**
 * Statically declare a setup step that runs a merge on already setup file contents.
 *
 * This step doesn't actually have any effect on the working directory.
 * It only exists to generate a named content so that you can expect it to exist later.
 * The named content is generated by merging two existing named contents (previously generated with SETUP_ADD_FILE or SETUP_MODIFY_FILE).
 * The resulting content can then be used to verify that a file that was merged contains the correct contents.
 *
 * For example, if you setup a divergent edit test, you'll end up with a merged file.
 * If the merge is successfuly, the file will contain contents that were generated by the merge.
 * Chances are good that the contents won't match the file's ancestor, baseline, or other versions.
 * Therefore, without using this step, there's nothing that you can use to verify the merged file's contents against.
 * Use this step to generate the expected contents of the merged file by merging the
 * file's expected baseline and other contents against its expected ancestor contents.
 * Then you can verify the merged file's contents against the contents generated by this step.
 *
 * szName         - The name to assign to the merged content.
 * szAncestorName - The name of already existing content to use as the merge's common ancestor.
 * szBaselineName - The name of already existing content to use as the merge's baseline parent.
 * szOtherName    - The name of already existing content to use as the merge's other parent.
 */
#define SETUP_CONTENTS(szName, szAncestorName, szBaselineName, szOtherName) SETUP_STEP(SETUP_ACTION_CONTENTS, NULL, szName, NULL, SG_TREENODEENTRY_TYPE__INVALID, NULL, szAncestorName, szBaselineName, szOtherName)

/**
 * Statically declare no expected items.
 * Use this instead of EXPECT_ITEM and EXPECT_ITEM_END if no items are expected.
 */
#define EXPECT_NONE { SG_FALSE, NULL, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL, NULL, SG_FALSE, SG_FALSE, { { SG_FALSE, SG_RESOLVE__STATE__COUNT, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO, SG_FALSE, NULL, NULL, { { SG_FALSE, EXPECT_VALUE_COUNT, NULL, SG_FALSE, SG_FALSE, 0, NULL, NULL, SG_FALSE, NULL, SG_FALSE, NULL, NULL, NULL, 0, SG_FALSE }, }, { { SG_FALSE, EXPECT_RELATION_COUNT, NULL }, } }, } },

/**
 * Statically declare an instance of u0109__test__expect_item that will match given expectations.
 *
 * szName             - A name to use to refer to the item later in the test.
 *                      May be NULL if the test won't refer back to the item.
 * eType              - The type of item expected (an SG_treenode_entry_type).
 * szAncestorRepoPath - The expected repo path of the item in the ancestor changeset.
 *                      Use NULL if the item didn't exist in the ancestor changeset.
 * szBaselineRepoPath - The expected repo path of the item in the baseline changeset.
 *                      Use NULL if the item didn't exist in the baseline changeset.
 * szOtherRepoPath    - The expected repo path of the item in the other changeset.
 *                      Use NULL if the item didn't exist in the other changeset.
 * bResolved          - Whether or not the item is currently expected to be resolved.
 *
 * TODO: Add a bDeleted flag once the system supports resolution via deletion and
 *       we need to add some tests for it.
 *
 * In between EXPECT_ITEM and EXPECT_ITEM_END you must use at least one and as
 * many as MAX_EXPECT_CHOICES EXPECT_CHOICE macros to declare the choices that
 * are expected on the item.
 */
#define EXPECT_ITEM(szName, eType, szAncestorRepoPath, szBaselineRepoPath, szOtherRepoPath, bResolved) { SG_TRUE, szName, eType, szAncestorRepoPath, szBaselineRepoPath, szOtherRepoPath, bResolved, SG_FALSE, {
#define EXPECT_ITEM_END }},

/**
 * Statically declare an instance of u0109__test__expect_choice that will match given expectations.
 *
 * eState          - The state of choice expected on the item.
 * eConflictCauses - The set of conflict flags expected to cause/prompt the choice.
 *                   A set of SG_mrg_cset_entry_conflict_flags flags.
 * bCollisionCause - Whether or not the choice is caused/prompted by a collision.
 * szAcceptedValue - If the choice is expected to be resolved, this is the label of its accepted value.
 *                   This should be NULL if the choice is expected to be unresolved.
 *                   Note: This is NOT a value NAME, but a value LABEL.
 *                   Use one of the ACCEPTED_VALUE_* macros, and see their comments for more info.
 *
 * In between EXPECT_CHOICE and EXPECT_CHOICE_END you must use at least one and as
 * many as MAX_EXPECT_VALUE EXPECT_*_VALUE_* macros to declare the values that
 * are expected on the choice.
 *
 * After the last EXPECT_*_VALUE_* macro, you must specify a block
 * of EXPECT_RELATED_* macros to declare the names of other items which are
 * expected to be related to this choice.  If the choice has no expected relations
 * then just use EXPECT_RELATED_NONE.
 */
#define EXPECT_CHOICE(eState, eConflictCauses, bCollisionCause, szAcceptedValue) { SG_TRUE, eState, eConflictCauses, bCollisionCause, szAcceptedValue, NULL, {
#define EXPECT_CHOICE_END }},

/*
 * A set of macros to use for the szAcceptedValue parameter of EXPECT_CHOICE.
 *
 * These are necessary because a value name as defined in the test cannot be used.
 * They won't work because at the time we're matching an expected choice to an actual choice,
 * the expected value names of the choice aren't yet known, and therefore cannot be translated
 * to their equivalent label.  Therefore, we must specify the label directly.  The values
 * of these macros is thus directly dependent upon the resolve code that assigns labels
 * to merged values.  If that format changes, these values must change as well.
 */
#define ACCEPTED_VALUE_NONE NULL
#define ACCEPTED_VALUE_ANCESTOR  "ancestor"
#define ACCEPTED_VALUE_BASELINE  "baseline"
#define ACCEPTED_VALUE_OTHER     "other"
#define ACCEPTED_VALUE_WORKING   "working"
#define ACCEPTED_VALUE_AUTOMERGE "automerge"
#define ACCEPTED_VALUE_MERGE     "merge"
//#define ACCEPTED_VALUE_MERGE2    "merge2"
//#define ACCEPTED_VALUE_MERGE3    "merge3"
//...

/**
 * Statically declare an instance of u0109__test__expect_value that will match given expectations.
 * Use one of the other EXPECT_*_VALUE macros to correctly configure the instance for a specific type of value.
 */
#define EXPECT_VALUE(eType, szName, bLeaf, bValue, iValue, szValue, szCommitName, bLive, szMWParentName, bMWModified, szMergeName, szBaselineParentName, szOtherParentName, iMergeToolResult, bAutomatic) { SG_TRUE, eType, szName, bLeaf, bValue, iValue, szValue, szCommitName, bLive, szMWParentName, bMWModified, szMergeName, szBaselineParentName, szOtherParentName, iMergeToolResult, bAutomatic },

/**
 * Statically declare a value that matches a changeset-based value with boolean data.
 *
 * szName       - A name to use to refer to the value later in the test.
 *                May be NULL if the test won't refer back to the value.
 * bValue       - The data that the value is expected to have.
 * szCommitName - The name of the commit that the value is expected to be associated with.
 * bLeaf        - Whether or not the value is expected to be a leaf.
 */
#define EXPECT_CHANGESET_VALUE__BOOL(szName, bValue, szCommitName, bLeaf) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, bValue, 0, NULL, szCommitName, SG_FALSE, NULL, SG_FALSE, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a changeset-based value with integral data.
 *
 * szName       - A name to use to refer to the value later in the test.
 *                May be NULL if the test won't refer back to the value.
 * iValue       - The data that the value is expected to have.
 * szCommitName - The name of the commit that the value is expected to be associated with.
 * bLeaf        - Whether or not the value is expected to be a leaf.
 */
#define EXPECT_CHANGESET_VALUE__INT(szName, iValue, szCommitName, bLeaf) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, SG_FALSE, iValue, NULL, szCommitName, SG_FALSE, NULL, SG_FALSE, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a changeset-based value with textual data.
 *
 * szName       - A name to use to refer to the value later in the test.
 *                May be NULL if the test won't refer back to the value.
 * szValue      - The data that the value is expected to have.
 * szCommitName - The name of the commit that the value is expected to be associated with.
 * bLeaf        - Whether or not the value is expected to be a leaf.
 */
#define EXPECT_CHANGESET_VALUE__SZ(szName, szValue, szCommitName, bLeaf) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, SG_FALSE, 0, szValue, szCommitName, SG_FALSE, NULL, SG_FALSE, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a working value with boolean data
 * on a non-mergeable choice.
 *
 * szName - A name to use to refer to the value later in the test.
 *          May be NULL if the test won't refer back to the value.
 * bValue - The data that the value is expected to have.
 * bLeaf  - Whether or not the value is expected to be a leaf.
 */
#define EXPECT_WORKING_VALUE__BOOL(szName, bValue, bLeaf) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, bValue, 0, NULL, NULL, SG_TRUE, NULL, SG_FALSE, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a working value with integer data
 * on a non-mergeable choice.
 *
 * szName - A name to use to refer to the value later in the test.
 *          May be NULL if the test won't refer back to the value.
 * iValue - The data that the value is expected to have.
 * bLeaf  - Whether or not the value is expected to be a leaf.
 */
#define EXPECT_WORKING_VALUE__INT(szName, iValue, bLeaf) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, SG_FALSE, iValue, NULL, NULL, SG_TRUE, NULL, SG_FALSE, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a working value with string data
 * on a non-mergeable choice.
 *
 * szName  - A name to use to refer to the value later in the test.
 *           May be NULL if the test won't refer back to the value.
 * szValue - The data that the value is expected to have.
 * bLeaf   - Whether or not the value is expected to be a leaf.
 *
 * In the case of an SG_RESOLVE__STATE__NAME choice, the value associated with the working
 * directory might be a temporary name assigned by merge.  This temp name cannot be entirely
 * predicted statically by the test, so specifying szValue here becomes tricky.  To handle
 * this, specify something like "filename~P~g~".  Replace the "filename" part with the
 * actual filename expected.  Replace the "P" with the shorthand for the parent commit that
 * the value is expected to be associated with.  Use "A" for the ancestor, "L0" for the
 * baseline, and L1/L2/etc for the "other" parents.  Make sure the expected value ends with
 * "~g~".  The test code can then take that value and translate it into the full value to
 * expect and match against.
 *
 * In the case of an SG_RESOLVE__STATE__LOCATION choice, the actual value is the GID of
 * the item's parent.  However, since the test can't know the GID ahead of time, there are
 * two ways to specify the parent as szValue.  Either specify the full repo path of the
 * expected parent, or specify the name of another item declared in the test that is
 * expected to be the parent.
 */
#define EXPECT_WORKING_VALUE__SZ(szName, szValue, bLeaf) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, SG_FALSE, 0, szValue, NULL, SG_TRUE, NULL, SG_FALSE, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a working value with boolean data
 * on a mergeable choice.
 *
 * szName    - A name to use to refer to the value later in the test.
 *             May be NULL if the test won't refer back to the value.
 * bValue    - The data that the value is expected to have.
 * bLeaf     - Whether or not the value is expected to be a leaf.
 * bLive     - Whether or not the value is expected to be associated with the live working copy.
 * szParent  - The name of the value's expected parent value.
 * bModified - Whether or not the value is expected to be modified relative to its parent.
 */
#define EXPECT_MERGEABLE_WORKING_VALUE__BOOL(szName, bValue, bLeaf, bLive, szParent, bModified) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, bValue, 0, NULL, NULL, bLive, szParent, bModified, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a working value with integer data
 * on a mergeable choice.
 *
 * szName    - A name to use to refer to the value later in the test.
 *             May be NULL if the test won't refer back to the value.
 * iValue    - The data that the value is expected to have.
 * bLeaf     - Whether or not the value is expected to be a leaf.
 * bLive     - Whether or not the value is expected to be associated with the live working copy.
 * szParent  - The name of the value's expected parent value.
 * bModified - Whether or not the value is expected to be modified relative to its parent.
 */
#define EXPECT_MERGEABLE_WORKING_VALUE__INT(szName, iValue, bLeaf, bLive, szParent, bModified) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, SG_FALSE, iValue, NULL, NULL, bLive, szParent, bModified, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a working value with string data
 * on a mergeable choice.
 *
 * szName    - A name to use to refer to the value later in the test.
 *             May be NULL if the test won't refer back to the value.
 * szValue   - The data that the value is expected to have.
 * bLeaf     - Whether or not the value is expected to be a leaf.
 * bLive     - Whether or not the value is expected to be associated with the live working copy.
 * szParent  - The name of the value's expected parent value.
 * bModified - Whether or not the value is expected to be modified relative to its parent.
 */
#define EXPECT_MERGEABLE_WORKING_VALUE__SZ(szName, szValue, bLeaf, bLive, szParent, bModified) EXPECT_VALUE(EXPECT_VALUE_CHANGESET, szName, bLeaf, SG_FALSE, 0, szValue, NULL, bLive, szParent, bModified, NULL, NULL, NULL, SG_FILETOOL__RESULT__COUNT, SG_FALSE)

/**
 * Statically declare a value that matches a merged value.
 *
 * szName               - A name to use to refer to the value later in the test.
 *                        May be NULL if the test won't refer back to the value.
 * szValue                The data that the value is expected to have.
 *                        Usually the name of contents generated by SETUP_CONTENTS.
 * szMergeName          - The name of the merge that the value is associated with.
 * szBaselineParentName - The name of the value that is expected to be the value's baseline parent.
 * szOtherParentName    - The name of the value that is expected to be the value's other parent.
 * iMergeToolResult     - The expected result of running the default merge tool to generate the merged value.
 * bLeaf                - Whether or not the value is expected to be a leaf.
 */
#define EXPECT_MERGE_VALUE(szName, szValue, szMergeName, szBaselineParentName, szOtherParentName, iMergeToolResult, bLeaf) EXPECT_VALUE(EXPECT_VALUE_MERGE, szName, bLeaf, SG_FALSE, 0, szValue, NULL, SG_FALSE, NULL, SG_FALSE, szMergeName, szBaselineParentName, szOtherParentName, iMergeToolResult, SG_TRUE)

/**
 * Statically declare an instance of u0109__test__expect_relation that will match given expectations.
 *
 * EXPECT_RELATED_BEGIN may only appear directly after the last EXPECT_*_VALUE_*
 * macro within an EXPECT_CHOICE block.  Between the EXPECT_RELATED_BEGIN and its
 * corresponding EXPECT_RELATED_END, between one and MAX_RELATED_ITEMS
 * EXPECT_RELATED_* macros may be used to declare the names of other items that are
 * expected to be related to the current one.
 */
#define EXPECT_RELATED_BEGIN }, {
#define EXPECT_RELATED_END

/**
 * Statically declare that a choice has no expected relations.
 *
 * Use this INSTEAD OF EXPECT_RELATED_BEGIN and EXPECT_RELATED_END.
 */
#define EXPECT_RELATED_NONE }, { { SG_FALSE, EXPECT_RELATION_COUNT, NULL },

/**
 * Statically declare the name of an item that's related to the current choice.
 * It's usually easier to use one of the more specific macros rather than this
 * general one.
 */
#define EXPECT_RELATED(eType, szItemName) { SG_TRUE, eType, szItemName },

/**
 * Statically declare the name of an item that is involved in the collision
 * that caused the current choice.
 *
 * szItemName - The name of the item that is involved in the collision.
 */
#define EXPECT_RELATED_COLLISION(szItemName) EXPECT_RELATED(EXPECT_RELATION_COLLISION, szItemName)

/**
 * Statically declare the name of an item that is involved in the cycle
 * that caused the current choice.
 *
 * szItemName - The name of the item that is involved in the cycle.
 */
#define EXPECT_RELATED_CYCLE(szItemName) EXPECT_RELATED(EXPECT_RELATION_CYCLE, szItemName)

/**
 * Statically declare no resolve steps.
 * Use this instead of RESOLVE_* to not perform any resolve actions.
 */
#define RESOLVE_NONE { SG_FALSE, RESOLVE_ACTION_COUNT, NULL, SG_RESOLVE__STATE__COUNT, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, SG_FALSE, SG_FALSE, NULL },

/**
 * Statically declare an instance of u0109__test__resolve_step that will perform a resolve action.
 * Use one of the other RESOLVE_* macros to correctly configure the instance for a specific action.
 */
#define RESOLVE_STEP(eAction, szItemName, eState, szValueName, szContentName, szContentData, szBaselineValueName, szOtherValueName, szMergeTool, iMergeToolResult, szMergedContents, bAccept, bResolved, fContentFunction) { SG_TRUE, eAction, szItemName, eState, szValueName, szContentName, szContentData, szBaselineValueName, szOtherValueName, szMergeTool, iMergeToolResult, szMergedContents, bAccept, bResolved, fContentFunction },

/**
 * Statically declare a resolve action that matches a value.
 *
 * szItemName        - The name of the item to accept a value on.
 * eState            - The state on the item to accept a value for.
 * szValueName       - The name of the value to accept for the state on the item.
 * szWorkingValue    - Name of a new expected saved working value to create.
 *                     May be NULL if no new expected saved working value is needed.
 *                     This is only necessary if the current live working value contains edits and is NOT the one being accepted.
 */
#define RESOLVE_ACCEPT(szItemName, eState, szValueName, szWorkingValue) RESOLVE_STEP(RESOLVE_ACTION_ACCEPT, szItemName, eState, szValueName, szWorkingValue, NULL, NULL, NULL, NULL, 0, NULL, SG_FALSE, SG_FALSE, NULL)

/**
 * Statically declare a resolve action that merges two values to create another value.
 *
 * szItemName          - The name of the item to merge values on.
 * eState              - The state on the item to merge values for.
 * szBaselineValueName - The name of the value to use as the new value's baseline parent.
 * szOtherValueName    - The name of the value to use as the new value's other parent.
 * szNewValueName      - The name to assign to the newly created value.
 * bOverwrite          - Whether or not the new value is allowed to replace an existing one.
 *                       If false, szNewValueName must not match any existing values.
 * szMergeTool         - The name of the merge tool to use.
 *                       May be NULL to use the default tool, if there is one.
 * iMergeToolResult    - Expected result of running the merge tool.
 * szMergedContents    - Name of the contents expected to be output from the merge.
 * bAccept             - Whether or not merge should automatically accept a final leaf result.
 * bResolved           - Whether or not the new merge value is expected to be automatically accepted as the resolution.
 * szWorkingValue      - Name of a new expected live working value to create.
 *                       May be NULL if no new expected live working value is needed.
 *                       This is only necessary if the current live working value is one of the values being merged.
 */
#define RESOLVE_MERGE(szItemName, eState, szBaselineValueName, szOtherValueName, szNewValueName, bOverwrite, szMergeTool, iMergeToolResult, szMergedContents, bAccept, bResolved, szWorkingValue) RESOLVE_STEP(RESOLVE_ACTION_MERGE, szItemName, eState, szNewValueName, szWorkingValue, NULL, szBaselineValueName, szOtherValueName, szMergeTool, iMergeToolResult, szMergedContents, bAccept, bResolved, NULL)

/**
 * Statically declare a resolve action that deletes a file from the WD.
 *
 * szItemName - The name of the item to delete from the WD.
 */
#define RESOLVE_DELETE(szItemName) RESOLVE_STEP(RESOLVE_ACTION_DELETE, szItemName, SG_RESOLVE__STATE__COUNT, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, SG_FALSE, SG_FALSE, NULL)

/**
 * Variation of RESOLVE_DELETE that builds a temporary path based on an item's GID.
 *
 * This macro is necesary when you want to delete an item whose name is temporary
 * as assigned by merge.  Such temporary names include the partial GID of the item.
 * This works identically to using EXPECT_CHANGESET_VALUE__SZ when the value is the
 * name of a temporary file.  See the documentation there for more information.
 *
 * szRepoPath - Repo path of the file to delete.
 * szItemName - The name of the item whose partial GID should be substituted into szRepoPath.
 */
#define RESOLVE_DELETE_TEMP_FILE(szRepoPath, szItemName) RESOLVE_STEP(RESOLVE_ACTION_DELETE, szItemName, SG_RESOLVE__STATE__COUNT, szRepoPath, NULL, NULL, NULL, NULL, NULL, 0, NULL, SG_FALSE, SG_FALSE, NULL)

/**
 * Statically declare a resolve action that modifies a file in the WD.
 *
 * szItemName       - The name of the item to modify in the WD.
 * szValueName      - The name of the working value on the item's CONTENTS conflict to update with the file's new contents.
 *                    If this is NULL, no value will be updated.
 *                    If this is non-NULL, szContentName must also be non-NULL.
 * szContentName    - The name of the file's new contents.
 *                    If this is NULL, the file's new contents will be inaccessible by name.
 *                    If this is NULL, szValueName must also be NULL.
 * fContentFunction - The content function to use to modify the file.
 * szContentData    - The data to pass to fContentFunction.
 */
#define RESOLVE_MODIFY_FILE(szItemName, szValueName, szContentName, fContentFunction, szContentData) RESOLVE_STEP(RESOLVE_ACTION_MODIFY, szItemName, SG_RESOLVE__STATE__COUNT, szValueName, szContentName, szContentData, NULL, NULL, NULL, 0, NULL, SG_FALSE, SG_FALSE, fContentFunction)

// TODO: Add actions to re-resolve an already resolved choice and add tests that do this.
//       Some tests should choose different values, and others should choose the same value again.

/**
 * Statically declare no verify steps.
 * Use this instead of VERIFY_* to not perform any verification.
 */
#define VERIFY_NONE { SG_FALSE, NULL, SG_FALSE, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL },

/**
 * Statically declare an instance of u0109__test__verify_step that will perform a verification action.
 * Use one of the other VERIFY_* macros to correctly configure the instance for a specific action.
 */
#define VERIFY_STEP(szRepoPath, bExists, eType, szContent, szItemName) { SG_TRUE, szRepoPath, bExists, eType, szContent, szItemName },

/**
 * Verify that an item exists in the file system at a specific path.
 * Use VERIFY_FILE, VERIFY_FOLDER, or VERIFY_SYMLINK to also verify its type.
 *
 * szRepoPath - The repo path where an item must exist.
 */
#define VERIFY_EXISTS(szRepoPath) VERIFY_STEP(szRepoPath, SG_TRUE, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL)

/**
 * A variation of VERIFY_FILE that builds a temporary filename based on an item's GID.
 *
 * This macro is necesary when you want to verify the existence of an item
 * whose name is temporary as assigned by merge.  Such temporary names include
 * the partial GID of an item.  This works identically to using
 * EXPECT_CHANGESET_VALUE__SZ when the value is the name of a temporary file.
 * See the documentation there for more information.
 *
 * szRepoPath and szContentName are identical to VERIFY_FILE.
 * szItemName - The name of the item whose partial GID should be substituted into szRepoPath.
 */
#define VERIFY_TEMP_FILE(szRepoPath, szItemName, szContentName) VERIFY_STEP(szRepoPath, SG_TRUE, SG_TREENODEENTRY_TYPE_REGULAR_FILE, szContentName, szItemName)

/**
 * Verify that a file exists at a specific path.
 *
 * szRepoPath    - The repo path where a file must exist.
 * szContentName - The name of the content expected to appear in the file.
 *                 May be NULL to not verify the content.
 */
#define VERIFY_FILE(szRepoPath, szContentName) VERIFY_TEMP_FILE(szRepoPath, NULL, szContentName)

/**
 * A variation of VERIFY_FOLDER that builds a temporary filename based on an item's GID.
 *
 * This macro is necesary when you want to verify the existence of an item
 * whose name is temporary as assigned by merge.  Such temporary names include
 * the partial GID of an item.  This works identically to using
 * EXPECT_CHANGESET_VALUE__SZ when the value is the name of a temporary file.
 * See the documentation there for more information.
 *
 * szRepoPath and szContentName are identical to VERIFY_FILE.
 * szItemName - The name of the item whose partial GID should be substituted into szRepoPath.
 */
#define VERIFY_TEMP_FOLDER(szRepoPath, szItemName) VERIFY_STEP(szRepoPath, SG_TRUE, SG_TREENODEENTRY_TYPE_DIRECTORY, NULL, szItemName)

/**
 * Verify that a folder exists at a specific path.
 *
 * szRepoPath - The repo path where a folder must exist.
 */
#define VERIFY_FOLDER(szRepoPath) VERIFY_TEMP_FOLDER(szRepoPath, NULL)

/**
 * A variation of VERIFY_SYMLINK that builds a temporary filename based on an item's GID.
 *
 * This macro is necesary when you want to verify the existence of an item
 * whose name is temporary as assigned by merge.  Such temporary names include
 * the partial GID of an item.  This works identically to using
 * EXPECT_CHANGESET_VALUE__SZ when the value is the name of a temporary file.
 * See the documentation there for more information.
 *
 * szRepoPath and szContentName are identical to VERIFY_FILE.
 * szItemName - The name of the item whose partial GID should be substituted into szRepoPath.
 * szTarget   - The symlink's expected target.
 *              May be NULL to not verify the target.
 */
#define VERIFY_TEMP_SYMLINK(szRepoPath, szItemName, szTarget) VERIFY_STEP(szRepoPath, SG_TRUE, SG_TREENODEENTRY_TYPE_SYMLINK, szTarget, szItemName)

/**
 * Verify that a symlink exists at a specific path.
 *
 * szRepoPath - The repo path where a symlink must exist.
 * szTarget   - The symlink's expected target.
 *              May be NULL to not verify the target.
 */
#define VERIFY_SYMLINK(szRepoPath, szTarget) VERIFY_TEMP_SYMLINK(szRepoPath, NULL, szTarget)

/**
 * A variation of VERIFY_GONE that builds a temporary filename based on an item's GID.
 *
 * This macro is necesary when you want to verify the non-existence of an item
 * whose name is temporary as assigned by merge.  Such temporary names include
 * the partial GID of an item.  This works identically to using
 * EXPECT_CHANGESET_VALUE__SZ when the value is the name of a temporary file.
 * See the documentation there for more information.
 *
 * szRepoPath is identical to VERIFY_GONE.
 * szItemName - The name of the item whose partial GID should be substituted into szRepoPath.
 */
#define VERIFY_TEMP_GONE(szRepoPath, szItemName) VERIFY_STEP(szRepoPath, SG_FALSE, SG_TREENODEENTRY_TYPE__INVALID, NULL, szItemName)

/**
 * Verify that no item exists at a specific path.
 *
 * szRepoPath - The repo path where an item must not exist.
 */
#define VERIFY_GONE(szRepoPath) VERIFY_TEMP_GONE(szRepoPath, NULL)

/**
 * Verify whether or not saved choice data exists in the pendingtree.
 *
 * bExists - Whether or not saved choice data should exist.
 */
#define VERIFY_CHOICES(bExists) VERIFY_STEP(NULL, bExists, SG_TREENODEENTRY_TYPE__INVALID, NULL, NULL)


/*
**
** Forward Declarations - Content Functions
**
*/

/**
 * A content function that appends a single line to a file.
 */
static void _content__append_line(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	const SG_pathname* pPath, //< [in] The path of the file to append a line to.
	const char*        szLine //< [in] The text of the line to append to the file.
	);

/**
 * A content function that appends numbered lines to a file.
 */
static void _content__append_numbered_lines(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const SG_pathname* pPath,  //< [in] The path of the file to append lines to.
	const char*        szCount //< [in] Number of lines to add as a string (i.e. "10").
	);

/**
 * A content function that appends NULL bytes to a file.
 */
static void _content__append_nulls(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const SG_pathname* pPath,  //< [in] The path of the file to append nulls to.
	const char*        szCount //< [in] Number of NULL bytes to append to the file.
	);

/**
 * A content function that replaces the first line of a file with a given string.
 */
static void _content__replace_first_line(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	const SG_pathname* pPath,        //< [in] The path of the file to replace the first line of.
	const char*        szReplacement //< [in] The string to replace the first line of the file with.
	);

/**
 * A content function that generates a symlink.
 */
static void _content__symlink(
	SG_context*        pCtx,    //< [in] [out] Error and context info.
	const SG_pathname* pPath,   //< [in] The path to generate a symlink at.
	const char*        szTarget //< [in] The new symlink's target.
	);


/*
**
** Globals
**
*/

// TODO: Would be nice to have a way to verify that no unexpected files
//       exist after each test's VERIFY step.  Maybe iterate through the
//       existing files and make sure they all line up with a repo path
//       specified in a VERIFY_* declaration?

/**
 * Global array declaring all of the tests that will be executed.
 * The test code simply iterates over this array and executes each declared test.
 * The tests are split out into separate files to make them easier to manage.
 */
static u0109__test u0109__gaTests[] =
{
#include "u0109_resolve__no_conflicts.h"
#include "u0109_resolve__divergent_rename.h"
#include "u0109_resolve__divergent_move.h"
#include "u0109_resolve__divergent_edit.h"
#include "u0109_resolve__delete_vs_rename.h"
#include "u0109_resolve__delete_vs_move.h"
#include "u0109_resolve__delete_vs_edit.h"
#include "u0109_resolve__orphan.h"
#include "u0109_resolve__cycle.h"
#include "u0109_resolve__collision_rename.h"
#include "u0109_resolve__collision_move.h"
#include "u0109_resolve__collision_add.h"
#include "u0109_resolve__pendingtree.h"
};

/**
 * This is a list of names that will be used to name merges generated
 * by a RESOLVE_MERGE command.
 */
static const char* gaMergeNames[] =
{
	"__merge_0",
	"__merge_1",
	"__merge_2",
	"__merge_3",
	"__merge_4",
	"__merge_5",
	"__merge_6",
	"__merge_7",
	"__merge_8",
	"__merge_9",
};

/**
 * These are used to build scripts when DUMP_RESOLVE is defined.
 */
#if defined(WINDOWS)
static const char* gszDumpFilename = "resolve.bat";
static const char* gszDumpComment  = "::";
#else
static const char* gszDumpFilename = "resolve.sh";
static const char* gszDumpComment  = "#";
#endif


/*
**
** Helpers - Content Functions
**
*/

static void _content__append_line(
	SG_context*        pCtx,
	const SG_pathname* pPath,
	const char*        szLine
	)
{
	SG_file*  pFile  = NULL;

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE | SG_FILE_WRONLY, 0644, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__seek_end(pCtx, pFile, NULL)  );
	VERIFY_ERR_CHECK(  SG_file__write__format(pCtx, pFile, "%s%s", szLine, SG_PLATFORM_NATIVE_EOL_STR)  );

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	return;
}

static void _content__append_numbered_lines(
	SG_context*        pCtx,
	const SG_pathname* pPath,
	const char*        szCount
	)
{
	SG_file*  pFile  = NULL;
	SG_uint32 uCount = 0u;
	SG_uint32 uIndex = 0u;

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE | SG_FILE_WRONLY, 0644, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__seek_end(pCtx, pFile, NULL)  );

	VERIFY_ERR_CHECK(  SG_uint32__parse(pCtx, &uCount, szCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_file__write__format(pCtx, pFile, "%u%s", uIndex, SG_PLATFORM_NATIVE_EOL_STR)  );
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	return;
}

static void _content__append_nulls(
	SG_context*        pCtx,
	const SG_pathname* pPath,
	const char*        szCount
	)
{
	static const SG_byte aBuffer[] = { 0 };

	SG_file*  pFile  = NULL;
	SG_uint32 uCount = 0u;
	SG_uint32 uIndex = 0u;

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE | SG_FILE_WRONLY, 0644, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__seek_end(pCtx, pFile, NULL)  );

	VERIFY_ERR_CHECK(  SG_uint32__parse(pCtx, &uCount, szCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, 1u, aBuffer, NULL)  );
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	return;
}

static void _content__replace_first_line(
	SG_context*        pCtx,
	const SG_pathname* pPath,
	const char*        szReplacement
	)
{
	SG_string*  sContents   = NULL;
	const char* szContents  = NULL;
	const char* szNewline   = NULL;
	SG_file*    pFile       = NULL;

	VERIFY_ERR_CHECK(  SG_file__read_into_string(pCtx, pPath, &sContents)  );
	szContents = SG_string__sz(sContents);

	szNewline = strstr(szContents, SG_PLATFORM_NATIVE_EOL_STR);
	if (szNewline == NULL)
	{
		VERIFY_ERR_CHECK(  SG_string__set__sz(pCtx, sContents, szReplacement)  );
	}
	else
	{
		SG_uint32 uLineLength = (SG_uint32)(szNewline - szContents);

		VERIFY_ERR_CHECK(  SG_string__remove(pCtx, sContents, 0u, uLineLength)  );
		VERIFY_ERR_CHECK(  SG_string__insert__sz(pCtx, sContents, 0u, szReplacement)  );
	}

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_EXISTING | SG_FILE_WRONLY | SG_FILE_TRUNC, 0644, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__write__string(pCtx, pFile, sContents)  );
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
	SG_ASSERT(pFile == NULL);

fail:
	SG_STRING_NULLFREE(pCtx, sContents);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	return;
}

static void _content__symlink(
	SG_context*        pCtx,
	const SG_pathname* pPath,
	const char*        szTarget
	)
{
	SG_bool    bExists = SG_FALSE;
	SG_string* sTarget = NULL;

	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(szTarget);

	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
	if (bExists == SG_TRUE)
	{
		VERIFY_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
	}

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sTarget, szTarget)  );
	VERIFY_ERR_CHECK(  SG_fsobj__symlink(pCtx, sTarget, pPath)  );

fail:
	SG_STRING_NULLFREE(pCtx, sTarget);
	return;
}


/*
**
** Helpers - General
**
*/

/**
 * Converts a boolean to a string, usually for display.
 */
static const char* _bool__sz(
	SG_bool bValue //< [in] The boolean to convert.
	)
{
	return bValue == SG_FALSE ? "false" : "true";
}

/**
 * Hashes a file.
 */
static void _repo__hash_file__pathname(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	SG_repo*           pRepo, //< [in] The repo to hash the file with.
	const SG_pathname* pPath, //< [in] The path of the file to hash.
	char**             ppHid  //< [out] The HID of the file.
	)
{
	SG_file* pFile = NULL;
	char*    szHid = NULL;

	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(ppHid);

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY, 0644, &pFile)  );
	VERIFY_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx, pRepo, pFile, &szHid)  );

	*ppHid = szHid;
	szHid = NULL;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, szHid);
	return;
}

/**
 * Maps the friendly name of a commit to its HID.
 */
static void _test__map_name__commit_to_hid(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const u0109__test* pTest,  //< [in] The test that "owns" the commit name.
	const char*        szName, //< [in] The friendly commit name to map.
	const char**       ppHid   //< [out] The HID associated with the name.
	)
{
	const char* szHid = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppHid);

	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTest->pCommitsHids, szName, &szHid)  );
	VERIFYP_COND_FAIL("HID not found for commit name.", szHid != NULL || pTest->pStartTest != NULL, ("name(%s)", szName));
	if (szHid == NULL)
	{
		SG_ASSERT(pTest->pStartTest != NULL);
		VERIFY_ERR_CHECK(  _test__map_name__commit_to_hid(pCtx, pTest->pStartTest, szName, ppHid)  );
	}
	else
	{
		*ppHid = szHid;
		szHid = NULL;
	}

fail:
	return;
}

/**
 * Maps the friendly name of an item to its GID.
 */
static void _test__map_name__item_to_gid(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const u0109__test* pTest,  //< [in] The test that "owns" the item name.
	const char*        szName, //< [in] The friendly item name to map.
	const char**       ppGid   //< [out] The GID associated with the name.
	)
{
	const char* szGid = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppGid);

	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTest->pItemsGids, szName, &szGid)  );
	VERIFYP_COND_FAIL("GID not found for item name.", szGid != NULL || pTest->pStartTest != NULL, ("name(%s)", szName));
	if (szGid == NULL)
	{
		SG_ASSERT(pTest->pStartTest != NULL);
		VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pTest->pStartTest, szName, ppGid)  );
	}
	else
	{
		*ppGid = szGid;
		szGid = NULL;
	}

fail:
	return;
}

/**
 * Maps the friendly name of a content to its HID.
 */
static void _test__map_name__content_to_hid(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const u0109__test* pTest,  //< [in] The test that "owns" the content name.
	const char*        szName, //< [in] The friendly content name to map.
	const char**       ppHid   //< [out] The HID associated with the name.
	)
{
	const char* szHid = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppHid);

	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTest->pContentsHids, szName, &szHid)  );
	VERIFYP_COND_FAIL("HID not found for content name.", szHid != NULL || pTest->pStartTest != NULL, ("name(%s)", szName));
	if (szHid == NULL)
	{
		SG_ASSERT(pTest->pStartTest != NULL);
		VERIFY_ERR_CHECK(  _test__map_name__content_to_hid(pCtx, pTest->pStartTest, szName, ppHid)  );
	}
	else
	{
		*ppHid = szHid;
		szHid = NULL;
	}

fail:
	return;
}

/**
 * Maps the friendly name of a content to its repo path.
 */
static void _test__map_name__content_to_repo_path(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	const u0109__test* pTest,     //< [in] The test that "owns" the content name.
	const char*        szName,    //< [in] The friendly content name to map.
	const char**       ppRepoPath //< [out] The repo path associated with the name.
	)
{
	const char* szRepoPath = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppRepoPath);

	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTest->pContentsRepoPaths, szName, &szRepoPath)  );
	VERIFYP_COND_FAIL("Repo path not found for content name.", szRepoPath != NULL || pTest->pStartTest != NULL, ("name(%s)", szName));
	if (szRepoPath == NULL)
	{
		SG_ASSERT(pTest->pStartTest != NULL);
		VERIFY_ERR_CHECK(  _test__map_name__content_to_repo_path(pCtx, pTest->pStartTest, szName, ppRepoPath)  );
	}
	else
	{
		*ppRepoPath = szRepoPath;
		szRepoPath = NULL;
	}

fail:
	return;
}

/**
 * Maps the friendly name of a content to its size.
 */
static void _test__map_name__content_to_size(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const u0109__test* pTest,  //< [in] The test that "owns" the content name.
	const char*        szName, //< [in] The friendly content name to map.
	SG_uint64*         pSize   //< [out] The size associated with the name.
	)
{
	SG_int64 iSize = -1;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(pSize);

	VERIFY_ERR_CHECK(  SG_vhash__check__int64(pCtx, pTest->pContentsSizes, szName, &iSize)  );
	VERIFYP_COND_FAIL("Size not found for content name.", iSize != -1 || pTest->pStartTest != NULL, ("name(%s)", szName));
	if (iSize == -1)
	{
		SG_ASSERT(pTest->pStartTest != NULL);
		VERIFY_ERR_CHECK(  _test__map_name__content_to_size(pCtx, pTest->pStartTest, szName, pSize)  );
	}
	else
	{
		*pSize = (SG_uint64)iSize;
	}

fail:
	return;
}

/**
 * Maps the friendly name of a merge to its start time.
 */
static void _test__map_name__merge_to_start_time(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const u0109__test* pTest,  //< [in] The test that "owns" the merge name.
	const char*        szName, //< [in] The friendly merge name to map.
	SG_int64*          pTime   //< [out] The time associated with the name.
	)
{
	SG_int64 iTime = -1;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(pTime);

	VERIFY_ERR_CHECK(  SG_vhash__check__int64(pCtx, pTest->pMergesStartTimes, szName, &iTime)  );
	VERIFYP_COND_FAIL("Start time not found for merge name.", iTime != -1 || pTest->pStartTest != NULL, ("name(%s)", szName));
	if (iTime == -1)
	{
		SG_ASSERT(pTest->pStartTest != NULL);
		VERIFY_ERR_CHECK(  _test__map_name__merge_to_start_time(pCtx, pTest->pStartTest, szName, pTime)  );
	}
	else
	{
		*pTime = iTime;
	}

fail:
	return;
}

/**
 * Maps the friendly name of a merge to its end time.
 */
static void _test__map_name__merge_to_end_time(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const u0109__test* pTest,  //< [in] The test that "owns" the merge name.
	const char*        szName, //< [in] The friendly merge name to map.
	SG_int64*          pTime   //< [out] The time associated with the name.
	)
{
	SG_int64 iTime = -1;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(pTime);

	VERIFY_ERR_CHECK(  SG_vhash__check__int64(pCtx, pTest->pMergesEndTimes, szName, &iTime)  );
	VERIFYP_COND_FAIL("End time not found for merge name.", iTime != -1 || pTest->pStartTest != NULL, ("name(%s)", szName));
	if (iTime == -1)
	{
		SG_ASSERT(pTest->pStartTest != NULL);
		VERIFY_ERR_CHECK(  _test__map_name__merge_to_end_time(pCtx, pTest->pStartTest, szName, pTime)  );
	}
	else
	{
		*pTime = iTime;
	}

fail:
	return;
}

/**
 * Gets a choice from an expected item by state.
 */
static void _expect_item__check_choice__state(
	SG_context*                  pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_item*    pItem,   //< [in] The item to find a choice in.
	SG_resolve__state            eState,  //< [in] The state of the choice to find.
	u0109__test__expect_choice** ppChoice //< [out] The found choice, or NULL if it wasn't found.
	)
{
	u0109__test__expect_choice* pChoice = pItem->aChoices;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppChoice);

	while (pChoice->bValid != SG_FALSE)
	{
		if (pChoice->eState == eState)
		{
			break;
		}

		++pChoice;
	}

	*ppChoice = pChoice;
	pChoice = NULL;

fail:
	return;
}

/**
 * Gets a choice from an expected item by state.
 * Fails to verify if the choice isn't found.
 */
static void _expect_item__get_choice__state(
	SG_context*                  pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_item*    pItem,   //< [in] The item to find a choice in.
	SG_resolve__state            eState,  //< [in] The state of the choice to find.
	u0109__test__expect_choice** ppChoice //< [out] The found choice.
	)
{
	u0109__test__expect_choice* pChoice = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppChoice);

	VERIFY_ERR_CHECK(  _expect_item__check_choice__state(pCtx, pItem, eState, &pChoice)  );
	VERIFYP_COND_FAIL("Expected choice not found in expected item by state.", pChoice != NULL, ("ChoiceState(%d)", eState));

	*ppChoice = pChoice;
	pChoice = NULL;

fail:
	return;
}

/**
 * Builds an informational string about an expected item.
 * Intended for use in failure messages about an expected item.
 */
static void _expect_item__build_info_string(
	SG_context*               pCtx,  //< [in] [out] Error and context info.
	u0109__test__expect_item* pItem, //< [in] The expected item to build an info string about.
	SG_string**               ppInfo //< [out] The built info string.
	)
{
	SG_string* sInfo = NULL;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sInfo)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, sInfo, "ItemType(%d) ItemAncestorPath(%s) ItemBaselinePath(%s) ItemOtherPath(%s) ItemResolved(%s) ItemDeleted(%s)", pItem->eType, pItem->szAncestorRepoPath, pItem->szBaselineRepoPath, pItem->szOtherRepoPath, _bool__sz(pItem->bResolved), _bool__sz(pItem->bDeleted))  );

	*ppInfo = sInfo;
	sInfo = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * Adds a new value to a choice.
 * The caller must fill in the new value's data.
 * Fails to verify if the choice cannot hold any additional values.
 */
static void _expect_choice__add_value(
	SG_context*                 pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_choice* pChoice, //< [in] The choice to add a value to.
	u0109__test__expect_value** ppValue  //< [out] The added value.
	)
{
	u0109__test__expect_value* pValue = pChoice->aValues;
	SG_uint32                  uCount = 0u;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppValue);

	while (pValue->bValid != SG_FALSE)
	{
		++pValue;
		++uCount;
		if (uCount >= MAX_EXPECT_VALUES)
		{
			SG_ERR_THROW2(SG_ERR_COLLECTIONFULL, (pCtx, "Ran out of available values on a choice."));
		}
	}

	pValue->bValid = SG_TRUE;

	*ppValue = pValue;
	pValue = NULL;

fail:
	return;
}

/**
 * Gets the expected resolved state of a choice.
 */
static void _expect_choice__get_resolved(
	SG_context*                 pCtx,      //< [in] [out] Error and context info.
	u0109__test__expect_item*   pItem,     //< [in] The item that owns pChoice.
	u0109__test__expect_choice* pChoice,   //< [in] The choice to get expected resolved info about.
	SG_bool*                    pResolved, //< [out] The expected resolved status.
	SG_bool*                    pDeleted   //< [out] The expected deleted status.
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pResolved);

	if (pItem->bDeleted == SG_TRUE || pChoice->szAcceptedValueLabel != NULL)
	{
		*pResolved = SG_TRUE;
		if (pDeleted != NULL)
		{
			*pDeleted = pItem->bDeleted;
		}
	}
	else
	{
		*pResolved = SG_FALSE;
	}

fail:
	return;
}

/**
 * Builds an informational string about an expected choice.
 * Intended for use in failure messages about an expected choice.
 */
static void _expect_choice__build_info_string(
	SG_context*                 pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_item*   pItem,   //< [in] The expected item that owns the expected choice.
	                                     //<      May be NULL to not include information about the item.
	u0109__test__expect_choice* pChoice, //< [in] The expected choice to build an info string about.
	SG_string**                 ppInfo   //< [out] The built info string.
	)
{
	SG_string* sInfo      = NULL;
	SG_string* sItemInfo  = NULL;
	SG_bool    bMergeable = SG_FALSE;
	SG_bool    bResolved  = SG_FALSE;
	SG_bool    bDeleted   = SG_FALSE;

	if (pItem != NULL)
	{
		VERIFY_ERR_CHECK(  _expect_item__build_info_string(pCtx, pItem, &sItemInfo)  );
	}

	VERIFY_ERR_CHECK(  _expect_choice__get_resolved(pCtx, pItem, pChoice, &bResolved, &bDeleted)  );
	if (
		   pChoice->eState == SG_RESOLVE__STATE__CONTENTS
		&& pItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE
		)
	{
		bMergeable = SG_TRUE;
	}

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sInfo)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, sInfo, "%s ChoiceState(%d) ChoiceConflicts(%d) ChoiceCollision(%s) ChoiceMergeable(%s) ChoiceResolved(%s) ChoiceDeleted(%s) ChoiceAcceptedLabel(%s)", (sItemInfo == NULL ? "" : SG_string__sz(sItemInfo)), pChoice->eState, pChoice->eConflictCauses, _bool__sz(pChoice->bCollisionCause), _bool__sz(bMergeable), _bool__sz(bResolved), _bool__sz(bDeleted), pChoice->szAcceptedValueLabel)  );

	*ppInfo = sInfo;
	sInfo = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	SG_STRING_NULLFREE(pCtx, sItemInfo);
	return;
}

/**
 * Gets the live working value from an expected choice.
 */
static void _expect_choice__get_value__live_working(
	SG_context*                 pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_choice* pChoice, //< [in] The choice to find the working value in.
	u0109__test__expect_value** ppValue  //< [out] The live working value for the given choice.
	)
{
	u0109__test__expect_value* pCurrent = pChoice->aValues;
	u0109__test__expect_value* pValue   = NULL;
	SG_string*                 sChoice  = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppValue);

	while (pCurrent->bValid != SG_FALSE)
	{
		if (
			   pCurrent->eType        == EXPECT_VALUE_CHANGESET
			&& pCurrent->szCommitName == NULL
			&& pCurrent->bLive        == SG_TRUE
			)
		{
			pValue = pCurrent;
			break;
		}

		++pCurrent;
	}

	if (pValue == NULL)
	{
		VERIFY_ERR_CHECK(  _expect_choice__build_info_string(pCtx, NULL, pChoice, &sChoice)  );
		VERIFYP_COND("No working value found on conflict.", pValue != NULL, ("%s", SG_string__sz(sChoice)));
	}

	*ppValue = pValue;

fail:
	SG_STRING_NULLFREE(pCtx, sChoice);
	return;
}

/**
 * Gets a value from an expected choice by name.
 */
static void _expect_choice__check_value__name(
	SG_context*                 pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_choice* pChoice, //< [in] The choice to find a value in.
	const char*                 szName,  //< [in] The name of the value to find.
	u0109__test__expect_value** ppValue  //< [out] The found value, or NULL if it wasn't found.
	)
{
	u0109__test__expect_value* pValue = pChoice->aValues;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppValue);

	while (pValue->bValid != SG_FALSE)
	{
		if (
			   pValue->szName != NULL
			&& strcmp(pValue->szName, szName) == 0
			)
		{
			break;
		}

		++pValue;
	}

	*ppValue = pValue;
	pValue = NULL;

fail:
	return;
}

/**
 * Gets a value from an expected choice by name.
 * Fails to verify if the value isn't found.
 */
static void _expect_choice__get_value__name(
	SG_context*                 pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_choice* pChoice, //< [in] The choice to find a value in.
	const char*                 szName,  //< [in] The name of the value to find.
	u0109__test__expect_value** ppValue  //< [out] The found value.
	)
{
	u0109__test__expect_value* pValue = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppValue);

	VERIFY_ERR_CHECK(  _expect_choice__check_value__name(pCtx, pChoice, szName, &pValue)  );
	VERIFYP_COND_FAIL("Expected value not found in expected choice by name.", pValue != NULL, ("ChoiceState(%d) ValueName(%s)", pChoice->eState, szName));

	*ppValue = pValue;
	pValue = NULL;

fail:
	return;
}

/**
 * Gets the flags that are expected for a value.
 */
static void _expect_value__get_flags(
	SG_context*                 pCtx,    //< [in] [out] Error and context info.
	u0109__test__expect_item*   pItem,   //< [in] The expected item that owns the expected choice.
	u0109__test__expect_choice* pChoice, //< [in] The expected choice that owns the expected value.
	u0109__test__expect_value*  pValue,  //< [in] The expected value to get flags for.
	SG_uint32*                  pFlags,  //< [out] The flags for the expected value.
	SG_uint32*                  pMask    //< [out] Mask of flags that are known correct.
	                                     //<       Bits that are not set in the mask should not be verified against.
	)
{
	SG_uint32   uFlags  = 0u;
	SG_uint32   uMask   = ~0u;
	const char* szLabel = NULL;

	SG_UNUSED(pItem);

	if (pValue->eType == EXPECT_VALUE_CHANGESET)
	{
		uFlags |= SG_RESOLVE__VALUE__CHANGESET;
		uFlags |= SG_RESOLVE__VALUE__AUTOMATIC;
	}
	else if (pValue->eType == EXPECT_VALUE_MERGE)
	{
		uFlags |= SG_RESOLVE__VALUE__MERGE;
		if (pValue->bAutomatic == SG_TRUE)
		{
			uFlags |= SG_RESOLVE__VALUE__AUTOMATIC;
		}
	}

	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pChoice->pValues, pValue->szName, &szLabel)  );
	if (szLabel == NULL)
	{
		// The value's name isn't in the choice's mapping yet,
		// meaning this is the first time we've tried verifying this value.
		// Without the name => label mapping, we can't know whether or not
		// we expect this value to be the resolution/result of the choice.
		// So we need to remove this bit from the mask so we don't verify
		// against it this time.  Next time we verify this value, we'll find
		// it in the hash and will be able to verify this bit as well.
		uMask &= ~SG_RESOLVE__VALUE__RESULT;
	}
	else if (
		   pChoice->szAcceptedValueLabel != NULL
		&& SG_stricmp__null(pChoice->szAcceptedValueLabel, szLabel) == 0
		)
	{
		uFlags |= SG_RESOLVE__VALUE__RESULT;
	}

	if ((uMask & SG_RESOLVE__VALUE__AUTOMATIC) == 0)
	{
		// since OVERWRITABLE is based on AUTOMATIC,
		// we don't want to verify against OVERWRITABLE
		// if we're not verifying against AUTOMATIC
		uMask &= ~SG_RESOLVE__VALUE__OVERWRITABLE;
	}
	if ((uMask & SG_RESOLVE__VALUE__RESULT) == 0)
	{
		// since OVERWRITABLE is based on RESULT,
		// we don't want to verify against OVERWRITABLE
		// if we're not verifying against RESULT
		uMask &= ~SG_RESOLVE__VALUE__OVERWRITABLE;
	}
	else if ((uFlags & (SG_RESOLVE__VALUE__AUTOMATIC | SG_RESOLVE__VALUE__RESULT)) == 0)
	{
		uFlags |= SG_RESOLVE__VALUE__OVERWRITABLE;
	}

	if (pValue->bLeaf == SG_TRUE)
	{
		uFlags |= SG_RESOLVE__VALUE__LEAF;
	}

	if (pValue->eType == EXPECT_VALUE_CHANGESET && pValue->szCommitName == NULL)
	{
		if (pValue->bLive == SG_TRUE)
		{
			uFlags |= SG_RESOLVE__VALUE__LIVE;
		}
	}

	*pFlags = uFlags;
	*pMask  = uMask;

fail:
	return;
}

/**
 * Builds an informational string about an expected value.
 * Intended for use in failure messages about an expected value.
 */
static void _expect_value__build_info_string(
	SG_context*                 pCtx,     //< [in] [out] Error and context info.
	SG_resolve*                 pResolve, //< [in] The resolve that is being tested against.
	SG_resolve__item*           pRItem,   //< [in] The resolve item that corresponds to the expected item.
	u0109__test*                pTest,    //< [in] The test that owns the expected data.
	u0109__test__expect_item*   pItem,    //< [in] The expected item that owns the expected choice.
	u0109__test__expect_choice* pChoice,  //< [in] The expected choice that owns the expected value.
	u0109__test__expect_value*  pValue,   //< [in] The expected value to build an info string about.
	SG_string**                 ppInfo    //< [out] The built info string.
	)
{
	SG_string*              sInfo       = NULL;
	SG_string*              sChoiceInfo = NULL;
	const char*             szLabel     = NULL;
	const char*             szChangeset = NULL;
	const char*             szMWParent  = NULL;
	SG_int64                iTimeStart  = 0;
	SG_int64                iTimeEnd    = 0;
	const char*             szBaseline  = NULL;
	const char*             szOther     = NULL;
	SG_uint32               uFlags      = 0u;
	SG_uint32               uMask       = 0u;
	const char*             szData      = NULL;
	SG_uint64               uSize       = 0u;
	SG_string*              sData       = NULL;
	SG_pathname*            pData       = NULL;
	SG_pathname*            pParentPath = NULL;
	SG_string*              sParentPath = NULL;
	char*                   szParentGid = NULL;
	SG_int_to_string_buffer szDataBuffer;
	SG_int_to_string_buffer szSizeBuffer;
	SG_int_to_string_buffer szTimeStartBuffer;
	SG_int_to_string_buffer szTimeEndBuffer;

	VERIFY_ERR_CHECK(  _expect_choice__build_info_string(pCtx, pItem, pChoice, &sChoiceInfo)  );

	// all of the following data translation is basically
	// copied from _verify_expectations__value__match

	if (pValue->eType == EXPECT_VALUE_CHANGESET)
	{
		if (pValue->szCommitName == NULL)
		{
			szChangeset = SG_RESOLVE__HID__WORKING;
		}
		else
		{
			VERIFY_ERR_CHECK(  _test__map_name__commit_to_hid(pCtx, pTest, pValue->szCommitName, &szChangeset)  );
		}
	}

	if (pValue->szMWParentName != NULL)
	{
		VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pChoice->pValues, pValue->szMWParentName, &szMWParent)  );
	}

	if (pValue->szMergeName != NULL)
	{
		VERIFY_ERR_CHECK(  _test__map_name__merge_to_start_time(pCtx, pTest, pValue->szMergeName, &iTimeStart)  );
		VERIFY_ERR_CHECK(  _test__map_name__merge_to_end_time(pCtx, pTest, pValue->szMergeName, &iTimeEnd)  );
	}

	if (pValue->szBaselineParentName != NULL)
	{
		VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pChoice->pValues, pValue->szBaselineParentName, &szBaseline)  );
	}
	if (pValue->szOtherParentName != NULL)
	{
		VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pChoice->pValues, pValue->szOtherParentName, &szOther)  );
	}

	VERIFY_ERR_CHECK(  _expect_value__get_flags(pCtx, pItem, pChoice, pValue, &uFlags, &uMask)  );

	switch (pChoice->eState)
	{
	case SG_RESOLVE__STATE__EXISTENCE:
		{
			szData = pValue->szData;
			uSize = sizeof(SG_bool);
		}
		break;
	case SG_RESOLVE__STATE__NAME:
		{
			SG_uint32 uLength = 0u;

			szData = pValue->szData;
			uLength = SG_STRLEN(szData);

			if (
				   uLength >= 3u
				&& szData[uLength-3u] == '~'
				&& szData[uLength-2u] == 'g'
				&& szData[uLength-1u] == '~'
				)
			{
				const char* szGid  = NULL;
				VERIFY_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pRItem, &szGid)  );
				VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sData, szData)  );
				VERIFY_ERR_CHECK(  SG_string__insert__buf_len(pCtx, sData, uLength-1u, (const SG_byte*)(szGid+1u), 6u)  );
				szData = SG_string__sz(sData);
			}

			uSize = strlen(szData);
		}
		break;
	case SG_RESOLVE__STATE__LOCATION:
		{
			SG_pendingtree* pPendingTree = NULL;

			VERIFY_ERR_CHECK(  SG_resolve__get_pendingtree(pCtx, pResolve, &pPendingTree)  );

			szData = pValue->szData;
			if (*szData == '@')
			{
				uSize = strlen(szData);
				VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path2(pCtx, pPendingTree, pValue->szData, &pParentPath)  );
				VERIFY_ERR_CHECK(  SG_pendingtree__get_gid_from_local_path(pCtx, pPendingTree, pParentPath, &szParentGid)  );
				szData = szParentGid;
			}
			else
			{
				VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pTest, szData, &szData)  );
				VERIFY_ERR_CHECK(  SG_pendingtree__get_repo_path(pCtx, pPendingTree, szData, &sParentPath)  );
				uSize = strlen(SG_string__sz(sParentPath));
			}
		}
		break;
	case SG_RESOLVE__STATE__CONTENTS:
		switch (pItem->eType)
		{
		case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
			{
				VERIFY_ERR_CHECK(  _test__map_name__content_to_hid(pCtx, pTest, pValue->szData, &szData)  );
				VERIFY_ERR_CHECK(  _test__map_name__content_to_size(pCtx, pTest, pValue->szData, &uSize)  );
			}
			break;
		case SG_TREENODEENTRY_TYPE_SYMLINK:
			{
				VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, pValue->szData, &pData)  );
				szData = SG_pathname__sz(pData);
				uSize = strlen(szData);
			}
			break;
		default:
			{
				VERIFYP_COND("Unsupported item type.", SG_FALSE, ("ItemType(%d)", pItem->eType));
			}
			break;
		}
		break;
	default:
		{
			VERIFYP_COND("Unsupported choice state.", SG_FALSE, ("ChoiceState(%d)", pChoice->eState));
		}
		break;
	}

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sInfo)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, sInfo, "%s ValueName(%s) ValueLabel(%s) ValueFlags(%x) ValueFlagsMask(%x) ValueBoolData(%s) ValueIntData(%s) ValueStringData(%s) ValueSize(%s) ValueCommit(%s) ValueChangeset(%s) ValueWorkingLive(%s) ValueMergeableWorkingParentName(%s) ValueMergeableWorkingParent(%s) ValueMergeableWorkingModified(%s) ValueMergeName(%s) ValueMergeTimeStart(%s) ValueMergeTimeEnd(%s) ValueBaselineName(%s) ValueBaseline(%s) ValueOtherName(%s) ValueOther(%s) ValueAutoMerge(%s) ValueMergeToolResult(%d)", SG_string__sz(sChoiceInfo), pValue->szName, szLabel, uFlags, uMask, _bool__sz(pValue->bData), SG_int64_to_sz(pValue->iData, szDataBuffer), szData, SG_uint64_to_sz(uSize, szSizeBuffer), pValue->szCommitName, szChangeset, _bool__sz(pValue->bLive), pValue->szMWParentName, szMWParent, _bool__sz(pValue->bMWModified), pValue->szMergeName, SG_int64_to_sz(iTimeStart, szTimeStartBuffer), SG_int64_to_sz(iTimeEnd, szTimeEndBuffer), pValue->szBaselineParentName, szBaseline, pValue->szOtherParentName, szOther, _bool__sz(pValue->bAutomatic), pValue->iMergeToolResult)  );

	*ppInfo = sInfo;
	sInfo = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	SG_STRING_NULLFREE(pCtx, sChoiceInfo);
	SG_STRING_NULLFREE(pCtx, sData);
	SG_PATHNAME_NULLFREE(pCtx, pData);
	SG_PATHNAME_NULLFREE(pCtx, pParentPath);
	SG_STRING_NULLFREE(pCtx, sParentPath);
	SG_NULLFREE(pCtx, szParentGid);
	return;
}

/**
 * Builds an informational string about an item.
 * Intended for use in failure messages about an item.
 */
static void _resolve_item__build_info_string(
	SG_context*       pCtx,  //< [in] [out] Error and context info.
	SG_resolve__item* pItem, //< [in] The item to build an info string about.
	SG_string**       ppInfo //< [out] The built info string.
	)
{
	SG_string*             sInfo          = NULL;
	SG_treenode_entry_type eType          = SG_TREENODEENTRY_TYPE__INVALID;
	const char*            szAncestorPath = NULL;
	const char*            szBaselinePath = NULL;
	const char*            szOtherPath    = NULL;
	const char*            szWorkingPath  = NULL;
	SG_bool                bResolved      = SG_FALSE;
	SG_bool                bDeleted       = SG_FALSE;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppInfo);

	VERIFY_ERR_CHECK(  SG_resolve__item__get_type(pCtx, pItem, &eType)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__ancestor(pCtx, pItem, &szAncestorPath)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__baseline(pCtx, pItem, &szBaselinePath)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__other(pCtx, pItem, 1u, &szOtherPath)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &szWorkingPath)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_resolved(pCtx, pItem, &bResolved, &bDeleted)  );

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sInfo)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, sInfo, "ItemType(%d) ItemAncestorPath(%s) ItemBaselinePath(%s) ItemOtherPath(%s) ItemWorkingPath(%s) ItemResolved(%s) ItemDeleted(%s)", eType, szAncestorPath, szBaselinePath, szOtherPath, szWorkingPath, _bool__sz(bResolved), _bool__sz(bDeleted))  );

	*ppInfo = sInfo;
	sInfo = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * Builds an informational string about a choice.
 * Intended for use in failure messages about a choice.
 */
static void _resolve_choice__build_info_string(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to build an info string about.
	SG_string**         ppInfo   //< [out] The built info string.
	)
{
	SG_string*                       sInfo           = NULL;
	SG_resolve__state                eState          = SG_RESOLVE__STATE__COUNT;
	SG_mrg_cset_entry_conflict_flags eConflictCauses = SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO;
	SG_bool                          bCollisionCause = SG_FALSE;
	SG_bool                          bMergeable      = SG_FALSE;
	SG_resolve__value*               pAcceptedValue  = NULL;
	const char*                      szAcceptedLabel = NULL;
	SG_resolve__item*                pItem           = NULL;
	SG_string*                       pItemInfo       = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppInfo);

	VERIFY_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_causes(pCtx, pChoice, &eConflictCauses, &bCollisionCause, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAcceptedValue)  );
	if (pAcceptedValue != NULL)
	{
		VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pAcceptedValue, &szAcceptedLabel)  );
	}
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );
	VERIFY_ERR_CHECK(  _resolve_item__build_info_string(pCtx, pItem, &pItemInfo)  );

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sInfo)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, sInfo, "%s ChoiceState(%d) ChoiceConflicts(%d) ChoiceCollision(%s) ChoiceMergeable(%s) ChoiceAcceptedLabel(%s)", SG_string__sz(pItemInfo), eState, eConflictCauses, _bool__sz(bCollisionCause), _bool__sz(bMergeable), szAcceptedLabel)  );

	*ppInfo = sInfo;
	sInfo = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	SG_STRING_NULLFREE(pCtx, pItemInfo);
	return;
}

/**
 * Builds an informational string about a value.
 * Intended for use in failure messages about a value.
 */
static void _resolve_value__build_info_string(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pValue, //< [in] The value to build an info string about.
	SG_string**        ppInfo  //< [out] The built info string.
	)
{
	SG_string*              sInfo             = NULL;
	SG_resolve__choice*     pChoice           = NULL;
	SG_string*              pChoiceInfo       = NULL;
	const char*             szLabel           = NULL;
	SG_uint32               uFlags            = 0u;
	SG_bool                 bChangeset        = SG_FALSE;
	const char*             szChangeset       = NULL;
	SG_bool                 bMergeableWorking = SG_FALSE;
	SG_resolve__value*      pMWParent         = NULL;
	SG_bool                 bMWModified       = SG_FALSE;
	SG_bool                 bMerge            = SG_FALSE;
	SG_resolve__value*      pBaseline         = NULL;
	SG_resolve__value*      pOther            = NULL;
	SG_bool                 bAutomatic        = SG_FALSE;
	SG_int32                iToolResult       = SG_FILETOOL__RESULT__COUNT;
	SG_int64                iTime             = 0;
	const char*             szBaselineLabel   = NULL;
	const char*             szOtherLabel      = NULL;
	const char*             szMWParentLabel   = NULL;
	SG_bool                 bData             = SG_FALSE;
	SG_int64                iData             = 0;
	const char*             szData            = NULL;
	SG_uint64               uSize             = 0u;
	SG_int_to_string_buffer szDataBuffer;
	SG_int_to_string_buffer szSizeBuffer;
	SG_int_to_string_buffer szTimeBuffer;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(ppInfo);

	VERIFY_ERR_CHECK(  SG_resolve__value__get_choice(pCtx, pValue, &pChoice)  );
	VERIFY_ERR_CHECK(  _resolve_choice__build_info_string(pCtx, pChoice, &pChoiceInfo)  );

	VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szLabel)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_flags(pCtx, pValue, &uFlags)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_changeset(pCtx, pValue, &bChangeset, &szChangeset)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_mergeable_working(pCtx, pValue, &bMergeableWorking, &pMWParent, &bMWModified)  );
	if (pMWParent != NULL)
	{
		VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pMWParent, &szMWParentLabel)  );
	}
	VERIFY_ERR_CHECK(  SG_resolve__value__get_merge(pCtx, pValue, &bMerge, &pBaseline, &pOther, &bAutomatic, NULL, &iToolResult, &iTime)  );
	if (pBaseline != NULL)
	{
		VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pBaseline, &szBaselineLabel)  );
	}
	if (pOther != NULL)
	{
		VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pOther, &szOtherLabel)  );
	}
	SG_ERR_TRY(  SG_resolve__value__get_data__bool(pCtx, pValue, &bData)  );
	SG_ERR_CATCH_IGNORE(SG_ERR_USAGE)
	SG_ERR_CATCH_END;
	SG_ERR_TRY(  SG_resolve__value__get_data__int64(pCtx, pValue, &iData)  );
	SG_ERR_CATCH_IGNORE(SG_ERR_USAGE)
	SG_ERR_CATCH_END;
	SG_ERR_TRY(  SG_resolve__value__get_data__sz(pCtx, pValue, &szData)  );
	SG_ERR_CATCH_IGNORE(SG_ERR_USAGE)
	SG_ERR_CATCH_END;
	VERIFY_ERR_CHECK(  SG_resolve__value__get_size(pCtx, pValue, &uSize)  );

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sInfo)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, sInfo, "%s ValueLabel(%s) ValueFlags(%x) ValueBoolData(%s) ValueIntData(%s) ValueStringData(%s) ValueSize(%s) ValueChangeset(%s) ValueMergeableWorkingParent(%s) ValueMergeableWorkingModified(%s) ValueBaseline(%s) ValueOther(%s) ValueAutoMerge(%s) ValueMergeToolResult(%d) ValueMergeTime(%s)", SG_string__sz(pChoiceInfo), szLabel, uFlags, _bool__sz(bData), SG_int64_to_sz(iData, szDataBuffer), szData, SG_int64_to_sz(uSize, szSizeBuffer), szChangeset, szMWParentLabel, _bool__sz(bMWModified), szBaselineLabel, szOtherLabel, _bool__sz(bAutomatic), iToolResult, SG_int64_to_sz(iTime, szTimeBuffer))  );

	*ppInfo = sInfo;
	sInfo = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	SG_STRING_NULLFREE(pCtx, pChoiceInfo);
	return;
}

/**
 * SG_resolve__item__callback that does nothing.
 */
static void _resolve_item__null(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current item.
	void*             pUserData, //< [in] Ignored.
	SG_bool*          pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pItem);
	SG_UNUSED(pUserData);
	SG_UNUSED(pContinue);
}

/**
 * SG_resolve__choice__callback that does nothing.
 */
static void _resolve_choice__null(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The current choice.
	void*               pUserData, //< [in] Ignored.
	SG_bool*            pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pChoice);
	SG_UNUSED(pUserData);
	SG_UNUSED(pContinue);
}

/**
 * SG_resolve__value__callback that does nothing.
 */
static void _resolve_value__null(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The current value.
	void*              pUserData, //< [in] Ignored.
	SG_bool*           pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pValue);
	SG_UNUSED(pUserData);
	SG_UNUSED(pContinue);
}

/**
 * SG_resolve__value__callback that calls as many value-specific read-only resolve
 * functions as readily possible.
 * This is intended to exercise the ability to read value-specific resolve data
 * while it's in various states.
 */
static void _resolve_value__read_data(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The value to read data about.
	void*              pUserData, //< [in] Ignored.
	SG_bool*           pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_bool             bValue        = SG_FALSE;
	SG_int32            i32Value      = 0;
	SG_int64            i64Value      = 0;
	SG_uint32           u32Value      = 0u;
	SG_uint64           u64Value      = 0u;
	const char*         szValue       = NULL;
	SG_string*          sSummary      = NULL;
	SG_resolve__choice* pChoice       = NULL;
	SG_resolve__state   eState        = SG_RESOLVE__STATE__COUNT;
	SG_resolve__value*  pRelatedValue = NULL;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pContinue);
	SG_UNUSED(pUserData);

	VERIFY_ERR_CHECK(  SG_resolve__value__get_choice(pCtx, pValue, &pChoice)  );

	VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szValue, 0u, &pRelatedValue)  );
	VERIFY_COND("Search by value's label didn't return value.", pValue == pRelatedValue);
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, szValue, 0u, &pRelatedValue)  );
	VERIFY_COND("Search by value's label didn't return value.", pValue == pRelatedValue);

	VERIFY_ERR_CHECK(  SG_resolve__value__get_changeset(pCtx, pValue, &bValue, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_changeset(pCtx, pValue, &bValue, &szValue)  );
	if (bValue != SG_FALSE && szValue != SG_RESOLVE__HID__WORKING)
	{
		VERIFY_ERR_CHECK(  SG_resolve__choice__check_value__changeset(pCtx, pChoice, szValue, &pRelatedValue)  );
		VERIFY_COND("Search by value's changeset didn't return value.", pValue == pRelatedValue);
		VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__changeset(pCtx, pChoice, szValue, &pRelatedValue)  );
		VERIFY_COND("Search by value's changeset didn't return value.", pValue == pRelatedValue);
	}

	VERIFY_ERR_CHECK(  SG_resolve__value__get_summary(pCtx, pValue, &sSummary)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_flags(pCtx, pValue, &u32Value)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_merge(pCtx, pValue, &bValue, NULL, NULL, NULL, NULL, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_merge(pCtx, pValue, &bValue, &pRelatedValue, &pRelatedValue, &bValue, &szValue, &i32Value, &i64Value)  );
	VERIFY_ERR_CHECK(  SG_resolve__value__get_size(pCtx, pValue, &u64Value)  );

	VERIFY_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	switch (eState)
	{
	case SG_RESOLVE__STATE__EXISTENCE:
		VERIFY_ERR_CHECK(  SG_resolve__value__get_data__bool(pCtx, pValue, &bValue)  );
		break;
	case SG_RESOLVE__STATE__ATTRIBUTES:
		VERIFY_ERR_CHECK(  SG_resolve__value__get_data__int64(pCtx, pValue, &i64Value)  );
		break;
	case SG_RESOLVE__STATE__NAME:
	case SG_RESOLVE__STATE__LOCATION:
	case SG_RESOLVE__STATE__CONTENTS:
		VERIFY_ERR_CHECK(  SG_resolve__value__get_data__sz(pCtx, pValue, &szValue)  );
		break;
	default:
		break;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sSummary);
	return;
}

/**
 * SG_resolve__choice__callback that calls as many choice-specific read-only resolve
 * functions as readily possible.
 * This is intended to exercise the ability to read choice-specific resolve data
 * while it's in various states.
 */
static void _resolve_choice__read_data(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The choice to read data about.
	void*               pUserData, //< [in] Ignored.
	SG_bool*            pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_bool                          bValue         = SG_FALSE;
	const char*                      szValue        = NULL;
	SG_resolve__item*                pItem          = NULL;
	SG_resolve__choice*              pRelatedChoice = NULL;
	SG_resolve__state                eState         = SG_RESOLVE__STATE__COUNT;
	SG_resolve__value*               pValue         = NULL;
	SG_mrg_cset_entry_conflict_flags eConflicts     = SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO;
	SG_vhash*                        pDescriptions  = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pContinue);
	SG_UNUSED(pUserData);

	VERIFY_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );

	VERIFY_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__check_choice__state(pCtx, pItem, eState, &pRelatedChoice)  );
	VERIFY_COND("Search by choice's state didn't return choice.", pChoice == pRelatedChoice);
	VERIFY_ERR_CHECK(  SG_resolve__item__get_choice__state(pCtx, pItem, eState, &pRelatedChoice)  );
	VERIFY_COND("Search by choice's state didn't return choice.", pChoice == pRelatedChoice);

	VERIFY_ERR_CHECK(  SG_resolve__choice__get_causes(pCtx, pChoice, &eConflicts, &bValue, &pDescriptions)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_cycle_info(pCtx, pChoice, &szValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_colliding_item(pCtx, pChoice, _resolve_item__null, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_cycling_item(pCtx, pChoice, _resolve_item__null, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_value(pCtx, pChoice, SG_TRUE, _resolve_value__null, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_value(pCtx, pChoice, SG_FALSE, _resolve_value__read_data, NULL)  );

	*pContinue = SG_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pDescriptions);
	return;
}

/**
 * SG_resolve__item__callback that calls as many item-specific read-only resolve
 * functions as readily possible.
 * This is intended to exercise the ability to read item-specific resolve data
 * while it's in various states.
 */
static void _resolve_item__read_data(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The item to read data about.
	void*             pUserData, //< [in] Ignored.
	SG_bool*          pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_bool                bValue       = SG_FALSE;
	const char*            szValue      = NULL;
	SG_resolve*            pResolve     = NULL;
	SG_resolve__item*      pRelatedItem = NULL;
	SG_treenode_entry_type eType        = SG_TREENODEENTRY_TYPE__INVALID;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pContinue);
	SG_UNUSED(pUserData);

	VERIFY_ERR_CHECK(  SG_resolve__item__get_resolve(pCtx, pItem, &pResolve)  );

	VERIFY_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pItem, &szValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__check_item__gid(pCtx, pResolve, szValue, &pRelatedItem)  );
	VERIFY_COND("Search by item's gid didn't return item.", pItem == pRelatedItem);
	VERIFY_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, szValue, &pRelatedItem)  );
	VERIFY_COND("Search by item's gid didn't return item.", pItem == pRelatedItem);

	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &szValue)  );
	if (szValue != NULL)
	{
		VERIFY_ERR_CHECK(  SG_resolve__check_item__path(pCtx, pResolve, szValue, &pRelatedItem)  );
		VERIFY_COND("Search by item's path didn't return item.", pItem == pRelatedItem);
		VERIFY_ERR_CHECK(  SG_resolve__get_item__path(pCtx, pResolve, szValue, &pRelatedItem)  );
		VERIFY_COND("Search by item's path didn't return item.", pItem == pRelatedItem);
	}

	VERIFY_ERR_CHECK(  SG_resolve__item__get_type(pCtx, pItem, &eType)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__ancestor(pCtx, pItem, &szValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__baseline(pCtx, pItem, &szValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__other(pCtx, pItem, 1u, &szValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_resolved(pCtx, pItem, &bValue, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_resolved(pCtx, pItem, &bValue, &bValue)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pItem, SG_TRUE, _resolve_choice__null, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pItem, SG_FALSE, _resolve_choice__read_data, NULL)  );

	*pContinue = SG_TRUE;

fail:
	return;
}

/**
 * Calls as many read-only resolve functions as readily possible.
 * This is intended to exercise the ability to read resolve data while it's in various states.
 */
static void _resolve__read_data(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	SG_resolve* pResolve //< [in] The resolve to read data from.
	)
{
	SG_pendingtree* pPendingTree = NULL;

	VERIFY_ERR_CHECK(  SG_resolve__get_pendingtree(pCtx, pResolve, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_resolve__foreach_item(pCtx, pResolve, SG_TRUE, _resolve_item__null, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__foreach_item(pCtx, pResolve, SG_FALSE, _resolve_item__read_data, NULL)  );

fail:
	return;
}

/**
 * Gets an item from a test by its name.
 * Fails to verify if the item isn't found.
 */
static void _test__get_item__name(
	SG_context*                pCtx,   //< [in] [out] Error and context info.
	u0109__test*               pTest,  //< [in] The test to find an item in.
	const char*                szName, //< [in] The name of the item to find.
	u0109__test__expect_item** ppItem  //< [out] The found item.
	)
{
	u0109__test__expect_item* pItem = pTest->aExpectedItems;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppItem);

	while (pItem->bValid != SG_FALSE)
	{
		if (strcmp(pItem->szName, szName) == 0)
		{
			break;
		}

		++pItem;
	}

	VERIFYP_COND_FAIL("Expected item not found in test by name.", pItem != NULL, ("ItemName(%s)", szName));

	*ppItem = pItem;
	pItem = NULL;

fail:
	return;
}

/**
 * Gets the actual and expected choices based on an expected item name and state.
 */
static void _test__find_choices(
	SG_context*                  pCtx,             //< [in] [out] Error and context info.
	u0109__test*                 pTest,            //< [in] The test to find the expected choice in.
	SG_resolve*                  pResolve,         //< [in] The resolve to find the actual choice in.
	const char*                  szItemName,       //< [in] The name of the test item to find a choice in.
	SG_resolve__state            eState,           //< [in] The state of the choice to find.
	u0109__test__expect_item**   ppExpectedItem,   //< [out] The item that owns the expected choice.
	u0109__test__expect_choice** ppExpectedChoice, //< [out] The requested expected choice.
	SG_resolve__item**           ppActualItem,     //< [out] The item that owns the actual choice.
	SG_resolve__choice**         ppActualChoice    //< [out] The requested actual choice.
	)
{
	u0109__test__expect_item*   pExpectedItem   = NULL;
	const char*                 szItemGid       = NULL;
	SG_resolve__item*           pActualItem     = NULL;
	u0109__test__expect_choice* pExpectedChoice = NULL;
	SG_resolve__choice*         pActualChoice   = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(szItemName);
	SG_NULLARGCHECK(ppExpectedItem);
	SG_NULLARGCHECK(ppExpectedChoice);
	SG_NULLARGCHECK(ppActualItem);
	SG_NULLARGCHECK(ppActualChoice);

	// get the expected item by its name
	VERIFY_ERR_CHECK(  _test__get_item__name(pCtx, pTest, szItemName, &pExpectedItem)  );

	// convert the step's item name into its GID, then use that to get the actual item
	VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pTest, szItemName, &szItemGid)  );
	VERIFY_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, szItemGid, &pActualItem)  );

	// find the expected choice in the expected item by state
	VERIFY_ERR_CHECK(  _expect_item__get_choice__state(pCtx, pExpectedItem, eState, &pExpectedChoice)  );

	// retrieve the actual choice by its item and state
	VERIFY_ERR_CHECK(  SG_resolve__item__get_choice__state(pCtx, pActualItem, eState, &pActualChoice)  );

	// return the data
	*ppExpectedItem = pExpectedItem;
	pExpectedItem = NULL;
	*ppExpectedChoice = pExpectedChoice;
	pExpectedChoice = NULL;
	*ppActualItem = pActualItem;
	pActualItem = NULL;
	*ppActualChoice = pActualChoice;
	pActualChoice = NULL;

fail:
	return;
}

/**
 * Gets the next merge name to use for a test.
 * Asserts if no more are available.
 */
static const char* _test__get_merge_name(
	u0109__test* pTest //< [in] The test to get the next merge name for.
	)
{
	// if this trips, just add more names to gaMergeNames
	SG_ASSERT(pTest->uNextMergeName < SG_NrElements(gaMergeNames));
	return gaMergeNames[pTest->uNextMergeName++];
}

/**
 * Builds the repo path of a temporary file from a base repo path
 * and the name of an item.
 */
static void _test__build_temp_repo_path(
	SG_context*  pCtx,       //< [in] [out] Error and context info.
	u0109__test* pTest,      //< [in] The test being executed.
	const char*  szRepoPath, //< [in] Base repo path to insert a partial GID into.
	                         //<      Must end with "~g~".  Partial GID inserted after the 'g'.
	const char*  szItemName, //< [in] Name of the item to use the GID of.
	SG_string**  ppRepoPath  //< [out] The resulting repo path including item's partial GID.
	)
{
	SG_string*  sRepoPath = NULL;
	SG_uint32   uLength   = 0u;
	const char* szGid     = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szRepoPath);
	SG_NULLARGCHECK(szItemName);
	SG_NULLARGCHECK(ppRepoPath);

	// make sure the repo path ends with "~g~"
	uLength = SG_STRLEN(szRepoPath);
	VERIFYP_COND_FAIL("Specified temp file doesn't end with '~g~'.", (uLength >= 3u && szRepoPath[uLength-3u] == '~' && szRepoPath[uLength-2u] == 'g' && szRepoPath[uLength-1u] == '~'), ("RepoPath(%s) ItemName(%s)", szRepoPath, szItemName));

	// get the GID of the referenced item
	VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pTest, szItemName, &szGid)  );

	// allocate a string for the path and insert the partial GID into it
	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sRepoPath, szRepoPath)  );
	VERIFY_ERR_CHECK(  SG_string__insert__buf_len(pCtx, sRepoPath, uLength-1u, (const SG_byte*)(szGid+1u), 6u)  );

	// return the string
	*ppRepoPath = sRepoPath;
	sRepoPath = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sRepoPath);
	return;
}


/*
**
** Helpers - Setup
**
*/

/**
 * Runs an ADD setup step.
 */
static void _execute_setup_step__add(
	SG_context*                    pCtx,             //< [in] [out] Error and context info.
	u0109__test*                   pTest,            //< [in] The test we're executing the setup plan for.
	const char*                    szRepoPath,       //< [in] Repo path of the item to add.
	SG_treenode_entry_type         eType,            //< [in] Type of item to add.
	const char*                    szItemName,       //< [in] Name to assign to the item to refer to it later instead of repo path.
	                                                 //<      If NULL, the item won't be retrievable by name.
	const char*                    szContentName,    //< [in] Name to assign to the item's content.
	                                                 //<      If NULL, the content won't be retrievable by name.
	u0109__test__content_function* fContentFunction, //< [in] Function to use to generate the item's content.
	                                                 //<      Only used for file and symlink items.
	                                                 //<      NULL allowed to create empty files.
	const char*                    szUserData        //< [in] Value to pass to fContentFunction.
	)
{
	SG_pathname*    pItemPath       = NULL;
	SG_file*        pFile           = NULL;
	SG_pendingtree* pPendingTree    = NULL;
	SG_repo*        pRepo           = NULL;
	char*           szHid           = NULL;
	SG_pathname*    pTarget         = NULL;
	SG_string*      sRelativeTarget = NULL;
	SG_string*      sTarget         = NULL;
	const char*     szItemPath      = NULL;
	char*           szItemGid       = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szRepoPath);

	// convert the repo path to an absolute one
	VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szRepoPath, &pItemPath)  );

	// decide what to do for each type
	switch (eType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		{
			// create the file
			VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pItemPath, SG_FILE_OPEN_OR_CREATE | SG_FILE_RDWR | SG_FILE_TRUNC, 0644, &pFile)  );
			SG_FILE_NULLCLOSE(pCtx, pFile);

			// if we have a function to generate content, call it
			if (fContentFunction != NULL)
			{
				VERIFY_ERR_CHECK(  fContentFunction(pCtx, pItemPath, szUserData)  );
			}

			// if they provided a name for the content, add it to the list
			if (szContentName != NULL)
			{
				SG_fsobj_stat cStat;

				// allocate a pendingtree from our path..
				VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );

				// to get the associated repo..
				VERIFY_ERR_CHECK(  SG_pendingtree__get_repo(pCtx, pPendingTree, &pRepo)  );

				// to generate the file's hash
				VERIFY_ERR_CHECK(  _repo__hash_file__pathname(pCtx, pRepo, pItemPath, &szHid)  );

				// add the name/hash to the list
				VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pContentsHids, szContentName, szHid)  );

				// add the name/repopath to the list
				VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pContentsRepoPaths, szContentName, szRepoPath)  );

				// stat the file to get its size
				VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pItemPath, &cStat)  );

				// add the name/size to the list
				VERIFY_ERR_CHECK( SG_vhash__add__int64(pCtx, pTest->pContentsSizes, szContentName, (SG_int64)cStat.size)  );
			}

			break;
		}
	case SG_TREENODEENTRY_TYPE_SYMLINK:
		{
			const char* szTarget = NULL;

			SG_NULLARGCHECK(fContentFunction);

			// if the target is a repo path, convert it
			if (szUserData[0] == '@')
			{
				// convert the repo path to absolute
				VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szUserData, &pTarget)  );

				// make the symlink target relative to the working copy
				VERIFY_ERR_CHECK(  SG_pathname__make_relative(pCtx, pTest->pPath, pTarget, &sRelativeTarget)  );
				szTarget = SG_string__sz(sRelativeTarget);
			}
			else
			{
				szTarget = szUserData;
			}

			// call the content function to generate the symlink
			VERIFY_ERR_CHECK(  fContentFunction(pCtx, pItemPath, szTarget)  );

			// if they provided a name for the content, add it to the list
			if (szContentName != NULL)
			{
				// read the symlink target
				if (fContentFunction == _content__symlink)
				{
					// if they used the _content__symlink function
					// then we already know the target
					SG_STRING__ALLOC__SZ(pCtx, &sTarget, szTarget);
				}
				else
				{
					// if they used some other function
					// we'll have to read the target from the file system
					VERIFY_ERR_CHECK(  SG_fsobj__readlink(pCtx, pItemPath, &sTarget)  );
				}

				// add the name/target to the list
				VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pContentsHids, szContentName, SG_string__sz(sTarget))  );

				// add the name/size to the list
				VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTest->pContentsSizes, szContentName, (SG_int64)strlen(SG_string__sz(sTarget)))  );
			}
		}
	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		{
			// check if something already exists at this path
			SG_bool bExists = SG_FALSE;
			VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pItemPath, &bExists, NULL, NULL)  );

			// if nothing exists, create the directory
			if (bExists == SG_FALSE)
			{
				VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pItemPath)  );
			}

			break;
		}
	default:
		VERIFYP_COND_FAIL("Unsupported item type.", SG_FALSE, ("ItemType(%d)", eType));
		break;
	}

	// add the item in the pendingtree
	if (pPendingTree == NULL)
	{
		VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
	}
	szItemPath = SG_pathname__sz(pItemPath);
	VERIFY_ERR_CHECK(  SG_pendingtree__add(pCtx, pPendingTree, 1u, &szItemPath, SG_TRUE, gpBlankFilespec, SG_FALSE)  );

	// if they provided a name for the item
	// then look up its GID and store the mapping
	if (szItemName != NULL)
	{
		const char* szExistingGid = NULL;

		VERIFY_ERR_CHECK(  SG_pendingtree__get_gid_from_local_path(pCtx, pPendingTree, pItemPath, &szItemGid)  );
		VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTest->pItemsGids, szItemName, &szExistingGid)  );
		VERIFYP_COND("Item nickname assigned to multiple GIDs.", szExistingGid == NULL || SG_stricmp(szItemGid, szExistingGid) == 0, ("ItemNickname(%s) ExistingGid(%s) NewGid(%s)", szItemName, szExistingGid, szItemGid));
		if (szExistingGid == NULL)
		{
			VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pItemsGids, szItemName, szItemGid)  );
		}
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pItemPath);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	SG_NULLFREE(pCtx, szHid);
	SG_PATHNAME_NULLFREE(pCtx, pTarget);
	SG_STRING_NULLFREE(pCtx, sRelativeTarget);
	SG_STRING_NULLFREE(pCtx, sTarget);
	SG_NULLFREE(pCtx, szItemGid);
	return;
}

/**
 * Runs a DELETE setup step.
 */
static void _execute_setup_step__delete(
	SG_context*     pCtx,        //< [in] [out] Error and context info.
	u0109__test*    pTest,       //< [in] The test we're executing the setup plan for.
	const char*     szRepoPath,  //< [in] Repo path of the item to add.
	SG_pendingtree* pPendingTree //< [in] Pendingtree to delete the file from.
	                             //<      If NULL, one is temporarily allocated from the test's path.
	)
{
	SG_pathname* pItemPath         = NULL;
	const char*  szItemPath        = NULL;
	SG_bool      bPendingTreeOwned = SG_FALSE;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szRepoPath);

	// TODO: it'd be preferable to use a verbs-level remove if one was available
	VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szRepoPath, &pItemPath)  );
	szItemPath = SG_pathname__sz(pItemPath);
	if (pPendingTree == NULL)
	{
		bPendingTreeOwned = SG_TRUE;
		VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
		VERIFY_ERR_CHECK(  SG_pendingtree__remove(pCtx, pPendingTree, 1u, &szItemPath, gpBlankFilespec, SG_FALSE, SG_FALSE, SG_FALSE)  );
	}
	else
	{
		bPendingTreeOwned = SG_FALSE;
		VERIFY_ERR_CHECK(  SG_pendingtree__remove_dont_save_pendingtree(pCtx, pPendingTree, 1u, &szItemPath, gpBlankFilespec, SG_FALSE, SG_FALSE, SG_FALSE)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pItemPath);
	if (bPendingTreeOwned != SG_FALSE)
	{
		SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	}
	return;
}

/**
 * Runs a MOVE setup step.
 */
static void _execute_setup_step__move(
	SG_context*  pCtx,            //< [in] [out] Error and context info.
	u0109__test* pTest,           //< [in] The test we're executing the setup plan for.
	const char*  szRepoPath,      //< [in] Repo path of the item to move.
	const char*  szParentRepoPath //< [in] Parent repo path to move the item to.
	)
{
	SG_pathname*    pItemPath    = NULL;
	SG_pathname*    pOtherPath   = NULL;
	SG_pendingtree* pPendingTree = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szRepoPath);
	SG_NULLARGCHECK(szParentRepoPath);

	// TODO: it'd be preferable to use a verbs-level move if one was available
	VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szRepoPath, &pItemPath)  );
	VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szParentRepoPath, &pOtherPath)  );
	VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_pendingtree__move(pCtx, pPendingTree, (const SG_pathname**)&pItemPath, 1u, pOtherPath)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pItemPath);
	SG_PATHNAME_NULLFREE(pCtx, pOtherPath);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	return;
}

/**
 * Runs a RENAME setup step.
 */
static void _execute_setup_step__rename(
	SG_context*  pCtx,       //< [in] [out] Error and context info.
	u0109__test* pTest,      //< [in] The test we're executing the setup plan for.
	const char*  szRepoPath, //< [in] Repo path of the item to rename.
	const char*  szName      //< [in] New name for the item.
	)
{
	SG_pathname*    pItemPath    = NULL;
	SG_pendingtree* pPendingTree = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szRepoPath);
	SG_NULLARGCHECK(szName);

	// TODO: it'd be preferable to use a verbs-level rename if one was available
	VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szRepoPath, &pItemPath)  );
	VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_pendingtree__rename(pCtx, pPendingTree, pItemPath, szName, SG_FALSE)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pItemPath);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	return;
}

/**
 * Runs a MODIFY setup step.
 */
static void _execute_setup_step__modify(
	SG_context*                    pCtx,             //< [in] [out] Error and context info.
	u0109__test*                   pTest,            //< [in] The test we're executing the setup plan for.
	const char*                    szRepoPath,       //< [in] Repo path of the item to modify.
	const char*                    szName,           //< [in] Name to assign to the item's new content.
	                                                 //<      If NULL, the new content won't be retrievable by name.
	u0109__test__content_function* fContentFunction, //< [in] Function to use to modify the item's content.
	const char*                    szUserData,       //< [in] Value to pass to fContentFunction.
	SG_repo*                       pRepo             //< [in] Repo to use to compute the file's new HID.
	                                                 //<      If NULL, a pendingtree is allocated for the test and its repo is used.
	)
{
	SG_pathname*    pItemPath    = NULL;
	SG_pathname*    pTarget      = NULL;
	SG_string*      sTarget      = NULL;
	SG_pendingtree* pPendingTree = NULL;
	char*           szHid        = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szRepoPath);
	SG_NULLARGCHECK(fContentFunction);

	// convert the repo path to an absolute one
	VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szRepoPath, &pItemPath)  );

	// if we're modifying a symlink
	// convert the target repo path to a relative one
	if (fContentFunction == _content__symlink)
	{
		VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szUserData, &pTarget)  );
		VERIFY_ERR_CHECK(  SG_pathname__make_relative(pCtx, pTest->pPath, pTarget, &sTarget)  );
		szUserData = SG_string__sz(sTarget);
	}

	// run the modify function to change the item
	VERIFY_ERR_CHECK(  fContentFunction(pCtx, pItemPath, szUserData)  );

	// if they provided a name for the content, add it to the list
	if (szName != NULL)
	{
		SG_fsobj_stat cStat;

		// if we weren't given a repo, allocate a pendingtree for the test's path and use its
		if (pRepo == NULL)
		{
			VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
			VERIFY_ERR_CHECK(  SG_pendingtree__get_repo(pCtx, pPendingTree, &pRepo)  );
		}

		// generate the file's hash
		VERIFY_ERR_CHECK(  _repo__hash_file__pathname(pCtx, pRepo, pItemPath, &szHid)  );

		// add the name/hash to the list
		VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pContentsHids, szName, szHid)  );

		// add the name/repopath to the list
		VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pContentsRepoPaths, szName, szRepoPath)  );

		// stat the file
		VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pItemPath, &cStat)  );

		// add the name/size to the list
		VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTest->pContentsSizes, szName, (SG_int64)cStat.size)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pItemPath);
	SG_PATHNAME_NULLFREE(pCtx, pTarget);
	SG_STRING_NULLFREE(pCtx, sTarget);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	SG_NULLFREE(pCtx, szHid);
	return;
}

/**
 * Runs a REVERT setup step.
 */
static void _execute_setup_step__revert(
	SG_context*  pCtx,
	u0109__test* pTest,
	const char*  szRepoPath
	)
{
	SG_bool      bRecursive = SG_FALSE;
	SG_uint32    uItems     = 1u;
	const char** ppItems    = &szRepoPath;
	SG_vhash*    pResult    = NULL;

	SG_NULLARGCHECK(pTest);

	if (szRepoPath == NULL)
	{
		bRecursive = SG_TRUE;
		uItems     = 0u;
		ppItems    = NULL;
	}

	VERIFY_ERR_CHECK(  SG_vv_verbs__revert(pCtx, SG_pathname__sz(pTest->pPath), bRecursive, SG_FALSE, SG_FALSE, SG_FALSE, SG_TRUE, NULL, gpBlankFilespec, uItems, ppItems, &pResult)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pResult);
	return;
}

/**
 * Runs a COMMIT setup step.
 */
static void _execute_setup_step__commit(
	SG_context*  pCtx,  //< [in] [out] Error and context info.
	u0109__test* pTest, //< [in] The test we're executing the setup plan for.
	const char*  szName //< [in] Name and message for the commit.
	)
{
	char* szHid = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);

	VERIFY_ERR_CHECK(  SG_vv_verbs__commit(pCtx, SG_pathname__sz(pTest->pPath), szName, gpBlankFilespec, SG_TRUE, SG_FALSE, SG_AUDIT__WHO__FROM_SETTINGS, SG_AUDIT__WHEN__NOW, 0, NULL, 0, NULL, NULL, NULL, NULL, &szHid)  );
	VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pCommitsHids, szName, szHid)  );

fail:
	SG_NULLFREE(pCtx, szHid);
	return;
}

/**
 * Runs an UPDATE setup step.
 */
static void _execute_setup_step__update(
	SG_context*  pCtx,  //< [in] [out] Error and context info.
	u0109__test* pTest, //< [in] The test we're executing the setup plan for.
	const char*  szName //< [in] Name of the commit to update to.
	)
{
	const char* szHid = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szName);

	VERIFY_ERR_CHECK(  _test__map_name__commit_to_hid(pCtx, pTest, szName, &szHid)  );

#if defined(UPDATE_BY_CHECKOUT)
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pTest->pPath, pTest->pRootPath)  );
	VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pTest->pPaths, (void*)pTest->pPath, NULL)  );
	VERIFY_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, pTest->pPath, "")  );
	VERIFY_ERR_CHECK(  SG_vv_verbs__checkout(pCtx, SG_string__sz(pTest->sRepo), szHid, SG_pathname__sz(pTest->pPath), SG_TRUE, SG_FALSE, NULL, NULL, NULL)  );
#else
	VERIFY_ERR_CHECK(  SG_vv_verbs__update(pCtx, SG_pathname__sz(pTest->pPath), szHid, NULL, SG_FALSE, SG_FALSE, NULL, NULL, NULL, NULL)  );
#endif

fail:
	return;
}

/**
 * Runs a MERGE setup step.
 */
static void _execute_setup_step__merge(
	SG_context*  pCtx,   //< [in] [out] Error and context info.
	u0109__test* pTest,  //< [in] The test we're executing the setup plan for.
	const char*  szName, //< [in] The name of the merge being executed.
	const char*  szOther //< [in] Name of the commit to merge with.
	)
{
	SG_int64    iTimeStart    = 0;
	SG_int64    iTimeEnd      = 0;
	const char* szOtherHid    = NULL;
	SG_vhash*   pMergeResults = NULL;
	SG_wd_plan* pMergePlan    = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szOther);

	VERIFY_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &iTimeStart)  );
#ifdef TIMESTAMP_CHECK_DELAY
	VERIFY_ERR_CHECK(  SG_sleep_ms(TIMESTAMP_CHECK_DELAY)  );
#endif
	VERIFY_ERR_CHECK(  _test__map_name__commit_to_hid(pCtx, pTest, szOther, &szOtherHid)  );
	VERIFY_ERR_CHECK(  SG_vv_verbs__merge(pCtx, SG_pathname__sz(pTest->pPath), szOtherHid, NULL, SG_FALSE, NULL, &pMergeResults, &pMergePlan)  );
	VERIFY_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &iTimeEnd)  );

	VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTest->pMergesStartTimes, szName, iTimeStart)  );
	VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTest->pMergesEndTimes, szName, iTimeEnd)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pMergeResults);
	SG_WD_PLAN_NULLFREE(pCtx, pMergePlan);
	return;
}

/**
 * Runs a CONTENTS setup step.
 */
static void _execute_setup_step__contents(
	SG_context*  pCtx,                   //< [in] [out] Error and context info.
	u0109__test* pTest,                  //< [in] The test we're executing the setup plan for.
	const char*  szAncestorContentsName, //< [in] Name of the contents to use for the common ancestor.
	const char*  szBaselineContentsName, //< [in] Name of the contents to use for the baseline parent of the merge.
	const char*  szOtherContentsName,    //< [in] Name of the contents to use for the other parent of the merge.
	const char*  szResultContentsName    //< [in] Name to assign to the new merged contents.
	)
{
	const char*      szAncestorHid             = NULL;
	const char*      szBaselineHid             = NULL;
	const char*      szOtherHid                = NULL;
	const char*      szAncestorRepoPath        = NULL;
	const char*      szBaselineRepoPath        = NULL;
	const char*      szOtherRepoPath           = NULL;
	SG_pendingtree*  pPendingTree              = NULL;
	SG_bool          bIssues                   = SG_FALSE;
	const SG_varray* pIssues                   = NULL;
	SG_uint32        uIssueCount               = 0u;
	SG_uint32        uIssueIndex               = 0u;
	SG_pathname*     pOutputResultAbsolutePath = NULL;
	char*            szOutputHid               = NULL;

	// translate the parent content names into their HIDs
	VERIFY_ERR_CHECK(  _test__map_name__content_to_hid(pCtx, pTest, szAncestorContentsName, &szAncestorHid)  );
	VERIFY_ERR_CHECK(  _test__map_name__content_to_hid(pCtx, pTest, szBaselineContentsName, &szBaselineHid)  );
	VERIFY_ERR_CHECK(  _test__map_name__content_to_hid(pCtx, pTest, szOtherContentsName, &szOtherHid)  );
	VERIFY_ERR_CHECK(  _test__map_name__content_to_repo_path(pCtx, pTest, szAncestorContentsName, &szAncestorRepoPath)  );
	VERIFY_ERR_CHECK(  _test__map_name__content_to_repo_path(pCtx, pTest, szBaselineContentsName, &szBaselineRepoPath)  );
	VERIFY_ERR_CHECK(  _test__map_name__content_to_repo_path(pCtx, pTest, szOtherContentsName, &szOtherRepoPath)  );

	// load the pendingtree we're testing in
	VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );

	// run through each issue in the pendingtree to find the matching one
	VERIFY_ERR_CHECK(  SG_pendingtree__get_wd_issues__ref(pCtx, pPendingTree, &bIssues, &pIssues)  );
	VERIFY_ERR_CHECK(  SG_varray__count(pCtx, pIssues, &uIssueCount)  );
	for (uIssueIndex = 0u; uIssueIndex < uIssueCount; ++uIssueIndex)
	{
		SG_vhash* pIssue      = NULL;
		SG_vhash* pInputs     = NULL;
		SG_uint32 uInputCount = 0u;
		SG_uint32 uInputIndex = 0u;
		SG_bool   bAncestor   = SG_FALSE;
		SG_bool   bBaseline   = SG_FALSE;
		SG_bool   bOther      = SG_FALSE;

		// get the set of inputs from this issue
		VERIFY_ERR_CHECK(  SG_varray__get__vhash(pCtx, pIssues, uIssueIndex, &pIssue)  );
		VERIFY_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pIssue, "input", &pInputs)  );
		VERIFY_ERR_CHECK(  SG_vhash__count(pCtx, pInputs, &uInputCount)  );

		// if there aren't exactly three, we definitely won't match
		if (uInputCount == 3u)
		{
			// run through each input and match it
			for (uInputIndex = 0u; uInputIndex < uInputCount; ++uInputIndex)
			{
				SG_vhash*   pInput          = NULL;
				const char* szInputHid      = NULL;
				const char* szInputRepoPath = NULL;

				// get the HID and repo path of this input
				VERIFY_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pInputs, uInputIndex, NULL, &pInput)  );
				VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pInput, "hid", &szInputHid)  );
				VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pInput, "repopath", &szInputRepoPath)  );

				// check to see if it matches any of our HID/repopath pairs
				if (
					   strcmp(szInputHid, szAncestorHid) == 0
					&& strcmp(szInputRepoPath, szAncestorRepoPath) == 0
					)
				{
					bAncestor = SG_TRUE;
				}
				else if (
					   strcmp(szInputHid, szBaselineHid) == 0
					&& strcmp(szInputRepoPath, szBaselineRepoPath) == 0
					)
				{
					bBaseline = SG_TRUE;
				}
				else if (
					   strcmp(szInputHid, szOtherHid) == 0
					&& strcmp(szInputRepoPath, szOtherRepoPath) == 0
					)
				{
					bOther = SG_TRUE;
				}
			}
		}

		// if we matched all of our HID/repopath pairs against this input
		// then we'll take the data from this issue's output as our result
		if (bAncestor == SG_TRUE && bBaseline == SG_TRUE && bOther == SG_TRUE)
		{
			SG_vhash*     pOutput                = NULL;
			const char*   szOutputRepoPath       = NULL;
			const char*   szOutputResultRepoPath = NULL;
			SG_repo*      pRepo                  = NULL;
			SG_fsobj_stat cStat;

			// get the output data and its repo path
			VERIFY_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pIssue, "output", &pOutput)  );
			VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pOutput, "repopath", &szOutputRepoPath)  );

			// get the repo path of the issue's temporary result file
			// and convert it to an absolute path
			VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pIssue, "repopath_tempfile_result", &szOutputResultRepoPath)  );
			VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path2(pCtx, pPendingTree, szOutputResultRepoPath, &pOutputResultAbsolutePath)  );

			// generate the hash of the temporary result file
			VERIFY_ERR_CHECK(  SG_pendingtree__get_repo(pCtx, pPendingTree, &pRepo)  );
			VERIFY_ERR_CHECK(  _repo__hash_file__pathname(pCtx, pRepo, pOutputResultAbsolutePath, &szOutputHid)  );

			// get the size of the temporary result file
			VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pOutputResultAbsolutePath, &cStat)  );

			// store the output data in the test
			VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pContentsHids, szResultContentsName, szOutputHid)  );
			VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pContentsRepoPaths, szResultContentsName, szOutputRepoPath)  );
			VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTest->pContentsSizes, szResultContentsName, (SG_int64)cStat.size)  );

			// stop iterating issues, we found the one we wanted
			break;
		}
	}

fail:
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	SG_PATHNAME_NULLFREE(pCtx, pOutputResultAbsolutePath);
	SG_NULLFREE(pCtx, szOutputHid);
	return;
}

/**
 * Executes a single step of a setup plan.
 */
static void _execute_setup_step(
	SG_context*              pCtx,  //< [in] [out] Error and context info.
	u0109__test*             pTest, //< [in] The test we're executing the setup plan for.
	u0109__test__setup_step* pStep  //< [in] The setup step to execute.
	)
{
	const char*     szRepoPath   = pStep->szRepoPath;
	SG_pendingtree* pPendingTree = NULL;
	SG_string*      sRepoPath    = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pStep);

	// if this step has a repo path that doesn't start with @
	// then assume it's a symbolic name for its GID
	// and translate to its current repo path
	if (szRepoPath != NULL && szRepoPath[0] != '@')
	{
		const char* szGid = NULL;

		VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pTest, pStep->szRepoPath, &szGid)  );
		VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
		VERIFY_ERR_CHECK(  SG_pendingtree__get_repo_path(pCtx, pPendingTree, szGid, &sRepoPath)  );
		SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
		szRepoPath = SG_string__sz(sRepoPath);
	}

	switch (pStep->eAction)
	{
	case SETUP_ACTION_ADD:
		VERIFY_ERR_CHECK(  _execute_setup_step__add(pCtx, pTest, szRepoPath, pStep->eItemType, pStep->szOther, pStep->szName, pStep->fContentFunction, pStep->szValue)  );
		break;
	case SETUP_ACTION_DELETE:
		VERIFY_ERR_CHECK(  _execute_setup_step__delete(pCtx, pTest, szRepoPath, NULL)  );
		break;
	case SETUP_ACTION_MOVE:
		VERIFY_ERR_CHECK(  _execute_setup_step__move(pCtx, pTest, szRepoPath, pStep->szValue)  );
		break;
	case SETUP_ACTION_RENAME:
		VERIFY_ERR_CHECK(  _execute_setup_step__rename(pCtx, pTest, szRepoPath, pStep->szValue)  );
		break;
	case SETUP_ACTION_MODIFY:
		VERIFY_ERR_CHECK(  _execute_setup_step__modify(pCtx, pTest, szRepoPath, pStep->szName, pStep->fContentFunction, pStep->szValue, NULL)  );
		break;
	case SETUP_ACTION_REVERT:
		VERIFY_ERR_CHECK(  _execute_setup_step__revert(pCtx, pTest, szRepoPath)  );
		break;
	case SETUP_ACTION_COMMIT:
		VERIFY_ERR_CHECK(  _execute_setup_step__commit(pCtx, pTest, pStep->szName)  );
		break;
	case SETUP_ACTION_UPDATE:
		VERIFY_ERR_CHECK(  _execute_setup_step__update(pCtx, pTest, pStep->szName)  );
		break;
	case SETUP_ACTION_MERGE:
		VERIFY_ERR_CHECK(  _execute_setup_step__merge(pCtx, pTest, pStep->szName, pStep->szValue)  );
		break;
	case SETUP_ACTION_CONTENTS:
		VERIFY_ERR_CHECK(  _execute_setup_step__contents(pCtx, pTest, pStep->szAncestor, pStep->szBaseline, pStep->szOther, pStep->szName)  );
		break;
	default:
		VERIFYP_COND_FAIL("Unsupported setup action.", SG_FALSE, ("Action(%d)", pStep->eAction));
		break;
	}

fail:
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	SG_STRING_NULLFREE(pCtx, sRepoPath);
	return;
}


/*
**
** Helpers - Verification - Value
**
*/

/**
 * SG_resolve__value__callback that errors if any value doesn't appear in a vector.
 */
static void _verify_expectations__value__must_appear(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The current value.
	void*              pUserData, //< [in] The vector to check for the value in.
	SG_bool*           pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_vector* pVector   = (SG_vector*)pUserData;
	SG_bool    bContains = SG_FALSE;
	SG_string* sInfo     = NULL;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, (void*)pValue, &bContains)  );
	if (bContains == SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _resolve_value__build_info_string(pCtx, pValue, &sInfo)  );
		VERIFYP_COND_FAIL("Unexpected value", bContains != SG_FALSE, ("%s", SG_string__sz(sInfo)));
	}

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * User data used by _verify_expectations__value__match.
 */
typedef struct _verify_expectations__value__match__data
{
	u0109__test*                pTest;           //< [in] The test we're verifying in.
	u0109__test__expect_item*   pExpectedItem;   //< [in] The item that owns the choice and value.
	u0109__test__expect_choice* pExpectedChoice; //< [in] The choice that owns the value.
	u0109__test__expect_value*  pExpectedValue;  //< [in] The expectation to match against.
	SG_resolve*                 pResolve;        //< [in] The resolve that owns the actual item and choice.
	SG_resolve__item*           pActualItem;     //< [in] The item that owns the actual choice.
	SG_resolve__choice*         pActualChoice;   //< [in] The choice that we're searching for a matching value.
	SG_vector*                  pInvalidValues;  //< [in] A list of actual values that cannot match.
	SG_resolve__value*          pMatch;          //< [out] The value that matched the expectation.
}
_verify_expectations__value__match__data;

/**
 * SG_resolve__value__callback that matches against an expected value.
 */
static void _verify_expectations__value__match(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The current value.
	void*              pUserData, //< [in] An instance of _verify_expectations__value__match__data.
	SG_bool*           pContinue  //< [out] Whether or not iteration should continue.
	)
{
	_verify_expectations__value__match__data* pData                   = (_verify_expectations__value__match__data*)pUserData;
	SG_bool                                   bInvalid                = SG_FALSE;
	SG_uint32                                 uActualFlags            = 0u;
	SG_bool                                   bActualChangeset        = SG_FALSE;
	const char*                               szActualChangeset       = NULL;
	SG_bool                                   bActualMergeableWorking = SG_FALSE;
	SG_resolve__value*                        pActualMWParent         = NULL;
	SG_bool                                   bActualMWModified       = SG_FALSE;
	SG_bool                                   bActualMerge            = SG_FALSE;
	SG_resolve__value*                        pActualBaseline         = NULL;
	SG_resolve__value*                        pActualOther            = NULL;
	SG_bool                                   bActualAutomatic        = SG_FALSE;
	SG_int32                                  iActualToolResult       = SG_FILETOOL__RESULT__COUNT;
	SG_int64                                  iActualTime             = 0;
	const char*                               szActualBaselineLabel   = NULL;
	const char*                               szActualOtherLabel      = NULL;
	const char*                               szActualMWParentLabel   = NULL;
	SG_uint64                                 uActualSize             = 0u;
	SG_bool                                   bActualData             = SG_FALSE; // important that this matches the default value for an expected value's bValue field
	SG_int64                                  iActualData             = 0; // important that this matches the default value for an expected value's iValue field
	const char*                               szActualData            = NULL; // important that this matches the default value for an expected value's szValue field
	const char*                               szExpectedChangeset     = NULL;
	SG_uint32                                 uExpectedFlags          = 0u;
	SG_uint32                                 uExpectedMask           = 0u;
	const char*                               szExpectedMWParentLabel = NULL;
	SG_int64                                  iExpectedTimeStart      = 0;
	SG_int64                                  iExpectedTimeEnd        = 0;
	const char*                               szExpectedBaselineLabel = NULL;
	const char*                               szExpectedOtherLabel    = NULL;
	const char*                               szExpectedData          = NULL;
	SG_string*                                sExpectedData           = NULL;
	SG_uint64                                 uExpectedSize           = 0u;
	SG_pathname*                              pExpectedPath           = NULL;
	SG_pathname*                              pExpectedParentPath     = NULL;
	SG_string*                                sExpectedParentPath     = NULL;
	char*                                     szExpectedParentGid     = NULL;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	if (pData->pInvalidValues != NULL)
	{
		VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pData->pInvalidValues, (void*)pValue, &bInvalid)  );
	}
	if (bInvalid == SG_FALSE)
	{
		// get the info from the actual value
		VERIFY_ERR_CHECK(  SG_resolve__value__get_flags(pCtx, pValue, &uActualFlags)  );
		VERIFY_ERR_CHECK(  SG_resolve__value__get_changeset(pCtx, pValue, &bActualChangeset, &szActualChangeset)  );
		VERIFY_ERR_CHECK(  SG_resolve__value__get_mergeable_working(pCtx, pValue, &bActualMergeableWorking, &pActualMWParent, &bActualMWModified)  );
		if (pActualMWParent != NULL)
		{
			VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pActualMWParent, &szActualMWParentLabel)  );
		}
		VERIFY_ERR_CHECK(  SG_resolve__value__get_merge(pCtx, pValue, &bActualMerge, &pActualBaseline, &pActualOther, &bActualAutomatic, NULL, &iActualToolResult, &iActualTime)  );
		if (pActualBaseline != NULL)
		{
			VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pActualBaseline, &szActualBaselineLabel)  );
		}
		if (pActualOther != NULL)
		{
			VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pActualOther, &szActualOtherLabel)  );
		}
		VERIFY_ERR_CHECK(  SG_resolve__value__get_size(pCtx, pValue, &uActualSize)  );

		// get the variant/data from the actual value
		// if we ask for the wrong type, we'll ignore the error and leave the variable with its default
		// that default will successfully match the default data used for expected values
		SG_ERR_TRY(  SG_resolve__value__get_data__bool(pCtx, pValue, &bActualData)  );
		SG_ERR_CATCH_IGNORE(SG_ERR_USAGE)
		SG_ERR_CATCH_END;
		SG_ERR_TRY(  SG_resolve__value__get_data__int64(pCtx, pValue, &iActualData)  );
		SG_ERR_CATCH_IGNORE(SG_ERR_USAGE)
		SG_ERR_CATCH_END;
		SG_ERR_TRY(  SG_resolve__value__get_data__sz(pCtx, pValue, &szActualData)  );
		SG_ERR_CATCH_IGNORE(SG_ERR_USAGE)
		SG_ERR_CATCH_END;

		// for changeset values, translate the commit name into a HID
		// translate a NULL commit name into the working HID
		if (pData->pExpectedValue->eType == EXPECT_VALUE_CHANGESET)
		{
			if (pData->pExpectedValue->szCommitName == NULL)
			{
				szExpectedChangeset = SG_RESOLVE__HID__WORKING;
			}
			else
			{
				VERIFY_ERR_CHECK(  _test__map_name__commit_to_hid(pCtx, pData->pTest, pData->pExpectedValue->szCommitName, &szExpectedChangeset)  );
			}
		}

		// get flags for the expected value
		VERIFY_ERR_CHECK(  _expect_value__get_flags(pCtx, pData->pExpectedItem, pData->pExpectedChoice, pData->pExpectedValue, &uExpectedFlags, &uExpectedMask)  );

		// translate the working parent name into a label
		if (pData->pExpectedValue->szMWParentName != NULL)
		{
			// Note: If there's a crash in here about not being able to find the
			//       parent's label in the name => label mappings in the choice,
			//       that's because the parent value hasn't been verified yet.
			//       Make sure that the test lists the parent's value BEFORE this
			//       one.  Usually this means moving the "merged" value above the
			//       "working" value so that the working value can reference the
			//       merged value as its parent (the merged value will have already
			//       been verified when the working value is processed, that way,
			//       and therefore already be in the name => label mappings).
			VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pData->pExpectedChoice->pValues, pData->pExpectedValue->szMWParentName, &szExpectedMWParentLabel)  );
		}

		// get the expected start and end times
		if (pData->pExpectedValue->szMergeName != NULL)
		{
			VERIFY_ERR_CHECK(  _test__map_name__merge_to_start_time(pCtx, pData->pTest, pData->pExpectedValue->szMergeName, &iExpectedTimeStart)  );
			VERIFY_ERR_CHECK(  _test__map_name__merge_to_end_time(pCtx, pData->pTest, pData->pExpectedValue->szMergeName, &iExpectedTimeEnd)  );
		}

		// translate the baseline/other names into labels
		if (pData->pExpectedValue->szBaselineParentName != NULL)
		{
			VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pData->pExpectedChoice->pValues, pData->pExpectedValue->szBaselineParentName, &szExpectedBaselineLabel)  );
		}
		if (pData->pExpectedValue->szOtherParentName != NULL)
		{
			VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pData->pExpectedChoice->pValues, pData->pExpectedValue->szOtherParentName, &szExpectedOtherLabel)  );
		}

		// translate the expected value's data as necessary
		switch (pData->pExpectedChoice->eState)
		{
		case SG_RESOLVE__STATE__EXISTENCE:
			{
				szExpectedData = pData->pExpectedValue->szData;
				uExpectedSize = sizeof(SG_bool);
			}
			break;
		case SG_RESOLVE__STATE__NAME:
			{
				SG_uint32 uLength = 0u;

				szExpectedData = pData->pExpectedValue->szData;
				uLength = SG_STRLEN(szExpectedData);

				// check for data that ends with "~g~"
				// and translate it to include the current item's partial GID
				if (
					   uLength >= 3u
					&& szExpectedData[uLength-3u] == '~'
					&& szExpectedData[uLength-2u] == 'g'
					&& szExpectedData[uLength-1u] == '~'
					)
				{
					const char* szGid  = NULL;
					VERIFY_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pData->pActualItem, &szGid)  );
					VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sExpectedData, szExpectedData)  );
					VERIFY_ERR_CHECK(  SG_string__insert__buf_len(pCtx, sExpectedData, uLength-1u, (const SG_byte*)(szGid+1u), 6u)  );
					szExpectedData = SG_string__sz(sExpectedData);
				}

				// calculate the expected size
				uExpectedSize = strlen(szExpectedData);
			}
			break;
		case SG_RESOLVE__STATE__LOCATION:
			{
				SG_pendingtree* pPendingTree = NULL;

				// we need the pendingtree no matter what
				VERIFY_ERR_CHECK(  SG_resolve__get_pendingtree(pCtx, pData->pResolve, &pPendingTree)  );

				// check what type of expectation is specified
				szExpectedData = pData->pExpectedValue->szData;
				if (*szExpectedData == '@')
				{
					// parent repo path
					// translate it into a GID
					VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path2(pCtx, pPendingTree, pData->pExpectedValue->szData, &pExpectedParentPath)  );
					VERIFY_ERR_CHECK(  SG_pendingtree__get_gid_from_local_path(pCtx, pPendingTree, pExpectedParentPath, &szExpectedParentGid)  );
					szExpectedData = szExpectedParentGid;
				}
				else 
				{
					// item name
					// translate it into a GID
					VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pData->pTest, szExpectedData, &szExpectedData)  );
				}

				// get a repo path for the GID so we can get its size
				VERIFY_ERR_CHECK(  SG_pendingtree__get_repo_path(pCtx, pPendingTree, szExpectedData, &sExpectedParentPath)  );
				uExpectedSize = strlen(SG_string__sz(sExpectedParentPath));
			}
			break;
		case SG_RESOLVE__STATE__ATTRIBUTES:
			{
				szExpectedData = pData->pExpectedValue->szData;
				uExpectedSize = sizeof(SG_int64);
			}
			break;
		case SG_RESOLVE__STATE__CONTENTS:
			switch (pData->pExpectedItem->eType)
			{
			case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
				{
					// translate the name into the corresponding file contents HID and size
					VERIFY_ERR_CHECK(  _test__map_name__content_to_hid(pCtx, pData->pTest, pData->pExpectedValue->szData, &szExpectedData)  );
					VERIFY_ERR_CHECK(  _test__map_name__content_to_size(pCtx, pData->pTest, pData->pExpectedValue->szData, &uExpectedSize)  );
				}
				break;
			case SG_TREENODEENTRY_TYPE_SYMLINK:
				{
					// translate the repo path into a relative one
					VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pData->pTest->pPath, pData->pExpectedValue->szData, &pExpectedPath)  );
					VERIFY_ERR_CHECK(  SG_pathname__make_relative(pCtx, pData->pTest->pPath, pExpectedPath, &sExpectedData)  );
					szExpectedData = SG_string__sz(sExpectedData);
					uExpectedSize = SG_string__length_in_bytes(sExpectedData);
				}
				break;
			case SG_TREENODEENTRY_TYPE_SUBMODULE:
				{
					// some kind of translation will be needed here, once submodules are supported
					VERIFY_COND("Submodules not supported by resolve tests.", SG_FALSE);
					// szExpectedData = ?
					// uExpectedSize = ?
				}
			default:
				{
					VERIFYP_COND("Unknown item type.", SG_FALSE, ("ItemType(%d)", pData->pExpectedItem->eType));
				}
			}
			break;
		default:
			{
				VERIFYP_COND("Unknown state.", SG_FALSE, ("ChoiceState(%d)", pData->pExpectedChoice->eState));
			}
			break;
		}

		// compare the expected value to the actual one
		if (
			   pData->pExpectedValue->bAutomatic == bActualAutomatic
			&& pData->pExpectedValue->iMergeToolResult == iActualToolResult
			&& pData->pExpectedValue->bData == bActualData
			&& pData->pExpectedValue->iData == iActualData
			&& pData->pExpectedValue->bMWModified == bActualMWModified
#if defined(TIMESTAMP_CHECK)
			&& (
				// If this test was based on a previous test then we can't do timestamp
				// matching, because the recorded merge times from the previous test
				// won't match the times from the files in this test's working copy
				// (which is a copy of the previous tests, but the timestamps will
				// reflect the time of the copy, not the original creation time).
				(pData->pTest->pStartTest != NULL) || (iExpectedTimeStart <= iActualTime && iExpectedTimeEnd >= iActualTime)
			)
#endif
			&& uExpectedSize == uActualSize
			&& (uExpectedFlags & uExpectedMask) == (uActualFlags & uExpectedMask)
			&& SG_strcmp__null(szExpectedData, szActualData) == 0
			&& SG_strcmp__null(szExpectedChangeset, szActualChangeset) == 0
			&& SG_strcmp__null(szExpectedBaselineLabel, szActualBaselineLabel) == 0
			&& SG_strcmp__null(szExpectedOtherLabel, szActualOtherLabel) == 0
			&& SG_strcmp__null(szExpectedMWParentLabel, szActualMWParentLabel) == 0
			)
		{
			pData->pMatch = pValue;
			*pContinue = SG_FALSE;
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, sExpectedData);
	SG_PATHNAME_NULLFREE(pCtx, pExpectedPath);
	SG_PATHNAME_NULLFREE(pCtx, pExpectedParentPath);
	SG_STRING_NULLFREE(pCtx, sExpectedParentPath);
	SG_NULLFREE(pCtx, szExpectedParentGid);
	return;
}

/**
 * Verifies that a choice contains a value that matches a given expected value.
 */
static void _verify_expectations__value(
	SG_context*                 pCtx,            //< [in] [out] Error and context info.
	u0109__test*                pTest,           //< [in] The test we're verifying in.
	u0109__test__expect_item*   pExpectedItem,   //< [in] The item that owns the choice and value.
	u0109__test__expect_choice* pExpectedChoice, //< [in] The choice that owns the value.
	u0109__test__expect_value*  pExpectedValue,  //< [in] The expected value to verify against.
	SG_resolve*                 pResolve,        //< [in] The resolve that owns the actual item and choice.
	SG_resolve__item*           pActualItem,     //< [in] The item that owns the actual choice.
	SG_resolve__choice*         pActualChoice,   //< [in] The choice to match a value in.
	SG_vector*                  pUsedValues      //< [in] A vector to add the matched value to.
	)
{
	_verify_expectations__value__match__data cFindData;
	SG_string*                               sInfo     = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pExpectedItem);
	SG_NULLARGCHECK(pExpectedChoice);
	SG_NULLARGCHECK(pExpectedValue);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pActualItem);
	SG_NULLARGCHECK(pActualChoice);

	// find the resolve value that matches our expectation
	cFindData.pTest           = pTest;
	cFindData.pExpectedItem   = pExpectedItem;
	cFindData.pExpectedChoice = pExpectedChoice;
	cFindData.pExpectedValue  = pExpectedValue;
	cFindData.pResolve        = pResolve;
	cFindData.pActualItem     = pActualItem;
	cFindData.pActualChoice   = pActualChoice;
	cFindData.pInvalidValues  = pUsedValues;
	cFindData.pMatch          = NULL;
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_value(pCtx, pActualChoice, SG_FALSE, _verify_expectations__value__match, (void*)&cFindData)  );
	if (cFindData.pMatch == NULL)
	{
		VERIFY_ERR_CHECK(  _expect_value__build_info_string(pCtx, pResolve, pActualItem, pTest, pExpectedItem, pExpectedChoice, pExpectedValue, &sInfo)  );
		VERIFYP_COND_FAIL("Expected value not found", cFindData.pMatch != NULL, ("%s", SG_string__sz(sInfo)));
	}

	// add this value to the list of known values in this choice
	if (pExpectedValue->szName != NULL)
	{
		const char* szActualLabel = NULL;

		VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, cFindData.pMatch, &szActualLabel)  );
		VERIFY_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pExpectedChoice->pValues, pExpectedValue->szName, szActualLabel)  );
	}

	// if we have a vector to add the matched value to, add it
	if (pUsedValues != NULL)
	{
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pUsedValues, (void*)cFindData.pMatch, NULL)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * Verifies that all values in an expectated choice exactly match all values in the corresponding WD choice.
 */
static void _verify_expectations__values(
	SG_context*                 pCtx,            //< [in] [out] Error and context info.
	u0109__test*                pTest,           //< [in] The test we're verifying in.
	u0109__test__expect_item*   pExpectedItem,   //< [in] The item that owns the choice.
	u0109__test__expect_choice* pExpectedChoice, //< [in] The choice to verify the expected values on.
	SG_resolve*                 pResolve,        //< [in] The resolve that owns the actual item and choice.
	SG_resolve__item*           pActualItem,     //< [in] The item that owns the actual choice.
	SG_resolve__choice*         pActualChoice    //< [in] The resolve choice to verify the expected values against.
	)
{
	SG_vector*                 pUsedValues    = NULL;
	u0109__test__expect_value* pExpectedValue = pExpectedChoice->aValues;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pExpectedItem);
	SG_NULLARGCHECK(pExpectedChoice);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pActualItem);
	SG_NULLARGCHECK(pActualChoice);

	// allocate a vector to track all the actual values that we matched to expected ones
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pUsedValues, MAX_EXPECT_VALUES)  );

	// verify each of the choice's expected values
	while (pExpectedValue->bValid != SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _verify_expectations__value(pCtx, pTest, pExpectedItem, pExpectedChoice, pExpectedValue, pResolve, pActualItem, pActualChoice, pUsedValues)  );
		++pExpectedValue;
	}

	// run through each actual value and make sure we matched it to an expected one
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_value(pCtx, pActualChoice, SG_FALSE, _verify_expectations__value__must_appear, pUsedValues)  );

fail:
	SG_VECTOR_NULLFREE(pCtx, pUsedValues);
	return;
}


/*
**
** Helpers - Verification - Relation
**
*/

/**
 * SG_resolve__item__callback that errors if any item doesn't appear in a vector.
 */
static void _verify_expectations__relation__must_appear(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current item.
	void*             pUserData, //< [in] The vector to check for the item in.
	SG_bool*          pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_vector* pVector   = (SG_vector*)pUserData;
	SG_bool    bContains = SG_FALSE;
	SG_string* pInfo     = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, (void*)pItem, &bContains)  );
	if (bContains == SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _resolve_item__build_info_string(pCtx, pItem, &pInfo)  );
		VERIFYP_COND_FAIL("Unexpected relation", bContains != SG_FALSE, ("%s", SG_string__sz(pInfo)));
	}

fail:
	SG_STRING_NULLFREE(pCtx, pInfo);
	return;
}

/**
 * User data used by _verify_expectations__relation__match.
 */
typedef struct _verify_expectations__relation__match__data
{
	u0109__test*                  pTest;             //< [in] The test we're verifying in.
	u0109__test__expect_item*     pExpectedItem;     //< [in] The item that owns the choice and relation.
	u0109__test__expect_choice*   pExpectedChoice;   //< [in] The choice that owns the relation.
	u0109__test__expect_relation* pExpectedRelation; //< [in] The expectation to match against.
	SG_resolve*                   pResolve;          //< [in] The resolve that owns the actual item and choice.
	SG_resolve__item*             pActualItem;       //< [in] The item that owns the actual choice.
	SG_resolve__choice*           pActualChoice;     //< [in] The choice that we're searching for a matching relation.
	SG_resolve__item*             pMatch;            //< [out] The item that matched the expectation.
}
_verify_expectations__relation__match__data;

/**
 * SG_resolve__item__callback that matches against an expected relation.
 */
static void _verify_expectations__relation__match(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current item.
	void*             pUserData, //< [in] An instance of _verify_expectations__relation__match__data.
	SG_bool*          pContinue  //< [out] Whether or not iteration should continue.
	)
{
	_verify_expectations__relation__match__data* pData         = (_verify_expectations__relation__match__data*)pUserData;
	const char*                                  szExpectedGid = NULL;
	const char*                                  szActualGid   = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	// translate the expected relation item name into its GID
	VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pData->pTest, pData->pExpectedRelation->szName, &szExpectedGid)  );

	// get the current item's actual GID
	VERIFY_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pItem, &szActualGid)  );

	// match the item against expectations
	if (
		strcmp(szExpectedGid, szActualGid) == 0
		)
	{
		pData->pMatch = pItem;
		*pContinue = SG_FALSE;
	}

fail:
	return;
}

/**
 * Verifies that a choice contains a relation that matches a given expected relation.
 */
static void _verify_expectations__relation(
	SG_context*                       pCtx,              //< [in] [out] Error and context info.
	u0109__test*                      pTest,             //< [in] The test we're verifying in.
	u0109__test__expect_relation_type eType,             //< [in] The type of relation to verify.
	u0109__test__expect_item*         pExpectedItem,     //< [in] The item that owns the choice and relation.
	u0109__test__expect_choice*       pExpectedChoice,   //< [in] The choice that owns the relation.
	u0109__test__expect_relation*     pExpectedRelation, //< [in] The expected relation to verify against.
	SG_resolve*                       pResolve,          //< [in] The resolve that owns the actual item and choice.
	SG_resolve__item*                 pActualItem,       //< [in] The item that owns the actual choice.
	SG_resolve__choice*               pActualChoice,     //< [in] The choice to match a relation in.
	SG_vector*                        pUsedRelations     //< [in] A vector to add the matched relation to.
	)
{
	_verify_expectations__relation__match__data cFindData;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pExpectedItem);
	SG_NULLARGCHECK(pExpectedChoice);
	SG_NULLARGCHECK(pExpectedRelation);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pActualItem);
	SG_NULLARGCHECK(pActualChoice);

	// only try to match relations of the specified type
	if (eType == pExpectedRelation->eType)
	{
		// find the resolve value that matches our expectation
		cFindData.pTest             = pTest;
		cFindData.pExpectedItem     = pExpectedItem;
		cFindData.pExpectedChoice   = pExpectedChoice;
		cFindData.pExpectedRelation = pExpectedRelation;
		cFindData.pResolve          = pResolve;
		cFindData.pActualItem       = pActualItem;
		cFindData.pActualChoice     = pActualChoice;
		cFindData.pMatch            = NULL;
		switch (eType) 
		{
		case EXPECT_RELATION_COLLISION:
			VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_colliding_item(pCtx, pActualChoice, _verify_expectations__relation__match, (void*)&cFindData)  );
			break;
		case EXPECT_RELATION_CYCLE:
			VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_cycling_item(pCtx, pActualChoice, _verify_expectations__relation__match, (void*)&cFindData)  );
			break;
		default:
			VERIFYP_COND_FAIL("Unknown relation type", SG_FALSE, ("ItemName(%s) ChoiceState(%d) RelatedItem(%s) RelationType(%d)", pExpectedItem->szName, pExpectedChoice->eState, pExpectedRelation->szName, eType));
			break;
		}
		VERIFYP_COND_FAIL("Expected relation not found", cFindData.pMatch != NULL, ("ItemName(%s) ChoiceState(%d) RelatedItem(%s)", pExpectedItem->szName, pExpectedChoice->eState, pExpectedRelation->szName));

		// if we have a vector to add the matched relation to, add it
		if (pUsedRelations != NULL)
		{
			VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pUsedRelations, (void*)cFindData.pMatch, NULL)  );
		}
	}

fail:
	return;
}

/**
 * Verifies that all relations in an expectated choice exactly match all relations in the corresponding WD choice.
 */
static void _verify_expectations__relations(
	SG_context*                 pCtx,            //< [in] [out] Error and context info.
	u0109__test*                pTest,           //< [in] The test we're verifying in.
	u0109__test__expect_item*   pExpectedItem,   //< [in] The item that owns the choice.
	u0109__test__expect_choice* pExpectedChoice, //< [in] The choice to verify the expected relations on.
	SG_resolve*                 pResolve,        //< [in] The resolve that owns the actual item and choice.
	SG_resolve__item*           pActualItem,     //< [in] The item that owns the actual choice.
	SG_resolve__choice*         pActualChoice    //< [in] The resolve choice to verify the expected relations against.
	)
{
	SG_vector*                    pUsedCollisionRelations = NULL;
	SG_vector*                    pUsedCycleRelations     = NULL;
	u0109__test__expect_relation* pExpectedRelation       = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pExpectedItem);
	SG_NULLARGCHECK(pExpectedChoice);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pActualItem);
	SG_NULLARGCHECK(pActualChoice);

	// allocate a vector to track all the actual relations that we matched to expected ones
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pUsedCollisionRelations, MAX_EXPECT_RELATIONS)  );
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pUsedCycleRelations, MAX_EXPECT_RELATIONS)  );

	// verify each of the choice's expected COLLISION relations
	pExpectedRelation = pExpectedChoice->aRelations;
	while (pExpectedRelation->bValid != SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _verify_expectations__relation(pCtx, pTest, EXPECT_RELATION_COLLISION, pExpectedItem, pExpectedChoice, pExpectedRelation, pResolve, pActualItem, pActualChoice, pUsedCollisionRelations)  );
		++pExpectedRelation;
	}

	// run through each actual COLLISION relation and make sure we matched it to an expected one
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_colliding_item(pCtx, pActualChoice, _verify_expectations__relation__must_appear, pUsedCollisionRelations)  );

	// verify each of the choice's expected CYCLE relations
	pExpectedRelation = pExpectedChoice->aRelations;
	while (pExpectedRelation->bValid != SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _verify_expectations__relation(pCtx, pTest, EXPECT_RELATION_CYCLE, pExpectedItem, pExpectedChoice, pExpectedRelation, pResolve, pActualItem, pActualChoice, pUsedCycleRelations)  );
		++pExpectedRelation;
	}

	// run through each actual CYCLE relation and make sure we matched it to an expected one
	VERIFY_ERR_CHECK(  SG_resolve__choice__foreach_cycling_item(pCtx, pActualChoice, _verify_expectations__relation__must_appear, pUsedCycleRelations)  );

fail:
	SG_VECTOR_NULLFREE(pCtx, pUsedCollisionRelations);
	SG_VECTOR_NULLFREE(pCtx, pUsedCycleRelations);
	return;
}


/*
**
** Helpers - Verification - Choice
**
*/

/**
 * SG_resolve__choice__callback that errors if any choice doesn't appear in a vector.
 */
static void _verify_expectations__choice__must_appear(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The current choice.
	void*               pUserData, //< [in] The vector to check for the choice in.
	SG_bool*            pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_vector* pVector     = (SG_vector*)pUserData;
	SG_bool    bContains   = SG_FALSE;
	SG_string* sInfo       = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, (void*)pChoice, &bContains)  );
	if (bContains == SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _resolve_choice__build_info_string(pCtx, pChoice, &sInfo)  );
		VERIFYP_COND_FAIL("Unexpected choice", bContains != SG_FALSE, ("%s", SG_string__sz(sInfo)));
	}

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * User data used by _verify_expectations__choice__match.
 */
typedef struct _verify_expectations__choice__match__data
{
	u0109__test*                pTest;           //< [in] The test we're verifying in.
	u0109__test__expect_item*   pExpectedItem;   //< [in] The item that owns the choice.
	u0109__test__expect_choice* pExpectedChoice; //< [in] The expectation to match against.
	SG_resolve*                 pResolve;        //< [in] The resolve that owns the actual item.
	SG_resolve__item*           pActualItem;     //< [in] The item being searched for a matching choice.
	SG_resolve__choice*         pMatch;          //< [out] The choice that matched the expectation.
}
_verify_expectations__choice__match__data;

/**
 * SG_resolve__choice__callback that matches against an expected choice.
 */
static void _verify_expectations__choice__match(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The current choice.
	void*               pUserData, //< [in] An instance of _verify_expectations__choice__match__data.
	SG_bool*            pContinue  //< [out] Whether or not iteration should continue.
	)
{
	_verify_expectations__choice__match__data* pData              = (_verify_expectations__choice__match__data*)pUserData;
	SG_resolve__state                          eState             = SG_RESOLVE__STATE__COUNT;
	SG_mrg_cset_entry_conflict_flags           eConflictCauses    = SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO;
	SG_bool                                    bCollisionCause    = SG_FALSE;
	SG_bool                                    bMergeable         = SG_FALSE;
	SG_bool                                    bResolved          = SG_FALSE;
	SG_bool                                    bDeleted           = SG_FALSE;
	SG_resolve__value*                         pAcceptedValue     = NULL;
	const char*                                szAcceptedLabel    = NULL;
	SG_bool                                    bExpectedMergeable = SG_FALSE;
	SG_bool                                    bExpectedResolved  = SG_FALSE;
	SG_bool                                    bExpectedDeleted   = SG_FALSE;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	VERIFY_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_causes(pCtx, pChoice, &eConflictCauses, &bCollisionCause, NULL)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_resolved(pCtx, pChoice, &bResolved, &bDeleted)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAcceptedValue)  );
	if (pAcceptedValue != NULL)
	{
		VERIFY_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pAcceptedValue, &szAcceptedLabel)  );
	}

	if (
		   pData->pExpectedChoice->eState == SG_RESOLVE__STATE__CONTENTS
		&& pData->pExpectedItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE
		)
	{
		bExpectedMergeable = SG_TRUE;
	}
	VERIFY_ERR_CHECK(  _expect_choice__get_resolved(pCtx, pData->pExpectedItem, pData->pExpectedChoice, &bExpectedResolved, &bExpectedDeleted)  );


	if (
		   pData->pExpectedChoice->eState == eState
		&& pData->pExpectedChoice->eConflictCauses == eConflictCauses
		&& pData->pExpectedChoice->bCollisionCause == bCollisionCause
		&& bExpectedMergeable == bMergeable
		&& bExpectedResolved == bResolved
		&& bExpectedDeleted == bDeleted
		&& SG_strcmp__null(pData->pExpectedChoice->szAcceptedValueLabel, szAcceptedLabel) == 0
		)
	{
		pData->pMatch = pChoice;
		*pContinue = SG_FALSE;
	}

fail:
	return;
}

/**
 * Verifies that an item contains a choice that matches a given expected choice.
 */
static void _verify_expectations__choice(
	SG_context*                 pCtx,            //< [in] [out] Error and context info.
	u0109__test*                pTest,           //< [in] The test we're verifying in.
	u0109__test__expect_item*   pExpectedItem,   //< [in] The item that owns the expected choice.
	u0109__test__expect_choice* pExpectedChoice, //< [in] The expected choice to verify against.
	SG_resolve*                 pResolve,        //< [in] The resolve that owns the actual item.
	SG_resolve__item*           pActualItem,     //< [in] The item to match a choice in.
	SG_vector*                  pUsedChoices     //< [in] A vector to add the matched choice to.
	)
{
	_verify_expectations__choice__match__data cFindData;
	SG_string*                                sInfo     = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pExpectedItem);
	SG_NULLARGCHECK(pExpectedChoice);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pActualItem);

	// find the resolve choice that matches our expectation
	cFindData.pTest           = pTest;
	cFindData.pExpectedItem   = pExpectedItem;
	cFindData.pExpectedChoice = pExpectedChoice;
	cFindData.pResolve        = pResolve;
	cFindData.pActualItem     = pActualItem;
	cFindData.pMatch          = NULL;
	VERIFY_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pActualItem, SG_FALSE, _verify_expectations__choice__match, (void*)&cFindData)  );
	if (cFindData.pMatch == NULL)
	{
		VERIFY_ERR_CHECK(  _expect_choice__build_info_string(pCtx, pExpectedItem, pExpectedChoice, &sInfo)  );
		VERIFYP_COND_FAIL("Expected choice not found", cFindData.pMatch != NULL, ("%s", SG_string__sz(sInfo)));
	}

	// match the choice's values
	VERIFY_ERR_CHECK(  _verify_expectations__values(pCtx, pTest, pExpectedItem, pExpectedChoice, pResolve, pActualItem, cFindData.pMatch)  );

	// match the choice's relations
	VERIFY_ERR_CHECK(  _verify_expectations__relations(pCtx, pTest, pExpectedItem, pExpectedChoice, pResolve, pActualItem, cFindData.pMatch)  );

	// if we have a vector to add the matched choice to, add it
	if (pUsedChoices != NULL)
	{
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pUsedChoices, (void*)cFindData.pMatch, NULL)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * Verifies that all choices in an expectated item exactly match all choices in the corresponding WD item.
 */
static void _verify_expectations__choices(
	SG_context*               pCtx,          //< [in] [out] Error and context info.
	u0109__test*              pTest,         //< [in] The test we're verifying in.
	u0109__test__expect_item* pExpectedItem, //< [in] The item to verify the expected choices on.
	SG_resolve*               pResolve,      //< [in] The resolve that owns the actual item.
	SG_resolve__item*         pActualItem    //< [in] The resolve item to verify the expected choices against.
	)
{
	SG_vector*                  pUsedChoices    = NULL;
	u0109__test__expect_choice* pExpectedChoice = pExpectedItem->aChoices;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pExpectedItem);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pActualItem);

	// allocate a vector to track all the actual choices that we matched to expected ones
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pUsedChoices, MAX_EXPECT_CHOICES)  );

	// verify each of the item's expected choices
	while (pExpectedChoice->bValid != SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _verify_expectations__choice(pCtx, pTest, pExpectedItem, pExpectedChoice, pResolve, pActualItem, pUsedChoices)  );
		++pExpectedChoice;
	}

	// run through each actual choice and make sure we matched it to an expected one
	VERIFY_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pActualItem, SG_FALSE, _verify_expectations__choice__must_appear, pUsedChoices)  );

fail:
	SG_VECTOR_NULLFREE(pCtx, pUsedChoices);
	return;
}


/*
**
** Helpers - Verification - Item
**
*/

/**
 * SG_resolve__item__callback that errors if any item doesn't appear in a vector.
 */
static void _verify_expectations__item__must_appear(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current item.
	void*             pUserData, //< [in] The vector to check for the item in.
	SG_bool*          pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_vector* pVector   = (SG_vector*)pUserData;
	SG_bool    bContains = SG_FALSE;
	SG_string* sInfo     = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, (void*)pItem, &bContains)  );
	if (bContains == SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _resolve_item__build_info_string(pCtx, pItem, &sInfo)  );
		VERIFYP_COND_FAIL("Unexpected item", bContains != SG_FALSE, ("%s", SG_string__sz(sInfo)));
	}

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * User data used by _verify_expectations__item__match.
 */
typedef struct _verify_expectations__item__match__data
{
	u0109__test*              pTest;         //< [in] The test we're verifying in.
	u0109__test__expect_item* pExpectedItem; //< [in] The expectation to match against.
	SG_resolve*               pResolve;      //< [in] The resolve that we're searching for a matching item.
	SG_resolve__item*         pMatch;        //< [out] The item that matched the expectation.
}
_verify_expectations__item__match__data;

/**
 * SG_resolve__item__callback that matches against an expected item.
 */
static void _verify_expectations__item__match(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current item.
	void*             pUserData, //< [in] An instance of _verify_expectations__item__match__data.
	SG_bool*          pContinue  //< [out] Whether or not iteration should continue.
	)
{
	_verify_expectations__item__match__data* pData          = (_verify_expectations__item__match__data*)pUserData;
	SG_treenode_entry_type                   eType          = SG_TREENODEENTRY_TYPE__INVALID;
	const char*                              szAncestorPath = NULL;
	const char*                              szBaselinePath = NULL;
	const char*                              szOtherPath    = NULL;
	SG_bool                                  bResolved      = SG_FALSE;
	SG_bool                                  bDeleted       = SG_FALSE;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	VERIFY_ERR_CHECK(  SG_resolve__item__get_type(pCtx, pItem, &eType)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__ancestor(pCtx, pItem, &szAncestorPath)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__baseline(pCtx, pItem, &szBaselinePath)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__other(pCtx, pItem, 1u, &szOtherPath)  );
	VERIFY_ERR_CHECK(  SG_resolve__item__get_resolved(pCtx, pItem, &bResolved, &bDeleted)  );

	if (
		   pData->pExpectedItem->eType == eType
		&& pData->pExpectedItem->bResolved == bResolved
		&& pData->pExpectedItem->bDeleted == bDeleted
		&& SG_stricmp__null(pData->pExpectedItem->szAncestorRepoPath, szAncestorPath) == 0
		&& SG_stricmp__null(pData->pExpectedItem->szBaselineRepoPath, szBaselinePath) == 0
		&& SG_stricmp__null(pData->pExpectedItem->szOtherRepoPath, szOtherPath) == 0
		)
	{
		pData->pMatch = pItem;
		*pContinue = SG_FALSE;
	}

fail:
	return;
}

/**
 * Verifies that a resolve contains an item that matches a given expected item.
 */
static void _verify_expectations__item(
	SG_context*               pCtx,          //< [in] [out] Error and context info.
	u0109__test*              pTest,         //< [in] The test we're running in.
	u0109__test__expect_item* pExpectedItem, //< [in] The expected item to verify against.
	SG_resolve*               pResolve,      //< [in] The resolve to match an item in.
	SG_vector*                pUsedItems     //< [in] A vector to add the matched item to.
	)
{
	_verify_expectations__item__match__data cFindData;
	SG_string*                              sInfo     = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pExpectedItem);
	SG_NULLARGCHECK(pResolve);

	// find the resolve item that matches our expectation
	cFindData.pTest         = pTest;
	cFindData.pExpectedItem = pExpectedItem;
	cFindData.pResolve      = pResolve;
	cFindData.pMatch        = NULL;
	VERIFY_ERR_CHECK(  SG_resolve__foreach_item(pCtx, pResolve, SG_FALSE, _verify_expectations__item__match, (void*)&cFindData)  );
	if (cFindData.pMatch == NULL)
	{
		VERIFY_ERR_CHECK(  _expect_item__build_info_string(pCtx, pExpectedItem, &sInfo)  );
		VERIFYP_COND_FAIL("Expected item not found", cFindData.pMatch != NULL, ("%s", SG_string__sz(sInfo)));
	}

	// add this item to the list of known items
	if (pExpectedItem->szName != NULL)
	{
		const char* szActualGid   = NULL;
		const char* szExistingGid = NULL;

		VERIFY_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, cFindData.pMatch, &szActualGid)  );
		VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTest->pItemsGids, pExpectedItem->szName, &szExistingGid)  );
		VERIFYP_COND("Item nickname assigned to multiple GIDs.", szExistingGid == NULL || SG_stricmp(szActualGid, szExistingGid) == 0, ("ItemNickname(%s) ExistingGid(%s) NewGid(%s)", pExpectedItem->szName, szExistingGid, szActualGid));
		if (szExistingGid == NULL)
		{
			VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTest->pItemsGids, pExpectedItem->szName, szActualGid)  );
		}
	}

	// if we have a vector to add the matched item to, add it
	if (pUsedItems != NULL)
	{
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pUsedItems, (void*)cFindData.pMatch, NULL)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, sInfo);
	return;
}

/**
 * Verifies that all items in the expectations exactly match all items in the WD.
 */
static void _verify_expectations__items(
	SG_context*  pCtx,    //< [in] [out] Error and context info.
	u0109__test* pTest,   //< [in] The test to verify items in.
	SG_resolve*  pResolve //< [in] The resolve data to verify against.
	)
{
	SG_vector*                pUsedItems    = NULL;
	u0109__test__expect_item* pExpectedItem = pTest->aExpectedItems;
	SG_uint32                 uItemCount    = 0u;
	SG_uint32                 uUsedCount    = 0u;
	SG_uint32                 uIndex        = 0u;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pResolve);

	// verify that we find actual items for all our expected items
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pUsedItems, MAX_EXPECT_ITEMS)  );
	while (pExpectedItem->bValid != SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _verify_expectations__item(pCtx, pTest, pExpectedItem, pResolve, pUsedItems)  );
		++pExpectedItem;
		++uItemCount;
	}

	// verify that there are no actual items that weren't matched
	VERIFY_ERR_CHECK(  SG_resolve__foreach_item(pCtx, pResolve, SG_FALSE, _verify_expectations__item__must_appear, pUsedItems)  );

	// if all the items successfully matched
	// then go ahead and verify each item's choices
	// (if they didn't all match, we can't know which expected items go with which actual used ones)
	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pUsedItems, &uUsedCount)  );
	if (uItemCount == uUsedCount)
	{
		// run through and match each item's choices
		// We purposefully do this AFTER we've already matched all the items
		// so that each item's name/GID pair is already cached before we match choices/values
		// because we sometimes need to translate them (even ones that appear later in the test)
		// to match choices/values properly.
		pExpectedItem = pTest->aExpectedItems;
		while (pExpectedItem->bValid != SG_FALSE)
		{
			SG_resolve__item* pActualItem = NULL;

			VERIFY_ERR_CHECK(  SG_vector__get(pCtx, pUsedItems, uIndex, (void**)&pActualItem)  );
			VERIFY_ERR_CHECK(  _verify_expectations__choices(pCtx, pTest, pExpectedItem, pResolve, pActualItem)  );

			++pExpectedItem;
			++uIndex;
		}
	}


fail:
	SG_VECTOR_NULLFREE(pCtx, pUsedItems);
	return;
}

/**
 * Verifies that the unresolved count in both the
 * resolve system and pendingtree data match expectations.
 */
static void _verify_expectations__unresolved_counts(
	SG_context*  pCtx,
	u0109__test* pTest,
	SG_resolve*  pResolve
	)
{
	SG_pendingtree*           pPendingTree      = NULL;
	u0109__test__expect_item* pItem             = pTest->aExpectedItems;
	SG_uint32                 uResolveCount     = 0u;
	SG_uint32                 uPendingTreeCount = 0u;
	SG_uint32                 uExpectedCount    = 0u;

	VERIFY_ERR_CHECK(  SG_resolve__get_pendingtree(pCtx, pResolve, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_resolve__get_unresolved_count(pCtx, pResolve, NULL, &uResolveCount)  );
	VERIFY_ERR_CHECK(  SG_pendingtree__count_unresolved_wd_issues(pCtx, pPendingTree, &uPendingTreeCount)  );

	while (pItem->bValid != SG_FALSE)
	{
		if (pItem->bResolved == SG_FALSE)
		{
			++uExpectedCount;
		}
		++pItem;
	}

	VERIFYP_COND("Expected unresolved count doesn't match pendingtree.", uExpectedCount == uPendingTreeCount, ("Expected(%d) PendingTree(%d)", uExpectedCount, uPendingTreeCount));
	VERIFYP_COND("Expected unresolved count doesn't match resolve.", uExpectedCount == uResolveCount, ("Expected(%d) Resolve(%d)", uExpectedCount, uResolveCount));

fail:
	return;
}


/*
**
** Helpers - Verification
**
*/

/**
 * Verifies that a test's current WD state matches expectations.
 */
static void _verify_expectations(
	SG_context*  pCtx,    //< [in] [out] Error and context info.
	u0109__test* pTest,   //< [in] The test to verify.
	SG_resolve*  pResolve //< [in] The resolve data to verify against.
	)
{
	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pResolve);

	VERIFY_ERR_CHECK(  _verify_expectations__items(pCtx, pTest, pResolve)  );
	VERIFY_ERR_CHECK(  _verify_expectations__unresolved_counts(pCtx, pTest, pResolve)  );

fail:
	return;
}




/*
**
** Helpers - Resolve
**
*/

/**
 * Runs an ACCEPT resolve step.
 */
static void _execute_resolve_step__accept(
	SG_context*       pCtx,           //< [in] [out] Error and context info.
	u0109__test*      pTest,          //< [in] The test that's being executed.
	const char*       szItemName,     //< [in] The name of the item to accept a value on.
	SG_resolve__state eState,         //< [in] The conflicting state on the item to accept a value for.
	const char*       szValueName,    //< [in] The name of the value to accept.
	const char*       szWorkingValue, //< [in] The name of the new saved working value to create.
	                                  //<      Necessary if the current live working value has edits.
	                                  //<      NULL if no new working value is necessary.
	SG_resolve*       pResolve        //< [in] The resolve data to accept the value in.
	)
{
	u0109__test__expect_item*   pExpectedItem     = NULL;
	u0109__test__expect_choice* pExpectedChoice   = NULL;
	u0109__test__expect_value*  pExpectedValue    = NULL;
	SG_resolve__item*           pActualItem       = NULL;
	SG_resolve__choice*         pActualChoice     = NULL;
	const char*                 szValueLabel      = NULL;
	SG_resolve__value*          pActualValue      = NULL;
	u0109__test__expect_value*  pWorkingValue     = NULL;
	u0109__test__expect_value*  pExpectedNewValue = NULL;
	u0109__test__expect_value*  pCurrentValue     = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szItemName);
	SG_NULLARGCHECK(szValueName);
	SG_NULLARGCHECK(pResolve);

	// get the expected and actual choices that own the relevant value
	VERIFY_ERR_CHECK(  _test__find_choices(pCtx, pTest, pResolve, szItemName, eState, &pExpectedItem, &pExpectedChoice, &pActualItem, &pActualChoice)  );

	// get the expected value from the expected choice
	VERIFY_ERR_CHECK(  _expect_choice__get_value__name(pCtx, pExpectedChoice, szValueName, &pExpectedValue)  );

	// convert the step's value name into its label
	// then retrieve the actual value using that
	VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pExpectedChoice->pValues, szValueName, &szValueLabel)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pActualChoice, szValueLabel, 0u, &pActualValue)  );

	// accept the value
	VERIFY_ERR_CHECK(  SG_resolve__accept_value(pCtx, pActualValue)  );

	// find the current expected working value
	VERIFY_ERR_CHECK(  _expect_choice__get_value__live_working(pCtx, pExpectedChoice, &pWorkingValue)  );

	// if we need to create a new working value, do that
	// this occurs if the current working value gets saved before
	// being overwritten by the newly accepted value
	if (szWorkingValue != NULL)
	{
		// create the new expected value
		// this value represents the saved version of the previous working value
		VERIFY_ERR_CHECK(  _expect_choice__add_value(pCtx, pExpectedChoice, &pExpectedNewValue)  );
		pExpectedNewValue->bAutomatic           = SG_FALSE;
		pExpectedNewValue->bData                = SG_FALSE;
		pExpectedNewValue->bLeaf                = SG_FALSE;
		pExpectedNewValue->bLive                = SG_FALSE;
		pExpectedNewValue->bMWModified          = SG_FALSE;
		pExpectedNewValue->eType                = EXPECT_VALUE_CHANGESET;
		pExpectedNewValue->iData                = 0;
		pExpectedNewValue->iMergeToolResult     = SG_FILETOOL__RESULT__COUNT;
		pExpectedNewValue->szBaselineParentName = NULL;
		pExpectedNewValue->szCommitName         = NULL;
		pExpectedNewValue->szData               = NULL;
		pExpectedNewValue->szMergeName          = NULL;
		pExpectedNewValue->szMWParentName       = NULL;
		pExpectedNewValue->szName               = szWorkingValue;
		pExpectedNewValue->szOtherParentName    = NULL;

		// add the new value's label to the expected choice's value map
		// note that the new value's label is equal to the name given to it during the test
		VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pExpectedChoice->pValues, szWorkingValue, szWorkingValue)  );

		// the new saved working value has the same data and mergable/working info as the current live one
		VERIFY_COND("Expected choice doesn't contain an expected live working value.", pWorkingValue != NULL);
		if (pWorkingValue != NULL)
		{
			pExpectedNewValue->bData          = pWorkingValue->bData;
			pExpectedNewValue->iData          = pWorkingValue->iData;
			pExpectedNewValue->szData         = pWorkingValue->szData;
			pExpectedNewValue->bMWModified    = pWorkingValue->bMWModified;
			pExpectedNewValue->szMWParentName = pWorkingValue->szMWParentName;
		}
	}

	// update the current expected working value
	if (pWorkingValue == NULL)
	{
		// there isn't actually a working value
		// but there will be now since we just accepted something on it
		// so make a new expected working value
		VERIFY_ERR_CHECK(  _expect_choice__add_value(pCtx, pExpectedChoice, &pWorkingValue)  );
		pWorkingValue->bAutomatic           = SG_FALSE;
		pWorkingValue->bData                = SG_FALSE;
		pWorkingValue->bLeaf                = SG_FALSE;
		pWorkingValue->bLive                = SG_TRUE;
		pWorkingValue->bMWModified          = SG_FALSE;
		pWorkingValue->eType                = EXPECT_VALUE_CHANGESET;
		pWorkingValue->iData                = 0;
		pWorkingValue->iMergeToolResult     = SG_FILETOOL__RESULT__COUNT;
		pWorkingValue->szBaselineParentName = NULL;
		pWorkingValue->szCommitName         = NULL; // working values are signified by NULL commit
		pWorkingValue->szData               = pExpectedValue->szData;
		pWorkingValue->szMergeName          = NULL;
		pWorkingValue->szMWParentName       = NULL;
		pWorkingValue->szName               = NULL; // no sense giving it a name when nothing can already be referring to it
		pWorkingValue->szOtherParentName    = NULL;

		// if this is on a mergeable choice, set the working parent to the new value
		if (
			   pExpectedItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE
			&& pExpectedChoice->eState == SG_RESOLVE__STATE__CONTENTS
			)
		{
			pWorkingValue->szMWParentName = szValueName;
		}
	}
	else
	{
		// update the expected working value data to match what we just did
		switch (pExpectedChoice->eState)
		{
		case SG_RESOLVE__STATE__EXISTENCE:
			pWorkingValue->bData = pExpectedValue->bData;
			break;
		case SG_RESOLVE__STATE__ATTRIBUTES:
			pWorkingValue->iData = pExpectedValue->iData;
			break;
		case SG_RESOLVE__STATE__NAME:
		case SG_RESOLVE__STATE__LOCATION:
		case SG_RESOLVE__STATE__CONTENTS:
			pWorkingValue->szData = pExpectedValue->szData;
			break;
		default:
			VERIFYP_COND_FAIL("Unsupported state type", SG_FALSE, ("ChoiceState(%d)", eState));
			break;
		}

		// if this is on a mergeable choice, update the mergeable working data
		if (
			   pExpectedItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE
			&& pExpectedChoice->eState == SG_RESOLVE__STATE__CONTENTS
			)
		{
			// update the working value's parent to be the accepted value
			pWorkingValue->szMWParentName = szValueName;

			// since we just made its value equal to its parent's
			// the working value is not modified relative to its parent
			pWorkingValue->bMWModified = SG_FALSE;
		}
	}

	// update the expected choice's accepted value
	pExpectedChoice->szAcceptedValueLabel = szValueLabel;

	// update the expected item's resolve status
	pExpectedItem->bResolved = SG_TRUE;
	if (pExpectedChoice->eState == SG_RESOLVE__STATE__EXISTENCE)
	{
		pExpectedItem->bDeleted = pExpectedValue->bData == SG_FALSE ? SG_TRUE : SG_FALSE;
	}

	// now that the choice is resolved
	// we no longer expect it to have any leaf values
	pCurrentValue = pExpectedChoice->aValues;
	while (pCurrentValue->bValid == SG_TRUE)
	{
		pCurrentValue->bLeaf = SG_FALSE;
		++pCurrentValue;
	}

fail:
	return;
}

/**
 * Runs a MERGE resolve step.
 */
static void _execute_resolve_step__merge(
	SG_context*       pCtx,              //< [in] [out] Error and context info.
	u0109__test*      pTest,             //< [in] The test that's being executed.
	const char*       szItemName,        //< [in] The name of the item to accept a value on.
	SG_resolve__state eState,            //< [in] The conflicting state on the item to accept a value for.
	const char*       szBaselineName,    //< [in] The name of the baseline value to merge.
	const char*       szOtherName,       //< [in] The name of the other value to merge.
	const char*       szMergedName,      //< [in] The name and label to assign to the merged value.
	const char*       szMergeTool,       //< [in] The name of the merge tool to use.
	SG_int32          iMergeToolResult,  //< [in] The expected result of the merge tool.
	const char*       szMergedContents,  //< [in] The name of the expected merged contents.
	SG_bool           bAccept,           //< [in] Whether or not merge should automatically accept the last leaf.
	SG_bool           bResolved,         //< [in] Whether or not the merged value is expected to be accepted as the resolution.
	const char*       szWorkingValue,    //< [in] The name of the new live working value to create.
	                                     //<      Necessary if the current live working value is one of the merge parents.
	                                     //<      NULL if no new working value is necessary.
	SG_resolve*       pResolve           //< [in] The resolve data to accept the value in.
	)
{
	u0109__test__expect_item*   pExpectedItem     = NULL;
	u0109__test__expect_choice* pExpectedChoice   = NULL;
	SG_resolve__item*           pActualItem       = NULL;
	SG_resolve__choice*         pActualChoice     = NULL;
	u0109__test__expect_value*  pExpectedBaseline = NULL;
	u0109__test__expect_value*  pExpectedOther    = NULL;
	const char*                 szBaselineLabel   = NULL;
	const char*                 szOtherLabel      = NULL;
	SG_int64                    iTimeStart        = 0;
	SG_int64                    iTimeEnd          = 0;
	SG_resolve__value*          pActualBaseline   = NULL;
	SG_resolve__value*          pActualOther      = NULL;
	SG_int32                    iResult           = SG_FILETOOL__RESULT__COUNT;
	SG_resolve__value*          pResult           = NULL;
	u0109__test__expect_value*  pExpectedNewValue = NULL;

	// get the expected and actual choices that own the relevant value
	VERIFY_ERR_CHECK(  _test__find_choices(pCtx, pTest, pResolve, szItemName, eState, &pExpectedItem, &pExpectedChoice, &pActualItem, &pActualChoice)  );

	// get the expected values to be merged
	VERIFY_ERR_CHECK(  _expect_choice__get_value__name(pCtx, pExpectedChoice, szBaselineName, &pExpectedBaseline)  );
	VERIFY_ERR_CHECK(  _expect_choice__get_value__name(pCtx, pExpectedChoice, szOtherName, &pExpectedOther)  );

	// convert the step's value names into their labels
	// then retrieve the actual values using those
	VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pExpectedChoice->pValues, szBaselineName, &szBaselineLabel)  );
	VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx, pExpectedChoice->pValues, szOtherName, &szOtherLabel)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pActualChoice, szBaselineLabel, 0u, &pActualBaseline)  );
	VERIFY_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pActualChoice, szOtherLabel, 0u, &pActualOther)  );

	// run the merge on the values
	VERIFY_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &iTimeStart)  );
#ifdef TIMESTAMP_CHECK_DELAY
	VERIFY_ERR_CHECK(  SG_sleep_ms(TIMESTAMP_CHECK_DELAY)  );
#endif
	VERIFY_ERR_CHECK(  SG_resolve__merge_values(pCtx, pActualBaseline, pActualOther, szMergeTool, szMergedName, bAccept, &iResult, &pResult, NULL)  );
	VERIFYP_COND_FAIL("Result of merge doesn't match expectation.", iResult == iMergeToolResult, ("ExpectedResult(%d) ActualResult(%d)", iMergeToolResult, iResult));
	VERIFY_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &iTimeEnd)  );

	// if successful, do some additional work
	if (pResult != NULL)
	{
		// generate a new merge name for this merge
		const char* szMergeName = _test__get_merge_name(pTest);

		// should never get a result value along with a CANCEL result code
		VERIFYP_COND("Unexpected merge result code.", iResult != SG_MERGETOOL__RESULT__CANCEL, ("iResult(%d)", iResult));

		// file the merge times away under the name
		VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTest->pMergesStartTimes, szMergeName, iTimeStart)  );
		VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTest->pMergesEndTimes, szMergeName, iTimeEnd)  );

		// create a new expected value to reflect the newly merged actual value
		VERIFY_ERR_CHECK(  _expect_choice__add_value(pCtx, pExpectedChoice, &pExpectedNewValue)  );
		pExpectedNewValue->bAutomatic           = SG_FALSE;
		pExpectedNewValue->bData                = SG_FALSE;
		pExpectedNewValue->bLeaf                = SG_FALSE;
		pExpectedNewValue->bLive                = SG_FALSE;
		pExpectedNewValue->bMWModified          = SG_FALSE;
		pExpectedNewValue->eType                = EXPECT_VALUE_MERGE;
		pExpectedNewValue->iData                = 0;
		pExpectedNewValue->iMergeToolResult     = iResult;
		pExpectedNewValue->szBaselineParentName = szBaselineName;
		pExpectedNewValue->szCommitName         = NULL;
		pExpectedNewValue->szData               = szMergedContents;
		pExpectedNewValue->szMergeName          = szMergeName;
		pExpectedNewValue->szMWParentName       = NULL;
		pExpectedNewValue->szName               = szMergedName;
		pExpectedNewValue->szOtherParentName    = szOtherName;

		// add the new value's label to the expected choice's value map
		// note that the new value's label is equal to the name given to it during the test
		VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pExpectedChoice->pValues, szMergedName, szMergedName)  );

		// if the merge was successful, update which values are leaves
		if (iResult == SG_FILETOOL__RESULT__SUCCESS)
		{
			if (pExpectedBaseline->bLeaf == SG_TRUE || pExpectedOther->bLeaf == SG_TRUE)
			{
				pExpectedNewValue->bLeaf = SG_TRUE;
				pExpectedBaseline->bLeaf = SG_FALSE;
				pExpectedOther->bLeaf = SG_FALSE;
			}
		}

		// if we need to create a new working value, do that
		// This is necessary in cases where the current working value is a parent
		// of the merge and has edits that we don't want to lose.  The parent
		// becomes a saved working value and we need a new live working value.
		// The test provided the details to create this new value.
		if (szWorkingValue != NULL)
		{
			u0109__test__expect_value* pOldLiveWorking = NULL;

			// create the new expected value
			VERIFY_ERR_CHECK(  _expect_choice__add_value(pCtx, pExpectedChoice, &pExpectedNewValue)  );
			pExpectedNewValue->bAutomatic           = SG_FALSE;
			pExpectedNewValue->bData                = SG_FALSE;
			pExpectedNewValue->bLeaf                = SG_FALSE;
			pExpectedNewValue->bLive                = SG_TRUE;
			pExpectedNewValue->bMWModified          = SG_FALSE;
			pExpectedNewValue->eType                = EXPECT_VALUE_CHANGESET;
			pExpectedNewValue->iData                = 0;
			pExpectedNewValue->iMergeToolResult     = SG_FILETOOL__RESULT__COUNT;
			pExpectedNewValue->szBaselineParentName = NULL;
			pExpectedNewValue->szCommitName         = NULL;
			pExpectedNewValue->szData               = NULL;
			pExpectedNewValue->szMergeName          = NULL;
			pExpectedNewValue->szMWParentName       = NULL;
			pExpectedNewValue->szName               = szWorkingValue;
			pExpectedNewValue->szOtherParentName    = NULL;

			// add the new value's label to the expected choice's value map
			// note that the new value's label is equal to the name given to it during the test
			VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pExpectedChoice->pValues, szWorkingValue, szWorkingValue)  );

			// check which parent was the old live modified working value
			if (
				   pExpectedBaseline->eType == EXPECT_VALUE_CHANGESET
				&& pExpectedBaseline->szCommitName == NULL
				&& pExpectedBaseline->bLive == SG_TRUE
				)
			{
				pOldLiveWorking = pExpectedBaseline;
			}
			else if (
				   pExpectedOther->eType == EXPECT_VALUE_CHANGESET
				&& pExpectedOther->szCommitName == NULL
				&& pExpectedOther->bLive == SG_TRUE
				)
			{
				pOldLiveWorking = pExpectedOther;
			}

			// update the old and new working values
			VERIFY_COND("MERGE specified creating a new live working value, but neither parent was the existing live working value.", pOldLiveWorking != NULL);
			if (pOldLiveWorking != NULL)
			{
				// the old live working value is no longer live
				pOldLiveWorking->bLive = SG_FALSE;

				// the new value has the same data as the old
				pExpectedNewValue->bData  = pOldLiveWorking->bData;
				pExpectedNewValue->iData  = pOldLiveWorking->iData;
				pExpectedNewValue->szData = pOldLiveWorking->szData;

				// the new value has the same mergeable/working info as the old
				pExpectedNewValue->bMWModified    = pOldLiveWorking->bMWModified;
				pExpectedNewValue->szMWParentName = pOldLiveWorking->szMWParentName;
			}
		}

		// update the expected item's resolve status
		pExpectedItem->bResolved = bResolved;
	}

fail:
	return;
}

/**
 * Runs a DELETE resolve step.
 */
static void _execute_resolve_step__delete(
	SG_context*  pCtx,       //< [in] [out] Error and context info.
	u0109__test* pTest,      //< [in] The test that's being executed.
	const char*  szRepoPath, //< [in] Repo path to delete.
	                         //<      If NULL, the item named by szItemName is deleted.
	const char*  szItemName, //< [in] If szRepoPath is NULL, the name of the item to delete.
	                         //<      If szRepoPath is non-NULL, the name of the item whose GID must be inserted into it.
	                         //<      May be NULL if szRepoPath indicates a non-temporary filename.
	SG_resolve*  pResolve    //< [in] The resolve data to work in.
	)
{
	u0109__test__expect_item*   pItem        = NULL;
	const char*                 szGid        = NULL;
	SG_pendingtree*             pPendingTree = NULL;
	SG_string*                  sRepoPath    = NULL;
	SG_repo*                    pRepo        = NULL;
	u0109__test__expect_choice* pChoice      = NULL;

	SG_NULLARGCHECK(pTest);

	// we'll need the resolve's pendingtree no matter what
	VERIFY_ERR_CHECK(  SG_resolve__get_pendingtree(pCtx, pResolve, &pPendingTree)  );

	// figure out the repo path we need to delete
	if (szRepoPath == NULL)
	{
		// none specified, get the repo path of the named item
		SG_NULLARGCHECK(szItemName);
		VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pTest, szItemName, &szGid)  );
		VERIFY_ERR_CHECK(  SG_pendingtree__get_repo_path(pCtx, pPendingTree, szGid, &sRepoPath)  );
	}
	else if (szItemName != NULL)
	{
		// we have a repo name and item name both
		// we need to insert the item's GID into the temporary repo path
		VERIFY_ERR_CHECK(  _test__build_temp_repo_path(pCtx, pTest, szRepoPath, szItemName, &sRepoPath)  );
	}
	szRepoPath = SG_string__sz(sRepoPath);

	// run the delete, just as if it happened during setup
	VERIFY_ERR_CHECK(  SG_pendingtree__get_repo(pCtx, pPendingTree, &pRepo)  );
	VERIFY_ERR_CHECK(  _execute_setup_step__delete(pCtx, pTest, szRepoPath, pPendingTree)  );

	// update our expectations for the item
	VERIFY_ERR_CHECK(  _test__get_item__name(pCtx, pTest, szItemName, &pItem)  );
	pItem->bResolved = SG_TRUE;
	pItem->bDeleted = SG_TRUE;

	// update our expectations for live working values on the item's choices
	pChoice = pItem->aChoices;
	while (pChoice->bValid == SG_TRUE)
	{
		u0109__test__expect_value* pValue = NULL;

		VERIFY_ERR_CHECK(  _expect_choice__get_value__live_working(pCtx, pChoice, &pValue)  );
		if (pValue != NULL)
		{
			if (pChoice->eState == SG_RESOLVE__STATE__EXISTENCE)
			{
				// a working value on an existence choice doesn't go away when the file is deleted
				// instead its value becomes false (non-existent)
				pValue->bData = SG_FALSE;
			}
			else
			{
				// a working value on any other type of choice goes away if the file is deleted
				// remove the expected working value
				while (pValue->bValid == SG_TRUE)
				{
					*pValue = *(pValue + 1u);
					++pValue;
				}
			}
		}

		++pChoice;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sRepoPath);
	return;
}

/**
 * Runs a MODIFY_FILE resolve step.
 */
static void _execute_resolve_step__modify(
	SG_context*                    pCtx,             //< [in] [out] Error and context info.
	u0109__test*                   pTest,            //< [in] The test that's being executed.
	const char*                    szItemName,       //< [in] The name of the item to modify.
	const char*                    szValueName,      //< [in] The name of the value whose data will be the file's new contents.
	                                                 //<      If the named value doesn't exist, it will be created.
	                                                 //<      If NULL, no value will be updated.
	                                                 //<      If non-NULL, szContentName must also be non-NULL.
	const char*                    szContentName,    //< [in] Name to assign to the item's new content.
	                                                 //<      If NULL, the new content won't be retrievable by name.
	                                                 //<      If NULL, szValueName must also be NULL.
	u0109__test__content_function* fContentFunction, //< [in] Function to use to modify the item's content.
	const char*                    szUserData,       //< [in] Value to pass to fContentFunction.
	SG_resolve*                    pResolve          //< [in] The resolve data to work in.
	)
{
	u0109__test__expect_item*   pItem        = NULL;
	u0109__test__expect_choice* pChoice      = NULL;
	u0109__test__expect_value*  pValue       = NULL;
	SG_pendingtree*             pPendingTree = NULL;
	SG_repo*                    pRepo        = NULL;
	const char*                 szGid        = NULL;
	SG_string*                  sRepoPath    = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szItemName);
	SG_NULLARGCHECK(fContentFunction);
	SG_ARGCHECK(szValueName == NULL || szContentName != NULL, szValueName+szContentName);

	// get the repo path of the named item
	VERIFY_ERR_CHECK(  _test__map_name__item_to_gid(pCtx, pTest, szItemName, &szGid)  );
	VERIFY_ERR_CHECK(  SG_resolve__get_pendingtree(pCtx, pResolve, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_pendingtree__get_repo_path(pCtx, pPendingTree, szGid, &sRepoPath)  );

	// run the modification, just as if it happened during setup
	VERIFY_ERR_CHECK(  SG_resolve__get_pendingtree(pCtx, pResolve, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_pendingtree__get_repo(pCtx, pPendingTree, &pRepo)  );
	VERIFY_ERR_CHECK(  _execute_setup_step__modify(pCtx, pTest, SG_string__sz(sRepoPath), szContentName, fContentFunction, szUserData, pRepo)  );

	// if we need to update a value with the new contents, do that
	if (szValueName != NULL && szContentName != NULL)
	{
		VERIFY_ERR_CHECK(  _test__get_item__name(pCtx, pTest, szItemName, &pItem)  );
		VERIFY_ERR_CHECK(  _expect_item__get_choice__state(pCtx, pItem, SG_RESOLVE__STATE__CONTENTS, &pChoice)  );
		VERIFY_ERR_CHECK(  _expect_choice__get_value__name(pCtx, pChoice, szValueName, &pValue)  );
		pValue->szData = szContentName;
		pValue->bLeaf = SG_TRUE;

		// if this is a live working value on a mergeable choice
		// then mark it as modified relative to its parent value
		if (
			   pItem->eType         == SG_TREENODEENTRY_TYPE_REGULAR_FILE
			&& pChoice->eState      == SG_RESOLVE__STATE__CONTENTS
			&& pValue->eType        == EXPECT_VALUE_CHANGESET
			&& pValue->szCommitName == NULL
			)
		{
			pValue->bMWModified = SG_TRUE;
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, sRepoPath);
	return;
}

/**
 * Executes a single resolve step.
 */
static void _execute_resolve_step(
	SG_context*                pCtx,    //< [in] [out] Error and context info.
	u0109__test*               pTest,   //< [in] The test that's being executed.
	u0109__test__resolve_step* pStep,   //< [in] The resolve step to execute.
	SG_resolve*                pResolve //< [in] The resolve data to use for execution.
	)
{
	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pStep);

	switch (pStep->eAction)
	{
	case RESOLVE_ACTION_ACCEPT:
		VERIFY_ERR_CHECK(  _execute_resolve_step__accept(pCtx, pTest, pStep->szItemName, pStep->eState, pStep->szValueName, pStep->szContentName, pResolve)  );
		break;
	case RESOLVE_ACTION_MERGE:
		VERIFY_ERR_CHECK(  _execute_resolve_step__merge(pCtx, pTest, pStep->szItemName, pStep->eState, pStep->szBaselineValueName, pStep->szOtherValueName, pStep->szValueName, pStep->szMergeTool, pStep->iMergeToolResult, pStep->szMergedContents, pStep->bAccept, pStep->bResolved, pStep->szContentName, pResolve)  );
		break;
	case RESOLVE_ACTION_DELETE:
		VERIFY_ERR_CHECK(  _execute_resolve_step__delete(pCtx, pTest, pStep->szValueName, pStep->szItemName, pResolve)  );
		break;
	case RESOLVE_ACTION_MODIFY:
		VERIFY_ERR_CHECK(  _execute_resolve_step__modify(pCtx, pTest, pStep->szItemName, pStep->szValueName, pStep->szContentName, pStep->fContentFunction, pStep->szContentData, pResolve)  );
		break;
	default:
		VERIFYP_COND_FAIL("Unsupported resolve action.", SG_FALSE, ("Action(%d)", pStep->eAction));
		break;
	}

fail:
	return;
}

/**
 * Dumps an ACCEPT resolve step.
 */
static void _dump_resolve_step__accept(
	SG_context*       pCtx,        //< [in] [out] Error and context info.
	u0109__test*      pTest,       //< [in] The test that's being executed.
	const char*       szItemName,  //< [in] The name of the item to accept a value on.
	SG_resolve__state eState,      //< [in] The conflicting state on the item to accept a value for.
	const char*       szValueName, //< [in] The name of the value to accept.
	SG_resolve*       pResolve,    //< [in] The resolve data to accept the value in.
	SG_string**       ppDump       //< [out] The dumped resolve step command(s).
	)
{
	u0109__test__expect_item*   pExpectedItem   = NULL;
	u0109__test__expect_choice* pExpectedChoice = NULL;
	SG_resolve__item*           pActualItem     = NULL;
	SG_resolve__choice*         pActualChoice   = NULL;
	const char*                 szRepoPath      = NULL;
	const char*                 szState         = NULL;
	const char*                 szValue         = NULL;
	SG_string*                  sDump           = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(szItemName);
	SG_NULLARGCHECK(szValueName);
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(ppDump);

	// get the item and choice to accept a value on
	VERIFY_ERR_CHECK(  _test__find_choices(pCtx, pTest, pResolve, szItemName, eState, &pExpectedItem, &pExpectedChoice, &pActualItem, &pActualChoice)  );

	// get the item's working repo path
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pActualItem, &szRepoPath)  );

	// get the choice's state type
	VERIFY_ERR_CHECK(  SG_resolve__state__get_info(pCtx, eState, &szState, NULL, NULL)  );

	// convert the step's value name into its label
	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pExpectedChoice->pValues, szValueName, &szValue)  );

	// format the command
	if (szValue == NULL)
	{
		VERIFY_ERR_CHECK(  SG_string__alloc__format(pCtx, &sDump, "%s Value not available: %s", gszDumpComment, szValueName)  );
	}
	else
	{
		VERIFY_ERR_CHECK(  SG_string__alloc__format(pCtx, &sDump, "vv resolve accept %s \"%s\" %s", szValue, szRepoPath, szState)  );
	}

	// return the dump
	*ppDump = sDump;
	sDump = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sDump);
	return;
}

/**
 * Dumps a MERGE resolve step.
 */
static void _dump_resolve_step__merge(
	SG_context*       pCtx,           //< [in] [out] Error and context info.
	u0109__test*      pTest,          //< [in] The test that's being executed.
	const char*       szItemName,     //< [in] The name of the item to accept a value on.
	SG_resolve__state eState,         //< [in] The conflicting state on the item to accept a value for.
	const char*       szBaselineName, //< [in] The name of the baseline value to merge.
	const char*       szOtherName,    //< [in] The name of the other value to merge.
	const char*       szMergeTool,    //< [in] The name of the merge tool to use.
	SG_resolve*       pResolve,       //< [in] The resolve data to accept the value in.
	SG_string**       ppDump          //< [out] The dumped resolve command(s).
	)
{
	u0109__test__expect_item*   pExpectedItem   = NULL;
	u0109__test__expect_choice* pExpectedChoice = NULL;
	SG_resolve__item*           pActualItem     = NULL;
	SG_resolve__choice*         pActualChoice   = NULL;
	const char*                 szRepoPath      = NULL;
	const char*                 szState         = NULL;
	const char*                 szBaseline      = NULL;
	const char*                 szOther         = NULL;
	SG_string*                  sDump           = NULL;

	// get the expected and actual choices that own the relevant value
	VERIFY_ERR_CHECK(  _test__find_choices(pCtx, pTest, pResolve, szItemName, eState, &pExpectedItem, &pExpectedChoice, &pActualItem, &pActualChoice)  );

	// get the item's working repo path
	VERIFY_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pActualItem, &szRepoPath)  );

	// get the choice's state type
	VERIFY_ERR_CHECK(  SG_resolve__state__get_info(pCtx, eState, &szState, NULL, NULL)  );

	// convert the step's value names into their labels
	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pExpectedChoice->pValues, szBaselineName, &szBaseline)  );
	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx, pExpectedChoice->pValues, szOtherName, &szOther)  );

	// format the merge command
	if (szBaseline == NULL)
	{
		VERIFY_ERR_CHECK(  SG_string__alloc__format(pCtx, &sDump, "%s Value not available: %s", gszDumpComment, szBaselineName)  );
	}
	else if (szOther == NULL)
	{
		VERIFY_ERR_CHECK(  SG_string__alloc__format(pCtx, &sDump, "%s Value not available: %s", gszDumpComment, szOtherName)  );
	}
	else
	{
		VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sDump)  );
		if (strcmp(szBaseline, SG_RESOLVE__LABEL__BASELINE) != 0)
		{
			VERIFY_ERR_CHECK(  SG_string__append__format(pCtx, sDump, "%s Cannot merge arbitrary value from command line: %s%s", gszDumpComment, szBaseline, SG_PLATFORM_NATIVE_EOL_STR)  );
		}
		if (strcmp(szOther, SG_RESOLVE__LABEL__OTHER) != 0)
		{
			VERIFY_ERR_CHECK(  SG_string__append__format(pCtx, sDump, "%s Cannot merge arbitrary value from command line: %s%s", gszDumpComment, szOther, SG_PLATFORM_NATIVE_EOL_STR)  );
		}
		if (SG_string__length_in_bytes(sDump) > 0u)
		{
			VERIFY_ERR_CHECK(  SG_string__append__format(pCtx, sDump, "%s Must merge '%s' and '%s' instead.%s", gszDumpComment, SG_RESOLVE__LABEL__BASELINE, SG_RESOLVE__LABEL__OTHER, SG_PLATFORM_NATIVE_EOL_STR)  );
		}
		VERIFY_ERR_CHECK(  SG_string__append__format(pCtx, sDump, "vv resolve merge \"%s\" %s --tool %s", szRepoPath, szState, szMergeTool)  );
	}

	// return the dump
	*ppDump = sDump;
	sDump = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sDump);
	return;
}

/**
 * Dumps the command to execute a single resolve step to a text file.
 */
static void _dump_resolve_step(
	SG_context*                pCtx,     //< [in] [out] Error and context info.
	u0109__test*               pTest,    //< [in] The test that's being executed.
	u0109__test__resolve_step* pStep,    //< [in] The resolve step to execute.
	SG_resolve*                pResolve, //< [in] The resolve data to use for execution.
	SG_string**                ppDump    //< [in] The command(s) to run the resolve step.
	)
{
	SG_string* sDump = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pStep);

	switch (pStep->eAction)
	{
	case RESOLVE_ACTION_ACCEPT:
		VERIFY_ERR_CHECK(  _dump_resolve_step__accept(pCtx, pTest, pStep->szItemName, pStep->eState, pStep->szValueName, pResolve, &sDump)  );
		break;
	case RESOLVE_ACTION_MERGE:
		VERIFY_ERR_CHECK(  _dump_resolve_step__merge(pCtx, pTest, pStep->szItemName, pStep->eState, pStep->szBaselineValueName, pStep->szOtherValueName, pStep->szMergeTool, pResolve, &sDump)  );
		break;
	case RESOLVE_ACTION_MODIFY:
		VERIFY_ERR_CHECK(  SG_string__alloc__format(pCtx, &sDump, "%s Cannot dump RESOLVE_ACTION_MODIFY.", gszDumpComment)  );
		break;
	default:
		sDump = NULL;
		break;
	}

	*ppDump = sDump;
	sDump = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sDump);
	return;
}


/*
**
** Helpers - Verify
**
*/

/**
 * Converts an SG_fsobj_type into an SG_treenode_entry_type.
 */
static void _verify_wd__convert_type(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	SG_fsobj_type           eFileType, //< [in] The type to convert.
	SG_treenode_entry_type* pTreeType  //< [out] The converted type.
	)
{
	SG_UNUSED(pCtx);

	switch (eFileType)
	{
	case SG_FSOBJ_TYPE__REGULAR:
		*pTreeType = SG_TREENODEENTRY_TYPE_REGULAR_FILE;
		break;
	case SG_FSOBJ_TYPE__DIRECTORY:
		*pTreeType = SG_TREENODEENTRY_TYPE_DIRECTORY;
		// TODO: When we want to handle submodules, we'll have to somehow
		//       check if the directory is actually a submodule.
		break;
	case SG_FSOBJ_TYPE__SYMLINK:
		*pTreeType = SG_TREENODEENTRY_TYPE_SYMLINK;
		break;
	default:
		*pTreeType = SG_TREENODEENTRY_TYPE__INVALID;
		break;
	}
}

/**
 * Verifies that a file's contents and size match expectations.
 */
static void _verify_wd__file_contents(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	u0109__test*       pTest,  //< [in] The test being verified.
	SG_repo*           pRepo,  //< [in] The repo that the test is running in.
	const char*        szName, //< [in] The name of the content that is expected.
	const SG_pathname* pPath   //< [in] The path of the file to verify the contents of.
	)
{
	const char*             szExpectedHid = NULL;
	SG_uint64               uExpectedSize = 0u;
	char*                   szActualHid   = NULL;
	SG_fsobj_stat           cStat;
	SG_int_to_string_buffer szExpectedBuffer;
	SG_int_to_string_buffer szActualBuffer;

	VERIFY_ERR_CHECK(  _test__map_name__content_to_hid(pCtx, pTest, szName, &szExpectedHid)  );
	VERIFY_ERR_CHECK(  _repo__hash_file__pathname(pCtx, pRepo, pPath, &szActualHid)  );
	VERIFYP_COND_FAIL("Item's contents don't match expectation.", strcmp(szExpectedHid, szActualHid) == 0, ("AbsolutePath(%s) ExpectedHid(%s) ActualHid(%s)", SG_pathname__sz(pPath), szExpectedHid, szActualHid));

	VERIFY_ERR_CHECK(  _test__map_name__content_to_size(pCtx, pTest, szName, &uExpectedSize)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &cStat)  );
	VERIFYP_COND_FAIL("Item's size doesn't match expectation.", uExpectedSize == cStat.size, ("AbsolutePath(%s) ExpectedSize(%s) ActualSize(%s)", SG_pathname__sz(pPath), SG_uint64_to_sz(uExpectedSize, szExpectedBuffer), SG_uint64_to_sz(cStat.size, szActualBuffer)));

fail:
	SG_NULLFREE(pCtx, szActualHid);
	return;
}

/**
 * Verifies that a file's contents match expectations.
 */
static void _verify_wd__symlink_target(
	SG_context*        pCtx,     //< [in] [out] Error and context info.
	u0109__test*       pTest,    //< [in] The test being verified.
	const char*        szTarget, //< [in] The expected target.
	const SG_pathname* pPath     //< [in] The path of the symlink to verify the target of.
	)
{
	SG_string*   sActualTarget    = NULL;
	SG_pathname* pExpectedTarget  = NULL;
	SG_string*   sExpectedTarget  = NULL;
	const char*  szActualTarget   = NULL;
	const char*  szExpectedTarget = NULL;

	VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szTarget, &pExpectedTarget)  );
	VERIFY_ERR_CHECK(  SG_pathname__make_relative(pCtx, pTest->pPath, pExpectedTarget, &sExpectedTarget)  );
	szExpectedTarget = SG_string__sz(sExpectedTarget);

	VERIFY_ERR_CHECK(  SG_fsobj__readlink(pCtx, pPath, &sActualTarget)  );
	szActualTarget = SG_string__sz(sActualTarget);

	VERIFYP_COND_FAIL("Item's target doesn't match expectation.", strcmp(szExpectedTarget, szActualTarget) == 0, ("AbsolutePath(%s) ExpectedTarget(%s) ActualTarget(%s)", SG_pathname__sz(pPath), szExpectedTarget, szActualTarget));

fail:
	SG_STRING_NULLFREE(pCtx, sActualTarget);
	SG_PATHNAME_NULLFREE(pCtx, pExpectedTarget);
	SG_STRING_NULLFREE(pCtx, sExpectedTarget);
	return;
}

/**
 * Verifies a single step in a test's WD.
 */
static void _verify_wd__step(
	SG_context*               pCtx,         //< [in] [out] Error and context info.
	u0109__test*              pTest,        //< [in] The test being verified.
	SG_pendingtree*           pPendingTree, //< [in] The pendingtree of the WD being verified.
	u0109__test__verify_step* pVerify       //< [in] The step to verify.
	)
{
	SG_string*   sRepoPath = NULL;
	SG_pathname* pPath     = NULL;

	if (pVerify->szRepoPath == NULL)
	{
		// verify the pendingtree "choices" hash
		SG_bool bChoices = SG_FALSE;

		// get the choices hash
		VERIFY_ERR_CHECK(  SG_pendingtree__get_wd_choices__ref(pCtx, pPendingTree, &bChoices, NULL)  );

		// verify the choice hash's existence
		VERIFYP_COND_FAIL("Choice data existence doesn't match expectation.", pVerify->bExists == bChoices, ("ExpectedExistance(%d) ActualExistance(%d)", pVerify->bExists, bChoices));
	}
	else
	{
		// verify a file system object
		const char*            szRepoPath = pVerify->szRepoPath;
		SG_bool                bExists    = SG_FALSE;
		SG_fsobj_type          eFileType  = SG_FSOBJ_TYPE__UNSPECIFIED;
		SG_treenode_entry_type eTreeType  = SG_TREENODEENTRY_TYPE__INVALID;

		// if we need to translate the path to include a partial GID, do that
		if (pVerify->szItemName != NULL)
		{
			VERIFY_ERR_CHECK(  _test__build_temp_repo_path(pCtx, pTest, pVerify->szRepoPath, pVerify->szItemName, &sRepoPath)  );
			szRepoPath = SG_string__sz(sRepoPath);
		}

		// convert the repo path to an absolute one
		VERIFY_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pTest->pPath, szRepoPath, &pPath)  );

		// verify the item's existence
		VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, &eFileType, NULL)  );
		VERIFYP_COND_FAIL("Item's existence doesn't match expectation.", pVerify->bExists == bExists, ("RepoPath(%s) AbsolutePath(%s) ExpectedExistance(%d) ActualExistance(%d)", szRepoPath, SG_pathname__sz(pPath), pVerify->bExists, bExists));

		// verify the item's type
		if (pVerify->eType != SG_TREENODEENTRY_TYPE__INVALID)
		{
			VERIFY_ERR_CHECK(  _verify_wd__convert_type(pCtx, eFileType, &eTreeType)  );
			VERIFYP_COND_FAIL("Item's type doesn't match expectation.", pVerify->eType == eTreeType, ("RepoPath(%s) AbsolutePath(%s) ExpectedType(%d) ActualType(%d)", szRepoPath, SG_pathname__sz(pPath), pVerify->eType, eTreeType));
		}

		// verify the item's content
		if (pVerify->szContent != NULL)
		{
			if (pVerify->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
			{
				SG_repo* pRepo = NULL;

				VERIFY_ERR_CHECK(  SG_pendingtree__get_repo(pCtx, pPendingTree, &pRepo)  );
				VERIFY_ERR_CHECK(  _verify_wd__file_contents(pCtx, pTest, pRepo, pVerify->szContent, pPath)  );
			}
			else if (pVerify->eType == SG_TREENODEENTRY_TYPE_SYMLINK)
			{
				VERIFY_ERR_CHECK(  _verify_wd__symlink_target(pCtx, pTest, pVerify->szContent, pPath)  );
			}
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, sRepoPath);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Verifies that the test's working directory is in the expected state.
 */
static void _verify_wd(
	SG_context*     pCtx,        //< [in] [out] Error and context info.
	u0109__test*    pTest,       //< [in] The test to verify the WD of.
	SG_pendingtree* pPendingTree //< [in] The pendingtree of the WD being verified.
	)
{
	u0109__test__verify_step* pVerify = pTest->aVerification;

	while (pVerify->bValid == SG_TRUE)
	{
		VERIFY_ERR_CHECK(  _verify_wd__step(pCtx, pTest, pPendingTree, pVerify)  );
		++pVerify;
	}

fail:
	return;
}


/*
**
** Helpers - Tests
**
*/

/**
 * An SG_free_callback for SG_pathname objects.
 */
static void _execute_test__free__pathname(
	SG_context* pCtx,
	void*       pData
	)
{
	SG_pathname* pPath = (SG_pathname*)pData;
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Frees the intermediate data used by a test.
 */
static void _execute_test__free(
	SG_context*  pCtx,
	u0109__test* pTest
	)
{
	u0109__test__expect_item* pItem = pTest->aExpectedItems;

	SG_STRING_NULLFREE(pCtx, pTest->sRepo);
	SG_PATHNAME_NULLFREE(pCtx, pTest->pRootPath);
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pTest->pPaths, _execute_test__free__pathname);
	SG_VHASH_NULLFREE(pCtx, pTest->pCommitsHids);
	SG_VHASH_NULLFREE(pCtx, pTest->pItemsGids);
	SG_VHASH_NULLFREE(pCtx, pTest->pContentsHids);
	SG_VHASH_NULLFREE(pCtx, pTest->pContentsRepoPaths);
	SG_VHASH_NULLFREE(pCtx, pTest->pContentsSizes);
	SG_VHASH_NULLFREE(pCtx, pTest->pMergesStartTimes);
	SG_VHASH_NULLFREE(pCtx, pTest->pMergesEndTimes);
	SG_VARRAY_NULLFREE(pCtx, pTest->pStrings);

	while (pItem->bValid != SG_FALSE)
	{
		u0109__test__expect_choice* pChoice = pItem->aChoices;
		while (pChoice->bValid != SG_FALSE)
		{
			SG_VHASH_NULLFREE(pCtx, pChoice->pValues);
			++pChoice;
		}
		++pItem;
	}
}

/**
 * NULLFREE macro for _execute_test__free.
 */
#define _TEST__NULLFREE(pCtx, pTest) _sg_generic_nullfree(pCtx, pTest, _execute_test__free)

/**
 * Allocates the intermediate data for a test.
 */
static void _execute_test__alloc(
	SG_context*   pCtx, //< [in] [out] Error and context info.
	u0109__test*  pTest //< [in] The test to initialize.
	)
{
	u0109__test__expect_item* pItem = pTest->aExpectedItems;

	// allocate top-level containers
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pTest->pPaths, 5u)  );
	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTest->pCommitsHids)  );
	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTest->pItemsGids)  );
	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTest->pContentsHids)  );
	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTest->pContentsRepoPaths)  );
	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTest->pContentsSizes)  );
	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTest->pMergesStartTimes)  );
	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTest->pMergesEndTimes)  );
	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pTest->pStrings)  );

	// allocate deeper containers
	while (pItem->bValid != SG_FALSE)
	{
		u0109__test__expect_choice* pChoice = pItem->aChoices;
		while (pChoice->bValid != SG_FALSE)
		{
			VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pChoice->pValues)  );
			++pChoice;
		}
		++pItem;
	}

	// done, success exit
	return;

fail:
	// failure cleanup
	_TEST__NULLFREE(pCtx, pTest);
	return;
}

/**
 * Executes a test's SETUP plan.
 */
void u0109__execute_test__setup(
	SG_context*  pCtx, //< [in] [out] Error and context info.
	u0109__test* pTest //< [in] The test to execute.
	)
{
	u0109__test__setup_step* pStep = pTest->aSetupPlan;

	// run the setup plan
	while (pStep->bValid != SG_FALSE)
	{
		VERIFY_ERR_CHECK(  _execute_setup_step(pCtx, pTest, pStep)  );
		++pStep;
	}

fail:
	return;
}

/**
 * Dumps a test's RESOLVE plan.
 */
void u0109__execute_test__dump(
	SG_context*  pCtx, //< [in] [out] Error and context info.
	u0109__test* pTest //< [in] The test to execute.
	)
{
	u0109__test__resolve_step* pStep        = pTest->aResolvePlan;
	SG_pendingtree*            pPendingTree = NULL;
	SG_resolve*                pResolve     = NULL;
	SG_string*                 sCommand     = NULL;
	SG_string*                 sDump        = NULL;
	SG_pathname*               pPath        = NULL;
	SG_file*                   pFile        = NULL;

	// start with an empty dump
	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sDump)  );

	// dump the resolve plan to a string
	while (pStep->bValid != SG_FALSE)
	{
		// allocate a pendingtree and its resolve data
		VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
		VERIFY_ERR_CHECK(  SG_resolve__alloc(pCtx, pPendingTree, &pResolve)  );

		// verify that the resolve data matches current expectations
		// this generates most of our name => item/value mappings
		VERIFY_ERR_CHECK(  _verify_expectations(pCtx, pTest, pResolve)  );

		// dump the step
		VERIFY_ERR_CHECK(  _dump_resolve_step(pCtx, pTest, pStep, pResolve, &sCommand)  );
		if (sCommand == NULL)
		{
			// the command can't be dumped, give up
			SG_STRING_NULLFREE(pCtx, sDump);
			sDump = NULL;
			break;
		}
		else
		{
			// append the command to the dump and reset it
			VERIFY_ERR_CHECK(  SG_string__append__string(pCtx, sDump, sCommand)  );
			VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx, sDump, SG_PLATFORM_NATIVE_EOL_STR)  );
			SG_STRING_NULLFREE(pCtx, sCommand);
		}
		++pStep;

		// cleanup
		SG_RESOLVE_NULLFREE(pCtx, pResolve);
		SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	}

	// if we have a dump, write it to a file
	if (sDump != NULL)
	{
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pTest->pPath, gszDumpFilename)  );
		VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE | SG_FILE_WRONLY | SG_FILE_TRUNC, 0666, &pFile)  );
		VERIFY_ERR_CHECK(  SG_file__write__string(pCtx, pFile, sDump)  );
		VERIFY_ERR_CHECK(  SG_FILE_NULLCLOSE(pCtx, pFile)  );
	}

fail:
	SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	SG_STRING_NULLFREE(pCtx, sCommand);
	SG_STRING_NULLFREE(pCtx, sDump);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

/**
 * Executes a test's RESOLVE plan.
 */
void u0109__execute_test__resolve(
	SG_context*  pCtx, //< [in] [out] Error and context info.
	u0109__test* pTest //< [in] The test to execute.
	)
{
	u0109__test__resolve_step* pStep        = pTest->aResolvePlan;
	SG_pendingtree*            pPendingTree = NULL;
	SG_resolve*                pResolve     = NULL;

	// run the resolve plan
	while (pStep->bValid != SG_FALSE)
	{
		// allocate a pendingtree and its resolve data
		VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
		VERIFY_ERR_CHECK(  SG_resolve__alloc(pCtx, pPendingTree, &pResolve)  );
		VERIFY_ERR_CHECK(  _resolve__read_data(pCtx, pResolve)  );

		// verify that the resolve data matches current expectations
		VERIFY_ERR_CHECK(  _verify_expectations(pCtx, pTest, pResolve)  );
		VERIFY_ERR_CHECK(  _resolve__read_data(pCtx, pResolve)  );

		// execute the resolve step
		VERIFY_ERR_CHECK(  _execute_resolve_step(pCtx, pTest, pStep, pResolve)  );
		++pStep;
		VERIFY_ERR_CHECK(  _resolve__read_data(pCtx, pResolve)  );

		// save the new state of the resolve/pendingtree
		VERIFY_ERR_CHECK(  SG_resolve__save(pCtx, pResolve, SG_TRUE)  );
		VERIFY_ERR_CHECK(  _resolve__read_data(pCtx, pResolve)  );

		// cleanup
		SG_RESOLVE_NULLFREE(pCtx, pResolve);
		SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	}

fail:
	SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
}

/**
 * Executes a test's VERIFY plan.
 */
void u0109__execute_test__verify(
	SG_context*  pCtx, //< [in] [out] Error and context info.
	u0109__test* pTest //< [in] The test to execute.
	)
{
	SG_pendingtree* pPendingTree = NULL;
	SG_resolve*     pResolve     = NULL;

	// allocate a pendingtree and resolve data for the final state
	VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_resolve__alloc(pCtx, pPendingTree, &pResolve)  );
	VERIFY_ERR_CHECK(  _resolve__read_data(pCtx, pResolve)  );

	// verify that our final expected resolve data matches our final actual resolve data
	VERIFY_ERR_CHECK(  _verify_expectations(pCtx, pTest, pResolve)  );
	VERIFY_ERR_CHECK(  _resolve__read_data(pCtx, pResolve)  );

	// verify that the working directory is in the correct state
	VERIFY_ERR_CHECK(  _verify_wd(pCtx, pTest, pPendingTree)  );

	// have the pendingtree do a full scan
	// then make sure we can still allocate resolve data from it afterward
	SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	SG_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
	SG_ERR_CHECK(  SG_pendingtree__scan(pCtx, pPendingTree, SG_TRUE, SG_TRUE, SG_TRUE, gpBlankFilespec)  );
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
	VERIFY_ERR_CHECK(  SG_PENDINGTREE__ALLOC(pCtx, pTest->pPath, &pPendingTree)  );
	VERIFY_ERR_CHECK(  SG_resolve__alloc(pCtx, pPendingTree, &pResolve)  );
	VERIFY_ERR_CHECK(  _resolve__read_data(pCtx, pResolve)  );

fail:
	SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_PENDINGTREE_NULLFREE(pCtx, pPendingTree);
}

/**
 * Executes a single test.
 */
void u0109__execute_test(
	SG_context*        pCtx,     //< [in] [out] Error and context info.
	u0109__test*       pTest,    //< [in] The test to execute.
	const char*        szRunId,  //< [in] Unique ID for this execution of the test.
	const SG_pathname* pRootPath //< [in] The root path that all test files are stored in.
	)
{
	static const char* szRepoNameFormat = "%s-%s";
	static const char* szUsername       = "testing@sourcegear.com";

	SG_bool  bPathExists = SG_FALSE;
	SG_repo* pRepo       = NULL;

	SG_NULLARGCHECK(pTest);
	SG_NULLARGCHECK(pRootPath);

#if defined(TEST_PATTERN)
	{
		SG_bool bPatternMatch = SG_FALSE;

		// if we're only executing matching tests,
		// check if this one matches
		VERIFY_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, TEST_PATTERN, pTest->szName, 0u, &bPatternMatch)  );
		if (bPatternMatch == SG_FALSE)
		{
			return;
		}
	}
#endif

	// if this test is a continuation of another one, find that other one
	if (pTest->szStartName != NULL)
	{
		SG_uint32 uIndex = 0u;

#if defined(SETUP_ONLY)
		// if we're only running setup, ignore tests that are continuations of previous tests
		// Continuations of previous tests will almost never work in this case
		// because they usually expect the previous test to have run all of its
		// resolution code before the continuation starts, which it won't have done
		// if we're only running setup code.
		return;
#endif

		// find the base test
		for (uIndex = 0u; uIndex < SG_NrElements(u0109__gaTests); ++uIndex)
		{
			u0109__test* pCurrentTest = u0109__gaTests + uIndex;

			// The base test must come before the current test in the list
			// to ensure that the base test has already been executed.
			// So if we find the test we're trying to run before we find
			// its base test, we've got a problem.
			VERIFY_COND_FAIL("Base test not found.", strcmp(pTest->szName, pCurrentTest->szName) != 0);

			if (strcmp(pTest->szStartName, pCurrentTest->szName) == 0)
			{
				pTest->pStartTest = pCurrentTest;
				break;
			}
		}
	}

	VERIFY_ERR_CHECK(  _begin_test_label(__FILE__, __LINE__, pTest->szName)  );

	// allocate the test's intermediate data
	VERIFY_ERR_CHECK(  _execute_test__alloc(pCtx, pTest)  );

	// create a root directory for the test's working copies
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pTest->pRootPath, pRootPath, szRunId)  );
	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pTest->pRootPath, pTest->szName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pTest->pRootPath, &bPathExists, NULL, NULL)  );
	VERIFYP_COND_FAIL("Test path already exists, must be duplicate name.", bPathExists == SG_FALSE, ("Path(%s)", SG_pathname__sz(pTest->pRootPath)));
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pTest->pRootPath)  );

	// create a directory for our initial working copy
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pTest->pPath, pTest->pRootPath)  );
	VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pTest->pPaths, (void*)pTest->pPath, NULL)  );
	VERIFY_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, pTest->pPath, "")  );
	if (pTest->pStartTest == NULL)
	{
		// we're not starting with an existing directory, so just create an empty one
		VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pTest->pPath)  );
	}
	else
	{
		// we're starting with the WD from a prior test, so copy that one
		VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pTest->pStartTest->pPath, &bPathExists, NULL, NULL)  );
		VERIFYP_COND_FAIL("Start test path doesn't exist.", bPathExists == SG_TRUE, ("Path(%s)", SG_pathname__sz(pTest->pStartTest->pPath)));
		VERIFY_ERR_CHECK(  SG_fsobj__cpdir_recursive__pathname_pathname(pCtx, pTest->pStartTest->pPath, pTest->pPath)  );
	}

	// create a repo for the test
	if (pTest->pStartTest == NULL)
	{
		// we need a new repo for this test
		VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pTest->sRepo)  );
		VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, pTest->sRepo, szRepoNameFormat, szRunId, pTest->szName)  );
		VERIFY_ERR_CHECK(  SG_vv_verbs__init_new_repo(pCtx, SG_string__sz(pTest->sRepo), SG_pathname__sz(pTest->pPath), NULL, NULL, SG_FALSE, SG_FALSE, NULL, SG_FALSE, NULL, NULL)  );
		VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pTest->sRepo), &pRepo)  );
		VERIFY_ERR_CHECK(  SG_user__create(pCtx, pRepo, szUsername, NULL)  );
		VERIFY_ERR_CHECK(  SG_user__set_user__repo(pCtx, pRepo, szUsername)  );
		SG_REPO_NULLFREE(pCtx, pRepo);
	}
	else
	{
		// find the repo that we need to re-use from a prior test
		VERIFY_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pTest->sRepo, pTest->pStartTest->sRepo)  );
	}

	// run the setup plan
	VERIFY_ERR_CHECK(  u0109__execute_test__setup(pCtx, pTest)  );

#if defined(SETUP_ONLY) && defined(DUMP_RESOLVE)
	// dump the resolve plan
	VERIFY_ERR_CHECK(  u0109__execute_test__dump(pCtx, pTest)  );
#endif

#if !defined(SETUP_ONLY)
	// run the resolve plan
	VERIFY_ERR_CHECK(  u0109__execute_test__resolve(pCtx, pTest)  );

	// run the verify plan
	VERIFY_ERR_CHECK(  u0109__execute_test__verify(pCtx, pTest)  );
#endif

fail:
	// Don't free pTest here.  Its intermediate data needs to stick around in case
	// another test continues from where it left off.  All the tests will be freed
	// after they've all been executed.
	SG_REPO_NULLFREE(pCtx, pRepo);
	return;
}


/*
**
** Main
**
*/

TEST_MAIN(u0109_resolve)
{
	char*        szRunId   = NULL;
	SG_pathname* pRootPath = NULL;
	SG_uint32    uIndex    = 0u;

	TEMPLATE_MAIN_START;

	// generate a unique ID for this execution of the test
	VERIFY_ERR_CHECK(  SG_tid__alloc2(pCtx, &szRunId, 8)  );

	// allocate a root path to store all of our test working copies in
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pRootPath, "u0109")  );

	// run through each test and execute it
	for (uIndex = 0u; uIndex < SG_NrElements(u0109__gaTests); ++uIndex)
	{
		VERIFY_ERR_CHECK_DISCARD(  u0109__execute_test(pCtx, u0109__gaTests + uIndex, szRunId, pRootPath)  );
	}

fail:
	// free the intermediate data for all the tests
	for (uIndex = 0u; uIndex < SG_NrElements(u0109__gaTests); ++uIndex)
	{
		u0109__test* pTest = u0109__gaTests + uIndex;
		_TEST__NULLFREE(pCtx, pTest);
	}

	SG_NULLFREE(pCtx, szRunId);
	SG_PATHNAME_NULLFREE(pCtx, pRootPath);
	TEMPLATE_MAIN_END;
}
