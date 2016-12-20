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

#ifndef H_SG_WC__UNDO_DELETE__PUBLIC_TYPEDEFS_H
#define H_SG_WC__UNDO_DELETE__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct SG_wc_undo_delete_args
{
	SG_int64 attrbits;
	const char * pszHidBlob;		// not used for directories
	const SG_string * pStringRepoPathTempFile;	// used for 'merge3' type values in resolve
};

typedef struct SG_wc_undo_delete_args SG_wc_undo_delete_args;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC__UNDO_DELETE__PUBLIC_TYPEDEFS_H
