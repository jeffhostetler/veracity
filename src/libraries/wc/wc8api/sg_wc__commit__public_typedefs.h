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

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC__COMMIT__PUBLIC_TYPEDEFS_H
#define H_SG_WC__COMMIT__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct SG_wc_commit_args
{
	// bDetached:
	// 
	// if TRUE:
	//      [] we require that the WD ***NOT*** be ATTACHED to any branch.
	//      [] the CSET that we create will NOT be ASSOCIATED with a branch.
	// if FALSE:
	//      [] a normal ATTACHED WD will create a new ASSOCIATED CSET.
	//      [] a non-ATTACHED WD will create a new un-ASSOCIATED CSET.
	// the WD is not attached to a branch.

	SG_bool				bDetached;

	// pszWho:
	//
	// Optional.
	//
	// If set: we lookup the given USER_NAME or USER_ID and
	//      try to attribute the new CSET to that user.
	//
	// If not set: we default to the current user.
	//
	// NOTE: This option is mainly intended for IMPORT.

	const char *				pszUser;

	// pszWhen:
	//
	// The date to set on the commit.  When NULL, we set
	// it to the current time.  This may be a formatted
	// date or a time-ms-since-epoch.
	//
	// NOTE: This option is mainly intended for IMPORT.

	const char *				pszWhen;

	// pszMessage:  The commit message.
	// pfnPrompt:   Interactive routine to call if message omitted.

	const char *				pszMessage;
	SG_ut__prompt_for_comment__callback * pfnPrompt;

	// psaInputs:  OPTIONAL list of <absolute-path>,
	//             <relative-path>, or <repo-path> of
	//             the items to commit.
	//
	//             ONLY NEEDED WHEN YOU WANT TO DO A PARTIAL COMMIT.
	//             When NULL (or length zero) we do a FULL commit.

	const SG_stringarray *		psaInputs;

	// depth:      OPTIONAL limit for directories in a
	//             partial commit.
	//
	//             Depth should be MAX_INT most of the time.
	//             Depth makes no sense when no inputs given.

	SG_uint32			depth;

	// psaAssocs:  OPTIONAL list of associated WIT items.

	SG_stringarray *	psaAssocs;

	// bAllowLost:
	//
	// Optionally allow COMMITS (or PARTIAL-COMMITS) when
	// there is a LOST item (or a LOST item in the subset
	// being committed).
	//
	// For a LOST item, we assume that the {file/symlink
	// content, attrbits} are clean.  For a lost
	// directory we assume whatever content we still have
	// info on.  We try to respect MOVES/RENAMES.

	SG_bool			bAllowLost;

	// psaStamps:
	//
	// OPTIONAL set of stamps to apply to the new changeset.

	SG_stringarray *	psaStamps;
};

typedef struct SG_wc_commit_args SG_wc_commit_args;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC__COMMIT__PUBLIC_TYPEDEFS_H
