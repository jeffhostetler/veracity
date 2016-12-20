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

#ifndef H_SG_FILE_SPEC__LOAD_IGNORES_H
#define H_SG_FILE_SPEC__LOAD_IGNORES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

const char * gaszIgnores_Repo[] = { "<@/.vvignores", "<@/.sgignores", NULL };

static _accumulate_collapsed__builtins gacbIgnores = {
	NULL,
	NULL,
	NULL,
	gaszIgnores_Repo,
	NULL
};

//////////////////////////////////////////////////////////////////

void SG_file_spec__load_ignores(SG_context * pCtx,
								SG_file_spec * pThis,
								SG_repo * pRepo,
								const SG_pathname * pPathWorkingDirTop,
								SG_uint32 uFlags)
{
	// ignores always match anywhere
	uFlags |= SG_FILE_SPEC__MATCH_ANYWHERE;

	SG_ERR_CHECK(  _accumulate_collapsed(pCtx,
										 pThis,
										 pRepo,
										 pPathWorkingDirTop,
										 SG_FILE_SPEC__PATTERN__IGNORE,
										 uFlags,
										 SG_LOCALSETTING__IGNORES,
										 &gacbIgnores)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_FILE_SPEC__LOAD_IGNORES_H
