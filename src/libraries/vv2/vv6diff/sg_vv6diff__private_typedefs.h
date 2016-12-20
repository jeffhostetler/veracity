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

#ifndef H_SG_VV6DIFF__PRIVATE_TYPEDEFS_H
#define H_SG_VV6DIFF__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct _sg_vv6diff__diff_to_stream_data
{
	SG_repo *			pRepo;
	SG_pathname *		pPathSessionTempDir;

	const char *		pszTool;		// name of file-tool to override suffix-based computed defaults.
	const char *		pszDiffToolContext;		// see SG_DIFFTOOL__CONTEXT__ "console" or "gui"

	const char *		pszSubsectionLeft;
	const char *		pszSubsectionRight;

	SG_vhash *			pvhResultCodes;

	SG_bool				bInteractive;	// when GUI
};

typedef struct _sg_vv6diff__diff_to_stream_data      sg_vv6diff__diff_to_stream_data;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV6DIFF__PRIVATE_TYPEDEFS_H
