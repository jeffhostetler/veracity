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

#ifndef H_SG_VV2__API__PUBLIC_TYPEDEFS_H
#define H_SG_VV2__API__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct _sg_vv2_history_args
{
	const char *			pszRepoDescriptorName;
	const SG_stringarray *	psaSrcArgs;
	const SG_rev_spec *		pRevSpec;
	const SG_rev_spec *		pRevSpec_single_revisions;
	const char *			pszUser;
	const char *			pszStamp;
	SG_int64				nFromDate;
	SG_int64				nToDate;
	SG_uint32				nResultLimit;
	SG_bool					bHideObjectMerges;
	SG_bool					bLeaves;
	SG_bool					bReverse;
	SG_bool					bListAll;
};

typedef struct _sg_vv2_history_args SG_vv2_history_args;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV2__API__PUBLIC_TYPEDEFS_H
