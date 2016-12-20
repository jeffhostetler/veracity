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

#ifndef H_SG_VV2__PUBLIC_TYPEDEFS_H
#define H_SG_VV2__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// Prefixes/Domains 0 and 1 are used for historical status/diffs
// when reporting repo-paths.  However, we still use A/B.

#define SG_VV2__REPO_PATH_PREFIX__0				"@0/"
#define SG_VV2__REPO_PATH_PREFIX__1				"@1/"

#define SG_VV2__REPO_PATH_DOMAIN__0				'0'
#define SG_VV2__REPO_PATH_DOMAIN__1				'1'

//////////////////////////////////////////////////////////////////

#include "vv8api/sg_vv2__api__public_typedefs.h"

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV2__PUBLIC_TYPEDEFS_H
