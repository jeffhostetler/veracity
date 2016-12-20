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

#ifndef H_SG_FILE_SPEC__ACCUMULATE_COLLAPSED_H
#define H_SG_FILE_SPEC__ACCUMULATE_COLLAPSED_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * We use this as a crude stack to keep track of include-files
 * we are currently visiting.
 */
struct _acc_data
{
	const SG_pathname * pPathWorkingDirTop;		// only set if we have a pendingtree;    we do not own this
	SG_repo * pRepo;							// we do not own this
	SG_vhash * pvhAccumulator;					// map[ char * pattern ];		we do not own this
	SG_pathname * pPathCurrentFile;				// only set when in a "<pathname" include-file;  we own this
};

typedef struct _acc_data _acc_data;

//////////////////////////////////////////////////////////////////

static void _apply_file_to_accumulator(SG_context * pCtx,
									   _acc_data * p_acc_data,
									   const char * pszFilename);
static void _apply_pattern_to_accumulator(SG_context * pCtx,
										  _acc_data * p_acc_data,
										  const char * pszPattern);
static void _apply_varray_to_accumulator(SG_context * pCtx,
										 _acc_data * p_acc_data,
										 SG_varray * pvaValues);
static void _apply_scope_to_accumulator(SG_context * pCtx,
										_acc_data * p_acc_data,
										SG_vhash * pvhScopes,
										const char * pszScopeKey,
										const char ** aszBuiltinPatterns);

//////////////////////////////////////////////////////////////////

static void _apply_file_to_accumulator(SG_context * pCtx,
									   _acc_data * p_acc_data,
									   const char * pszFilename)
{
	SG_varray * pvaValues = NULL;
	_acc_data acc_data;

	acc_data.pPathWorkingDirTop = p_acc_data->pPathWorkingDirTop;
	acc_data.pRepo              = p_acc_data->pRepo;
	acc_data.pvhAccumulator     = p_acc_data->pvhAccumulator;
	acc_data.pPathCurrentFile   = NULL;

	if (pszFilename[0] == '@')
	{
		if (!acc_data.pPathWorkingDirTop)
		{
			// if we weren't given a pendingtree, repo-paths
			// don't make much sense.  quietly ignore it.
#if TRACE_SPEC
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "_apply_file_to_accumulator: omitting '%s' because WD top not set\n",
									   pszFilename)  );
#endif
			goto fail;
		}
		
		SG_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx,
																			 acc_data.pPathWorkingDirTop,
																			 pszFilename,
																			 &acc_data.pPathCurrentFile)  );
	}
	else
	{
		// non-repo-paths should be absolute for this to be consisent
		// (independent of their CWD).  we could check for a relative
		// path and make it relative to the parent directory of the
		// containing include-file, but i'm not sure that's any better.

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &acc_data.pPathCurrentFile, pszFilename)  );
	}
	
	// if file not found or cannot be parsed, ignore it quietly.

	SG_ERR_IGNORE(  _sg_file_spec__parse_file(pCtx, acc_data.pPathCurrentFile, &pvaValues)  );
	if (pvaValues)
		SG_ERR_CHECK(  _apply_varray_to_accumulator(pCtx, &acc_data, pvaValues)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, acc_data.pPathCurrentFile);
	SG_VARRAY_NULLFREE(pCtx, pvaValues);
}

static void _apply_pattern_to_accumulator(SG_context * pCtx,
										  _acc_data * p_acc_data,
										  const char * pszPattern)
{
	// if pattern starts with '<' it indicates a CPP-style file-include;
	// parse the file and recurse and accumulate.
	//
	//      special case "<@/" indicate a repo-path. do the right thing.
	//      to use this implies that we have a pendingtree hanging around.
	//
	//      for any file-include, we silently go on if the file cannot
	//      be found.
	//
	// if pattern starts with '-' it indicates an unignore of a previously
	// seen pattern.  remove the (exact match) pattern from the accumulator
	// if it exists.
	//
	//      special case of pattern starting with "\-" because our getopt
	//      is stupid and we don't have any way of passing the rest of
	//      the line args as data (like when a command uses "--").
	//      see sg_filetool.c for additional info on this hack.
	//
	// if no special first char, add (uniquely) the pattern to the accumulator.

	switch (*pszPattern)
	{
	case '<':
		if (pszPattern[1])		// expect "<pattern"; disregard blank pattern
			SG_ERR_CHECK(  _apply_file_to_accumulator(pCtx, p_acc_data, &pszPattern[1])  );
		return;

	case '-':
		if (pszPattern[1])		// expect "-pattern"; disregard simple "-" and SG_ERR_VHASH_KEYNOTFOUND
			SG_ERR_IGNORE(  SG_vhash__remove(pCtx, p_acc_data->pvhAccumulator, &pszPattern[1])  );
		return;

	case '\\':
		if (pszPattern[1])		// expect "\-pattern"; (recurse to eat any number of leading backslashes)
			SG_ERR_CHECK(  _apply_pattern_to_accumulator(pCtx, p_acc_data, &pszPattern[1])  );
		return;

	default:
		SG_ERR_CHECK(  SG_vhash__update__null(pCtx, p_acc_data->pvhAccumulator, pszPattern)  );
		return;
	}

fail:
	return;
}

static void _apply_varray_to_accumulator(SG_context * pCtx,
										 _acc_data * p_acc_data,
										 SG_varray * pvaValues)
{
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaValues, &count)  );
	for (k=0; k<count; k++)
	{
		const char * pszValue_k;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaValues, k, &pszValue_k)  );
		SG_ERR_CHECK(  _apply_pattern_to_accumulator(pCtx, p_acc_data, pszValue_k)  );
	}

fail:
	return;
}

static void _apply_scope_to_accumulator(SG_context * pCtx,
										_acc_data * p_acc_data,
										SG_vhash * pvhScopes,
										const char * pszScopeKey,
										const char ** aszBuiltinPatterns)	// optional, only used when no /scope values
{
	SG_bool bScopePresent;

	// interpret the contents of the varray at the requested scope
	// or the builtin values and apply them to the accumulator.

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhScopes, pszScopeKey, &bScopePresent)  );
	if (bScopePresent)
	{
		SG_varray * pvaValues;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvhScopes, pszScopeKey, &pvaValues)  );
		SG_ERR_CHECK(  _apply_varray_to_accumulator(pCtx, p_acc_data, pvaValues)  );
	}
	else if (aszBuiltinPatterns)
	{
		SG_uint32 k = 0;

		while (aszBuiltinPatterns[k])
			SG_ERR_CHECK(  _apply_pattern_to_accumulator(pCtx, p_acc_data, aszBuiltinPatterns[k++])  );
	}
	else
	{
		// nothing to do for this scope
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Optional builtin values to be applied at each scope level.
 */
struct _accumulate_collapsed__builtins
{
	const char ** paszDefaults;
	const char ** paszMachine;
	const char ** paszAdmin;
	const char ** paszRepo;
	const char ** paszInstance;
};

typedef struct _accumulate_collapsed__builtins _accumulate_collapsed__builtins;

/**
 * Collect the config settings at each scope level (with optional
 * builtin defaults at each level) and collapse them together.
 *
 * This handles the full "add", "-remove", and "<includefile" syntax.
 */
static void _accumulate_collapsed(SG_context * pCtx,
								  SG_file_spec * pThis,
								  SG_repo * pRepo,
								  const SG_pathname * pPathWorkingDirTop,
								  SG_file_spec__pattern_type eType,
								  SG_uint32 uFlags,
								  const char * pszSettingName,
								  _accumulate_collapsed__builtins * pBuiltins)
{
	SG_vhash * pvhScopes = NULL;				// map[ "/scope" ==> varray[ char * pattern ] ]
	SG_vhash * pvhAccumulatorAllocated = NULL;	// map[ char * pattern ]
	SG_uint32 k, count;
	_acc_data acc_data;

	SG_NULLARGCHECK(pThis);

	// populate acc_data

	acc_data.pRepo = pRepo;
	acc_data.pPathWorkingDirTop = pPathWorkingDirTop;
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhAccumulatorAllocated)  );
	acc_data.pvhAccumulator = pvhAccumulatorAllocated;
	acc_data.pPathCurrentFile = NULL;

	// get a VHASH (keyed by "/<scope>") where each value is
	// a VARRAY of PATTERNS defined at that scope.

	SG_ERR_CHECK(  SG_localsettings__get_varray_at_each_scope(pCtx, pszSettingName, acc_data.pRepo, &pvhScopes)  );
#if TRACE_SPEC
	SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console__named(pCtx, pvhScopes, "_accumulate_collapsed: Scopes")  );
#endif

	// build a VHASH (keyed by PATTERN) of all of the PATTERNS
	// that should be in the final collapsed set.  This allows
	// us to quickly de-dup/remove.  We accumulate patterns into
	// it in priority order.

	SG_ERR_CHECK(  _apply_scope_to_accumulator(pCtx, &acc_data, pvhScopes, SG_LOCALSETTING__SCOPE__DEFAULT,  ((pBuiltins) ? pBuiltins->paszDefaults : NULL))  );
	SG_ERR_CHECK(  _apply_scope_to_accumulator(pCtx, &acc_data, pvhScopes, SG_LOCALSETTING__SCOPE__MACHINE,  ((pBuiltins) ? pBuiltins->paszMachine  : NULL))  );
	SG_ERR_CHECK(  _apply_scope_to_accumulator(pCtx, &acc_data, pvhScopes, SG_LOCALSETTING__SCOPE__ADMIN,    ((pBuiltins) ? pBuiltins->paszAdmin    : NULL))  );
	SG_ERR_CHECK(  _apply_scope_to_accumulator(pCtx, &acc_data, pvhScopes, SG_LOCALSETTING__SCOPE__REPO,     ((pBuiltins) ? pBuiltins->paszRepo     : NULL))  );
	SG_ERR_CHECK(  _apply_scope_to_accumulator(pCtx, &acc_data, pvhScopes, SG_LOCALSETTING__SCOPE__INSTANCE, ((pBuiltins) ? pBuiltins->paszInstance : NULL))  );

	// after all add/remove/scope processing, use the (keys in the)
	// VHASH of patterns to build the acutal set of ignores in the
	// filespec.

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhAccumulatorAllocated, &count)  );
	for (k=0; k<count; k++)
	{
		const char * pszKey_k;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhAccumulatorAllocated, k, &pszKey_k, NULL)  );
		SG_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pThis, eType, pszKey_k, uFlags)  );
	}
	
#if TRACE_SPEC
	SG_ERR_CHECK(  SG_file_spec_debug__dump_to_console(pCtx, pThis, "_accumulate_collapsed: At Bottom")  );
#endif

fail:
	SG_VHASH_NULLFREE(pCtx, pvhScopes);
	SG_VHASH_NULLFREE(pCtx, pvhAccumulatorAllocated);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_FILE_SPEC__ACCUMULATE_COLLAPSED_H
