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

#ifndef H_SG_WC__UPDATE__PUBLIC_TYPEDEFS_H
#define H_SG_WC__UPDATE__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct SG_wc_update_args
{
	const SG_rev_spec *		pRevSpec;	// optional: -r/-b/-t goal arg

	// we allow at most one of: --attach, --attach-new, --detach, --attach-current
	const char *			pszAttach;		// optional: --attach branch
	const char *			pszAttachNew;	// optional: --attach-new branch
	SG_bool					bDetached;		// optional: --detach
	SG_bool					bAttachCurrent; // optional: --attach-current

	// args for passing to inner-merge
	SG_bool					bNoAutoMerge;
	SG_bool					bDisallowWhenDirty;
};

typedef struct SG_wc_update_args SG_wc_update_args;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC__UPDATE__PUBLIC_TYPEDEFS_H
