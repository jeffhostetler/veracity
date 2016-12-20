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

#include <sg.h>

#include "unittests.h"


/*
**
** Types
**
*/

/**
 * Maximum number of arguments any command can accept.
 */
#define U0111__MAX_COMMAND_ARGS 2u

/**
 * Global data used by an entire test.
 */
typedef struct u0111__test_data
{
	// information about the test file itself
	SG_pathname* pTestFile;    //< Path of the test file being processed.
	SG_pathname* pBasePath;    //< Base path that other paths in the test are relative to.
	                           //< Usually the folder containing pTestFile.

	// information read from the test file's global/header data
	SG_pathname* pImportFile;  //< Path of the fast-import file being tested.
	SG_error     eResult;      //< Result expected when importing pImportFile.
	SG_vhash*    pOptions;     //< Global options in effect.
	SG_vhash*    pSharedUsers; //< Usernames to import from a shared repo.
	                           //< [USERNAME]=NULL
	                           //< NULL if we're not importing shared users.
	SG_string*   sSharedUsers; //< Name of the shared users repo.
	                           //< NULL if we're not importing shared users.

	// information about the imported repo
	SG_string*   sRepoName;    //< Name of the repo imported from pImportFile.
	SG_repo*     pRepo;        //< Handle to the imported repo.
	SG_pathname* pTempPath;    //< Path to store temporary data for/from the test.
	SG_vhash*    pUsers;       //< Set of users in the imported repo
	                           //< [ID]=ZING_USER_RECORD
	SG_rbtree*   pLeaves;      //< Set of leaves in the imported repo.
	                           //< [HID]=?
	SG_vhash*    pBranches;    //< Branch pile from the imported repo.
	                           //< [branches][NAME][values][HID]=HIDREC
	                           //< [branches][NAME][records][HIDREC]=HID
	                           //< [closed][NAME]=HIDREC
	                           //< [values][HID][HIDREC]=NULL
	SG_rbtree*   pTags;        //< Set of tags in the imported repo.
	                           //< [NAME]=HID

	// information read from the remainder of the test file
	SG_vhash*    pCommits;     //< Set of expected commits, indexed by name.
	                           //< Keys are u0111__commit_key__*
}
u0111__test_data;

/**
 * Data for a single command.
 */
typedef struct u0111__command
{
	SG_string* sName;                          //< Name of the command.
	SG_string* aArgs[U0111__MAX_COMMAND_ARGS]; //< Arguments specified to the command, in order.
	                                           //< NULLs in positions greater than the command's argument count.
}
u0111__command;

/**
 * Prototype of a function that implements a command.
 */
typedef void u0111__command_function(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_file*          pFile,     //< [in] The test file being run.
	u0111__command*   pCommand,  //< [in] The command to execute.
	u0111__test_data* pTestData, //< [in] Data for the test being executed.
	void*             pData      //< [in] Command-specific data to operate on.
	);

/**
 * Specification of a single type of command.
 */
typedef struct u0111__command_spec
{
	const char*              szName;          //< The command's name.
	SG_uint32                uArgs;           //< Number of arguments the command takes.
	                                          //< Must be <= U0111__MAX_COMMAND_ARGS
	SG_uint32                uCount;          //< Number of times the command may be executed.
	                                          //< Zero indicates no limit.
	u0111__command_function* fImplementation; //< Function that implements the command.
}
u0111__command_spec;

/**
 * Prototype of a function that parses a string into a value.
 */
typedef void u0111__value_parser(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	const char* szValue, //< [in] String value to parse.
	SG_variant* pValue   //< [out] The parsed value.
	);

/**
 * Specification of a single test-wide option.
 */
typedef struct u0111__option_spec
{
	const char*          szName;    //< Name of the option.
	const char*          szDefault; //< The option's default value.
	u0111__value_parser* fParser;   //< Function used to parse the option's value.
}
u0111__option_spec;


/*
**
** Data
**
*/

/**
 * Format of repo names used for tests.
 * A single %s parameter containing the name of the test is passed in.
 */
static const char* u0111__repo_name_format = "u0111_%s";

/**
 * Format of repo names for importing shared users from.
 * A single %s parameter containing the name of the repo being imported.
 */
static const char* u0111__shared_users_repo_name_format = "%s_shared_users";

/**
 * The filename of the "priority" test.
 *
 * When running a folder of tests, if the folder contains a test file with this
 * filename, then ONLY that test will be run.  This is mainly for debugging purposes.
 * If a test is giving you trouble, rename it to be the priority test and then it's
 * easy to step through that one test while ignoring all the others.
 */
static const char* u0111__priority_test_name = "_priority.test";

/*
 * Extensions for files that contain data about a working copy match.
 */
static const char* u0111__match_wc__diff_log    = ".diff.log";
static const char* u0111__match_wc__true_cache  = ".match.true";
static const char* u0111__match_wc__false_cache = ".match.false";

/*
 * Constants for keys in zing records.
 */
static const char* u0111__zing_user__recid    = "recid";
static const char* u0111__zing_user__name     = "name";
static const char* u0111__zing_user__inactive = "inactive";
static const char* u0111__zing_comment__text  = "text";

/*
 * Constants for keys used in commit hashes within test data's pCommits.
 */
static const char* u0111__commit_key__name     = "name";
static const char* u0111__commit_key__user     = "user";
static const char* u0111__commit_key__time     = "time";
static const char* u0111__commit_key__parents  = "parents";
static const char* u0111__commit_key__comments = "comments";
static const char* u0111__commit_key__wc       = "wc";
static const char* u0111__commit_key__hid      = "hid";

/*
 * Constants for test-wide option names.
 */
#define U0111__OPTION__LONG_TEST "long-test"

/*
 * Forward declarations for command implementations.
 */
static u0111__command_function u0111__do_command__import;
static u0111__command_function u0111__do_command__import__option;
static u0111__command_function u0111__do_command__import__error;
static u0111__command_function u0111__do_command__import__shared_user;
static u0111__command_function u0111__do_command__commit;
static u0111__command_function u0111__do_command__commit__user;
static u0111__command_function u0111__do_command__commit__time;
static u0111__command_function u0111__do_command__commit__parent;
static u0111__command_function u0111__do_command__commit__comment;
static u0111__command_function u0111__do_command__commit__wc;
static u0111__command_function u0111__do_command__leaf;
static u0111__command_function u0111__do_command__branch;
static u0111__command_function u0111__do_command__branch__status;
static u0111__command_function u0111__do_command__branch__commit;
static u0111__command_function u0111__do_command__branch__wc;
static u0111__command_function u0111__do_command__tag;
static u0111__command_function u0111__do_command__tag__commit;
static u0111__command_function u0111__do_command__tag__wc;

/*
 * Forward declarations for value parsers.
 */
static u0111__value_parser u0111__parse_value__bool;
static u0111__value_parser u0111__parse_value__branch_status;

/**
 * Global commands available for the header of a test file.
 */
static const u0111__command_spec u0111__gaHeaderCommands[] =
{
	{ "import", 1u, 1u, u0111__do_command__import }, //< Specifies global data about the test.
};

/**
 * Global commands available for the remainder (non-header) of a test file.
 */
static const u0111__command_spec u0111__gaGlobalCommands[] =
{
	{ "commit", 1u, 0u, u0111__do_command__commit }, //< Specifies details about an expected commit.
	{ "leaf",   1u, 0u, u0111__do_command__leaf },   //< Specifies details about an expected leaf.
	{ "branch", 1u, 0u, u0111__do_command__branch }, //< Specifies details about an expected branch.
	{ "tag",    1u, 0u, u0111__do_command__tag },    //< Specifies details about an expected tag.
};

/**
 * Subcommands available under an import command.
 */
static const u0111__command_spec u0111__gaImportCommands[] =
{
	{ "option",      2u, 0u, u0111__do_command__import__option },      //< Specifies a values for a test-global option.
	{ "error",       1u, 1u, u0111__do_command__import__error },       //< Specifies the error that the import should fail with.
	{ "shared-user", 1u, 0u, u0111__do_command__import__shared_user }, //< Specifies a user that should be imported from a different repo.
	                                                                   //< All specified users will be imported from the same repo.
};

/**
 * Subcommands available under a leaf command.
 */
static const u0111__command_spec u0111__gaLeafCommands[] =
{
	{ "leaf", 1u, 0u, u0111__do_command__leaf }, //< Specifies details about an expected leaf.
};

/**
 * Subcommands available under a commit command.
 */
static const u0111__command_spec u0111__gaCommitCommands[] =
{
	{ "user",    1u, 1u, u0111__do_command__commit__user },    //< Specifies the commit's expected user.
	{ "time",    1u, 1u, u0111__do_command__commit__time },    //< Specifies the commit's expected timestamp.
	{ "parent",  1u, 0u, u0111__do_command__commit__parent },  //< Specifies one of the commit's expected parents.
	{ "comment", 1u, 0u, u0111__do_command__commit__comment }, //< Specifies one of the commit's expected comments.
	{ "wc",      1u, 1u, u0111__do_command__commit__wc },      //< Specifies the working copy expected when checking out the commit.
};

/**
 * Subcommands available under a branch command.
 */
static const u0111__command_spec u0111__gaBranchCommands[] =
{
	{ "status", 1u, 1u, u0111__do_command__branch__status }, //< Specifies the branch's expected open/closed status.
	{ "commit", 1u, 0u, u0111__do_command__branch__commit }, //< Specifies the commit that the branch is expected to point to.
	{ "wc",     1u, 1u, u0111__do_command__branch__wc },     //< Specifies the working copy expected when checking out the branch.
};

/**
 * Subcommands available under a tag command.
 */
static const u0111__command_spec u0111__gaTagCommands[] =
{
	{ "commit", 1u, 1u, u0111__do_command__tag__commit }, //< Specifies the commit that the tag is expected to point to.
	{ "wc",     1u, 1u, u0111__do_command__tag__wc },     //< Specifies the working copy expected when checking out the tag.
};

/**
 * Global options available to be configured.
 */
static const u0111__option_spec u0111__options[] =
{
	{ U0111__OPTION__LONG_TEST, "false", u0111__parse_value__bool },
};


/*
**
** Data Management
**
*/

/**
 * Initializes the data in a command.
 */
static void u0111__command__init(
	SG_context*     pCtx,    //< [in] [out] Error and context info.
	u0111__command* pCommand //< [in] Command to initialize the data in.
	)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pCommand);

	pCommand->sName = NULL;
	for (uIndex = 0u; uIndex < U0111__MAX_COMMAND_ARGS; ++uIndex)
	{
		pCommand->aArgs[uIndex] = NULL;
	}

fail:
	return;
}

/**
 * Frees the data in a command.
 */
static void u0111__command__free(
	SG_context*     pCtx,    //< [in] [out] Error and context info.
	u0111__command* pCommand //< [in] Command to free the data in.
	)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pCommand);

	SG_STRING_NULLFREE(pCtx, pCommand->sName);
	for (uIndex = 0u; uIndex < U0111__MAX_COMMAND_ARGS; ++uIndex)
	{
		SG_STRING_NULLFREE(pCtx, pCommand->aArgs[uIndex]);
	}

fail:
	return;
}

/**
 * Initializes a test's data.
 */
static void u0111__test_data__init(
	SG_context*       pCtx,  //< [in] [out] Error and context info.
	u0111__test_data* pData, //< [in] Data to initialize.
	SG_pathname**     ppPath //< [in] [out] Path of the test file to init from.
	                         //<            We steal and NULL the caller's pointer.
	)
{
	const u0111__option_spec* pSpec = NULL;

	SG_NULLARGCHECK(pData);
	SG_NULLARGCHECK(ppPath);

	pData->pTestFile = *ppPath;
	*ppPath           = NULL;

	// calculate our base path from the test file path
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pData->pBasePath, pData->pTestFile)  );
	SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pData->pBasePath)  );

	// start with empty data for everything else
	pData->pImportFile  = NULL;
	pData->eResult      = SG_ERR_OK;
	SG_VHASH__ALLOC(pCtx, &pData->pOptions);
	pData->pSharedUsers = NULL;
	pData->sSharedUsers = NULL;
	pData->sRepoName    = NULL;
	pData->pRepo        = NULL;
	pData->pTempPath    = NULL;
	pData->pUsers       = NULL;
	pData->pLeaves      = NULL;
	pData->pBranches    = NULL;
	pData->pTags        = NULL;
	SG_VHASH__ALLOC(pCtx, &pData->pCommits);

	// set default option values
	for (pSpec = u0111__options; pSpec != u0111__options + SG_NrElements(u0111__options); ++pSpec)
	{
		SG_variant cValue;

		SG_ERR_CHECK(  pSpec->fParser(pCtx, pSpec->szDefault, &cValue)  );
		SG_ERR_CHECK(  SG_vhash__addcopy__variant(pCtx, pData->pOptions, pSpec->szName, &cValue)  );
	}

fail:
	return;
}

/**
 * Frees a test's data.
 */
static void u0111__test_data__free(
	SG_context*       pCtx, //< [in] [out] Error and context info.
	u0111__test_data* pData //< [in] Data to free.
	)
{
	if (pData != NULL)
	{
		SG_PATHNAME_NULLFREE(pCtx, pData->pTestFile);
		SG_PATHNAME_NULLFREE(pCtx, pData->pBasePath);
		SG_PATHNAME_NULLFREE(pCtx, pData->pImportFile);
		SG_VHASH_NULLFREE(pCtx, pData->pOptions);
		SG_VHASH_NULLFREE(pCtx, pData->pSharedUsers);
		SG_STRING_NULLFREE(pCtx, pData->sSharedUsers);
		SG_STRING_NULLFREE(pCtx, pData->sRepoName);
		SG_REPO_NULLFREE(pCtx, pData->pRepo);
		SG_PATHNAME_NULLFREE(pCtx, pData->pTempPath);
		SG_VHASH_NULLFREE(pCtx, pData->pUsers);
		SG_RBTREE_NULLFREE(pCtx, pData->pLeaves);
		SG_VHASH_NULLFREE(pCtx, pData->pBranches);
		SG_RBTREE_NULLFREE(pCtx, pData->pTags);
		SG_VHASH_NULLFREE(pCtx, pData->pCommits);
	}
}

/**
 * Gets a list of users in a repo, indexed by their ID.
 */
static void u0111__get_users(
	SG_context* pCtx,   //< [in] [out] Error and context info.
	SG_repo*    pRepo,  //< [in] Repo to get users from.
	SG_vhash**  ppUsers //< [out] Users, indexed by ID.  Values are zing user records.
	)
{
	SG_varray* pArray = NULL;
	SG_vhash*  pHash  = NULL;
	SG_uint32  uCount = 0u;
	SG_uint32  uIndex = 0u;

	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(ppUsers);

	// start with an empty hash
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pHash)  );

	// get the list of users from the repo and iterate them
	SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pArray)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pArray, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_vhash*   pUser        = NULL;
		const char* szId         = NULL;
		SG_bool     bHasInactive = SG_FALSE;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pArray, uIndex, &pUser)  );

		// get the current user's ID
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, u0111__zing_user__recid, &szId)  );

		// make sure the user has a value for the "inactive" field
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pUser, u0111__zing_user__inactive, &bHasInactive)  );
		if (bHasInactive == SG_FALSE)
		{
			SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pUser, u0111__zing_user__inactive, SG_FALSE)  );
		}

		// copy the user's information into our hash, indexed by ID
		SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pHash, szId, pUser)  );
	}

	// return the built hash
	*ppUsers = pHash;
	pHash = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pArray);
	SG_VHASH_NULLFREE(pCtx, pHash);
}


/*
**
** Data Matching
**
*/

/**
 * Checks if the contents of two folders exactly matches.
 */
static void u0111__match_wc__folders(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	const u0111__test_data* pTestData, //< [in] Data for the test that's executing.
	const SG_pathname*      pLeft,     //< [in] Path of the first folder to match.
	const SG_pathname*      pRight,    //< [in] Path of the second folder to match.
	const SG_pathname*      pLog,      //< [in] Path of a file to log diff results to.
	                                   //<      If NULL, output is logged to current stdout/stderr.
	SG_bool*                pMatch     //< [out] Whether or not the folders' contents match.
	)
{
#define WC_DIFF_TOOL "diff"

	SG_file*        pLogFile = NULL;
	SG_exec_argvec* pArgs    = NULL;
	SG_bool         bError   = SG_FALSE;
	SG_exit_status  iResult  = 0;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pLeft);
	SG_NULLARGCHECK(pRight);
	SG_NULLARGCHECK(pMatch);

	if (pLog != NULL)
	{
		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pLog, SG_FILE_CREATE_NEW | SG_FILE_WRONLY, 0644, &pLogFile)  );
	}

	SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx, &pArgs)  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgs, "-q")  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgs, "-r")  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgs, "-x")  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgs, ".sgdrawer")  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgs, SG_pathname__sz(pLeft))  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgs, SG_pathname__sz(pRight))  );

	// try to run diff to compare the directories
	// if we have any problem running it, just consider it a match
	// we don't want tests to fail just because of things like diff being unavailable
	SG_ERR_TRY(  SG_exec__exec_sync__files(pCtx, WC_DIFF_TOOL, pArgs, NULL, pLogFile, pLogFile, &iResult)  );
	SG_ERR_CATCH_SET(SG_ERR_EXEC_FAILED, bError, SG_TRUE);
	SG_ERR_CATCH_END;

	// if there was an execution problem, note it in the log file
	if (bError != SG_FALSE)
	{
		SG_ERR_CHECK(  SG_file__write__sz(pCtx, pLogFile,
			"There was an error executing working copy comparison program: " WC_DIFF_TOOL SG_PLATFORM_NATIVE_EOL_STR
			"Skipping the comparison and counting it as a match." SG_PLATFORM_NATIVE_EOL_STR
			"This behavior allows the test to run without failing on systems where the program is unavailable."
			)  );
	}

	// return the result
	*pMatch = (bError != SG_FALSE || iResult == 0 ? SG_TRUE : SG_FALSE);

fail:
	SG_FILE_NULLCLOSE(pCtx, pLogFile);
	SG_ERR_IGNORE(  SG_exec_argvec__free(pCtx, pArgs)  );
	return;
#undef WC_DIFF_TOOL
}

/**
 * Builds a path to data about a working copy match.
 */
static void u0111__match_wc__get_data_path(
	SG_context*        pCtx,        //< [in] [out] Error and context info.
	const SG_pathname* pRepoFolder, //< [in] Folder where the matched working copy was checked out.
	const char*        szKey,       //< [in] A key/name to include in the path.
	                                //<      May be NULL to not include one.
	const char*        szExtension, //< [in] File extension to use on the generated path.
	SG_pathname**      ppPath       //< [out] Generated data path.
	)
{
	SG_string*   sPath = NULL;
	SG_pathname* pPath = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sPath, SG_pathname__sz(pRepoFolder))  );
	if (szKey != NULL)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, sPath, "_")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, sPath, szKey)  );
	}
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, sPath, szExtension)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, SG_string__sz(sPath))  );

	*ppPath = pPath;
	pPath = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sPath);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	return;
}

/**
 * Caches the result of matching a working copy in the file system.
 */
static void u0111__match_wc__cache__write(
	SG_context*        pCtx,        //< [in] [out] Error and context info.
	const SG_pathname* pRepoFolder, //< [in] Folder where the matched working copy was checked out.
	const char*        szKey,       //< [in] A key/name to identify this value in the cache.
	                                //<      May be NULL to only key on the repo folder.
	SG_bool            bMatch       //< [in] Value to write to the cache.
	)
{
	SG_pathname* pCachePath = NULL;
	SG_file*     pCacheFile = NULL;

	VERIFY_ERR_CHECK(  u0111__match_wc__get_data_path(pCtx, pRepoFolder, szKey, bMatch == SG_FALSE ? u0111__match_wc__false_cache : u0111__match_wc__true_cache, &pCachePath)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pCachePath, SG_FILE_CREATE_NEW | SG_FILE_WRONLY, 0644, &pCacheFile)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pCachePath);
	SG_FILE_NULLCLOSE(pCtx, pCacheFile);
	return;
}

/**
 * Reads the value cached by a prior call to u0111__match_wc__cache__write.
 */
static void u0111__match_wc__cache__read(
	SG_context*        pCtx,        //< [in] [out] Error and context info.
	const SG_pathname* pRepoFolder, //< [in] Folder where the matched working copy was checked out.
	const char*        szKey,       //< [in] The key/name to retrieve the cached value for.
	                                //<      May be NULL to only key on the repo folder.
	SG_bool*           pCached,     //< [out] Whether or not a cached result existed.
	SG_bool*           pMatch       //< [out] The value from the cached result.
	)
{
	SG_pathname* pPath   = NULL;
	SG_bool      bCached = SG_FALSE;
	SG_bool      bMatch  = SG_FALSE;

	VERIFY_ERR_CHECK(  u0111__match_wc__get_data_path(pCtx, pRepoFolder, szKey, u0111__match_wc__true_cache, &pPath)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bCached, NULL, NULL)  );
	if (bCached != SG_FALSE)
	{
		bMatch = SG_TRUE;
	}
	else
	{
		SG_PATHNAME_NULLFREE(pCtx, pPath);
		VERIFY_ERR_CHECK(  u0111__match_wc__get_data_path(pCtx, pRepoFolder, szKey, u0111__match_wc__false_cache, &pPath)  );
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bCached, NULL, NULL)  );
		if (bCached != SG_FALSE)
		{
			bMatch = SG_FALSE;
		}
	}

	*pCached = bCached;
	if (bCached != SG_FALSE)
	{
		*pMatch = bMatch;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	return;
}

/**
 * Matches the working folder from checking out a revision to a folder already on disk.
 */
static void u0111__match_wc(
	SG_context*             pCtx,           //< [in] [out] Error and context info.
	const u0111__test_data* pTestData,      //< [in] Data for the test that's executing.
	SG_rev_spec*            pRevision,      //< [in] The revision to checkout and compare.
	const char*             szFolder,       //< [in] The folder to compare the checked out one against.
	                                        //<      Relative to the test's base path.
	const char*             szRevisionName, //< [in] A unique name for the revision being compared.
	                                        //<      Should generally be the name of the branch/tag/rev in pRevision.
	                                        //<      If NULL, then szFolder should only ever be compared to one pRevision.
	SG_bool*                pMatch,         //< [out] Whether or not the folder's contents match.
	SG_pathname**           ppTestFolder,   //< [out] Full path containing the expected working copy.
	                                        //<       May be NULL if not needed.
	SG_pathname**           ppRepoFolder    //< [out] Full path containing the checked out working copy.
	                                        //<       May be NULL if not needed.
	)
{
	SG_bool      bCached     = SG_FALSE;
	SG_bool      bMatch      = SG_FALSE;
	SG_pathname* pTestFolder = NULL;
	SG_pathname* pRepoFolder = NULL;
	SG_string*   sLogPath    = NULL;
	SG_pathname* pLogPath    = NULL;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pRevision);
	SG_NULLARGCHECK(szFolder);
	SG_NULLARGCHECK(pMatch);

	// build a full path to the test's folder already on disk
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pTestFolder, pTestData->pBasePath, szFolder)  );

	// build a path to checkout to
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pRepoFolder, pTestData->pTempPath, szFolder)  );

	// check if we've already done this match
	VERIFY_ERR_CHECK(  u0111__match_wc__cache__read(pCtx, pRepoFolder, szRevisionName, &bCached, &bMatch)  );
	if (bCached == SG_FALSE)
	{
		SG_bool bExists = SG_FALSE;

		// build a path to log to
		VERIFY_ERR_CHECK(  u0111__match_wc__get_data_path(pCtx, pRepoFolder, szRevisionName, u0111__match_wc__diff_log, &pLogPath)  );

		// checkout the revision, if we haven't before
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pRepoFolder, &bExists, NULL, NULL)  );
		if (bExists == SG_FALSE)
		{
			SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pRepoFolder)  );
			SG_ERR_CHECK(  SG_wc__checkout(pCtx,
										   SG_string__sz(pTestData->sRepoName),
										   SG_pathname__sz(pRepoFolder),
										   pRevision,
										   NULL, NULL,
										   NULL)  );
		}

		// compare the folder contents
		VERIFY_ERR_CHECK(  u0111__match_wc__folders(pCtx, pTestData, pTestFolder, pRepoFolder, pLogPath, &bMatch)  );

		// cache the result so we don't try to do the match again
		VERIFY_ERR_CHECK(  u0111__match_wc__cache__write(pCtx, pRepoFolder, szRevisionName, bMatch)  );
	}

	// return the results
	*pMatch = bMatch;
	if (ppTestFolder != NULL)
	{
		*ppTestFolder = pTestFolder;
		pTestFolder = NULL;
	}
	if (ppRepoFolder != NULL)
	{
		*ppRepoFolder = pRepoFolder;
		pRepoFolder = NULL;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pLogPath);
	SG_STRING_NULLFREE(pCtx, sLogPath);
	SG_PATHNAME_NULLFREE(pCtx, pRepoFolder);
	SG_PATHNAME_NULLFREE(pCtx, pTestFolder);
	return;
}

// forward-declared because it is indirectly called recursively
static void u0111__match_commit(SG_context*, const u0111__test_data*, const char*, const char*, SG_bool*);

/**
 * Matches the user from a test commit with the user associated with a real HID.
 */
static void u0111__match_commit__user(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	const u0111__test_data* pTestData, //< [in] Data for the test that's executing.
	const SG_vhash*         pCommit,   //< [in] Test commit record to match.
	const char*             szHid,     //< [in] HID of the real commit to match against.
	SG_bool*                pMatch     //< [out] True if the users match, false otherwise.
	)
{
	SG_bool    bMatch   = SG_FALSE;
	SG_bool    bHasUser = SG_FALSE;
	SG_varray* pAudits  = NULL;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pCommit);
	SG_NULLARGCHECK(szHid);
	SG_NULLARGCHECK(pMatch);

	// if the test commit doesn't have a user, then it's a match
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pCommit, u0111__commit_key__user, &bHasUser)  );
	if (bHasUser == SG_FALSE)
	{
		bMatch = SG_TRUE;
	}
	else
	{
		const char* szTestUsername = NULL;
		SG_uint32   uAudits        = 0u;
		SG_uint32   uAudit         = 0u;

		// get the username associated with the test commit
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pCommit, u0111__commit_key__user, &szTestUsername)  );

		// get the audits for the given HID and look for one that matches
		SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pTestData->pRepo, SG_DAGNUM__VERSION_CONTROL, szHid, &pAudits)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pAudits, &uAudits)  );
		for (uAudit = 0u; uAudit < uAudits && bMatch == SG_FALSE; ++uAudit)
		{
			SG_vhash*   pAudit         = NULL;
			const char* szUserId       = NULL;
			SG_vhash*   pUser          = NULL;
			const char* szRepoUsername = NULL;

			// get the username associated with the audit
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pAudits, uAudit, &pAudit)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pAudit, SG_AUDIT__USERID, &szUserId)  );
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pTestData->pUsers, szUserId, &pUser)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, u0111__zing_user__name, &szRepoUsername)  );

			// match the user names
			if (strcmp(szTestUsername, szRepoUsername) == 0)
			{
				bMatch = SG_TRUE;
			}
		}
	}

	// return the result
	*pMatch = bMatch;

fail:
	SG_VARRAY_NULLFREE(pCtx, pAudits);
	return;
}

/**
 * Matches the timestamp from a test commit with the timestamp associated with a real HID.
 */
static void u0111__match_commit__time(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	const u0111__test_data* pTestData, //< [in] Data for the test that's executing.
	const SG_vhash*         pCommit,   //< [in] Test commit record to match.
	const char*             szHid,     //< [in] HID of the real commit to match against.
	SG_bool*                pMatch     //< [out] True if the times match, false otherwise.
	)
{
	SG_bool    bMatch   = SG_FALSE;
	SG_bool    bHasTime = SG_FALSE;
	SG_varray* pAudits  = NULL;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pCommit);
	SG_NULLARGCHECK(szHid);
	SG_NULLARGCHECK(pMatch);

	// if the test commit doesn't have a time, then it's a match
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pCommit, u0111__commit_key__time, &bHasTime)  );
	if (bHasTime == SG_FALSE)
	{
		bMatch = SG_TRUE;
	}
	else
	{
		SG_int64  iTestTime = 0;
		SG_uint32 uAudits   = 0u;
		SG_uint32 uAudit    = 0u;

		// get the timestamp associated with the test commit and convert it to milliseconds
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pCommit, u0111__commit_key__time, &iTestTime)  );
		iTestTime *= 1000;

		// get the audits for the given HID and look for one that matches
		SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pTestData->pRepo, SG_DAGNUM__VERSION_CONTROL, szHid, &pAudits)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pAudits, &uAudits)  );
		for (uAudit = 0u; uAudit < uAudits && bMatch == SG_FALSE; ++uAudit)
		{
			SG_vhash* pAudit    = NULL;
			SG_int64  iRepoTime = 0;

			// get the time associated with the audit
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pAudits, uAudit, &pAudit)  );
			SG_ERR_CHECK(  SG_vhash__get__int64_or_double(pCtx, pAudit, SG_AUDIT__TIMESTAMP, &iRepoTime)  );

			// match the times
			if (iTestTime == iRepoTime)
			{
				bMatch = SG_TRUE;
			}
		}
	}

	// return the result
	*pMatch = bMatch;

fail:
	SG_VARRAY_NULLFREE(pCtx, pAudits);
	return;
}

/**
 * Matches the comments from a test commit with the comments associated with a real HID.
 */
static void u0111__match_commit__comments(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	const u0111__test_data* pTestData, //< [in] Data for the test that's executing.
	const SG_vhash*         pCommit,   //< [in] Test commit record to match.
	const char*             szHid,     //< [in] HID of the real commit to match against.
	SG_bool*                pMatch     //< [out] True if the comments all match, false otherwise.
	)
{
	SG_bool    bMatch        = SG_FALSE;
	SG_varray* pTestComments = NULL;
	SG_varray* pRepoComments = NULL;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pCommit);
	SG_NULLARGCHECK(szHid);
	SG_NULLARGCHECK(pMatch);

	// if the test commit doesn't have any comments, then it's a match
	SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pCommit, u0111__commit_key__comments, &pTestComments)  );
	if (pTestComments == NULL)
	{
		bMatch = SG_TRUE;
	}
	else
	{
		SG_uint32 uTestComments = 0u;
		SG_uint32 uTestComment  = 0u;

		// get the comments associated with the HID
		SG_ERR_CHECK(  SG_vc_comments__lookup(pCtx, pTestData->pRepo, szHid, &pRepoComments)  );

		// assume they will all match until we find one that doesn't
		bMatch = SG_TRUE;

		// run through the test comments
		SG_ERR_CHECK(  SG_varray__count(pCtx, pTestComments, &uTestComments)  );
		for (
			uTestComment = 0u;
			uTestComment < uTestComments && bMatch == SG_TRUE;
			++uTestComment
			)
		{
			const char* szTestComment = NULL;
			SG_bool     bCurrentMatch = SG_FALSE;
			SG_uint32   uRepoComments = 0u;
			SG_uint32   uRepoComment  = 0u;

			// get the text of the current test comment
			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pTestComments, uTestComment, &szTestComment)  );

			// run through the repo comments looking for a match
			SG_ERR_CHECK(  SG_varray__count(pCtx, pRepoComments, &uRepoComments)  );
			for (
				uRepoComment = 0u;
				uRepoComment < uRepoComments && bCurrentMatch == SG_FALSE;
				++uRepoComment
				)
			{
				SG_vhash*   pRepoComment  = NULL;
				const char* szRepoComment = NULL;

				// get the text of the current repo comment
				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pRepoComments, uRepoComment, &pRepoComment)  );
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pRepoComment, u0111__zing_comment__text, &szRepoComment)  );

				// check if this one matches
				// Only compare the length of the test comment, it's meant to just be
				// the beginning of a long comment, usually just the first line.
				if (strncmp(szTestComment, szRepoComment, SG_STRLEN(szTestComment)) == 0)
				{
					bCurrentMatch = SG_TRUE;
				}
			}

			// if we didn't find a match for this one,
			// then the comments as a group don't match each other
			if (bCurrentMatch == SG_FALSE)
			{
				bMatch = SG_FALSE;
			}
		}
	}

	// return the result
	*pMatch = bMatch;

fail:
	SG_VARRAY_NULLFREE(pCtx, pRepoComments);
	return;
}

/**
 * Matches the working copy from a test commit with a checkout from a real HID.
 */
static void u0111__match_commit__wc(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	const u0111__test_data* pTestData, //< [in] Data for the test that's executing.
	const SG_vhash*         pCommit,   //< [in] Test commit record to match.
	const char*             szHid,     //< [in] HID of the real commit to match against.
	SG_bool*                pMatch     //< [out] True if the working copies match, false otherwise.
	)
{
	SG_rev_spec* pRevision = NULL;
	const char*  szFolder  = NULL;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pCommit);
	SG_NULLARGCHECK(szHid);
	SG_NULLARGCHECK(pMatch);

	// build a rev spec containing the tag
	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevision)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevision, szHid)  );

	// get the folder to match against from the commit data
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pCommit, u0111__commit_key__wc, &szFolder)  );

	// run the match
	if (szFolder != NULL)
	{
		const char* szCommit = NULL;

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pCommit, u0111__commit_key__name, &szCommit)  );
		VERIFY_ERR_CHECK(  u0111__match_wc(pCtx, pTestData, pRevision, szFolder, szCommit, pMatch, NULL, NULL)  );
	}
	else
	{
		// no expected working copy for this commit, it matches by default
		*pMatch = SG_TRUE;
	}

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevision);
	return;
}

/**
 * Matches the parents from a test commit with the parents from a real HID.
 */
static void u0111__match_commit__parents(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	const u0111__test_data* pTestData, //< [in] Data for the test that's executing.
	const SG_vhash*         pCommit,   //< [in] Test commit record to match.
	const char*             szHid,     //< [in] HID of the real commit to match against.
	SG_bool*                pMatch     //< [out] True if the parents match, false otherwise.
	)
{
	SG_bool     bMatch       = SG_FALSE;
	SG_varray*  pTestParents = NULL;
	SG_dagnode* pNode        = NULL;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pCommit);
	SG_NULLARGCHECK(szHid);
	SG_NULLARGCHECK(pMatch);

	// if the test commit doesn't have any parents, then it's a match
	SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pCommit, u0111__commit_key__parents, &pTestParents)  );
	if (pTestParents == NULL)
	{
		bMatch = SG_TRUE;
	}
	else
	{
		SG_uint32 uRepoParents  = 0u;
		const char ** ppRepoParents = NULL;
		SG_uint32 uTestParents  = 0u;
		SG_uint32 uTestParent   = 0u;

		// get the parents associated with the repo commit
		SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pTestData->pRepo, SG_DAGNUM__VERSION_CONTROL, szHid, &pNode)  );
		SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pNode, &uRepoParents, &ppRepoParents)  );

		// assume they will all match until we find one that doesn't
		bMatch = SG_TRUE;

		// run through all the test parents
		SG_ERR_CHECK(  SG_varray__count(pCtx, pTestParents, &uTestParents)  );
		for (
			uTestParent = 0u;
			uTestParent < uTestParents && bMatch == SG_TRUE;
			++uTestParent
			)
		{
			SG_bool     bCurrentMatch = SG_FALSE;
			const char* szTestParent  = NULL;
			const char**ppRepoParent  = NULL;

			// get the current test parent
			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pTestParents, uTestParent, &szTestParent)  );

			// run through the repo parents looking for a match
			for (
				ppRepoParent = ppRepoParents;
				(ppRepoParent != ppRepoParents + uRepoParents) && (bCurrentMatch == SG_FALSE);
				++ppRepoParent
				)
			{
				VERIFY_ERR_CHECK(  u0111__match_commit(pCtx, pTestData, szTestParent, *ppRepoParent, &bCurrentMatch)  );
			}

			// if we didn't find a match for this one,
			// then the parents as a group don't match each other
			if (bCurrentMatch == SG_FALSE)
			{
				bMatch = SG_FALSE;
			}
		}
	}

	// return the result
	*pMatch = bMatch;

fail:
	SG_DAGNODE_NULLFREE(pCtx, pNode);
	return;
}

/**
 * Checks if a named commit from the test matches an actual commit from the repo.
 */
static void u0111__match_commit(
	SG_context*             pCtx,      //< [in] [out] Error and context info.
	const u0111__test_data* pTestData, //< [in] Data for the test that's executing.
	const char*             szName,    //< [in] Named commit to match against the real one.
	const char*             szHid,     //< [in] HID of the real commit to match against the named one.
	SG_bool*                pMatch     //< [out] Whether or not the commits match.
	)
{
	SG_vhash*   pCommit   = NULL;
	SG_bool     bMatch    = SG_FALSE;
	const char* szTestHid = NULL;

	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(szHid);
	SG_NULLARGCHECK(pMatch);

	// get the test commit data
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pTestData->pCommits, szName, &pCommit)  );

	// check if we already know this test commit's HID from a prior match
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pCommit, u0111__commit_key__hid, &szTestHid)  );
	if (szTestHid != NULL)
	{
		// yup we do, just compare the HIDs
		if (strcmp(szTestHid, szHid) == 0)
		{
			bMatch = SG_TRUE;
		}
	}
	else
	{
		// nope, we'll have to compare field by field
		VERIFY_ERR_CHECK(  u0111__match_commit__user(pCtx, pTestData, pCommit, szHid, &bMatch)  );
		if (bMatch != SG_FALSE)
		{
			VERIFY_ERR_CHECK(  u0111__match_commit__time(pCtx, pTestData, pCommit, szHid, &bMatch)  );
			if (bMatch != SG_FALSE)
			{
				VERIFY_ERR_CHECK(  u0111__match_commit__comments(pCtx, pTestData, pCommit, szHid, &bMatch)  );
				if (bMatch != SG_FALSE)
				{
					VERIFY_ERR_CHECK(  u0111__match_commit__wc(pCtx, pTestData, pCommit, szHid, &bMatch)  );
					if (bMatch != SG_FALSE)
					{
						VERIFY_ERR_CHECK(  u0111__match_commit__parents(pCtx, pTestData, pCommit, szHid, &bMatch)  );
						if (bMatch == SG_FALSE)
						{
							// log that we matched except for the parents, helps in debugging
							INFOP("Commit Match Without Parents", ("Name(%s) CSID(%s)", szName, szHid));
						}
						else
						{
							// remember the HID that matched this commit
							SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pCommit, u0111__commit_key__hid, szHid)  );
							INFOP("Commit Match", ("Name(%s) CSID(%s)", szName, szHid));
						}
					}
				}
			}
		}
	}

	// return the result
	*pMatch = bMatch;

fail:
	return;
}


/*
**
** Parsing
**
*/

/**
 * Builds the repo name to use for a test file.
 */
static void u0111__get_test_repo_name(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	const SG_pathname* pPath, //< [in] Path of the test file being run.
	SG_string**        ppName //< [out] Name of the repo to use.
	)
{
	const char* szFilename = NULL;
	SG_string*  sTestName  = NULL;
	SG_string*  sName      = NULL;

	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(ppName);

	SG_ERR_CHECK(  SG_pathname__filename(pCtx, pPath, &szFilename)  );
	SG_ERR_CHECK(  SG_validate__sanitize__trim(pCtx, szFilename, 1u, 256u, SG_VALIDATE__BANNED_CHARS, SG_VALIDATE__RESULT__ALL, "_", "_", &sTestName)  );
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sName, u0111__repo_name_format, SG_string__sz(sTestName))  );

	*ppName = sName;
	sName = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sTestName);
	SG_STRING_NULLFREE(pCtx, sName);
	return;
}

/**
 * Creates a repo to pull shared users from.
 */
static void u0111__create_shared_users_repo(
	SG_context*      pCtx,             //< [in] [out] Error and context info.
	const SG_string* sBaseRepo,        //< [in] Name of the base repo being imported.
	const SG_vhash*  pUsers,           //< [in] Users to create in the shared users repo.
	                                   //<      [USERNAME]=NULL.
	SG_string**      ppSharedUsersRepo //< [out] Name of the created shared users repo.
	                                   //<       May be NULL if not needed.
	)
{
	char szAdminId[SG_GID_BUFFER_LENGTH];
	SG_string* sSharedUsersRepo = NULL;
	SG_repo*   pRepo            = NULL;
	SG_uint32  uUsers           = 0u;
	SG_uint32  uUser            = 0u;

	// generate the repo's admin ID and descriptor name
	SG_ERR_CHECK(  SG_gid__generate(pCtx, szAdminId, sizeof(szAdminId))  );
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sSharedUsersRepo, u0111__shared_users_repo_name_format, SG_string__sz(sBaseRepo))  );

	// create the repo and open it
	SG_ERR_CHECK(  SG_repo__create__completely_new__empty__closet(pCtx, szAdminId, NULL, NULL, SG_string__sz(sSharedUsersRepo))  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(sSharedUsersRepo), &pRepo)  );

	// add any specified users to it
	if (pUsers != NULL)
	{
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pUsers, &uUsers)  );
		for (uUser = 0u; uUser < uUsers; ++uUser)
		{
			const char* szUser = NULL;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pUsers, uUser, &szUser, NULL)  );
			SG_ERR_CHECK(  SG_user__create(pCtx, pRepo, szUser, NULL)  );
		}
	}

	// return the repo's name
	if (ppSharedUsersRepo != NULL)
	{
		*ppSharedUsersRepo = sSharedUsersRepo;
		sSharedUsersRepo = NULL;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sSharedUsersRepo);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

#if 0
/**
 * Reads a single newline (of any platform-style) from a file.
 * Throws an error if the file's current position isn't a newline.
 *
 * TODO: Move this into SG_file someday.
 */
static void u0111__read_newline(
	SG_context* pCtx,  //< [in] [out] Error and context info.
	SG_file*    pFile, //< [in] File to read a newline from.
	SG_bool*    pEOF   //< [out] Whether or not we hit the end-of-file.
	)
{
	SG_uint64 uPosition  = 0u;
	SG_byte   aBytes[2u];
	SG_uint32 uBytes     = 0u;
	SG_bool   bEOF       = SG_FALSE;

	SG_NULLARGCHECK(pFile);

	SG_ERR_CHECK(  SG_file__tell(pCtx, pFile, &uPosition)  );

	SG_ERR_TRY(  SG_file__read(pCtx, pFile, sizeof(aBytes), aBytes, &uBytes)  );
	SG_ERR_CATCH_SET(SG_ERR_EOF, bEOF, SG_TRUE);
	SG_ERR_CATCH_END;

	if (uBytes == 2u && (aBytes[0u] == '\r' && aBytes[1u] == '\n'))
	{
		uPosition += 2u;
	}
	else if (uBytes >= 1u && (aBytes[0u] == '\r' || aBytes[0u] == '\n'))
	{
		uPosition += 1u;
	}
	else if (bEOF == SG_FALSE)
	{
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected newline not found."));
	}

	SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, uPosition)  );

	if (pEOF != NULL)
	{
		*pEOF = bEOF;
	}

fail:
	return;
}

/**
 * Reads until a specific set of characters is [not] found.
 *
 * TODO: Move this into SG_file someday.
 */
static void u0111__read_while(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	SG_file*    pFile,   //< [in] File to read a word from.
	const char* szChars, //< [in] The set of characters to compare against.
	SG_bool     bWhile,  //< [in] If true, read WHILE next char is in szChars.
	                     //<      If false, read UNTIL next char is in szChars.
	SG_string** ppWord,  //< [out] Data that was read.
	                     //<       May be NULL if not needed.
	SG_bool*    pEOF,    //< [out] True if EOF was encountered.
	                     //<       May be NULL if not needed.
	SG_int32*   pChar    //< [out] The first character that didn't meet criteria, and so stopped the reading.
	                     //<       Unchanged/ignored if EOF was found first.
	                     //<       May be NULL if not needed.
	)
{
	SG_uint64  uPosition = 0u;       //< Position in the file we've read through.
	SG_string* sWord     = NULL;     //< Return value.
	SG_bool    bEOF      = SG_FALSE; //< Whether or not we found the end-of-file.
	SG_bool    bFound    = SG_FALSE; //< Whether or not we found something [not] in szChars.
	SG_byte    aBytes[80];           //< Buffer that we're reading into and processing.
	SG_uint32  uIndex    = 0u;       //< Currently interesting index into aBytes.
	SG_int32   iChar     = 0;        //< Current character we're examining.

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(szChars);

	// start at the file's current position
	SG_ERR_CHECK(  SG_file__tell(pCtx, pFile, &uPosition)  );

	// if they want the string we read, allocate an empty one to start
	if (ppWord != NULL)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sWord)  );
	}

	// keep going until we find the EOF or something [not] in szChars
	while (bEOF == SG_FALSE && bFound == SG_FALSE)
	{
		SG_uint32 uBytes = 0u; //< Effective size of aBytes.

		// fill aBytes from the file, starting at uIndex
		// We might have left ourselves uIndex number of bytes at the beginning
		// of the buffer that still require processing.  That'd happen if a multi-
		// byte character got split across buffer boundaries.
		SG_ERR_TRY(  SG_file__read(pCtx, pFile, sizeof(aBytes) - uIndex, aBytes + uIndex, &uBytes)  );
		SG_ERR_CATCH_SET(SG_ERR_EOF, bEOF, SG_TRUE);
		SG_ERR_CATCH_END;

		// if we didn't find the EOF, process the buffer
		if (bEOF == SG_FALSE)
		{
			SG_uint32 uNextIndex = 0u; //< Index of the character after the current one.

			// start processing at the beginning of the buffer
			uBytes += uIndex;
			uIndex = 0u;

			// loop over each UTF8 character in the buffer
			// leave uIndex indicating the character we stopped at
			SG_ERR_CHECK(  SG_utf8__next_char(pCtx, aBytes, uBytes, uIndex, &iChar, &uNextIndex)  );
			while (bFound == SG_FALSE && iChar >= 0 && uIndex < uBytes)
			{
				SG_bool bContains = SG_FALSE;

				// check if this character is in the set of interesting characters
				SG_ERR_CHECK(  SG_utf8__contains_char(pCtx, szChars, iChar, &bContains)  );
				if (bWhile != bContains)
				{
					bFound = SG_TRUE;
				}
				else
				{
					uIndex = uNextIndex;
					SG_ERR_CHECK(  SG_utf8__next_char(pCtx, aBytes, uBytes, uIndex, &iChar, &uNextIndex)  );
				}
			}
			SG_ASSERT(uIndex <= uBytes);

			// append everything we've read so far onto the return value, if they care about it
			if (sWord != NULL && uIndex > 0u)
			{
				SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, sWord, aBytes, uIndex)  );
			}

			// keep track of where in the file we've processed up to
			uPosition += uIndex;

			// if we haven't found what we're looking for yet, prepare to read another buffer
			if (bFound == SG_FALSE)
			{
				// we should either be at the end of the buffer, or have a negative
				// (invalid) char, which would indicate that we chopped a multi-byte
				// character at the end of the buffer
				SG_ASSERT(uIndex == uBytes || iChar < 0);

				// if there are unprocessed characters at the end of the buffer, copy
				// them to the beginning of the buffer and we'll process them next
				// time; leave uIndex indicating how many such characters there are
				if (uIndex < uBytes)
				{
					memcpy(aBytes, aBytes + uIndex, uBytes - uIndex);
				}
				uIndex = uBytes - uIndex;
			}
		}
	}
	SG_ASSERT(bEOF != bFound);

	// if we didn't hit the end of the file, seek to the end of where we processed
	if (bEOF == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, uPosition)  );
	}

	// return whichever values the caller wanted
	if (ppWord != NULL)
	{
		*ppWord = sWord;
		sWord = NULL;
	}
	if (pChar != NULL && bFound != SG_FALSE)
	{
		*pChar = iChar;
	}
	if (pEOF != NULL)
	{
		*pEOF = bEOF;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sWord);
	return;
}
#endif

/**
 * Read the next line/command from the file and run it.
 */
static void u0111__run_command(
	SG_context*                pCtx,           //< [in] [out] Error and context info.
	SG_file*                   pFile,          //< [in] File to read/run the next line/command from.
	const u0111__command_spec* pCommands,      //< [in] Array of available commands.
	SG_uint32                  uCommandsCount, //< [in] Size of pCommands array.
	SG_ihash*                  pCommandCounts, //< [in] Hash to track how many times each command has run.
	u0111__test_data*          pTestData,      //< [in] Data for the current test.
	void*                      pData,          //< [in] Data to pass to the command's implementation.
	SG_bool*                   pEmpty          //< [out] True if the next line was empty, false otherwise.
	                                           //<       May be NULL if not needed.
	)
{
	SG_bool        bEOF     = SG_FALSE; //< Whether or not we hit the end-of-file.
	SG_string*     sCommand = NULL;     //< Name of the command we're processing.
	SG_bool        bEmpty   = SG_FALSE; //< Whether or not the line was empty.
	u0111__command oCommand;            //< Data for the command we're processing.

	SG_NULLARGCHECK(pCommands);
	SG_NULLARGCHECK(pCommandCounts);
	SG_NULLARGCHECK(pTestData);

	// initialize the command data
	u0111__command__init(pCtx, &oCommand);

	// read the first word on the line
	SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, " \t", SG_TRUE, NULL, NULL)  );
	SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, " \t\r\n", SG_FALSE, &sCommand, NULL)  );
	bEmpty = (SG_string__length_in_bytes(sCommand) == 0u ? SG_TRUE : SG_FALSE);

	// skip lines whose first word indicates a comment
	SG_ERR_CHECK(  SG_file__eof(pCtx, pFile, &bEOF)  );
	while (bEOF == SG_FALSE && bEmpty == SG_FALSE && SG_string__sz(sCommand)[0] == '#')
	{
		SG_STRING_NULLFREE(pCtx, sCommand);
		SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, "\r\n", SG_FALSE, NULL, NULL)  );
		SG_ERR_CHECK(  SG_file__read_newline(pCtx, pFile)  );
		SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, " \t", SG_TRUE, NULL, NULL)  );
		SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, " \t\r\n", SG_FALSE, &sCommand, NULL)  );
		bEmpty = (SG_string__length_in_bytes(sCommand) == 0u ? SG_TRUE : SG_FALSE);
		SG_ERR_CHECK(  SG_file__eof(pCtx, pFile, &bEOF)  );
	}

	// if we have a command to process, do it
	if (bEOF == SG_FALSE && bEmpty == SG_FALSE)
	{
		SG_uint32                  uIndex = 0u;   //< Current index in pCommands array.
		const u0111__command_spec* pSpec  = NULL; //< Spec of the command we're processing.
		SG_int64                   iCount = 0;    //< Number of times we've processed this command.
		SG_uint32                  uCount = 0u;   //< Number of times we've processed this command.
		SG_uint32                  uArg   = 0u;   //< Index into the command's arguments.

		// look for a matching command spec in the given list
		for (uIndex = 0u; uIndex < uCommandsCount; ++uIndex)
		{
			if (strcmp(SG_string__sz(sCommand), pCommands[uIndex].szName) == 0)
			{
				pSpec = pCommands + uIndex;
			}
		}
		if (pSpec == NULL)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Unknown command: %s", SG_string__sz(sCommand)));
		}

		// update the number of times this command has occurred in the block
		// and make sure we haven't gone over its limit
		SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pCommandCounts, SG_string__sz(sCommand), &iCount)  );
		uCount = (SG_uint32)iCount;
		++uCount;
		if (pSpec->uCount > 0u && uCount > pSpec->uCount)
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Command occurs too many times: %s (Max: %u)", pSpec->szName, pSpec->uCount));
		}
		SG_ERR_CHECK(  SG_ihash__update__int64(pCtx, pCommandCounts, SG_string__sz(sCommand), (SG_int64)uCount)  );

		// configure the command
		oCommand.sName = sCommand;
		sCommand = NULL;

		// parse the line's remaining arguments according to the command
		for (uArg = 0u; uArg < pSpec->uArgs; ++uArg)
		{
			// skip any whitespace in front of the argument
			SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, " \t", SG_TRUE, NULL, NULL)  );

			if (uArg + 1u == pSpec->uArgs)
			{
				// last argument gets the rest of the line
				SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, "\r\n", SG_FALSE, &oCommand.aArgs[uArg], NULL)  );
				SG_ERR_CHECK(  SG_file__read_newline(pCtx, pFile)  );
			}
			else
			{
				SG_int32 iChar = 0;

				// current argument gets the next word
				SG_ERR_CHECK(  SG_file__read_while(pCtx, pFile, " \t\r\n", SG_FALSE, &oCommand.aArgs[uArg], &iChar)  );
				SG_ERR_CHECK(  SG_file__eof(pCtx, pFile, &bEOF)  );
				if (bEOF != SG_FALSE || iChar == '\r' || iChar == '\n')
				{
					SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Not enough arguments for command: %s (Expected: %u)", pSpec->szName, pSpec->uCount));
				}
			}
		}

		// run the command's implementation and ignore any errors it throws
		SG_ERR_CHECK(  pSpec->fImplementation(pCtx, pFile, &oCommand, pTestData, pData)  );
	}

	// finished
	if (pEmpty != NULL)
	{
		*pEmpty = bEmpty;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sCommand);
	u0111__command__free(pCtx, &oCommand);
}

/**
 * Read and run the next block of commands, terminated by an empty line.
 * Leaves the file pointing at the terminating empty line.
 */
static void u0111__run_block(
	SG_context*                pCtx,      //< [in] [out] Error and context info.
	SG_file*                   pFile,     //< [in] File to read/run the next line/command from.
	const u0111__command_spec* pCommands, //< [in] Array of available commands.
	SG_uint32                  uCommands, //< [in] Size of pCommands array.
	u0111__test_data*          pTestData, //< [in] Data for the current test.
	void*                      pData      //< [in] Data to pass to the command's implementation.
	)
{
	SG_ihash* pCounts = NULL;
	SG_bool   bEmpty  = SG_FALSE;
	SG_bool   bEOF    = SG_FALSE;

	SG_NULLARGCHECK(pCommands);
	SG_NULLARGCHECK(pTestData);

	SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pCounts)  );

	SG_ERR_CHECK(  SG_file__eof(pCtx, pFile, &bEOF)  );
	while (bEmpty == SG_FALSE && bEOF == SG_FALSE)
	{
		VERIFY_ERR_CHECK(  u0111__run_command(pCtx, pFile, pCommands, uCommands, pCounts, pTestData, pData, &bEmpty)  );
		SG_ERR_CHECK(  SG_file__eof(pCtx, pFile, &bEOF)  );
	}

fail:
	SG_IHASH_NULLFREE(pCtx, pCounts);
}

/**
 * Runs a test import defined by a .test file.
 */
static void u0111__run_test_file(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_pathname**      ppPath,    //< [in] The .test file that defines the test.
	                              //<      We steal this pointer and NULL the caller's.
	const SG_pathname* pTempPath, //< [in] Folder where tests can store temporary files.
	SG_bool            bLong      //< [in] Whether or not we should run tests with the "long-test" option enabled.
	)
{
	u0111__test_data oData;
	SG_file*         pFile     = NULL;
	SG_bool          bEOF      = SG_FALSE;
	SG_bool          bLongTest = SG_FALSE;

	SG_NULLARGCHECK(ppPath);

	// initialize our data
	u0111__test_data__init(pCtx, &oData, ppPath);

	// open the test file
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, oData.pTestFile, SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY, 0, &pFile)  );

	// run the header block to populate our global data
	VERIFY_ERR_CHECK(  u0111__run_block(pCtx, pFile, u0111__gaHeaderCommands, SG_NrElements(u0111__gaHeaderCommands), &oData, NULL)  );
	SG_ERR_CHECK(  SG_file__read_newline(pCtx, pFile)  );

	// only run this test if it's short, or if long tests are okay
	SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, oData.pOptions, U0111__OPTION__LONG_TEST, &bLongTest)  );
	if (bLongTest == SG_FALSE || bLong != SG_FALSE)
	{
		const char* szSharedUsers = NULL;

		// build a repo name for this test import
		VERIFY_ERR_CHECK(  u0111__get_test_repo_name(pCtx, oData.pTestFile, &oData.sRepoName)  );

		// build a shared users repo for this test import, if needed
		if (oData.pSharedUsers != NULL)
		{
			VERIFY_ERR_CHECK(  u0111__create_shared_users_repo(pCtx, oData.sRepoName, oData.pSharedUsers, &oData.sSharedUsers)  );
			szSharedUsers = SG_string__sz(oData.sSharedUsers);
		}

		// import the specified file and make sure we get the expected result
		VERIFY_ERR_CHECK_ERR_EQUALS(  SG_fast_import__import(pCtx, SG_pathname__sz(oData.pImportFile), SG_string__sz(oData.sRepoName), NULL, NULL, szSharedUsers), oData.eResult  );
		if (SG_IS_OK(oData.eResult))
		{
			SG_uint32 uCommits = 0u;
			SG_uint32 uCommit  = 0u;

			// open the imported repo and fetch some data
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(oData.sRepoName), &oData.pRepo)  );
			SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, oData.pRepo, SG_DAGNUM__VERSION_CONTROL, &oData.pLeaves)  );
			VERIFY_ERR_CHECK(  u0111__get_users(pCtx, oData.pRepo, &oData.pUsers)  );
			SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, oData.pRepo, &oData.pBranches)  );
			SG_ERR_CHECK(  SG_vc_tags__list(pCtx, oData.pRepo, &oData.pTags)  );

			// create a path where this individual test can store temporary data
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &oData.pTempPath, pTempPath, SG_string__sz(oData.sRepoName))  );
			SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, oData.pTempPath)  );

			// run the rest of the blocks in the test
			SG_ERR_CHECK(  SG_file__eof(pCtx, pFile, &bEOF)  );
			while (bEOF == SG_FALSE)
			{
				VERIFY_ERR_CHECK(  u0111__run_block(pCtx, pFile, u0111__gaGlobalCommands, SG_NrElements(u0111__gaGlobalCommands), &oData, NULL)  );
				SG_ERR_CHECK(  SG_file__read_newline(pCtx, pFile)  );
				SG_ERR_CHECK(  SG_file__eof(pCtx, pFile, &bEOF)  );
			}

			// make sure all the commits in the test data were matched by real ones
			SG_ERR_CHECK(  SG_vhash__count(pCtx, oData.pCommits, &uCommits)  );
			for (uCommit = 0u; uCommit < uCommits; ++uCommit)
			{
				SG_vhash*   pCommit  = NULL;
				const char* szCommit = NULL;
				SG_bool     bMatched = SG_FALSE;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, oData.pCommits, uCommit, &szCommit, &pCommit)  );
				SG_ERR_CHECK(  SG_vhash__has(pCtx, pCommit, u0111__commit_key__hid, &bMatched)  );
				VERIFYP_COND("Named commit never matched to actual commit.", bMatched != SG_FALSE, ("CommitName(%s)", szCommit));
			}
		}
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	u0111__test_data__free(pCtx, &oData);
	return;
}

/**
 * Type of data expected by u0111__run_test_files__callback.
 */
typedef struct u0111__run_test_files__callback_data
{
	const SG_pathname* pFolder;   //< [in] Path of the folder being iterated.
	const SG_pathname* pTempPath; //< [in] Folder where tests can store temporary files.
	SG_bool            bLong;     //< [in] Whether or not to run long tests.
}
u0111__run_test_files__callback_data;

/**
 * SG_dir__foreach callback that finds .test files and runs them.
 */
static void u0111__run_test_files__callback(
	SG_context*      pCtx,     //< [in] [out] Error and context info.
	const SG_string* sName,    //< [in] Entry name of the current file, no path.
	SG_fsobj_stat*   pStat,    //< [in] Stat of the current file.
	void*            pUserData //< [in] u0111__run_test_files__callback_data instance.
	)
{
	const u0111__run_test_files__callback_data* pData = (const u0111__run_test_files__callback_data*)pUserData;
	SG_pathname*                                pPath = NULL;

	SG_NULLARGCHECK(sName);
	SG_NULLARGCHECK(pStat);
	SG_NULLARGCHECK(pUserData);

	// make sure this is a file
	if (pStat->type == SG_FSOBJ_TYPE__REGULAR)
	{
		SG_bool bTest = SG_FALSE;

		// check if it has a .test extension
		SG_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, "*.test", SG_string__sz(sName), 0u, &bTest)  );
		if (bTest != SG_FALSE)
		{
			// write a header in the log for this test
			VERIFY_ERR_CHECK(  _begin_test_label(__FILE__, __LINE__, SG_string__sz(sName))  );

			// construct the .test file's full path and run it
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pData->pFolder, SG_string__sz(sName))  );
			VERIFY_ERR_CHECK(  u0111__run_test_file(pCtx, &pPath, pData->pTempPath, pData->bLong)  );
		}
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	return;
}

/**
 * Runs the priority test in a given folder, if it exists.
 */
static void u0111__run_priority_test_file(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	const SG_pathname* pFolder,   //< [in] The folder to run the priority test in.
	const SG_pathname* pTempPath, //< [in] Folder where tests can store temporary files.
	SG_bool*           pExists    //< [out] Whether or not the priority test existed and was run.
	)
{
	SG_pathname* pPriority = NULL;
	SG_bool      bExists   = SG_FALSE;

	SG_NULLARGCHECK(pFolder);
	SG_NULLARGCHECK(pExists);

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPriority, pFolder, u0111__priority_test_name)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPriority, &bExists, NULL, NULL)  );
	if (bExists != SG_FALSE)
	{
		VERIFY_ERR_CHECK(  u0111__run_test_file(pCtx, &pPriority, pTempPath, SG_TRUE)  );
	}

	*pExists = bExists;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPriority);
	return;
}

/**
 * Run all tests defined by .test files in the given folder.
 */
static void u0111__run_test_files(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	const SG_pathname* pFolder,   //< [in] Folder to run .test files in.
	const SG_pathname* pTempPath, //< [in] Folder where tests can store temporary files.
	SG_bool            bLong      //< [in] Whether or not to run long tests.
	)
{
	u0111__run_test_files__callback_data oData;
	SG_bool                              bPriority = SG_FALSE;

	SG_NULLARGCHECK(pFolder);

	// try running the priority test first
	VERIFY_ERR_CHECK(  u0111__run_priority_test_file(pCtx, pFolder, pTempPath, &bPriority)  );
	if (bPriority == SG_FALSE)
	{
		// okay no priority test, run all the tests in the folder
		oData.pFolder   = pFolder;
		oData.pTempPath = pTempPath;
		oData.bLong     = bLong;
		SG_ERR_CHECK(  SG_dir__foreach(pCtx, pFolder, SG_DIR__FOREACH__SKIP_SPECIAL | SG_DIR__FOREACH__STAT, u0111__run_test_files__callback, (void*)&oData)  );
	}

fail:
	return;
}


/*
**
** Parsers
**
*/

static void u0111__parse_value__bool(
	SG_context* pCtx,
	const char* szValue,
	SG_variant* pValue
	)
{
	SG_bool bValue = SG_FALSE;

	SG_NULLARGCHECK(szValue);
	SG_NULLARGCHECK(pValue);

	if (strcmp("true", szValue) == 0)
	{
		bValue = SG_TRUE;
	}
	else if (strcmp("yes", szValue) == 0)
	{
		bValue = SG_TRUE;
	}
	else if (strcmp("on", szValue) == 0)
	{
		bValue = SG_TRUE;
	}
	else if (strcmp("1", szValue) == 0)
	{
		bValue = SG_TRUE;
	}
	if (strcmp("false", szValue) == 0)
	{
		bValue = SG_FALSE;
	}
	else if (strcmp("no", szValue) == 0)
	{
		bValue = SG_FALSE;
	}
	else if (strcmp("off", szValue) == 0)
	{
		bValue = SG_FALSE;
	}
	else if (strcmp("0", szValue) == 0)
	{
		bValue = SG_FALSE;
	}
	else
	{
		SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Can't parse bool from: %s", szValue));
	}

	pValue->type = SG_VARIANT_TYPE_BOOL;
	pValue->v.val_bool = bValue;

fail:
	return;
}

static void u0111__parse_value__branch_status(
	SG_context* pCtx,
	const char* szValue,
	SG_variant* pValue
	)
{
	SG_bool bValue = SG_FALSE;

	SG_NULLARGCHECK(szValue);
	SG_NULLARGCHECK(pValue);

	if (strcmp("open", szValue) == 0)
	{
		bValue = SG_TRUE;
	}
	else if (strcmp("closed", szValue) == 0)
	{
		bValue = SG_FALSE;
	}
	else
	{
		SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Can't parse branch status from: %s", szValue));
	}

	pValue->type = SG_VARIANT_TYPE_BOOL;
	pValue->v.val_bool = bValue;

fail:
	return;
}


/*
**
** Commands
**
*/

static void u0111__do_command__import(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// first argument is the filename to import, relative to the test's base path
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pTestData->pImportFile, pTestData->pBasePath, SG_string__sz(pCommand->aArgs[0u]))  );

	// run the import block
	VERIFY_ERR_CHECK(  u0111__run_block(pCtx, pFile, u0111__gaImportCommands, SG_NrElements(u0111__gaImportCommands), pTestData, NULL)  );

fail:
	return;
}

static void u0111__do_command__import__option(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_uint32                 uIndex  = 0u;
	const u0111__option_spec* pOption = u0111__options + uIndex;
	SG_variant                oValue;

	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// find the spec of the option being specified
	for (uIndex = 0u; uIndex < SG_NrElements(u0111__options); ++uIndex)
	{
		if (strcmp(u0111__options[uIndex].szName, SG_string__sz(pCommand->aArgs[0u])) == 0)
		{
			pOption = u0111__options + uIndex;
			break;
		}
	}
	if (pOption == NULL)
	{
		SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "Unknown option: %s", SG_string__sz(pCommand->aArgs[0u])));
	}

	// parse the option's value
	SG_ERR_CHECK(  pOption->fParser(pCtx, SG_string__sz(pCommand->aArgs[1u]), &oValue)  );

	// add the value to our options
	SG_ERR_CHECK(  SG_vhash__addcopy__variant(pCtx, pTestData->pOptions, SG_string__sz(pCommand->aArgs[0u]), &oValue)  );

fail:
	return;
}

static void u0111__do_command__import__error(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// parse the argument into our expected error code
	SG_ASSERT(sizeof(SG_error) == sizeof(SG_uint64));
	SG_ERR_CHECK(  SG_uint64__parse__strict(pCtx, &pTestData->eResult, SG_string__sz(pCommand->aArgs[0u]))  );

fail:
	return;
}

static void u0111__do_command__import__shared_user(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// add the user to the list of shared users
	if (pTestData->pSharedUsers == NULL)
	{
		SG_VHASH__ALLOC(pCtx, &pTestData->pSharedUsers);
	}
	SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pTestData->pSharedUsers, SG_string__sz(pCommand->aArgs[0u]))  );

fail:
	return;
}

static void u0111__do_command__commit(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_vhash*  pCommit = NULL;
	SG_string* sName   = pCommand->aArgs[0u];

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// allocate a test-wide hash to store this commit's data
	SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pTestData->pCommits, SG_string__sz(sName), &pCommit)  );

	// store the commit's name in its new hash
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pCommit, u0111__commit_key__name, SG_string__sz(sName))  );

	// run the commit block
	VERIFY_ERR_CHECK(  u0111__run_block(pCtx, pFile, u0111__gaCommitCommands, SG_NrElements(u0111__gaCommitCommands), pTestData, (void*)pCommit)  );

fail:
	return;
}

static void u0111__do_command__commit__user(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_vhash* pCommit = (SG_vhash*)pVoidData;

	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pTestData);
	SG_NULLARGCHECK(pVoidData);

	// first argument is the username, add it to the current commit
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pCommit, u0111__commit_key__user, SG_string__sz(pCommand->aArgs[0u]))  );

fail:
	return;
}

static void u0111__do_command__commit__time(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_vhash* pCommit = (SG_vhash*)pVoidData;
	SG_int64  iTime   = 0;

	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pTestData);
	SG_NULLARGCHECK(pVoidData);

	// first argument is the time
	// parse it into an actual timestamp and store it in the current commit
	SG_ERR_CHECK(  SG_int64__parse(pCtx, &iTime, SG_string__sz(pCommand->aArgs[0u]))  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pCommit, u0111__commit_key__time, iTime)  );

fail:
	return;
}

static void u0111__do_command__commit__parent(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_vhash*  pCommit  = (SG_vhash*)pVoidData;
	SG_varray* pParents = NULL;

	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pTestData);
	SG_NULLARGCHECK(pVoidData);

	// if the current commit doesn't have parents yet, start a list
	SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pCommit, u0111__commit_key__parents, &pParents)  );
	if (pParents == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pCommit, u0111__commit_key__parents, &pParents)  );
	}

	// first argument is a parent commit, add it to the current commit's list
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pParents, SG_string__sz(pCommand->aArgs[0u]))  );

fail:
	return;
}

static void u0111__do_command__commit__comment(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_vhash*  pCommit   = (SG_vhash*)pVoidData;
	SG_varray* pComments = NULL;

	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pTestData);
	SG_NULLARGCHECK(pVoidData);

	// if the current commit doesn't have comments yet, start a list
	SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pCommit, u0111__commit_key__comments, &pComments)  );
	if (pComments == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pCommit, u0111__commit_key__comments, &pComments)  );
	}

	// first argument is a comment, add it to the current commit's list
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pComments, SG_string__sz(pCommand->aArgs[0u]))  );

fail:
	return;
}

static void u0111__do_command__commit__wc(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_vhash* pCommit = (SG_vhash*)pVoidData;

	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_NULLARGCHECK(pVoidData);

	// first argument is the folder of the expected working copy
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pCommit, u0111__commit_key__wc, SG_string__sz(pCommand->aArgs[0u]))  );

fail:
	return;
}

static void u0111__do_command__leaf(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string*          sName     = pCommand->aArgs[0u];
	SG_rbtree_iterator* pIterator = NULL;
	SG_bool             bIterator = SG_FALSE;
	const char*         szHid     = NULL;
	SG_bool             bMatch    = SG_FALSE;

	SG_UNUSED(pFile);
	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// look for this commit in the repo's list of leaves
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pTestData->pLeaves, &bIterator, &szHid, NULL)  );
	while (bIterator != SG_FALSE && bMatch == SG_FALSE)
	{
		VERIFY_ERR_CHECK(  u0111__match_commit(pCtx, pTestData, SG_string__sz(sName), szHid, &bMatch)  );
		if (bMatch == SG_FALSE)
		{
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, &szHid, NULL)  );
		}
	}

	VERIFYP_COND("No matching commit found for specified leaf.", bMatch != SG_FALSE, ("LeafCommitName(%s)", SG_string__sz(sName)));

	// run the leaf block
	VERIFY_ERR_CHECK(  u0111__run_block(pCtx, pFile, u0111__gaLeafCommands, SG_NrElements(u0111__gaLeafCommands), pTestData, NULL)  );

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	return;
}

static void u0111__do_command__branch(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string* sBranch   = pCommand->aArgs[0u];
	SG_vhash*  pBranches = NULL;
	SG_bool    bExists   = SG_FALSE;

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// make sure the branch exists
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pTestData->pBranches, "branches", &pBranches)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pBranches, SG_string__sz(sBranch), &bExists)  );
	VERIFYP_COND("Expected branch doesn't exist.", bExists != SG_FALSE, ("Branch(%s)", SG_string__sz(sBranch)));

	// run the branch block
	VERIFY_ERR_CHECK(  u0111__run_block(pCtx, pFile, u0111__gaBranchCommands, SG_NrElements(u0111__gaBranchCommands), pTestData, bExists == SG_FALSE ? NULL : (void*)sBranch)  );

fail:
	return;
}

static void u0111__do_command__branch__status(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string* sBranch = (SG_string*)pVoidData;

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);

	if (sBranch != NULL)
	{
		SG_variant cValue;
		SG_vhash*  pClosed = NULL;
		SG_bool    bClosed = SG_FALSE;

		// first argument is the expected status of the current branch
		// (true means open, false means closed)
		cValue.type = SG_VARIANT_TYPE_BOOL;
		cValue.v.val_bool = SG_FALSE;
		VERIFY_ERR_CHECK(  u0111__parse_value__branch_status(pCtx, SG_string__sz(pCommand->aArgs[0u]), &cValue)  );
		SG_ASSERT(cValue.type == SG_VARIANT_TYPE_BOOL);
		SG_ASSERT(cValue.v.val_bool == SG_FALSE || cValue.v.val_bool == SG_TRUE);

		// check the branch's actual status
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pTestData->pBranches, "closed", &pClosed)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pClosed, SG_string__sz(sBranch), &bClosed)  );
		SG_ASSERT(bClosed == SG_FALSE || bClosed == SG_TRUE);

		// make sure they match
		VERIFYP_COND_FAIL("Branch status doesn't match expectation.", cValue.v.val_bool != bClosed, ("Expected(%s) Actual(%s)", cValue.v.val_bool == SG_FALSE ? "closed" : "open", bClosed == SG_FALSE ? "open" : "closed"));
	}

fail:
	return;
}

static void u0111__do_command__branch__commit(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string* sBranch = (SG_string*)pVoidData;

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);

	if (sBranch != NULL)
	{
		SG_string* sCommit = pCommand->aArgs[0u];
		SG_vhash*  pHids   = NULL;
		SG_uint32  uHid    = 0u;
		SG_uint32  uHids   = 0u;
		SG_bool    bMatch  = SG_FALSE;

		// get the HIDs that this branch actually points to
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pTestData->pBranches, "branches", &pHids)  );
		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pHids, SG_string__sz(sBranch), &pHids)  );
		if (pHids != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pHids, "values", &pHids)  );

			// run through the list of HIDs
			// try to find the one whose commit is named by this command
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pHids, &uHids)  );
			for (uHid = 0u; uHid < uHids && bMatch == SG_FALSE; ++uHid)
			{
				const char* szHid = NULL;

				// check if this HID matches the named commit
				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pHids, uHid, &szHid, NULL)  );
				VERIFY_ERR_CHECK(  u0111__match_commit(pCtx, pTestData, SG_string__sz(sCommit), szHid, &bMatch)  );
			}

			// make sure we found a match
			VERIFYP_COND_FAIL("Branch's named commit doesn't match any actual heads.", bMatch != SG_FALSE, ("Branch(%s) CommitName(%s)", SG_string__sz(sBranch), SG_string__sz(sCommit)));
		}
	}

fail:
	return;
}

static void u0111__do_command__branch__wc(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string*   sBranch     = (SG_string*)pVoidData;
	SG_rev_spec* pRevision   = NULL;
	SG_pathname* pTestFolder = NULL;
	SG_pathname* pRepoFolder = NULL;

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);

	if (sBranch != NULL)
	{
		SG_bool bMatch = SG_FALSE;

		// build a rev spec containing the branch
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevision)  );
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevision, SG_string__sz(sBranch))  );

		// match the working copies
		VERIFY_ERR_CHECK(  u0111__match_wc(pCtx, pTestData, pRevision, SG_string__sz(pCommand->aArgs[0u]), SG_string__sz(sBranch), &bMatch, &pTestFolder, &pRepoFolder)  );
		VERIFYP_COND_FAIL("Branch's expected working copy doesn't match its checkout.", bMatch != SG_FALSE, ("Branch(%s) TestFolder(%s) RepoFolder(%s)", SG_string__sz(sBranch), SG_pathname__sz(pTestFolder), SG_pathname__sz(pRepoFolder)));
	}

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevision);
	SG_PATHNAME_NULLFREE(pCtx, pTestFolder);
	SG_PATHNAME_NULLFREE(pCtx, pRepoFolder);
	return;
}

static void u0111__do_command__tag(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string* sTag    = pCommand->aArgs[0u];
	SG_bool    bExists = SG_FALSE;

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);
	SG_UNUSED(pVoidData);

	// make sure the tag exists
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pTestData->pTags, SG_string__sz(sTag), &bExists, NULL)  );
	VERIFYP_COND("Expected tag doesn't exist.", bExists != SG_FALSE, ("Tag(%s)", SG_string__sz(sTag)));

	// run the tag block
	VERIFY_ERR_CHECK(  u0111__run_block(pCtx, pFile, u0111__gaTagCommands, SG_NrElements(u0111__gaTagCommands), pTestData, bExists == SG_FALSE ? NULL : (void*)sTag)  );

fail:
	return;
}

static void u0111__do_command__tag__commit(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string* sTag = (SG_string*)pVoidData;

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);

	if (sTag != NULL)
	{
		SG_string*  sCommit = pCommand->aArgs[0u];
		SG_bool     bExists = SG_FALSE;
		const char* szHid   = NULL;
		SG_bool     bMatch  = SG_FALSE;

		// get the HID that this tag actually points to and make sure it matches
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pTestData->pTags, SG_string__sz(sTag), &bExists, (void**)&szHid)  );
		VERIFYP_COND_FAIL("Current tag doesn't exist.", bExists != SG_FALSE, ("Tag(%s)", SG_string__sz(sTag)));
		VERIFY_ERR_CHECK(  u0111__match_commit(pCtx, pTestData, SG_string__sz(sCommit), szHid, &bMatch)  );
		VERIFYP_COND_FAIL("Tag's named commit doesn't match actual changeset.", bMatch != SG_FALSE, ("Tag(%s) CommitName(%s)", SG_string__sz(sTag), SG_string__sz(sCommit)));
	}

fail:
	return;
}

static void u0111__do_command__tag__wc(
	SG_context*       pCtx,
	SG_file*          pFile,
	u0111__command*   pCommand,
	u0111__test_data* pTestData,
	void*             pVoidData
	)
{
	SG_string*   sTag        = (SG_string*)pVoidData;
	SG_rev_spec* pRevision   = NULL;
	SG_pathname* pTestFolder = NULL;
	SG_pathname* pRepoFolder = NULL;

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(pCommand);
	SG_NULLARGCHECK(pTestData);

	if (sTag != NULL)
	{
		SG_bool bMatch = SG_FALSE;

		// build a rev spec containing the tag
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevision)  );
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevision, SG_string__sz(sTag))  );

		// match the working copies
		VERIFY_ERR_CHECK(  u0111__match_wc(pCtx, pTestData, pRevision, SG_string__sz(pCommand->aArgs[0u]), SG_string__sz(sTag), &bMatch, &pTestFolder, &pRepoFolder)  );
		VERIFYP_COND_FAIL("Tag's expected working copy doesn't match its checkout.", bMatch != SG_FALSE, ("Tag(%s) TestFolder(%s) RepoFolder(%s)", SG_string__sz(sTag), SG_pathname__sz(pTestFolder), SG_pathname__sz(pRepoFolder)));
	}

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevision);
	SG_PATHNAME_NULLFREE(pCtx, pTestFolder);
	SG_PATHNAME_NULLFREE(pCtx, pRepoFolder);
	return;
}


/*
**
** MAIN
**
*/

TEST_MAIN(u0111_main)
{
	char*        szRunId   = NULL;
	SG_pathname* pTempPath = NULL;

	TEMPLATE_MAIN_START;

	// generate a base temp path for this execution of the tests
	VERIFY_ERR_CHECK(  SG_tid__alloc2(pCtx, &szRunId, 8)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pTempPath, "u0111")  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pTempPath, szRunId)  );
	SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pTempPath)  );

	// find the folder with our test data and run every test file in it
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pDataDir, "u0111_fast_import_data")  );
	VERIFY_ERR_CHECK(  u0111__run_test_files(pCtx, pDataDir, pTempPath, SG_FALSE)  );
	// TODO: figure out how to determine if we can run long tests or not

fail:
	SG_NULLFREE(pCtx, szRunId);
	SG_PATHNAME_NULLFREE(pCtx, pTempPath);
	TEMPLATE_MAIN_END;
}
