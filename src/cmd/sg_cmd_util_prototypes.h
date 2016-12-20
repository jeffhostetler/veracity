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
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if the cwd's not inside a working copy.
 */
void SG_cmd_util__get_descriptor_name_from_cwd(SG_context* pCtx, SG_string** ppstrDescriptorName, SG_pathname** ppPathCwd);

/**
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if the cwd's not inside a working copy.
 */
void SG_cmd_util__get_repo_from_cwd(SG_context* pCtx, SG_repo** ppRepo, SG_pathname** ppPathCwd);

/**
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if no --repo was specified and the cwd's not inside a working copy.
 */
void SG_cmd_util__get_descriptor_name_from_options_or_cwd(
	SG_context* pCtx, 
	SG_option_state* pOptSt, 
	SG_string** ppstrDescriptorName,
	SG_pathname** ppPathCwd);

/**
 * "Throws" SG_ERR_NOT_A_WORKING_COPY if no --repo was specified and the cwd's not inside a working copy.
 */
void SG_cmd_util__get_repo_from_options_or_cwd(SG_context* pCtx, SG_option_state* pOptSt, SG_repo** ppRepo, SG_pathname** ppPathCwd);



void SG_cmd_util__get_comment_from_editor(SG_context * pCtx,
										  SG_bool bForCommit,
										  const char * pszUserName,
										  const char * pszBranchName,
										  const SG_varray * pvaStatusCanonical,
										  const SG_varray * pvaDescsAssociations,
										  char ** ppszComment);

void SG_cmd_util__dump_history_results(
	SG_context * pCtx, 
	SG_console_stream cs,
	SG_history_result* pHistResult, 
	SG_vhash* pvh_pile, 
	SG_bool bShowOnlyOpenBranchNames,
	SG_bool bShowFullComments,
	SG_bool bHideRevnums);

void SG_cmd_util__dump_log(
	SG_context * pCtx, 
	SG_console_stream cs,
	SG_repo* pRepo, 
	const char* psz_hid_cs, 
	SG_vhash* pvhCleanPileOfBranches, 
	SG_bool bShowOnlyOpenBranchNames,
	SG_bool bShowFullComments);

void SG_cmd_util__dump_log__revspec(
	SG_context * pCtx, 
	SG_console_stream cs,
	SG_repo* pRepo, 
	SG_rev_spec* pRevSpec, 
	SG_vhash* pvhCleanPileOfBranches, 
	SG_bool bShowOnlyOpenBranchNames,
	SG_bool bShowFullComments);


/**
 * Gets the 'whoami' username. szRepoName can be a descriptor name,
 * a remote repo url, or NULL. If NULL, we look up the 'whoami'
 * based on the repo associated with the cwd.
 * 
 * If no 'whoami' is set for the repo, we leave *ppUsername untouched
 * and do not return an error.
 */
void SG_cmd_util__get_username_for_repo(
	SG_context *pCtx,
	const char *szRepoName,
	char **ppUsername // Caller owns the result.
	);

/**
 * Gets username and password.
 */
void SG_cmd_util__get_username_and_password(
	SG_context *pCtx,
	const char *szWhoami,   // [in] Default username. (Optional)
	SG_bool force_whoami,   // [in] SG_TRUE to surpress prompt for username.
	SG_bool bHasSavedCredential,  // [in] Was there a previously remembered username/password
	SG_uint32 kAttempt,           // [in] How many times have we asked them
	SG_string **ppUsername, // [out] Caller owns the result.
	SG_string **ppPassword  // [out] Caller owns the result.
	);

/**
 * Stores the user credentials in the system keyring if available.
 */
void SG_cmd_util__set_password(SG_context * pCtx,
							   const char * pszSrcRepoSpec,
							   SG_string * pStringUsername,
							   SG_string * pStringPassword);

void sg_report_addremove_journal(SG_context * pCtx, const SG_varray * pva,
								 SG_bool bAdded, SG_bool bRemoved,
								 SG_bool bVerbose);
